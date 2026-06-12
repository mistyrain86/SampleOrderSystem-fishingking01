# PLAN.md — 구현 계획서

> **S-Semi SampleOrderSystem** 의 단계별 구현 계획입니다.  
> 각 Phase는 독립적으로 완료 가능하며, Phase 완료 전 설계문서(`DESIGN_PhaseN.md`)를 별도 작성합니다.  
> 모든 구현은 TDD(Red-Green-Refactor) 원칙을 준수합니다.

---

## 전체 Phase 개요

| Phase | 명칭 | 핵심 산출물 | 의존 Phase |
|-------|------|-------------|-----------|
| 1 | 프로젝트 골격 구성 | VS 프로젝트 + 테스트 프로젝트 빌드 성공 | — |
| 2 | 도메인 모델 정의 | `Sample`, `Order`, `OrderStatus` 헤더 | 1 |
| 3 | Repository 계층 | In-memory CRUD + 단위 테스트 전체 통과 | 2 |
| 4 | 데이터 영속성 | JSON save/load + DummyDataGenerator | 3 |
| 5 | 주문 비즈니스 로직 | `OrderController` + `SampleController` + 테스트 | 3 |
| 6 | 생산·출고 비즈니스 로직 | `ProductionController` + `ReleaseController` + 테스트 | 5 |
| 7 | 대시보드 로직 | `DashboardController` + 테스트 | 5, 6 |
| 8 | View & 메인 메뉴 | 콘솔 UI 전체 연결 + 통합 시나리오 검증 | 4, 5, 6, 7 |

---

## Phase 1 — 프로젝트 골격 구성

> **설계문서:** `DESIGN_Phase1.md` (구현 전 작성)

### 목표
빌드·테스트 파이프라인이 동작하는 빈 뼈대를 완성한다.

### 구현 항목

| # | 항목 | 설명 |
|---|------|------|
| 1-1 | 메인 애플리케이션 프로젝트 | `SampleOrderSystem` — Console Application, C++20, x64 |
| 1-2 | 테스트 프로젝트 추가 | `SampleOrderSystemTest` — gmock 1.11.0 NuGet 참조, 동일 솔루션 |
| 1-3 | 폴더 구조 생성 | `Model / Controller / View / Util` 디렉토리 |
| 1-4 | `json.hpp` 추가 | `Util/json.hpp` — nlohmann/json 단일 헤더 |
| 1-5 | 빈 `main.cpp` | "Hello S-Semi" 출력 후 종료 |
| 1-6 | 더미 테스트 1개 | `TEST(Sanity, AlwaysPass)` — 테스트 프로젝트 동작 확인용 |

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V1-1 | Debug\|x64 빌드 성공 | 경고·오류 0건 |
| V1-2 | 테스트 프로젝트 빌드 성공 | gmock 링크 오류 없음 |
| V1-3 | 더미 테스트 실행 | `[  PASSED  ] 1 test` |
| V1-4 | 폴더 구조 확인 | `Model/Controller/View/Util` 4개 디렉토리 존재 |
| V1-5 | `.vcxproj` 업데이트 | 새 파일이 `<ClCompile>` / `<ClInclude>` 에 등록됨 |

---

## Phase 2 — 도메인 모델 정의

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

## Phase 4 — 데이터 영속성

> **설계문서:** `DESIGN_Phase4.md` (구현 전 작성)

### 목표
Phase 3의 Repository에 JSON 파일 기반 save/load를 추가하고,  
앱 최초 실행 시 더미 데이터를 자동 생성한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 4-1 | `SampleRepository` 확장 | 생성자에 `filePath` 인자 추가, `load()` / `save()` 구현 |
| 4-2 | `OrderRepository` 확장 | 동일 방식으로 `load()` / `save()` 구현 |
| 4-3 | `Util/DummyDataGenerator.h/.cpp` | Sample 5종 + Order 10건 생성 로직 |
| 4-4 | `main.cpp` 초기화 로직 | `data/` 디렉토리 자동 생성, 최초 실행 시 더미 데이터 삽입 |

### JSON 키 규칙

