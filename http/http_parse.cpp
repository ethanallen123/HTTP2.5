#include "http_parse.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <cctype>

void trim(std::string& str) {
    auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base();
    str = (start < end) ? std::string(start, end) : "";
}

bool contains_valid_chars(const std::string& str) {
    for (char ch : str) {
        switch (ch) {
            case '!': case '#': case '$': case '%': case '&': case '\'': case '*':
            case '+': case '-': case '.': case '^': case '_': case '`': case '|':
            case '~':
                break;
            default:
                if (!std::isalnum(static_cast<unsigned char>(ch))) {
                    return false;
                }
        }
    }
    return true;
}

std::optional<Request> parse_http_request(const std::vector<uint8_t>& raw_request) {
    Request request;
    std::string raw_str(raw_request.begin(), raw_request.end());
    
    auto result = parse_request_line(raw_str);
    if (!result) {
        return std::nullopt;
    }
    request.request_line = std::get<0>(*result);
    size_t pos = std::get<1>(*result);
    
    auto headers_result = parse_request_headers(raw_str, pos);
    if (!headers_result) {
        return std::nullopt;
    }
    request.headers = std::get<0>(*headers_result);
    pos = std::get<1>(*headers_result);

    auto content_length_it = request.headers.find("content-length");
    if (content_length_it == request.headers.end()) {
        request.body = {};
    } else {
        auto body_result = parse_request_body(raw_str, pos, std::stoul(content_length_it->second));
        if (!body_result) {
            return std::nullopt;
        }
        request.body = std::get<0>(*body_result);
    }
    
    return request;
}

std::optional<std::tuple<Request_Line, size_t>> parse_request_line(std::string_view raw_request) {
    Request_Line request_line;

    size_t raw_request_line_end = raw_request.find(SEPERATOR);
    if (raw_request_line_end == std::string_view::npos) {
        std::cout << "Malformed request (No CRLF found in request)\n";
        return std::nullopt;
    } 
    std::string raw_request_line(raw_request.substr(0, raw_request_line_end));
    
    size_t method_end = raw_request_line.find(" ");
    if (method_end == std::string_view::npos) {
        std::cout << "Malformed request line (No space found after method): " << raw_request_line <<"\n";
        return std::nullopt;
    }
    request_line.method = std::string(raw_request_line.substr(0, method_end));
    
    size_t uri_end = raw_request_line.find(" ", method_end + 1);
    if (uri_end == std::string_view::npos) {
        std::cout << "Malformed request line (No space found after URI): " << raw_request_line << "\n";
        return std::nullopt;
    }
    request_line.uri = std::string(raw_request_line.substr(method_end + 1, uri_end - method_end - 1));

    request_line.version = std::string(raw_request_line.substr(uri_end + 1, raw_request_line_end - uri_end - 1));

    if (request_line.version != "HTTP/2.5") {
        std::cout << "Unsupported HTTP version: " << request_line.version << "\n";
        return std::nullopt;
    }
    
    return std::make_tuple(request_line, raw_request_line_end + SEPERATOR.size());
}

std::optional<std::tuple<std::unordered_map<std::string, std::string>, size_t>> parse_request_headers(std::string_view raw_request_line, const size_t& start_pos) {
    std::unordered_map<std::string, std::string> headers;
    size_t pos = start_pos;
    
    while (true) {
        size_t line_end = raw_request_line.find(SEPERATOR, pos);
        if (line_end == pos) {
            pos += SEPERATOR.size();
            break;
        }
        std::string header_line(raw_request_line.substr(pos, line_end - pos));
        size_t colon_pos = header_line.find(':');
        if (colon_pos == std::string::npos) {
            std::cout << "Malformed header line (missing colon): " << header_line << "\n";
            return std::nullopt;
        }
        if (header_line.at(colon_pos + 1) != ' ') {
            std::cout << "Malformed header line (missing space after colon): " << header_line << "\n";
            return std::nullopt;
        }

        if (header_line.at(colon_pos - 1) == ' ') {
            std::cout << "Malformed header line (cannot have space before colon): " << header_line << "\n";
            return std::nullopt;
        }

        std::string header_name = header_line.substr(0, colon_pos);
        std::transform(header_name.begin(), header_name.end(), header_name.begin(), [](unsigned char c) { return std::tolower(c); });
        trim(header_name);

        std::string header_value = header_line.substr(colon_pos + 2);
        std::transform(header_value.begin(), header_value.end(), header_value.begin(), [](unsigned char c) { return std::tolower(c); });
        trim(header_value);

        if (header_name.empty() || header_value.empty()) {
            std::cout << "Malformed header line (empty name or value): " << header_line << "\n";
            return std::nullopt;
        }
        if (!contains_valid_chars(header_name)) {
            std::cout << "Malformed header line (invalid characters): " << header_line << "\n";
            return std::nullopt;
        }

        if (headers.find(header_name) != headers.end()) {
            headers[header_name] += ", " + header_value;
        } else {
            headers[header_name] = header_value;
        }

        pos = line_end + SEPERATOR.size();
    }
    
    return std::make_tuple(headers, pos);
}

