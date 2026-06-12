# DESIGN_Phase7.md — 모니터링 기능 완성

> **Phase 7 목표:**  
> `MonitoringController`와 `MonitoringView`를 완성하여  
> **[4] 모니터링**에서 주문량(상태별 건수) · 재고량(시료별 현황)을 조회할 수 있게 한다.  
> `MainView` 시스템 현황에 `MonitoringController`를 연동한다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `Controller/MonitoringController.h/.cpp` — `getOrderSummary`, `getInventorySummary`
- `Model/OrderSummary.h` — 상태별 주문 건수 구조체
- `Model/InventoryStatus.h` — 시료별 재고 현황 구조체
- `View/MonitoringView.h/.cpp` — 스텁 교체, 모니터링 화면 완성
- `View/MainView.h/.cpp` — `MonitoringController` setter 추가
- `Test/MonitoringControllerTest.cpp` — TDD 선행 작성

### 제외 (Out of Scope)
- JSON 파일 저장 (Phase 8)
- 실시간 폴링/자동 갱신 (수동 메뉴 재진입으로 갱신)

---

## 2. 파일 구조

```
SampleOrderSystem/
├── Model/
│   ├── OrderSummary.h      ← NEW
│   └── InventoryStatus.h   ← NEW
├── Controller/
│   ├── MonitoringController.h    ← NEW
│   └── MonitoringController.cpp  ← NEW
├── View/
│   └── MonitoringView.h/.cpp     ← 수정 (스텁 → 완성)
├── View/MainView.h/.cpp          ← 수정 (MonitoringController* setter 추가)
└── Test/
    └── MonitoringControllerTest.cpp  ← NEW (TDD 선행 작성)
```

---

## 3. 클래스 / 구조체 설계

### 3-1. `Model/OrderSummary.h`

```cpp
#pragma once

struct OrderSummary {
    int reserved  = 0;
    int producing = 0;
    int confirmed = 0;
    int released  = 0;
    // REJECTED 는 집계 제외 (PRD §6-5)
};
```

### 3-2. `Model/InventoryStatus.h`

```cpp
#pragma once
#include <string>

enum class StockLevel {
    SUFFICIENT,   // 여유: pureQty >= RESERVED 주문 합계
    LOW,          // 부족: 0 < pureQty < RESERVED 주문 합계
    DEPLETED      // 고갈: pureQty == 0
};

struct InventoryStatus {
    std::string sampleId;
    std::string sampleName;
    int         pureQuantity;
    int         reservedQuantity;
    StockLevel  stockLevel;
    double      remainRate;   // pureQty / (pureQty + reservedQty) × 100, 합계 0 이면 0.0
};
```

### 3-3. `Controller/MonitoringController.h`

```cpp
#pragma once
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Model/OrderSummary.h"
#include "Model/InventoryStatus.h"

class MonitoringController {
public:
    MonitoringController(IOrderRepository&  orderRepo,
                         ISampleRepository& sampleRepo);

    OrderSummary                  getOrderSummary()     const;
    std::vector<InventoryStatus>  getInventorySummary() const;

private:
    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
};
```

> `IClock` 의존성 없음 — 조회 전용이므로 시각 기록이 불필요하다.  
> `MonitoringView` 헤더의 현재 시각은 View 레이어에서 `SystemClock`을 직접 사용한다.

---

## 4. 핵심 비즈니스 로직

### 4-1. `getOrderSummary` (PRD §6-5)

```
1. orders = orderRepo_.findAll()
2. OrderSummary 초기화 (모두 0)
3. orders 순회:
   - RESERVED   → summary.reserved++
   - PRODUCING  → summary.producing++
   - CONFIRMED  → summary.confirmed++
   - RELEASE    → summary.released++
   - REJECTED   → 무시 (집계 제외)
4. return summary
```

### 4-2. `getInventorySummary` (PRD §6-5)

```
1. samples = sampleRepo_.findAll()
2. orders  = orderRepo_.findAll()
3. samples 순회:
   a. reservedOrderQty = 해당 sampleId 의 RESERVED 주문 quantity 합계
   b. StockLevel 판정:
      - pureQty == 0                       → DEPLETED
      - pureQty < reservedOrderQty         → LOW
      - else                               → SUFFICIENT
   c. remainRate:
      - (pureQty + reservedQty) > 0  →  pureQty × 100.0 / (pureQty + reservedQty)
      - else                         →  0.0
   d. InventoryStatus 생성 후 결과 벡터에 추가
4. return 결과 벡터
```

> **재고 상태 판정 기준 (PRD §6-5):**
> - 여유: RESERVED 주문 합계 대비 가용 재고 충분
> - 부족: 가용 재고 > 0 이나 RESERVED 주문 합계보다 부족
> - 고갈: `pureQuantity == 0`
>
> `reservedQuantity`(예약재고) 가 아닌 **RESERVED 상태 주문의 quantity 합계** 를 기준으로 판정한다.  
> `reservedQuantity` 는 이미 승인된 물량이므로 가용재고 충분 여부와 무관하다.

---

## 5. 화면 설계

### 5-1. 모니터링 메인 서브 메뉴

```
================================================================
  [4] 모니터링   2026-06-12 09:32:15
----------------------------------------------------------------
  [1] 주문량 확인    [2] 재고량 확인    [0] 위로
  선택 > _
```

### 5-2. 주문량 확인 화면 (PDF p.19 기반)

```
================================================================
  [4] 모니터링  >  [1] 주문량 확인
----------------------------------------------------------------
  상태별 주문 현황

  RESERVED      3 건
  PRODUCING     3 건   ← 생산라인 대기
  CONFIRMED     8 건
  RELEASE      18 건

  [0] 위로
```

