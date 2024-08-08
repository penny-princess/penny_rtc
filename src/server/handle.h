//
// Created by penny_developers@outlook.com on 2024-08-02.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
// 


#pragma once
#include "tcp_connection.h"

namespace penny {
    class Handle: public TcpConnection{
    public:
        Handle() = default;
        ~Handle() override;

        void connect() override;

        int handle0(const std::string &header,const std::string &body) override;

        void error() override;


    public:


    };
}
