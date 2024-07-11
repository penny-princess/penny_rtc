//
// Created by penny_developers@outlook.com on 2024-07-11.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
// 
#pragma once
#include <core.h>
#include "xhead.h"

using namespace core;

namespace penny {
    class TcpConnection {
    using request_callback_func = std::function<int(std::string, std::string, TcpConnection*)>;
    enum {
        HEAD_STATUS = 0,
        BODY_STATUS = 1
    };
    public:

        int fd;
        char ip[64];
        int port;

        int bytes_expected;
        size_t bytes_processed = 0;
        int current_state = HEAD_STATUS;
        Buffer request_buffer;
        unsigned long last_interaction = 0;
    private:
        EventLoop* _loop = nullptr;
        std::map<int, TcpConnection *> _connections;
        IOEvent* _io_event = nullptr;
        TimerEvent* _timer_watcher = nullptr;
        request_callback_func _func;
    public:
        TcpConnection(EventLoop *pLoop, int i, const std::map<int, TcpConnection *> connections);
        ~TcpConnection();
        void request_callback(request_callback_func func);
        void response(std::string response_data);
    private:
        void _handle_connection(EventLoop *loop, IOEvent *event, int fd, int events, void *data);
        void _handle_request(int fd);
        int _unpacking(TcpConnection* connection);
    };
}
