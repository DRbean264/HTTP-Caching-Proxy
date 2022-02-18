#ifndef MYUTILS_HPP
#define MYUTILS_HPP

#include <string>
#include <ctime>

std::string ltrimSpace(const std::string &s);
std::string rtrimSpace(const std::string &s);
std::string trimSpace(const std::string &s);

bool isValidNumber(const std::string &line);
struct tm getCurrentTime();
void addSeconds(struct tm *startTime, int seconds);
std::string timeToASC(struct tm timeinfo);
std::string time_tToASC(time_t t);
struct tm ASCTotime(const std::string &raw);
void convertToLower(std::string &s);

#endif //MYUTILS_HPP