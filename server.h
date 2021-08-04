#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include "server_client.h"

class Server {
   public:
    Server();
    ~Server();
    void Start(std::string ip, int port);
    bool Update();
    void HandleData();
    bool IsRunning() const { return isRunning; }
    ServerClient* GetClient(int fd);

   private:
    int epfd{-1};
    epoll_event events[1024];
    bool isRunning{false};
    std::shared_ptr<std::map<int, ServerClient*>> clients;
    int server_sock{-1};
    sockaddr_in server_addr;
    int SetNonBlocking(int fd);
    void SetReusable(int fd);
    int Bind(std::string ip, int port);
};