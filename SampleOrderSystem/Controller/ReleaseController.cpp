#include "ReleaseController.h"

ReleaseController::ReleaseController(IOrderRepository&  orderRepo,
                                     ISampleRepository& sampleRepo,
                                     IClock&            clock)
    : orderRepo_(orderRepo), sampleRepo_(sampleRepo), clock_(clock) {}

std::vector<Order> ReleaseController::getConfirmedOrders() const {
    return orderRepo_.findByStatus(OrderStatus::CONFIRMED);
}

bool ReleaseController::releaseOrder(const std::string& orderId) {
    auto maybeOrder = orderRepo_.findById(orderId);
    if (!maybeOrder) return false;

    Order order = *maybeOrder;
    if (order.status != OrderStatus::CONFIRMED) return false;

    auto maybeSample = sampleRepo_.findById(order.sampleId);
    if (!maybeSample) return false;

    Sample sample = *maybeSample;

    sample.reservedQuantity -= order.quantity;
    order.status = OrderStatus::RELEASE;

    sampleRepo_.update(sample);
    orderRepo_.update(order);
    return true;
}
