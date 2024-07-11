//
// Created by penny_developers@outlook.com on 2024-07-11.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
// 

#include "tcp_connection.h"

namespace penny {
    TcpConnection::TcpConnection(EventLoop* loop,int fd, const std::map<int, TcpConnection *> connections):fd(fd) {
        _loop = loop;
        _connections = connections;
        _io_event = loop->create_io_event(std::bind(&TcpConnection::_handle_connection,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), this);
    }

    TcpConnection::~TcpConnection() {

    }

    void TcpConnection::_handle_request(int fd) {
        LOG(INFO) << "receive read event, fd: " << fd;
        if (fd < 0 || (size_t)fd >= _connections.size()) {
            LOG(WARN) << "invalid fd: " << fd;
            return;
        }

        TcpConnection* conn = _connections[fd];
        int nread = 0;
        int read_len = conn->bytes_expected;
        int qb_len = buffer_len(conn->request_buffer);
        nread = sock_read_data(fd, conn->request_buffer + qb_len, read_len);
        conn->last_interaction = _loop->now();

        LOG(INFO) << "sock read data, len: " << nread;
        if (-1 == nread) {
//            _close_conn(c);
            return;
        } else if (nread > 0) {
            buffer_allocate(conn->request_buffer, nread);
        }

        int ret = _unpacking(conn);
        if (ret != 0) {
//            _close_conn(c);
            return;
        }

    }

    void TcpConnection::request_callback(request_callback_func func) {
        _func = func;
    }

    void TcpConnection::response(std::string response_data) {

    }

    void TcpConnection::_handle_connection(EventLoop *, IOEvent *, int fd, int events, void *) {
        if(events & EventLoop::READ) {
            // request message logic
            _handle_request(fd);
        }

        if(events & EventLoop::WRITE) {
            // response message logic
        }
    }

    int TcpConnection::_unpacking(TcpConnection *connection) {
        while (buffer_len(connection->request_buffer) >= connection->bytes_processed + connection->bytes_expected) {
            protocol_header_t* head = (protocol_header_t*)(connection->request_buffer);
            if (TcpConnection::HEAD_STATUS == connection->current_state) {
                if (XHEAD_MAGIC_NUM != head->magic_num) {
                    LOG(WARN) << "invalid data, fd: " << connection->fd;
                    return -1;
                }

                connection->current_state = TcpConnection::BODY_STATUS;
                connection->bytes_processed = XHEAD_SIZE;
                connection->bytes_expected = head->body_len;
            } else {
                std::string header(connection->request_buffer, XHEAD_SIZE);
                std::string body(connection->request_buffer + XHEAD_SIZE, head->body_len);

                int ret = _func(header, body, connection);
                if (ret != 0) {
                    return -1;
                }
                // 短连接处理
                connection->bytes_processed = 65535;
            }
        }
        return 0;
    }

}