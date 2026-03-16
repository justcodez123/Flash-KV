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
    // std::cout << "--- Booting FlashKV Internal Test ---\n";

    // // 1. Initialize the Database Engine
    // FlashKV db;

    // // 2. Scope block for the Thread Pool
    // // We do this so the Thread Pool's destructor gets called at the end of the block,
    // // which automatically waits for all workers to finish before moving on.
    // {
    //     FlashThreadPool pool(4); // Booting 4 worker threads

    //     std::cout << "Submitting 20 concurrent WRITE tasks to the Pool...\n\n";

    //     for (int i = 0; i < 20; ++i) {
    //         // Push a Lambda function into the task queue
    //         pool.enqueue_task([&db, i]() {
    //             std::string key = "user:" + std::to_string(i);
    //             std::string value = "data_" + std::to_string(i);
                
    //             // Write to the database
    //             db.set_string(key, value);
                
    //             safe_print("[Worker Thread] Successfully wrote " + key);
    //         });
    //     }
        
    // } // Pool destructor is called here. Main thread pauses until all 20 tasks are done.

    // std::cout << "\n--- Verification Phase ---\n";
    
    // // 3. Read the data back sequentially to prove the engine saved it
    // int success_count = 0;
    // for (int i = 0; i < 20; ++i) {
    //     std::string key = "user:" + std::to_string(i);
    //     std::string out_value;
        
    //     if (db.get_string(key, out_value)) {
    //         success_count++;
    //     }
    // }

    // std::cout << "Successfully retrieved " << success_count << "/20 records!\n";
    
    // if (success_count == 20) {
    //     std::cout << "SUCCESS: Engine and Thread Pool are perfectly synced.\n";
    // } else {
    //     std::cout<< "No. of threads which got processed : " <<success_count <<std::endl;
    //     std::cout << "FAILURE: Data was lost!\n";
    // }

    /* The Network layer is the entrypoint for the Database*/
    std::cout << "--- Booting FlashKV Server ---\n";

    FlashServer server(6379);
    
    // Open the door
    server.start();
    
    // Trap the program in the infinite epoll loop
    server.run_event_loop();

    return 0;
}