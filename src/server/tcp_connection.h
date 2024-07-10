#pragma once
#include <core.h>

using namespace core;
namespace penny {
    
    class TcpConnection {
    private:
        EventLoop* _loop = nullptr;
        IOEvent* io_event = nullptr;
    public:
        TcpConnection(EventLoop* loop);
        ~TcpConnection();

        void request();
        void response();
    };    
} // namespace penny
