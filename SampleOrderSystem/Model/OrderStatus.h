#pragma once
#include <string>

enum class OrderStatus {
    RESERVED,   // 주문 예약 (초기 상태)
    REJECTED,   // 주문 거절
    PRODUCING,  // 생산 중 (순수 재고 부족)
    CONFIRMED,  // 주문 확정 (재고 충분 or 생산 완료)
    RELEASE     // 출고 완료
};

inline std::string toString(OrderStatus status) {
    switch (status) {
        case OrderStatus::RESERVED:  return "RESERVED";
        case OrderStatus::REJECTED:  return "REJECTED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::RELEASE:   return "RELEASE";
        default:                     return "UNKNOWN";
    }
}

inline OrderStatus fromString(const std::string& s) {
    if (s == "REJECTED")  return OrderStatus::REJECTED;
    if (s == "PRODUCING") return OrderStatus::PRODUCING;
    if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (s == "RELEASE")   return OrderStatus::RELEASE;
    return OrderStatus::RESERVED; // RESERVED 및 알 수 없는 값 기본값
}
