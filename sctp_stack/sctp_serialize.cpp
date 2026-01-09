#include "sctp_serialize.hpp"
#include <cstring>
#include <stdexcept>
#include "sctp_checksum.hpp"


SCTP_Packet deserialize_sctp_packet(const uint8_t* data, size_t len) {
    SCTP_Packet out;

    /*--------------Deserializing Helpers--------------*/
    size_t offset = 0;

    auto can_read = [&](size_t read_len) {
        return offset + read_len <= len;
    };

    auto read_ptr = [&](size_t read_len) -> const uint8_t* {
        if (!can_read(read_len))
            throw std::runtime_error("buffer underflow");
        const uint8_t* p = data + offset;
        offset += read_len;
        return p;
    };
    /*--------------------------------------------*/

    std::memcpy(&out.header, read_ptr(sizeof(SCTP_Common_Header)), sizeof(SCTP_Common_Header));
    
    while(offset < len) {
        if (!can_read(sizeof(SCTP_Chunk_Header)))
            throw std::runtime_error("truncated chunk header");

        SCTP_Chunk_Header ch;
        std::memcpy(&ch, read_ptr(sizeof(ch)), sizeof(ch));

        // Use ch.length to determine body size, subtract header size
        size_t body_len = ch.length - sizeof(SCTP_Chunk_Header);
        if (!can_read(body_len))
            throw std::runtime_error("truncated chunk body");

        SCTP_Chunk chunk;
        chunk.chunk_header = ch;

        const uint8_t* body = read_ptr(body_len);
        deserialize_chunk_value(ch.type, body, body_len, chunk.chunk_value);

        out.chunks.push_back(std::move(chunk));

        // Padding to 4-byte boundary based on chunk.length
        size_t padded = (ch.length + 3) & ~3;
        size_t pad = padded - ch.length;
        if (pad > 0 && !can_read(pad))
            throw std::runtime_error("truncated padding");
        offset += pad;
    }
    return out;
}

void deserialize_chunk_value(Chunk_Type type, const uint8_t* data, size_t len, std::variant<init_chunk_value, cookie_echo_chunk_value, cookie_ack_chunk_value, data_chunk_value>& out) {
    switch (type) {
        case INIT: 
        case INIT_ACK: {
            init_chunk_value v;
            deserialize_init_chunk(data, len, v);
            out = std::move(v);
            break;
        }
        case COOKIE_ECHO: {
            cookie_echo_chunk_value v;
            deserialize_cookie_echo_chunk(data, len, v);
            out = std::move(v);
            break;
        }
        case COOKIE_ACK: {
            cookie_ack_chunk_value v;
            deserialize_cookie_ack_chunk(data, len, v);
            out = std::move(v);
            break;
        }
        case DATA: {
            data_chunk_value v;
            deserialize_data_chunk(data, len, v);
            out = std::move(v);
            break;
        }
        default:
            throw std::runtime_error("unsupported chunk type");
    }
}

void deserialize_init_chunk(const uint8_t* data, size_t len, init_chunk_value& out) {
    if (len < 16)
        throw std::runtime_error("INIT chunk too short");

    /*--------------Deserializing Helpers--------------*/
    size_t offset = 0;

    auto read32 = [&](uint32_t& v) {
        std::memcpy(&v, data + offset, 4);
        offset += 4;
    };

    auto read16 = [&](uint16_t& v) {
        std::memcpy(&v, data + offset, 2);
        offset += 2;
    };
    /*--------------------------------------------*/

    read32(out.initiate_tag);
    read32(out.a_rwnd);
    read16(out.out_streams);
    read16(out.in_streams);
    read32(out.initial_tsn);

    out.optional_parameters.assign(data + offset, data + len);
}

void deserialize_cookie_echo_chunk(const uint8_t* data, size_t len, cookie_echo_chunk_value& out) {
    out.cookie_data.assign(data, data + len);
}

