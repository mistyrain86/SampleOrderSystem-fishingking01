# DESIGN_Phase5.md — 주문 기능 완성

> **Phase 5 목표:**  
> `OrderController`, `OrderView`(주문 생성), `ApprovalView`(승인/거절)를 완성하여  
> **[2] 시료 주문**과 **[3] 주문 승인/거절**을 실제로 조작할 수 있게 한다.  
> `MainView` 시스템 현황의 "전체 주문 수"를 실제 데이터로 연동한다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `Controller/OrderController.h/.cpp`
- `View/OrderView.h/.cpp` — 스텁 교체, 주문 생성 화면 완성
- `View/ApprovalView.h/.cpp` — 스텁 교체, 승인/거절 화면 완성
- `View/MainView.h/.cpp` — `OrderController` setter 연동
- `Test/OrderControllerTest.cpp` — TDD 선행 작성

### 제외 (Out of Scope)
- 생산라인·출고 기능 (Phase 6)
- 모니터링 기능 (Phase 7)
- JSON 파일 저장 (Phase 8)

---

## 2. 파일 구조

```
SampleOrderSystem/
├── Controller/
│   ├── OrderController.h    ← NEW
│   └── OrderController.cpp  ← NEW
└── View/
    ├── OrderView.h/.cpp     ← 수정 (스텁 → 완성)
    ├── ApprovalView.h/.cpp  ← 수정 (스텁 → 완성)
    ├── MainView.h/.cpp      ← 수정 (OrderController* setter 추가)

    └── Test/
        └── OrderControllerTest.cpp ← NEW (TDD 선행 작성)
```

---

## 3. 열거형 설계

### 3-1. `ReserveResult`

```cpp
enum class ReserveResult {
    SUCCESS,           // 예약 성공
    INVALID_SAMPLE_ID  // 존재하지 않는 시료 ID
};
```

### 3-2. `ApproveResult`

```cpp
enum class ApproveResult {
    SUCCESS_CONFIRMED,  // 케이스 A: 순수 재고 충분 → CONFIRMED
    SUCCESS_PRODUCING,  // 케이스 B: 순수 재고 부족 → PRODUCING
    FAILED              // RESERVED 아닌 주문 / 존재하지 않는 주문
};
```

> 두 열거형 모두 `OrderController.h` 에 함께 정의한다.

---

## 4. 클래스 설계

### 4-1. `Controller/OrderController.h`

```cpp
#pragma once
#include <string>
#include <vector>
#include "Model/IOrderRepository.h"
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

enum class ReserveResult { SUCCESS, INVALID_SAMPLE_ID };
enum class ApproveResult { SUCCESS_CONFIRMED, SUCCESS_PRODUCING, FAILED };

class OrderController {
public:
    OrderController(IOrderRepository&  orderRepo,
                    ISampleRepository& sampleRepo,
                    IClock&            clock);

    ReserveResult reserveOrder(const std::string& sampleId,
                               const std::string& customerName,
                               int                quantity);

    ApproveResult approveOrder(const std::string& orderId);
    bool          rejectOrder(const std::string& orderId);

    std::vector<Order> getReservedOrders() const;
    int                getOrderCount()     const;

    // 수율·안전계수 기반 필요 생산량 계산 (화면 표시용으로도 사용)
    static int calcRequiredProduction(int quantity, double yield);

private:
    std::string generateOrderId() const;

    IOrderRepository&  orderRepo_;
    ISampleRepository& sampleRepo_;
    IClock&            clock_;
};
```

### 4-2. 주요 메서드 흐름

#### `reserveOrder`

```
1. sampleRepo_.findById(sampleId) → nullopt → INVALID_SAMPLE_ID
2. Order 생성:
     id            = generateOrderId()   // "ORD" + zero-padded 순번
     sampleId, customerName, quantity
     status        = RESERVED
     orderedAt     = clock_.now()
     requiredProduction = 0
3. orderRepo_.add(order)
4. return SUCCESS
```

#### `approveOrder`

```
1. orderRepo_.findById(orderId) → nullopt → FAILED
2. order.status != RESERVED → FAILED
3. sampleRepo_.findById(order.sampleId) → 케이스 분기

케이스 A (sample.pureQuantity >= order.quantity):
   sample.pureQuantity     -= order.quantity
   sample.reservedQuantity += order.quantity
   order.status = CONFIRMED
   sampleRepo_.update(sample)
   orderRepo_.update(order)
   return SUCCESS_CONFIRMED

케이스 B (sample.pureQuantity < order.quantity):
   order.requiredProduction  = calcRequiredProduction(order.quantity, sample.yield)
   sample.reservedQuantity  += order.quantity   // 즉시 선점
   order.status = PRODUCING
   sampleRepo_.update(sample)
   orderRepo_.update(order)
   return SUCCESS_PRODUCING
```