```
samples.json 키: id, name, pureQuantity, reservedQuantity, yield, cycleTime, registeredAt
orders.json  키: id, sampleId, customerName, quantity, status, orderedAt, requiredProduction
```

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V4-1 | `save()` 후 파일 존재 | `data/samples.json`, `data/orders.json` 생성 확인 |
| V4-2 | `load()` 왕복 | save → 새 Repository 생성 → load → 동일 데이터 |
| V4-3 | JSON 키 이름 | PRD 섹션 7의 스키마와 일치 |
| V4-4 | `data/` 자동 생성 | 디렉토리 없어도 최초 실행 시 생성됨 |
| V4-5 | 더미 데이터 생성 | `data/`가 비어있을 때 Sample ≥ 3개, Order ≥ 5개 자동 생성 |
| V4-6 | 재시작 후 데이터 유지 | Repository 재생성 후 load → 이전 데이터 동일 |
| V4-7 | `pureQuantity` / `reservedQuantity` 분리 저장 | JSON에 2개 필드 독립적으로 존재 |

---

## Phase 5 — 주문 비즈니스 로직

> **설계문서:** `DESIGN_Phase5.md` (구현 전 작성)

### 목표
시료 관리 및 주문 생성·승인·거절의 핵심 비즈니스 로직을 구현한다.  
재고 이중 분리 모델의 정확성과 **시간 주입 기반 결정론적 테스트**를 TDD로 완전 검증한다.

### 구현 항목

| # | 파일 | 메서드 |
|---|------|--------|
| 5-1 | `Controller/SampleController.h/.cpp` | `addSample`, `getAllSamples`, `searchByName` |
| 5-2 | `Controller/OrderController.h/.cpp` | 생성자에 `IClock&` 주입, `reserveOrder`, `approveOrder`, `rejectOrder`, `getReservedOrders`, `getAllOrders`, `calcRequiredProduction` |

### 시간 주입 적용 포인트

```cpp
// OrderController 생성자
OrderController(OrderRepository& orderRepo,
                SampleRepository& sampleRepo,
                IClock& clock);           // ← 주입

// reserveOrder 내부
o.orderedAt = clock_.now();               // ← 실제/가짜 시각 모두 동작
```

**테스트에서의 활용:**
```cpp
FakeClock clock("2026-06-12 10:00:00");
OrderController ctrl(orderRepo, sampleRepo, clock);

// orderedAt이 정확히 "2026-06-12 10:00:00"인지 단언 가능
clock.setTime("2026-06-12 11:00:00");    // 시간 이동 시뮬레이션
```

### 검증 항목 (모두 TDD로 작성)

**SampleController:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V5-1 | `addSample` 성공 | `findAll` 결과 수 증가 |
| V5-2 | `searchByName` 키워드 일치 | 결과 반환 |
| V5-3 | `searchByName` 키워드 미일치 | 빈 결과 반환 |

**OrderController:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V5-4 | `calcRequiredProduction` | `ceil(100 / (0.9 × 0.9)) = 124` |
| V5-5 | `calcRequiredProduction` 경계값 | yield=1.0 → `ceil(100/0.9) = 112` |
| V5-6 | `reserveOrder` 성공 | 상태 RESERVED, orderedAt 설정 확인 |
| V5-7 | `reserveOrder` 실패 | 없는 sampleId → `false` 반환, Order 미생성 |
| V5-8 | `approveOrder` 케이스 A | pureQty ≥ qty → CONFIRMED + pureQty 감소 + reservedQty 증가 |
| V5-9 | `approveOrder` 케이스 A — totalQty 불변 | pureQty 감소분 = reservedQty 증가분 |
| V5-10 | `approveOrder` 케이스 B | pureQty < qty → PRODUCING + requiredProduction 설정 |
| V5-11 | `approveOrder` 케이스 B — reservedQty 즉시 선점 | PRODUCING 전환 시 `reservedQty += order.quantity` |
| V5-12 | `approveOrder` 케이스 B — pureQty 불변 | PRODUCING 전환 시 pureQty 변동 없음 |
| V5-13 | `approveOrder` 상태 오류 | RESERVED 아닌 주문 → `FAILED` 반환 |
| V5-14 | `approveOrder` sampleId 무효 | 없는 시료 → `FAILED` 반환 |
| V5-15 | `rejectOrder` 성공 | RESERVED → REJECTED, 재고 변동 없음 |
| V5-16 | `rejectOrder` 상태 오류 | RESERVED 아닌 주문 → `false` 반환 |

---

## Phase 6 — 생산·출고 비즈니스 로직

> **설계문서:** `DESIGN_Phase6.md` (구현 전 작성)

