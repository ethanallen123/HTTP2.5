#include "http_parse.hpp"
#include "http_request.hpp"
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <iostream>



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

    auto body_result = parse_request_body(raw_str, pos);
    if (!body_result) {
        return std::nullopt;
    }
    request.body = std::get<0>(*body_result);

    return request;
}

std::optional<std::tuple<RequestLine, size_t>> parse_request_line(std::string_view raw_request) {
    RequestLine request_line;

    size_t raw_request_line_end = raw_request.find(SEPERATOR);
    if (raw_request_line_end == std::string_view::npos) {
        std::cout << "No CRLF found in request line.\n";
        return std::nullopt;
    } 
    std::string raw_request_line(raw_request.substr(0, raw_request_line_end));
    
    size_t method_end = raw_request_line.find(" ");
    if (method_end == std::string_view::npos) {
        std::cout << "No space found after method.\n";
        return std::nullopt;
    }
    request_line.method = std::string(raw_request_line.substr(0, method_end));
    
    size_t uri_end = raw_request_line.find(" ", method_end + 1);
    if (uri_end == std::string_view::npos) {
        std::cout << "No space found after URI.\n";
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

std::optional<std::tuple<std::map<std::string, std::string>, size_t>> parse_request_headers(std::string_view raw_request_line, size_t start_pos) {
    std::map<std::string, std::string> headers;
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
            std::cout << "Malformed header line: " << header_line << "\n";
            return std::nullopt;
        }
        std::string header_name = header_line.substr(0, colon_pos);
        std::string header_value = header_line.substr(colon_pos + 2);
        headers[header_name] = header_value;
        pos = line_end + SEPERATOR.size();
    }
    
    return std::make_tuple(headers, pos);
}

std::optional<std::tuple<std::vector<uint8_t>, size_t>> parse_request_body(std::string_view raw_request_line, size_t start_pos) {
    std::vector<uint8_t> body;
    size_t pos = start_pos;
    
    while (pos < raw_request_line.size()) {
        body.push_back(static_cast<uint8_t>(raw_request_line[pos]));
        pos++;
    }
    
    return std::make_tuple(body, pos);
}