# DESIGN_Phase6.md — 생산라인 조회 · 출고 처리

> **Phase 6 목표:**  
> `ProductionController`, `ReleaseController`, `ProductionView`, `ReleaseView`를 완성하여  
> **[5] 생산라인 조회**와 **[6] 출고 처리**를 실제로 조작할 수 있게 한다.  
> `PRODUCING → CONFIRMED → RELEASE` 전체 흐름이 동작한다.  
> `MainView` 시스템 현황의 "생산라인 대기 수"를 실제 데이터로 연동한다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `Controller/ProductionController.h/.cpp`
- `Controller/ReleaseController.h/.cpp`
- `View/ProductionView.h/.cpp` — 스텁 교체, 생산라인 화면 완성
- `View/ReleaseView.h/.cpp` — 스텁 교체, 출고 처리 화면 완성
- `View/MainView.h/.cpp` — `ProductionController` setter + 생산라인 대기 수 연동
- `Test/ProductionControllerTest.cpp` — TDD 선행 작성
- `Test/ReleaseControllerTest.cpp` — TDD 선행 작성

### 제외 (Out of Scope)
- 모니터링 기능 (Phase 7)
- JSON 파일 저장 (Phase 8)
- 실시간 생산 시뮬레이션

---

## 2. 파일 구조

```
SampleOrderSystem/
├── Controller/
│   ├── ProductionController.h    ← NEW
│   ├── ProductionController.cpp  ← NEW
│   ├── ReleaseController.h       ← NEW
│   └── ReleaseController.cpp     ← NEW
├── View/
│   ├── ProductionView.h/.cpp     ← 수정 (스텁 → 완성)
│   └── ReleaseView.h/.cpp        ← 수정 (스텁 → 완성)
├── View/MainView.h/.cpp          ← 수정 (ProductionController* setter 추가)
└── Test/
    ├── ProductionControllerTest.cpp  ← NEW (TDD 선행 작성)
    └── ReleaseControllerTest.cpp     ← NEW (TDD 선행 작성)
```

---

## 3. 클래스 설계

### 3-1. `Controller/ProductionController.h`

```cpp
#pragma once
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

class ProductionController {
public:
    ProductionController(IOrderRepository&  orderRepo,
                         ISampleRepository& sampleRepo,
                         IClock&            clock);

    std::vector<Order> getProductionQueue() const;   // PRODUCING 주문, FIFO 순
    int                getProductionCount() const;   // MainView 현황 표시용
    bool               completeProduction(const std::string& orderId);

    static double calcEstimatedTime(double cycleTime, int requiredProduction);

private:
    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
    IClock&            clock_;
};
```

### 3-2. `Controller/ReleaseController.h`

```cpp
#pragma once
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

class ReleaseController {
public:
    ReleaseController(IOrderRepository&  orderRepo,
                      ISampleRepository& sampleRepo,
                      IClock&            clock);

    std::vector<Order> getConfirmedOrders() const;   // CONFIRMED 주문, FIFO 순
    bool               releaseOrder(const std::string& orderId);

private:
    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
    IClock&            clock_;
};
```

> **PRD §8 메모:** 원래 `ReleaseController(IOrderRepository&, IClock&)` 이었으나,  
> `reservedQuantity` 차감을 위해 `ISampleRepository&` 가 추가로 필요하므로 3-인자 생성자를 사용한다.

---

## 4. 핵심 비즈니스 로직

### 4-1. `completeProduction` (PRD §4-4)

```
1. orderRepo_.findById(orderId)  → nullopt  → return false
2. order.status != PRODUCING     → return false
3. sampleRepo_.findById(order.sampleId) → sample

4. excessProduction = order.requiredProduction - order.quantity
5. sample.pureQuantity += excessProduction   // 초과 생산분 → 순수 재고 귀속
6. order.status = CONFIRMED
   (reservedQuantity 는 PRODUCING 전환 시 이미 선점 → 변동 없음)

7. sampleRepo_.update(sample)
8. orderRepo_.update(order)
9. return true
```

> `excessProduction` 은 항상 ≥ 0.  
> `requiredProduction = ceil(qty / (yield × 0.9)) ≥ qty` 이므로 pureQty 는 항상 증가하거나 유지됩니다.

### 4-2. `releaseOrder` (PRD §4-5)

