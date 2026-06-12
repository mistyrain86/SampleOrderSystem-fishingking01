#pragma once
#include "ISampleRepository.h"
#include <vector>
#include <string>

class SampleRepository : public ISampleRepository {
public:
    explicit SampleRepository(std::string filePath = "");

    void                  add(const Sample& sample)                    override;
    std::optional<Sample> findById(const std::string& id)       const  override;
    std::vector<Sample>   findAll()                              const  override;
    std::vector<Sample>   findByName(const std::string& keyword) const  override;
    bool                  update(const Sample& sample)                  override;
    bool                  remove(const std::string& id)                 override;
    int                   count()                                const  override;

private:
    void load();
    void save() const;

    std::string         filePath_;
    std::vector<Sample> samples_;
};
