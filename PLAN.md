# PLAN.md — 구현 계획서

> **S-Semi SampleOrderSystem** 의 단계별 구현 계획입니다.  
> 각 Phase는 독립적으로 완료 가능하며, Phase 완료 전 설계문서(`DESIGN_PhaseN.md`)를 별도 작성합니다.  
> 모든 구현은 TDD(Red-Green-Refactor) 원칙을 준수합니다.

---

## 전체 Phase 개요

> **핵심 원칙: Vertical Slice**  
> Phase 1에서 앱이 실행되고 메뉴를 조작할 수 있습니다.  
> 이후 각 Phase는 Controller + View를 함께 완성하여 Phase가 끝날 때마다 해당 기능을 실제로 사용해볼 수 있습니다.

| Phase | 명칭 | 핵심 산출물 | 사용자 조작 가능 여부 |
|-------|------|-------------|---------------------|
| 1 | 프로젝트 골격 + UI 뼈대 | 앱 실행 → 스플래시 → 메인 메뉴 루프 | ✅ **메뉴 탐색 가능** |
| 2 | 도메인 모델 + IClock | 도메인 구조체·상수·시간 추상화 헤더 | — (내부 구조 확정) |
| 3 | Repository (In-Memory) | CRUD 단위 테스트 전체 통과 | — (데이터 계층) |
| 4 | 시료 관리 기능 완성 | SampleController + SampleView 연결 | ✅ **시료 등록·조회·검색** |
| 5 | 주문 기능 완성 | OrderController + OrderView 연결 | ✅ **주문 생성·승인·거절** |
| 6 | 생산·출고 기능 완성 | ProductionController + ReleaseController + View 연결 | ✅ **전체 주문 흐름** |
| 7 | 대시보드 기능 완성 | DashboardController + DashboardView 연결 | ✅ **전체 현황 조회** |
| 8 | 데이터 영속성 + E2E | JSON save/load + DummyData + 재시작 검증 | ✅ **재시작 후 데이터 유지** |

---

## Phase 1 — 프로젝트 골격 + UI 뼈대

> **설계문서:** `DESIGN_Phase1.md` (구현 전 작성)

### 목표
앱이 실행되면 스플래시 화면이 뜨고, 메뉴를 선택하면 각 화면으로 이동하는 **껍데기 UI**를 완성한다.  
아직 데이터가 없으므로 각 메뉴는 "준비 중" 메시지를 출력한다.  
이 Phase가 끝나면 사용자가 앱을 실행하고 메뉴를 탐색할 수 있다.

### 구현 항목

| # | 항목 | 설명 |
|---|------|------|
| 1-1 | 메인 애플리케이션 프로젝트 | `SampleOrderSystem` — Console Application, C++20, x64 |
| 1-2 | 테스트 프로젝트 추가 | `SampleOrderSystemTest` — gmock 1.11.0 NuGet 참조, 동일 솔루션 |
| 1-3 | 폴더 구조 생성 | `Model / Controller / View / Util` 디렉토리 |
| 1-4 | `Util/json.hpp` 추가 | nlohmann/json 단일 헤더 |
| 1-5 | `View/SplashView.h/.cpp` | S-Semi 배너 + 시스템 소개 + "Press Enter to Start..." |
| 1-6 | `View/MainView.h/.cpp` | 메뉴 출력 + 번호 입력 → 각 View 호출 루프 |
| 1-7 | `View/SampleView.h/.cpp` | 스텁(stub) — "시료 관리 화면 (구현 예정)" 출력 |
| 1-8 | `View/OrderView.h/.cpp` | 스텁 — "주문 생성/승인/거절 화면 (구현 예정)" 출력 |
| 1-9 | `View/ProductionView.h/.cpp` | 스텁 — "생산 처리 화면 (구현 예정)" 출력 |
| 1-10 | `View/ReleaseView.h/.cpp` | 스텁 — "출고 처리 화면 (구현 예정)" 출력 |
| 1-11 | `View/DashboardView.h/.cpp` | 스텁 — "대시보드 화면 (구현 예정)" 출력 |
| 1-12 | `main.cpp` | `SetConsoleOutputCP(CP_UTF8)` → SplashView → MainView 루프 |
| 1-13 | 더미 테스트 1개 | `TEST(Sanity, AlwaysPass)` — 테스트 파이프라인 확인용 |

