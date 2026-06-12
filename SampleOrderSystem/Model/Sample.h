#pragma once
#include <string>

struct Sample {
    std::string id;
    std::string name;
    int         pureQuantity;     // 순수 재고 — 어떤 주문에도 묶이지 않은 가용 재고
    int         reservedQuantity; // 주문 접수 재고 — 출고 대기 중인 재고
    double      yield;            // 수율 (0.0 ~ 1.0)
    double      cycleTime;        // 생산 사이클타임 (min/ea)
    std::string registeredAt;     // 등록 일시 (YYYY-MM-DD HH:MM:SS)
};
