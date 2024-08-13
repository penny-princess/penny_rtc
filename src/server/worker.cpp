//
// Created by penny_developers@outlook.com on 2024-07-08.
// Copyright (c) 2024 penny_developers@outlook.com . All rights reserved.
//

#include "worker.h"
#include "handle.h"

namespace penny {

    Worker::Worker(int worker_id): _worker_id(worker_id), _loop(new EventLoop(this)) {
    }

    Worker::~Worker() {
        if(_loop) {
            delete _loop;
            _loop = nullptr;
        }

        if(_thread) {
            delete _thread;
            _thread = nullptr;
        }
    }

    int Worker::init() {
        LOG(INFO) << "worker init starting, worker_id:" << _worker_id;
        int fds[2];
        if(pipe(fds)) {
            LOG(WARN) << "pipe create error: " << strerror(errno) << ", errno: " << errno;
            return -1;
        }
        _notify_receive_fd = fds[0];
        _notify_send_fd = fds[1];
        _pipe_event = _loop->create_io_event(std::bind_front(&Worker::_notify_receive, this), this);
        _loop->start_io_event(_pipe_event, _notify_receive_fd, EventLoop::READ);
        LOG(INFO) << "worker init end, worker_id:" << _worker_id;
        return 0;
    }

    int Worker::start() {
        if(_thread) {
            LOG(WARN) << "thread already start, worker_id:" << _worker_id;
            return -1;
        }
        _thread = new std::thread([this]() {
            LOG(INFO) << "worker start";
            _loop->start();
            LOG(INFO) << "worker end, worker_id:" << _worker_id;
        });
        return 0;
    }

    void Worker::join() {
        if(_thread && _thread->joinable()) {
            _thread->join();
        }
    }
    
    int Worker::quit() {
        return _notify_send(QUIT);
    }

    int Worker::new_connection(int fd) {
        _lock_free_queue.produce(fd);
        return _notify_send(NEW_CONNECTION);
    }

    void Worker::_handle_new_connection(int client_fd) {
        LOG(DEBUG) << "_worker_id: ["<< _worker_id << "]" << "fd: ["<< client_fd << "]";
        sock_set_nodelay(client_fd);
        sock_setnonblock(client_fd);

        Handle* handle = new Handle();
        sock_peer_to_str(client_fd, handle->ip, &(handle->port));
        handle->set_timeout(std::bind_front(&Worker::_close_connection,this));
        handle->set_response(std::bind_front(&Worker::_handle_response,this));
        handle->set_response(std::bind_front(&Worker::_handle_response,this));
        handle->fd = client_fd;
        handle->connect();
        handle->io_event = _loop->create_io_event(std::bind_front(&Worker::_handle_request, this), this);
        _loop->start_io_event(handle->io_event, client_fd, EventLoop::READ);

        handle->timer_event = _loop->create_timer_event(std::bind_front(&Worker::_handle_timeout, this ), handle, true);
        _loop->start_timer_event(handle->timer_event, 1* 1000);

        handle->last_interaction = EventLoop::now();
        handle->last_interaction = EventLoop::now();

        if ((size_t)client_fd >= _connections.size()) {
            _connections.resize(client_fd * 2, nullptr);
        }

        _connections[handle->fd] = handle;
    }

    void Worker::_handle_request(EventLoop*, IOEvent*, int fd, int event, void*) {
        if(event & EventLoop::READ) {
            _read_request(fd);
        }

        if(event & EventLoop::WRITE) {
            _write_response(fd);
        }

        if(event & EventLoop::WRITE) {
            _write_response(fd);
        }
    }

    void Worker::_read_request(int fd) {
        LOG(DEBUG) << "signaling worker " << _worker_id << " receive read event, fd: " << fd;
        if (fd < 0 || (size_t)fd >= _connections.size()) {
            LOG(WARN) << "invalid fd: " << fd;
            return;
        }

        TcpConnection* c = _connections[fd];
        int nread = 0;
        const int read_len = static_cast<int>(static_cast<int>(c->bytes_expected));
        const int qb_len = static_cast<int>(static_cast<int>(c->_request_buffer.length()));
        c->_request_buffer.reserve(qb_len + read_len);
        nread = sock_read_data(fd, c->_request_buffer.data() + qb_len, read_len);

        c->last_interaction = EventLoop::now();
        c->last_interaction = EventLoop::now();

        if (-1 == nread) {
            _close_connection(c);
            return;
        } else if (nread > 0) {
            c->_request_buffer.append(c->_request_buffer.data() + qb_len, nread);
        }

        int ret = _handle_request_buffer(c);
        if (ret != 0) {
            _close_connection(c);
            LOG(ERROR) << "WLF";
            LOG(ERROR) << "WLF";
            return;
        }
    }