#### `calcRequiredProduction`

```cpp
// PRD §4-2: ceil(quantity / (yield × PRODUCTION_SAFETY_FACTOR))
static int calcRequiredProduction(int quantity, double yield) {
    return static_cast<int>(
        std::ceil(quantity / (yield * PRODUCTION_SAFETY_FACTOR))
    );
}
```

> `PRODUCTION_SAFETY_FACTOR = 0.9` (Constants.h)  
> 예: 수량=100, yield=0.9 → ceil(100 / 0.81) = **124**

#### `generateOrderId`

```
"ORD" + zero-padded 4자리 순번 (orderRepo_.count() + 1 기준)
예: ORD0001, ORD0002 ...
```

---

## 5. 화면 설계

### 5-1. 시료 주문 화면 (PDF p.15 기반)

```
================================================================
  [2] 시료 주문
----------------------------------------------------------------
  시료 ID    > S-003
  고객명      > 삼성전자 파운드리
  주문 수량   > 200
----------------------------------------------------------------
  입력 내용 확인
  시료     SiC 파워기판-6인치  (S-003)
  고객     삼성전자 파운드리
  수량     200 ea
  [Y] 예약 접수    [N] 취소
  선택 > Y
----------------------------------------------------------------
  예약 접수 완료.
  주문번호   ORD0001
  현재 상태  RESERVED
  ※ 재고 확인은 [3] 주문 승인 메뉴에서 진행하세요.
```

오류 케이스:
```
  오류: 등록되지 않은 시료 ID입니다. (S-999)
```

### 5-2. 주문 승인/거절 화면 (PDF p.17 기반)

```
================================================================
  [3] 주문 승인/거절
----------------------------------------------------------------
  승인 대기 중인 예약 목록 (RESERVED)

  번호    주문번호    고객            시료                   수량
  [1]    ORD0001    삼성전자 파운드리  SiC 파워기판-6인치      200 ea
  [2]    ORD0002    SK하이닉스        실리콘 웨이퍼-8인치      150 ea

  승인할 번호 > 1
```

케이스 A (재고 충분):
```
  재고 확인: 순수 재고 480 ea ≥ 주문 200 ea
  [Y] 승인    [N] 거절
  선택 > Y
  승인 완료.  RESERVED → CONFIRMED
```

케이스 B (재고 부족):
```
  재고 확인: 순수 재고 30 ea < 주문 200 ea
  부족분 170 ea → 실생산량 250 ea  (예상 300 min)
  [Y] 승인    [N] 거절
  선택 > Y
  승인 완료.  RESERVED → PRODUCING
```

거절:
```
  선택 > N
  거절 처리 완료.  RESERVED → REJECTED
```

예약 없음:
```
  현재 승인 대기 중인 주문이 없습니다.
```

### 5-3. MainView 시스템 현황 연동

```
  전체 주문   3 건      생산라인    0 건 대기    ← Phase 5에서 연동
```

---

## 6. 테스트 설계 (`Test/OrderControllerTest.cpp`)

> `MockOrderRepository` / `MockSampleRepository` / `FakeClock` 사용.

### 6-1. 테스트 헬퍼

```cpp
// Sample 빌더
static Sample makeSample(const std::string& id, int pureQty, double yield = 0.9,
                         double cycleTime = 0.5, int reservedQty = 0);
// Order 빌더
static Order makeOrder(const std::string& id, const std::string& sampleId,
                       int qty, OrderStatus status = OrderStatus::RESERVED);
```

