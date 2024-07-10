#include "logger.h"

using namespace core;
Logger* logger = nullptr;

class TThread {
    public:
        int start() {
            if(_thread) {
                return -1;
            }
            _thread = new std::thread([=]() {
                for(int i = 0;i<100000;i++) {
                    LOG(INFO) << "test";
                }
            });
            return 0;
        }
        void join() {
            if(_thread && _thread->joinable()) {
                _thread->join();
            }
        }
    private:
        std::thread* _thread = nullptr;
};

int main() {

    logger = Logger::instance();
    logger->set_stderr(false);
    logger->init();

    LOG(INFO) << "_thread s";

    TThread* t_thread = new TThread();
    t_thread->start();

    TThread* t_thread2 = new TThread();
    t_thread2->start();

    TThread* t_thread3 = new TThread();
    t_thread3->start();
    
    t_thread->join();
    t_thread2->join();
    t_thread3->join();

    LOG(INFO) << "log end";
    return 0;
}