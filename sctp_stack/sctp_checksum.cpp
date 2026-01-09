#include "sctp_checksum.hpp"
#include <cstring>

static uint32_t crc32c_table[256];

void init_crc32c_table() {
    const uint32_t poly = 0x1EDC6F41;
    
    for (int i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
        crc32c_table[i] = crc;
    }
}

uint32_t calculate_sctp_checksum(const uint8_t* data, size_t len) {
    static bool table_initialized = false;
    if (!table_initialized) {
        init_crc32c_table();
        table_initialized = true;
    }

    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        crc = crc32c_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}