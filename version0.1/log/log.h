#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Log
{
public:
    // C++后，使用局部变量懒汉模式不用再加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    //异步写日志公有方法，调用私有方法async_write_log
    static void *flush_log_thread(void *arg)
    {
        Log::get_instance()->async_write_log();
    }
    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char* file_name, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    // 将输出内容按照标准格式整理
    void write_log(int level, const char* format, ...);
    // 强行刷新缓冲区
    void flush(void);

private:
    Log();
    virtual ~Log();
    void* async_write_log()
    {
        string single_log;
        // 从阻塞队列中取出一个日志string，写入文件
        while(m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            // int fputs(const char *str, FILE *stream);
            // str，一个数组，包含了要写入的以空字符终止的字符序列。
            // stream，指向FILE对象的指针，该FILE对象标识了要被写入字符串的流。
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }
private:
    char dir_name[128];     // 路径名
    char log_name[128];     // log文件名
    int m_split_lines;      // 日志最大行数
    int m_log_buf_size;     // 日志缓冲区大小
    long long m_count;      // 日志行数记录
    int m_today;            // 因为需要按天分类，所以需要记录当前是哪一天
    FILE *m_fp;             // 打开log的文件指针
    char *m_buf;            
    block_queue<string> *m_log_queue;   // 阻塞队列
    bool m_is_async;        // 是否同步标志位
    locker m_mutex;
};

// 这4个宏定义在其他文件中使用，主要用于不同类型的日志输出
#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__)

#endif