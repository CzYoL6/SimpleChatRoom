#pragma once
#include <sys/socket.h>
#include "circle_buffer.hpp"

class ServerClient {
   public:
    ServerClient(int fd, sockaddr* addr);
    ~ServerClient();

    int GetSockFd() const { return m_nSockFd; }

    bool HasDataToSend() const;
    bool HasRecvData() const;

    // buffer operations
    int ReadFromSendBuffer(char* buf, int size, bool peek) const;
    int WriteIntoSendBuffer(char* buf, int size) const;
    int ReadFromRecvBuffer(char* buf, int size, bool peek) const;
    int WriteIntoRecvBuffer(char* buf, int size) const;

    CircleBuffer<char>* GetSendBuffer() const { return m_sendBuffer; }

    CircleBuffer<char>* GetRecvBuffer() const { return m_recvBuffer; }

   private:
    int m_nSockFd{-1};
    sockaddr m_sAddr;
    CircleBuffer<char>* m_sendBuffer{nullptr};
    CircleBuffer<char>* m_recvBuffer{nullptr};
};