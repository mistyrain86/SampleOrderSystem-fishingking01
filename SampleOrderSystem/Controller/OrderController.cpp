#include "OrderController.h"
#include "Util/Constants.h"
#include <cmath>
#include <iomanip>
#include <sstream>

OrderController::OrderController(IOrderRepository&  orderRepo,
                                 ISampleRepository& sampleRepo,
                                 IClock&            clock)
    : orderRepo_(orderRepo), sampleRepo_(sampleRepo), clock_(clock) {}

int OrderController::calcRequiredProduction(int quantity, double yield) {
    return static_cast<int>(std::ceil(quantity / (yield * PRODUCTION_SAFETY_FACTOR)));
}

std::string OrderController::generateOrderId() const {
    std::ostringstream oss;
    oss << "ORD" << std::setfill('0') << std::setw(4) << (orderRepo_.count() + 1);
    return oss.str();
}

ReserveResult OrderController::reserveOrder(const std::string& sampleId,
                                             const std::string& customerName,
                                             int                quantity) {
    if (!sampleRepo_.findById(sampleId)) {
        return ReserveResult::INVALID_SAMPLE_ID;
    }

    Order order{};
    order.id                 = generateOrderId();
    order.sampleId           = sampleId;
    order.customerName       = customerName;
    order.quantity           = quantity;
    order.status             = OrderStatus::RESERVED;
    order.orderedAt          = clock_.now();
    order.requiredProduction = 0;

    orderRepo_.add(order);
    return ReserveResult::SUCCESS;
}

ApproveResult OrderController::approveOrder(const std::string& orderId) {
    auto optOrder = orderRepo_.findById(orderId);
    if (!optOrder) return ApproveResult::FAILED;

    Order order = *optOrder;
    if (order.status != OrderStatus::RESERVED) return ApproveResult::FAILED;

    auto optSample = sampleRepo_.findById(order.sampleId);
    if (!optSample) return ApproveResult::FAILED;

    Sample sample = *optSample;

    if (sample.pureQuantity >= order.quantity) {
        sample.pureQuantity     -= order.quantity;
        sample.reservedQuantity += order.quantity;
        order.status = OrderStatus::CONFIRMED;
        sampleRepo_.update(sample);
        orderRepo_.update(order);
        return ApproveResult::SUCCESS_CONFIRMED;
    } else {
        int shortage                  = order.quantity - sample.pureQuantity;
        order.requiredProduction      = calcRequiredProduction(shortage, sample.yield);
        order.productionShortage      = shortage;
        order.productionStartedAt     = clock_.now();
        sample.pureQuantity           = 0;   // 가용 재고 전량 소진 → 이중 주문 방지
        sample.reservedQuantity      += order.quantity;
        order.status = OrderStatus::PRODUCING;
        sampleRepo_.update(sample);
        orderRepo_.update(order);
        return ApproveResult::SUCCESS_PRODUCING;
    }
}

bool OrderController::rejectOrder(const std::string& orderId) {
    auto optOrder = orderRepo_.findById(orderId);
    if (!optOrder) return false;

    Order order = *optOrder;
    if (order.status != OrderStatus::RESERVED) return false;

    order.status = OrderStatus::REJECTED;
    orderRepo_.update(order);
    return true;
}

std::vector<Order> OrderController::getReservedOrders() const {
    return orderRepo_.findByStatus(OrderStatus::RESERVED);
}

int OrderController::getOrderCount() const {
    return orderRepo_.count();
}
