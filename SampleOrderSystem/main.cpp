#ifdef _DEBUG
#include <gtest/gtest.h>
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

#else
#include <windows.h>
#include "Model/SampleRepository.h"
#include "Util/SystemClock.h"
#include "Util/DummyDataGenerator.h"
#include "Controller/SampleController.h"
#include "View/SplashView.h"
#include "View/MainView.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 인프라
    SampleRepository sampleRepo;
    SystemClock      clock;

    // 최초 실행 시 PDF p.13 예시 데이터 자동 삽입
    DummyDataGenerator::populate(sampleRepo, clock);

    // Controller
    SampleController sampleCtrl(sampleRepo, clock);

    // View
    SplashView splash;
    splash.show();

    MainView mainView;
    mainView.setSampleController(&sampleCtrl);
    mainView.run();

    return 0;
}
#endif
