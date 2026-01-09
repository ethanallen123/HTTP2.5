#ifndef HTTP_PARSE_HPP
#define HTTP_PARSE_HPP

#include "http/http_request.hpp"
#include <vector>

Request parse_http_request(const std::vector<uint8_t>& raw_request);

#endif