#include "HTTPResponse.hpp"
#include "../myUtils.hpp"
#include "../MyLog/MyLog.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <unordered_set>
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

// default constructor with 200 OK response
HTTPResponse::HTTPResponse() 
    : HTTPBase(OK200), requestId(0) {
    setRaw(OK200);
    parse(OK200);
}

HTTPResponse::HTTPResponse(string raw, size_t _id) 
    : HTTPBase(raw), requestId(_id) {
    setRaw(raw);
    parse(raw);
}

void HTTPResponse::parseFirstLine(string &firstLine) {
    stringstream ss(firstLine);
    string ver;
    string status;
    string reason;
    ss >> ver >> status;
    if (ss.eof()) 
        throw HTTPResponseWrongFormat();
    else if (ss.bad() || ss.fail())
        throw ParseFailure();
    size_t reasonStart = ver.size() + status.size() + 2;
    reason = firstLine.substr(reasonStart, firstLine.size() - reasonStart);

    // set and check version, status
    setVersion(ver);
    setStatusCode(status);
    setReasonPhrase(reason);
    if (!checkStatusCode() || !checkVersion()) {
        throw HTTPResponseWrongFormat();
    }
}

// this will throw exception, remember to deal with it
void HTTPResponse::parse(const string &raw) {
    // cout << raw << endl;
    string delimiter = "\r\n";

    // extract and parse first line
    size_t pos = raw.find(delimiter);
    // if not a single delimiter, wrong format
    if (pos == string::npos)
        throw HTTPResponseWrongFormat();
    string firstLine = raw.substr(0, pos);
    parseFirstLine(firstLine);

    // extract >=0 header fields from line 2
    size_t startPos = pos + 2;
    pos = raw.find(delimiter, startPos);
    // loop through all header fields
    // while not reach the empty line
    while (pos != startPos) {
        // if not find any delimiter, wrong format
        if (pos == string::npos) {
            throw HTTPResponseWrongFormat();
        }

        string header = raw.substr(startPos, pos - startPos);
        parseHeaderField(header);

        startPos = pos + 2;
        pos = raw.find(delimiter, startPos);
    }

    if (!checkHeaderFields())
        throw HTTPHeaderWrongFormat();
    processHeaderFields();
    // reach the empty line
    // after it would be optional message
    // anything after the empty line would be the message body
    string messageBody;
    // = raw.substr(pos + 2, raw.size() - pos - 2);
    
    // check if the message length exists
    const std::unordered_map<std::string, std::string> &hf = getHeaderFields();
    if (hf.find("content-length") != hf.cend()) {
        string lengths = hf.find("content-length")->second;
        unsigned long lengthd = std::stoul(lengths, NULL, 0);
        messageBody = raw.substr(pos + 2, lengthd);
    } else {
        messageBody = raw.substr(pos + 2, raw.size() - pos - 2);
    }
    setMessage(messageBody);

    // display();

    if (hf.find("content-length") != hf.cend()) {
        string lengths = hf.find("content-length")->second;
        unsigned long lengthd = strtoul(lengths.c_str(), NULL, 10);
        // cout << "Message length: " << getMessage().size() << endl;
        // cout << "Header content length: " << lengths << endl;
        assert(lengthd == getMessage().size());
    }
}

bool HTTPResponse::checkHeaderFields() const {
    const std::unordered_map<std::string, std::string> &hf = getHeaderFields();
    // A sender MUST NOT send a Content-Length header field in any message
    // that contains a Transfer-Encoding header field.
    if (hf.find("transfer-encoding") != hf.cend() &&
        hf.find("content-length") != hf.cend()) {
        return false;
    }

    // content-length should be a valid number
    if (hf.find("content-length") != hf.cend()) {
        size_t idx;
        string lengths = hf.find("content-length")->second;
        // unsigned long lengthd = strtoul(lengths.c_str(), NULL, 10);
        std::stoul(lengths, &idx, 0);
        if (idx != lengths.size()) {
            return false;
        }
    }
    return true;
}

void HTTPResponse::requireValidationHelper(const unordered_map<string, string> &directives) {
    // if either proxy-revalidate or must-revalidate exists
    if (directives.find("proxy-revalidate") != directives.cend() ||
    directives.find("must-revalidate") != directives.cend()) {
        requireValidation = true;
    } else {
        requireValidation = false;
    }
}

void HTTPResponse::maxAgeHelper(const unordered_map<string, string> &directives, string age) {
    string seconds = directives.find(age)->second;
    // should be a non-negative integer
    if (!isValidNumber(seconds))
        throw HTTPResponseWrongFormat();
    unsigned long sec = strtoul(seconds.c_str(), NULL, 10);
    // if maxage is 0, then treat it as no-cache
    if (sec == 0) {
        validExpireTime = false;
        requireValidation = true;
    } else {
        validExpireTime = true;
        sec = min(sec, (unsigned long)1 << 31);
        struct tm expire = getReceiveTime();
        addSeconds(&expire, sec);
        setExpireTime(expire);
        requireValidationHelper(directives);
    }
}

void HTTPResponse::expiresHelper(const unordered_map<string, string> &hf) {
    validExpireTime = true;
    string timestamp = hf.find("expires")->second;
    struct tm expire;
    // if the timestamp format is not correct
    if (strptime(timestamp.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &expire) == NULL) {
        // set the expiration time to the past
        struct tm expire = getCurrentTime();
        addSeconds(&expire, -3000);
        setExpireTime(expire);
    } 
    // if the timestamp format is correct
    else {
        setExpireTime(expire);
    }
}

