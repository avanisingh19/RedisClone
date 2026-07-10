# RedisClone

A lightweight **Redis-inspired in-memory key-value database** built in **Modern C++17**. This project demonstrates networking, multithreading, synchronization, persistence, and efficient data structures while following a modular, production-style architecture.

---

## Features

* TCP client-server architecture
* Concurrent client handling using threads
* Thread-safe in-memory key-value store
* Supported commands:

  * `SET`
  * `GET`
  * `DEL`
  * `EXPIRE`
  * `TTL`
* Key expiration management
* Data persistence to disk
* Snapshot generation
* Simple command parser
* Interactive command-line client
* Modular architecture
* Unit tests
* Optional replication support (Bonus)

---

# Architecture

```text
                  +-------------------+
                  |   CLI Client      |
                  +---------+---------+
                            |
                      TCP Socket
                            |
                  +---------v---------+
                  |   Redis Server    |
                  +---------+---------+
                            |
        +-------------------+-------------------+
        |                   |                   |
        |                   |                   |
+-------v------+    +--------v-------+   +-------v-------+
| Command      |    | Database       |   | Persistence   |
| Parser       |    | Hash Table     |   | Snapshot/File |
+--------------+    +----------------+   +---------------+
```

---

# Supported Commands

## SET

```text
SET username Avani
```

Stores a key-value pair.

---

## GET

```text
GET username
```

Returns the stored value.

---

## DEL

```text
DEL username
```

Deletes a key.

---

## EXPIRE

```text
EXPIRE username 30
```

Expires the key after 30 seconds.

---

## TTL

```text
TTL username
```

Returns the remaining lifetime of a key.

---

# Build Instructions

## Requirements

* C++17 compatible compiler
* CMake 3.15+
* Linux or macOS (Windows supported with minor socket changes)

---

## Build

```bash
mkdir build
cd build

cmake ..

make
```

---

# Run the Server

```bash
./RedisServer
```

Default port:

```text
6379
```

---

# Run the Client

```bash
./RedisClient
```

Example session:

```text
SET city Delhi
OK

GET city
Delhi

EXPIRE city 10
OK

TTL city
9

DEL city
OK
```

---

# Thread Safety

The database is protected using mutexes to ensure safe concurrent access from multiple client connections.

Implemented using:

* std::mutex
* std::lock_guard
* std::thread

---

# Persistence

The server periodically stores the in-memory database to disk.

Supported operations:

* Save database
* Load database at startup
* Snapshot generation

Persistence directory:

```text
data/
```

---

# Technologies Used

* Modern C++17
* STL Containers
* Smart Pointers
* Multithreading
* TCP Sockets
* File I/O
* Synchronization Primitives
* Hash Tables
* CMake

---

---

# Learning Outcomes

This project demonstrates practical experience with:

* Socket Programming
* Concurrent Server Design
* Thread Synchronization
* Custom Data Storage
* Command Parsing
* Serialization
* File Persistence
* Modular Software Architecture
* Modern C++17 Development
* Production-Style Project Organization

---

# License

This project is intended for educational purposes and portfolio demonstration.

---

## Author

**Avani Singh**

**Project:** RedisClone — A Redis-inspired in-memory database built using Modern C++17 to demonstrate systems programming, networking, multithreading, and persistence.
