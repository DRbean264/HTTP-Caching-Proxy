#ifndef MYLOG_HPP
#define MYLOG_HPP

#include <fstream>
#include <string>

class MyLog {
    std::string logFile;
    std::ofstream ofs;
public:
    MyLog(std::string _logFile);
    ~MyLog();
    void write(std::string);

    void setLogFile(std::string l);
    std::string getLogFile() const;
};

#endif //MYLOG_HPP