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
| 1 | 프로젝트 골격 + UI 뼈대 | 앱 실행 → 스플래시 → 시스템현황 + 메인 메뉴 루프 | ✅ **메뉴 탐색 가능** |
| 2 | 도메인 모델 + IClock | 도메인 구조체·상수·시간 추상화 헤더 | — (내부 구조 확정) |
| 3 | Repository (In-Memory) | CRUD 단위 테스트 전체 통과 | — (데이터 계층) |
| 4 | 시료 관리 기능 완성 | SampleController + SampleView 연결 | ✅ **[1] 시료 등록·목록·검색** |
| 5 | 주문 기능 완성 | OrderController + OrderView + ApprovalView 연결 | ✅ **[2] 시료 주문 / [3] 주문 승인·거절** |
| 6 | 생산라인·출고 기능 완성 | ProductionController + ReleaseController + View 연결 | ✅ **[5] 생산라인 조회 / [6] 출고 처리** |
| 7 | 모니터링 기능 완성 | MonitoringController + MonitoringView 연결 | ✅ **[4] 모니터링 (주문량·재고량)** |
| 8 | 데이터 영속성 + E2E | JSON save/load + DummyData + 재시작 검증 | ✅ **재시작 후 데이터 유지** |

---

## Phase 1 — 프로젝트 골격 + UI 뼈대

> **설계문서:** `DESIGN_Phase1.md` (구현 전 작성)

### 목표
앱이 실행되면 스플래시 화면이 뜨고, **시스템 현황 요약**과 함께 메뉴를 선택하면 각 View로 이동하는 **껍데기 UI**를 완성한다.  
각 메뉴는 "구현 예정" 스텁 메시지를 출력한다.  
이 Phase가 끝나면 사용자가 앱을 실행하고 메뉴를 탐색할 수 있다.

### 구현 항목

| # | 항목 | 설명 |
|---|------|------|
| 1-1 | 메인 애플리케이션 프로젝트 | `SampleOrderSystem` — Console Application, C++20, x64 |
| 1-2 | 테스트 프로젝트 추가 | `SampleOrderSystemTest` — gmock 1.11.0 NuGet 참조, 동일 솔루션 |
| 1-3 | 폴더 구조 생성 | `Model / Controller / View / Util` 디렉토리 |
| 1-4 | `Util/json.hpp` 추가 | nlohmann/json 단일 헤더 |
| 1-5 | `View/SplashView.h/.cpp` | S-Semi 배너 + "Press Enter to Start..." |
| 1-6 | `View/MainView.h/.cpp` | 시스템 현황 요약 + 메뉴 출력 + 번호 입력 루프 |
| 1-7 | `View/SampleView.h/.cpp` | 스텁 — "[1] 시료 관리 (Phase 4 구현 예정)" |
| 1-8 | `View/OrderView.h/.cpp` | 스텁 — "[2] 시료 주문 (Phase 5 구현 예정)" |
| 1-9 | `View/ApprovalView.h/.cpp` | 스텁 — "[3] 주문 승인/거절 (Phase 5 구현 예정)" |
| 1-10 | `View/MonitoringView.h/.cpp` | 스텁 — "[4] 모니터링 (Phase 7 구현 예정)" |
| 1-11 | `View/ProductionView.h/.cpp` | 스텁 — "[5] 생산라인 조회 (Phase 6 구현 예정)" |
| 1-12 | `View/ReleaseView.h/.cpp` | 스텁 — "[6] 출고 처리 (Phase 6 구현 예정)" |
| 1-13 | `main.cpp` | `SetConsoleOutputCP(CP_UTF8)` → SplashView → MainView 루프 |
| 1-14 | 더미 테스트 1개 | `TEST(Sanity, AlwaysPass)` — 테스트 파이프라인 확인용 |

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
╠══════════════════════════════════════════════════════════════╣
║                                                              ║
║                    Press Enter to Start...                   ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
```

### 메인 메뉴 (`MainView`) — PDF p.11 예시 기반

```
================================================================
  S-Semi 반도체 시료 생산주문관리 시스템
================================================================
  시스템 현황   2026-06-12 09:32:15

  등록 시료   0 종      총 재고     0 ea
  전체 주문   0 건      생산라인    0 건 대기
----------------------------------------------------------------
  [1] 시료 관리                   [2] 시료 주문
  [3] 주문 승인/거절               [4] 모니터링
  [5] 생산라인 조회                [6] 출고 처리
  [0] 종료
----------------------------------------------------------------
  선택 > _
