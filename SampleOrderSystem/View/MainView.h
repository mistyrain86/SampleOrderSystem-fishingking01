#pragma once

class SampleController;
class OrderController;
class ProductionController;
class ReleaseController;
class MonitoringController;

class MainView {
public:
    void setSampleController(SampleController*         ctrl);
    void setOrderController(OrderController*           ctrl);
    void setProductionController(ProductionController* ctrl);
    void setReleaseController(ReleaseController*       ctrl);
    void setMonitoringController(MonitoringController* ctrl);
    void run();

private:
    void printHeader() const;
    void printMenu()   const;
    int  readChoice()  const;
    void dispatch(int choice);

    SampleController*     sampleCtrl_     = nullptr;
    OrderController*      orderCtrl_      = nullptr;
    ProductionController* productionCtrl_ = nullptr;
    ReleaseController*    releaseCtrl_    = nullptr;
    MonitoringController* monitoringCtrl_ = nullptr;
};
