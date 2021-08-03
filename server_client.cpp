#include "server_client.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

ServerClient::ServerClient(int fd, sockaddr* addr) {
    m_nSockFd = fd;
    m_sAddr = *addr;
    m_sendBuffer = new CircleBuffer<char>(2048);
    m_recvBuffer = new CircleBuffer<char>(2048);
}

ServerClient::~ServerClient() {
    if (m_sendBuffer != nullptr) {
        delete m_sendBuffer;
        m_sendBuffer = nullptr;
    }
    if (m_recvBuffer != nullptr) {
        delete m_recvBuffer;
        m_recvBuffer = nullptr;
    }
}

bool ServerClient::HasDataToSend() const {
    return !m_sendBuffer->isEmpty();
}

bool ServerClient::HasRecvData() const {
    return !m_recvBuffer->isEmpty();
}

int ServerClient::ReadFromSendBuffer(char* buf,
                                     int size,
                                     bool peek = false) const {
    return m_sendBuffer->Read(buf, size, peek);
}

int ServerClient::ReadFromRecvBuffer(char* buf,
                                     int size,
                                     bool peek = false) const {
    return m_recvBuffer->Read(buf, size, peek);
}

int ServerClient::WriteIntoSendBuffer(char* buf, int size) const {
    return m_sendBuffer->Write(buf, size);
}

int ServerClient::WriteIntoRecvBuffer(char* buf, int size) const {
    return m_recvBuffer->Write(buf, size);
}