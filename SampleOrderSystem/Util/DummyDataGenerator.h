#pragma once
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

class DummyDataGenerator {
public:
    // 시료 저장소가 비어있을 때만 PDF p.13 예시 데이터를 삽입한다.
    static void populate(ISampleRepository& sampleRepo, IClock& clock);
};
