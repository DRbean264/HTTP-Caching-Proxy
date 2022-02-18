#include <string>
#include <unordered_map>
#include <iostream>
#include "HTTPBase.hpp"
#include "../myUtils.hpp"
using namespace std;

HTTPBase::HTTPBase(string _raw) {
    setRaw(_raw);
    setReceiveTime(getCurrentTime());
}

bool HTTPBase::checkVersion() {
    // must be HTTP/[Integer].[Integer] or HTTP/[Integer]
    string ver = getVersion();
    size_t dotPos = ver.find('.');
    if (ver.substr(0, 5) != "HTTP/") {
        return false;
    }
    string major = ver.substr(5, dotPos - 5);
    string minor = "0";
    if (dotPos != string::npos) {
        minor = ver.substr(dotPos + 1, ver.size() - 1 - dotPos);
    }
    return isValidNumber(major) && isValidNumber(minor);
}

// the headers' names are case-insensitive
void HTTPBase::parseHeaderField(std::string &header) {
    size_t colonPos = header.find(":");
    // if not find a colon, wrong format
    if (colonPos == string::npos) {
        throw HTTPHeaderWrongFormat();
    }

    // convert key to lower case
    string key = header.substr(0, colonPos);
    convertToLower(key);
    string value = header.substr(colonPos + 1, header.size() - 1 - colonPos);
    // get rid of the leading and trailing space of the value
    value = trimSpace(value);
    // store it into headerFields
    addHeaderField(key, value);
}

// getter 
string HTTPBase::getVersion() const {
    return version;
}

string HTTPBase::getMessage() const {
    return message;
}
string HTTPBase::getRaw() const {
    return raw;
}
const unordered_map<string, string> &HTTPBase::getHeaderFields() const {
    return headerFields;
}
struct tm HTTPBase::getReceiveTime() const {
    return receiveTime;
}
// setter
void HTTPBase::setVersion(string v) {
    version = v;
}

void HTTPBase::setMessage(string m) {
    message = m;
}
void HTTPBase::setRaw(string r) {
    raw = r;
}
void HTTPBase::setHeaderFields(unordered_map<string, string> hf) {
    headerFields = hf;
}
void HTTPBase::setReceiveTime(struct tm t) {
    receiveTime = t;
}

void HTTPBase::addHeaderField(const string &key, const string &value) {
    headerFields[key] = value;
}