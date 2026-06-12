# DESIGN_Phase2.md — 도메인 모델 + IClock

> **Phase 2 목표:**  
> 비즈니스 엔티티 구조체·열거형과 시간 추상화 인터페이스를 헤더 파일로 정의한다.  
> 구현 로직은 없으며, 이후 Phase 3~8의 모든 계층이 이 타입들을 참조한다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `Model/OrderStatus.h` — `enum class OrderStatus` + `toString()` 자유 함수
- `Model/Sample.h` — `struct Sample` (7개 필드)
- `Model/Order.h` — `struct Order` (7개 필드)
- `Util/Constants.h` — `PRODUCTION_SAFETY_FACTOR`, `MONITOR_POLL_INTERVAL_SEC`
- `Util/IClock.h` — 순수 가상 인터페이스
- `Util/SystemClock.h / .cpp` — `std::chrono` 기반 실제 시각 구현체

### 제외 (Out of Scope)
- Repository CRUD 구현 (Phase 3)
- Controller / View 비즈니스 로직 (Phase 4~)
- `FakeClock` 클래스 — 테스트 프로젝트 내부에서만 정의 (프로덕션 코드 오염 방지)

---

## 2. 파일 구조

```
SampleOrderSystem/
├── Model/
│   ├── OrderStatus.h        ← NEW (헤더 전용)
│   ├── Sample.h             ← NEW (헤더 전용)
│   └── Order.h              ← NEW (헤더 전용)
└── Util/
    ├── Constants.h          ← NEW (헤더 전용)
    ├── IClock.h             ← NEW (헤더 전용)
    └── SystemClock.h / .cpp ← NEW

SampleOrderSystemTest/
└── DomainModelTest.cpp      ← NEW (TDD: 테스트 선행 작성)
```

---

## 3. 클래스 / 구조체 설계

### 3-1. `Model/OrderStatus.h`

```cpp
#pragma once
#include <string>

enum class OrderStatus {
    RESERVED,   // 주문 예약 (초기 상태)
    REJECTED,   // 주문 거절
    PRODUCING,  // 생산 중 (순수 재고 부족)
    CONFIRMED,  // 주문 확정 (재고 충분 or 생산 완료)
    RELEASE     // 출고 완료
};

inline std::string toString(OrderStatus status) {
    switch (status) {
        case OrderStatus::RESERVED:  return "RESERVED";
        case OrderStatus::REJECTED:  return "REJECTED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::RELEASE:   return "RELEASE";
        default:                     return "UNKNOWN";
    }
}
```

**설계 근거:**
- `enum class` — 암묵적 정수 변환 방지, 스코프 명확화
- `toString()` — `inline` 자유 함수로 헤더 전용 구현 (`.cpp` 불필요)

---

### 3-2. `Model/Sample.h`

```cpp
#pragma once
#include <string>

struct Sample {
    std::string id;               // 시료 고유 ID (예: SAM1)
    std::string name;             // 시료 이름
    int         pureQuantity;     // 순수 재고 — 어떤 주문에도 묶이지 않은 가용 재고
    int         reservedQuantity; // 주문 접수 재고 — 출고 대기 중인 재고
    double      yield;            // 수율 (0.0 ~ 1.0)
    double      cycleTime;        // 생산 사이클타임 (min/ea)
    std::string registeredAt;     // 등록 일시 (YYYY-MM-DD HH:MM:SS)
};
```

**설계 근거:**
- 단순 데이터 집합이므로 `struct` 사용 (캡슐화 불필요)
- `pureQuantity` / `reservedQuantity` 2-필드 분리 — PRD §2-1 재고 구분 규칙
- `cycleTime` 단위: **min/ea** — PRD §11 변경 사항

---

### 3-3. `Model/Order.h`

```cpp
#pragma once
#include <string>
#include "OrderStatus.h"

struct Order {
    std::string id;                 // 주문 ID (예: ORD1)
    std::string sampleId;           // 대상 시료 ID
    std::string customerName;       // 고객명
    int         quantity;           // 주문 수량
    OrderStatus status;             // 주문 상태
    std::string orderedAt;          // 주문 생성 일시 (YYYY-MM-DD HH:MM:SS)
    int         requiredProduction; // 필요 생산량 (PRODUCING 전환 시 산정, 이후 불변)
};
```

**설계 근거:**
- `requiredProduction` — 케이스 B 승인 시 1회 설정 후 불변 (PRD §4-2)
- 기본값: `status = RESERVED`, `requiredProduction = 0`

---

### 3-4. `Util/Constants.h`

```cpp
#pragma once

constexpr double PRODUCTION_SAFETY_FACTOR  = 0.9; // 생산 안전 계수 (공정 중 추가 불량 보정)
constexpr int    MONITOR_POLL_INTERVAL_SEC = 5;   // 모니터링 폴링 주기 (초)
```

**설계 근거:**
- 매직 넘버 `0.9` 가 소스 전체에 흩어지는 것을 방지 (PRD §9, CLAUDE.md 규칙)

---

### 3-5. `Util/IClock.h`

```cpp
#pragma once
#include <string>

class IClock {
public:
    virtual ~IClock() = default;
    virtual std::string now() const = 0; // "YYYY-MM-DD HH:MM:SS"
};
```

---

### 3-6. `Util/SystemClock.h / .cpp`

```cpp
// SystemClock.h
#pragma once
#include "IClock.h"

class SystemClock : public IClock {
public:
    std::string now() const override;
};

// SystemClock.cpp
// std::chrono + localtime_s 로 현재 시각을 "YYYY-MM-DD HH:MM:SS" 형식으로 반환
```

---

## 4. 테스트 설계 (`DomainModelTest.cpp`)

> **TDD 원칙:** 테스트를 먼저 작성 → 빌드 실패 확인 → 헤더/소스 작성 → 테스트 통과 확인

