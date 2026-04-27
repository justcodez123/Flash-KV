#include <iostream>
#include <string>
#include <mutex>
#include "engine/FlashKV.h"
#include "engine/FlashThreadPool.h"
#include "network/FlashServer.h"

// We need a mutex just for the terminal, otherwise the threads 
// will garble the printed text when they talk at the same time.
std::mutex print_mutex;

void safe_print(const std::string& msg) {
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << msg << "\n";
}

int main() {
    std::cout << "--- Booting FlashKV Server ---\n";

    // 1. Initialize the Database Engine
    FlashKV db;

    // 2. Initialize the Thread Pool with 4 workers
    FlashThreadPool pool(4);

    // 3. Initialize the Network Layer with port 6379, the DB, and the Pool
    FlashServer server(6379, &db, &pool);
    
    // Open the door
    server.start();
    
    // Trap the program in the infinite epoll loop
    server.run_event_loop();

    return 0;
}