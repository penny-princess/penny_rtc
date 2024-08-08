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

namespace penny {
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
            memset(&_epoll_events[0], 0x00,sizeof (struct epoll_event)*_epoll_events.size());
            const int num_events = epoll_wait(
                _epoll_fd,
                (struct epoll_event*)&_epoll_events[0],
                static_cast<int>(_epoll_events.size()),
                -1
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
                        process_timer_event(data.fd);
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

    IOEvent *EventLoop::create_io_event(io_callback callback, void *data) {
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

                struct epoll_event event{};
                memset(&event, 0x00, sizeof(struct epoll_event));
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
        memset(&event, 0x00, sizeof(struct epoll_event));
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
        struct epoll_event ev;
        memset(&ev, 0x00, sizeof(struct epoll_event));
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
        timer_event->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timer_event->fd == -1) {
            std::cerr << "Failed to create timerfd: " << strerror(errno) << std::endl;
            return;
        }

        itimerspec new_value{};
        new_value.it_value.tv_sec = usec / 1000000;
        new_value.it_value.tv_nsec = (usec % 1000000) * 1000;
        new_value.it_interval.tv_sec = timer_event->repeat ? new_value.it_value.tv_sec : 0;
        new_value.it_interval.tv_nsec = timer_event->repeat ? new_value.it_value.tv_nsec : 0;


        if (timerfd_settime(timer_event->fd, 0, &new_value, nullptr) == -1) {
            std::cerr << "Failed to set timerfd: " << strerror(errno) << std::endl;
            ::close(timer_event->fd);
            return;
        }

        epoll_event event{};
        event.data.fd = timer_event->fd;
        event.events = EPOLLIN;

        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, timer_event->fd, &event) == -1) {
            std::cerr << "Failed to add timerfd to epoll: " << strerror(errno) << std::endl;
            ::close(timer_event->fd);
            return;
        }

        _timers[timer_event->fd] = timer_event;
    }

    void EventLoop::stop_timer_event(const TimerEvent *timer_event) {
        if (const auto iter = _timers.find(timer_event->fd); iter == _timers.end()) {
            return;
        }
        struct epoll_event ev;
        ev.data.fd = timer_event->fd;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, timer_event->fd, &ev) == -1) {
            std::cerr << "Failed to remove timerfd from epoll: " << strerror(errno) << std::endl;
            return;
        }
        _timers.erase(timer_event->fd);
    }

    void EventLoop::delete_timer_event(const TimerEvent *timer_event) {
        stop_timer_event(timer_event);
        ::close(timer_event->fd);
        delete timer_event;
    }

    unsigned long EventLoop::now() {
        using namespace std::chrono;
        auto now = steady_clock::now().time_since_epoch();
        double now_double = duration_cast<duration<double>>(now).count();
        return static_cast<unsigned long>(now_double * 1000000);
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

    void EventLoop::process_timer_event(const int fd) {
        if (_timers.find(fd) != _timers.end()) {
            const auto timer_event = _timers[fd];
            if(timer_event && timer_event) {
                timer_event->callback(timer_event->loop, timer_event, timer_event->data);
            }
            if (!timer_event->repeat) {
                delete_timer_event(timer_event);
            }
        }
    }
}
