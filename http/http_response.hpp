#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

enum Status_Code {
    OK = 200,
    BadRequest = 400,
    NotFound = 404,
    InternalServerError = 500
};

struct Response_Line {
    std::string version;
    Status_Code status_code;
    std::string reason_phrase;
};

struct Response {
    Response_Line response_line;
    std::unordered_map<std::string, std::string> headers;
    std::vector<uint8_t> body;
};

Response create_response(Status_Code status_code, const std::string& body);
std::vector<uint8_t> serialize_response(const Response& response);

#endif