# PRD.md — 반도체 시료 주문 관리 시스템 (SampleOrderSystem)

> **S-Semi** 반도체 회사의 시료(Sample) 주문·생산·출고 흐름을 관리하는 콘솔 애플리케이션의 제품 요구사항 정의서입니다.

---

## 1. 배경 및 목적

S-Semi는 다양한 종류의 반도체 시료(Sample)를 생산하여 팹리스 업체·연구기관 등에 공급합니다.  
기존에는 수작업으로 주문을 관리하다 보니 아래와 같은 문제가 반복됐습니다.

- "이 주문 처리됐나요?"
- "재고가 있는지, 생산이 필요한지 모르겠어요."
- "이미 출고된 시료 재고를 또 차감했어요."

이를 해결하기 위해 시료 등록 → 주문 생성 → 생산/확정 → 출고까지의 전 흐름을 하나의 시스템으로 통합합니다.

---

## 2. 도메인 모델

### 2-1. Sample (시료)

시스템의 기본 단위. 등록된 시료만 주문이 가능합니다.

| 필드 | 타입 | 설명 |
|------|------|------|
| `id` | `std::string` | 시료 고유 ID (예: SAM1, SAM2 …) |
| `name` | `std::string` | 시료 이름 |
| `pureQuantity` | `int` | 순수 재고 — 출고 딱지 없는 가용 재고 (초과 생산분 + 이전 주문 처리 후 잔여분) |
| `reservedQuantity` | `int` | 주문 접수 재고 — 특정 주문에 할당되어 출고 대기 중인 재고 |
| `yield` | `double` | 수율 (0.0 ~ 1.0) |
| `cycleTime` | `double` | 생산 사이클타임 (단위: **분/개, min/ea**) |
| `registeredAt` | `std::string` | 등록 일시 (`YYYY-MM-DD HH:MM:SS`) |

**재고 구분 규칙:**

```
totalQuantity = pureQuantity + reservedQuantity

- pureQuantity  : 어떤 주문에도 묶여있지 않은 자유 재고.
                  주문 승인 판단의 기준이 되며, 생산 초과분이 이곳으로 귀속됩니다.
- reservedQuantity : 승인된 주문에 1:1로 묶인 재고.
                     해당 주문이 출고(RELEASE)되면 차감됩니다.
```

**수율 정의:**

```
yield = 합격 수량 / 전체 생산 수량
예) 100개 생산 중 90개 합격 → yield = 0.9
```

### 2-2. Order (주문)

고객이 특정 시료를 요청하면 생성됩니다.

| 필드 | 타입 | 설명 |
|------|------|------|
| `id` | `std::string` | 주문 ID (형식: `ORD` + 순번, 예: ORD1) |
| `sampleId` | `std::string` | 대상 시료 ID |
| `customerName` | `std::string` | 고객명 |
| `quantity` | `int` | 주문 수량 |
| `status` | `OrderStatus` | 주문 상태 (아래 참조) |
| `orderedAt` | `std::string` | 주문 생성 일시 |
| `requiredProduction` | `int` | 필요 생산량 (PRODUCING 전환 시 산정, 이후 불변) |

### 2-3. OrderStatus (주문 상태)

```cpp
enum class OrderStatus {
    RESERVED,   // 주문 예약 (초기 상태)
    REJECTED,   // 주문 거절
    PRODUCING,  // 생산 중 (순수 재고 부족으로 생산 필요)
    CONFIRMED,  // 주문 확정 (순수 재고 충분 or 생산 완료)
    RELEASE     // 출고 완료
};
```

---

## 3. 주문 상태 흐름

```
          ┌──────────────────────────────────┐
          │                                  ▼
       [생성]                           REJECTED
          │
       RESERVED ──[승인: 순수 재고 충분]──▶ CONFIRMED ──[출고]──▶ RELEASE
          │
          └──[승인: 순수 재고 부족]──▶ PRODUCING ──[생산 완료]──▶ CONFIRMED
```

- **REJECTED** 는 유효하지 않은 주문으로 간주하며, 대시보드·생산 큐에서 제외합니다.
- `RESERVED` 상태의 주문만 승인/거절할 수 있습니다.
- `CONFIRMED` 상태의 주문만 출고 처리할 수 있습니다.
- `PRODUCING` 상태의 주문만 생산 완료 처리할 수 있습니다.

---

## 4. 핵심 비즈니스 로직

### 4-1. 주문 생성 (Reserve)

- `sampleId`가 실제 등록된 시료인지 검증합니다.
- 통과 시 `status = RESERVED`, `orderedAt = 현재 시각`으로 Order를 생성합니다.
- 입력값: `sampleId`, `customerName`, `quantity`

---

