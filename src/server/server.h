//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once

#include <core.h>
#include "worker.h"

using namespace core;

namespace penny {

    class Server {
    private:
        int _listen_fd = -1;
        int _notify_send_fd = -1;
        int _notify_receive_fd = -1;

        EventLoop *_loop = nullptr;
        IOEvent *_io_event = nullptr;
        IOEvent *_pipe_event = nullptr;
        std::thread *_thread = nullptr;

        std::vector<Worker*> _workers;
    public:
        enum : int {
            QUIT = 0
        };

        // handle io receive message
        friend void accept_new_conn(EventLoop *loop, IOEvent *event, int fd, int events, void *data);

        Server();

        ~Server();

        int init();

        int start();

        void join();

        int quit();

    private:
        // stop logic
        void _stop();
        // init logic
        int _create_worker(int worker_id);

    private:
        // handle notify
        int _notify_send(int msg);
        void _notify_receive(EventLoop *loop, IOEvent *event, int fd, int events, void *data);

    };
} // namespace penny::server