#pragma once

class MainView {
public:
    void run();
private:
    void printHeader() const;
    void printMenu() const;
    int  readChoice() const;
    void dispatch(int choice);
};
