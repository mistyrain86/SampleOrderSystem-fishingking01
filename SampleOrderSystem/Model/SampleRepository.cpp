#include "SampleRepository.h"
#include "Util/json.hpp"
#include <algorithm>
#include <fstream>

SampleRepository::SampleRepository(std::string filePath)
    : filePath_(std::move(filePath)) {
    load();
}

void SampleRepository::load() {
    if (filePath_.empty()) return;
    std::ifstream ifs(filePath_);
    if (!ifs.is_open()) return;

    auto j = nlohmann::json::parse(ifs, nullptr, false);
    if (j.is_discarded()) return;

    for (const auto& item : j) {
        Sample s{};
        s.id               = item["id"].get<std::string>();
        s.name             = item["name"].get<std::string>();
        s.pureQuantity     = item["pureQuantity"].get<int>();
        s.reservedQuantity = item["reservedQuantity"].get<int>();
        s.yield            = item["yield"].get<double>();
        s.cycleTime        = item["cycleTime"].get<double>();
        s.registeredAt     = item["registeredAt"].get<std::string>();
        samples_.push_back(s);
    }
}

void SampleRepository::save() const {
    if (filePath_.empty()) return;
    nlohmann::json j = nlohmann::json::array();
    for (const auto& s : samples_) {
        j.push_back({
            {"id",               s.id},
            {"name",             s.name},
            {"pureQuantity",     s.pureQuantity},
            {"reservedQuantity", s.reservedQuantity},
            {"yield",            s.yield},
            {"cycleTime",        s.cycleTime},
            {"registeredAt",     s.registeredAt}
        });
    }
    std::ofstream ofs(filePath_);
    ofs << j.dump(2);
}

void SampleRepository::add(const Sample& sample) {
    samples_.push_back(sample);
    save();
}

std::optional<Sample> SampleRepository::findById(const std::string& id) const {
    auto it = std::find_if(samples_.begin(), samples_.end(),
                           [&id](const Sample& s) { return s.id == id; });
    if (it == samples_.end()) return std::nullopt;
    return *it;
}

std::vector<Sample> SampleRepository::findAll() const {
    return samples_;
}

std::vector<Sample> SampleRepository::findByName(const std::string& keyword) const {
    std::vector<Sample> result;
    for (const auto& s : samples_) {
        if (s.name.find(keyword) != std::string::npos)
            result.push_back(s);
    }
    return result;
}

bool SampleRepository::update(const Sample& sample) {
    auto it = std::find_if(samples_.begin(), samples_.end(),
                           [&sample](const Sample& s) { return s.id == sample.id; });
    if (it == samples_.end()) return false;
    *it = sample;
    save();
    return true;
}

bool SampleRepository::remove(const std::string& id) {
    auto it = std::find_if(samples_.begin(), samples_.end(),
                           [&id](const Sample& s) { return s.id == id; });
    if (it == samples_.end()) return false;
    samples_.erase(it);
    save();
    return true;
}

int SampleRepository::count() const {
    return static_cast<int>(samples_.size());
}
