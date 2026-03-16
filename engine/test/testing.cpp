#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cassert>

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <variant>
#include <functional>

// ==========================================
// 1. DATA TYPES (The Value)
// ==========================================
// Using modern C++ aliases for clean reading
using FlashString = std::string;
using FlashList = std::vector<std::string>;
using FlashHash = std::unordered_map<std::string, std::string>;

// std::variant acts as a type-safe union. A key can hold ONE of these types.
struct FlashObject {
    std::variant<FlashString, FlashList, FlashHash> data;
};

// ==========================================
// 2. THE SHARD (Lock Striping)
// ==========================================
struct DatabaseShard {
    std::mutex mtx; // Protects only this specific shard
    std::unordered_map<std::string, FlashObject> kv_store;
};

// ==========================================
// 3. THE MAIN DATABASE CLASS
// ==========================================
class FlashKV {
private:
    // Power of 2 for optimal modulo hashing
    static constexpr size_t NUM_SHARDS = 1024; 
    
    // The actual database memory
    std::vector<DatabaseShard> shards;
    
    // The Hash Function
    std::hash<std::string> hasher;

    // Routing Helper: Converts "user:1" -> Bucket 402
    size_t get_shard_index(const std::string& key) {
        return hasher(key) % NUM_SHARDS;
    }

public:
    // Constructor initializes the vector with 1024 shards
    //FlashKV() : shards(NUM_SHARDS) {}
    FlashKV(){
      shards.resize(NUM_SHARDS);
    }

    // ==========================================
    // 4. SINGLE-KEY OPERATIONS
    // ==========================================
    void set_string(const std::string& key, const std::string& value) {
        size_t idx = get_shard_index(key);
        
        // Lock ONLY this specific bucket. Unlocks automatically when function ends.
        std::lock_guard<std::mutex> lock(shards[idx].mtx);
        
        shards[idx].kv_store[key] = FlashObject{value};
    }

    bool get_string(const std::string& key, std::string& out_value) {
        size_t idx = get_shard_index(key);
        
        std::lock_guard<std::mutex> lock(shards[idx].mtx);
        
        auto it = shards[idx].kv_store.find(key);
        if (it != shards[idx].kv_store.end()) {
            // Check if it's actually a string before reading
            if (std::holds_alternative<FlashString>(it->second.data)) {
                out_value = std::get<FlashString>(it->second.data);
                return true;
            }
        }
        return false; // Key not found or wrong data type
    }

    // ==========================================
    // 5. MULTI-KEY OPERATIONS (Deadlock Avoidance)
    // ==========================================
    void rename_key(const std::string& old_key, const std::string& new_key) {
        size_t idx1 = get_shard_index(old_key);
        size_t idx2 = get_shard_index(new_key);

        // Edge Case: Both keys hash to the exact same shard
        if (idx1 == idx2) {
            std::lock_guard<std::mutex> single_lock(shards[idx1].mtx);
            auto it = shards[idx1].kv_store.find(old_key);
            if (it != shards[idx1].kv_store.end()) {
                shards[idx1].kv_store[new_key] = std::move(it->second);
                shards[idx1].kv_store.erase(it);
            }
            return;
        }

        // Takes multiple mutexes, sorts them by memory address internally, 
        // and locks them in a globally safe order. Deadlocks are impossible.
        std::scoped_lock multi_lock(shards[idx1].mtx, shards[idx2].mtx);
        
        // --- CRITICAL SECTION ---
        auto it = shards[idx1].kv_store.find(old_key);
        if (it != shards[idx1].kv_store.end()) {
            // Move the data to the new shard and delete the old one
            shards[idx2].kv_store[new_key] = std::move(it->second);
            shards[idx1].kv_store.erase(it);
        }
        // --- END CRITICAL SECTION ---
        // Both mutexes automatically unlock here.
    }
};



FlashKV db;
std::atomic<int> successful_renames{0};

// --- Test 1: Massive Concurrent Writes ---
// 10 threads will try to write 10,000 keys at the exact same time
void writer_thread_task(int thread_id) {
    for (int i = 0; i < 10000; ++i) {
        std::string key = "user:" + std::to_string(thread_id) + ":" + std::to_string(i);
        std::string value = "data_" + std::to_string(i);
        db.set_string(key, value);
    }
}

// --- Test 2: The Deadlock Survivor ---
// Two threads will violently try to rename the exact same two keys 
// in opposite directions simultaneously. 
void rename_collision_task(int thread_id) {
    for (int i = 0; i < 5000; ++i) {
        if (thread_id % 2 == 0) {
            db.rename_key("conflict_A", "conflict_B");
        } else {
            db.rename_key("conflict_B", "conflict_A");
        }
        successful_renames++;
    }
}

int main() {
    std::cout << "Starting FlashKV Core Engine Test...\n";

    // 1. Seed the conflict keys
    db.set_string("conflict_A", "Alice");
    db.set_string("conflict_B", "Bob");

    // 2. Spawn the Worker Threads
    std::vector<std::thread> workers;

    std::cout << "[Test 1] Launching 10 concurrent writer threads...\n";
    for (int i = 0; i < 101; ++i) {
        workers.push_back(std::thread(writer_thread_task, i));
    }

    std::cout << "[Test 2] Launching 4 deadlock-inducing rename threads...\n";
    for (int i = 0; i < 4; ++i) {
        workers.push_back(std::thread(rename_collision_task, i));
    }

    // 3. Wait for all threads to finish (Join them back to main)
    for (auto& t : workers) {
        t.join();
    }

    // 4. Verify the Data
    std::string result;
    bool found = db.get_string("user:5:9999", result);
    
    std::cout << "\n--- TEST RESULTS ---\n";
    if (found && result == "data_9999") {
        std::cout << "✅ Concurrent Writes: PASSED (Data intact)\n";
    } else {
        std::cout << "❌ Concurrent Writes: FAILED (Data lost or corrupted)\n";
    }

    std::cout << "✅ Deadlock Avoidance: PASSED (" << successful_renames << " cross-renames survived)\n";
    std::cout << "If you are reading this, the database did not freeze or SegFault!\n";

    return 0;
}
