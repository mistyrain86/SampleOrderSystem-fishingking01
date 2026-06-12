#include "ProductionController.h"
#include <cmath>

ProductionController::ProductionController(IOrderRepository&  orderRepo,
                                           ISampleRepository& sampleRepo,
                                           IClock&            clock)
    : orderRepo_(orderRepo), sampleRepo_(sampleRepo), clock_(clock) {}

std::vector<Order> ProductionController::getProductionQueue() const {
    return orderRepo_.findByStatus(OrderStatus::PRODUCING);
}

int ProductionController::getProductionCount() const {
    return static_cast<int>(orderRepo_.findByStatus(OrderStatus::PRODUCING).size());
}

bool ProductionController::completeProduction(const std::string& orderId) {
    auto maybeOrder = orderRepo_.findById(orderId);
    if (!maybeOrder) return false;

    Order order = *maybeOrder;
    if (order.status != OrderStatus::PRODUCING) return false;

    auto maybeSample = sampleRepo_.findById(order.sampleId);
    if (!maybeSample) return false;

    Sample sample = *maybeSample;

    // excessProduction = 실제 생산한 것 중 부족분을 초과한 수량 → 가용재고로 귀속
    int excessProduction = order.requiredProduction - order.productionShortage;
    sample.pureQuantity += excessProduction;

    order.status = OrderStatus::CONFIRMED;

    sampleRepo_.update(sample);
    orderRepo_.update(order);
    return true;
}

double ProductionController::calcEstimatedTime(double cycleTime, int requiredProduction) {
    return cycleTime * requiredProduction;
}
