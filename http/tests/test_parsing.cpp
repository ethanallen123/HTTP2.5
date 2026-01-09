#include "tests.hpp"
#include "../http_parse.hpp"
#include <iostream>

void test_parse_request_line() {
    std::cout << "Testing parse_request_line:" << std::endl;
    std::string_view raw_request_line = "GET /index.html HTTP/2.5\r\n";
    auto result = parse_request_line(raw_request_line);
    if (result) {
        auto [request_line, next_pos] = *result;
        std::cout << "Method: " << request_line.method << "\n";
        std::cout << "URI: " << request_line.uri << "\n";
        std::cout << "Version: " << request_line.version << "\n";
    } else {
        std::cout << "Failed to parse request line.\n";
    }

    std::cout << std::endl;
}

void test_parse_request_headers() {
    std::cout << "Testing parse_request_headers:" << std::endl;
    std::string_view raw_headers = "Host: example.com\r\nUser-Agent: TestAgent\r\n\r\n";
    size_t start_pos = 0;
    auto result = parse_request_headers(raw_headers, start_pos);
    if (result) {
        auto [headers, next_pos] = *result;
        for (const auto& [key, value] : headers) {
            std::cout << key << ": " << value << "\n";
        }
    } else {
        std::cout << "Failed to parse request headers.\n";
    }

    std::cout << std::endl;
}

void test_parse_request_body() {
    std::cout << "Testing parse_request_body:" << std::endl;
    std::string_view raw_body = "This is the body of the request.";
    size_t start_pos = 0;
    auto result = parse_request_body(raw_body, start_pos);  
    if (result) {
        auto [body, next_pos] = *result;
        std::string body_str(body.begin(), body.end());
        std::cout << "Body: " << body_str << "\n";
    } else {
        std::cout << "Failed to parse request body.\n";
    }

    std::cout << std::endl;
}

void test_parse_http_request() {
    std::cout << "Testing parse_http_request:" << std::endl;
    std::string raw_request = 
        "GET /index.html HTTP/2.5\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestAgent\r\n"
        "\r\n"
        "This is the body of the request.";
    std::vector<uint8_t> raw_request_vec(raw_request.begin(), raw_request.end());
    
    auto result = parse_http_request(raw_request_vec);
    if (result) {
        Request request = *result;
        std::cout << "Method: " << request.request_line.method << "\n";
        std::cout << "URI: " << request.request_line.uri << "\n";
        std::cout << "Version: " << request.request_line.version << "\n";
        for (const auto& [key, value] : request.headers) {
            std::cout << key << ": " << value << "\n";
        }
        std::string body_str(request.body.begin(), request.body.end());
        std::cout << "Body: " << body_str << "\n";
    } else {
        std::cout << "Failed to parse HTTP request.\n";
    }

    std::cout << std::endl;
}

void test_parsing() {
    test_parse_request_line();
    test_parse_request_headers();
    test_parse_request_body();
    test_parse_http_request();
}