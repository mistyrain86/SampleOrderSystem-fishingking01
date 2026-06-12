#pragma once
#include "Controller/SampleController.h"

class SampleView {
public:
    explicit SampleView(SampleController& controller);
    void show();

private:
    void showRegister();
    void showList()   const;
    void showSearch() const;
    int  readChoice() const;

    SampleController& controller_;
};
