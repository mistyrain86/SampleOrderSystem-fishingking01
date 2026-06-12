#include "MonitoringController.h"

MonitoringController::MonitoringController(IOrderRepository&  orderRepo,
                                           ISampleRepository& sampleRepo)
    : orderRepo_(orderRepo), sampleRepo_(sampleRepo) {}

OrderSummary MonitoringController::getOrderSummary() const {
    OrderSummary summary;
    for (const auto& o : orderRepo_.findAll()) {
        switch (o.status) {
        case OrderStatus::RESERVED:  summary.reserved++;  break;
        case OrderStatus::PRODUCING: summary.producing++; break;
        case OrderStatus::CONFIRMED: summary.confirmed++; break;
        case OrderStatus::RELEASE:   summary.released++;  break;
        default: break; // REJECTED 무시
        }
    }
    return summary;
}

std::vector<InventoryStatus> MonitoringController::getInventorySummary() const {
    const auto samples        = sampleRepo_.findAll();
    const auto reservedOrders = orderRepo_.findByStatus(OrderStatus::RESERVED);

    std::vector<InventoryStatus> result;
    for (const auto& s : samples) {
        int reservedOrderQty = 0;
        for (const auto& o : reservedOrders) {
            if (o.sampleId == s.id) {
                reservedOrderQty += o.quantity;
            }
        }

        StockLevel level;
        if (s.pureQuantity == 0) {
            level = StockLevel::DEPLETED;
        } else if (s.pureQuantity < reservedOrderQty) {
            level = StockLevel::LOW;
        } else {
            level = StockLevel::SUFFICIENT;
        }

        int    total      = s.pureQuantity + s.reservedQuantity;
        double remainRate = (total > 0) ? (s.pureQuantity * 100.0 / total) : 0.0;

        result.push_back({s.id, s.name, s.pureQuantity, s.reservedQuantity, level, remainRate});
    }
    return result;
}