### 스플래시 화면 (`SplashView`)

```
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║              S-Semi 반도체 시료 주문 관리 시스템               ║
║            Semiconductor Sample Order Management             ║
║                                                              ║
╠══════════════════════════════════════════════════════════════╣
║                                                              ║
║   시료(Sample) 등록부터 주문 · 생산 · 출고까지                  ║
║   S-Semi의 전체 시료 공급 흐름을 하나의 시스템으로 관리합니다.    ║
║                                                              ║
║   주요 기능                                                    ║
║     [1] 시료 관리       [2] 주문 생성/승인/거절                  ║
║     [3] 대시보드        [4] 생산 처리 · 출고 처리                ║
║                                                              ║
╠══════════════════════════════════════════════════════════════╣
║                                                              ║
║                    Press Enter to Start...                   ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
```

### 메인 메뉴 (`MainView`)

```
╔══════════════════════════════════════════╗
║       S-Semi 시료 주문 관리 시스템         ║
╠══════════════════════════════════════════╣
║  [1] 시료 관리                            ║
║  [2] 주문 생성                            ║
║  [3] 주문 승인 / 거절                     ║
║  [4] 대시보드                             ║
║  [5] 생산 처리                            ║
║  [6] 출고 처리                            ║
║  [0] 종료                                ║
╚══════════════════════════════════════════╝
선택 >
```

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V1-1 | Debug\|x64 빌드 성공 | 경고·오류 0건 |
| V1-2 | 테스트 프로젝트 빌드 성공 | gmock 링크 오류 없음 |
| V1-3 | 더미 테스트 실행 | `[  PASSED  ] 1 test` |
| V1-4 | 앱 실행 시 스플래시 화면 출력 | 배너·소개 문구·"Press Enter" 표시 |
| V1-5 | Enter 후 메인 메뉴 진입 | 6개 메뉴 항목 표시 |
| V1-6 | 각 메뉴 선택 시 스텁 화면 출력 | "구현 예정" 메시지 출력 후 메인 메뉴 복귀 |
| V1-7 | `[0]` 선택 시 정상 종료 | 프로세스 종료 확인 |
| V1-8 | 잘못된 번호 입력 | 오류 메시지 + 재입력 유도 |
| V1-9 | 한글 깨짐 없음 | `SetConsoleOutputCP(CP_UTF8)` 적용 확인 |

---

## Phase 2 — 도메인 모델 + IClock

> **설계문서:** `DESIGN_Phase2.md` (구현 전 작성)

### 목표
비즈니스 엔티티와 **시간 추상화 인터페이스**를 헤더 파일로 정의한다. 로직 없이 **데이터 구조만** 확정한다.

### 구현 항목

| # | 파일 | 핵심 내용 |
|---|------|----------|
| 2-1 | `Model/OrderStatus.h` | `enum class OrderStatus { RESERVED, REJECTED, PRODUCING, CONFIRMED, RELEASE }` + `toString()` |
| 2-2 | `Model/Sample.h` | `struct Sample` — id, name, pureQuantity, reservedQuantity, yield, cycleTime(min/ea), registeredAt |
| 2-3 | `Model/Order.h` | `struct Order` — id, sampleId, customerName, quantity, status, orderedAt, requiredProduction |
| 2-4 | `Util/Constants.h` | `PRODUCTION_SAFETY_FACTOR = 0.9` |
| 2-5 | `Util/IClock.h` | 시간 추상화 인터페이스 — `virtual std::string now() const = 0` |
| 2-6 | `Util/SystemClock.h` | `IClock` 구현체 — `std::chrono` 기반 실제 시각 반환 |

