#ifndef BASESOCKET_H
#define BASESOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <exception>
#include <string>

class SocketCreateFailure : public std::exception
{
public:    
    virtual const char* what() const noexcept {
       return "Fail to create socket.";
    }
};

//int socket(int domain, int type, int protocol);

class BaseSocket {
private:
    int socket_fd;
    int domain;
    int type;
    int protocol;   
public:
    BaseSocket(int _domain, int _type, int _protocol);
    int createSocket(int _domain, int _type, int _protocol);

    // getter function
    int getSocketFd() const;
    int getDomain() const;
    int getType() const;
    int getProtocol() const;
    // setter funciton
    void setSocketFd(int s);
    void setDomain(int d);
    void setType(int t);
    void setProtocol(int p);
};

class MySocket {
private:
    // socket file descripter
    int socket_fd;
    int domain;
    int type;
    int protocol;
    struct addrinfo *host_info_list;  

    int createSocket(int _domain, int _type, int _protocol);
public:
    MySocket(int _domain, int _type, int _protocol, struct addrinfo *host_info_list);
    ~MySocket();

    // bind to this socket
    // on failure, throw SocketBindFailure();
    void bindSocket();
    // listen to this socket
    // on failure, throw SocketListenFailure();
    void listenSocket();
    // connect to this socket
    // on failure, throw SocketConnectFailure();
    void connectSocket();

    // receive data from this socket
    std::string receiveDataFull();
    // receive data with transfer-encoding: chunk
    std::string receiveDataChunk();
    // send data to this socket
    void sendData(std::string);

    // getter function
    int getSocketFd() const;
    int getDomain() const;
    int getType() const;
    int getProtocol() const;
    struct addrinfo *getHostInfoList() const;

    // setter funciton
    void setSocketFd(int s);
    void setDomain(int d);
    void setType(int t);
    void setProtocol(int p);
};

#endif /* BASESOCKET_H */
