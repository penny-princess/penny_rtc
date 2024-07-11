//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once

#include "pch.h"

namespace core {
    class EventLoop;

    class IOEvent;

    class TimerEvent;

    using io_callback = std::function<void(EventLoop *loop, IOEvent *event, int fd, int events, void *data)>;
    using timer_callback = std::function<void(EventLoop *loop, TimerEvent *w, void *data)>;

    class EventLoop {
    private:
        int _epoll_fd = -1;
        void *_owner = nullptr;
        bool _running = false;
        std::vector<epoll_event> _events;

    public:
        enum : int {
            READ = EPOLLIN,
            WRITE = EPOLLOUT
        };
        EventLoop(void * owner);

        ~EventLoop();

        void start();

        void stop();

        void *owner();
    public:
        IOEvent *create_io_event(io_callback, void *data);

        int start_io_event(IOEvent *io_event, int fd, int mask);

        int stop_io_event(IOEvent *io_event);

        void delete_io_event(IOEvent *io_event);

    public:
        TimerEvent* create_timer_event(timer_callback, void *data, bool repeat);

        void start_timer_event(TimerEvent *timer_event, unsigned int usec);

        void stop_timer_event(TimerEvent *timer_event);

        void delete_timer_event(TimerEvent *timer_event);

        unsigned long now();
    };

    class TimerEvent {
        public:
            EventLoop* loop;
            timer_callback callback;
            void* data;
            bool repeat;
            int interval;
            int fd;

        public:
            TimerEvent(EventLoop* loop, timer_callback callback, void* data, bool repeat): loop(loop),callback(callback), data(data), repeat(repeat) {};
    };

    class IOEvent {
        public:
            EventLoop* loop = nullptr;
            io_callback callback;
            void* data = nullptr;
            int fd = -1;
        public:
            IOEvent(EventLoop* loop, io_callback callback, void* data): loop(loop),callback(callback), data(data) {}
    };

}
