#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include "HTTPBase.hpp"
#include "../MyLog/MyLog.hpp"
#include <string>

class HTTPResponseWrongFormat : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "The HTTP response is of wrong format.";
    }
};

class HTTPResponse : public HTTPBase {
private:
    size_t requestId;
    std::string statusCode;
    std::string reasonPhrase;
    struct tm expireTime;
public:
    bool cacheable;
    bool requireValidation;
    bool validExpireTime;
    // if this response is not cacheable, then the reason would be 
    // this string
    std::string reason;

    HTTPResponse();
    HTTPResponse(std::string raw, size_t _id);
    virtual void parse(const std::string &raw);
    virtual void parseFirstLine(std::string &firstLine);
    bool checkStatusCode() const;
    bool checkHeaderFields() const;
    void processHeaderFields();
    std::unordered_map<std::string, std::string> parseCacheControl(const std::string &);
    void requireValidationHelper(const std::unordered_map<std::string, std::string> &);
    void maxAgeHelper(const std::unordered_map<std::string, std::string> &, std::string);
    void expiresHelper(const std::unordered_map<std::string, std::string> &);
    void addToDict(const std::string &directive, std::unordered_map<std::string, std::string> &);
    virtual void display() const;

    // getter
    std::string getStatusCode() const;
    std::string getReasonPhrase() const;
    struct tm getExpireTime() const;
    size_t getRequestId() const;

    // setter
    void setStatusCode(std::string);
    void setReasonPhrase(std::string);
    void setExpireTime(struct tm);
    void setRequestId(size_t);

    virtual std::string firstLine() const;
};

#endif //HTTP_RESPONSE