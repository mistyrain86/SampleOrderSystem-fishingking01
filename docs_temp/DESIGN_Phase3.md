# DESIGN_Phase3.md — Repository 계층 (In-Memory)

> **Phase 3 목표:**  
> 도메인 데이터의 CRUD 인터페이스를 순수 가상 클래스로 정의하고,  
> In-Memory 구현체를 작성하여 단위 테스트를 모두 통과시킨다.  
> 이 Phase가 끝나면 Controller(Phase 4~)가 Repository에 의존성을 주입받아 사용할 수 있다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `ISampleRepository` — 순수 가상 인터페이스 (gmock 모킹 대상)
- `SampleRepository` — In-Memory 구현체 (`std::vector<Sample>`)
- `IOrderRepository` — 순수 가상 인터페이스
- `OrderRepository` — In-Memory 구현체 (`std::vector<Order>`, FIFO 보장)
- `Test/RepositoryTest.cpp` — TDD: 26개 단위 테스트 선행 작성

### 제외 (Out of Scope)
- JSON 파일 저장/로드 (Phase 8)
- ID 자동 생성 — 책임은 Controller (Phase 4~)
- Controller / View 연동

---

## 2. 파일 구조

```
SampleOrderSystem/
└── Model/
    ├── ISampleRepository.h   ← NEW (순수 가상 인터페이스)
    ├── SampleRepository.h    ← NEW
    ├── SampleRepository.cpp  ← NEW
    ├── IOrderRepository.h    ← NEW (순수 가상 인터페이스)
    ├── OrderRepository.h     ← NEW
    └── OrderRepository.cpp   ← NEW

    └── Test/
        └── RepositoryTest.cpp ← NEW (TDD 선행 작성, #ifdef _DEBUG)
```

---

## 3. 인터페이스 설계

### 3-1. `Model/ISampleRepository.h`

```cpp
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "Sample.h"

class ISampleRepository {
public:
    virtual ~ISampleRepository() = default;

    virtual void                       add(const Sample& sample)                        = 0;
    virtual std::optional<Sample>      findById(const std::string& id)           const  = 0;
    virtual std::vector<Sample>        findAll()                                  const  = 0;
    virtual std::vector<Sample>        findByName(const std::string& keyword)     const  = 0;
    virtual bool                       update(const Sample& sample)                     = 0;
    virtual bool                       remove(const std::string& id)                    = 0;
    virtual int                        count()                                    const  = 0;
};
```

### 3-2. `Model/IOrderRepository.h`

```cpp
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "Order.h"
#include "OrderStatus.h"

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    virtual void                       add(const Order& order)                          = 0;
    virtual std::optional<Order>       findById(const std::string& id)           const  = 0;
    virtual std::vector<Order>         findAll()                                  const  = 0;
    virtual std::vector<Order>         findByStatus(OrderStatus status)           const  = 0;
    virtual std::vector<Order>         findBySampleId(const std::string& sampleId) const = 0;
    virtual bool                       update(const Order& order)                       = 0;
    virtual int                        count()                                    const  = 0;
};
```

---

## 4. 구현체 설계

### 4-1. `Model/SampleRepository.h`

```cpp
#pragma once
#include "ISampleRepository.h"
#include <vector>

class SampleRepository : public ISampleRepository {
public:
    void                  add(const Sample& sample)                    override;
    std::optional<Sample> findById(const std::string& id)       const  override;
    std::vector<Sample>   findAll()                              const  override;
    std::vector<Sample>   findByName(const std::string& keyword) const  override;
    bool                  update(const Sample& sample)                  override;
    bool                  remove(const std::string& id)                 override;
    int                   count()                                const  override;

private:
    std::vector<Sample> samples_;
};
```

### 4-2. `Model/SampleRepository.cpp` — 구현 요점

| 메서드 | 구현 방식 |
|--------|---------|
| `add` | `samples_.push_back(sample)` |
| `findById` | `id` 일치 항목 반환, 없으면 `std::nullopt` |
| `findAll` | `samples_` 복사 반환 |
| `findByName` | `keyword`가 `name`에 포함(`find != npos`)된 항목 필터링 |
| `update` | `id` 일치 항목 전체 교체, 없으면 `false` |
| `remove` | `id` 일치 항목 삭제(`erase`), 없으면 `false` |
| `count` | `static_cast<int>(samples_.size())` |

### 4-3. `Model/OrderRepository.h`

