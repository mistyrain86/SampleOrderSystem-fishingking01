#pragma once
#include "Controller/ProductionController.h"
#include "Controller/SampleController.h"

class ProductionView {
public:
    explicit ProductionView(ProductionController& productionCtrl,
                            SampleController&     sampleCtrl);
    void show();

private:
    ProductionController& productionCtrl_;
    SampleController&     sampleCtrl_;
};
