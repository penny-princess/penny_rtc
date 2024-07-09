//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "worker.h"

namespace penny {

    Worker::Worker(int worker_id) {
        _worker_id = worker_id;
        _loop = new EventLoop(this);
    }

    Worker::~Worker() {
        if(_loop != nullptr) {
            delete _loop;
            _loop = nullptr;
        }

        if(_thread != nullptr) {
            delete _thread;
            _thread = nullptr;
        }
    }

    int Worker::init() {
        LOG(INFO) << "worker init starting";
        int fds[2];
        if(pipe(fds)) {
            LOG(WARN) << "pipe create error: " << strerror(errno) << ", errno: " << errno;
            return -1;
        }
        _notify_receive_fd = fds[0];
        _notify_send_fd = fds[1];
        _pipe_event = _loop->create_io_event(std::bind(&Worker::_notify_receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), this);
        _loop->start_io_event(_pipe_event, _notify_receive_fd, EventLoop::READ);
        LOG(INFO) << "worker init end";
        return 0;
    }

    int Worker::start() {
        if(_thread) {
            LOG(WARN) << "thread already start";
            return -1;
        }
        _thread = new std::thread([=]() {
            LOG(INFO) << "worker start";
            _loop->start();
            LOG(INFO) << "worker end";
        });
        return 0;
    }

    void Worker::join() {
        if(_thread && _thread->joinable()) {
            _thread->join();
        }
    }

    void Worker::_stop() {
        LOG(INFO) << "worker stopping";
        close(_notify_receive_fd);
        close(_notify_send_fd);

        _loop->delete_io_event(_pipe_event);
        _loop->stop();
        LOG(INFO) << "worker stopped";
    }

    int Worker::_notify_send(int msg) {
        int ret = write(_notify_send_fd, &msg, sizeof(int));
        if(ret != sizeof(int)) {
            return -1;
        }
        return 0;
    }

    void Worker::_notify_receive(EventLoop *, IOEvent *, int fd, int, void *) {
        int msg;
        int ret = read(fd, &msg, sizeof(int));
        if (ret != sizeof(int)) {
            LOG(WARN) << "read from pipe error: " << strerror(errno) << ", errno: " << errno;
            return;
        }
        switch (msg) {
            case QUIT:
                _stop();
                break;
            case NEW_CONNECTION:
                break;
            default:
                LOG(WARN) << "unknown msg: " << msg;
                break;
        }
    }

    int Worker::quit() {
        return 0;
    }
}