```cpp
#pragma once
#include "IOrderRepository.h"
#include <vector>

class OrderRepository : public IOrderRepository {
public:
    void                 add(const Order& order)                          override;
    std::optional<Order> findById(const std::string& id)          const   override;
    std::vector<Order>   findAll()                                 const   override;
    std::vector<Order>   findByStatus(OrderStatus status)          const   override;
    std::vector<Order>   findBySampleId(const std::string& sampleId) const override;
    bool                 update(const Order& order)                        override;
    int                  count()                                   const   override;

private:
    std::vector<Order> orders_; // 삽입 순서 = FIFO 순서 보장
};
```

### 4-4. `Model/OrderRepository.cpp` — 구현 요점

| 메서드 | 구현 방식 |
|--------|---------|
| `add` | `orders_.push_back(order)` — 삽입 순서가 곧 FIFO |
| `findByStatus` | `status` 일치 항목 필터링 (삽입 순서 유지) |
| `findBySampleId` | `sampleId` 일치 항목 필터링 |
| `update` | `id` 일치 항목 전체 교체, 없으면 `false` |

---

## 5. 테스트 설계 (`Test/RepositoryTest.cpp`)

> **TDD 원칙:** 테스트 먼저 작성 → 빌드 실패 확인 → 구현 후 전체 통과

### 5-1. MockRepository (gmock) — Controller 테스트용 사전 정의

```cpp
// gmock Mock 클래스 — Controller 단위 테스트 시 Repository를 대체
class MockSampleRepository : public ISampleRepository {
public:
    MOCK_METHOD(void,                       add,        (const Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>,      findById,   (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Sample>,        findAll,    (),                   (const, override));
    MOCK_METHOD(std::vector<Sample>,        findByName, (const std::string&), (const, override));
    MOCK_METHOD(bool,                       update,     (const Sample&),              (override));
    MOCK_METHOD(bool,                       remove,     (const std::string&),         (override));
    MOCK_METHOD(int,                        count,      (),                   (const, override));
};

class MockOrderRepository : public IOrderRepository {
public:
    MOCK_METHOD(void,                       add,           (const Order&),               (override));
    MOCK_METHOD(std::optional<Order>,       findById,      (const std::string&), (const, override));
    MOCK_METHOD(std::vector<Order>,         findAll,       (),                   (const, override));
    MOCK_METHOD(std::vector<Order>,         findByStatus,  (OrderStatus),        (const, override));
    MOCK_METHOD(std::vector<Order>,         findBySampleId,(const std::string&), (const, override));
    MOCK_METHOD(bool,                       update,        (const Order&),               (override));
    MOCK_METHOD(int,                        count,         (),                   (const, override));
};
```

> Mock 클래스는 이 파일에 정의해두고 Phase 4~부터 Controller 테스트에서 재사용한다.

### 5-2. SampleRepository 테스트 케이스

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T3-1 | `Add_IncreasesCount` | `add` 1회 → `count()` | `1` |
| T3-2 | `FindById_Exists` | `add` 후 동일 id `findById` | `has_value() == true`, 필드 일치 |
| T3-3 | `FindById_NotExists` | 없는 id `findById` | `std::nullopt` |
| T3-4 | `FindAll_ReturnsAllItems` | 3개 `add` → `findAll` | 크기 `3` |
| T3-5 | `FindAll_Empty` | 빈 저장소 `findAll` | 빈 벡터 |
| T3-6 | `FindByName_PartialMatch` | `"웨이퍼"` 포함 시료 2개 등록 → `findByName("웨이퍼")` | 크기 `2` |
| T3-7 | `FindByName_NoMatch` | `findByName("없는키워드")` | 빈 벡터 |
| T3-8 | `FindByName_ExactMatch` | 정확한 이름 검색 | 크기 `1` |
| T3-9 | `Update_Success` | `add` 후 `pureQuantity` 변경 `update` → `findById` | 변경된 값 반환 |
| T3-10 | `Update_NotExists` | 없는 id `update` | `false` |
| T3-11 | `Remove_Success` | `add` 후 `remove` → `findById` | `std::nullopt`, `count() == 0` |
| T3-12 | `Remove_NotExists` | 없는 id `remove` | `false` |
| T3-13 | `Remove_DecreasesCount` | 2개 add → 1개 remove → `count()` | `1` |

