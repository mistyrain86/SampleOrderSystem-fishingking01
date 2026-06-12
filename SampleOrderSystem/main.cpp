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
#include "Model/OrderRepository.h"
#include "Util/SystemClock.h"
#include "Util/DummyDataGenerator.h"
#include "Controller/SampleController.h"
#include "Controller/OrderController.h"
#include "Controller/ProductionController.h"
#include "Controller/ReleaseController.h"
#include "View/SplashView.h"
#include "View/MainView.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    SampleRepository sampleRepo;
    OrderRepository  orderRepo;
    SystemClock      clock;

    DummyDataGenerator::populate(sampleRepo, clock);

    SampleController     sampleCtrl(sampleRepo, clock);
    OrderController      orderCtrl(orderRepo, sampleRepo, clock);
    ProductionController productionCtrl(orderRepo, sampleRepo, clock);
    ReleaseController    releaseCtrl(orderRepo, sampleRepo, clock);

    SplashView splash;
    splash.show();

    MainView mainView;
    mainView.setSampleController(&sampleCtrl);
    mainView.setOrderController(&orderCtrl);
    mainView.setProductionController(&productionCtrl);
    mainView.setReleaseController(&releaseCtrl);
    mainView.run();

    return 0;
}
#endif
