#pragma once
#include <string>

class IClock {
public:
    virtual ~IClock() = default;
    virtual std::string now() const = 0; // "YYYY-MM-DD HH:MM:SS"
};
