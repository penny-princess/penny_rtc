//
// Created by BUHE on 2024/8/11.
//
#include "tcp_connection.h"

namespace penny {
    void TcpConnection::response(const std::string &buffer) {

        _response_buffer.append(_request_buffer.data(), HEADER_SIZE);
        auto* head = reinterpret_cast<protocol_header_t*>(_response_buffer.data());
        head->body_len = static_cast<int32_t>(buffer.size());
        _response_buffer.append(buffer);
        reply_list.push_back(_response_buffer);
        if(response_cb) {
            response_cb(this);
        }
    }
}