```
1. orderRepo_.findById(orderId)  → nullopt  → return false
2. order.status != CONFIRMED     → return false
3. sampleRepo_.findById(order.sampleId) → sample

4. sample.reservedQuantity -= order.quantity  // 출고됐으므로 물리 재고에서 제거
5. order.status = RELEASE

6. sampleRepo_.update(sample)
7. orderRepo_.update(order)
8. return true
```

### 4-3. `calcEstimatedTime`

```cpp
// 단위: 분 (min)
static double calcEstimatedTime(double cycleTime, int requiredProduction) {
    return cycleTime * requiredProduction;
}
```

---

## 5. 화면 설계

### 5-1. 생산라인 조회 화면 (PDF p.21 기반)

```
================================================================
  [5] 생산라인 조회   FIFO 방식
----------------------------------------------------------------
  생산 대기 목록   (총 2건)

  순서   주문번호    시료                   주문량   부족분   실생산량   예상시간
  ──────────────────────────────────────────────────────────────────────────
  [1]   ORD0001    SiC 파워기판-6인치       80 ea    50 ea    61 ea    73 min
  [2]   ORD0002    산화막 웨이퍼-SiO2      150 ea   150 ea   190 ea   76 min

  ※ 부족분 = 주문량 - 가용재고  |  실생산량 = ceil(주문량 / (수율 × 0.9))
  ※ 예상시간 = 사이클타임 × 실생산량

  [C] 생산 완료 처리    [0] 위로
  선택 > C

  완료할 순번 > 1
----------------------------------------------------------------
  생산 완료 처리 결과
  주문번호   ORD0001
  초과 생산   11 ea → 가용 재고에 귀속 (순수 재고 증가)
  상태       PRODUCING → CONFIRMED
```

생산 대기 없음:
```
  현재 생산 대기 중인 주문이 없습니다.
```

### 5-2. 출고 처리 화면 (PDF p.23 기반)

```
================================================================
  [6] 출고 처리
----------------------------------------------------------------
  출고 가능 주문   (CONFIRMED)

  번호    주문번호    고객                시료                   수량
  ──────────────────────────────────────────────────────────────────
  [1]    ORD0003    SK하이닉스           실리콘 웨이퍼-8인치     150 ea
  [2]    ORD0004    DB하이텍             포토레지스트-PR7        400 ea

  출고할 번호 > 1
----------------------------------------------------------------
  출고 처리 완료.
  주문번호   ORD0003
  고객       SK하이닉스
  출고 수량  150 ea
  처리 일시  2026-06-12 09:34:02
  상태       CONFIRMED → RELEASE
```

출고 가능 주문 없음:
```
  현재 출고 가능한 주문이 없습니다.
```

### 5-3. MainView 시스템 현황 연동

```
  전체 주문   5 건      생산라인    2 건 대기    ← Phase 6에서 연동
```

---

## 6. 테스트 설계

### 6-1. `Test/ProductionControllerTest.cpp`

> `MockOrderRepoPC`, `MockSampleRepoPC`, `FakeClockP` 사용 (anonymous namespace).

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T6-1 | `CalcEstimatedTime_Formula` | cycleTime=1.2, reqProd=61 | `73.2` |
| T6-2 | `CalcEstimatedTime_Zero` | cycleTime=0.5, reqProd=0 | `0.0` |
| T6-3 | `GetProductionQueue_OnlyProducing` | `findByStatus(PRODUCING)` 위임 | PRODUCING 주문만 반환 |
| T6-4 | `GetProductionCount_Delegates` | `getProductionCount` | `findByStatus(PRODUCING).size()` |
| T6-5 | `CompleteProduction_Success` | PRODUCING 주문 | `true`, CONFIRMED 상태 |
| T6-6 | `CompleteProduction_ExcessToPureQty` | qty=80, reqProd=61, 부족분 기준 아님 — qty=80, reqProd=100 → excess=20 | `pureQty += 20` |
| T6-7 | `CompleteProduction_ReservedUnchanged` | 생산 완료 후 sample | `reservedQty` 불변 |
| T6-8 | `CompleteProduction_NotProducing` | CONFIRMED 주문 완료 시도 | `false`, update 미호출 |
| T6-9 | `CompleteProduction_NotFound` | 없는 orderId | `false` |

### 6-2. `Test/ReleaseControllerTest.cpp`

