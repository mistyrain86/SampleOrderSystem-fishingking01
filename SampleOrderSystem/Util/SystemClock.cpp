#include "SystemClock.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string SystemClock::now() const {
    auto tp = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
