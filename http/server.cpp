#include "server.hpp"
#include "../sctp_stack/sctp_socket.hpp"
#include "http_response.hpp"
#include "http_parse.hpp"
#include <string_view>
#include <stdexcept>

Server::Server(std::string_view ip, int p) : ip_address(ip), port(p), running(false), socket() {
    socket.sctp_bind(ip, port);
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (!socket.sctp_run()) {
        socket.sctp_close();
        throw std::runtime_error("Failed to start SCTP socket");
    }
    running = true;
    processor_thread = std::thread(&Server::process_requests, this);
}

void Server::process_requests() {
    while(running) {
        std::vector<uint8_t> recv_buffer(8192);
        size_t received = socket.sctp_recv_data(recv_buffer);
        if (received > 0) {
            recv_buffer.resize(received);
            auto request_opt = parse_http_request(recv_buffer);
            if (!request_opt) {
                continue; 
            }
            Request request = *request_opt;

            auto route_match = match_route(request.request_line.uri);
            Response response;
            if (route_match) {
                Route* route = route_match->first;
                auto& params = route_match->second;
                response = route->handler(request, params);
            } else {
                response = create_response(Status_Code::NotFound, std::vector<uint8_t>{});
            }

            std::vector<uint8_t> serialized_response = serialize_response(response);
            socket.sctp_send_data(socket.get_this_association_key(), serialized_response);
        }
    }
}

void Server::stop() {
    socket.sctp_close();
    running = false;
    if (processor_thread.joinable()) {
        processor_thread.join();
    }
}

void Server::register_route(const std::string& pattern, std::function<Response(const Request&, const std::unordered_map<std::string, std::string>&)> handler) {
    Route new_route;
    new_route.pattern = pattern;
    new_route.handler = handler;

    std::string regex_pattern;
    std::vector<std::string> params;
    size_t pos = 0;
    while (pos < pattern.size()) {
        if (pattern[pos] == ':') {
            size_t end_pos = pattern.find('/', pos);
            if (end_pos == std::string::npos) {
                end_pos = pattern.size();
            }
            std::string param_name = pattern.substr(pos + 1, end_pos - pos - 1);
            params.push_back(param_name);
            regex_pattern += "([^/]+)";
            pos = end_pos;
        } else {
            if (std::ispunct(pattern[pos])) {
                regex_pattern += '\\';
            }
            regex_pattern += pattern[pos];
            pos++;
        }
    }
    new_route.regex = std::regex("^" + regex_pattern + "$");
    new_route.params = params;

    routes.push_back(new_route);
}

std::optional<std::pair<Route*, std::unordered_map<std::string, std::string>>> Server::match_route(const std::string& uri) {
    for (auto& route : routes) {
        std::smatch match;
        if (std::regex_match(uri, match, route.regex)) {
            std::unordered_map<std::string, std::string> param_map;
            for (size_t i = 0; i < route.params.size(); i++) {
                param_map[route.params[i]] = match[i + 1];
            }
            return std::make_pair(&route, param_map);
        }
    }
    return std::nullopt;
}