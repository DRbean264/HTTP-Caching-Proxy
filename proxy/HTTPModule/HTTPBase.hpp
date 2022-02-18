#ifndef HTTPBASE_HPP
#define HTTPBASE_HPP

#include <unordered_map>
#include <string>
#include <ctime>

class HTTPHeaderWrongFormat : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "The HTTP header is of wrong format.";
    }
};

class MethodUnsupportedError : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "The HTTP request method is not supported so far.";
    }
};

class ParseFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to parse the http request.";
    }
};

class HTTPBase {
private:
    std::string version;
    std::unordered_map<std::string, std::string> headerFields;
    std::string message;
    std::string raw;
    struct tm receiveTime;

public:
    std::unordered_map<std::string, std::string> cacheControlFields;
    HTTPBase(std::string _raw);
    virtual void parse(const std::string &raw) = 0;
    virtual void parseFirstLine(std::string &firstLine) = 0;
    void parseHeaderField(std::string &header);
    bool checkVersion();

    // getter 
    std::string getVersion() const;
    std::string getMessage() const;
    std::string getRaw() const;
    const std::unordered_map<std::string, std::string> &getHeaderFields() const;
    struct tm getReceiveTime() const;
    // setter
    void setVersion(std::string);
    void setMessage(std::string);
    void setRaw(std::string);
    void setHeaderFields(std::unordered_map<std::string, std::string>);
    void setReceiveTime(struct tm);

    void addHeaderField(const std::string &, const std::string &);
    virtual void display() const = 0;
    virtual std::string firstLine() const = 0;
};

#endif //HTTPBASE_HPP