std::optional<std::tuple<std::vector<uint8_t>, size_t>> parse_request_body(std::string_view raw_request_line, const size_t& start_pos, const size_t& content_length) {
    std::vector<uint8_t> body;
    size_t pos = start_pos;
    
    for (size_t i{0}; i < content_length && pos < raw_request_line.size(); ++i) {
        body.push_back(static_cast<uint8_t>(raw_request_line[pos]));
        pos++;
    }
    
    return std::make_tuple(body, pos);
}

std::vector<uint8_t> serialize_request(const Request& request) {
    std::vector<uint8_t> serialized;

    std::string request_line = request.request_line.method + " " +
                               request.request_line.uri + " " +
                               request.request_line.version + std::string(SEPERATOR);
    serialized.insert(serialized.end(), request_line.begin(), request_line.end());

    for (const auto& [key, value] : request.headers) {
        std::string header_line = key + ": " + value + std::string(SEPERATOR);
        serialized.insert(serialized.end(), header_line.begin(), header_line.end());
    }

    serialized.insert(serialized.end(), SEPERATOR.begin(), SEPERATOR.end());

    serialized.insert(serialized.end(), request.body.begin(), request.body.end());

    return serialized;
}

// TODO: Seperate into helper functions
std::optional<Response> parse_http_response(const std::vector<uint8_t>& raw_response) {
    std::string response_str(raw_response.begin(), raw_response.end());
    
    size_t status_line_end = response_str.find("\r\n");
    if (status_line_end == std::string::npos) {
        std::cout << "Malformed response (No CRLF found in status line)\n";
        return std::nullopt;
    }
    
    std::string status_line = response_str.substr(0, status_line_end);
    
    size_t first_space = status_line.find(" ");
    size_t second_space = status_line.find(" ", first_space + 1);
    
    if (first_space == std::string::npos || second_space == std::string::npos) {
        std::cout << "Malformed status line: " << status_line << "\n";
        return std::nullopt;
    }
    
    Response response;
    response.response_line.version = status_line.substr(0, first_space);
    response.response_line.status_code = static_cast<Status_Code>(std::stoi(status_line.substr(first_space + 1, second_space - first_space - 1)));
    response.response_line.reason_phrase = status_line.substr(second_space + 1);
    
    size_t headers_end = response_str.find("\r\n\r\n", status_line_end);
    if (headers_end == std::string::npos) {
        std::cout << "Malformed response (No blank line between headers and body)\n";
        return std::nullopt;
    }
    
    size_t current_pos = status_line_end + 2;
    while (current_pos < headers_end) {
        size_t line_end = response_str.find("\r\n", current_pos);
        if (line_end == std::string::npos || line_end > headers_end) {
            break;
        }
        
        std::string header_line = response_str.substr(current_pos, line_end - current_pos);
        size_t colon_pos = header_line.find(":");
        
        if (colon_pos != std::string::npos) {
            std::string header_name = header_line.substr(0, colon_pos);
            std::string header_value = header_line.substr(colon_pos + 2); 
            response.headers[header_name] = header_value;
        }
        
        current_pos = line_end + 2;
    }
    
    size_t body_start = headers_end + 4; 
    if (body_start < raw_response.size()) {
        response.body.insert(response.body.end(), raw_response.begin() + body_start, raw_response.end());
    }
    
    return response;
}