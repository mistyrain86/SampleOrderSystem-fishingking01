# DESIGN_Phase4.md — 시료 관리 기능 완성

> **Phase 4 목표:**  
> `SampleController`와 `SampleView`를 완성하여 **[1] 시료 관리** 메뉴에서  
> 시료 등록·목록·검색을 실제로 조작할 수 있게 한다.  
> `MainView` 시스템 현황의 "등록 시료 / 총 재고" 수치를 실제 데이터로 연동한다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `Controller/SampleController.h/.cpp`
- `View/SampleView.h/.cpp` — 스텁 교체, 서브 메뉴 완성
- `View/MainView.h/.cpp` — `SampleController` 연동, 시스템 현황 갱신
- `Test/SampleControllerTest.cpp` — TDD 선행 작성

### 제외 (Out of Scope)
- 주문·승인·생산·출고 기능 (Phase 5~)
- MainView 전체 주문 수 / 생산라인 대기 수 연동 (Phase 5~6)
- JSON 파일 저장 (Phase 8)

---

## 2. 파일 구조

```
SampleOrderSystem/
├── Controller/
│   ├── SampleController.h   ← NEW
│   └── SampleController.cpp ← NEW
└── View/
    ├── SampleView.h         ← 수정 (스텁 → 완성)
    ├── SampleView.cpp       ← 수정
    ├── MainView.h           ← 수정 (SampleController* 연동)
    └── MainView.cpp         ← 수정

    └── Test/
        └── SampleControllerTest.cpp ← NEW (TDD 선행 작성)
```

---

## 3. 클래스 설계

### 3-1. `Controller/SampleController.h`

```cpp
#pragma once
#include <string>
#include <vector>
#include "Model/ISampleRepository.h"
#include "Util/IClock.h"

class SampleController {
public:
    SampleController(ISampleRepository& repo, IClock& clock);

    // 시료 등록 — id 중복 시 false 반환
    bool addSample(const std::string& id,
                   const std::string& name,
                   int                pureQuantity,
                   double             yield,
                   double             cycleTime);

    std::vector<Sample> getAllSamples()                          const;
    std::vector<Sample> searchByName(const std::string& keyword) const;

    // MainView 시스템 현황용
    int getSampleCount()    const; // 등록된 시료 종 수
    int getTotalInventory() const; // 전체 pureQty + reservedQty 합계

private:
    ISampleRepository& repo_;
    IClock&            clock_;
};
```

**설계 근거:**
- `ISampleRepository&` / `IClock&` 주입 → 테스트에서 Mock/FakeClock 교체 가능
- `addSample` 반환값 `bool` — 중복 ID 실패를 View에서 메시지로 처리
- `getTotalInventory()` — `pureQuantity + reservedQuantity` 전 시료 합산

### 3-2. `SampleController::addSample` 흐름

```
1. repo_.findById(id) → has_value() → return false  // 중복 ID
2. Sample 생성:
     id, name, pureQuantity, yield, cycleTime
     reservedQuantity = 0
     registeredAt = clock_.now()
3. repo_.add(sample)
4. return true
```

### 3-3. `View/SampleView.h` (수정)

```cpp
#pragma once
#include "Controller/SampleController.h"

class SampleView {
public:
    explicit SampleView(SampleController& controller);
    void show();

private:
    void showRegister();
    void showList()   const;
    void showSearch() const;
    int  readChoice() const;

    SampleController& controller_;
};
```

### 3-4. `View/MainView.h` (수정)

```cpp
#pragma once
class SampleController;

class MainView {
public:
    MainView() = default;
    void setSampleController(SampleController* ctrl); // Phase 4에서 연동
    void run();

private:
    void printHeader() const;
    void printMenu()   const;
    int  readChoice()  const;
    void dispatch(int choice);

    SampleController* sampleCtrl_ = nullptr; // nullptr → 0 표시
};
```

**설계 근거:**
- setter 패턴 — Phase마다 새 Controller를 추가하면서 MainView 생성자 시그니처를 바꾸지 않아도 됨
- `nullptr` → 기존처럼 0 표시 (Phase 1 동작 보존)

---

## 4. 화면 설계

### 4-1. 시료 관리 서브 메뉴

```
================================================================
  [1] 시료 관리
----------------------------------------------------------------
  [1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 위로
  선택 > _
```

### 4-2. 시료 등록 화면

```
----------------------------------------------------------------
  [1] 시료 등록
  시료 ID       > SAM1
  시료명         > 실리콘 웨이퍼-8인치
  초기 재고 (ea) > 500
  수율 (0~1)    > 0.92
  사이클타임(min/ea) > 0.5
----------------------------------------------------------------
  등록 완료: SAM1 / 실리콘 웨이퍼-8인치
  Enter를 눌러 계속...
```

오류 케이스:
```
  오류: 이미 등록된 시료 ID입니다. (SAM1)
```

### 4-3. 시료 목록 화면 (PDF p.13 기반)

```
----------------------------------------------------------------
  등록 시료 목록  (총 N종)

  ID      시료명                  사이클타임    수율    순수재고   접수재고   총재고
  SAM1    실리콘 웨이퍼-8인치      0.50 min/ea  0.92    480 ea     20 ea    500 ea
  SAM2    GaN 에피택셜-4인치       0.30 min/ea  0.78    220 ea      0 ea    220 ea
----------------------------------------------------------------
  Enter를 눌러 계속...
```

### 4-4. 시료 검색 화면

