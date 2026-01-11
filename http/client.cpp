#include "client.hpp"
#include "http_parse.hpp"
#include <iostream>
#include <chrono>
#include <thread>

Client::Client(const std::string& ip, int p) : ip_address(ip), port(p), connected(false), socket() {
        socket.sctp_bind(ip, p);
}

Client::~Client() {
    if (connected) {
        disconnect();
    }
}

void Client::start() {
    if (!socket.sctp_run()) {
        socket.sctp_close();
        throw std::runtime_error("Failed to start SCTP socket");
    }
}

void Client::stop() {
    if (connected) {
        disconnect();
    }
    socket.sctp_close();
}

bool Client::connect(const std::string& server_ip, int server_port) {
    try {
        // Associate with the server
        Association_Key key = socket.sctp_associate(server_ip, server_port);
        
        // Wait for association to be established
        int result = socket.await_established_association(key, 5000);
        
        if (result == 0) {
            connected = true;
            server_association_key = key;
            return true;
        } else {
            std::cout << "Failed to establish SCTP association with server\n";
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "Connection error: " << e.what() << "\n";
        return false;
    }
}

void Client::disconnect() {
    if (connected) {
        socket.sctp_close();
        connected = false;
    }
}

bool Client::is_connected() const {
    return connected;
}

Request Client::build_request(const std::string& method, const std::string& uri, const std::string& body) {
    Request request;
    request.request_line.method = method;
    request.request_line.uri = uri;
    request.request_line.version = "HTTP/2.5";
    
    // Add default headers
    request.headers["Host"] = ip_address + ":" + std::to_string(port);
    request.headers["Connection"] = "close";
    request.headers["User-Agent"] = "HTTP2.5-Client/1.0";
    
    // Add body if provided
    if (!body.empty()) {
        request.headers["Content-Length"] = std::to_string(body.length());
        request.headers["Content-Type"] = "application/octet-stream";
        request.body.insert(request.body.end(), body.begin(), body.end());
    }
    
    return request;
}

std::optional<Response> Client::send_request(const Request& request) {
    if (!connected) {
        std::cout << "Client is not connected to server\n";
        return std::nullopt;
    }
    
    size_t received{0};
    try {
        while (received == 0) {
            // Serialize the request
            std::vector<uint8_t> serialized_request = serialize_request(request);
                
            // Send the request
            socket.sctp_send_data(server_association_key, serialized_request);
            
            // Receive the response
            std::vector<uint8_t> response_buffer(65536);
            received = socket.sctp_recv_data_from(server_association_key, response_buffer);
            
            if (received > 0) {
                response_buffer.resize(received);
                return parse_http_response(response_buffer);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cout << "Error sending request: " << e.what() << "\n";
        return std::nullopt;
    }
}

std::optional<Response> Client::get_request(const std::string& uri) {
    Request request = build_request("GET", uri);
    return send_request(request);
}

std::optional<Response> Client::post_request(const std::string& uri, const std::string& body) {
    Request request = build_request("POST", uri, body);
    return send_request(request);
}

std::optional<Response> Client::put_request(const std::string& uri, const std::string& body) {
    Request request = build_request("PUT", uri, body);
    return send_request(request);
}

std::optional<Response> Client::delete_request(const std::string& uri) {
    Request request = build_request("DELETE", uri);
    return send_request(request);
}
