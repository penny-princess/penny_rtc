//
// Created by penny_developers@outlook.com on 2024-08-02.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
// 


#pragma once
#include <core.h>
#include <list>
#include "protocol.h"


using namespace core;

namespace penny {

    class TcpConnection {
    public:
        enum {
            STATE_HEAD = 0,
            STATE_BODY = 1
        };

        TcpConnection() = default;
        virtual ~TcpConnection() = default;
        using timeout_callback = std::function<void(TcpConnection* c)>;

    public:
        virtual void connect() = 0;
        virtual int handle0(const std::string& header, const std::string& body) = 0;
        virtual void error() = 0;

    public:
        virtual void timeout(TcpConnection* c) { clear_connect(c); }
        virtual void set_timeout(timeout_callback callback) { clear_connect = std::move(callback); }

    public:
        int fd;
        char ip[64];
        int port;
        IOEvent* io_event = nullptr;
        TimerEvent* timer_event = nullptr;
        timeout_callback clear_connect;

        size_t bytes_expected = HEADER_SIZE;
        size_t bytes_processed = 0;
        int current_state = STATE_HEAD;
        unsigned long last_interaction = 0;
        std::list<std::string> reply_list;
        size_t cur_resp_pos = 0;

    private:
        int _interval;

    public:
        std::string _request_buffer;
        std::string _response_buffer;
    };

}