### 6-2. 테스트 케이스

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T5-1 | `CalcRequired_Formula` | qty=100, yield=0.9 | `124` (`ceil(100/0.81)`) |
| T5-2 | `CalcRequired_EdgeCase` | qty=81, yield=0.9 | `100` (`ceil(81/0.81)`) |
| T5-3 | `ReserveOrder_Success` | 유효 sampleId | `SUCCESS`, `orderRepo.add` 1회 |
| T5-4 | `ReserveOrder_SetsOrderedAt` | FakeClock 고정 시각 | `orderedAt == clock.now()` |
| T5-5 | `ReserveOrder_InvalidSampleId` | 없는 sampleId | `INVALID_SAMPLE_ID`, add 미호출 |
| T5-6 | `ApproveOrder_CaseA_Confirmed` | pureQty(500) ≥ qty(200) | `SUCCESS_CONFIRMED`, `CONFIRMED` |
| T5-7 | `ApproveOrder_CaseA_PureQtyDecreases` | pureQty=500, qty=200 | `pureQty = 300` |
| T5-8 | `ApproveOrder_CaseA_ReservedQtyIncreases` | pureQty=500, qty=200 | `reservedQty += 200` |
| T5-9 | `ApproveOrder_CaseB_Producing` | pureQty(30) < qty(200) | `SUCCESS_PRODUCING`, `PRODUCING` |
| T5-10 | `ApproveOrder_CaseB_ReservedQtyImmediate` | pureQty=30, qty=200 | `reservedQty += 200` (즉시 선점) |
| T5-11 | `ApproveOrder_CaseB_PureQtyUnchanged` | pureQty=30, qty=200 | `pureQty == 30` (변동 없음) |
| T5-12 | `ApproveOrder_CaseB_RequiredProduction` | qty=200, yield=0.85 | `requiredProduction = ceil(200/0.765) = 262` |
| T5-13 | `ApproveOrder_NotReserved` | CONFIRMED 주문에 승인 시도 | `FAILED` |
| T5-14 | `ApproveOrder_NotFound` | 없는 orderId | `FAILED` |
| T5-15 | `RejectOrder_Success` | RESERVED 주문 | `true`, `REJECTED` |
| T5-16 | `RejectOrder_NoInventoryChange` | 거절 후 sample | `pureQty·reservedQty 변동 없음` |
| T5-17 | `RejectOrder_NotReserved` | CONFIRMED 주문 거절 시도 | `false` |
| T5-18 | `GetReservedOrders_OnlyReserved` | RESERVED 2개, CONFIRMED 1개 | 크기 `2` |
| T5-19 | `GetOrderCount_Delegates` | `getOrderCount` | `orderRepo.count()` 위임 |

---

## 7. `.vcxproj` 업데이트 항목

### `SampleOrderSystem.vcxproj`

**추가 `<ClInclude>`:**
```xml
<ClInclude Include="Controller\OrderController.h" />
```

**추가 `<ClCompile>`:**
```xml
<ClCompile Include="Controller\OrderController.cpp" />
```

**추가 `<ClCompile>` (Test, Release ExcludedFromBuild):**
```xml
<ClCompile Include="Test\OrderControllerTest.cpp">
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
</ClCompile>
```

### `SampleOrderSystem.vcxproj.filters`

```xml
<ClInclude Include="Controller\OrderController.h">
  <Filter>Controller</Filter>
</ClInclude>
<ClCompile Include="Controller\OrderController.cpp">
  <Filter>Controller</Filter>
</ClCompile>
<ClCompile Include="Test\OrderControllerTest.cpp">
  <Filter>Test</Filter>
</ClCompile>
```

---

## 8. PRD 요구사항 매핑

| PRD 항목 | Phase 5 반영 여부 | 비고 |
|---------|-----------------|------|
| `reserveOrder` — sampleId 검증 | ✅ | PRD §4-1 |
| `reserveOrder` — `orderedAt = IClock.now()` | ✅ | PRD §4-1 |
| `approveOrder` 케이스 A — CONFIRMED + 재고 이동 | ✅ | PRD §4-2 |
| `approveOrder` 케이스 B — PRODUCING + 즉시 선점 | ✅ | PRD §4-2 |
| `calcRequiredProduction` = ceil(qty / (yield × 0.9)) | ✅ | PRD §4-2, Constants.h |
| `rejectOrder` — REJECTED + 재고 불변 | ✅ | PRD §4-3 |
| RESERVED 아닌 주문 승인/거절 시도 → 실패 | ✅ | PRD §3 상태 흐름 |
| `ApproveResult` 3종 열거형 | ✅ | PRD §6-4 |
| MainView 전체 주문 수 연동 | ✅ | PRD §6-1 |
| TDD | ✅ | `OrderControllerTest.cpp` 선행 작성 |

---

## 9. Phase 5 완료 기준

| # | 검증 | 기준 |
|---|------|------|
| V5-1 | Debug 빌드 성공 | 경고·오류 0건 |
| V5-2 | OrderController 단위 테스트 | T5-1 ~ T5-19 (19개) 전체 통과 |
| V5-3 | 주문 생성 (직접 조작) | 입력 → 확인 → RESERVED |
| V5-4 | 잘못된 시료 ID (직접 조작) | 오류 메시지 출력 |
| V5-5 | 주문 승인 케이스 A (직접 조작) | CONFIRMED 전환 + 재고 변화 목록에서 확인 |
| V5-6 | 주문 승인 케이스 B (직접 조작) | PRODUCING + 부족분·실생산량·예상시간 표시 |
| V5-7 | 주문 거절 (직접 조작) | REJECTED 전환 + 재고 불변 |
| V5-8 | MainView 전체 주문 수 갱신 | 주문 생성 후 메인 메뉴 현황 수치 증가 확인 |
| V5-9 | Release 빌드 성공 | 테스트 코드 제외 후 앱 빌드 정상 |