// determine if the response is cacheable, require validation,
// or has valid expiration time to check
void HTTPResponse::processHeaderFields() {
    const unordered_map<string, string> &hf = getHeaderFields();
    string cacheControl;
    
    // if there is no cache-control header
    if (hf.find("cache-control") == hf.cend()) {
        requireValidation = false;
        cacheable = true;
        // if no expires header
        if (hf.find("expires") == hf.cend()) {
            // manually set expiration time to 1 months later
            struct tm expire = getCurrentTime();
            addSeconds(&expire, 2592000);
            setExpireTime(expire);
            return;
        } else {
            expiresHelper(hf);
            return;
        }
    }

    // if there is cache-control header
    unordered_map<string, string> directives = parseCacheControl(hf.find("cache-control")->second);
    // if there is private/no-store directives or the response is not 200 OK
    // then we should not cache it
    if (directives.find("private") != directives.end() ||
    directives.find("no-store") != directives.end() ||
    getStatusCode() != "200") {
        cacheable = false;
        validExpireTime = false;
        requireValidation = false;
        // ID: not cacheable because REASON
        if (directives.find("private") != directives.end()) {
            reason = "this response is private";
        } else if (directives.find("no-store") != directives.end()) {
            reason = "this response specified as no-store";
        }
        return;
    }
    // all the following conditions are cacheable
    cacheable = true;
    // if there's no-cache
    if (directives.find("no-cache") != directives.end()) {
        validExpireTime = false;
        requireValidation = true;
        return;
    }
    // if there's s-maxage
    if (directives.find("s-maxage") != directives.end()) {
        maxAgeHelper(directives, "s-maxage");
        return;
    }
    // if there's max-age
    if (directives.find("max-age") != directives.end()) {
        maxAgeHelper(directives, "max-age");
        return;
    }
    // if there's expires
    if (hf.find("expires") != hf.cend()) {
        expiresHelper(hf);
        requireValidationHelper(directives);
        return;
    }
    // else, no expiration specifications
    // manually set expiration time to 1 months later
    validExpireTime = true;
    struct tm expire = getCurrentTime();
    addSeconds(&expire, 2592000);
    setExpireTime(expire);
    requireValidationHelper(directives);
}

// directives are case-insensitive
unordered_map<string, string> HTTPResponse::parseCacheControl(const string &raw) {
    unordered_map<string, string> directives;
    // find comma
    size_t commaPos = raw.find(',');
    size_t startPos = 0;
    string directive;
    while (commaPos != string::npos) {
        directive = trimSpace(raw.substr(startPos, commaPos - startPos));
        convertToLower(directive);
        addToDict(directive, directives);
        startPos = commaPos + 1;
        commaPos = raw.find(',', startPos);
    }
    directive = trimSpace(raw.substr(startPos, raw.size() - startPos));
    convertToLower(directive);
    addToDict(directive, directives);
    return directives;
}

void HTTPResponse::addToDict(const string &directive, unordered_map<string, string> &directives) {
    string key;
    string value;
    // find =
    size_t equalPos = directive.find('=');
    if (equalPos != string::npos) {
        key = directive.substr(0, equalPos);
        value = directive.substr(equalPos + 1, directive.size() - equalPos - 1);
    } else {
        key = directive;
    }
    directives[key] = value;
}

// the status code is 3-digit integer starting with only 1,2,3,4,5
bool HTTPResponse::checkStatusCode() const {
    if (statusCode.size() != 3)
        return false;
    unordered_set<char> correct = {'1','2','3','4','5'};
    if (correct.find(statusCode[0]) == correct.end())
        return false;
    // check if the other 2 chars are numbers
    if (statusCode[1] < '0' || statusCode[1] > '9' ||
        statusCode[2] < '0' || statusCode[2] > '9')
        return false;
    return true;
}

// getter
std::string HTTPResponse::getStatusCode() const {
    return statusCode;
}
std::string HTTPResponse::getReasonPhrase() const {
    return reasonPhrase;
}
struct tm HTTPResponse::getExpireTime() const {
    return expireTime;
}
size_t HTTPResponse::getRequestId() const {
    return requestId;
}

// setter
void HTTPResponse::setStatusCode(std::string status) {
    statusCode = status;
}
void HTTPResponse::setReasonPhrase(std::string reason) {
    reasonPhrase = reason;
}
void HTTPResponse::setExpireTime(struct tm t) {
    expireTime = t;
}
void HTTPResponse::setRequestId(size_t id) {
    requestId = id;
}

std::string HTTPResponse::firstLine() const {
    stringstream ss;
    ss << getVersion() << ' ' << getStatusCode() << ' ' <<
    getReasonPhrase();
    return ss.str();
}

void HTTPResponse::display() const {
    cout << "\n\nMy http response object:\n";
    cout << "ID: " << getRequestId() << '\n';
    cout << "Status: " << getStatusCode() << '\n';
    cout << "Reason Phrase: " << getReasonPhrase() << '\n';
    cout << "HTTP Version: " << getVersion() << '\n';
    cout << "Header Fields:\n";
    for (auto &header : getHeaderFields()) {
        cout << header.first << ": " << header.second << '\n';
    }
    cout << "Message Body:\n";
    cout << getMessage().substr(0, 105) << "...\n";
}

std::ostream &operator<<(std::ostream & out, const HTTPResponse &response) {
    out << response.getRequestId() << "--" << response.firstLine() << endl;
    return out;
}