void deserialize_cookie_ack_chunk(const uint8_t* data, size_t len, cookie_ack_chunk_value& out) {
    // COOKIE_ACK has no payload
    (void)data;
    (void)len;
}
void deserialize_data_chunk(const uint8_t* data, size_t len, data_chunk_value& out) {
    if (len < 12)
        throw std::runtime_error("DATA chunk too short");

    /*--------------Deserializing Helpers--------------*/
    size_t offset = 0;

    auto read32 = [&](uint32_t& v) {
        std::memcpy(&v, data + offset, 4);
        offset += 4;
    };

    auto read16 = [&](uint16_t& v) {
        std::memcpy(&v, data + offset, 2);
        offset += 2;
    };
    /*--------------------------------------------*/

    read32(out.tsn);
    read16(out.stream_identifier);
    read16(out.stream_seq_num);
    read32(out.payload_protocal);

    out.user_data.assign(data + offset, data + len);
}

std::vector<uint8_t> serialize_sctp_packet(const SCTP_Packet& pkt) {
    std::vector<uint8_t> out;

    // Write header with checksum = 0
    SCTP_Common_Header header = pkt.header;
    header.checksum = 0;
    
    const uint8_t* h = reinterpret_cast<const uint8_t*>(&header);
    out.insert(out.end(), h, h + sizeof(header));

    for (const auto& chunk : pkt.chunks) {
        serialize_chunk(chunk, out);
    }

    uint32_t checksum = calculate_sctp_checksum(out.data(), out.size());
    std::memcpy(out.data() + offsetof(SCTP_Common_Header, checksum), &checksum, sizeof(checksum));

    return out;
}

void serialize_chunk(const SCTP_Chunk& chunk, std::vector<uint8_t>& out) {
    size_t start = out.size();

    // Reserve space for header (written later)
    out.resize(start + sizeof(SCTP_Chunk_Header));

    switch (chunk.chunk_header.type) {
        case INIT:
        case INIT_ACK:
            serialize_init_chunk(std::get<init_chunk_value>(chunk.chunk_value), out);
            break;
        case COOKIE_ECHO:
            serialize_cookie_echo_chunk(std::get<cookie_echo_chunk_value>(chunk.chunk_value), out);
            break;
        case COOKIE_ACK:
            serialize_cookie_ack_chunk(std::get<cookie_ack_chunk_value>(chunk.chunk_value), out);
            break;
        case DATA:
            serialize_data_chunk(std::get<data_chunk_value>(chunk.chunk_value), out);
            break;
        default:
            throw std::runtime_error("unsupported chunk type");
    }

    // Compute length
    uint16_t length = static_cast<uint16_t>(out.size() - start);

    // Write header
    SCTP_Chunk_Header h = chunk.chunk_header;
    h.length = length;

    std::memcpy(out.data() + start, &h, sizeof(h));

    // Pad to 4-byte boundary
    size_t padded = (length + 3) & ~3;
    out.resize(start + padded, 0);
}

void serialize_init_chunk(const init_chunk_value& v, std::vector<uint8_t>& out) {
    auto append32 = [&](uint32_t x) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&x);
        out.insert(out.end(), p, p + 4);
    };

    auto append16 = [&](uint16_t x) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&x);
        out.insert(out.end(), p, p + 2);
    };

    append32(v.initiate_tag);
    append32(v.a_rwnd);
    append16(v.out_streams);
    append16(v.in_streams);
    append32(v.initial_tsn);

    out.insert(out.end(),
               v.optional_parameters.begin(),
               v.optional_parameters.end());
}

void serialize_cookie_echo_chunk(const cookie_echo_chunk_value& v, std::vector<uint8_t>& out) {
    out.insert(out.end(), v.cookie_data.begin(), v.cookie_data.end());
}

void serialize_cookie_ack_chunk(const cookie_ack_chunk_value& v, std::vector<uint8_t>& out) {
    // COOKIE_ACK has no payload
    (void)v;
}

void serialize_data_chunk(const data_chunk_value& v,std::vector<uint8_t>& out) {
    auto append32 = [&](uint32_t x) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&x);
        out.insert(out.end(), p, p + 4);
    };

    auto append16 = [&](uint16_t x) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&x);
        out.insert(out.end(), p, p + 2);
    };

    append32(v.tsn);
    append16(v.stream_identifier);
    append16(v.stream_seq_num);
    append32(v.payload_protocal);

    out.insert(out.end(),
               v.user_data.begin(),
               v.user_data.end());
}