### 시간 추상화 설계 배경

`orderedAt` 등 현재 시각을 기록하는 필드가 있어 **테스트에서 시간을 제어할 수 없으면** 아래 문제가 발생합니다:

- 동일한 테스트가 실행 시점에 따라 다른 결과를 낼 수 있음 (비결정적)
- 특정 시간 조건(예: 주문 시간 순서, 생산 시간 계산)을 시뮬레이션할 수 없음
- 생산 처리·출고에서 시간 기반 로직이 추가될 경우 테스트 불가

**해결책 — 클록 의존성 주입 (Clock Injection):**

```cpp
// Util/IClock.h
class IClock {
public:
    virtual ~IClock() = default;
    virtual std::string now() const = 0;  // "YYYY-MM-DD HH:MM:SS"
};

// Util/SystemClock.h  (프로덕션용)
class SystemClock : public IClock {
public:
    std::string now() const override { /* std::chrono 실제 시각 */ }
};

// 테스트용 (테스트 프로젝트 내 정의)
class FakeClock : public IClock {
public:
    explicit FakeClock(std::string fixedTime) : time_(std::move(fixedTime)) {}
    std::string now() const override { return time_; }
    void setTime(const std::string& t) { time_ = t; }
private:
    std::string time_;
};
```

Controller는 생성자에서 `IClock&`를 받아 의존성 주입:
```cpp
OrderController(OrderRepository&, SampleRepository&, IClock&);
```

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V2-1 | 컴파일 성공 | 헤더 include 후 빌드 오류 없음 |
| V2-2 | `OrderStatus` 5종 | RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE 모두 정의됨 |
| V2-3 | `toString()` 테스트 | 각 enum 값이 올바른 문자열 반환 |
| V2-4 | `Sample` 필드 검증 | `pureQuantity` + `reservedQuantity` 2개 필드 분리 확인 |
| V2-5 | `cycleTime` 단위 | 헤더 주석에 `min/ea` 명시 |
| V2-6 | 매직 넘버 없음 | `0.9` 가 코드 내 리터럴로 사용되지 않고 상수 참조 |
| V2-7 | `IClock` 인터페이스 | `FakeClock`으로 고정 시각 주입 후 `now()` 반환값 일치 |
| V2-8 | `SystemClock` 컴파일 | `std::chrono` 기반 빌드 오류 없음 |

---

## Phase 3 — Repository 계층 (In-Memory)

> **설계문서:** `DESIGN_Phase3.md` (구현 전 작성)

### 목표
데이터 저장소의 CRUD 인터페이스를 in-memory로 구현하고 단위 테스트를 완성한다.  
이 Phase에서는 파일 I/O 없이 메모리만 사용한다.

### 구현 항목

| # | 파일 | 메서드 |
|---|------|--------|
| 3-1 | `Model/SampleRepository.h/.cpp` | `add`, `findById`, `findAll`, `findByName`, `update`, `remove`, `count` |
| 3-2 | `Model/OrderRepository.h/.cpp` | `add`, `findById`, `findAll`, `findByStatus`, `findBySampleId`, `update`, `count` |

### 검증 항목 (모두 TDD로 작성)

**SampleRepository:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V3-1 | `add` / `count` | 추가 후 count 증가 확인 |
| V3-2 | `findById` 존재 | 등록된 id → `optional` 값 반환 |
| V3-3 | `findById` 미존재 | 없는 id → `std::nullopt` 반환 |
| V3-4 | `findAll` | 추가된 전체 항목 반환 |
| V3-5 | `findByName` 키워드 | 부분 일치 시료 반환 |
| V3-6 | `findByName` 미존재 | 빈 벡터 반환 |
| V3-7 | `update` 성공 | 수정된 필드 반영 확인 |
| V3-8 | `update` 미존재 | `false` 반환 |
| V3-9 | `remove` 성공 | 제거 후 `findById` → nullopt |
| V3-10 | `remove` 미존재 | `false` 반환 |

