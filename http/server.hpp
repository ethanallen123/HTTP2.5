#ifndef SERVER_HPP
#define SERVER_HPP

#include "../sctp_stack/sctp_socket.hpp"
#include <string_view>

class Server {
    public:
        Server(std::string_view ip_address, int p);
        ~Server();
        void start();
        void stop();
    private:
        SCTP_Socket socket;
        int port;
};

#endif