```
----------------------------------------------------------------
  [3] 시료 검색
  검색어 > 웨이퍼
----------------------------------------------------------------
  검색 결과  (2건)

  ID      시료명                  사이클타임    수율    순수재고   접수재고   총재고
  SAM1    실리콘 웨이퍼-8인치      0.50 min/ea  0.92    480 ea     20 ea    500 ea
----------------------------------------------------------------
  Enter를 눌러 계속...
```

검색 결과 없음:
```
  검색 결과 없음.
```

### 4-5. MainView 시스템 현황 (연동 후)

```
================================================================
  S-Semi 반도체 시료 생산주문관리 시스템
================================================================
  시스템 현황   2026-06-12 09:32:15

  등록 시료   2 종      총 재고   720 ea        ← 실제 데이터
  전체 주문   0 건      생산라인    0 건 대기    ← Phase 5~6에서 연동
----------------------------------------------------------------
```

---

## 5. 테스트 설계 (`Test/SampleControllerTest.cpp`)

> MockSampleRepository / FakeClock 사용. `RepositoryTest.cpp`에 정의된 Mock 재사용.

### 5-1. 테스트 케이스

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T4-1 | `AddSample_Success` | 없는 id → `addSample` | `true`, `repo.add` 1회 호출 |
| T4-2 | `AddSample_DuplicateId` | 이미 있는 id → `addSample` | `false`, `repo.add` 미호출 |
| T4-3 | `AddSample_SetsRegisteredAt` | FakeClock 고정 시각 → `addSample` | `registeredAt == FakeClock.now()` |
| T4-4 | `AddSample_ReservedQuantityZero` | `addSample` 후 저장된 Sample | `reservedQuantity == 0` |
| T4-5 | `GetAllSamples_DelegatesToRepo` | `getAllSamples` 호출 | `repo.findAll` 1회 호출 |
| T4-6 | `SearchByName_DelegatesToRepo` | `searchByName("웨이퍼")` | `repo.findByName("웨이퍼")` 1회 호출 |
| T4-7 | `GetSampleCount_DelegatesToRepo` | `getSampleCount` 호출 | `repo.count()` 1회 호출 |
| T4-8 | `GetTotalInventory_SumsBothFields` | pureQty=100, reservedQty=20인 시료 2개 | `240` 반환 |
| T4-9 | `GetTotalInventory_EmptyRepo` | 시료 없음 | `0` 반환 |

### 5-2. FakeClock 위치

`DomainModelTest.cpp`에 이미 정의된 `FakeClock`을 동일 패턴으로 `SampleControllerTest.cpp` 상단에 재정의한다.  
(각 테스트 파일이 독립적으로 컴파일되므로 중복 정의 문제 없음)

---

## 6. `.vcxproj` 업데이트 항목

### `SampleOrderSystem.vcxproj`

**추가 `<ClInclude>`:**
```xml
<ClInclude Include="Controller\SampleController.h" />
```

**추가 `<ClCompile>`:**
```xml
<ClCompile Include="Controller\SampleController.cpp" />
```

**추가 `<ClCompile>` (Test, Release ExcludedFromBuild):**
```xml
<ClCompile Include="Test\SampleControllerTest.cpp">
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
</ClCompile>
```

### `SampleOrderSystem.vcxproj.filters`

```xml
<Filter Include="Controller" />

<ClInclude Include="Controller\SampleController.h">
  <Filter>Controller</Filter>
</ClInclude>
<ClCompile Include="Controller\SampleController.cpp">
  <Filter>Controller</Filter>
</ClCompile>
<ClCompile Include="Test\SampleControllerTest.cpp">
  <Filter>Test</Filter>
</ClCompile>
```

---

## 7. PRD 요구사항 매핑

| PRD 항목 | Phase 4 반영 여부 | 비고 |
|---------|-----------------|------|
| 시료 등록 (id/name/pureQty/yield/cycleTime) | ✅ | PRD §6-2 |
| 시료 목록 (pureQty + reservedQty + totalQty) | ✅ | PRD §6-2 |
| 시료 검색 (키워드 부분 일치) | ✅ | PRD §6-2 |
| `registeredAt = IClock.now()` | ✅ | PRD §8 아키텍처 규칙 |
| `reservedQuantity` 초기값 0 | ✅ | PRD §5 재고 흐름 |
| 중복 ID 방어 | ✅ | PRD §4-1 sampleId 검증 |
| MainView 시스템 현황 등록 시료·총 재고 연동 | ✅ | PRD §6-1 |
| TDD | ✅ | `SampleControllerTest.cpp` 선행 작성 |

---

## 8. Phase 4 완료 기준

| # | 검증 | 기준 |
|---|------|------|
| V4-1 | Debug 빌드 성공 | 경고·오류 0건 |
| V4-2 | SampleController 단위 테스트 | T4-1 ~ T4-9 (9개) 전체 통과 |
| V4-3 | 시료 등록 (직접 조작) | 입력 후 목록에서 확인 가능 |
| V4-4 | 중복 ID 거부 (직접 조작) | 오류 메시지 출력 |
| V4-5 | 목록 조회 (직접 조작) | ID·시료명·사이클타임·수율·순수재고·접수재고·총재고 표시 |
| V4-6 | 이름 검색 (직접 조작) | 키워드 → 매칭 시료만 출력 |
| V4-7 | `[0]` 메인 메뉴 복귀 | 정상 복귀 확인 |
| V4-8 | MainView 시스템 현황 | 시료 등록 후 등록 시료 수·총 재고 수치 갱신 확인 |
| V4-9 | Release 빌드 성공 | 테스트 코드 제외 후 앱 빌드 정상 |