### 5-3. OrderRepository 테스트 케이스

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T3-14 | `Add_IncreasesCount` | `add` 1회 → `count()` | `1` |
| T3-15 | `FindById_Exists` | `add` 후 동일 id `findById` | `has_value() == true` |
| T3-16 | `FindById_NotExists` | 없는 id | `std::nullopt` |
| T3-17 | `FindByStatus_Reserved` | RESERVED 2개, CONFIRMED 1개 → `findByStatus(RESERVED)` | 크기 `2` |
| T3-18 | `FindByStatus_Rejected` | REJECTED 주문 → `findByStatus(REJECTED)` | 정상 조회 가능 |
| T3-19 | `FindByStatus_Empty` | 해당 상태 주문 없음 | 빈 벡터 |
| T3-20 | `FindBySampleId_Match` | 동일 sampleId 2개 → `findBySampleId` | 크기 `2` |
| T3-21 | `FindBySampleId_NoMatch` | 없는 sampleId | 빈 벡터 |
| T3-22 | `Update_StatusChange` | RESERVED → CONFIRMED `update` → `findById` | `CONFIRMED` |
| T3-23 | `Update_NotExists` | 없는 id `update` | `false` |
| T3-24 | `FifoOrder_PreservedOnFindAll` | A→B→C 순 `add` → `findAll` | A, B, C 순서 |
| T3-25 | `FifoOrder_PreservedOnFindByStatus` | A→B→C RESERVED → `findByStatus(RESERVED)` | A, B, C 순서 |
| T3-26 | `FindAll_Empty` | 빈 저장소 `findAll` | 빈 벡터 |

---

## 6. `.vcxproj` 업데이트 항목

### `SampleOrderSystem.vcxproj`

**추가 `<ClInclude>`:**
```xml
<ClInclude Include="Model\ISampleRepository.h" />
<ClInclude Include="Model\SampleRepository.h" />
<ClInclude Include="Model\IOrderRepository.h" />
<ClInclude Include="Model\OrderRepository.h" />
```

**추가 `<ClCompile>`:**
```xml
<ClCompile Include="Model\SampleRepository.cpp" />
<ClCompile Include="Model\OrderRepository.cpp" />
```

**추가 `<ClCompile>` (Test, Release ExcludedFromBuild):**
```xml
<ClCompile Include="Test\RepositoryTest.cpp">
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
</ClCompile>
```

### `SampleOrderSystem.vcxproj.filters`

```xml
<ClInclude Include="Model\ISampleRepository.h"><Filter>Model</Filter></ClInclude>
<ClInclude Include="Model\SampleRepository.h"><Filter>Model</Filter></ClInclude>
<ClInclude Include="Model\IOrderRepository.h"><Filter>Model</Filter></ClInclude>
<ClInclude Include="Model\OrderRepository.h"><Filter>Model</Filter></ClInclude>
<ClCompile Include="Model\SampleRepository.cpp"><Filter>Model</Filter></ClCompile>
<ClCompile Include="Model\OrderRepository.cpp"><Filter>Model</Filter></ClCompile>
<ClCompile Include="Test\RepositoryTest.cpp"><Filter>Test</Filter></ClCompile>
```

---

## 7. PRD 요구사항 매핑

| PRD 항목 | Phase 3 반영 여부 | 비고 |
|---------|-----------------|------|
| `ISampleRepository` 7개 메서드 | ✅ | PRD §6-2 인터페이스 완전 구현 |
| `IOrderRepository` 7개 메서드 | ✅ | PRD §6-4 인터페이스 완전 구현 |
| FIFO 생산 큐 보장 | ✅ | `std::vector` 삽입 순서 = 조회 순서 |
| `findByName` 부분 일치 | ✅ | `std::string::find` 사용 |
| gmock 인터페이스 모킹 | ✅ | `MockSampleRepository`, `MockOrderRepository` 정의 |
| TDD | ✅ | `RepositoryTest.cpp` 선행 작성 후 구현 |

---

## 8. Phase 3 완료 기준

| # | 검증 | 기준 |
|---|------|------|
| V3-1 | Debug 빌드 성공 | 경고·오류 0건 |
| V3-2 | SampleRepository 테스트 | T3-1 ~ T3-13 (13개) 전체 통과 |
| V3-3 | OrderRepository 테스트 | T3-14 ~ T3-26 (13개) 전체 통과 |
| V3-4 | FIFO 순서 검증 | T3-24, T3-25 통과 |
| V3-5 | Mock 클래스 컴파일 | `MockSampleRepository`, `MockOrderRepository` 빌드 오류 없음 |
| V3-6 | Release 빌드 성공 | 테스트 코드 제외 후 앱 빌드 정상 |
