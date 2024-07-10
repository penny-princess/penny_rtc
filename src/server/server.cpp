//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "server.h"

namespace penny {

    Server::Server() : _loop(new EventLoop(this)) {}

    Server::~Server() {
        if (_loop) {
            delete _loop;
            _loop = nullptr;
        }

        if (_thread) {
            delete _thread;
            _thread = nullptr;
        }
    }

    int Server::init() {
        LOG(INFO) << "server init starting";
        int fds[2];
        if (pipe(fds)) {
            LOG(WARN) << "pipe create error: " << strerror(errno) << ", errno: " << errno;
            return -1;
        }
        _notify_receive_fd = fds[0];
        _notify_send_fd = fds[1];
        _pipe_event = _loop->create_io_event(std::bind(&Server::_notify_receive,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), this);
        _loop->start_io_event(_pipe_event, _notify_receive_fd, EventLoop::READ);

        _listen_fd = create_tcp_server("0.0.0.0", 8000);
        LOG(ERROR)  << "_listen_fd: "<< _listen_fd;
        if(_listen_fd == -1) {
            return -1;
        }
        _io_event = _loop->create_io_event(std::bind(&Server::_io_listen,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), this);
        _loop->start_io_event(_io_event, _listen_fd, EventLoop::READ);
        for(int i = 0; i < 4; i++) {
            int ret = _create_worker(i);
            if(ret == -1) return -1;
        }
        LOG(INFO) << "server init end";
        return 0;
    }

    int Server::start() {
        if (_thread) {
            LOG(WARN) << "thread already start";
            return -1;
        }
        _thread = new std::thread([=]() {
            LOG(INFO) << "server starting";
            _loop->start();
            LOG(INFO) << "server end";
        });
        return 0;
    }

    void Server::join() {
        if (_thread && _thread->joinable()) {
            _thread->join();
        }
    }

    void Server::_stop() {
        LOG(INFO) << "server stopping";

        _loop->delete_io_event(_pipe_event);
        _loop->delete_io_event(_io_event);
        _loop->stop();

        close(_listen_fd);
        close(_notify_receive_fd);
        close(_notify_send_fd);

        for(const auto& item: _workers) {
            item->quit();
            item->join();
        }
        LOG(INFO) << "server stopped";
    }

    int Server::_create_worker(int worker_id) {
        Worker* worker = new Worker(worker_id);
        int ret = worker->init();
        if(-1 == ret) return -1;
        ret = worker->start();
        if(-1 == ret) return -1;
        _workers.push_back(worker);
        return 0;
    }

    void Server::_io_listen(EventLoop *, IOEvent *, int fd, int, void *) {
        char ip[128];
        int port;
        int client_fd = tcp_accept(fd, ip, &port);

        if(_worker_index >= _workers.size()) {
            _worker_index = 0;
        }
        
        LOG(INFO) << "worker.size: [" << _workers.size() << "],_worker_index: [" << _worker_index << "]";
        Worker* worker = _workers[_worker_index];
        worker->new_connection(client_fd);
        _worker_index++;
    }

    int Server::_notify_send(int msg) {
        int ret = write(_notify_send_fd, &msg, sizeof(int));
        if(ret != sizeof(int)) {
            return -1;
        }
        return 0;
    }

    void Server::_notify_receive(EventLoop *, IOEvent *, int fd, int, void *) {
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
            default:
                LOG(WARN) << "unknown msg: " << msg;
                break;
        }
    }

    int Server::quit() {
        return _notify_send(Server::QUIT);
    }
} // namespace penny