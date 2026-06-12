#pragma once
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

class ProductionController {
public:
    ProductionController(IOrderRepository&  orderRepo,
                         ISampleRepository& sampleRepo,
                         IClock&            clock);

    std::vector<Order> getProductionQueue() const;  // PRODUCING 주문, FIFO 순
    int                getProductionCount() const;  // MainView 현황 표시용
    bool               completeProduction(const std::string& orderId);

    // 예상 생산 시간 (min) = cycleTime × requiredProduction
    static double calcEstimatedTime(double cycleTime, int requiredProduction);

private:
    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
    IClock&            clock_;
};
