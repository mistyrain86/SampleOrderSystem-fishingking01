#pragma once

class OrderController;
class SampleController;

class OrderView {
public:
    explicit OrderView(OrderController& orderCtrl, SampleController& sampleCtrl);
    void show();

private:
    OrderController&  orderCtrl_;
    SampleController& sampleCtrl_;
};
