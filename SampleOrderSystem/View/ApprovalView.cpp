#include "ApprovalView.h"
#include <iostream>
#include <limits>

void ApprovalView::show() {
    std::cout << "\n================================================================\n";
    std::cout << "  [3] 주문 승인/거절\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  구현 예정입니다. (Phase 5)\n\n";
    std::cout << "  Enter를 눌러 메인 메뉴로 돌아갑니다...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
