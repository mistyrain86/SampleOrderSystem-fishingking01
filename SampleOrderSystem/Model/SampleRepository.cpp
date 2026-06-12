#include "SampleRepository.h"
#include <algorithm>

void SampleRepository::add(const Sample& sample) {
    samples_.push_back(sample);
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
    return true;
}

bool SampleRepository::remove(const std::string& id) {
    auto it = std::find_if(samples_.begin(), samples_.end(),
                           [&id](const Sample& s) { return s.id == id; });
    if (it == samples_.end()) return false;
    samples_.erase(it);
    return true;
}

int SampleRepository::count() const {
    return static_cast<int>(samples_.size());
}
