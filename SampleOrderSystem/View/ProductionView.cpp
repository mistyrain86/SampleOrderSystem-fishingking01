#ifndef _DEBUG
#include "ProductionView.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include <ctime>
#include <algorithm>
#include <cstdio>

ProductionView::ProductionView(ProductionController& productionCtrl,
                               SampleController&     sampleCtrl)
    : productionCtrl_(productionCtrl), sampleCtrl_(sampleCtrl) {}

namespace {

std::string findSampleNameP(const std::vector<Sample>& samples,
                            const std::string& sampleId) {
    for (const auto& s : samples) {
        if (s.id == sampleId) return s.name;
    }
    return sampleId;
}

double findCycleTimeP(const std::vector<Sample>& samples,
                      const std::string& sampleId) {
    for (const auto& s : samples) {
        if (s.id == sampleId) return s.cycleTime;
    }
    return 1.0;
}

int findPureQtyP(const std::vector<Sample>& samples,
                 const std::string& sampleId) {
    for (const auto& s : samples) {
        if (s.id == sampleId) return s.pureQuantity;
    }
    return 0;
}

std::string calcCompletionTime(double accumulatedMinutes) {
    std::time_t now = std::time(nullptr);
    std::tm     tmNow{};
    localtime_s(&tmNow, &now);

    int totalMinutes = tmNow.tm_hour * 60 + tmNow.tm_min
                       + static_cast<int>(accumulatedMinutes);
    int hour = (totalMinutes / 60) % 24;
    int min  =  totalMinutes % 60;

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, min);
    return buf;
}

// "YYYY-MM-DD HH:MM:SS" → time_t 변환
std::time_t parseDateTime(const std::string& dt) {
    std::tm tm{};
    int y, mo, d, h, mi, s;
    if (sscanf_s(dt.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) == 6) {
        tm.tm_year  = y  - 1900;
        tm.tm_mon   = mo - 1;
        tm.tm_mday  = d;
        tm.tm_hour  = h;
        tm.tm_min   = mi;
        tm.tm_sec   = s;
        tm.tm_isdst = -1;
        return mktime(&tm);
    }
    return std::time(nullptr);
}

} // namespace

