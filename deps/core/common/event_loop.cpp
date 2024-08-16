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
                auto time_until_next = std::chrono::duration_cast<std::chrono::microseconds>(next_timer->get_expiry_time() - now).count();
                timeout = std::max(0, static_cast<int>(time_until_next / 1000));  // 将微秒转换为毫秒
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

    void EventLoop::start_io_event(IOEvent *io_event, int fd, int mask) {
        if ( const auto iter = _io_events_map.find(fd); iter != _io_events_map.end()) {
            struct epoll_event event{};
            const uint32_t active_events = iter->second->events;
            const uint32_t events = active_events | mask;
            if (events == active_events) {
                return;
            }
            std::memset(&event, 0, sizeof(event));
            event.events = events;
            event.data.fd = fd;
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
                throw std::runtime_error("Failed to modify epoll event");
            }
            io_event->events = events;
            _io_events_map[fd] = io_event;
        } else {
            struct epoll_event event{};
            std::memset(&event, 0, sizeof(event));
            event.events = mask;
            event.data.fd = fd;
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
                throw std::runtime_error("Failed to add epoll event");
            }
            io_event->fd = fd;
            io_event->events = event.events;
            _io_events_map[fd] = io_event;
        }
    }

    void EventLoop::stop_io_event(const IOEvent *io_event,const uint32_t mask) {
        const auto iter= _io_events_map.find(io_event->fd);
        if (iter == _io_events_map.end()) {
            return;
        }

        if (const uint32_t remaining_events = iter->second->events & ~mask; remaining_events != 0) {
            epoll_event event{};
            event.events = remaining_events;
            event.data.fd = io_event->fd;
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, io_event->fd, &event) == -1) {
                throw std::runtime_error("Failed to modify epoll event");
            }
        } else {
            if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, io_event->fd, nullptr) == -1) {
                throw std::runtime_error("Failed to delete epoll event");
            }
            _io_events_map.erase(io_event->fd);
        }
    }

    void EventLoop::delete_io_event(const IOEvent *io_event) {
        if (const auto iter= _io_events_map.find(io_event->fd); iter == _io_events_map.end()) {
            return;
        }
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, io_event->fd, nullptr) == -1) {
            throw std::runtime_error("Failed to delete epoll event");
        }
        ::close(io_event->fd);
        _io_events_map.erase(io_event->fd);
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
        timer_event->deleted = true;
        stop_timer_event(timer_event);
    }

    unsigned long EventLoop::now() {
        const auto now = std::chrono::system_clock::now();
        const auto microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        return microseconds_since_epoch;
    }

    void EventLoop::process_io_event(const int fd) {
        auto iter = _io_events_map.find(fd);
        if (iter != _io_events_map.end()) {
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

            if (timer && timer->active) {
                if (timer->get_expiry_time() <= now) {
                    _timer_queue.pop();  // 先从队列中移除

                    bool repeat = timer->repeat;  // 缓存repeat标志
                    timer->callback(timer->loop, timer, timer->data);  // 执行回调

                    if (repeat && !timer->deleted) {  // 如果是重复定时器且仍然活跃
                        // 计算下一个到期时间并重新启动定时器
                        timer->start_time = std::chrono::steady_clock::now();  // 重置开始时间
                        _timer_queue.push(timer);  // 重新加入队列
                    } else {
                        stop_timer_event(timer);  // 停止定时器
                        delete timer;
                    }
                } else {
                    break;  // 下一个定时器尚未到期
                }
            } else {
                _timer_queue.pop();  // 如果定时器不再活跃，直接移除
            }
        }
    }
}
