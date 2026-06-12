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

    // 실시간 미적립 잔여분을 마저 반영 (shortage→reservedQty, 초과→pureQty)
    int origReserved                = order.quantity - order.productionShortage;
    int productionCreditedToReserved = sample.reservedQuantity - origReserved;
    int totalCredited               = productionCreditedToReserved + sample.pureQuantity;
    int remaining                   = order.requiredProduction - totalCredited;
    if (remaining > 0) {
        int reservedRemaining = order.productionShortage - productionCreditedToReserved;
        int toReserved = std::min(remaining, std::max(0, reservedRemaining));
        int toPure     = remaining - toReserved;
        sample.reservedQuantity += toReserved;
        sample.pureQuantity     += toPure;
    }

    order.status = OrderStatus::CONFIRMED;
    sampleRepo_.update(sample);
    orderRepo_.update(order);
    return true;
}

void ProductionController::creditRealTimeExcess(const std::string& orderId,
                                                int producedSoFar) {
    auto maybeOrder = orderRepo_.findById(orderId);
    if (!maybeOrder) return;
    const Order& order = *maybeOrder;
    if (order.status != OrderStatus::PRODUCING) return;

    auto maybeSample = sampleRepo_.findById(order.sampleId);
    if (!maybeSample) return;
    Sample sample = *maybeSample;

    // shortage 이하 → reservedQty 적립 / 초과분 → pureQty 적립
    int origReserved                 = order.quantity - order.productionShortage;
    int productionCreditedToReserved = sample.reservedQuantity - origReserved;
    int totalCredited                = productionCreditedToReserved + sample.pureQuantity;
    int delta                        = producedSoFar - totalCredited;
    if (delta <= 0) return;

    int reservedRemaining = order.productionShortage - productionCreditedToReserved;
    int toReserved = std::min(delta, std::max(0, reservedRemaining));
    int toPure     = delta - toReserved;
    if (toReserved > 0) sample.reservedQuantity += toReserved;
    if (toPure     > 0) sample.pureQuantity     += toPure;
    sampleRepo_.update(sample);
}

double ProductionController::calcEstimatedTime(double cycleTime, int requiredProduction) {
    return cycleTime * requiredProduction;
}
