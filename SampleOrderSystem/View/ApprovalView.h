#pragma once

class OrderController;
class SampleController;

class ApprovalView {
public:
    explicit ApprovalView(OrderController& orderCtrl, SampleController& sampleCtrl);
    void show();

private:
    OrderController&  orderCtrl_;
    SampleController& sampleCtrl_;
};
