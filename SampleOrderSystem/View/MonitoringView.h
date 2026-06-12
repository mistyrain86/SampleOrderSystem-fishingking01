#pragma once
#include "Controller/MonitoringController.h"

class MonitoringView {
public:
    explicit MonitoringView(MonitoringController& monitoringCtrl);
    void show();

private:
    void showOrderSummary()     const;
    void showInventorySummary() const;

    MonitoringController& monitoringCtrl_;
};