### 목표
생산 완료 시 재고 실시간 귀속 로직과 출고 처리를 구현한다.  
FIFO 순서, 초과 생산분 귀속, **시간 기반 생산 완료 시각 기록**을 TDD로 검증한다.

### 구현 항목

| # | 파일 | 메서드 |
|---|------|--------|
| 6-1 | `Controller/ProductionController.h/.cpp` | 생성자에 `IClock&` 주입, `getProductionQueue`, `completeProduction`, `calcEstimatedTime` |
| 6-2 | `Controller/ReleaseController.h/.cpp` | 생성자에 `IClock&` 주입, `getConfirmedOrders`, `releaseOrder` |

### 시간 주입 적용 포인트

향후 `completedAt`(생산 완료 시각), `releasedAt`(출고 시각) 필드 추가 시 동일 패턴 적용:

```cpp
// 생산 완료 기록 예시 (Order에 completedAt 필드 추가 시)
o.completedAt = clock_.now();

// 출고 기록 예시
o.releasedAt = clock_.now();
```

`FakeClock`으로 생산 완료 시각을 고정하면 다음 시나리오가 테스트 가능해집니다:
- 주문 시각 → 생산 시작 시각 → 생산 완료 시각의 순서 검증
- 예상 생산 시간(`estimatedTime`) 대비 실제 소요 시간 비교 시뮬레이션

### 검증 항목 (모두 TDD로 작성)

**ProductionController:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V6-1 | `getProductionQueue` | PRODUCING 주문만 반환 |
| V6-2 | `getProductionQueue` FIFO | 등록 순서 = 반환 순서 |
| V6-3 | `calcEstimatedTime` | `cycleTime × requiredProduction` 정확성 |
| V6-4 | `completeProduction` 상태 전환 | PRODUCING → CONFIRMED |
| V6-5 | `completeProduction` pureQty 귀속 | `pureQty += (requiredProduction - quantity)` |
| V6-6 | `completeProduction` reservedQty 불변 | 생산 완료 시 reservedQty 변동 없음 (이미 선점) |
| V6-7 | `completeProduction` excessProduction 계산 | `124 - 100 = 24` → pureQty += 24 |
| V6-8 | `completeProduction` 상태 오류 | PRODUCING 아닌 주문 → `false` 반환 |
| V6-9 | 전체 재고 흐름 통합 | 케이스 B 승인 → 생산 완료 → totalQty 변화량 = excessProduction |

**ReleaseController:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V6-10 | `releaseOrder` 성공 | CONFIRMED → RELEASE |
| V6-11 | `releaseOrder` reservedQty 차감 | `reservedQty -= order.quantity` |
| V6-12 | `releaseOrder` pureQty 불변 | 출고 시 pureQty 변동 없음 |
| V6-13 | `releaseOrder` 상태 오류 | CONFIRMED 아닌 주문 → `false` 반환 |

---

## Phase 7 — 대시보드 로직

> **설계문서:** `DESIGN_Phase7.md` (구현 전 작성)

### 목표
전체 주문·재고 현황을 집계하는 대시보드 로직을 구현한다.  
REJECTED 주문 제외, 시료별 수량 집계 정확성을 검증한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 7-1 | `Controller/DashboardController.h/.cpp` | `getDashboardData()` — 상태별 주문 수 + 시료별 수량 집계 |
| 7-2 | `DashboardData` 구조체 | `reservedCount`, `producingCount`, `confirmedCount`, `releaseCount`, `sampleStatuses` |
| 7-3 | `SampleStatus` 구조체 | `sample`, `reservedQty`, `producingQty`, `confirmedQty`, `releasedQty` |

### 검증 항목

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V7-1 | 상태별 카운트 | RESERVED/PRODUCING/CONFIRMED/RELEASE 각 개수 정확 |
| V7-2 | REJECTED 제외 | REJECTED 주문은 집계에 포함되지 않음 |
| V7-3 | 시료별 예약 수량 | 특정 시료의 RESERVED 주문 수량 합계 정확 |
| V7-4 | 시료별 생산 중 수량 | PRODUCING 주문 수량 합계 |
| V7-5 | 시료별 확정 수량 | CONFIRMED 주문 수량 합계 |
| V7-6 | 시료별 출고 수량 | RELEASE 주문 수량 합계 |
| V7-7 | 수량 0 시료 포함 | 주문이 없는 시료도 sampleStatuses에 포함 |
| V7-8 | 다중 시료 혼합 | 시료 A, B 주문이 혼재할 때 각 시료별 집계 오염 없음 |

