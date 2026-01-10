#include "http_response.hpp"
#include "http_parse.hpp"

Response create_response(Status_Code status_code, const std::vector<uint8_t>& body) {
    Response response;

    response.response_line = create_response_line(status_code);
    response.headers = create_standard_headers(body);
    response.body = body;

    return response;
}

Response_Line create_response_line(const Status_Code& code) {
    Response_Line line;
    line.version = "HTTP/2.5"; 

    switch (code) {
        case OK:
            line.reason_phrase = "OK";
            break;
        case BadRequest:
            line.reason_phrase = "Bad Request";
            break;
        case NotFound:
            line.reason_phrase = "Not Found";
            break;
        case InternalServerError:
            line.reason_phrase = "Internal Server Error";
            break;
        default:
            line.reason_phrase = "Unknown Status";
            break;
    }
    line.status_code = code;
    return line;
}

std::unordered_map<std::string, std::string> create_standard_headers(const std::vector<uint8_t>& body) {
    std::unordered_map<std::string, std::string> headers;
    headers["Content-Length"] = std::to_string(body.size());
    headers["Content-Type"] = "text/plain";
    headers["Connection"] = "close";
    return headers;
}

std::vector<uint8_t> serialize_response(const Response& response) {
    std::vector<uint8_t> serialized;

    std::string status_line = response.response_line.version + " " +
                              std::to_string(response.response_line.status_code) + " " +
                              response.response_line.reason_phrase + std::string(SEPERATOR);
    serialized.insert(serialized.end(), status_line.begin(), status_line.end());

    for (const auto& [key, value] : response.headers) {
        std::string header_line = key + ": " + value + std::string(SEPERATOR);
        serialized.insert(serialized.end(), header_line.begin(), header_line.end());
    }

    serialized.insert(serialized.end(), SEPERATOR.begin(), SEPERATOR.end());

    serialized.insert(serialized.end(), response.body.begin(), response.body.end());

    return serialized;
}
