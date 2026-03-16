#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class FlashThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> task_queue; 
    
    std::mutex queue_mutex;                       
    std::condition_variable condition;            
    bool stop_pool; // Removed initializer                      

public:
    // Traditional C++ Constructor
    FlashThreadPool(size_t num_threads) {
        stop_pool = false; // Assigned inside the body
        
        // Create the worker threads
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                // The infinite loop for each thread
                while (true) {
                    std::function<void()> task;

                    {
                        // Lock the queue to check for tasks
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        
                        // Go to sleep until a task arrives OR the pool is shut down
                        this->condition.wait(lock, [this] {
                            return this->stop_pool || !this->task_queue.empty();
                        });

                        // If the pool is shutting down and queue is empty, kill the thread
                        if (this->stop_pool && this->task_queue.empty()) {
                            return; 
                        }

                        // Grab the task and remove it from the queue
                        task = std::move(this->task_queue.front());
                        this->task_queue.pop();
                    } // Mutex auto-unlocks here so other threads can use the queue

                    // Execute the database command outside the lock
                    task(); 
                }
            });
        }
    }

    // Function to add a new task to the queue
    void enqueue_task(std::function<void()> new_task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            task_queue.push(new_task);
        }
        // Ring the bell to wake up one sleeping worker
        condition.notify_one(); 
    }

    // Traditional Destructor
    ~FlashThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_pool = true;
        }
        
        condition.notify_all(); // Wake everyone up to realize it's time to exit
        
        // Wait for all threads to finish their current job before closing
        for (std::thread &worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};