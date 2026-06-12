#pragma once

class SampleController;
class OrderController;

class MainView {
public:
    void setSampleController(SampleController* ctrl);
    void setOrderController(OrderController* ctrl);
    void run();

private:
    void printHeader() const;
    void printMenu()   const;
    int  readChoice()  const;
    void dispatch(int choice);

    SampleController* sampleCtrl_ = nullptr;
    OrderController*  orderCtrl_  = nullptr;
};
