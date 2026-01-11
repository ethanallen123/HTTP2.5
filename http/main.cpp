#include "server.hpp"
#include "client.hpp"
#include <iostream>


int main() {
    Server server("127.0.0.1", 8080);
    server.register_route("/hello", [](const Request& req, const std::unordered_map<std::string, std::string>& params) {
        std::string body = "Hello, World!";
        return create_response(Status_Code::OK, std::vector<uint8_t>(body.begin(), body.end()));
    });
    server.start();

    Client client("127.0.0.1", 5000);
    client.start();
    client.connect("127.0.0.1", 8080);

    auto response_opt = client.get_request("/hello");
    if (response_opt) {
        Response response = *response_opt;
        std::string response_body(response.body.begin(), response.body.end());
        std::cout << "Response Body: " << response_body << "\n";
    } else {
        std::cout << "Failed to get response from server\n";
    }
    return 0;
}