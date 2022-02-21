#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include "BaseServer.hpp"
#include "../CacheController.hpp"
#include "../MyLog/MyLog.hpp"
#include <string>
#include <unordered_map>

class ProxyServer : public BaseServer
{
    const std::string logFile;
    const std::string errFile;
    CacheController cachecont;
    //BaseSocket *sockAsClient;
    MyLog log;
    MyLog errLog;
public:
    ProxyServer(struct addrinfo host_info, 
                const std::string _logFile, const std::string _errFile,
                const char *_hostname, 
                const char *_port, int _backlog);
    virtual ~ProxyServer();
    virtual void setup(struct addrinfo host_info);
    virtual int acceptIncomingRequest();
    virtual void processRequest(int);
    virtual void daemonize();
    virtual void launch();
    void sendContent(int fd, std::string message);
    void exchangeContent(int client_fd, int server_fd);
    void requestAndRespond(HTTPRequest &request, bool cache, bool &keepConnection);
    std::string getResponseForRequest(HTTPRequest &request);
    HTTPRequest generateValidationRequest(const HTTPRequest &origreq, const HTTPResponse &response);
    std::string getIpFromSocket(int socket_fd) const;

    void processGETRequest(const HTTPRequest &);
    void processPOSTRequest(const HTTPRequest &);
    void processCONNECTRequest(const HTTPRequest &);
    void addToDict(const std::string &, std::unordered_map<std::string, std::string> &);
    std::unordered_map<std::string, std::string> parseCacheControl(const std::string &);
    BaseSocket *setUpAsClientSocket(const HTTPRequest &);
};


#endif /* PROXYSERVER_H */