**OrderRepository:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V3-11 | `add` / `count` | 추가 후 count 증가 |
| V3-12 | `findByStatus` | 특정 상태 주문만 필터링 반환 |
| V3-13 | `findByStatus` REJECTED | REJECTED 주문도 조회 가능 (대시보드 제외는 Controller 책임) |
| V3-14 | `findBySampleId` | 특정 시료의 주문만 반환 |
| V3-15 | `update` 상태 변경 | 상태 업데이트 후 재조회 반영 확인 |
| V3-16 | FIFO 순서 | `findByStatus` 반환 순서 = 삽입 순서 |

---

## Phase 4 — 시료 관리 기능 완성

> **설계문서:** `DESIGN_Phase4.md` (구현 전 작성)

### 목표
`SampleController`와 `SampleView`를 완성하여 **실제로 시료를 등록·조회·검색**할 수 있게 한다.  
Phase 1에서 스텁이었던 `SampleView`가 이 Phase에서 완전히 동작한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 4-1 | `Controller/SampleController.h/.cpp` | `addSample`, `getAllSamples`, `searchByName` |
| 4-2 | `View/SampleView.h/.cpp` 완성 | 시료 등록 입력 폼 / 목록(pureQty·reservedQty·totalQty) / 검색 결과 |
| 4-3 | `MainView` 연결 | `[1]` 선택 시 `SampleView` 호출 (스텁 → 실제) |

### 검증 항목

**SampleController (TDD):**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V4-1 | `addSample` 성공 | `getAllSamples` 결과 수 증가 |
| V4-2 | `searchByName` 키워드 일치 | 결과 반환 |
| V4-3 | `searchByName` 키워드 미일치 | 빈 결과 반환 |

**SampleView (직접 조작):**

| # | 검증 | 기준 |
|---|------|------|
| V4-4 | 시료 등록 | 입력 후 목록에서 확인 가능 |
| V4-5 | 목록 조회 | pureQty / reservedQty / totalQty 3개 컬럼 표시 |
| V4-6 | 이름 검색 | 키워드 입력 → 매칭 시료만 출력 |
| V4-7 | 메뉴 복귀 | 기능 완료 후 메인 메뉴로 돌아옴 |

---

## Phase 5 — 주문 기능 완성

> **설계문서:** `DESIGN_Phase5.md` (구현 전 작성)

### 목표
`OrderController`와 `OrderView`를 완성하여 **주문 생성·승인·거절**을 실제로 조작할 수 있게 한다.  
재고 이중 분리 모델의 정확성과 **시간 주입 기반 결정론적 테스트**를 TDD로 완전 검증한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 5-1 | `Controller/OrderController.h/.cpp` | 생성자에 `IClock&` 주입, `reserveOrder`, `approveOrder`, `rejectOrder`, `getReservedOrders`, `getAllOrders`, `calcRequiredProduction` |
| 5-2 | `View/OrderView.h/.cpp` 완성 | 주문 생성 폼 / RESERVED 목록 / 승인·거절 선택 UI |
| 5-3 | `MainView` 연결 | `[2]`, `[3]` 선택 시 `OrderView` 호출 (스텁 → 실제) |

### 시간 주입 적용 포인트

```cpp
// OrderController 생성자
OrderController(OrderRepository& orderRepo,
                SampleRepository& sampleRepo,
                IClock& clock);

// reserveOrder 내부
o.orderedAt = clock_.now();
```

**테스트에서의 활용:**
```cpp
FakeClock clock("2026-06-12 10:00:00");
OrderController ctrl(orderRepo, sampleRepo, clock);
// orderedAt이 정확히 "2026-06-12 10:00:00"인지 단언 가능
clock.setTime("2026-06-12 11:00:00");  // 시간 이동 시뮬레이션
```

### 검증 항목

