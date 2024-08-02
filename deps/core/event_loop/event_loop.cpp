//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "event_loop.h"
#include <sys/timerfd.h>
#include <chrono>
#include <stdexcept>

namespace core {

    EventLoop::EventLoop(void *owner):_events(1024) {
        this->_owner = owner;
        _epoll_fd = epoll_create1(0);
        if (_epoll_fd == -1) {
            exit(-1);
        }
    }

    EventLoop::~EventLoop() {
        ::close(_epoll_fd);
    }

    void EventLoop::start() {
        _running = true;

        while (_running) {
            int num_events = epoll_wait(_epoll_fd, _events.data(), _events.size(), -1);
            if (num_events == -1) {
                if (errno == EINTR) continue;
                throw std::runtime_error("epoll_wait failed");
            }
            for (int i = 0; i < num_events; ++i) {
                IOEvent *io_event = static_cast<IOEvent*>(_events[i].data.ptr);
                if (io_event && io_event->callback) {
                    io_event->callback(this, io_event, io_event->fd, _events[i].events, io_event->data);
                }
            }
            if(num_events >= static_cast<int>(_events.size()) ) {
                _events.resize(_events.size() * 2);
            }
        }
    }

    void EventLoop::stop() {
        _running = false;
    }

    void *EventLoop::owner() {
        return _owner;
    }

    IOEvent* EventLoop::create_io_event(io_callback callback, void* data) {
        return new IOEvent(this, callback, data);
    }

    int EventLoop::start_io_event(IOEvent *io_event, int fd, int mask) {
        epoll_event event;
        event.events = mask;
        event.data.ptr = io_event;
        io_event->fd = fd;
        int ret = epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event);
        if(-1 == ret) {
            return -1;
        }
        return 0;
    }

    int EventLoop::stop_io_event(IOEvent* io_event) {
        epoll_event ev;
        ev.data.ptr = io_event;
        int ret = epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, io_event->fd, &ev);
        if ( ret == -1) {
            return -1;
        }
        return 0;
    }

    void EventLoop::delete_io_event(IOEvent* io_event) {
        stop_io_event(io_event);
        if(io_event) {
            delete io_event;
            io_event = nullptr;
        }
    }

    TimerEvent* EventLoop::create_timer_event(timer_callback callback, void* data,bool repeat) {
        return new TimerEvent(this, callback, data, repeat);
    }

    void EventLoop::start_timer_event(TimerEvent *timer_event, unsigned int usec) {
        int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timer_fd == -1) {
            throw std::runtime_error("Failed to create timer file descriptor");
        }

        struct itimerspec ts;
        ts.it_value.tv_sec = usec / 1000000;
        ts.it_value.tv_nsec = (usec % 1000000) * 1000;
        ts.it_interval.tv_sec = timer_event->repeat ? ts.it_value.tv_sec : 0;
        ts.it_interval.tv_nsec = timer_event->repeat ? ts.it_value.tv_nsec : 0;

        if (timerfd_settime(timer_fd, 0, &ts, nullptr) == -1) {
            close(timer_fd);
            throw std::runtime_error("Failed to set timer");
        }

        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.ptr = timer_event;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
            close(timer_fd);
            throw std::runtime_error("Failed to add timer file descriptor to epoll");
        }

        timer_event->fd = timer_fd;
    }

    void EventLoop::stop_timer_event(TimerEvent *timer_event) {
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.ptr = timer_event;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, timer_event->fd, &ev) == -1) {
            throw std::runtime_error("Failed to remove timer file descriptor from epoll");
        }
        close(timer_event->fd);
        timer_event->fd = -1;
    }

    void EventLoop::delete_timer_event(TimerEvent *timer_event) {
        epoll_event ev;
        ev.events = EPOLLIN;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, timer_event->fd, &ev) == -1) {
            throw std::runtime_error("Failed to remove timer file descriptor from epoll");
        }
        delete timer_event;
    }

    unsigned long EventLoop::now() {
        using namespace std::chrono;
        auto now = steady_clock::now().time_since_epoch();
        double now_double = duration_cast<duration<double>>(now).count();
        return static_cast<unsigned long>(now_double * 1000000);
    }
}