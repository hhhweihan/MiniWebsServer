#include "server.h"
#include <iostream>
#include <signal.h>

void signal_handler(int sig) {
    std::cout << "\n捕获到信号 " << sig << "，优雅退出..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    
    int port = 8080;
    int thread_num = 8;
    
    if(argc >= 2) {
        port = atoi(argv[1]);
    }
    if(argc >= 3) {
        thread_num = atoi(argv[2]);
    }
    
    WebServer server(port, thread_num);
    server.start();
    
    return 0;
} 