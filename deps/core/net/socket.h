//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#pragma once

#include "pch.h"
#include "core.h"

namespace core {

    #define NET_LOG LOG
    #define NET_WARN WARN
    #define NET_ERROR ERROR


    int create_tcp_server(const char *addr, int port);

    int tcp_accept(int sock, char *host, int *port);

    int sock_setnonblock(int sock);

    int sock_setnodelay(int sock);

    int sock_peer_to_str(int sock, char *ip, int *port);

    int sock_read_data(int sock, char *buf, size_t len);

    int sock_write_data(int sock, const char *buf, size_t len);

}
