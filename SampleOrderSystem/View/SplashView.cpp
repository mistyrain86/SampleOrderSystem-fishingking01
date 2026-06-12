#include "SplashView.h"
#include <iostream>
#include <limits>

void SplashView::show() const {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "║               S-Semi 반도체 시료 주문 관리 시스템                ║\n";
    std::cout << "║              Semiconductor Sample Order Management               ║\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "║    시료(Sample) 등록부터 주문 · 생산 · 출고까지                  ║\n";
    std::cout << "║   S-Semi의 전체 시료 공급 흐름을 하나의 시스템으로 관리합니다.   ║\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "║                     Press Enter to Start...                      ║\n";
    std::cout << "║                                                                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    system("cls");
}