주문 없음:
```
  등록된 주문이 없습니다.
```

### 5-3. 재고량 확인 화면 (PDF p.19 기반)

```
================================================================
  [4] 모니터링  >  [2] 재고량 확인
----------------------------------------------------------------
  시료별 재고 현황   (총 N종)

  시료명                    재고       상태    잔여율
  ──────────────────────────────────────────────────
  실리콘 웨이퍼-8인치        480 ea    여유      80 %
  GaN 에피택셜-4인치         220 ea    여유      44 %
  SiC 파워기판-6인치           30 ea    부족       6 %
  산화막 웨이퍼-SiO2            0 ea    고갈       0 %

  * 잔여율 = 가용재고 / (가용재고 + 예약재고) × 100

  [0] 위로
```

시료 없음:
```
  등록된 시료가 없습니다.
```

### 5-4. MainView 연동

```
  등록 시료   4 종      총 재고     730 ea    ← Phase 4 (기존)
  전체 주문  32 건      생산라인    3 건 대기  ← Phase 5~6 (기존)
```

> Phase 7에서 MonitoringView는 메뉴 [4] 진입 시 독립 서브 메뉴로 동작.  
> MainView 헤더 수치(등록 시료 수, 총 재고, 전체 주문, 생산라인 대기 수)는 Phase 4~6에서 이미 연동되어 있으므로 추가 변경 없음.

---

## 6. 테스트 설계

### 6-1. `Test/MonitoringControllerTest.cpp`

> `MockOrderRepoM`, `MockSampleRepoM` 사용 (anonymous namespace).  
> IClock 불필요 — 테스트 픽스처에 FakeClock 없음.

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T7-1 | `GetOrderSummary_CountByStatus` | RESERVED 2, PRODUCING 1, CONFIRMED 3, RELEASE 4 | 각 카운트 정확 |
| T7-2 | `GetOrderSummary_ExcludesRejected` | REJECTED 2건 포함 | `summary.reserved+producing+confirmed+released` 에 REJECTED 미포함 |
| T7-3 | `GetInventorySummary_StatusSufficient` | pureQty=100, RESERVED 주문 qty 합계=50 | `stockLevel == SUFFICIENT` |
| T7-4 | `GetInventorySummary_StatusLow` | pureQty=30, RESERVED 주문 qty 합계=80 | `stockLevel == LOW` |
| T7-5 | `GetInventorySummary_StatusDepleted` | pureQty=0 | `stockLevel == DEPLETED` |
| T7-6 | `GetInventorySummary_RemainRate` | pureQty=480, reservedQty=120 | `remainRate == 80.0` |
| T7-7 | `GetInventorySummary_ZeroTotal` | pureQty=0, reservedQty=0 | `remainRate == 0.0` (0 나누기 방지) |
| T7-8 | `GetInventorySummary_AllSamplesIncluded` | 시료 3종, 일부 pureQty=0 | 결과 벡터 크기 == 3 |
| T7-9 | `GetInventorySummary_OnlyReservedOrdersCount` | 같은 sampleId 에 RESERVED 1건 + CONFIRMED 1건 | RESERVED qty 만 합산하여 판정 |

---

## 7. `.vcxproj` 업데이트 항목

### `SampleOrderSystem.vcxproj`

**추가 `<ClInclude>`:**
```xml
<ClInclude Include="Model\OrderSummary.h" />
<ClInclude Include="Model\InventoryStatus.h" />
<ClInclude Include="Controller\MonitoringController.h" />
```

**추가 `<ClCompile>` (Controller — 양 Configuration 빌드):**
```xml
<ClCompile Include="Controller\MonitoringController.cpp" />
```

**추가 `<ClCompile>` (Test — Release ExcludedFromBuild):**
```xml
<ClCompile Include="Test\MonitoringControllerTest.cpp">
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
</ClCompile>
```

---

## 8. PRD 요구사항 매핑

| PRD 항목 | Phase 7 반영 여부 | 비고 |
|---------|-----------------|------|
| `[1] 주문량 확인` — 상태별 주문 건수 표시 | ✅ | PRD §6-5 |
| REJECTED 주문 집계 제외 | ✅ | PRD §6-5 |
| `[2] 재고량 확인` — 시료별 재고 현황 | ✅ | PRD §6-5 |
| 재고 상태 여유 판정 | ✅ | PRD §6-5 |
| 재고 상태 부족 판정 | ✅ | PRD §6-5 |
| 재고 상태 고갈 판정 (`pureQty == 0`) | ✅ | PRD §6-5 |
| 잔여율 = `pureQty / (pureQty + reservedQty) × 100` | ✅ | PRD §6-5 |
| 수량 0인 시료도 목록 표시 | ✅ | PRD §6-5 |
| TDD | ✅ | 선행 테스트 작성 (T7-1~T7-9) |

---

## 9. Phase 7 완료 기준

| # | 검증 | 기준 |
|---|------|------|
| V7-1 | Debug 빌드 성공 | 경고·오류 0건 |
| V7-2 | MonitoringController 단위 테스트 | T7-1~T7-9 (9개) 전체 통과 |
| V7-3 | 주문량 확인 (직접 조작) | RESERVED/PRODUCING/CONFIRMED/RELEASE 건수 표시, REJECTED 미포함 |
| V7-4 | 재고량 확인 (직접 조작) | 시료별 재고·상태(여유/부족/고갈)·잔여율 표시 |
| V7-5 | 재고 상태 정합성 | 케이스 B 승인 후 해당 시료 → 부족 또는 고갈 표시 확인 |
| V7-6 | Release 빌드 성공 | 테스트 코드 제외 후 앱 빌드 정상 |
