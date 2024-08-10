//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once

#include <core.h>
#include "protocol.h"
#include "handle.h"

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
        std::vector<Handle*> _connections;

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
        //handle delete connection
        void _close_connection(TcpConnection* c);

        // handle tcp content
        void _handle_request(EventLoop *, IOEvent *, int, int, void *);
        // handle tcp request
        void _read_request(int fd);
        static int _handle_request_buffer(TcpConnection* c);
        // handle tcp response
        void _write_response();

        // handle tcp timeout
        void _handle_timeout(EventLoop *loop, TimerEvent *w, void *data);
    private:
        // stop logic
        void _stop() const;
        // server business logic
        void _handle_new_connection(int client_fd);
    private:
        // handle notify
        [[nodiscard]] int _notify_send(int msg) const;
        void _notify_receive(EventLoop *, IOEvent *, int fd, int, void *);

    };
}



