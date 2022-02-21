#include "MyLog.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <mutex>

using namespace std;

mutex logMutex;

MyLog::MyLog(string _logFile) : logFile(_logFile) {
    // open the file
    ofs.open(logFile, std::ofstream::out);
    cout << "Opened the log file.\n";
}

MyLog::~MyLog() {
    ofs.close();
}

void MyLog::write(string log) {
    logMutex.lock();
    ofs << log << endl;
    logMutex.unlock();
}

void MyLog::setLogFile(string l) {
    logFile = l;
}

string MyLog::getLogFile() const {
    return logFile;
}