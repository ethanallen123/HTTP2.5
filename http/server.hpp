#ifndef SERVER_HPP
#define SERVER_HPP

#include "../sctp_stack/sctp_socket.hpp"
#include "http_response.hpp"
#include "http_request.hpp"
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>
#include <regex>
#include <optional>

struct Route {
    std::string pattern;
    std::regex regex;
    std::vector<std::string> params;
    std::function<Response(const Request&, const std::unordered_map<std::string, std::string>&)> handler;
};

class Server {
    public:
        Server(std::string_view ip_address, int p);
        ~Server();
        void start();
        void stop();
        void register_route(const std::string& pattern, std::function<Response(const Request&, const std::unordered_map<std::string, std::string>&)> handler);
    private:
        SCTP_Socket socket;
        std::string ip_address;
        std::vector<Route> routes;
        int port;
        bool running;
        std::thread processor_thread;
        
        std::optional<std::pair<Route*, std::unordered_map<std::string, std::string>>> match_route(const std::string& uri);
        void process_requests();
};

#endif