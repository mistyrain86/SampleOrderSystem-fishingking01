#include "MainView.h"
#include "SampleView.h"
#include "OrderView.h"
#include "ApprovalView.h"
#include "MonitoringView.h"
#include "ProductionView.h"
#include "ReleaseView.h"
#include "Controller/SampleController.h"
#include "Controller/OrderController.h"
#include "Controller/ProductionController.h"
#include "Controller/ReleaseController.h"
#include <iostream>
#include <string>
#include <ctime>
#include <limits>

void MainView::setSampleController(SampleController* ctrl) {
    sampleCtrl_ = ctrl;
}

void MainView::setOrderController(OrderController* ctrl) {
    orderCtrl_ = ctrl;
}

void MainView::setProductionController(ProductionController* ctrl) {
    productionCtrl_ = ctrl;
}

void MainView::setReleaseController(ReleaseController* ctrl) {
    releaseCtrl_ = ctrl;
}

void MainView::run() {
    while (true) {
        system("cls");
        printHeader();
        printMenu();
        int choice = readChoice();
        if (choice == 0) {
            std::cout << "\n  시스템을 종료합니다. 안녕히 가세요.\n\n";
            break;
        }
        dispatch(choice);
    }
}

void MainView::printHeader() const {
    std::time_t now = std::time(nullptr);
    std::tm tmNow{};
    localtime_s(&tmNow, &now);
    char timeStr[32];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tmNow);

    int sampleCount      = sampleCtrl_     ? sampleCtrl_->getSampleCount()        : 0;
    int totalInventory   = sampleCtrl_     ? sampleCtrl_->getTotalInventory()      : 0;
    int orderCount       = orderCtrl_      ? orderCtrl_->getOrderCount()           : 0;
    int productionCount  = productionCtrl_ ? productionCtrl_->getProductionCount() : 0;

    std::cout << "================================================================\n";
    std::cout << "  S-Semi 반도체 시료 생산주문관리 시스템\n";
    std::cout << "================================================================\n";
    std::cout << "  시스템 현황   " << timeStr << "\n\n";
    std::cout << "  등록 시료   " << sampleCount    << " 종      총 재고   " << totalInventory << " ea\n";
    std::cout << "  전체 주문   " << orderCount     << " 건      생산라인  " << productionCount << " 건 대기\n";
    std::cout << "----------------------------------------------------------------\n";
}

void MainView::printMenu() const {
    std::cout << "  [1] 시료 관리                   [2] 시료 주문\n";
    std::cout << "  [3] 주문 승인/거절               [4] 모니터링\n";
    std::cout << "  [5] 생산라인 조회                [6] 출고 처리\n";
    std::cout << "  [0] 종료\n";
    std::cout << "----------------------------------------------------------------\n";
}

int MainView::readChoice() const {
    std::cout << "  선택 > ";
    std::string line;
    std::getline(std::cin, line);
    try { return std::stoi(line); }
    catch (...) { return -1; }
}

void MainView::dispatch(int choice) {
    switch (choice) {
    case 1:
        if (sampleCtrl_) { SampleView v(*sampleCtrl_); v.show(); }
        break;
    case 2:
        if (orderCtrl_ && sampleCtrl_) { OrderView v(*orderCtrl_, *sampleCtrl_); v.show(); }
        break;
    case 3:
        if (orderCtrl_ && sampleCtrl_) { ApprovalView v(*orderCtrl_, *sampleCtrl_); v.show(); }
        break;
    case 4: { MonitoringView v; v.show(); break; }
    case 5:
        if (productionCtrl_ && sampleCtrl_) {
            ProductionView v(*productionCtrl_, *sampleCtrl_);
            v.show();
        }
        break;
    case 6:
        if (releaseCtrl_ && sampleCtrl_) {
            ReleaseView v(*releaseCtrl_, *sampleCtrl_);
            v.show();
        }
        break;
    default:
        std::cout << "\n  잘못된 선택입니다. 다시 입력해 주세요.\n";
        std::cout << "  Enter를 눌러 계속...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        break;
    }
}
