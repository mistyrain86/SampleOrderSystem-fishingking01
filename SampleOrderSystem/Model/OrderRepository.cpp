#include "OrderRepository.h"
#include "Model/OrderStatus.h"
#include "Util/json.hpp"
#include <algorithm>
#include <fstream>

OrderRepository::OrderRepository(std::string filePath)
    : filePath_(std::move(filePath)) {
    load();
}

void OrderRepository::load() {
    if (filePath_.empty()) return;
    std::ifstream ifs(filePath_);
    if (!ifs.is_open()) return;

    auto j = nlohmann::json::parse(ifs, nullptr, false);
    if (j.is_discarded()) return;

    for (const auto& item : j) {
        Order o{};
        o.id                  = item["id"].get<std::string>();
        o.sampleId            = item["sampleId"].get<std::string>();
        o.customerName        = item["customerName"].get<std::string>();
        o.quantity            = item["quantity"].get<int>();
        o.status              = fromString(item["status"].get<std::string>());
        o.orderedAt           = item["orderedAt"].get<std::string>();
        o.requiredProduction  = item["requiredProduction"].get<int>();
        o.productionShortage  = item.value("productionShortage",  0);
        o.productionStartedAt = item.value("productionStartedAt", std::string{});
        orders_.push_back(o);
    }
}

void OrderRepository::save() const {
    if (filePath_.empty()) return;
    nlohmann::json j = nlohmann::json::array();
    for (const auto& o : orders_) {
        j.push_back({
            {"id",                   o.id},
            {"sampleId",             o.sampleId},
            {"customerName",         o.customerName},
            {"quantity",             o.quantity},
            {"status",               toString(o.status)},
            {"orderedAt",            o.orderedAt},
            {"requiredProduction",   o.requiredProduction},
            {"productionShortage",   o.productionShortage},
            {"productionStartedAt",  o.productionStartedAt}
        });
    }
    std::ofstream ofs(filePath_);
    ofs << j.dump(2);
}

void OrderRepository::add(const Order& order) {
    orders_.push_back(order);
    save();
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
    save();
    return true;
}

int OrderRepository::count() const {
    return static_cast<int>(orders_.size());
}