### 4-1. `FakeClock` (테스트 전용 헬퍼)

```cpp
// DomainModelTest.cpp 상단에 인라인으로 정의
class FakeClock : public IClock {
public:
    explicit FakeClock(std::string fixedTime) : time_(std::move(fixedTime)) {}
    std::string now() const override { return time_; }
    void setTime(const std::string& t) { time_ = t; }
private:
    std::string time_;
};
```

### 4-2. 테스트 케이스 목록

| # | 테스트 이름 | 입력 | 예상 출력 |
|---|------------|------|-----------|
| T2-1 | `OrderStatus_ToString_Reserved` | `OrderStatus::RESERVED` | `"RESERVED"` |
| T2-2 | `OrderStatus_ToString_Rejected` | `OrderStatus::REJECTED` | `"REJECTED"` |
| T2-3 | `OrderStatus_ToString_Producing` | `OrderStatus::PRODUCING` | `"PRODUCING"` |
| T2-4 | `OrderStatus_ToString_Confirmed` | `OrderStatus::CONFIRMED` | `"CONFIRMED"` |
| T2-5 | `OrderStatus_ToString_Release` | `OrderStatus::RELEASE` | `"RELEASE"` |
| T2-6 | `Sample_Fields_TwoQuantityFields` | `Sample` 생성 후 pureQuantity·reservedQuantity 각각 설정 | 필드 독립 확인 |
| T2-7 | `Order_DefaultRequiredProduction` | `Order` 생성 시 `requiredProduction = 0` | 초기값 0 |
| T2-8 | `Constants_ProductionSafetyFactor` | `PRODUCTION_SAFETY_FACTOR` | `0.9` |
| T2-9 | `FakeClock_ReturnsFixedTime` | `FakeClock("2026-06-12 09:00:00")` → `now()` | `"2026-06-12 09:00:00"` |
| T2-10 | `FakeClock_SetTime` | `setTime("2026-06-12 10:00:00")` → `now()` | `"2026-06-12 10:00:00"` |
| T2-11 | `SystemClock_ReturnsNonEmpty` | `SystemClock` → `now()` | 길이 19의 문자열 (`YYYY-MM-DD HH:MM:SS`) |

---

## 5. `.vcxproj` 업데이트 항목

### `SampleOrderSystem.vcxproj`

**추가 `<ClInclude>`:**
```xml
<ClInclude Include="Model\OrderStatus.h" />
<ClInclude Include="Model\Sample.h" />
<ClInclude Include="Model\Order.h" />
<ClInclude Include="Util\Constants.h" />
<ClInclude Include="Util\IClock.h" />
<ClInclude Include="Util\SystemClock.h" />
```

**추가 `<ClCompile>`:**
```xml
<ClCompile Include="Util\SystemClock.cpp" />
```

### `SampleOrderSystem.vcxproj.filters`

```xml
<Filter Include="Model" />

<ClInclude Include="Model\OrderStatus.h"><Filter>Model</Filter></ClInclude>
<ClInclude Include="Model\Sample.h"><Filter>Model</Filter></ClInclude>
<ClInclude Include="Model\Order.h"><Filter>Model</Filter></ClInclude>
<ClInclude Include="Util\Constants.h"><Filter>Util</Filter></ClInclude>
<ClInclude Include="Util\IClock.h"><Filter>Util</Filter></ClInclude>
<ClInclude Include="Util\SystemClock.h"><Filter>Util</Filter></ClInclude>
<ClCompile Include="Util\SystemClock.cpp"><Filter>Util</Filter></ClCompile>
```

### `SampleOrderSystemTest.vcxproj`

**추가 `<ClCompile>`:**
```xml
<ClCompile Include="DomainModelTest.cpp" />
```

---

## 6. PRD 요구사항 매핑

| PRD 항목 | Phase 2 반영 여부 | 비고 |
|---------|-----------------|------|
| `Sample` 7개 필드 | ✅ | `pureQuantity` + `reservedQuantity` 2필드 분리 |
| `Order` 7개 필드 | ✅ | `requiredProduction` 포함 |
| `OrderStatus` 5종 | ✅ | RESERVED/REJECTED/PRODUCING/CONFIRMED/RELEASE |
| `PRODUCTION_SAFETY_FACTOR = 0.9` | ✅ | `Constants.h` 에 `constexpr` |
| `MONITOR_POLL_INTERVAL_SEC = 5` | ✅ | `Constants.h` 에 `constexpr` |
| `IClock` 인터페이스 | ✅ | 순수 가상, 생성자 주입 패턴 |
| `SystemClock` 구현체 | ✅ | `std::chrono` 기반 |
| 매직 넘버 금지 | ✅ | `0.9` 를 `PRODUCTION_SAFETY_FACTOR` 로 추출 |
| TDD | ✅ | `DomainModelTest.cpp` 선행 작성 후 구현 |

---

## 7. Phase 2 완료 기준

| # | 검증 | 기준 |
|---|------|------|
| V2-1 | Debug\|x64 빌드 성공 | 경고·오류 0건 |
| V2-2 | `OrderStatus` 5종 `toString()` | T2-1 ~ T2-5 모두 통과 |
| V2-3 | `Sample` 2-필드 분리 | T2-6 통과 |
| V2-4 | `Order` 기본값 | T2-7 통과 |
| V2-5 | `PRODUCTION_SAFETY_FACTOR` 값 | T2-8 통과 |
| V2-6 | `FakeClock` 주입·시각 변경 | T2-9, T2-10 통과 |
| V2-7 | `SystemClock` 비어있지 않은 시각 | T2-11 통과 |
| V2-8 | 매직 넘버 `0.9` 없음 | Constants.h 상수 참조 확인 |
