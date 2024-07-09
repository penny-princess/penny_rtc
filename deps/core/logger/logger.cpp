//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "logger.h"

namespace core {

    Logger *Logger::_instance = NULL;

    std::string time_iso_time() {
        struct timeval tv;
        struct tm tm;
        gettimeofday(&tv, NULL);
        time_t t = time(NULL);
        localtime_r(&t, &tm);
        char buf[128] = {0};
        auto n = sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d",
                         tm.tm_year + 1900,
                         tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        return std::string(buf, buf + n);
    }

    void Logger::set_log_dir(const std::string &log_dir) {
        _log_dir = log_dir;
    }

    void Logger::set_log_file(const std::string &log_file) {
        _log_file = log_file;
    }

    Logger::Logger() {
    }

    LogStream::~LogStream() {
        _stream << "\n";
        _logger->write(_stream.str());
    }

    Logger::~Logger() {
        stop();
        _out_file.close();
        if (_thread) {
            delete _thread;
            _thread = nullptr;
        }
    }

    void Logger::stop() {
        _running = false;
        _cv.notify_all();
        this->_join();
    }

    void Logger::_join() {
        if (_thread && _thread->joinable()) {
            _thread->join();
        }
    }

    void Logger::set_level(int level) {
        _level = level;
    }

    int Logger::get_level() const {
        return _level;
    }

    void Logger::set_stderr(bool is_stderr) {
        _stderr = is_stderr;
    }

    void Logger::write(const std::string &msg) {
        if (_stderr) {
            std::cout << msg << std::flush;
        }
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _log_queue.push(msg);
        }
        _cv.notify_one();
    }

    Logger *Logger::instance() {
        if (_instance == NULL) {
            _instance = new Logger();
        }
        return _instance;
    }

    const char *log_string[] = {
            " TRACE ",
            " DEBUG ",
            " INFO ",
            " WARN ",
            " ERROR ",
    };

    static thread_local pid_t thread_id = 0;

    LogStream::LogStream(Logger *logger, const char *file, int line, int level, const char *func) {
        _logger = logger;
        const char *file_name = strchr(file, '/');
        if (file_name) {
            file_name = file_name + 1;
        } else {
            file_name = file;
        }

        _stream << time_iso_time();
        if (thread_id == 0) {
            thread_id = static_cast<pid_t>(::syscall(SYS_gettid));
        }
        _stream << " [" << thread_id << "]";
        _stream << std::setw(7);
        _stream << std::setiosflags(std::ios::left);
        _stream << log_string[level];
        _stream << "[" << file_name << ":" << line << "]";
        if (func) {
            std::string tmp;
            _stream << std::setw(20);
            _stream << std::setiosflags(std::ios::left);
            tmp.append("[");
            tmp.append(func);
            tmp.append("]");
            _stream << tmp;
            tmp.clear();
        }

    }

    int Logger::init() {
        _log_file = _log_dir + "/" + _log_name + ".log";
        int ret = mkdir(_log_dir.c_str(), 0755);
        if (ret != 0 && errno != EEXIST) {
            fprintf(stderr, "create log_dir[%s] failed\n", _log_dir.c_str());
            return -1;
        }

        // 打开文件
        _out_file.open(_log_file, std::ios::app);
        if (!_out_file.is_open()) {
            fprintf(stderr, "open log_file[%s] failed\n", _log_file.c_str());
            return -1;
        }

        ret = _thread_start();
        if(ret == -1) {
            return -1;
        }
        return 0;
    }

    int Logger::_thread_start() {
        if (_running) {
            fprintf(stderr, "log thread already running\n");
            return -1;
        }

        _running = true;
        _thread = new std::thread([=]() {
            struct stat stat_data;
            std::stringstream ss;
            while (_running) {
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    _cv.wait(lock, [this] { return !_log_queue.empty() || !_running; });

                    if (!_running && _log_queue.empty()) {
                        break;
                    }

                    while (!_log_queue.empty()) {
                        ss << _log_queue.front();
                        _log_queue.pop();
                    }
                }

                if (stat(_log_file.c_str(), &stat_data) < 0) {
                    _out_file.close();
                    _out_file.open(_log_file, std::ios::app);
                }
                _out_file << ss.str();
                _out_file.flush();
                ss.str("");
            }
        });
        return 0;
    }

}