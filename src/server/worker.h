//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once

#include <core.h>
#include "xhead.h"

using namespace core;

namespace penny {

    class Worker {
    private:
        int _worker_id = -1;
        int _notify_receive_fd = -1;
        int _notify_send_fd = -1;

        EventLoop *_loop = nullptr;
        IOEvent *_pipe_event = nullptr;
        IOEvent *_io_event = nullptr;
        std::thread *_thread = nullptr;

        LockFreeQueue<int> _lock_free_queue;

    public:
        enum {
            QUIT = 0,
            NEW_CONNECTION = 1
        };

        Worker(int worker_id);

        ~Worker();

        int init();

        int start();

        void join();

        int quit();

        int new_connection(int fd);

    private:
        // stop logic
        void _stop();
        // server business logic
        void _handle_new_connection(int client_fd);
    private:
        // handle notify
        int _notify_send(int msg);
        void _notify_receive(EventLoop *, IOEvent *, int fd, int, void *);

    };
}



