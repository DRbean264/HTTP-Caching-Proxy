#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "HTTPBase.hpp"
#include <string>
#include <exception>

class HTTPRequestWrongFormat : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "The HTTP request is of wrong format.";
    }
};

class HTTPRequest : public HTTPBase {
private:
    int client_connection_fd;
    size_t id;
    std::string method;
    std::string uri;
    int errorCode;
    std::string hostname;
    std::string port;
public:
    HTTPRequest(int _client_connection_fd, std::string raw);
    ~HTTPRequest();
    virtual void parse(const std::string &raw);
    virtual void parseFirstLine(std::string &firstLine);
    bool checkMethod();
    bool checkURI();
    bool checkPort();
    size_t generateRequestUniqueId();

    // getter
    std::string getMethod() const;
    std::string getURI() const;
    size_t getId() const;
    int getClientFd() const;
    int getErrorCode() const;
    std::string getHostName() const;
    std::string getPort() const;
    // setter
    void setMethod(std::string);
    void setURI(std::string);
    void setId(size_t);
    void setHostName(std::string);
    void setPort(std::string);

    virtual void display() const;
    virtual std::string firstLine() const;
};

#endif //HTTPREQUEST_HPP