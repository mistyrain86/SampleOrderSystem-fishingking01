#include "OrderView.h"
#include "Controller/OrderController.h"
#include "Controller/SampleController.h"
#include <iostream>
#include <limits>
#include <algorithm>

OrderView::OrderView(OrderController& orderCtrl, SampleController& sampleCtrl)
    : orderCtrl_(orderCtrl), sampleCtrl_(sampleCtrl) {}

void OrderView::show() {
    std::cout << "\n================================================================\n";
    std::cout << "  [2] 시료 주문\n";
    std::cout << "----------------------------------------------------------------\n";

    auto readLine = [](const std::string& prompt) {
        std::cout << "  " << prompt;
        std::string s;
        std::getline(std::cin, s);
        return s;
    };

    std::string sampleId     = readLine("시료 ID    > ");
    std::string customerName = readLine("고객명      > ");
    std::string qtyStr       = readLine("주문 수량   > ");

    int quantity = 0;
    try {
        quantity = std::stoi(qtyStr);
    } catch (...) {
        std::cout << "  오류: 수량은 숫자로 입력해 주세요.\n";
        std::cout << "  Enter를 눌러 계속...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    // 시료 존재 여부 및 이름 확인
    auto samples = sampleCtrl_.getAllSamples();
    auto it = std::find_if(samples.begin(), samples.end(),
                           [&](const Sample& s) { return s.id == sampleId; });
    if (it == samples.end()) {
        std::cout << "  오류: 등록되지 않은 시료 ID입니다. (" << sampleId << ")\n";
        std::cout << "  Enter를 눌러 계속...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    const Sample& sample = *it;

    // 확인 화면
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  입력 내용 확인\n";
    std::cout << "  시료     " << sample.name << "  (" << sample.id << ")\n";
    std::cout << "  고객     " << customerName << "\n";
    std::cout << "  수량     " << quantity << " ea\n";
    std::cout << "  [Y] 예약 접수    [N] 취소\n";

    std::cout << "  선택 > ";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm != "Y" && confirm != "y") {
        std::cout << "  취소되었습니다.\n";
        std::cout << "  Enter를 눌러 계속...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    // 예약 접수
    auto result = orderCtrl_.reserveOrder(sampleId, customerName, quantity);
    std::cout << "----------------------------------------------------------------\n";
    if (result == ReserveResult::SUCCESS) {
        std::cout << "  예약 접수 완료.\n";
        std::cout << "  현재 상태  RESERVED\n";
        std::cout << "  ※ 재고 확인은 [3] 주문 승인 메뉴에서 진행하세요.\n";
    } else {
        std::cout << "  오류: 예약 접수에 실패했습니다.\n";
    }

    std::cout << "  Enter를 눌러 계속...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