---

## Phase 8 — View & 메인 메뉴 (UI 통합)

> **설계문서:** `DESIGN_Phase8.md` (구현 전 작성)

### 목표
모든 Controller를 콘솔 UI로 연결하고, 전체 비즈니스 흐름을 E2E로 검증한다.  
앱 시작 시 **스플래시 화면**을 표시하고 메인 메뉴로 진입한다.

### 구현 항목

| # | 파일 | 담당 화면 |
|---|------|----------|
| 8-1 | `View/SplashView.h/.cpp` | 앱 시작 스플래시 화면 (S-Semi 배너 + 시스템 소개) |
| 8-2 | `View/MainView.h/.cpp` | 메인 메뉴 출력 및 입력 라우팅 |
| 8-3 | `View/SampleView.h/.cpp` | 시료 등록 / 목록(pureQty·reservedQty·totalQty) / 검색 |
| 8-4 | `View/OrderView.h/.cpp` | 주문 생성 / RESERVED 목록 / 승인·거절 입력 |
| 8-5 | `View/ProductionView.h/.cpp` | PRODUCING 큐(FIFO) / 예상 생산 시간(min) / 생산 완료 처리 |
| 8-6 | `View/ReleaseView.h/.cpp` | CONFIRMED 목록 / 출고 처리 |
| 8-7 | `View/DashboardView.h/.cpp` | 주문 현황 요약 + 시료별 재고·수량 현황 |
| 8-8 | `main.cpp` 완성 | SplashView → Repository 초기화 → DummyData → MainView 루프 |

### 스플래시 화면 설계 (`SplashView`)

앱 최초 진입 시 아래 구성의 화면을 표시한 뒤 Enter 입력 시 메인 메뉴로 이동합니다.  
PDF 과제 명세의 시스템 소개(배경, 목적)를 반영합니다.

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

> **구현 참고:** 실제 화면 너비·디자인은 `DESIGN_Phase8.md`에서 확정합니다.  
> PDF UI 목업 페이지("화면 디자인과 유사하게 표현")의 레이아웃을 기준으로 작성합니다.

### View 설계 원칙

- View는 Controller를 호출하고 결과를 출력하는 것 외에 비즈니스 로직을 포함하지 않는다.
- 사용자 입력 유효성 검사(숫자 범위, 빈 문자열 등)는 View에서 처리한다.
- 잘못된 입력 시 재입력을 유도하는 루프를 구성한다.
- 콘솔 UTF-8 출력을 위해 `SetConsoleOutputCP(CP_UTF8)` 설정을 `main.cpp`에 포함한다.

### 검증 항목 (E2E 시나리오)

| # | 시나리오 | 검증 포인트 |
|---|---------|------------|
| V8-1 | **Happy Path A** — 재고 충분 | 주문 생성 → 승인(케이스 A) → 대시보드 확인 → 출고 → 재고 감소 확인 |
| V8-2 | **Happy Path B** — 재고 부족 | 주문 생성 → 승인(케이스 B) → reservedQty 즉시 반영 확인 → 생산 완료 → pureQty 초과분 확인 → 출고 |
| V8-3 | **거절 시나리오** | 주문 생성 → 거절 → 재고 변동 없음 → 대시보드 미포함 확인 |
| V8-4 | **복수 주문 FIFO** | 주문 A, B, C 순서로 케이스 B 승인 → 생산 큐 A→B→C 순서 확인 |
| V8-5 | **데이터 영속성** | 데이터 변경 후 앱 재시작 → 동일 상태 복원 확인 |
| V8-6 | **대시보드 정확성** | 모든 상태 주문이 혼재할 때 집계 수치 PRD 섹션 6-5와 일치 |
| V8-7 | **잘못된 입력** | 없는 주문 ID 입력, 음수 수량, 빈 문자열 → 오류 메시지 + 재입력 |
| V8-8 | **재고 흐름 일관성** | 전체 시나리오 후 `Σ pureQty + Σ reservedQty` = 초기 등록량 + Σ 생산량 - Σ 출고량 |

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
  └── phase/1-skeleton
  └── phase/2-domain-model
  └── phase/3-repository
  └── phase/4-persistence
  └── phase/5-order-logic
  └── phase/6-production-release
  └── phase/7-dashboard
  └── phase/8-view-integration
```

각 Phase 브랜치는 해당 Phase의 모든 검증 항목이 통과된 후 `master`에 병합합니다.