```

> Phase 1에서 시스템 현황 수치는 모두 0으로 하드코딩. Phase 4~8에서 실제 데이터 연동.

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V1-1 | Debug\|x64 빌드 성공 | 경고·오류 0건 |
| V1-2 | 테스트 프로젝트 빌드 성공 | gmock 링크 오류 없음 |
| V1-3 | 더미 테스트 실행 | `[  PASSED  ] 1 test` |
| V1-4 | 앱 실행 시 스플래시 화면 출력 | 배너·"Press Enter" 표시 |
| V1-5 | Enter 후 메인 메뉴 진입 | 시스템 현황 + 6개 메뉴 항목 표시 |
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
| 2-5 | `Util/IClock.h` | `virtual std::string now() const = 0` |
| 2-6 | `Util/SystemClock.h` | `IClock` 구현체 — `std::chrono` 기반 실제 시각 반환 |

### 시간 추상화 설계 배경

`orderedAt` 등 현재 시각을 기록하는 필드가 있어 테스트에서 시간을 제어할 수 없으면 비결정적 테스트가 된다.

**해결책 — 클록 의존성 주입:**
```cpp
class IClock {
public:
    virtual ~IClock() = default;
    virtual std::string now() const = 0;  // "YYYY-MM-DD HH:MM:SS"
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

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V2-1 | 컴파일 성공 | 헤더 include 후 빌드 오류 없음 |
| V2-2 | `OrderStatus` 5종 | RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE |
| V2-3 | `toString()` 테스트 | 각 enum 값이 올바른 문자열 반환 |
| V2-4 | `Sample` 필드 검증 | `pureQuantity` + `reservedQuantity` 2개 필드 분리 확인 |
| V2-5 | `cycleTime` 단위 | 헤더 주석에 `min/ea` 명시 |
| V2-6 | 매직 넘버 없음 | `0.9`가 리터럴로 사용되지 않고 상수 참조 |
| V2-7 | `FakeClock` 주입 | 고정 시각 주입 후 `now()` 반환값 일치 |

---

## Phase 3 — Repository 계층 (In-Memory)

> **설계문서:** `DESIGN_Phase3.md` (구현 전 작성)

### 목표
데이터 저장소의 CRUD 인터페이스를 in-memory로 구현하고 단위 테스트를 완성한다.

### 구현 항목

| # | 파일 | 메서드 |
|---|------|--------|
| 3-1 | `Model/SampleRepository.h/.cpp` | `add`, `findById`, `findAll`, `findByName`, `update`, `remove`, `count` |
| 3-2 | `Model/OrderRepository.h/.cpp` | `add`, `findById`, `findAll`, `findByStatus`, `findBySampleId`, `update`, `count` |

### 검증 항목 (모두 TDD로 작성)

**SampleRepository:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V3-1 | `add` / `count` | 추가 후 count 증가 |
| V3-2 | `findById` 존재 | optional 값 반환 |
| V3-3 | `findById` 미존재 | `std::nullopt` 반환 |
| V3-4 | `findAll` | 전체 항목 반환 |
| V3-5 | `findByName` 키워드 | 부분 일치 시료 반환 |
| V3-6 | `findByName` 미존재 | 빈 벡터 반환 |
| V3-7 | `update` 성공 | 수정된 필드 반영 |
| V3-8 | `update` 미존재 | `false` 반환 |
| V3-9 | `remove` 성공 | 제거 후 `findById` → nullopt |
| V3-10 | `remove` 미존재 | `false` 반환 |

**OrderRepository:**

| # | 테스트 케이스 | 검증 내용 |
|---|-------------|----------|
| V3-11 | `add` / `count` | 추가 후 count 증가 |
| V3-12 | `findByStatus` | 특정 상태 주문만 필터링 반환 |
| V3-13 | `findByStatus` REJECTED | REJECTED 주문 조회 가능 |
| V3-14 | `findBySampleId` | 특정 시료의 주문만 반환 |
| V3-15 | `update` 상태 변경 | 상태 업데이트 후 재조회 반영 |
| V3-16 | FIFO 순서 | 반환 순서 = 삽입 순서 |

---

## Phase 4 — 시료 관리 기능 완성

> **설계문서:** `DESIGN_Phase4.md` (구현 전 작성)

### 목표
`SampleController`와 `SampleView`를 완성하여 **[1] 시료 관리** 메뉴에서 등록·목록·검색을 실제로 조작할 수 있게 한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 4-1 | `Controller/SampleController.h/.cpp` | `addSample`, `getAllSamples`, `searchByName`, `getSummary` |
| 4-2 | `View/SampleView.h/.cpp` 완성 | 서브 메뉴 `[1] 시료 등록  [2] 시료 목록  [3] 시료 검색  [0] 위로` |
| 4-3 | `MainView` 시스템 현황 연동 | 등록 시료 수, 총 재고 실제 데이터 표시 |

### 시료 목록 화면 (PDF p.13 기반)

```
================================================================
  [1] 시료 관리
----------------------------------------------------------------
  [1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 위로
  선택 > 2
----------------------------------------------------------------
  등록 시료 목록  (총 N종)

  ID        시료명                평균 생산시간    수율     현재 재고
  S-001     실리콘 웨이퍼-8인치    0.5 min/ea     0.92     480 ea
  S-002     GaN 에피택셜-4인치     0.3 min/ea     0.78     220 ea
  ...
```

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V4-1 | `addSample` 성공 (TDD) | getAllSamples 결과 수 증가 |
| V4-2 | `searchByName` 일치 (TDD) | 결과 반환 |
| V4-3 | `searchByName` 미일치 (TDD) | 빈 결과 반환 |
| V4-4 | 시료 등록 (직접 조작) | 입력 후 목록에서 확인 가능 |
| V4-5 | 목록 조회 (직접 조작) | ID·시료명·평균생산시간·수율·현재재고 표시 |
| V4-6 | 이름 검색 (직접 조작) | 키워드 → 매칭 시료만 출력 |
| V4-7 | `[0]` 메인 메뉴 복귀 | |
| V4-8 | 메인 메뉴 시스템 현황 업데이트 | 시료 등록 후 등록 시료 수·총 재고 증가 확인 |

---

## Phase 5 — 주문 기능 완성

> **설계문서:** `DESIGN_Phase5.md` (구현 전 작성)

### 목표
`OrderController`와 `OrderView`(주문 생성), `ApprovalView`(승인/거절)를 완성하여  
**[2] 시료 주문**과 **[3] 주문 승인/거절**을 실제로 조작할 수 있게 한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 5-1 | `Controller/OrderController.h/.cpp` | 생성자에 `IClock&` 주입, `reserveOrder`, `approveOrder`, `rejectOrder`, `getReservedOrders`, `calcRequiredProduction` |
| 5-2 | `View/OrderView.h/.cpp` 완성 | 시료 ID·고객명·수량 입력 → 확인 → [Y] 예약접수 / [N] 취소 |
| 5-3 | `View/ApprovalView.h/.cpp` 완성 | RESERVED 목록 → 번호 선택 → 재고 확인 → [Y] 승인 / [N] 거절 |
| 5-4 | `MainView` 전체 주문 수 연동 | 주문 생성/처리 후 전체 주문 수 갱신 |

### 주문 생성 화면 (PDF p.15 기반)

```
  [2] 시료 주문
  시료 ID    > S-003
  고객명      > 삼성전자 파운드리
  주문 수량   > 200
  ──────────────────────────────────
  입력 내용 확인
  시료     SiC 파워기판-6인치  (S-003)
  고객     삼성전자 파운드리
  수량     200 ea
  [Y] 예약 접수    [N] 취소
  선택 > Y
  ──────────────────────────────────
  예약 접수 완료.
  주문번호   ORD-20260612-0001
  현재 상태  RESERVED
  ※ 재고 확인은 [3] 주문 승인 메뉴에서 진행하세요.
```

### 주문 승인/거절 화면 (PDF p.17 기반)

```
  [3] 주문 승인/거절
  승인 대기 중인 예약 목록 (RESERVED)
  번호    주문번호          고객           시료                   수량     상태
  [1]    ORD-0041         LG이노텍        산화막 웨이퍼-SiO2      300 ea   RESERVED
  [2]    ORD-0042         SK하이닉스      실리콘 웨이퍼-8인치      150 ea   RESERVED
  승인할 번호 > 1
  재고 확인 중...
  재고 부족.  부족분 300 ea → 실생산량 375 ea  (225 min)
  [Y] 승인    [N] 주문 거절
  선택 > Y
  승인 완료.  RESERVED → PRODUCING
```

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V5-1 | `calcRequiredProduction` (TDD) | `ceil(100 / (0.9 × 0.9)) = 124` |
| V5-2 | `reserveOrder` FakeClock (TDD) | `orderedAt` = FakeClock 값 |
| V5-3 | `approveOrder` 케이스 A (TDD) | CONFIRMED + pureQty 감소 + reservedQty 증가 |
| V5-4 | `approveOrder` 케이스 B (TDD) | PRODUCING + reservedQty 즉시 선점 |
| V5-5 | `approveOrder` 케이스 B pureQty 불변 (TDD) | PRODUCING 전환 시 pureQty 변동 없음 |
| V5-6 | `rejectOrder` (TDD) | REJECTED + 재고 변동 없음 |
| V5-7 | 주문 생성 (직접 조작) | 입력 → 확인 → RESERVED |
| V5-8 | 주문 승인 케이스 A (직접 조작) | CONFIRMED 전환 + 재고 변화 확인 |
| V5-9 | 주문 승인 케이스 B (직접 조작) | PRODUCING + 부족분/실생산량/예상시간 표시 |
| V5-10 | 주문 거절 (직접 조작) | REJECTED 전환 + 재고 불변 |

---

## Phase 6 — 생산라인·출고 기능 완성

> **설계문서:** `DESIGN_Phase6.md` (구현 전 작성)

### 목표
**[5] 생산라인 조회**와 **[6] 출고 처리**를 완성하여 PRODUCING → CONFIRMED → RELEASE 흐름을 직접 조작할 수 있게 한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 6-1 | `Controller/ProductionController.h/.cpp` | 생성자에 `IClock&`, `getProductionQueue`, `completeProduction`, `calcEstimatedTime` |
| 6-2 | `Controller/ReleaseController.h/.cpp` | 생성자에 `IClock&`, `getConfirmedOrders`, `releaseOrder` |
| 6-3 | `View/ProductionView.h/.cpp` 완성 | 현재 처리 중 + 대기 큐(FIFO) + [C] 완료 처리 선택 |
| 6-4 | `View/ReleaseView.h/.cpp` 완성 | CONFIRMED 목록 + 번호 선택 → 출고 처리 |
| 6-5 | `MainView` 생산라인 대기 수 연동 | PRODUCING 건수 실시간 반영 |

### 생산라인 조회 화면 (PDF p.21 기반)

```
  [5] 생산라인 조회  FIFO 방식

  [현재 처리 중]
  주문번호  ORD-0038     시료  SiC 파워기판-6인치
  주문량   80 ea        재고 30 ea → 부족 50 ea → 실생산량 61 ea
  총 생산시간  49 min

  [대기 중인 주문  FIFO 순]
  순서   주문번호     시료                   주문량    부족분    실생산량   예상완료
  1     ORD-0040    산화막 웨이퍼-SiO2      150 ea   150 ea   190 ea   11:43
  2     ORD-0043    SiC 파워기판-6인치      200 ea   170 ea   206 ea   14:28

  * 부족분 = 주문량 - 재고,  실생산량 = ceil(부족분 / (수율 * 0.9))
  [C] 생산 완료 처리    [0] 위로
```

### 출고 처리 화면 (PDF p.23 기반)

```
  [6] 출고 처리
  출고 가능 주문  (CONFIRMED)
  번호   주문번호          고객          시료                   수량
  [1]   ORD-0042         SK하이닉스     실리콘 웨이퍼-8인치     150 ea
  [2]   ORD-0035         DB하이텍       포토레지스트-PR7        400 ea
  출고할 번호 > 1
  ──────────────────────────────────
  출고 처리 완료.
  주문번호   ORD-20260612-0042
  출고수량   150 ea
  처리일시   2026-06-12 09:34:02
  상태       CONFIRMED → RELEASE
```

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V6-1 | `getProductionQueue` FIFO (TDD) | 등록 순서 = 반환 순서 |
| V6-2 | `calcEstimatedTime` (TDD) | `cycleTime × requiredProduction` |
| V6-3 | `completeProduction` 상태 전환 (TDD) | PRODUCING → CONFIRMED |
| V6-4 | `completeProduction` pureQty 귀속 (TDD) | `pureQty += (requiredProduction - quantity)` |
| V6-5 | `releaseOrder` (TDD) | CONFIRMED → RELEASE + `reservedQty -= quantity` |
| V6-6 | 생산 완료 처리 (직접 조작) | CONFIRMED 전환 + 초과분 pureQty 증가 확인 |
| V6-7 | 출고 처리 (직접 조작) | RELEASE 전환 + reservedQty 감소 확인 |

---

## Phase 7 — 모니터링 기능 완성

> **설계문서:** `DESIGN_Phase7.md` (구현 전 작성)

### 목표
`MonitoringController`와 `MonitoringView`를 완성하여 **[4] 모니터링**에서 주문량·재고량 현황을 조회할 수 있게 한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 7-1 | `Controller/MonitoringController.h/.cpp` | `getOrderSummary`, `getInventorySummary` |
| 7-2 | `OrderSummary` 구조체 | 상태별 주문 수 (RESERVED/PRODUCING/CONFIRMED/RELEASE, REJECTED 제외) |
| 7-3 | `InventoryStatus` 구조체 | 시료별 재고, 상태(여유/부족/고갈), 잔여율 |
| 7-4 | `View/MonitoringView.h/.cpp` 완성 | 서브 메뉴 `[1] 주문량 확인  [2] 재고량 확인  [0] 위로` |

### 모니터링 화면 (PDF p.19 기반)

```
  [4] 모니터링   2026-06-12 09:32:15
  [1] 주문량 확인    [2] 재고량 확인    [0] 위로

  -- 주문량 확인 선택 시 --
  상태별 주문 현황
  RESERVED     3건
  CONFIRMED    8건
  PRODUCING    3건  ← 생산라인 대기
  RELEASE     18건

  -- 재고량 확인 선택 시 --
  시료명                   재고       상태    잔여율
  실리콘 웨이퍼-8인치       480 ea    여유     80%
  GaN 에피택셜-4인치        220 ea    여유     44%
  SiC 파워기판-6인치          30 ea    부족      6%
  산화막 웨이퍼-SiO2           0 ea    고갈      0%
```

**재고 상태 판정 기준 (PDF p.18):**
- **여유**: 주문 대비 재고 충분
- **부족**: 주문 대비 재고 수량 부족
- **고갈**: 수량이 0

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V7-1 | 상태별 카운트 (TDD) | RESERVED/PRODUCING/CONFIRMED/RELEASE 각 개수 정확 |
| V7-2 | REJECTED 제외 (TDD) | REJECTED 주문 집계 미포함 |
| V7-3 | 재고 상태 여유 (TDD) | pureQty ≥ RESERVED 주문 합계 |
| V7-4 | 재고 상태 부족 (TDD) | 0 < pureQty < RESERVED 주문 합계 |
| V7-5 | 재고 상태 고갈 (TDD) | pureQty == 0 |
| V7-6 | 주문량 확인 (직접 조작) | 4개 상태별 건수 표시, REJECTED 없음 |
| V7-7 | 재고량 확인 (직접 조작) | 시료별 재고·상태(여유/부족/고갈)·잔여율 표시 |

---

## Phase 8 — 데이터 영속성 + E2E 검증

> **설계문서:** `DESIGN_Phase8.md` (구현 전 작성)

### 목표
JSON 파일 기반 데이터 영속성을 추가하고, 앱 재시작 후에도 데이터가 유지됨을 검증한다.

### 구현 항목

| # | 파일 | 내용 |
|---|------|------|
| 8-1 | `SampleRepository` 확장 | 생성자에 `filePath`, `load()` / `save()` |
| 8-2 | `OrderRepository` 확장 | 동일 방식 `load()` / `save()` |
| 8-3 | `Util/DummyDataGenerator.h/.cpp` | Sample 5종 + Order 10건 생성 |
| 8-4 | `main.cpp` 완성 | `data/` 자동 생성 + 최초 실행 시 DummyData + `SystemClock` 주입 |

### JSON 키 규칙

```
samples.json: id, name, pureQuantity, reservedQuantity, yield, cycleTime, registeredAt
orders.json:  id, sampleId, customerName, quantity, status, orderedAt, requiredProduction
```

### 검증 항목

| # | 검증 | 기준 |
|---|------|------|
| V8-1 | save/load 왕복 (TDD) | 저장 → 새 Repository → load → 동일 데이터 |
| V8-2 | JSON 키 이름 (TDD) | PRD 스키마와 일치 |
| V8-3 | 재시작 후 데이터 유지 (직접 조작) | 앱 종료 → 재시작 → 이전 상태 동일 |
| V8-4 | **E2E Happy Path A** | 주문→승인(케이스A)→모니터링→출고→재고 감소 |
| V8-5 | **E2E Happy Path B** | 주문→승인(케이스B)→생산완료→출고→pureQty 초과분 |
| V8-6 | **E2E 거절** | 주문→거절→재고불변→모니터링 미집계 |
| V8-7 | **E2E FIFO** | A,B,C 케이스B 승인→생산큐 A→B→C 순서 |
| V8-8 | **재고 흐름 일관성** | `Σ pureQty + Σ reservedQty` = 초기 + 생산 - 출고 |

---

## 설계문서 작성 가이드

각 Phase 착수 전 `DESIGN_PhaseN.md` 작성. 포함 내용:

```
1. 목표 및 범위
2. 클래스/구조체 설계 (필드, 메서드 시그니처)
3. 화면 흐름 및 UI 레이아웃
4. 테스트 케이스 목록 (입력 → 예상 출력)
5. PRD 요구사항 매핑
```
