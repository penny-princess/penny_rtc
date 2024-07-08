//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "worker.h"

namespace penny {

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

    Worker::Worker() {
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

    void Worker::start() {
        _thread = new std::thread([this] {
            LOG(INFO) << "worker start";
            _loop->start();
            LOG(INFO) << "worker end";
        });
    }

    void Worker::join() {
        if(_thread && _thread->joinable()) {
            _thread->join();
        }
    }

    int Worker::quit() {

    }

    void Worker::_stop() {

    }

    int Worker::_notify_send(int msg) {
        int ret = write(_notify_send_fd, &msg, sizeof(int));
        if(ret != sizeof(int)) {
            return -1;
        }
        return 0;
    }
}