    int Worker::_handle_request_buffer(TcpConnection *c) {
        while (c->_request_buffer.length() >= c->bytes_processed + c->bytes_expected) {
            const auto* head = reinterpret_cast<protocol_header_t*>(c->_request_buffer.data());
            if (TcpConnection::STATE_HEAD == c->current_state) {
                if (PROTOCOL_CHECK_NUM != head->protocol_check_num) {
                    LOG(WARN) << "invalid data, fd: " << c->fd;
                    return -1;
                }

                c->current_state = TcpConnection::STATE_BODY;
                c->bytes_processed = HEADER_SIZE;
                c->bytes_expected = head->body_len;
            } else {
                std::string header(c->_request_buffer.data(), HEADER_SIZE);
                std::string body(c->_request_buffer.data() + HEADER_SIZE, head->body_len);
                int ret = c->handle0(header, body);
                if (ret != 0) {
                    return -1;
                }

                // 短连接处理
                c->bytes_processed = 65535;
            }
        }

        return 0;
    }

    void Worker::_write_response(int fd) {
        if (fd <= 0 || (size_t)fd >= _connections.size()) {
            return;
        }

        TcpConnection* c = _connections[fd];
        if (!c) {
            return;
        }
        while (!c->reply_list.empty()) {
            std::string reply = c->reply_list.front();
            LOG(INFO) << "shit try agin:" << c->is_fd_valid(fd) << ", and fd is:" << fd << "and c->fd is:" << c->is_fd_valid(c->fd);
            int nwritten = sock_write_data(c->fd, reply.data() + c->cur_resp_pos, reply.size() - c->cur_resp_pos);
            if (-1 == nwritten) {
                _close_connection(c);
                return;
            } else if (0 == nwritten) {
                LOG(WARN) << "write zero bytes, fd: " << c->fd << ", worker_id: " << _worker_id;
            } else if ((nwritten + c->cur_resp_pos) >= reply.size()) {
                // 写入完成
                c->reply_list.pop_front();
                c->cur_resp_pos = 0;
                LOG(INFO) << "write finished, fd: " << c->fd << ", worker_id: " << _worker_id;
            } else {
                c->cur_resp_pos += nwritten;
            }
        }

        c->last_interaction = EventLoop::now();
        if (c->reply_list.empty()) {
            _loop->stop_io_event(c->io_event);
            LOG(INFO) << "stop write event, fd: " << c->fd << ", worker_id: " << _worker_id;
        }

    }

    void Worker::_close_connection(TcpConnection* c) {
        LOG(INFO) << "close connection, fd: " << c->fd;
        if(c) {
            if(_loop) {
                if(c->timer_event) {
                    _loop->delete_timer_event(c->timer_event);
                }
                if(c->io_event) {
                    _loop->delete_io_event(c->io_event);
                }
            }
            _connections[c->fd] = nullptr;
            delete c;
        }
    }

    void Worker::_handle_timeout(EventLoop *, TimerEvent *, void *data) {
        auto* handle = (Handle*)(data);
        if (EventLoop::now() - handle->last_interaction >= 5 * 1000) {
            handle->timeout(handle);
        }
    }

    void Worker::_handle_response(TcpConnection* c) {
        if(c && c->io_event) {
            _loop->start_io_event(c->io_event, c->fd, EventLoop::WRITE);
        }
    }

    void Worker::_stop() const {
        LOG(INFO) << "worker stopping, worker_id:" << _worker_id;
        if(_loop) {
            if(_pipe_event) {
                _loop->delete_io_event(_pipe_event);
            }
            _loop->stop();
        }
        ::close(_notify_send_fd);
        LOG(INFO) << "worker stopped, worker_id:" << _worker_id;
    }

    int Worker::_notify_send(const int msg) const {
        if(write(_notify_send_fd, &msg, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        return 0;
    }

    void Worker::_notify_receive(EventLoop *, IOEvent *, const int fd, int, void *) {
        int msg;
        if (const int ret = read(fd, &msg, sizeof(int)); ret != sizeof(int)) {
            LOG(WARN) << "read from pipe error: " << strerror(errno) << ", errno: " << errno;
            return;
        }
        switch (msg) {
            case QUIT:
                _stop();
                break;
            case NEW_CONNECTION:
                int fd;
                if(_lock_free_queue.consume(&fd)) {
                    _handle_new_connection(fd);
                }
                break;
            default:
                LOG(WARN) << "unknown msg: " << msg;
                break;
        }
    }
}