//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once

#include "pch.h"
#include <condition_variable>

namespace core {

    enum {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        MAX_LEN
    };


    class Logger {
    public:
        Logger();

        ~Logger();

        static Logger *instance();

        void write(const std::string &msg);

        void set_level(int level);

        void set_stderr(bool is_stderr);

        int init();

        void stop();

        void set_log_dir(const std::string &log_dir);

        void set_log_file(const std::string &log_file);

        int get_level() const;
    private:

        int _thread_start();
        void _join();

    private:
        bool _stderr = true;
        int _level = TRACE;
        static Logger *_instance;

        std::string _log_dir = "log";
        std::string _log_name = "penny_log";
        std::string _log_file;

        std::mutex _mtx;
        std::ofstream _out_file;
        std::thread *_thread = nullptr;
        std::queue<std::string> _log_queue;
        std::atomic<bool> _running{false};

        std::condition_variable _cv;
    };

    class LogStream {
    public:
        LogStream(Logger *logger, const char *file, int line, int level, const char *func);

        ~LogStream();

        template<class T>
        LogStream &operator<<(const T &value) {
            _stream << value;
            return *this;
        }

    private:
        std::ostringstream _stream;
        Logger *_logger;
    };


}

#define LOG(level) \
    if((core::Logger::instance()->get_level() == level || core::Logger::instance()->get_level() == level) || core::Logger::instance()->get_level() <= level) \
        core::LogStream(core::Logger::instance(),__FILE__, __LINE__,level, __func__)

#define LOG_TRACE \
    if(core::Logger::instance()->get_level() <= core::LogLevel::TRACE) core::LogStream(core::Logger::instance(),__FILE__, __LINE__,core::TRACE, __func__)

#define LOG_DEBUG \
    if(core::Logger::instance()->get_level() <= core::LogLevel::DEBUG) core::LogStream(core::Logger::instance(),__FILE__, __LINE__,core::DEBUG, __func__)

#define LOG_INFO \
    if(core::Logger::instance()->get_level() <= core::LogLevel::INFO) core::LogStream(core::Logger::instance(),__FILE__, __LINE__,core::INFO, __func__)

#define LOG_WARN \
    core::LogStream(core::Logger::instance(),__FILE__, __LINE__,core::WARN, __func__)

#define LOG_ERROR \
    core::LogStream(core::Logger::instance(),__FILE__, __LINE__,core::ERROR, __func__)
