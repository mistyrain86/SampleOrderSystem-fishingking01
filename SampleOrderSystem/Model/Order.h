#pragma once
#include <string>
#include "OrderStatus.h"

struct Order {
    std::string id;
    std::string sampleId;
    std::string customerName;
    int         quantity;
    OrderStatus status;
    std::string orderedAt;          // 주문 생성 일시 (YYYY-MM-DD HH:MM:SS)
    int         requiredProduction; // PRODUCING 전환 시 1회 산정 후 불변
};
