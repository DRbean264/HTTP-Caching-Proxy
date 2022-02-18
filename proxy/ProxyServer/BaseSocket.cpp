#include "BaseSocket.hpp"
#include <sys/socket.h>
#include <netinet/in.h>

BaseSocket::BaseSocket(int _domain, int _type, int _protocol)
    : domain(_domain), type(_type), protocol(_protocol) {
    int socketTemp = createSocket(_domain, _type, _protocol);
    setSocketFd(socketTemp);
}

int BaseSocket::createSocket(int _domain, int _type, int _protocol) {
    return socket(_domain, _type, _protocol);
}

int BaseSocket::getSocketFd() const {
    return socket_fd;
}
int BaseSocket::getDomain() const {
    return domain;
}
int BaseSocket::getType() const {
    return type;
}
int BaseSocket::getProtocol() const {
    return protocol;
}

void BaseSocket::setSocketFd(int s) {
    if (s == -1) {
        throw SocketCreateFailure();
    }
    socket_fd = s;
}
void BaseSocket::setDomain(int d) {
    domain = d;
}
void BaseSocket::setType(int t) {
    type = t;
}
void BaseSocket::setProtocol(int p) {
    protocol = p;
}