### 4-2. 주문 승인 (Approve)

대상 주문은 반드시 `RESERVED` 상태여야 합니다.  
**판단 기준은 `pureQuantity`(순수 재고)입니다. `reservedQuantity`는 이미 다른 주문에 묶인 재고이므로 고려하지 않습니다.**

#### 케이스 A — 순수 재고 충분 (`sample.pureQuantity >= order.quantity`)

```
1. sample.pureQuantity     -= order.quantity   // 순수 재고에서 차감
2. sample.reservedQuantity += order.quantity   // 주문 접수 재고로 이동
3. order.status = CONFIRMED
```

#### 케이스 B — 순수 재고 부족 (`sample.pureQuantity < order.quantity`)

```
1. requiredProduction = ceil(order.quantity / (sample.yield × 0.9))
2. order.requiredProduction = requiredProduction
3. sample.reservedQuantity += order.quantity   // 생산 시작과 동시에 주문분 즉시 선점
4. order.status = PRODUCING
```

> **예약 선점 이유:** 생산이 시작된 순간 해당 주문분은 확약된 물량이므로, 물리 완성 전이라도 `reservedQuantity`에 즉시 반영하여 대시보드와 재고 현황이 실시간으로 정확한 상태를 보여줍니다.

> **0.9 보정 계수:** 공정 중 추가 불량을 감안한 안전 계수.  
> 예) 주문 100개, yield=0.9 → `ceil(100 / 0.81) = 124`개 생산 지시

---

### 4-3. 주문 거절 (Reject)

- 대상 주문은 반드시 `RESERVED` 상태여야 합니다.
- `order.status = REJECTED`
- 재고 변동 없음.

---

### 4-4. 생산 완료 (Complete Production)

대상 주문은 반드시 `PRODUCING` 상태여야 합니다.

**모든 생산 수량(`requiredProduction`)은 양품(합격품)으로 가정합니다.**

생산은 아래 순서로 물량을 채웁니다:
1. **먼저 주문 수량(`order.quantity`)을 채움** → 이미 케이스 B 승인 시점에 `reservedQuantity`에 선점됐으므로 추가 변동 없음
2. **그 다음 초과 생산분을 순수 재고로 귀속** → 생산 완료 시점에 `pureQuantity`에 반영

```
excessProduction = order.requiredProduction - order.quantity

1. sample.pureQuantity += excessProduction   // 초과 생산분 → 순수 재고 실시간 귀속
2. order.status = CONFIRMED
   (reservedQuantity는 PRODUCING 전환 시점에 이미 반영됨 — 변동 없음)
```

**예시:**
```
주문 100개, requiredProduction = 124개 생산 완료
  [PRODUCING 전환 시]  reservedQuantity += 100  (즉시 선점, 대시보드 실시간 반영)
  [생산 완료 시]       pureQuantity     += 24   (초과분 순수 재고 귀속)
                       order.status = CONFIRMED
```

**예상 생산 시간 계산 (화면 표시용):**
```
estimatedTime (분) = sample.cycleTime (min/ea) × order.requiredProduction
```

**생산 큐 우선순위:** FIFO (주문 등록 순서, insertion order 유지)

---

### 4-5. 출고 처리 (Release)

- 대상 주문은 반드시 `CONFIRMED` 상태여야 합니다.

```
1. sample.reservedQuantity -= order.quantity   // 출고됐으므로 물리 재고에서 제거
2. order.status = RELEASE
```

---

## 5. 재고 상태 변화 전체 흐름 요약

| 이벤트 | pureQuantity | reservedQuantity | totalQuantity | 비고 |
|--------|-------------|-----------------|--------------|------|
| 시료 최초 등록 | 초기값 입력 | 0 | 초기값 | |
| 주문 승인 (케이스 A) | `-= order.quantity` | `+= order.quantity` | 변동 없음 | 순수→접수 이동 |
| 주문 승인 (케이스 B) | 변동 없음 | `+= order.quantity` | `+= order.quantity` | 생산 예약 즉시 선점 |
| 생산 완료 | `+= excessProduction` | 변동 없음 | `+= excessProduction` | 초과분만 순수 재고 귀속 |
| 출고 | 변동 없음 | `-= order.quantity` | `-= order.quantity` | 물리 출고 |
| 주문 거절 | 변동 없음 | 변동 없음 | 변동 없음 | |

> **케이스 B 승인 시 totalQuantity가 증가하는 이유:** 아직 생산되지 않은 물량이지만 확약된 주문이므로, "생산 중인 재고"를 선반영하여 대시보드가 실시간으로 정확한 커밋 현황을 표시합니다.

---

## 6. 기능 요구사항

### 6-1. 메인 메뉴

