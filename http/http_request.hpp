#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <cstdint>

struct RequestLine {
    std::string version;
    std::string uri;
    std::string method;
};

struct Request {
    RequestLine request_line;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
};


#endif