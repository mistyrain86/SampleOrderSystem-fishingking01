#pragma once
#include <string>

enum class StockLevel {
    SUFFICIENT,   // 여유: pureQty >= RESERVED 주문 합계
    LOW,          // 부족: 0 < pureQty < RESERVED 주문 합계
    DEPLETED      // 고갈: pureQty == 0
};

struct InventoryStatus {
    std::string sampleId;
    std::string sampleName;
    int         pureQuantity;
    int         reservedQuantity;
    StockLevel  stockLevel;
    double      remainRate;   // pureQty / (pureQty + reservedQty) × 100
};
