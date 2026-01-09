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