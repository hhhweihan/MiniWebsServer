#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <vector>
#include <mutex>
#include <unordered_map>

class Timer {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    using Callback = std::function<void()>;
    
    struct TimerNode {
        TimePoint expire;
        Callback cb;
        int id;
        
        bool operator>(const TimerNode& t) const {
            return expire > t.expire;
        }
    };
    
    Timer() = default;
    ~Timer() = default;
    
    // 添加定时器
    void add_timer(int id, int timeout, const Callback& cb) {
        if(timeout < 0) return;
        std::lock_guard<std::mutex> lock(mutex_);
        
        TimePoint expire = Clock::now() + std::chrono::milliseconds(timeout);
        timers_.push({expire, cb, id});
    }
    
    // 调整定时器
    void adjust_timer(int id, int timeout) {
        std::lock_guard<std::mutex> lock(mutex_);
        // 由于是小根堆，不支持随机访问和修改，这里采用延迟删除的方式
        ref_[id] = Clock::now() + std::chrono::milliseconds(timeout);
    }
    
    // 删除指定id的定时器
    void del_timer(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        ref_[id] = TimePoint();
    }
    
    // 处理所有到期的定时器
    void tick() {
        std::lock_guard<std::mutex> lock(mutex_);
        if(timers_.empty()) {
            return;
        }
        
        while(!timers_.empty()) {
            TimerNode node = timers_.top();
            if(std::chrono::duration_cast<std::chrono::milliseconds>
               (node.expire - Clock::now()).count() > 0) {
                break;
            }
            
            // 检查是否被延迟删除
            auto it = ref_.find(node.id);
            if(it == ref_.end() || it->second == node.expire) {
                node.cb();
            }
            timers_.pop();
            ref_.erase(node.id);
        }
    }
    
private:
    std::priority_queue<TimerNode, std::vector<TimerNode>, std::greater<TimerNode>> timers_;
    std::unordered_map<int, TimePoint> ref_;
    std::mutex mutex_;
};

#endif // WEBSERVER_TIMER_H 