> `MockOrderRepoRC`, `MockSampleRepoRC`, `FakeClockR` 사용 (anonymous namespace).

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T6-10 | `GetConfirmedOrders_OnlyConfirmed` | `findByStatus(CONFIRMED)` 위임 | CONFIRMED 주문만 반환 |
| T6-11 | `ReleaseOrder_Success` | CONFIRMED 주문 | `true`, RELEASE 상태 |
| T6-12 | `ReleaseOrder_ReservedQtyDecreases` | reservedQty=200, qty=150 | `reservedQty = 50` |
| T6-13 | `ReleaseOrder_PureQtyUnchanged` | 출고 후 sample | `pureQty` 불변 |
| T6-14 | `ReleaseOrder_NotConfirmed` | PRODUCING 주문 출고 시도 | `false`, update 미호출 |
| T6-15 | `ReleaseOrder_NotFound` | 없는 orderId | `false` |

---

## 7. `.vcxproj` 업데이트 항목

### `SampleOrderSystem.vcxproj`

**추가 `<ClInclude>`:**
```xml
<ClInclude Include="Controller\ProductionController.h" />
<ClInclude Include="Controller\ReleaseController.h" />
```

**추가 `<ClCompile>` (Controller — 양 Configuration 빌드):**
```xml
<ClCompile Include="Controller\ProductionController.cpp" />
<ClCompile Include="Controller\ReleaseController.cpp" />
```

**추가 `<ClCompile>` (Test — Release ExcludedFromBuild):**
```xml
<ClCompile Include="Test\ProductionControllerTest.cpp">
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
</ClCompile>
<ClCompile Include="Test\ReleaseControllerTest.cpp">
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
</ClCompile>
```

---

## 8. PRD 요구사항 매핑

| PRD 항목 | Phase 6 반영 여부 | 비고 |
|---------|-----------------|------|
| `completeProduction` PRODUCING → CONFIRMED | ✅ | PRD §4-4 |
| `excessProduction = reqProd - qty` → pureQty 귀속 | ✅ | PRD §4-4 |
| `completeProduction` reservedQty 변동 없음 | ✅ | PRD §4-4 (케이스 B 승인 시 이미 선점) |
| PRODUCING 아닌 주문 완료 시도 → 실패 | ✅ | PRD §3 상태 흐름 |
| `releaseOrder` CONFIRMED → RELEASE | ✅ | PRD §4-5 |
| `releaseOrder` reservedQty 차감 | ✅ | PRD §4-5 |
| CONFIRMED 아닌 주문 출고 시도 → 실패 | ✅ | PRD §3 상태 흐름 |
| `getProductionQueue` FIFO 순서 보장 | ✅ | PRD §4-4, OrderRepository FIFO 위임 |
| `calcEstimatedTime = cycleTime × reqProd` | ✅ | PRD §4-4 |
| MainView 생산라인 대기 수 연동 | ✅ | PRD §6-1 |
| TDD | ✅ | 선행 테스트 작성 (T6-1~T6-15) |

---

## 9. 재고 흐름 전체 요약 (Phase 5~6 완성 시점)

| 이벤트 | pureQty | reservedQty | totalQty | Phase |
|--------|---------|-------------|---------|-------|
| 시료 등록 | 초기값 | 0 | 초기값 | 4 |
| 주문 승인 케이스 A | `-= qty` | `+= qty` | 불변 | 5 |
| 주문 승인 케이스 B | 불변 | `+= qty` | `+= qty` | 5 |
| **생산 완료** | **`+= excessProd`** | 불변 | `+= excessProd` | **6** |
| **출고** | 불변 | **`-= qty`** | **`-= qty`** | **6** |
| 주문 거절 | 불변 | 불변 | 불변 | 5 |

---

## 10. Phase 6 완료 기준

| # | 검증 | 기준 |
|---|------|------|
| V6-1 | Debug 빌드 성공 | 경고·오류 0건 |
| V6-2 | ProductionController 단위 테스트 | T6-1~T6-9 (9개) 전체 통과 |
| V6-3 | ReleaseController 단위 테스트 | T6-10~T6-15 (6개) 전체 통과 |
| V6-4 | 생산 완료 처리 (직접 조작) | CONFIRMED 전환 + 초과분 가용재고 증가 목록 확인 |
| V6-5 | 출고 처리 (직접 조작) | RELEASE 전환 + 예약재고 감소 목록 확인 |
| V6-6 | MainView 생산라인 대기 수 갱신 | 케이스 B 승인 후 메인 현황 "생산라인 N 건 대기" 증가 확인 |
| V6-7 | Release 빌드 성공 | 테스트 코드 제외 후 앱 빌드 정상 |
