#pragma once
#include <string>
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

enum class ReserveResult { SUCCESS, INVALID_SAMPLE_ID };
enum class ApproveResult { SUCCESS_CONFIRMED, SUCCESS_PRODUCING, FAILED };

class OrderController {
public:
    OrderController(IOrderRepository&  orderRepo,
                    ISampleRepository& sampleRepo,
                    IClock&            clock);

    ReserveResult reserveOrder(const std::string& sampleId,
                               const std::string& customerName,
                               int                quantity);

    ApproveResult approveOrder(const std::string& orderId);
    bool          rejectOrder(const std::string& orderId);

    std::vector<Order> getReservedOrders() const;
    int                getOrderCount()     const;

    // PRD §4-2: ceil(quantity / (yield × PRODUCTION_SAFETY_FACTOR))
    static int calcRequiredProduction(int quantity, double yield);

private:
    std::string generateOrderId() const;

    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
    IClock&            clock_;
};
