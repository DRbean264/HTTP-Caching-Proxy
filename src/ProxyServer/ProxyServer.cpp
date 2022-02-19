#include "ProxyServer.hpp"
#include "../HTTPModule/HTTPRequest.hpp"
#include "../HTTPModule/HTTPResponse.hpp"
#include "../myUtils.hpp"
#include <cstdlib>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <cassert>
using namespace std;

static string OK200 = "HTTP/1.1 200 OK\r\n\
Date: Sun, 18 Oct 2012 10:36:20 GMT\r\n\
Server: Apache/2.2.14 (Win32)\r\n\
Connection: Closed\r\n\
Content-Type: text/html; charset=iso-8859-1\r\n\r\n\
<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n\
<html>\n\
<head>\n\
<title>Debugging...</title>\n\
</head>\n\
<body>\n\
<h1>Success Page</h1>\n\
<p>200 OK response.</p>\n\
</body>\n\
</html>";

static string Bad400 = "HTTP/1.1 400 Bad Request\r\n\
Server: Apache/2.2.14 (Win32)\r\n\
Connection: Closed\r\n\
Content-Type: text/html; charset=iso-8859-1\r\n\r\n\
<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n\
<html>\n\
<head>\n\
<title>Debugging...</title>\n\
</head>\n\
<body>\n\
<h1>Bad Request Page</h1>\n\
<p>400 Bad Request response.</p>\n\
</body>\n\
</html>";

static string NotAllowed405 = "HTTP/1.1 405 Method Not Allowed\r\n\
Server: Apache/2.2.14 (Win32)\r\n\
Connection: Closed\r\n\
Content-Type: text/html; charset=iso-8859-1\r\n\r\n\
<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n\
<html>\n\
<head>\n\
<title>Debugging...</title>\n\
</head>\n\
<body>\n\
<h1>Method Not Allowed Page</h1>\n\
<p>405 Method Not Allowed response.</p>\n\
</body>\n\
</html>";

static string Bad502 = "HTTP/1.1 502 Bad Gateway\r\n\
Server: Apache/2.2.14 (Win32)\r\n\
Connection: Closed\r\n\
Content-Type: text/html; charset=iso-8859-1\r\n\r\n\
<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n\
<html>\n\
<head>\n\
<title>Debugging...</title>\n\
</head>\n\
<body>\n\
<h1>Bad Gateway Page</h1>\n\
<p>502 Bad Gateway response.</p>\n\
</body>\n\
</html>";

static string Unknown520 = "HTTP/1.1 520 Proxy Server is Returning an Unknown Error\r\n\
Server: Apache/2.2.14 (Win32)\r\n\
Connection: Closed\r\n\
Content-Type: text/html; charset=iso-8859-1\r\n\r\n\
<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n\
<html>\n\
<head>\n\
<title>Debugging...</title>\n\
</head>\n\
<body>\n\
<h1>Proxy Server is Returning an Unknown Error Page</h1>\n\
<p>520 Proxy Server is Returning an Unknown Error response.</p>\n\
</body>\n\
</html>";


ProxyServer::ProxyServer(struct addrinfo host_info,
                        const string _logFile, const string _errFile,
                        const char *_hostname, 
                        const char *_port, int _backlog)
    : BaseServer(_hostname, _port, _backlog), logFile(_logFile), errFile(_errFile),
    log(_logFile), errLog(_errFile) {
    try {
        setup(host_info);
    }
    catch(const exception& e) {
        errLog.write("Proxy server setup failed");
        exit(EXIT_FAILURE);
    }
}

ProxyServer::~ProxyServer() {
    
}

string ProxyServer::getIpFromSocket(int socket_fd) const {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int status = getpeername(socket_fd, (struct sockaddr *)&addr, &addrlen);
    if (status == -1) {
        throw GetIpFailure();
    }
    char ip[20];
    const char *result = inet_ntop(AF_INET, &addr.sin_addr, ip, 20);
    if (result == NULL) {
        throw GetIpFailure();
    }
    return string(ip);
}

int ProxyServer::acceptIncomingRequest() {
    struct sockaddr_storage socket_addr;
    //struct sockaddr_in socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);

    int curr_socket_fd = getSocket()->getSocketFd();

    // create a new socket for this connection
    int client_connection_fd;
    client_connection_fd = accept(curr_socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
        errLog.write("Accept incoming connection failure");
        throw AcceptConnectFailure();
    }

    cout << "Accept connection!!!\n";
    return client_connection_fd;
}

