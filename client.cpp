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
#include "circle_buffer.hpp"
#include "message.pb.h"
#include "packet.h"

#define EPOLL_SIZE 50
#define BUF_SIZE 1024

int main(int argc, char** argv) {
    int client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(client_sock >= 0);
    sockaddr_in server_addr;
    socklen_t addr_len;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(atoi(argv[2]));

    addr_len = sizeof(server_addr);

    int epfd = epoll_create(EPOLL_SIZE);
    {
        epoll_event ev;
        ev.data.fd = client_sock;
        ev.events = EPOLLIN | EPOLLOUT;

        int connect_ret = connect(
            client_sock, reinterpret_cast<sockaddr*>(&server_addr), addr_len);
        if (connect_ret == 0) {
            std::cout << "connect() succeeded." << std::endl;
        } else if (connect_ret < 0) {
            std::cout << "connect() err. " << std::endl;
        }

        epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &ev);
    }

    epoll_event events[EPOLL_SIZE];

    int pipes[2];
    bool isClientWorking = true;
    if (pipe(pipes) < 0) {
        std::cout << "pipe() err..." << std::endl;
        return -1;
    } else {
        std::cout << "pipe() suceeded.." << std::endl;
    }
    {
        epoll_event ev;
        ev.data.fd = pipes[0];
        ev.events = EPOLLIN;
        epoll_ctl(epfd, EPOLL_CTL_ADD, pipes[0], &ev);
    }
    int pid = fork();

    if (pid < 0) {
        std::cout << "fork() err.." << std::endl;
        isClientWorking = false;
        return -1;
    } else if (pid == 0) {
        // child
        close(pipes[0]);
        // std::cout << "input 'exit' to  exit the chatroom" << std::endl;
        char buf[BUF_SIZE];
        // if the client work send message to the server
        while (isClientWorking) {
            memset(buf, 0, sizeof(buf));
            fgets(buf, BUF_SIZE, stdin);

            // child write the message to the pipe[1]
            if (write(pipes[1], buf, strlen(buf) - 1) < 0) {
                // std::cout << "write error" << std::endl;
                isClientWorking = false;
                return -1;
            }
        }
    } else {
        // father
        close(pipes[1]);

        CircleBuffer<char> recvbuf(2048);

        while (isClientWorking) {
            int cnt = epoll_wait(epfd, events, EPOLL_SIZE, 0);
            if (cnt < 0) {
                printf("epoll failure!\n");
                isClientWorking = false;
                break;
            }
            for (int i = 0; i < cnt; i++) {
                int fd = events[i].data.fd;
                int eve = events[i].events;
                if (eve & EPOLLIN) {
                    if (fd == client_sock) {
                        //服务器发来消息，有可读
                        char buf[1024];
                        memset(buf, 0, sizeof(buf));
                        int recv_ret = recv(fd, buf, 1024, 0);
                        if (recv_ret < 0) {
                            // std::cout << "recv() err..." << std::endl;
                            close(client_sock);
                            isClientWorking = false;
                            return -1;
                        } else if (recv_ret == 0) {
                            // std::cout << "the server is closed..." <<
                            // std::endl;
                            isClientWorking = false;
                            close(client_sock);
                            return 0;
                        } else {
                            recvbuf.Write(buf, recv_ret);
                            if (recvbuf.GetLength() >= sizeof(unsigned int)) {
                                char tmp[4];
                                memset(tmp, 0, sizeof(tmp));
                                int read_res = recvbuf.Read(
                                    tmp, sizeof(unsigned int), true);
                                unsigned int p_tmp_res =
                                    *(reinterpret_cast<unsigned int*>(tmp));
                                // std::cout << "read_res: " << read_res << " "
                                //           << " tmp_res: " << p_tmp_res
                                //           << std::endl;

                                if (recvbuf.GetLength() >=
                                    sizeof(unsigned int) + p_tmp_res) {
                                    memset(tmp, 0, sizeof(tmp));
                                    recvbuf.Read(tmp, sizeof(unsigned int));
                                    memset(tmp, 0, sizeof(tmp));
                                    recvbuf.Read(tmp, sizeof(Chat::TYPE));
                                    // std::cout << "recv from server: " << tmp
                                    //           << "(size: " << p_tmp_res <<
                                    //           ")"
                                    //           << std::endl;
                                    // std::cout
                                    //     << "type: "
                                    //     <<
                                    //     *reinterpret_cast<Chat::TYPE*>(tmp)
                                    //     << std::endl;
                                    memset(tmp, 0, sizeof(tmp));
                                    recvbuf.Read(
                                        tmp, p_tmp_res - sizeof(Chat::TYPE));
                                    // std::cout << p_tmp_res -
                                    // sizeof(Chat::TYPE)
                                    //           << std::endl;
                                    Chat::ChatMessage_C_TO_S newMsg;
                                    if (newMsg.ParseFromArray(
                                            tmp,
                                            p_tmp_res - sizeof(Chat::TYPE)))
                                        std::cout << "msg: " << newMsg.msg()
                                                  << std::endl;
                                    else {
                                        std::cout << "parse failed."
                                                  << std::endl;
                                    }
                                }
                            }
                        }
                    } else if (fd == pipes[0]) {
                        //从子进程中发来的消息，发送给服务器
                        char buf[1024];
                        memset(buf, 0, sizeof(buf));
                        int read_ret = read(fd, buf, 1024);
                        if (read_ret < 0) {
                            // std::cout << "read() err..." << std::endl;
                            close(client_sock);
                            isClientWorking = false;
                            return -1;
                        } else {
                            Packet* packet =
                                new Packet(Chat::TYPE::chatMessage_C_TO_S);
                            std::string msg(buf);
                            Chat::ChatMessage_C_TO_S protoMsg;
                            protoMsg.set_msg(msg);
                            char tmp[1024];
                            memset(tmp, 0, sizeof(tmp));
                            protoMsg.SerializeToArray(tmp,
                                                      protoMsg.ByteSizeLong());
                            packet->AddVal(tmp, protoMsg.ByteSizeLong());
                            packet->AddLength();

                            send(client_sock, packet->GetBuffer()->GetBuffer(),
                                 packet->GetAllLength(), 0);

                            // std::cout
                            //     << "packet length: " <<
                            //     packet->GetAllLength()
                            //     << std::endl;

                            // std::cout << "send() suceeded." << std::endl;
                            epoll_event ev;
                            ev.data.fd = fd;
                            ev.events = EPOLLIN;
                            epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);

                            delete packet;
                            packet = nullptr;
                        }
                    }
                }
                if (eve & EPOLLOUT) {
                    // socket可写

                    Packet* packet = new Packet(Chat::TYPE::chatMessage_C_TO_S);
                    std::string msg{"hello there!"};
                    Chat::ChatMessage_C_TO_S protoMsg;
                    protoMsg.set_msg(msg);
                    char tmp[1024];
                    memset(tmp, 0, sizeof(tmp));
                    protoMsg.SerializeToArray(tmp, protoMsg.ByteSizeLong());
                    packet->AddVal(tmp, protoMsg.ByteSizeLong());
                    packet->AddLength();

                    send(fd, packet->GetBuffer()->GetBuffer(),
                         packet->GetAllLength(), 0);

                    // std::cout << "packet length: " << packet->GetAllLength()
                    //           << std::endl;

                    // std::cout << "send() suceeded." << std::endl;
                    epoll_event ev;
                    ev.data.fd = fd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);

                    delete packet;
                    packet = nullptr;
                }
            }
        }
    }
}