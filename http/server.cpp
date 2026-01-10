#include "server.hpp"
#include "../sctp_stack/sctp_socket.hpp"
#include <string_view>
#include <stdexcept>

Server::Server(std::string_view ip_address, int p) : port(p), socket() {
    socket.sctp_bind(ip_address, port);
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (!socket.sctp_run()) {
        socket.sctp_close();
        throw std::runtime_error("Failed to start SCTP socket");
    }
}

void Server::stop() {
    socket.sctp_close();
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