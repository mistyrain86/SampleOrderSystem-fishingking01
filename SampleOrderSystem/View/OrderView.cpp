#include "OrderView.h"
#include <iostream>
#include <limits>

void OrderView::show() {
    std::cout << "\n================================================================\n";
    std::cout << "  [2] 시료 주문\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  구현 예정입니다. (Phase 5)\n\n";
    std::cout << "  Enter를 눌러 메인 메뉴로 돌아갑니다...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
