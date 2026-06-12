#include "SampleController.h"

SampleController::SampleController(ISampleRepository& repo, IClock& clock)
    : repo_(repo), clock_(clock) {}

bool SampleController::addSample(const std::string& id,
                                  const std::string& name,
                                  int                pureQuantity,
                                  double             yield,
                                  double             cycleTime) {
    if (repo_.findById(id).has_value()) return false;

    Sample s{};
    s.id               = id;
    s.name             = name;
    s.pureQuantity     = pureQuantity;
    s.reservedQuantity = 0;
    s.yield            = yield;
    s.cycleTime        = cycleTime;
    s.registeredAt     = clock_.now();
    repo_.add(s);
    return true;
}

std::vector<Sample> SampleController::getAllSamples() const {
    return repo_.findAll();
}

std::vector<Sample> SampleController::searchByName(const std::string& keyword) const {
    return repo_.findByName(keyword);
}

int SampleController::getSampleCount() const {
    return repo_.count();
}

int SampleController::getTotalInventory() const {
    int total = 0;
    for (const auto& s : repo_.findAll())
        total += s.pureQuantity + s.reservedQuantity;
    return total;
}
