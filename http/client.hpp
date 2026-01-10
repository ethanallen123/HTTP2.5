#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../sctp_stack/sctp_socket.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include <string>
#include <optional>

class Client {
    public:
        Client(const std::string& server_ip, int server_port);
        ~Client();
        
        // Core request methods
        std::optional<Response> get_request(const std::string& uri);
        std::optional<Response> post_request(const std::string& uri, const std::string& body);
        std::optional<Response> put_request(const std::string& uri, const std::string& body);
        std::optional<Response> delete_request(const std::string& uri);
        
        // Generic request method
        std::optional<Response> send_request(const Request& request);
        
        // Connection management
        bool connect();
        void disconnect();
        bool is_connected() const;
        
    private:
        SCTP_Socket socket;
        std::string server_ip;
        int server_port;
        bool connected;
        
        // Helper to build requests
        Request build_request(const std::string& method, const std::string& uri, const std::string& body = "");
        
        // Helper to parse responses
        std::optional<Response> parse_response(const std::vector<uint8_t>& raw_response);
};

#endif