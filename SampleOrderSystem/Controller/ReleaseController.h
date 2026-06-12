#pragma once
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

class ReleaseController {
public:
    ReleaseController(IOrderRepository&  orderRepo,
                      ISampleRepository& sampleRepo,
                      IClock&            clock);

    std::vector<Order> getConfirmedOrders() const;  // CONFIRMED 주문, FIFO 순
    bool               releaseOrder(const std::string& orderId);

private:
    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
    IClock&            clock_;
};
