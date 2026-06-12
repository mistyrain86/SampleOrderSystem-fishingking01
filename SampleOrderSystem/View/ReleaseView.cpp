#ifndef _DEBUG
#include "ReleaseView.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <string>

ReleaseView::ReleaseView(ReleaseController& releaseCtrl,
                         SampleController&  sampleCtrl)
    : releaseCtrl_(releaseCtrl), sampleCtrl_(sampleCtrl) {}

namespace {
    std::string findSampleNameR(const std::vector<Sample>& samples,
                                const std::string& sampleId) {
        for (const auto& s : samples) {
            if (s.id == sampleId) return s.name;
        }
        return sampleId;
    }
}

void ReleaseView::show() {
    while (true) {
        auto orders  = releaseCtrl_.getConfirmedOrders();
        auto samples = sampleCtrl_.getAllSamples();

        std::cout << "\n================================================================\n";
        std::cout << "  [6] 출고 처리\n";
        std::cout << "----------------------------------------------------------------\n";
        std::cout << "  출고 가능 주문   (CONFIRMED)\n\n";

        if (orders.empty()) {
            std::cout << "  현재 출고 가능한 주문이 없습니다.\n\n";
            std::cout << "  Enter를 눌러 메인 메뉴로 돌아갑니다...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }

        std::cout << "  " << std::left
                  << std::setw(6)  << "번호"
                  << std::setw(12) << "주문번호"
                  << std::setw(18) << "고객"
                  << std::setw(24) << "시료"
                  << "수량\n";
        std::cout << "  " << std::string(72, '-') << "\n";

        for (size_t i = 0; i < orders.size(); ++i) {
            const auto& o    = orders[i];
            std::string name = findSampleNameR(samples, o.sampleId);
            std::cout << "  [" << (i + 1) << "] "
                      << std::left  << std::setw(10) << o.id
                      << std::setw(18) << o.customerName
                      << std::setw(24) << name
                      << std::right << std::setw(4) << o.quantity << " ea\n";
        }

        std::cout << "\n  출고할 번호 ([0] 취소) > ";
        int idx;
        if (!(std::cin >> idx)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  잘못된 입력입니다.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (idx == 0) return;

        if (idx < 1 || idx > static_cast<int>(orders.size())) {
            std::cout << "  유효하지 않은 번호입니다.\n";
            continue;
        }

        const Order& target = orders[static_cast<size_t>(idx) - 1];

        std::cout << "----------------------------------------------------------------\n";
        std::cout << "  출고 처리 결과\n";

        if (releaseCtrl_.releaseOrder(target.id)) {
            std::cout << "  주문번호   " << target.id << "\n";
            std::cout << "  고객       " << target.customerName << "\n";
            std::cout << "  출고 수량  " << target.quantity << " ea\n";
            std::cout << "  상태       CONFIRMED → RELEASE\n";
        } else {
            std::cout << "  처리에 실패하였습니다.\n";
        }

        std::cout << "\n  Enter를 눌러 계속...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}
#endif
