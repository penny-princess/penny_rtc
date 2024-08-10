//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>
#include <utility>
#include <sys/epoll.h>

namespace penny {
    class EventLoop;

    class IOEvent;

    class TimerEvent;

    using io_callback = std::function<void(EventLoop *loop,IOEvent *event, int fd, int events,void *data)>;
    using timer_callback = std::function<void( EventLoop *loop, TimerEvent *w,void *data)>;

    class EventLoop {
    public:
        static const unsigned int READ = EPOLLIN;
        static const unsigned int WRITE = EPOLLOUT;
        
        explicit EventLoop(void * owner);

        ~EventLoop();

        void start();

        void stop();

        void *owner() const;

    private:
        int _epoll_fd = -1;
        void *_owner = nullptr;
        bool _running = false;
        std::vector<epoll_event> _epoll_events;
        std::unordered_map<int, IOEvent*> _events;
        std::unordered_map<int, TimerEvent*> _timers;
        std::priority_queue<TimerEvent*, std::vector<TimerEvent*>,  std::greater<>> _timer_queue;

    public:
        IOEvent *create_io_event(io_callback callback, void *data);

        void start_io_event(IOEvent *io_event, int fd, int mask);

        void stop_io_event(const IOEvent *io_event);

        void delete_io_event(const IOEvent *io_event);

    public:
        TimerEvent* create_timer_event(const timer_callback &cb, void *data, bool repeat);

        void start_timer_event(TimerEvent *timer_event,unsigned int usec);

        void stop_timer_event(TimerEvent *timer_event);

        void delete_timer_event(TimerEvent *timer_event);

        static unsigned long now();

    private:
        void process_timer_event();
        void process_io_event(int fd);

    };

    class IOEvent {
        public:
            EventLoop* loop = nullptr;
            io_callback callback;
            void* data = nullptr;
            int events = -1;
            int fd = -1;
        public:
            IOEvent(EventLoop* loop, io_callback callback, void* data):loop(loop),callback(std::move(callback)),data(data){}
    };

    class TimerEvent {
    public:
        EventLoop* loop = nullptr;
        timer_callback callback;
        void* data = nullptr;
        bool repeat = false;
        unsigned int usec = 1000;
        int active = false;
        std::chrono::time_point<std::chrono::steady_clock> start_time;

        TimerEvent(EventLoop* loop, timer_callback callback, void* data,const bool repeat)
            : loop(loop), callback(std::move(callback)), data(data), repeat(repeat) {}

        std::chrono::time_point<std::chrono::steady_clock> get_expiry_time() const {
            return start_time + std::chrono::milliseconds(usec);
        }

        bool operator>(const TimerEvent& other) const {
            return this->get_expiry_time() > other.get_expiry_time();
        }

    };

}
