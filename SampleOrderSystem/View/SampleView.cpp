#include "SampleView.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>

SampleView::SampleView(SampleController& controller)
    : controller_(controller) {}

int SampleView::readChoice() const {
    std::cout << "  선택 > ";
    std::string line;
    std::getline(std::cin, line);
    try { return std::stoi(line); }
    catch (...) { return -1; }
}

void SampleView::show() {
    while (true) {
        std::cout << "\n================================================================\n";
        std::cout << "  [1] 시료 관리\n";
        std::cout << "----------------------------------------------------------------\n";
        std::cout << "  [1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 위로\n";
        int choice = readChoice();
        switch (choice) {
            case 1: showRegister(); break;
            case 2: showList();     break;
            case 3: showSearch();   break;
            case 0: return;
            default:
                std::cout << "  잘못된 선택입니다.\n";
        }
    }
}

void SampleView::showRegister() {
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  [1] 시료 등록\n";

    auto readLine = [](const std::string& prompt) {
        std::cout << "  " << prompt;
        std::string s;
        std::getline(std::cin, s);
        return s;
    };

    std::string id       = readLine("시료 ID           > ");
    std::string name     = readLine("시료명             > ");
    std::string qtyStr   = readLine("초기 재고 (ea)     > ");
    std::string yieldStr = readLine("수율 (0~1)         > ");
    std::string ctStr    = readLine("사이클타임 (min/ea) > ");

    int    qty   = 0;
    double yield = 0.0;
    double ct    = 0.0;
    try {
        qty   = std::stoi(qtyStr);
        yield = std::stod(yieldStr);
        ct    = std::stod(ctStr);
    } catch (...) {
        std::cout << "  오류: 숫자 형식이 올바르지 않습니다.\n";
        return;
    }

    std::cout << "----------------------------------------------------------------\n";
    if (controller_.addSample(id, name, qty, yield, ct)) {
        std::cout << "  등록 완료: " << id << " / " << name << "\n";
    } else {
        std::cout << "  오류: 이미 등록된 시료 ID입니다. (" << id << ")\n";
    }
    std::cout << "  Enter를 눌러 계속...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void SampleView::showList() const {
    auto samples = controller_.getAllSamples();
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  등록 시료 목록  (총 " << samples.size() << "종)\n\n";

    if (samples.empty()) {
        std::cout << "  등록된 시료가 없습니다.\n";
    } else {
        std::cout << "  " << std::left
                  << std::setw(8)  << "ID"
                  << std::setw(24) << "시료명"
                  << std::setw(14) << "사이클타임"
                  << std::setw(8)  << "수율"
                  << std::setw(12) << "가용재고(*)"
                  << std::setw(12) << "예약재고(**)"
                  << "총재고\n";
        std::cout << "  " << std::string(82, '-') << "\n";

        for (const auto& s : samples) {
            int total = s.pureQuantity + s.reservedQuantity;
            std::cout << "  " << std::left
                      << std::setw(8)  << s.id
                      << std::setw(24) << s.name
                      << std::setw(10) << std::fixed << std::setprecision(2) << s.cycleTime
                      << std::setw(4)  << " min/ea"
                      << std::setw(8)  << std::fixed << std::setprecision(2) << s.yield
                      << std::setw(9)  << s.pureQuantity     << " ea   "
                      << std::setw(9)  << s.reservedQuantity << " ea   "
                      << total << " ea\n";
        }
        std::cout << "\n  (*) 가용재고: 즉시 출고 가능한 재고\n";
        std::cout << "  (**) 예약재고: 주문 승인 완료 후 출고 대기 중인 재고  (총재고 = 가용 + 예약)\n";
    }
    std::cout << "\n  Enter를 눌러 계속...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void SampleView::showSearch() const {
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  [3] 시료 검색\n";
    std::cout << "  검색어 > ";
    std::string keyword;
    std::getline(std::cin, keyword);

    auto results = controller_.searchByName(keyword);
    std::cout << "----------------------------------------------------------------\n";

    if (results.empty()) {
        std::cout << "  검색 결과 없음.\n";
    } else {
        std::cout << "  검색 결과  (" << results.size() << "건)\n\n";
        std::cout << "  " << std::left
                  << std::setw(8)  << "ID"
                  << std::setw(24) << "시료명"
                  << std::setw(14) << "사이클타임"
                  << std::setw(8)  << "수율"
                  << std::setw(12) << "가용재고(*)"
                  << std::setw(12) << "예약재고(**)"
                  << "총재고\n";
        std::cout << "  " << std::string(82, '-') << "\n";

        for (const auto& s : results) {
            int total = s.pureQuantity + s.reservedQuantity;
            std::cout << "  " << std::left
                      << std::setw(8)  << s.id
                      << std::setw(24) << s.name
                      << std::setw(10) << std::fixed << std::setprecision(2) << s.cycleTime
                      << std::setw(4)  << " min/ea"
                      << std::setw(8)  << std::fixed << std::setprecision(2) << s.yield
                      << std::setw(9)  << s.pureQuantity     << " ea   "
                      << std::setw(9)  << s.reservedQuantity << " ea   "
                      << total << " ea\n";
        }
        std::cout << "\n  (*) 가용재고: 즉시 출고 가능한 재고\n";
        std::cout << "  (**) 예약재고: 주문 승인 완료 후 출고 대기 중인 재고  (총재고 = 가용 + 예약)\n";
    }
    std::cout << "\n  Enter를 눌러 계속...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
