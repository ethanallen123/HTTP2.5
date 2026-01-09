#ifndef HTTP_PARSE_HPP
#define HTTP_PARSE_HPP

#include "http_request.hpp"
#include <vector>
#include <string_view>
#include <optional>
#include <tuple>
#include <unordered_map>

constexpr std::string_view SEPERATOR = "\r\n";

std::optional<Request> parse_http_request(const std::vector<uint8_t>& raw_request);
std::optional<std::tuple<Request_Line, size_t>> parse_request_line(std::string_view raw_request_line);
std::optional<std::tuple<std::unordered_map<std::string, std::string>, size_t>> parse_request_headers(std::string_view raw_request_line, const size_t& start_pos);
std::optional<std::tuple<std::vector<uint8_t>, size_t>> parse_request_body(std::string_view raw_request_line, const size_t& start_pos, const size_t& content_length);

#endif