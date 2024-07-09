#include <iostream>
#include <csignal>
#include "server/server.h"

using namespace penny;
Server* server = nullptr;
Logger* logger = nullptr;

static void process_signal(int sig) {
    if (SIGINT == sig || SIGTERM == sig) {
        if (server) {
            server->quit();
        }
    }
}

int init_server() {
    server = new Server();
    int ret = server->init();
    if (ret != 0) {
        return -1;
    }
    ret = server->start();
    if (ret != 0) {
        return -1;
    }
    return 0;

}

int main() {
    logger = Logger::instance();
    logger->init();

    signal(SIGINT, process_signal);
    signal(SIGTERM, process_signal);
    int ret = init_server();
    if(ret != 0) {
        return -1;
    }
    server->join();
    logger->stop();
    return 0;
}
