#ifndef WEBSERVER_BLOCK_QUEUE_H
#define WEBSERVER_BLOCK_QUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockQueue {
public:
    explicit BlockQueue(size_t max_size = 1000) : max_size_(max_size) {}

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        deq_.clear();
    }

    ~BlockQueue() {
        clear();
    }

    bool full() {
        std::lock_guard<std::mutex> lock(mutex_);
        return deq_.size() >= max_size_;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return deq_.empty();
    }

    bool front(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(deq_.empty()) {
            return false;
        }
        value = deq_.front();
        return true;
    }

    bool back(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(deq_.empty()) {
            return false;
        }
        value = deq_.back();
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return deq_.size();
    }

    size_t max_size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return max_size_;
    }

    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if(deq_.size() >= max_size_) {
            cond_producer_.notify_one();
            return false;
        }
        deq_.push_back(item);
        cond_consumer_.notify_one();
        return true;
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        while(deq_.empty()) {
            if(cond_consumer_.wait_for(lock, std::chrono::seconds(1)) == 
               std::cv_status::timeout) {
                return false;
            }
        }
        item = deq_.front();
        deq_.pop_front();
        cond_producer_.notify_one();
        return true;
    }

private:
    std::deque<T> deq_;
    size_t max_size_;
    std::mutex mutex_;
    std::condition_variable cond_consumer_;
    std::condition_variable cond_producer_;
};

#endif // WEBSERVER_BLOCK_QUEUE_H 