**OrderController (TDD):**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V5-1 | `calcRequiredProduction` | `ceil(100 / (0.9 × 0.9)) = 124` |
| V5-2 | `calcRequiredProduction` 경계값 | yield=1.0 → `ceil(100/0.9) = 112` |
| V5-3 | `reserveOrder` 성공 | 상태 RESERVED, orderedAt = FakeClock 값 |
| V5-4 | `reserveOrder` 실패 | 없는 sampleId → `false`, Order 미생성 |
| V5-5 | `approveOrder` 케이스 A | pureQty ≥ qty → CONFIRMED + pureQty 감소 + reservedQty 증가 |
| V5-6 | `approveOrder` 케이스 A — totalQty 불변 | pureQty 감소분 = reservedQty 증가분 |
| V5-7 | `approveOrder` 케이스 B | pureQty < qty → PRODUCING + requiredProduction 설정 |
| V5-8 | `approveOrder` 케이스 B — reservedQty 즉시 선점 | PRODUCING 전환 시 `reservedQty += order.quantity` |
| V5-9 | `approveOrder` 케이스 B — pureQty 불변 | PRODUCING 전환 시 pureQty 변동 없음 |
| V5-10 | `approveOrder` 상태 오류 | RESERVED 아닌 주문 → `FAILED` 반환 |
| V5-11 | `approveOrder` sampleId 무효 | 없는 시료 → `FAILED` 반환 |
| V5-12 | `rejectOrder` 성공 | RESERVED → REJECTED, 재고 변동 없음 |
| V5-13 | `rejectOrder` 상태 오류 | RESERVED 아닌 주문 → `false` 반환 |

**OrderView (직접 조작):**

| # | 검증 | 기준 |
|---|------|------|
| V5-14 | 주문 생성 | sampleId·고객명·수량 입력 후 RESERVED 목록 확인 |
| V5-15 | 주문 승인 (케이스 A) | 재고 충분 시 CONFIRMED 전환, 시료 pureQty 감소 확인 |
| V5-16 | 주문 승인 (케이스 B) | 재고 부족 시 PRODUCING 전환, reservedQty 즉시 증가 확인 |
| V5-17 | 주문 거절 | REJECTED 전환, 재고 변동 없음 확인 |

---

## Phase 6 — 생산·출고 기능 완성

> **설계문서:** `DESIGN_Phase6.md` (구현 전 작성)

### 목표
`ProductionController`, `ReleaseController`와 각 View를 완성하여 **생산 완료 처리 및 출고**를 실제로 조작할 수 있게 한다.  
이 Phase가 끝나면 RESERVED → PRODUCING → CONFIRMED → RELEASE 전체 흐름을 직접 조작할 수 있다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 6-1 | `Controller/ProductionController.h/.cpp` | 생성자에 `IClock&` 주입, `getProductionQueue`, `completeProduction`, `calcEstimatedTime` |
| 6-2 | `Controller/ReleaseController.h/.cpp` | 생성자에 `IClock&` 주입, `getConfirmedOrders`, `releaseOrder` |
| 6-3 | `View/ProductionView.h/.cpp` 완성 | PRODUCING 큐(FIFO) / 예상 생산 시간(분) / 생산 완료 선택 UI |
| 6-4 | `View/ReleaseView.h/.cpp` 완성 | CONFIRMED 목록 / 출고 처리 선택 UI |
| 6-5 | `MainView` 연결 | `[5]`, `[6]` 선택 시 각 View 호출 (스텁 → 실제) |

### 시간 주입 적용 포인트

향후 `completedAt`, `releasedAt` 필드 추가 시 동일 패턴 적용:
```cpp
o.completedAt = clock_.now();
o.releasedAt  = clock_.now();
```

### 검증 항목

