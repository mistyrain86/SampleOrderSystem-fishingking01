#pragma once

class SampleController;

class MainView {
public:
    void setSampleController(SampleController* ctrl);
    void run();

private:
    void printHeader() const;
    void printMenu()   const;
    int  readChoice()  const;
    void dispatch(int choice);

    SampleController* sampleCtrl_ = nullptr;
};
