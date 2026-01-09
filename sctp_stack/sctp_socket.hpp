#ifndef SCTP_SOCKET_HPP
#define SCTP_SOCKET_HPP

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string_view>
#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include "sctp.hpp"
#include "sctp_association.hpp"

struct Deliverable {
    Association_Key location;
    SCTP_Packet packet;
}; 

class SCTP_Socket {
    public:
        SCTP_Socket(); 
        ~SCTP_Socket();
    
    public:
        bool sctp_bind(std::string_view ip_address, int port);
        bool sctp_run();
        Association_Key sctp_associate(std::string_view ip_address, int port); // Adds a new association object to the map and returns the association id
        int await_established_association(const Association_Key& association_id, int timeout_ms);
        void sctp_send_data(const sockaddr_in& association_id, const std::vector<uint8_t>& data);
        void sctp_send_data(const Association_Key& association_id, const std::vector<uint8_t>& data);
        size_t sctp_recv_data(std::vector<uint8_t>& buffer);
        size_t sctp_recv_data_from(const sockaddr_in& association_id, std::vector<uint8_t>& buffer);
        size_t sctp_recv_data_from(const Association_Key& association_id, std::vector<uint8_t>& buffer);
        Association_Key get_this_association_key();

    private:
        bool running;
        int receive_buffer_size;
        sockaddr_in local_address;
        SOCKET udp_socket;
        std::unordered_map<Association_Key, Association, Association_Hash> associations; // Peer verification tag as the key
        std::mutex associations_mutex;
        std::queue<Deliverable> sending_queue;
        std::mutex sending_queue_mutex;
        std::thread event_loop_thread;

        void event_loop();
        Association init_new_association(const Association_Key& key);
        void handle_send_packet(const Deliverable& deliverable);
        void handle_recv_packet(const uint8_t* data, size_t n, const sockaddr_in& src);
        void read_ooo_buffer(Association& assoc, uint32_t& tsn);

        void handle_init(const SCTP_Common_Header& header, const SCTP_Chunk& chunk, const sockaddr_in& src);
        void handle_init_ack(const SCTP_Common_Header& header, const SCTP_Chunk& chunk, const sockaddr_in& src);
        void handle_cookie_echo(const SCTP_Common_Header& header, const SCTP_Chunk& chunk, const sockaddr_in& src);
        void handle_cookie_ack(const SCTP_Common_Header& header, const SCTP_Chunk& chunk, const sockaddr_in& src);
        void handle_data(const SCTP_Common_Header& header, const SCTP_Chunk& chunk, const sockaddr_in& src);
};

#endif