void ProxyServer::processRequest(int client_connection_fd) {
    cout << "Waiting for incoming data...\n";
    string receivedData = recvSocket_block(client_connection_fd);
    
    // create an request object, need to add mutex for unique id
    HTTPRequest request(client_connection_fd, receivedData);
    // request.display();

    // check if the request is error request
    // no error
    if (request.getErrorCode() == 0) {
        // logging this new request
        string ip = getIpFromSocket(request.getClientFd());
        string currTime = timeToASC(getCurrentTime());

        stringstream ss;
        ss << request.getId() << ": " << 
        "\"" << request.firstLine() << "\" from " << 
        ip << " @ " << currTime;
        log.write(ss.str());

        // deal with different HTTP methods
        if (request.getMethod() == "GET") {
            processGETRequest(request);
        } else if (request.getMethod() == "POST") {
            processPOSTRequest(request);
        } else if (request.getMethod() == "CONNECT") {
            processCONNECTRequest(request);
        }
    }
    // if client side closed the connection, proxy close the connection
    else if (request.getErrorCode() == -1) {
        errLog.write("Client closed the connection");
    }
    // wrong request format error
    else if (request.getErrorCode() == 1) {
        stringstream ss;
        ss << request.getId() << ": Responding \"HTTP/1.1 400 Bad Request\"";
        log.write(ss.str()); 
        sendContent(request.getClientFd(), Bad400);
    }
    // http method unsupported
    else if (request.getErrorCode() == 2) {
        stringstream ss;
        ss << request.getId() << ": Responding \"HTTP/1.1 405 Method Not Allowed\"";
        log.write(ss.str()); 
        sendContent(request.getClientFd(), NotAllowed405);
    }
    // other system errors
    else if (request.getErrorCode() == 3) {
        stringstream ss;
        ss << request.getId() << ": Responding \"HTTP/1.1 520 Proxy Server is Returning an Unknown Error\"";
        log.write(ss.str()); 
        sendContent(request.getClientFd(), Unknown520);
    }
    
    cout << "Close socket connection... with request: " << request.firstLine() << endl;
    close(request.getClientFd());
}

void ProxyServer::setup(struct addrinfo host_info) {
    // daemonize the server
    // daemonize();

    // get address information for host
    struct addrinfo *host_info_list_temp;
    int status = getaddrinfo(getHostname(), getPort(),
                    &host_info, &host_info_list_temp);
    if (status == -1) {
        freeaddrinfo(host_info_list_temp);
        errLog.write("During setup: Cannot get address info for host");
        throw GetAddrInfoFailure();
    }
    setHostInfoList(host_info_list_temp);

    // create server socket
    BaseSocket *sock_temp;
    try {
        sock_temp = new BaseSocket(host_info_list_temp->ai_family,
                                host_info_list_temp->ai_socktype,
                                host_info_list_temp->ai_protocol);
        setSocket(sock_temp);
    }
    catch(const std::exception& e) {
        freeaddrinfo(host_info_list_temp);
        errLog.write("During setup: Cannot create socket");
        throw;
    }

    try {
        // bind to the socket
        bindSocket(getSocket()->getSocketFd());

        // listening to the socket
        listenSocket(getSocket()->getSocketFd());
    }
    catch(const std::exception& e) {
        freeaddrinfo(host_info_list_temp);
        delete sock_temp;
        errLog.write("During setup: Cannot bind or listen to socket");
        throw;
    }
    cout << "Server setup done!!!\n";
}

void ProxyServer::launch() {
    while (1) {
        std::cout << "===========Waiting=============\n";
        int client_fd = acceptIncomingRequest();

        // spawn a new thread whenever the server receive a new request
        thread newThread(&ProxyServer::processRequest, this, client_fd);
        newThread.detach();
        std::cout << "==========Done============\n";
    }
}

void ProxyServer::daemonize(){
    // fork, create a child process
    pid_t pid = fork();
    // exit the parent process, guaranteed not to be a process group leader
    if (pid != 0) {
        exit(EXIT_SUCCESS);
    } else if (pid == -1) {
        errLog.write("During daemonize: First Fork failure");
        exit(EXIT_FAILURE);
    }
    // working on the child process
    // create a new session with no controlling tty
    pid_t sid = setsid();
    if (sid == -1) {
        errLog.write("During daemonize: Create new session failure");
        exit(EXIT_FAILURE);
    }
    // point stdin/stdout/stderr to it
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    // freopen(logFile.c_str(), "w", stdout);
    // freopen(errFile.c_str(), "w", stderr);
    // cout << "Redirection done.\n";
    
    // change working directory to root
    chdir("/");
    // clear umask
    umask(0);
    // fork again
    pid = fork();
    // exit the parent process, guaranteed not to be a session leader
    if (pid != 0) {
        exit(EXIT_SUCCESS);
    } else if (pid == -1) {
        errLog.write("During daemonize: Second Fork failure");
        exit(EXIT_FAILURE);
    }
    // cout << "Daemonization Done.\n";
}

