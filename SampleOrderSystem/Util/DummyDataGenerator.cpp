#include "DummyDataGenerator.h"

void DummyDataGenerator::populate(ISampleRepository& sampleRepo, IClock& clock) {
    if (sampleRepo.count() > 0) return;

    const std::string now = clock.now();

    // PDF p.13 시료 목록 예시 기반 하드코딩
    // { id, name, pureQuantity, reservedQuantity, yield, cycleTime }
    struct SampleData {
        const char* id;
        const char* name;
        int         pureQuantity;
        double      yield;
        double      cycleTime;
    };

    constexpr SampleData entries[] = {
        { "S-001", "실리콘 웨이퍼-8인치", 480,  0.92, 0.5 },
        { "S-002", "GaN 에피택셜-4인치",  220,  0.78, 0.3 },
        { "S-003", "SiC 파워기판-6인치",   30,  0.85, 1.2 },
        { "S-004", "산화막 웨이퍼-SiO2",    0,  0.90, 0.4 },
        { "S-005", "포토레지스트-PR7",     400,  0.95, 0.2 },
    };

    for (const auto& e : entries) {
        Sample s{};
        s.id               = e.id;
        s.name             = e.name;
        s.pureQuantity     = e.pureQuantity;
        s.reservedQuantity = 0;
        s.yield            = e.yield;
        s.cycleTime        = e.cycleTime;
        s.registeredAt     = now;
        sampleRepo.add(s);
    }
}