```
[1] 시료 관리
[2] 주문 생성
[3] 주문 승인 / 거절
[4] 대시보드
[5] 생산 처리
[6] 출고 처리
[0] 종료
```

---

### 6-2. 시료 관리 (SampleController + SampleView)

| 기능 | 설명 |
|------|------|
| 시료 등록 | `id`, `name`, `pureQuantity`, `yield`, `cycleTime` 입력 후 추가 |
| 시료 목록 조회 | 전체 시료 표시 (`pureQuantity`, `reservedQuantity`, `totalQuantity` 포함) |
| 시료 검색 | 이름(키워드)으로 시료 검색 |

**Repository 인터페이스:**
```cpp
void add(const Sample& sample);
std::optional<Sample> findById(const std::string& id) const;
std::vector<Sample> findAll() const;
std::vector<Sample> findByName(const std::string& keyword) const;
bool update(const Sample& sample);
bool remove(const std::string& id);
int count() const;
```

---

### 6-3. 주문 생성 (OrderController — reserveOrder)

- 입력: `sampleId`, `customerName`, `quantity`
- 유효하지 않은 `sampleId`면 실패 반환
- 성공 시 `RESERVED` 상태의 Order 생성

---

### 6-4. 주문 승인 / 거절 (OrderController — approveOrder / rejectOrder)

- `RESERVED` 상태 주문 목록을 표시
- 주문 ID 선택 후 승인 또는 거절
- 승인 결과: `ApproveResult::SUCCESS_CONFIRMED` / `ApproveResult::SUCCESS_PRODUCING` / `ApproveResult::FAILED`

**Repository 인터페이스:**
```cpp
void add(const Order& order);
std::optional<Order> findById(const std::string& id) const;
std::vector<Order> findAll() const;
std::vector<Order> findByStatus(OrderStatus status) const;
std::vector<Order> findBySampleId(const std::string& sampleId) const;
bool update(const Order& order);
int count() const;
```

---

### 6-5. 대시보드 (DashboardController + DashboardView)

**주문 현황 요약** (REJECTED 제외):

| 상태 | 표시 내용 |
|------|-----------|
| RESERVED | 예약 주문 수 |
| PRODUCING | 생산 중 주문 수 |
| CONFIRMED | 확정 주문 수 |
| RELEASE | 출고 완료 수 |

**시료별 재고 및 수량 현황:**

| 컬럼 | 설명 |
|------|------|
| 순수 재고 | `sample.pureQuantity` |
| 주문 접수 재고 | `sample.reservedQuantity` |
| 총 재고 | `pureQuantity + reservedQuantity` |
| 예약 주문량 | RESERVED 상태 주문들의 수량 합계 |
| 생산 중 주문량 | PRODUCING 상태 주문들의 수량 합계 |
| 확정 주문량 | CONFIRMED 상태 주문들의 수량 합계 |
| 출고 완료량 | RELEASE 상태 주문들의 수량 합계 |

수량이 0인 항목도 표시합니다.

---

### 6-6. 생산 처리 (ProductionController + View)

- `PRODUCING` 상태 주문을 FIFO 순서로 큐에 표시
- 각 항목에 `예상 생산 시간(분) = sample.cycleTime × order.requiredProduction` 표시
- 주문 ID 선택 후 생산 완료 처리 → 섹션 4-4 로직 실행

---

### 6-7. 출고 처리 (ReleaseController + View)

- `CONFIRMED` 상태 주문 목록 표시
- 주문 ID 선택 후 출고 처리 → 섹션 4-5 로직 실행

---

## 7. 데이터 영속성

`nlohmann/json` (`json.hpp`) 라이브러리를 사용하여 JSON 파일로 저장합니다.

### 저장 경로

```
data/samples.json
data/orders.json
```

### JSON 스키마

**samples.json:**
```json
[
  {
    "id": "SAM1",
    "name": "GaN-Wafer-A",
    "pureQuantity": 180,
    "reservedQuantity": 20,
    "yield": 0.9,
    "cycleTime": 15.0,
    "registeredAt": "2026-06-12 09:00:00"
  }
]
```

**orders.json:**
```json
[
  {
    "id": "ORD1",
    "sampleId": "SAM1",
    "customerName": "팹리스A",
    "quantity": 50,
    "status": "CONFIRMED",
    "orderedAt": "2026-06-12 10:30:00",
    "requiredProduction": 0
  }
]
```

### 저장/로드 규칙

- Repository 생성 시 파일 경로를 받아 `load()` 자동 실행
- 데이터 변경 후 반드시 `save()` 호출
- `data/` 디렉토리가 없으면 자동 생성 (`<filesystem>`)
- 최초 실행 시 `data/` 가 비어있으면 더미 데이터 자동 생성

