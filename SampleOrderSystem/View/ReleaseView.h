#pragma once
#include "Controller/ReleaseController.h"
#include "Controller/SampleController.h"

class ReleaseView {
public:
    explicit ReleaseView(ReleaseController& releaseCtrl,
                         SampleController&  sampleCtrl);
    void show();

private:
    ReleaseController& releaseCtrl_;
    SampleController&  sampleCtrl_;
};
