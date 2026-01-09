#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct Request_Line {
    std::string version;
    std::string uri;
    std::string method;
};

struct Request {
    Request_Line request_line;
    std::unordered_map<std::string, std::string> headers;
    std::vector<uint8_t> body;
};


#endif