void ProxyServer::processPOSTRequest(const HTTPRequest &req) {
    // default copy constructor, will not consume a new request ID
    HTTPRequest request = req;
    while (1) {
        bool keepConnection = true;

        requestAndRespond(request, false, keepConnection);

        // determine whether to keep this connection
        const unordered_map<string, string> &hf = request.getHeaderFields();
        if (keepConnection && (hf.find("connection") != hf.cend() && 
            hf.find("connection")->second == "keep-alive")) {
            // waiting for next request
            cout << "Waiting for next client request...\n";
            string newRequest = recvSocket_block(request.getClientFd());
            cout << "Received a new request.\n";
            if (newRequest.size() == 0) {
                cout << "Client has closed the connection.\n";
                break;
            }
            request = HTTPRequest(request.getClientFd(), newRequest);
            // I need to check again whether the new request is valid!!!!!!!
        } else {
            break;
        }
    }
}

void ProxyServer::processCONNECTRequest(const HTTPRequest &request) {
    // HTTPResponse response;
    // tcp connect to origin server
    BaseSocket *sockAsClient = setUpAsClientSocket(request);

    // send 200 ok to client
    stringstream ss1;
    ss1 << request.getId() << ": Responding \"HTTP/1.1 200 OK\"";
    log.write(ss1.str()); 
    sendContent(request.getClientFd(), OK200);

    // exchanging content for client and server
    exchangeContent(request.getClientFd(), sockAsClient->getSocketFd());
    stringstream ss2;
    ss2 << request.getId() << ": Tunnel closed";
    log.write(ss2.str());

    // close the socket connection
    close(sockAsClient->getSocketFd());
    delete sockAsClient;
}

void ProxyServer::processGETRequest(const HTTPRequest &req) {
    // default copy constructor, will not consume a new request ID
    HTTPRequest request = req;
    while (1) {
        // default, close the connection
        bool keepConnection = true;

        requestAndRespond(request, true, keepConnection);
        
        // determine whether to keep this connection
        const unordered_map<string, string> &hf = request.getHeaderFields();
        
        if (keepConnection && (hf.find("connection") != hf.cend() && 
            hf.find("connection")->second == "keep-alive")) {
            // waiting for next request
            cout << "Waiting for next client request...\n";
            string newRequest = recvSocket_timeout(request.getClientFd(), 1);
            cout << "Received a new request.\n";
            if (newRequest.size() == 0) {
                cout << "Client has closed the connection.\n";
                break;
            }
            request = HTTPRequest(request.getClientFd(), newRequest);
            
            // logging this new request
            string ip = getIpFromSocket(request.getClientFd());
            string currTime = timeToASC(getCurrentTime());

            stringstream ss;
            ss << request.getId() << ": " << 
            "\"" << request.firstLine() << "\" from " << 
            ip << " @ " << currTime;
            log.write(ss.str());
        } else {
            break;
        }
    }
    stringstream ss;
    ss << request.getId() << ": Connection closed";
    log.write(ss.str());
}

string ProxyServer::getResponseForRequest(HTTPRequest &request) {
    // tcp connect to origin server
    BaseSocket *sockAsClient = setUpAsClientSocket(request);

    // make HTTP request & logging
    stringstream ss = stringstream();
    ss << request.getId() << ": Requesting \"" << request.firstLine() <<
    "\" from " << request.getHostName();
    log.write(ss.str());
    string message = request.getRaw();
    sendContent(sockAsClient->getSocketFd(), message);

    // receive HTTP response, improve later with content length info!!!
    cout << "Waiting for origin server's response...\n";
    string resp = recvSocket_timeout(sockAsClient->getSocketFd(), 1);

    // close the origin server connection
    close(sockAsClient->getSocketFd());
    delete sockAsClient;
    return resp;
}

