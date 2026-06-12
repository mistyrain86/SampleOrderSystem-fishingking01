#include "ApprovalView.h"
#include "Controller/OrderController.h"
#include "Controller/SampleController.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>
#include <algorithm>
#include <optional>

ApprovalView::ApprovalView(OrderController& orderCtrl, SampleController& sampleCtrl)
    : orderCtrl_(orderCtrl), sampleCtrl_(sampleCtrl) {}

void ApprovalView::show() {
    auto reserved   = orderCtrl_.getReservedOrders();
    auto allSamples = sampleCtrl_.getAllSamples();

    auto findSample = [&](const std::string& id) -> std::optional<Sample> {
        auto it = std::find_if(allSamples.begin(), allSamples.end(),
                               [&](const Sample& s) { return s.id == id; });
        if (it != allSamples.end()) return *it;
        return std::nullopt;
    };

    std::cout << "\n================================================================\n";
    std::cout << "  [3] 주문 승인/거절\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  승인 대기 중인 예약 목록 (RESERVED)\n\n";

    if (reserved.empty()) {
        std::cout << "  현재 승인 대기 중인 주문이 없습니다.\n";
        std::cout << "\n  Enter를 눌러 계속...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    // 목록 출력
    std::cout << "  " << std::left
              << std::setw(6)  << "번호"
              << std::setw(10) << "주문번호"
              << std::setw(20) << "고객"
              << std::setw(22) << "시료"
              << "수량\n";
    std::cout << "  " << std::string(72, '-') << "\n";

    for (int i = 0; i < static_cast<int>(reserved.size()); ++i) {
        const auto& o = reserved[i];
        std::string sampleName = "알 수 없음";
        auto optS = findSample(o.sampleId);
        if (optS) sampleName = optS->name;

        std::cout << "  [" << (i + 1) << "] "
                  << std::left
                  << std::setw(10) << o.id
                  << std::setw(20) << o.customerName
                  << std::setw(22) << sampleName
                  << o.quantity << " ea\n";
    }

    // 번호 선택
    std::cout << "\n  승인할 번호 > ";
    std::string numStr;
    std::getline(std::cin, numStr);
    int sel = 0;
    try { sel = std::stoi(numStr); } catch (...) { sel = 0; }

    if (sel < 1 || sel > static_cast<int>(reserved.size())) {
        std::cout << "  잘못된 번호입니다.\n";
        std::cout << "  Enter를 눌러 계속...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    const Order& selected  = reserved[sel - 1];
    auto         optSample = findSample(selected.sampleId);

    // 재고 상태 표시
    std::cout << "----------------------------------------------------------------\n";
    if (optSample) {
        const Sample& sample = *optSample;
        int total = sample.pureQuantity + sample.reservedQuantity;

        // 재고 현황 한 눈에 보기
        std::cout << "  재고 현황   총 " << total << " ea"
                  << "  [ 가용(즉시출고) " << sample.pureQuantity << " ea"
                  << " + 예약(출고대기) " << sample.reservedQuantity << " ea ]\n";
        std::cout << "\n";

        if (sample.pureQuantity >= selected.quantity) {
            std::cout << "  재고 판정   가용 재고 " << sample.pureQuantity
                      << " ea >= 주문 " << selected.quantity
                      << " ea  →  즉시 출고 가능\n";
        } else {
            int shortage    = selected.quantity - sample.pureQuantity;
            int reqProd     = OrderController::calcRequiredProduction(
                                  selected.quantity, sample.yield);
            int estimateMin = static_cast<int>(std::ceil(sample.cycleTime * reqProd));
            std::cout << "  재고 판정   가용 재고 " << sample.pureQuantity
                      << " ea < 주문 " << selected.quantity
                      << " ea  →  가용 재고 부족, 생산 필요\n";
            std::cout << "  생산 정보   부족분 " << shortage
                      << " ea  →  실생산량 " << reqProd
                      << " ea  (예상 " << estimateMin << " min)\n";
        }
    }

    std::cout << "  [Y] 승인    [N] 거절\n";
    std::cout << "  선택 > ";
    std::string confirm;
    std::getline(std::cin, confirm);

    std::cout << "----------------------------------------------------------------\n";
    if (confirm == "Y" || confirm == "y") {
        auto result = orderCtrl_.approveOrder(selected.id);
        if (result == ApproveResult::SUCCESS_CONFIRMED) {
            std::cout << "  승인 완료.  RESERVED -> CONFIRMED\n";
        } else if (result == ApproveResult::SUCCESS_PRODUCING) {
            std::cout << "  승인 완료.  RESERVED -> PRODUCING\n";
        } else {
            std::cout << "  오류: 승인에 실패했습니다.\n";
        }
    } else {
        bool ok = orderCtrl_.rejectOrder(selected.id);
        if (ok) {
            std::cout << "  거절 처리 완료.  RESERVED -> REJECTED\n";
        } else {
            std::cout << "  오류: 거절 처리에 실패했습니다.\n";
        }
    }

    std::cout << "  Enter를 눌러 계속...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
