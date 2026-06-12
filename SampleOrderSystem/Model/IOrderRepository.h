#pragma once
#include <optional>
#include <string>
#include <vector>
#include "Order.h"
#include "OrderStatus.h"

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    virtual void                 add(const Order& order)                           = 0;
    virtual std::optional<Order> findById(const std::string& id)           const   = 0;
    virtual std::vector<Order>   findAll()                                  const   = 0;
    virtual std::vector<Order>   findByStatus(OrderStatus status)           const   = 0;
    virtual std::vector<Order>   findBySampleId(const std::string& sampleId) const  = 0;
    virtual bool                 update(const Order& order)                        = 0;
    virtual int                  count()                                    const   = 0;
};
