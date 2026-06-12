#pragma once
#include "IClock.h"

class SystemClock : public IClock {
public:
    std::string now() const override;
};