**ProductionController (TDD):**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V6-1 | `getProductionQueue` | PRODUCING 주문만 반환 |
| V6-2 | `getProductionQueue` FIFO | 등록 순서 = 반환 순서 |
| V6-3 | `calcEstimatedTime` | `cycleTime × requiredProduction` 정확성 |
| V6-4 | `completeProduction` 상태 전환 | PRODUCING → CONFIRMED |
| V6-5 | `completeProduction` pureQty 귀속 | `pureQty += (requiredProduction - quantity)` |
| V6-6 | `completeProduction` reservedQty 불변 | 생산 완료 시 reservedQty 변동 없음 |
| V6-7 | `completeProduction` excessProduction | `124 - 100 = 24` → pureQty += 24 |
| V6-8 | `completeProduction` 상태 오류 | PRODUCING 아닌 주문 → `false` 반환 |
| V6-9 | 전체 재고 흐름 통합 | 케이스 B 승인 → 생산 완료 → totalQty 변화량 = excessProduction |

**ReleaseController (TDD):**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V6-10 | `releaseOrder` 성공 | CONFIRMED → RELEASE |
| V6-11 | `releaseOrder` reservedQty 차감 | `reservedQty -= order.quantity` |
| V6-12 | `releaseOrder` pureQty 불변 | 출고 시 pureQty 변동 없음 |
| V6-13 | `releaseOrder` 상태 오류 | CONFIRMED 아닌 주문 → `false` 반환 |

**View (직접 조작):**

| # | 검증 | 기준 |
|---|------|------|
| V6-14 | 생산 큐 FIFO 확인 | 등록 순서대로 표시 |
| V6-15 | 예상 생산 시간 표시 | `cycleTime × requiredProduction` 값 (분 단위) |
| V6-16 | 생산 완료 처리 | CONFIRMED 전환 + pureQty 초과분 증가 화면 확인 |
| V6-17 | 출고 처리 | RELEASE 전환 + reservedQty 감소 화면 확인 |

---

## Phase 7 — 대시보드 기능 완성

> **설계문서:** `DESIGN_Phase7.md` (구현 전 작성)

### 목표
`DashboardController`와 `DashboardView`를 완성하여 전체 주문·재고 현황을 한눈에 볼 수 있게 한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 7-1 | `Controller/DashboardController.h/.cpp` | `getDashboardData()` — 상태별 주문 수 + 시료별 수량 집계 |
| 7-2 | `DashboardData` 구조체 | `reservedCount`, `producingCount`, `confirmedCount`, `releaseCount`, `sampleStatuses` |
| 7-3 | `SampleStatus` 구조체 | `sample`, `reservedQty`, `producingQty`, `confirmedQty`, `releasedQty` |
| 7-4 | `View/DashboardView.h/.cpp` 완성 | 주문 현황 요약 + 시료별 재고(pureQty·reservedQty·totalQty)·수량 현황 표 |
| 7-5 | `MainView` 연결 | `[4]` 선택 시 `DashboardView` 호출 (스텁 → 실제) |

### 검증 항목

**DashboardController (TDD):**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V7-1 | 상태별 카운트 | RESERVED/PRODUCING/CONFIRMED/RELEASE 각 개수 정확 |
| V7-2 | REJECTED 제외 | REJECTED 주문은 집계에 포함되지 않음 |
| V7-3 | 시료별 예약 수량 | RESERVED 주문 수량 합계 |
| V7-4 | 시료별 생산 중 수량 | PRODUCING 주문 수량 합계 |
| V7-5 | 시료별 확정 수량 | CONFIRMED 주문 수량 합계 |
| V7-6 | 시료별 출고 수량 | RELEASE 주문 수량 합계 |
| V7-7 | 수량 0 시료 포함 | 주문이 없는 시료도 sampleStatuses에 포함 |
| V7-8 | 다중 시료 혼합 | 시료 A, B 주문 혼재 시 각 시료별 집계 오염 없음 |

**DashboardView (직접 조작):**

| # | 검증 | 기준 |
|---|------|------|
| V7-9 | 주문 현황 요약 | 4개 상태별 카운트 표시 (REJECTED 없음) |
| V7-10 | 시료별 재고 현황 | pureQty / reservedQty / totalQty 표시 |
| V7-11 | 시료별 주문 수량 | 상태별 수량 합계 표시, 0도 표시 |

---

## Phase 8 — 데이터 영속성 + E2E 검증