HTTPRequest ProxyServer::generateValidationRequest(const HTTPRequest &origreq, const HTTPResponse &response) {
    unordered_map<string, string> ccf = response.cacheControlFields;
    
    // if neither etag nor last-modified exists
    if (ccf.find(CacheControlKey::ck_etag) == ccf.cend() &&
    ccf.find(CacheControlKey::ck_last_modified) == ccf.cend()) {
        return origreq;
    }
    
    stringstream ss;
    ss << origreq.firstLine() << "\r\n";
    // add all the original headers
    for (auto &header : origreq.getHeaderFields()) {
        ss << header.first << ": " << header.second << "\r\n";
    }
    // add other validation fields
    if (ccf.find(CacheControlKey::ck_etag) != ccf.cend()) {
        ss << "if-none-match: " << ccf[CacheControlKey::ck_etag] << "\r\n";
    }
    if (ccf.find(CacheControlKey::ck_last_modified) != ccf.cend()) {
        ss << "if-modified-since: " << ccf[CacheControlKey::ck_last_modified] << "\r\n";
    }
    ss << "\r\n";
    return HTTPRequest(origreq.getClientFd(), ss.str());
} 

void ProxyServer::requestAndRespond(HTTPRequest &request, bool cache, bool &keepConnection) {
    stringstream ss;

    // request.display();

    // if no error
    if (request.getErrorCode() == 0) {
        string resp;

        // check Cache with Cache api
        // cache logic here
        if (cache) {
            // if there's the same URI in cache
            if (cachecont.hasKey(request.getURI())) {
                cout << "In cache!!!" << endl;
                HTTPResponse response = cachecont.get(request.getURI()).first;
                // if the response is still fresh, send it right back
                if (cachecont.hasValidKey(request.getURI())) {
                    cout << "still valid!!!\n";
                    sendContent(request.getClientFd(), response.getRaw());
                    keepConnection = true;
                    return;
                } 
                // if the response expired
                else {
                    // generate re-validation request from the response in cache
                    HTTPRequest req = generateValidationRequest(request, response);
                    cout << "debugging...\n";
                    cout << "Generated Validation Request:\n";
                    req.display();
                    cout << "debugging...\n";
                    resp = getResponseForRequest(req);
                }
            } else {
                ss << request.getId() << ": not in cache";
                log.write(ss.str());
                resp = getResponseForRequest(request);
            }
        } else {
            resp = getResponseForRequest(request);
        }

        // parse the response
        HTTPResponse response;
        try {
            response = HTTPResponse(resp, request.getId());
            // response.display();

            if (cache) {
                // cache logic here
                // if is 200 OK, then check cache status and log
                if (response.getStatusCode() == "200") {
                    // update the cache
                    pair<bool, reasonStr> result = cachecont.set(request.getURI(), response);
                    // if update successfully
                    if (result.first) {
                        // log the expiration time
                        std::pair<HTTPResponse, int> cacheEntry = cachecont.get(request.getURI());
                        ss = stringstream();
                        ss << request.getId() << ": cached, expires at " << time_tToASC(cacheEntry.second);
                        assert(cacheEntry.second != -1);
                        log.write(ss.str());   
                        // log the re-validation info
                        if (cachecont.isMustRevalidate(response)) {
                            ss = stringstream();
                            ss << request.getId() << ": cached, but requires re-validation";
                            log.write(ss.str());
                        }                    
                    } 
                    // if not update successfully, log the reason
                    else {
                        ss = stringstream();
                        ss << request.getId() << ": not cacheable because " <<
                        result.second;
                        log.write(ss.str());
                    }
                } 
            }
            //request.display();
            //response.display();

            // logging
            ss = stringstream();
            ss << request.getId() << ": Received \"" << response.firstLine() <<
            "\" from " << request.getHostName();
            log.write(ss.str());

            // send back the response to client
            if (response.getStatusCode() == "304") {
                // if the response is for us, proxy server
                if (cachecont.hasKey(request.getURI())) {
                    response = cachecont.get(request.getURI()).first;
                }
            }
            ss = stringstream();
            ss << request.getId() << ": Responding \"" << response.firstLine() << "\"";
            log.write(ss.str());
            sendContent(request.getClientFd(), response.getRaw());
        }
        // http response is of wrong format, return Bad Gateway 502
        catch(const HTTPResponseWrongFormat& e) {
            cerr << e.what() << '\n';
            stringstream ss;
            ss << request.getId() << ": Responding \"HTTP/1.1 502 Bad Gateway\"";
            log.write(ss.str()); 
            sendContent(request.getClientFd(), HTTPResponse(Bad502, 0).getRaw());
        }

        keepConnection = true;
    } 
    // if client side closed the connection, proxy close the connection
    else if (request.getErrorCode() == -1) {
        errLog.write("Client closed the connection");
        keepConnection = false;
    }
    // wrong request format error
    else if (request.getErrorCode() == 1) {
        stringstream ss;
        ss << request.getId() << ": Responding \"HTTP/1.1 400 Bad Request\"";
        log.write(ss.str()); 
        sendContent(request.getClientFd(), Bad400);
        keepConnection = true;
    }
    // http method unsupported
    else if (request.getErrorCode() == 2) {
        stringstream ss;
        ss << request.getId() << ": Responding \"HTTP/1.1 405 Method Not Allowed\"";
        log.write(ss.str()); 
        sendContent(request.getClientFd(), NotAllowed405);
        keepConnection = true;
    } 
    // other system errors
    else if (request.getErrorCode() == 3) {
        stringstream ss;
        ss << request.getId() << ": Responding \"HTTP/1.1 520 Proxy Server is Returning an Unknown Error\"";
        log.write(ss.str()); 
        sendContent(request.getClientFd(), Unknown520);
        keepConnection = true;
    }
}

