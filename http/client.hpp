#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../sctp_stack/sctp_socket.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include <string>
#include <optional>

class Client {
    public:
        Client(const std::string& ip, int port);
        ~Client();
        
        std::optional<Response> get_request(const std::string& uri);
        std::optional<Response> post_request(const std::string& uri, const std::string& body);
        std::optional<Response> put_request(const std::string& uri, const std::string& body);
        std::optional<Response> delete_request(const std::string& uri);
        std::optional<Response> send_request(const Request& request);
        
        void start();
        void stop();
        bool connect(const std::string& server_ip, int server_port);
        void disconnect();
        bool is_connected() const;
        
    private:
        SCTP_Socket socket;
        std::string ip_address;
        int port;
        bool connected;
        Association_Key server_association_key;
        
        Request build_request(const std::string& method, const std::string& uri, const std::string& body = "");
};

#endif