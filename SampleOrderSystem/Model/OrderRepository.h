#pragma once
#include "IOrderRepository.h"
#include <vector>

class OrderRepository : public IOrderRepository {
public:
    void                 add(const Order& order)                           override;
    std::optional<Order> findById(const std::string& id)           const   override;
    std::vector<Order>   findAll()                                  const   override;
    std::vector<Order>   findByStatus(OrderStatus status)           const   override;
    std::vector<Order>   findBySampleId(const std::string& sampleId) const  override;
    bool                 update(const Order& order)                        override;
    int                  count()                                    const   override;

private:
    std::vector<Order> orders_; // 삽입 순서 = FIFO 순서
};