void ProxyServer::exchangeContent(int client_fd, int server_fd) {
    fd_set readSet;
    fd_set init;
    FD_ZERO(&readSet);
    FD_SET(client_fd, &readSet);
    FD_SET(server_fd, &readSet);
    FD_ZERO(&init);
    FD_SET(client_fd, &init);
    FD_SET(server_fd, &init);
    int FD_MAX = max(client_fd, server_fd) + 1;
    int status;
    while (1) {
        readSet = init;
        status = select(FD_MAX, &readSet, NULL, NULL, NULL);
        if (status == -1) {
            cerr << "Select function failed.\n";
            exit(EXIT_FAILURE);
        }
        if (status > 0) {
            if (FD_ISSET(client_fd, &readSet)) {
                // read from client side
                string content = recvSocket_block(client_fd);
                // the other side closed the connection
                if (content.size() == 0) 
                    return;
                // send the message to server
                else 
                    sendContent(server_fd, content);
            } else if (FD_ISSET(server_fd, &readSet)) {
                // read from server side
                string content = recvSocket_block(server_fd);
                // the other side closed the connection
                if (content.size() == 0)
                    return;
                // send the message to client
                else {
                    sendContent(client_fd, content);
                }
            }
        }
    }
    
}

void ProxyServer::sendContent(int fd, std::string message) {
    int status = send(fd, message.c_str(), message.length(), 0);
    if (status == -1) {
        errLog.write("Cannot send message");
        // exit(EXIT_FAILURE);
        // throw SendResponseFailure();
    }
}

// set up the socket to comunicate with the origin server
BaseSocket *ProxyServer::setUpAsClientSocket(const HTTPRequest &request) {
     // get address information for host
    struct addrinfo host_info;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;    // IPv4/IPv6
    host_info.ai_socktype = SOCK_STREAM;  // TCP

    struct addrinfo *host_info_list_temp;
    string hostname = request.getHostName();
    string port = request.getPort();
    // if no port specified by the HTTP request
    if (port.size() == 0) {
        if (request.getMethod() == "CONNECT") 
            port = string("443");
        else
            port = string("80");
    }

    int status;
    status = getaddrinfo(hostname.c_str(), port.c_str(),
                    &host_info, &host_info_list_temp);
    if (status == -1) {
        freeaddrinfo(host_info_list_temp);
        cerr << "Can not get address info for host\n";
        exit(EXIT_FAILURE);
    }

    // create asClient socket
    BaseSocket *sockAsClient;
    try {
        sockAsClient = new BaseSocket(host_info_list_temp->ai_family,
                                host_info_list_temp->ai_socktype,
                                host_info_list_temp->ai_protocol);
    }
    catch(const std::exception& e) {
        freeaddrinfo(host_info_list_temp);
        cerr << "Can not create socket\n";
        exit(EXIT_FAILURE);
    }

    cout << "Connecting to " << hostname << " on port " << port << "..." << endl;
    status = connect(sockAsClient->getSocketFd(), host_info_list_temp->ai_addr, host_info_list_temp->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        exit(EXIT_FAILURE);
    } //if
    freeaddrinfo(host_info_list_temp);
    cout << "Setup connection with origin server!!!\n";
    return sockAsClient;
}
