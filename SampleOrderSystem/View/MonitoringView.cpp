#ifndef _DEBUG
#include "MonitoringView.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <ctime>

namespace {

std::string currentTimeStr() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

std::string stockLevelLabel(StockLevel level) {
    switch (level) {
    case StockLevel::SUFFICIENT: return "여유";
    case StockLevel::LOW:        return "부족";
    case StockLevel::DEPLETED:   return "고갈";
    }
    return "";
}

void waitEnter() {
    std::cout << "\n  Enter를 눌러 계속...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

} // namespace

MonitoringView::MonitoringView(MonitoringController& monitoringCtrl)
    : monitoringCtrl_(monitoringCtrl) {}

void MonitoringView::show() {
    while (true) {
        system("cls");
        std::cout << "================================================================\n";
        std::cout << "  [4] 모니터링   " << currentTimeStr() << "\n";
        std::cout << "----------------------------------------------------------------\n";
        std::cout << "  [1] 주문량 확인    [2] 재고량 확인    [0] 위로\n";
        std::cout << "----------------------------------------------------------------\n";
        std::cout << "  선택 > ";

        std::string line;
        std::getline(std::cin, line);
        int choice = -1;
        try { choice = std::stoi(line); } catch (...) {}

        if (choice == 0) break;
        if (choice == 1) { showOrderSummary();     continue; }
        if (choice == 2) { showInventorySummary(); continue; }

        std::cout << "\n  잘못된 선택입니다.\n";
        waitEnter();
    }
}

void MonitoringView::showOrderSummary() const {
    system("cls");
    std::cout << "================================================================\n";
    std::cout << "  [4] 모니터링  >  [1] 주문량 확인\n";
    std::cout << "----------------------------------------------------------------\n";

    auto s = monitoringCtrl_.getOrderSummary();
    int  total = s.reserved + s.producing + s.confirmed + s.released;

    if (total == 0) {
        std::cout << "  등록된 주문이 없습니다.\n";
    } else {
        std::cout << "  상태별 주문 현황\n\n";
        std::cout << "  RESERVED    " << std::setw(4) << s.reserved  << " 건\n";
        std::cout << "  PRODUCING   " << std::setw(4) << s.producing << " 건   ← 생산라인 대기\n";
        std::cout << "  CONFIRMED   " << std::setw(4) << s.confirmed << " 건\n";
        std::cout << "  RELEASE     " << std::setw(4) << s.released  << " 건\n";
    }

    waitEnter();
}

void MonitoringView::showInventorySummary() const {
    system("cls");
    std::cout << "================================================================\n";
    std::cout << "  [4] 모니터링  >  [2] 재고량 확인\n";
    std::cout << "----------------------------------------------------------------\n";

    auto inventory = monitoringCtrl_.getInventorySummary();

    if (inventory.empty()) {
        std::cout << "  등록된 시료가 없습니다.\n";
        waitEnter();
        return;
    }

    std::cout << "  시료별 재고 현황   (총 " << inventory.size() << "종)\n\n";
    std::cout << "  " << std::left
              << std::setw(26) << "시료명"
              << std::setw(10) << "재고"
              << std::setw(8)  << "상태"
              << "잔여율\n";
    std::cout << "  " << std::string(54, '-') << "\n";

    for (const auto& item : inventory) {
        std::string qtyStr = std::to_string(item.pureQuantity) + " ea";
        std::string rateStr = std::to_string(static_cast<int>(item.remainRate + 0.5)) + " %";

        std::cout << "  " << std::left
                  << std::setw(26) << item.sampleName
                  << std::setw(10) << qtyStr
                  << std::setw(8)  << stockLevelLabel(item.stockLevel)
                  << rateStr << "\n";
    }

    std::cout << "\n  * 잔여율 = 가용재고 / (가용재고 + 예약재고) × 100\n";
    waitEnter();
}
#endif
