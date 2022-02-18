#include "HTTPRequest.hpp"
#include "../myUtils.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <unistd.h>
using namespace std;

static size_t requestUniqueId = 0;
mutex IdMutex;

HTTPRequest::HTTPRequest(int _client_connection_fd, string raw) 
    : HTTPBase(raw), client_connection_fd(_client_connection_fd) {
    setId(generateRequestUniqueId());
    // client side connection closed
    if (raw.size() == 0) {
        errorCode = -1;
    } else {
        try {
            parse(raw);
            checkVersion();
            checkMethod();
            checkURI();
            errorCode = 0;
        }
        catch (const HTTPRequestWrongFormat &e) {
            errorCode = 1;
        }
        catch (const MethodUnsupportedError &e) {
            errorCode = 2;
        }
        catch (const exception &e) {
            errorCode = 3;
        }
    }
}

HTTPRequest::~HTTPRequest() {
    // close(getClientFd());
}

void HTTPRequest::parseFirstLine(string &firstLine) {
    stringstream ss(firstLine);
    string method;
    string uri;
    string version;
    ss >> method >> uri >> version;
    // if there's something after version, wrong format
    if (!ss.eof()) {
        throw HTTPRequestWrongFormat();
    }
    if (ss.bad() || ss.fail()) {
        throw ParseFailure();
    }
    // set and check method, uri, and version
    setMethod(method);
    setURI(uri);
    setVersion(version);
    if (!checkURI() || !checkVersion()) {
        throw HTTPRequestWrongFormat();
    }
    if (!checkMethod())
        throw MethodUnsupportedError();
}

// this will throw exception, remember to deal with it
void HTTPRequest::parse(const string &raw) {
    string delimiter = "\r\n";

    // extract and parse first line
    size_t pos = raw.find(delimiter);
    // if not a single delimiter, wrong format
    if (pos == string::npos)
        throw HTTPRequestWrongFormat();
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
            throw HTTPRequestWrongFormat();
        }

        string header = raw.substr(startPos, pos - startPos);
        parseHeaderField(header);

        startPos = pos + 2;
        pos = raw.find(delimiter, startPos);
    }
    // check if header fields contain 'Host'
    const std::unordered_map<std::string, std::string> &hf = getHeaderFields();
    if (hf.find("host") == hf.cend()) {
        throw HTTPRequestWrongFormat();
    } else {
        string host = hf.find("host")->second;
        size_t colPos = host.find(':');
        // if port specified
        if (colPos != string::npos) {
            setPort(host.substr(colPos + 1, host.size() - colPos - 1));
            if (!checkPort())
                throw HTTPRequestWrongFormat();
        }
        setHostName(host.substr(0, colPos));
    }

    // reach the empty line
    // after it would be optional message
    // anything after the empty line would be the message body
    string messageBody = raw.substr(pos + 2, raw.size() - pos - 2);
    setMessage(messageBody);
}

bool HTTPRequest::checkPort() {
    return isValidNumber(getPort());
}

bool HTTPRequest::checkMethod() {
    unordered_set<string> supportedMethods = {"GET", "POST", "CONNECT"};
    string meth = getMethod();
    // if the method is not supported
    if (supportedMethods.find(meth) == supportedMethods.end()) {
        return false;
    }
    return true;
}

bool HTTPRequest::checkURI() {
    return true;
}

// getter
std::string HTTPRequest::getMethod() const {
    return method;
}

string HTTPRequest::getURI() const {
    return uri;
}

size_t HTTPRequest::getId() const {
    return id;
}

int HTTPRequest::getClientFd() const {
    return client_connection_fd;
}

int HTTPRequest::getErrorCode() const {
    return errorCode;
}
std::string HTTPRequest::getHostName() const {
    return hostname;
}
std::string HTTPRequest::getPort() const {
    return port;
}

// setter
void HTTPRequest::setMethod(std::string m) {
    method = m;
}

void HTTPRequest::setURI(string u) {
    uri = u;
}
void HTTPRequest::setId(size_t i) {
    id = i;
}
void HTTPRequest::setHostName(std::string h) {
    hostname = h;
}
void HTTPRequest::setPort(std::string p) {
    port = p;
}

void HTTPRequest::display() const {
    cout << "\n\nMy http request object:\n";
    cout << "Request ID: " << getId() << '\n';
    cout << "Method: " << getMethod() << '\n';
    cout << "URI: " << getURI() << '\n';
    cout << "HTTP Version: " << getVersion() << '\n';
    cout << "Header Fields:\n";
    for (auto &header : getHeaderFields()) {
        cout << header.first << ": " << header.second << '\n';
    }
    cout << "Message Body:\n";
    cout << getMessage().substr(0, 100) << '\n';
}

size_t HTTPRequest::generateRequestUniqueId() {
    // critical section, add mutex
    IdMutex.lock();
    int id = requestUniqueId;
    ++requestUniqueId;
    IdMutex.unlock();
    return id;
}

string HTTPRequest::firstLine() const {
    stringstream ss;
    ss << getMethod() << ' ' << getURI() << ' ' <<
    getVersion();
    return ss.str();
}