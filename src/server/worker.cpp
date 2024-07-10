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
        if(_thread) {
            delete _thread;
            _thread = nullptr;
        }

        if(_loop) {
            delete _loop;
            _loop = nullptr;
        }
    }

    int Worker::init() {
        LOG(INFO) << "worker init starting, worker_id:" << _worker_id;
        int fds[2];
        if(pipe(fds)) {
            LOG(WARN) << "pipe create error: " << strerror(errno) << ", errno: " << errno;
            return -1;
        }
        _notify_receive_fd = fds[0];
        _notify_send_fd = fds[1];
        _pipe_event = _loop->create_io_event(std::bind(&Worker::_notify_receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), this);
        _loop->start_io_event(_pipe_event, _notify_receive_fd, EventLoop::READ);
        LOG(INFO) << "worker init end, worker_id:" << _worker_id;
        return 0;
    }

    int Worker::start() {
        if(_thread) {
            LOG(WARN) << "thread already start, worker_id:" << _worker_id;
            return -1;
        }
        _thread = new std::thread([=]() {
            LOG(INFO) << "worker start";
            _loop->start();
            LOG(INFO) << "worker end, worker_id:" << _worker_id;
        });
        return 0;
    }

    void Worker::join() {
        if(_thread && _thread->joinable()) {
            _thread->join();
        }
    }
    
    int Worker::quit() {
        return _notify_send(QUIT);
    }

    int Worker::new_connection(int fd) {
        _lock_free_queue.produce(fd);
        return _notify_send(NEW_CONNECTION);
    }

    void Worker::_handle_new_connection(int client_fd) {
        LOG(INFO) << "_worker_id: ["<< _worker_id << "]" << "fd: ["<< client_fd << "]";

        sock_set_nodelay(client_fd);
        sock_setnonblock(client_fd);
    }

    void Worker::_stop() {
        LOG(INFO) << "worker stopping, worker_id:" << _worker_id;
        close(_notify_receive_fd);
        close(_notify_send_fd);

        _loop->delete_io_event(_pipe_event);
        _loop->stop();
        LOG(INFO) << "worker stopped, worker_id:" << _worker_id;
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
        LOG(INFO) << "test";
        if (ret != sizeof(int)) {
            LOG(WARN) << "read from pipe error: " << strerror(errno) << ", errno: " << errno;
            return;
        }
        switch (msg) {
            case QUIT:
                _stop();
                break;
            case NEW_CONNECTION:
                int fd;
                if(_lock_free_queue.consume(&fd)) {
                    _handle_new_connection(fd);
                }
                break;
            default:
                LOG(WARN) << "unknown msg: " << msg;
                break;
        }
    }
}