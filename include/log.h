#ifndef WEBSERVER_LOG_H
#define WEBSERVER_LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include <memory>
#include "block_queue.h"

class Log {
public:
    void init(const char* file_name, int close_log, int log_buf_size = 8192,
             int split_lines = 5000000, int max_queue_size = 0);

    static Log* instance() {
        static Log inst;
        return &inst;
    }

    static void flush_log_thread() {
        Log::instance()->async_write_log();
    }

    void write_log(int level, const char* format, ...);

    void flush();

    bool is_open() { return is_open_; }
    
private:
    Log();
    virtual ~Log();
    void async_write_log() {
        std::string single_log;
        while(log_queue_->pop(single_log)) {
            std::lock_guard<std::mutex> lock(mtx_);
            fputs(single_log.c_str(), fp_);
        }
    }

private:
    char dir_name_[128];    // 路径名
    char log_name_[128];    // log文件名
    int split_lines_;       // 日志最大行数
    int log_buf_size_;     // 日志缓冲区大小
    long long count_;       // 日志行数记录
    int today_;            // 因为按天分类,记录当前时间是那一天
    FILE* fp_;             // 打开log的文件指针
    char* buf_;
    bool is_open_;
    bool is_async_;        // 是否同步标志位
    std::unique_ptr<BlockQueue<std::string>> log_queue_; // 阻塞队列
    std::mutex mtx_;
};

#define LOG_DEBUG(format, ...) if(Log::instance()->is_open()) {Log::instance()->write_log(0, format, ##__VA_ARGS__); Log::instance()->flush();}
#define LOG_INFO(format, ...) if(Log::instance()->is_open()) {Log::instance()->write_log(1, format, ##__VA_ARGS__); Log::instance()->flush();}
#define LOG_WARN(format, ...) if(Log::instance()->is_open()) {Log::instance()->write_log(2, format, ##__VA_ARGS__); Log::instance()->flush();}
#define LOG_ERROR(format, ...) if(Log::instance()->is_open()) {Log::instance()->write_log(3, format, ##__VA_ARGS__); Log::instance()->flush();}

#endif // WEBSERVER_LOG_H