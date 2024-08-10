//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "event_loop.h"

#include <chrono>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/timerfd.h>

namespace core {
    EventLoop::EventLoop(void *owner): _epoll_fd(epoll_create1(0)), _owner(owner), _epoll_events(1024) {
        if (_epoll_fd == -1) {
            std::cerr << "Failed to create epoll file descriptor: " << strerror(errno) << std::endl;
            exit(-1);
        }
    }

    EventLoop::~EventLoop() {
        ::close(_epoll_fd);
    }

    void EventLoop::start() {
        _running = true;

        while (_running) {
            process_timer_event();
            int timeout = -1;
            if (!_timer_queue.empty()) {
                auto now = std::chrono::steady_clock::now();
                auto next_timer = _timer_queue.top();
                auto time_until_next = std::chrono::duration_cast<std::chrono::milliseconds>(next_timer->get_expiry_time() - now).count();
                timeout = std::max(0, static_cast<int>(time_until_next));
            }
            const int num_events = epoll_wait(
                _epoll_fd,
                (struct epoll_event*)&_epoll_events[0],
                static_cast<int>(_epoll_events.size()),
                timeout
            );
            if (num_events == -1) {
                if (errno == EINTR) continue;
                std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
            }
            if (num_events == 0) {
            }
            if (num_events > 0) {
                for (int i = 0; i < num_events; ++i) {
                    auto &[events, data] = _epoll_events[i];
                    if(data.fd) {
                        process_io_event(data.fd);
                    }
                }
                if (num_events >= static_cast<int>(_epoll_events.size())) {
                    _epoll_events.resize(_epoll_events.size() * 2);
                }
            }
        }
    }

    void EventLoop::stop() {
        _running = false;
    }

    void *EventLoop::owner() const {
        return _owner;
    }

    IOEvent *EventLoop::create_io_event(const io_callback& callback, void *data) {
        return new IOEvent(this, callback, data);
    }

    void EventLoop::start_io_event(IOEvent *io_event, const int fd, const int mask) {
        auto iter = _events.find(fd);
        if (iter != _events.end()) {
            if (iter->second->events == mask) {
                return;
            }
            if (iter->second->events != mask) {
                io_event->events = mask;

                epoll_event event{};
                event.events = io_event->events;
                event.data.fd = io_event->fd;

                _events[io_event->fd] = io_event;
                if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
                    std::cerr << "Failed to mod fd to epoll: " << strerror(errno) << std::endl;
                    return;
                }
                return;
            }
        }
        io_event->fd = fd;
        io_event->events = mask;

        struct epoll_event event{};
        event.data.fd = fd;
        event.events = mask;

        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
            std::cerr << "Failed to add fd to epoll: " << strerror(errno) << std::endl;
            ::close(fd);
            return;
        }

        _events[io_event->fd] = io_event;
    }

    void EventLoop::stop_io_event(const IOEvent *io_event) {
        if (const auto iter= _events.find(io_event->fd); iter == _events.end()) {
            return;
        }
        epoll_event ev{};
        ev.data.fd = io_event->fd;

        if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, io_event->fd, &ev) == -1) {
            std::cerr << "Failed to stop fd to epoll: " << strerror(errno) << std::endl;
            return;
        }
        _events.erase(io_event->fd);
    }

    void EventLoop::delete_io_event(const IOEvent *io_event) {
        stop_io_event(io_event);
        ::close(io_event->fd);
        delete io_event;
    }

    TimerEvent *EventLoop::create_timer_event(const timer_callback &cb, void *data, bool repeat) {
        return new TimerEvent(this, cb, data, repeat);
    }

    void EventLoop::start_timer_event(TimerEvent *timer_event, const unsigned int usec) {
        timer_event->active = true;
        timer_event->usec = usec;
        timer_event->start_time = std::chrono::steady_clock::now();
        _timer_queue.push(timer_event);
    }

    void EventLoop::stop_timer_event(TimerEvent *timer_event) {
        timer_event->active = false;
        timer_event->repeat = false;
    }

    void EventLoop::delete_timer_event(TimerEvent *timer_event) {
        stop_timer_event(timer_event);
        delete timer_event;
    }

    unsigned long EventLoop::now() {
        using namespace std::chrono;
        const auto now = std::chrono::system_clock::now();
        const auto milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return milliseconds_since_epoch;
    }

    void EventLoop::process_io_event(const int fd) {
        auto iter = _events.find(fd);
        if (iter != _events.end()) {
            auto* io_event = iter->second;
            if(io_event && io_event->callback) {
                io_event->callback(io_event->loop, io_event, io_event->fd, io_event->events, io_event->data);
            }
        }
    }

    void EventLoop::process_timer_event() {
        auto now = std::chrono::steady_clock::now();
        while (!_timer_queue.empty()) {
            TimerEvent* timer = _timer_queue.top();
            if (timer->get_expiry_time() <= now) {
                _timer_queue.pop();
                if (timer->active) {
                    timer->callback(timer->loop, timer, timer->data);
                    if(timer->repeat) {
                        timer->start_time = std::chrono::steady_clock::now();  // Reset the start time for the next interval
                        _timer_queue.push(timer);
                    } else {
                        stop_timer_event(timer);
                    }
                }
            } else {
                break;
            }
        }
    }
}
