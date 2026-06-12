#include "OrderRepository.h"
#include <algorithm>

void OrderRepository::add(const Order& order) {
    orders_.push_back(order);
}

std::optional<Order> OrderRepository::findById(const std::string& id) const {
    auto it = std::find_if(orders_.begin(), orders_.end(),
                           [&id](const Order& o) { return o.id == id; });
    if (it == orders_.end()) return std::nullopt;
    return *it;
}

std::vector<Order> OrderRepository::findAll() const {
    return orders_;
}

std::vector<Order> OrderRepository::findByStatus(OrderStatus status) const {
    std::vector<Order> result;
    for (const auto& o : orders_) {
        if (o.status == status)
            result.push_back(o);
    }
    return result;
}

std::vector<Order> OrderRepository::findBySampleId(const std::string& sampleId) const {
    std::vector<Order> result;
    for (const auto& o : orders_) {
        if (o.sampleId == sampleId)
            result.push_back(o);
    }
    return result;
}

bool OrderRepository::update(const Order& order) {
    auto it = std::find_if(orders_.begin(), orders_.end(),
                           [&order](const Order& o) { return o.id == order.id; });
    if (it == orders_.end()) return false;
    *it = order;
    return true;
}

int OrderRepository::count() const {
    return static_cast<int>(orders_.size());
}
