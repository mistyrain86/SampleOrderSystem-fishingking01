#pragma once
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Model/OrderSummary.h"
#include "Model/InventoryStatus.h"

class MonitoringController {
public:
    MonitoringController(IOrderRepository&  orderRepo,
                         ISampleRepository& sampleRepo);

    OrderSummary                 getOrderSummary()     const;
    std::vector<InventoryStatus> getInventorySummary() const;

private:
    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
};
