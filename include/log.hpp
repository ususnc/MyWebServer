#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <thread>
#include <mutex>

#include "block_queue.hpp"

using namespace std;

class Log
{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);

    bool close_log();

        
private:
    Log();
    virtual ~Log();
    void async_write_log();
    

private:
    char m_dir_name[128]; //路径名
    char m_log_file_name[128]; //log文件名
    int m_split_lines;  //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long m_count;  //日志行数记录
    int m_today;        //因为按天分类,记录当前时间是那一天
    FILE *m_fp;         //打开log的文件指针
    char *m_buf;
    block_queue<string> m_log_queue; //阻塞队列
    bool m_is_async;                  //是否同步标志位
    std::mutex m_mutex;
    int m_close_log; //关闭日志标志
};

#define LOG_DEBUG(format, ...) if(!Log::get_instance()->close_log()) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(!Log::get_instance()->close_log()) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(!Log::get_instance()->close_log()) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(!Log::get_instance()->close_log()) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}

