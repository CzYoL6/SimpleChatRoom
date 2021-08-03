
#include "server.h"
#include "message.pb.h"
#include "packet.h"

#define EPOLL_SIZE 50
#define BUF_SIZE 1024

int SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return flags;
}

bool Server::Start(std::string ip, int port) {
    clients.reset(new std::map<int, ServerClient*>);

    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(server_sock >= 0);
    SetNonBlocking(server_sock);

    int option = true;
    socklen_t optlen = sizeof(option);
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<void*>(&option), optlen);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(port);

    socklen_t addr_len;
    addr_len = sizeof(server_addr);
    int ret =
        bind(server_sock, reinterpret_cast<sockaddr*>(&server_addr), addr_len);
    assert(ret >= 0);
    std::cout << "bind() succeeded..." << std::endl;

    ret = listen(server_sock, 5);
    assert(ret >= 0);
    std::cout << "listen() succeeded..." << std::endl;

    epfd = epoll_create(EPOLL_SIZE);
    {
        epoll_event ev;
        ev.data.fd = server_sock;
        ev.events = EPOLLIN;
        epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &ev);
    }

    epoll_event events[EPOLL_SIZE];

    std::cout << "epoll created successfully..." << std::endl;

    isRunning = true;
}

bool Server::Update() {
    for (auto iter = clients->begin(); iter != clients->end(); iter++) {
        if (iter->second->HasDataToSend()) {
            std::cout << iter->first << " has data to send..." << std::endl;
            epoll_event ev;
            ev.data.fd = iter->first;
            ev.events = EPOLLIN | EPOLLOUT;
            epoll_ctl(epfd, EPOLL_CTL_MOD, iter->first, &ev);
            std::cout << "modify its fd (EPOLLOUT) " << std::endl;
        }
    }

    int cnt = epoll_wait(epfd, events, EPOLL_SIZE, 0);
    if (cnt < 0) {
        printf("epoll failure!\n");
        return false;
    }

    // std::cout << "epoll_wait cnt: " << cnt << std::endl;

    for (int i = 0; i < cnt; i++) {
        int fd = events[i].data.fd;
        int eve = events[i].events;
        if (fd == server_sock) {
            //有新连接
            std::cout << "new connection..." << std::endl;
            sockaddr_in new_cli_addr;
            memset(&new_cli_addr, 0, sizeof(new_cli_addr));
            socklen_t addr_len;
            addr_len = sizeof(new_cli_addr);
            int new_cli_sock =
                accept(server_sock, reinterpret_cast<sockaddr*>(&new_cli_addr),
                       &addr_len);

            ServerClient* new_cli = new ServerClient(
                new_cli_sock, reinterpret_cast<sockaddr*>(&new_cli_addr));

            clients->insert(std::make_pair(new_cli_sock, new_cli));

            epoll_event ev;
            ev.data.fd = new_cli_sock;
            ev.events = EPOLLIN;
            epoll_ctl(epfd, EPOLL_CTL_ADD, new_cli_sock, &ev);
            std::cout << inet_ntoa(new_cli_addr.sin_addr) << " connected..."
                      << std::endl;

        } else {
            if (eve & EPOLLIN) {
                char buf[BUF_SIZE];

                //有消息来
                memset(buf, 0, sizeof(buf));
                int cnt = recv(fd, buf, BUF_SIZE, 0);
                auto iter = clients->find(fd);
                if (cnt < 0) {
                    std::cout << "something went wrong.." << std::endl;
                    close(fd);
                    delete iter->second;
                    clients->erase(iter);
                    std::cout << "and has been removed from the server.."
                              << std::endl;
                    break;
                } else if (cnt == 0) {
                    //对端关闭
                    std::cout << "the other side is closed.." << std::endl;
                    close(fd);
                    delete iter->second;
                    clients->erase(iter);
                    std::cout << "and has been removed from the server.."
                              << std::endl;
                    break;
                } else {
                    //收到消息
                    std::cout << "recv() succeeded.."
                              << "cnt: " << cnt << std::endl;

                    auto cli = iter->second;

                    cli->WriteIntoRecvBuffer(buf, cnt);
                }
            }
        }
        if (eve & EPOLLOUT) {
        }
    }
    return true;
}

void Server::HandleData() {
    for (auto iter = clients->begin(); iter != clients->end(); iter++) {
        auto cli = iter->second;
        if (cli->GetRecvBuffer()->GetLength() >= sizeof(unsigned int)) {
            char tmp[4];
            memset(tmp, 0, sizeof(tmp));
            int read_res =
                cli->GetRecvBuffer()->Read(tmp, sizeof(unsigned int), true);
            unsigned int p_tmp_res = *(reinterpret_cast<unsigned int*>(tmp));
            std::cout << "read_res: " << read_res << " "
                      << " tmp_res: " << p_tmp_res << std::endl;

            if (cli->GetRecvBuffer()->GetLength() >=
                sizeof(unsigned int) + p_tmp_res) {
                memset(tmp, 0, sizeof(tmp));
                cli->GetRecvBuffer()->Read(tmp, sizeof(unsigned int));
                memset(tmp, 0, sizeof(tmp));
                cli->GetRecvBuffer()->Read(tmp, sizeof(Chat::TYPE));
                std::cout << "recv from client: " << tmp
                          << "(size: " << p_tmp_res << ")" << std::endl;
                std::cout << "type: " << *reinterpret_cast<Chat::TYPE*>(tmp)
                          << std::endl;
                memset(tmp, 0, sizeof(tmp));
                cli->GetRecvBuffer()->Read(tmp, p_tmp_res - sizeof(Chat::TYPE));
                std::cout << p_tmp_res - sizeof(Chat::TYPE) << std::endl;
                Chat::ChatMessage_C_TO_S newMsg;
                if (newMsg.ParseFromArray(tmp, p_tmp_res - sizeof(Chat::TYPE)))
                    std::cout << "msg: " << newMsg.msg() << std::endl;

                //获取当前时间
                //当前的时间点
                std::chrono::system_clock::time_point tp =
                    std::chrono::system_clock::now();
                //转换为time_t类型
                std::time_t t = std::chrono::system_clock::to_time_t(tp);
                //转换为字符串
                std::string now = ctime(&t);
                //去除末尾的换行
                now.resize(now.size() - 1);

                std::string res_(now);
                res_.append(": ");
                res_.append(newMsg.msg());

                Packet* packet = new Packet(Chat::TYPE::chatMessage_C_TO_S);
                std::string msg(res_);

                Chat::ChatMessage_C_TO_S protoMsg;
                protoMsg.set_msg(res_);
                char tmp[1024];
                memset(tmp, 0, sizeof(tmp));
                protoMsg.SerializeToArray(tmp, protoMsg.ByteSizeLong());
                packet->AddVal(tmp, protoMsg.ByteSizeLong());
                packet->AddLength();

                for (auto iter : *clients) {
                    if (iter.first != cli->GetSockFd())
                        send(iter.first, packet->GetBuffer()->GetBuffer(),
                             packet->GetAllLength(), 0);
                }

            } else {
                std::cout << "parse failed." << std::endl;
            }
        }
    }
}

Server::Server() {}

Server::~Server() {}