---

## 8. 아키텍처 규칙

### 계층 구조

```
View  ──▶  Controller  ──▶  Repository  ──▶  JSON 파일
                   │
                (Domain Model)
               Sample / Order
```

- **View**: 화면 출력 및 사용자 입력만 담당. 비즈니스 로직 금지
- **Controller**: 비즈니스 로직 수행. Repository를 통해서만 데이터 접근
- **Repository**: 데이터 CRUD 및 JSON 직렬화/역직렬화만 담당

### 파일 구조

```
SampleOrderSystem/
├── Model/
│   ├── Sample.h
│   ├── Order.h
│   ├── OrderStatus.h
│   ├── SampleRepository.h / .cpp
│   └── OrderRepository.h / .cpp
├── Controller/
│   ├── OrderController.h / .cpp
│   ├── ProductionController.h / .cpp
│   ├── ReleaseController.h / .cpp
│   ├── DashboardController.h / .cpp
│   └── SampleController.h / .cpp
├── View/
│   ├── MainView.h / .cpp
│   ├── SampleView.h / .cpp
│   ├── OrderView.h / .cpp
│   ├── ProductionView.h / .cpp
│   ├── ReleaseView.h / .cpp
│   └── DashboardView.h / .cpp
└── Util/
    ├── DummyDataGenerator.h / .cpp
    └── json.hpp
```

---

## 9. 상수 정의 (매직 넘버 금지)

```cpp
constexpr double PRODUCTION_SAFETY_FACTOR = 0.9;   // 생산 안전 계수 (초과 생산 보정)
constexpr int    MONITOR_POLL_INTERVAL_SEC = 5;     // 모니터링 폴링 주기 (초)
```

---

## 10. 테스트 요구사항

모든 비즈니스 로직은 TDD(Red-Green-Refactor)로 구현합니다.

| 대상 | 검증 항목 |
|------|-----------|
| `calcRequiredProduction` | `ceil(qty / (yield * 0.9))` 결과 정확성 |
| `approveOrder` (케이스 A) | pureQty 충분 → CONFIRMED + pureQty 차감 + reservedQty 증가 |
| `approveOrder` (케이스 B) | pureQty 부족 → PRODUCING + requiredProduction 설정 + reservedQty 즉시 선점 |
| `approveOrder` | RESERVED 아닌 주문 승인 시도 → 실패 |
| `rejectOrder` | RESERVED → REJECTED 전환, 재고 변동 없음 |
| `rejectOrder` | RESERVED 아닌 주문 거절 시도 → 실패 |
| `completeProduction` | PRODUCING → CONFIRMED + pureQty에 excessProduction 귀속, reservedQty 변동 없음 |
| `completeProduction` | excessProduction = `requiredProduction - order.quantity` 정확성 |
| `approveOrder` (케이스 B) + `completeProduction` | totalQuantity 흐름: 승인 시 `+= order.qty`, 완료 시 `+= excess` |
| `releaseOrder` | CONFIRMED → RELEASE + reservedQty 차감 |
| `releaseOrder` | CONFIRMED 아닌 주문 출고 시도 → 실패 |
| `reserveOrder` | 존재하지 않는 sampleId → 실패 |
| `DashboardData` | REJECTED 주문이 집계에서 제외 |
| `getProductionQueue` | FIFO 순서 보장 |
| `estimatedTime` | `cycleTime(min/ea) × requiredProduction` 정확성 |

---

## 11. POC 계승 사항

> `POC.md` 참고. 아래는 이 프로젝트에서 계승하기로 결정한 사항들입니다.

| 항목 | 계승 여부 | 비고 |
|------|-----------|------|
| MVC 폴더 구조 | ✅ 계승 | `Model/Controller/View/Util` 구조 동일 적용 |
| `OrderStatus` enum 5종 | ✅ 계승 | RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE |
| `calcRequiredProduction` 공식 | ✅ 계승 | `ceil(qty / (yield * 0.9))` |
| Repository 인터페이스 | ✅ 계승 | 메서드명 동일하게 유지 |
| `nlohmann/json` 직렬화 | ✅ 계승 | JSON 키 이름 동일 사용 |
| FIFO 생산 큐 | ✅ 계승 | `findByStatus(PRODUCING)` 등록 순서 유지 |
| `DummyDataGenerator` | ✅ 계승 | 최초 실행 시 자동 더미 데이터 생성 |
| `Sample.quantity` 단일 필드 | ❌ 변경 | `pureQuantity` + `reservedQuantity` 2개 필드로 분리 |
| `cycleTime` 단위 (시간/개) | ❌ 변경 | **분/개 (min/ea)** 로 변경 |