> **설계문서:** `DESIGN_Phase8.md` (구현 전 작성)

### 목표
JSON 파일 기반 데이터 영속성을 추가하고, 앱 재시작 후에도 데이터가 유지됨을 검증한다.  
전체 E2E 시나리오로 시스템 완결성을 확인한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 8-1 | `SampleRepository` 확장 | 생성자에 `filePath` 인자, `load()` / `save()` 구현 |
| 8-2 | `OrderRepository` 확장 | 동일 방식으로 `load()` / `save()` 구현 |
| 8-3 | `Util/DummyDataGenerator.h/.cpp` | Sample 5종 + Order 10건 생성 로직 |
| 8-4 | `main.cpp` 초기화 완성 | `data/` 자동 생성 + 최초 실행 시 DummyData 삽입 + Controller에 `SystemClock` 주입 |

### JSON 키 규칙

```
samples.json 키: id, name, pureQuantity, reservedQuantity, yield, cycleTime, registeredAt
orders.json  키: id, sampleId, customerName, quantity, status, orderedAt, requiredProduction
```

### 검증 항목

**데이터 영속성 (TDD):**

| # | 검증 | 기준 |
|---|------|------|
| V8-1 | `save()` 후 파일 존재 | `data/samples.json`, `data/orders.json` 생성 |
| V8-2 | `load()` 왕복 | save → 새 Repository → load → 동일 데이터 |
| V8-3 | JSON 키 이름 | PRD 섹션 7 스키마와 일치 |
| V8-4 | `data/` 자동 생성 | 디렉토리 없어도 실행 시 생성 |
| V8-5 | 더미 데이터 생성 | `data/` 비어있을 때 Sample ≥ 3개, Order ≥ 5개 |
| V8-6 | 재시작 후 데이터 유지 | Repository 재생성 → load → 이전 상태 동일 |
| V8-7 | `pureQuantity` / `reservedQuantity` 분리 저장 | JSON에 2개 필드 독립 존재 |

**E2E 시나리오 (직접 조작):**

| # | 시나리오 | 검증 포인트 |
|---|---------|------------|
| V8-8 | **Happy Path A** — 재고 충분 | 주문 생성 → 승인(케이스 A) → 대시보드 → 출고 → 재고 감소 |
| V8-9 | **Happy Path B** — 재고 부족 | 주문 생성 → 승인(케이스 B) → reservedQty 선점 → 생산 완료 → pureQty 초과분 → 출고 |
| V8-10 | **거절 시나리오** | 주문 생성 → 거절 → 재고 불변 → 대시보드 미집계 |
| V8-11 | **복수 주문 FIFO** | A, B, C 순서 케이스 B 승인 → 생산 큐 A→B→C 순서 |
| V8-12 | **재시작 후 복원** | 데이터 변경 → 앱 종료 → 재시작 → 동일 상태 |
| V8-13 | **재고 흐름 일관성** | `Σ pureQty + Σ reservedQty` = 초기 등록량 + Σ 생산량 - Σ 출고량 |

---

## 설계문서 작성 가이드

각 Phase 착수 전에 `DESIGN_PhaseN.md`를 작성합니다. 포함 내용:

```
1. 목표 및 범위
2. 클래스/구조체 설계 (필드, 메서드 시그니처)
3. 시퀀스 다이어그램 또는 흐름 설명
4. 테스트 케이스 목록 (입력 → 예상 출력)
5. PRD 요구사항 매핑 (어떤 PRD 항목을 충족하는지)
```

---

## 브랜치 전략

```
master
  └── phase/1-skeleton-ui
  └── phase/2-domain-model
  └── phase/3-repository
  └── phase/4-sample-feature
  └── phase/5-order-feature
  └── phase/6-production-release-feature
  └── phase/7-dashboard-feature
  └── phase/8-persistence-e2e
```

각 Phase 브랜치는 해당 Phase의 모든 검증 항목이 통과된 후 `master`에 병합합니다.
