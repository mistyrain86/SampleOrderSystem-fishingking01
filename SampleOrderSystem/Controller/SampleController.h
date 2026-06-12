#pragma once
#include <string>
#include <vector>
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

class SampleController {
public:
    SampleController(ISampleRepository& repo, IClock& clock);

    bool                addSample(const std::string& id,
                                  const std::string& name,
                                  int                pureQuantity,
                                  double             yield,
                                  double             cycleTime);

    std::vector<Sample> getAllSamples()                           const;
    std::vector<Sample> searchByName(const std::string& keyword) const;

    int getSampleCount()    const;
    int getTotalInventory() const;

private:
    ISampleRepository& repo_;
    IClock&            clock_;
};
