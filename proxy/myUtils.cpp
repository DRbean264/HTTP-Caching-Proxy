#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <algorithm>
#include "myUtils.hpp"
using namespace std;

string ltrimSpace(const string &s) {
    size_t start = s.find_first_not_of(' ');
    return (start == string::npos) ? "" : s.substr(start);
}
 
string rtrimSpace(const string &s)
{
    size_t end = s.find_last_not_of(' ');
    return (end == string::npos) ? "" : s.substr(0, end + 1);
}
 
string trimSpace(const string &s) {
    return rtrimSpace(ltrimSpace(s));
}

// check if the string is a pure number, positive integer
bool isValidNumber(const std::string &line) {
    if (line.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] < '0' || line[i] > '9')
            return false;
        if (line[i] == '0' && line.size() > 1 && i == 0) {
            return false;
        }
    }
    return true;
}

struct tm getCurrentTime() {
    time_t rawtime;
    struct tm *timeinfo;

    time (&rawtime);
    timeinfo = localtime(&rawtime);
    return *timeinfo;
}

void addSeconds(struct tm *startTime, int seconds) {
    startTime->tm_sec += seconds;
    mktime(startTime);
}

string timeToASC(struct tm timeinfo) {
    string currTime = string(asctime(&timeinfo));
    // remove the new line character
    currTime.pop_back();
    return currTime;
}

struct tm ASCTotime(const string &raw) {
    struct tm timeinfo;
    strptime(raw.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &timeinfo);
    return timeinfo;
}

string time_tToASC(time_t t) {
    struct tm *tminfo;
    tminfo = localtime(&t);
    return timeToASC(*tminfo);
}

void convertToLower(string &s) {
    for_each(s.begin(), s.end(), [](char & c) {
        c = ::tolower(c);
    });
}