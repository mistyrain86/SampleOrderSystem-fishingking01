#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

#else
#include <windows.h>
#include "View/SplashView.h"
#include "View/MainView.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SplashView splash;
    splash.show();
    MainView mainView;
    mainView.run();
    return 0;
}
#endif
