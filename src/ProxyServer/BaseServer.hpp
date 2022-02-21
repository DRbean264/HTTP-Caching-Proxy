#ifndef BASESERVER_H
#define BASESERVER_H

#include "BaseSocket.hpp"
#include "../HTTPModule/HTTPRequest.hpp"
#include "../HTTPModule/HTTPResponse.hpp"
#include <exception>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <memory>

class SocketBindFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to bind the socket to certain port.";
    }
};

class SendResponseFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to back the response.";
    }
};

class ReceiveDataFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to receive data from client socket.";
    }
};

class GetIpFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to get the ip address from socket.";
    }
};

class SocketListenFailure : public std::exception {
public:
    virtual const char* what() const noexcept {
       return "Fail to listening to the socket to.";
    }
};

class AcceptConnectFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
        return "Fail to accept client connections.";
    }
};

class GetAddrInfoFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
        return "Fail to get address info for host.";
    }
};

class BaseServer
{
private:
    BaseSocket *sock;
    struct addrinfo *host_info_list;
    const char *hostname;
    const char *port;
    int backlog;
public:
    BaseServer(const char *_hostname, 
        const char *_port, int _backlog);
    virtual ~BaseServer();
    virtual void setup(struct addrinfo) = 0;
    virtual void launch() = 0;
    void bindSocket(int socket_fd);
    void listenSocket(int socket_fd);
    std::string recvSocket_timeout(int s, int timeout);
    std::string recvSocket_block(int s);
    std::string recvSocket_block_test(int s);
    virtual void daemonize() = 0;
    virtual int acceptIncomingRequest() = 0;
    virtual void processRequest(int) = 0;

    // getter 
    const char *getHostname() const;
    const char *getPort() const;
    int getBacklog() const;
    const struct addrinfo *getHostInfoList() const;
    const BaseSocket *getSocket() const;

    // setter
    void setBacklog(int);
    void setHostInfoList(struct addrinfo *);
    void setSocket(BaseSocket *);
};


#endif /* BASESERVER_H */
