# ⚡ FlashKV: High-Throughput Multithreaded Key-Value Store

![C++17](https://img.shields.io/badge/C++-17-blue.svg) ![Linux](https://img.shields.io/badge/OS-Linux%20(epoll)-orange.svg) ![Architecture](https://img.shields.io/badge/Architecture-Multithreaded-success.svg)

FlashKV is a high-performance, in-memory Key-Value database engineered entirely from scratch in modern C++ (C++17). 

Designed to overcome the single-thread bottlenecks of traditional in-memory stores (like standard Redis), FlashKV implements a deeply multithreaded architecture utilizing **1024-way Lock Striping**, a zero-idle-CPU custom Thread Pool, and an asynchronous `epoll` network layer.

## 🧠 Core Architecture

FlashKV was built to demonstrate deep, bare-metal understanding of Linux system calls, concurrency, and memory management.

### 1. Storage Engine: 1024-Way Lock Striping
Instead of a global mutex bottleneck, the storage engine (`std::array<DatabaseShard, 1024>`) is partitioned into 1024 independent shards. Keys are routed via a bitwise-optimized modulo hash. This allows up to 1,024 CPU threads to execute concurrent write operations simultaneously without lock contention or memory corruption. Deadlocks on multi-key operations are strictly prevented using C++17 `std::scoped_lock`.

### 2. Network Layer: Asynchronous `epoll`
The server handles thousands of concurrent TCP connections on a single thread without blocking. By converting standard Linux sockets to `O_NONBLOCK` via `fcntl` and utilizing the `epoll` event loop, the server efficiently multiplexes network I/O, safely bypassing standard OS connection limits.

### 3. Concurrency: Zero-Idle Thread Pool
FlashKV utilizes a Producer-Consumer threading model to eliminate OS-level thread creation overhead. A fixed pool of `std::thread` workers (tied to hardware core count) sleeps at 0% CPU utilization until awakened by a `std::condition_variable`. 

### 4. "Lazy Parsing" Command Bridge
To ensure the `epoll` receptionist never freezes, raw TCP strings are pushed directly into a thread-safe `std::queue`. Worker threads pull the data, parse the RESP-style strings via C++ Lambdas, execute the database logic, and flush the response back to the client socket.

---

## 🚀 Quick Start

### Prerequisites
* Linux Environment (Ubuntu recommended)
* `g++` compiler supporting C++17
* `make`

### Build & Run
Compile the core engine and network layer using the included Makefile:
```bash
make
./flash_server
