# HTTP2.5 - HTTP over SCTP Stack

## Overview

HTTP2.5 is a custom implementation of HTTP running on top of a from-scratch SCTP (Stream Control Transmission Protocol) stack. This project demonstrates protocol implementation by building HTTP client and server functionality directly on top of a reliable, message-oriented transport layer rather than using TCP.

## Architecture

### Two-Layer Design

```
┌─────────────────────────────────────┐
│  HTTP Layer (Client & Server)       │
│  - HTTP Parsing & Generation        │
│  - Request/Response Handling         │
│  - Route Matching & Handlers         │
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│  SCTP Stack                          │
│  - Reliable Message Delivery         │
│  - Association Management            │
│  - Packet Serialization/Parsing      │
│  - Checksum Calculation              │
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│  UDP Socket (Windows Winsock2)       │
└─────────────────────────────────────┘
```

## Project Structure

### `/sctp_stack/` - SCTP Implementation

Core SCTP protocol stack with the following components:

- **`sctp.hpp`**: Core data structures
  - `SCTP_Common_Header`: Standard SCTP packet header with source/destination ports, verification tag, and checksum
  - `Chunk_Type`: Enum defining SCTP chunk types (DATA, INIT, INIT_ACK, SACK, HEARTBEAT, SHUTDOWN, etc.)
  - `init_chunk_value`: SCTP initialization handshake parameters
  - `data_chunk_value`: User data payload structure with TSN (Transmission Sequence Number), stream ID, and payload

- **`sctp_socket.hpp`**: Main SCTP socket class
  - Manages SCTP associations (connections)
  - Handles binding, listening, and data transmission
  - Runs an internal event loop for packet processing
  - Thread-safe queue for outgoing messages

- **`sctp_serialize.cpp/hpp`**: Packet serialization
  - Converts SCTP data structures to/from binary format
  - Handles chunking and packet construction

- **`sctp_checksum.cpp/hpp`**: Checksum calculation
  - Implements SCTP's Adler-32 checksum algorithm
  - Validates packet integrity

- **`sctp_association.hpp`**: Association management
  - Tracks connection state
  - Manages transmission/reception of data chunks
  - Handles sequence numbers and acknowledgments

### `/http/` - HTTP Implementation

HTTP client and server built directly on the SCTP socket:

- **`http_request.hpp`**: HTTP request structure
  - Request line (method, URI, version)
  - Headers map
  - Body (binary data)

- **`http_response.hpp`**: HTTP response structure
  - Status line
  - Headers
  - Body content

- **`http_parse.cpp/hpp`**: Request parsing
  - Parses incoming SCTP data into HTTP request objects
  - Handles headers, body, and request line parsing

- **`http_response.cpp/hpp`**: Response generation
  - Serializes HTTP response objects into binary format
  - Generates properly formatted HTTP responses

- **`server.hpp/cpp`**: HTTP Server
  - Binds to IP/port using SCTP
  - Supports route registration with pattern matching and parameter extraction
  - Processes incoming HTTP requests and invokes registered handlers
  - Sends HTTP responses back to clients

- **`client.hpp/cpp`**: HTTP Client
  - Connects to HTTP server via SCTP
  - Provides methods for GET, POST, PUT, DELETE requests
  - Handles request building and response parsing

- **`tests/`**: Test suite
  - `test_parsing.cpp`: Tests for HTTP parsing functionality
  - `tests.hpp`: Test utilities

## How It Works

### Connection Flow

1. **Server-side**:
   - Server creates an `SCTP_Socket` and binds to an IP address and port
   - Calls `sctp_run()` to start the internal event loop
   - Registers HTTP routes with handler functions
   - Waits for incoming SCTP associations (connections)
   - When HTTP data arrives, parses it and routes to appropriate handlers

2. **Client-side**:
   - Client creates an `SCTP_Socket` and associates with the server
   - `sctp_associate()` initiates SCTP handshake (INIT/INIT_ACK)
   - Once association is established, sends HTTP requests as SCTP data
   - Receives HTTP responses via SCTP

### Data Flow

```
HTTP Request (string)
    ↓ (http_parse converts to Request struct)
HTTP Request Struct
    ↓ (serialized to binary)
Binary HTTP Data
    ↓ (sent via SCTP_Socket::sctp_send_data)
SCTP Data Chunks
    ↓ (serialized with SCTP header, checksum added)
SCTP Packets
    ↓ (sent via UDP socket)
Network (UDP packets)
    ↓
UDP Socket
    ↓ (SCTP deserialization)
SCTP Packets
    ↓ (http_parse extracts data)
Binary HTTP Data
    ↓ (converted to Response struct)
HTTP Response Struct
    ↓ (returned to application)
```

### Key Features

- **Reliable Delivery**: SCTP ensures all data is delivered in order through sequence numbers and acknowledgments
- **Multi-streaming**: SCTP supports multiple independent streams within a single association
- **Ordered Data**: Uses TSN (Transmission Sequence Number) to maintain order
- **Route Matching**: Server supports parameterized routes with regex matching (e.g., `/users/:id`)
- **Request/Response**: Standard HTTP semantics with methods, headers, and bodies

## Building

Two build configurations are available:

### SCTP Stack Only
```
g++ -g sctp_stack/*.cpp -o sctp_stack/main.exe -lws2_32
```

### HTTP with SCTP Stack
```
g++ -g sctp_stack/sctp_*.cpp http/*.cpp -o http/main.exe -lws2_32
```

Both configurations require the Windows Winsock2 library (`-lws2_32`).

## Usage Example

### Server
```cpp
Server server("127.0.0.1", 8080);
server.register_route("/", [](const Request& req, const auto& params) {
    Response resp;
    resp.status_code = 200;
    resp.body = "Hello World";
    return resp;
});
server.start();
```

### Client
```cpp
Client client("127.0.0.1", 8080);
client.connect("127.0.0.1", 8080);
auto response = client.get_request("/");
```

## Technical Details

### SCTP Chunk Types Supported
- **DATA**: User data transmission
- **INIT/INIT_ACK**: Association establishment
- **SACK**: Selective acknowledgments
- **HEARTBEAT/HEARTBEAT_ACK**: Keep-alive mechanism
- **SHUTDOWN/SHUTDOWN_ACK/SHUTDOWN_COMPLETE**: Graceful closure
- **COOKIE_ECHO/COOKIE_ACK**: Four-way handshake completion
- **ABORT**: Immediate termination

### Thread Model
- Main application thread makes synchronous calls to `SCTP_Socket` and `Server`/`Client`
- Internal event loop thread handles incoming packets and state management
- Thread-safe queues ensure proper synchronization

## Platform Requirements
- Windows (uses Winsock2)
- C++11 or later
- GCC or compatible compiler

## Future Enhancements
- Support for additional HTTP methods and status codes
- SSL/TLS encryption over SCTP
- Performance optimizations
- Multi-platform support (Linux, macOS)
- Congestion control mechanisms
- Flow control improvements
