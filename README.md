# ValkeyLite

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/build-CMake-brightgreen.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An embeddable, high-performance in-memory key-value database engine implemented in modern C++ (C++23), compatible with a rich subset of the [Valkey](https://valkey.io/) / [Redis](https://redis.io/) command specification.

---

## Features

- **In-Memory Storage Engine**: Fast in-process storage designed with clean cache locality and custom memory allocator support.
- **Valkey / Redis Command Protocol**:
  - **Strings**: `SET`, `GET`, `STRLEN`, `APPEND`, `EXPIRE`, `TTL`
  - **Lists**: `LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LLEN`, `LRANGE`, `LINDEX`, `LSET`, `LINSERT`
  - **Sets**: `SADD`, `SREM`, `SISMEMBER`, `SMEMBERS`, `SCARD`, `SINTER`, `SUNION`, `SDIFF`
  - **Hashes**: `HSET`, `HGET`, `HDEL`, `HEXISTS`, `HGETALL`, `HKEYS`, `HVALS`, `HLEN`
  - **Generic / Key Management**: `DEL`, `EXISTS`, `KEYS`, `TYPE`, `FLUSHDB`
- **TTL & Expiration**: Per-key time-to-live expiration timers with lazy and proactive cleanup.
- **Embedded or CLI Mode**: Can be embedded directly as a C++ library or used as a standalone REPL interactive console application.
- **Custom Memory Management**: Custom allocator implementation tailored for minimal fragmentation and high-throughput allocations.

---

## Supported Commands Overview

| Data Structure | Supported Operations |
|----------------|----------------------|
| **String** | `SET`, `GET`, `APPEND`, `STRLEN`, `EXPIRE`, `TTL` |
| **List** | `LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LLEN`, `LRANGE`, `LINDEX`, `LSET`, `LINSERT` |
| **Set** | `SADD`, `SREM`, `SISMEMBER`, `SMEMBERS`, `SCARD`, `SINTER`, `SUNION`, `SDIFF` |
| **Hash** | `HSET`, `HGET`, `HDEL`, `HEXISTS`, `HGETALL`, `HKEYS`, `HVALS`, `HLEN` |
| **Server** | `DEL`, `EXISTS`, `KEYS`, `TYPE`, `FLUSHDB`, `PING` |

---

## Architecture Overview

```
ValkeyLite
├── bin/
│   └── main.cpp           # CLI interactive REPL entry point
├── lib/
│   ├── include/
│   │   ├── Allocator.hpp      # Custom memory pool allocator
│   │   ├── DataBase.hpp       # Core key-value database container
│   │   ├── DBTypes.hpp        # Polymorphic value variants (String, List, Set, Hash)
│   │   ├── ValKeyServer.hpp   # Command parsing, validation, and dispatch
│   │   └── ValKeyClient.hpp   # Client session handler
│   └── src/               # Implementation files
└── tests/
    └── test.cpp           # Comprehensive unit and integration test suite
```

---

## Build Instructions

### Prerequisites
- Modern C++ compiler with C++23 support (`gcc >= 13` or `clang >= 17`)
- CMake `>= 3.12`

### Building

```bash
# Clone the repository
git clone https://github.com/NikitaKkv/ValkeyLite.git
cd ValkeyLite

# Configure and compile
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The compiled binary `valkey` will be placed in `build/bin/`.

### Running Tests

```bash
ctest --test-dir build --output-on-failure
```

---

## Quick Start (Interactive REPL)

```bash
$ ./build/bin/valkey
> SET user:1:name "Nikita"
OK
> GET user:1:name
"Nikita"
> HSET user:1:profile role "Software Engineer" location "SPb"
(integer) 2
> HGETALL user:1:profile
1) "role"
2) "Software Engineer"
3) "location"
4) "SPb"
> EXPIRE user:1:name 300
(integer) 1
> TTL user:1:name
(integer) 299
```

---

## License

This project is licensed under the MIT License.
