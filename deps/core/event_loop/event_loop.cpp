//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "event_loop.h"

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

    int EventLoop::stop_io_event(IOEvent* io_event, int fd, int mask) {
        epoll_event ev = {};
        ev.events = mask;
        ev.data.ptr = io_event;
        int ret = epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &ev);
        if ( ret == -1) {
            return -1;
        }
        return 0;
    }

    void EventLoop::delete_io_event(IOEvent* io_event) {
        delete io_event;
    }
}