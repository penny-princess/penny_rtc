//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once

#include <core.h>

using namespace core;

namespace penny {

    class Worker {
    private:
        int _notify_receive_fd = -1;
        int _notify_send_fd = -1;

        EventLoop *_loop = nullptr;
        IOEvent *_io_event = nullptr;
        IOEvent *_pipe_watcher = nullptr;
        std::thread *_thread = nullptr;

    public:
        enum {
            QUIT = 0,
            NEW_CONNECTION = 1
        };
        Worker();
        ~Worker();

        void start();

        void join();

        int quit();

    private:
        // business logic
        void _handle_new_connection();

    private:
        // handle notify
        int _notify_send(int msg);
        void _notify_receive(EventLoop *, IOEvent *, int fd, int, void *);

        // stop logic
        void _stop();

    };
}