void ProductionView::show() {
    while (true) {
        auto queue   = productionCtrl_.getProductionQueue();
        auto samples = sampleCtrl_.getAllSamples();

        std::cout << "\n================================================================\n";
        std::cout << "  [5] 생산라인 조회   FIFO 방식\n";
        std::cout << "----------------------------------------------------------------\n\n";

        if (queue.empty()) {
            std::cout << "  현재 생산 대기 중인 주문이 없습니다.\n\n";
            std::cout << "  Enter를 눌러 메인 메뉴로 돌아갑니다...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }

        // ── 현재 처리 중 ──────────────────────────────────────────
        const Order& cur           = queue[0];
        std::string  curName       = findSampleNameP(samples, cur.sampleId);
        double       curCycle      = findCycleTimeP(samples, cur.sampleId);
        int          stockAtOrder  = cur.quantity - cur.productionShortage;
        int          shortage      = cur.productionShortage;
        double       curTime       = ProductionController::calcEstimatedTime(
                                    curCycle, cur.requiredProduction);

        // 진행률 계산
        double elapsedMin = 0.0;
        if (!cur.productionStartedAt.empty()) {
            std::time_t startT = parseDateTime(cur.productionStartedAt);
            std::time_t nowT   = std::time(nullptr);
            elapsedMin = std::max(0.0, std::difftime(nowT, startT) / 60.0);
        }
        int    producedSoFar = (curCycle > 0.0)
            ? std::min(cur.requiredProduction,
                       static_cast<int>(elapsedMin / curCycle))
            : 0;
        int    progressPct   = (curTime > 0.0)
            ? std::min(100, static_cast<int>(elapsedMin / curTime * 100.0))
            : 0;

        // 생산 완료 시 자동 처리 (수동 [C] 불필요)
        if (producedSoFar >= cur.requiredProduction) {
            int excess = cur.requiredProduction - cur.productionShortage;
            productionCtrl_.completeProduction(cur.id);
            std::cout << "\n----------------------------------------------------------------\n";
            std::cout << "  [생산 완료 — 자동 처리]\n";
            std::cout << "  주문번호   " << cur.id << "\n";
            if (excess > 0) {
                std::cout << "  초과 생산  " << excess
                          << " ea → 가용 재고 귀속 (순수 재고 증가)\n";
            }
            std::cout << "  상태       PRODUCING → CONFIRMED\n";
            std::cout << "\n  Enter를 눌러 계속...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        // 진행 바 (20칸)
        constexpr int BAR_WIDTH = 20;
        int filled = BAR_WIDTH * progressPct / 100;
        std::string bar(static_cast<size_t>(filled), '=');
        bar += std::string(static_cast<size_t>(BAR_WIDTH - filled), '-');

        std::cout << "  [현재 처리 중]\n";
        std::cout << "  주문번호  " << std::left << std::setw(14) << cur.id
                  << "시료  " << curName << "\n";
        std::cout << "  주문량   " << cur.quantity
                  << " ea        재고 " << stockAtOrder
                  << " ea → 부족 " << shortage
                  << " ea → 실생산량 " << cur.requiredProduction << " ea\n";
        std::cout << "  총 생산시간  "
                  << std::fixed << std::setprecision(0) << curTime << " min\n";
        std::cout << "\n";
        std::cout << "  진행률     [" << bar << "] " << progressPct << " %\n";
        std::cout << "  현재 생산량  " << producedSoFar << " / "
                  << cur.requiredProduction << " ea"
                  << "  (경과 " << std::fixed << std::setprecision(1)
                  << elapsedMin << " min)\n\n";

        // ── 대기 중인 주문 ────────────────────────────────────────
        std::cout << "  [대기 중인 주문  FIFO 순]\n";

        if (queue.size() <= 1) {
            std::cout << "  현재 대기 중인 주문이 없습니다.\n";
        } else {
            std::cout << "  " << std::left
                      << std::setw(6)  << "순서"
                      << std::setw(12) << "주문번호"
                      << std::setw(22) << "시료"
                      << std::setw(10) << "주문량"
                      << std::setw(10) << "부족분"
                      << std::setw(10) << "실생산량"
                      << "예상완료\n";
            std::cout << "  " << std::string(76, '-') << "\n";

            double accumulated = curTime;
            for (size_t i = 1; i < queue.size(); ++i) {
                const Order& o       = queue[i];
                std::string  name    = findSampleNameP(samples, o.sampleId);
                double       cycle   = findCycleTimeP(samples, o.sampleId);
                int          oShort  = o.productionShortage;
                double       oTime   = ProductionController::calcEstimatedTime(
                                           cycle, o.requiredProduction);
                accumulated += oTime;

                std::cout << "  " << std::left  << std::setw(6) << i
                          << std::setw(12) << o.id
                          << std::setw(22) << name
                          << std::right
                          << std::setw(6) << o.quantity         << " ea  "
                          << std::setw(6) << oShort             << " ea  "
                          << std::setw(6) << o.requiredProduction << " ea  "
                          << calcCompletionTime(accumulated) << "\n";
            }
        }

        std::cout << "\n  * 부족분 = 주문량 - 재고,  "
                     "실생산량 = ceil(부족분 / (수율 × 0.9))\n";
        std::cout << "\n  [C] 생산 완료 처리    [0] 위로\n";
        std::cout << "  선택 > ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "0") return;

        if (input == "C" || input == "c") {
            int excess = cur.requiredProduction - cur.productionShortage;

            std::cout << "----------------------------------------------------------------\n";
            std::cout << "  생산 완료 처리 결과\n";

            if (productionCtrl_.completeProduction(cur.id)) {
                std::cout << "  주문번호   " << cur.id << "\n";
                if (excess > 0) {
                    std::cout << "  초과 생산  " << excess
                              << " ea → 가용 재고에 귀속 (순수 재고 증가)\n";
                }
                std::cout << "  상태       PRODUCING → CONFIRMED\n";
            } else {
                std::cout << "  처리에 실패하였습니다.\n";
            }

            std::cout << "\n  Enter를 눌러 계속...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}
#endif
