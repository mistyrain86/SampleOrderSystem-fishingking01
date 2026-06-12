# DESIGN_Phase8.md — 데이터 영속성 + E2E 검증

> **Phase 8 목표:**  
> `nlohmann/json`을 이용한 JSON 파일 기반 데이터 영속성을 추가하여  
> 앱 재시작 후에도 시료·주문 데이터가 유지되도록 한다.  
> E2E 시나리오를 직접 조작하여 전체 비즈니스 흐름이 정상 동작함을 검증한다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `Model/SampleRepository.h/.cpp` — `filePath` 인자 추가, `load()` / `save()` 구현
- `Model/OrderRepository.h/.cpp` — 동일
- `Model/OrderStatus.h` — `fromString()` 역직렬화 헬퍼 추가
- `main.cpp` — `data/` 디렉토리 생성 + 파일 경로 전달
- `Test/PersistenceTest.cpp` — TDD 선행 작성 (파일 I/O 통합 테스트)

### 제외 (Out of Scope)
- 데이터 마이그레이션 (스키마 변경 대응)
- 동시성(다중 프로세스) 처리
- 암호화·압축

---

## 2. 파일 구조

```
SampleOrderSystem/
├── Model/
│   ├── SampleRepository.h/.cpp   ← 수정 (filePath 생성자 + load/save)
│   ├── OrderRepository.h/.cpp    ← 수정 (filePath 생성자 + load/save)
│   └── OrderStatus.h             ← 수정 (fromString() 추가)
├── main.cpp                      ← 수정 (data/ 생성, 파일 경로 전달)
└── Test/
    └── PersistenceTest.cpp       ← NEW (TDD 선행 작성)
```

---

## 3. JSON 스키마

### `data/samples.json`

```json
[
  {
    "id":               "S-001",
    "name":             "실리콘 웨이퍼-8인치",
    "pureQuantity":     480,
    "reservedQuantity": 0,
    "yield":            0.92,
    "cycleTime":        0.5,
    "registeredAt":     "2026-06-12 09:00:00"
  }
]
```

### `data/orders.json`

```json
[
  {
    "id":                   "ORD0001",
    "sampleId":             "S-001",
    "customerName":         "삼성전자 파운드리",
    "quantity":             100,
    "status":               "CONFIRMED",
    "orderedAt":            "2026-06-12 10:30:00",
    "requiredProduction":   0,
    "productionShortage":   0,
    "productionStartedAt":  ""
  }
]
```

> `productionShortage` / `productionStartedAt` 은 Phase 6에서 추가된 필드.  
> 기존 더미 데이터와의 호환을 위해 `at_or(key, default)` 방식으로 읽는다.

---

## 4. 클래스 설계

### 4-1. `Model/OrderStatus.h` — `fromString()` 추가

```cpp
inline OrderStatus fromString(const std::string& s) {
    if (s == "RESERVED")  return OrderStatus::RESERVED;
    if (s == "REJECTED")  return OrderStatus::REJECTED;
    if (s == "PRODUCING") return OrderStatus::PRODUCING;
    if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (s == "RELEASE")   return OrderStatus::RELEASE;
    return OrderStatus::RESERVED;  // 알 수 없는 값 → 기본값
}
```

### 4-2. `Model/SampleRepository.h` — 생성자 변경

```cpp
class SampleRepository : public ISampleRepository {
public:
    explicit SampleRepository(std::string filePath = "");

    // ISampleRepository 메서드 (동일)

private:
    void load();
    void save() const;

    std::string         filePath_;
    std::vector<Sample> samples_;
};
```

- `filePath`가 비어 있으면 in-memory 모드 (기존 단위 테스트 호환).
- `filePath`가 있으면:
  - 생성자에서 `load()` 호출
  - `add()` / `update()` / `remove()` 후 `save()` 호출

### 4-3. `Model/OrderRepository.h` — 동일 패턴

```cpp
class OrderRepository : public IOrderRepository {
public:
    explicit OrderRepository(std::string filePath = "");

    // IOrderRepository 메서드 (동일)

private:
    void load();
    void save() const;

    std::string        filePath_;
    std::vector<Order> orders_;
};
```

---

## 5. 핵심 구현 로직

### 5-1. `SampleRepository::load()`

```
1. filePath_ 가 비면 → return
2. ifstream 열기 — 파일 없으면 return (최초 실행 시 정상)
3. nlohmann::json 파싱
4. 배열 순회하며 Sample 구조체 복원 후 samples_ 에 push_back
```

### 5-2. `SampleRepository::save()`

```
1. filePath_ 가 비면 → return
2. samples_ 를 nlohmann::json 배열로 직렬화
3. ofstream 으로 json.dump(2) 기록
```

### 5-3. `OrderRepository::load()`

```
1. filePath_ 가 비면 → return
2. ifstream 열기 — 파일 없으면 return
3. nlohmann::json 파싱
4. 배열 순회:
   - status  : fromString(item["status"])
   - productionShortage  : item.value("productionShortage",  0)      // 누락 키 대비
   - productionStartedAt : item.value("productionStartedAt", "")     // 누락 키 대비
   - 나머지 필드 정상 읽기
5. orders_ 에 push_back
```

### 5-4. `main.cpp` 변경

```cpp
#include <filesystem>

// data/ 디렉토리 자동 생성
std::filesystem::create_directories("data");

SampleRepository sampleRepo("data/samples.json");   // ← filePath 전달
OrderRepository  orderRepo("data/orders.json");      // ← filePath 전달

// count() > 0 이면 내부에서 skip → 재시작 시 더미 데이터 재삽입 없음
DummyDataGenerator::populate(sampleRepo, clock);
```

---

## 6. 테스트 설계

### 6-1. `Test/PersistenceTest.cpp`

> 실제 파일 I/O 사용 (Mock 없음). 테스트마다 임시 파일 생성 후 TearDown에서 삭제.

```cpp
class PersistenceTest : public ::testing::Test {
    const std::string samplePath_ = "test_temp_samples.json";
    const std::string orderPath_  = "test_temp_orders.json";
    void TearDown() override {
        std::remove(samplePath_.c_str());
        std::remove(orderPath_.c_str());
    }
};
```

| # | 테스트 이름 | 시나리오 | 예상 결과 |
|---|------------|---------|-----------|
| T8-1 | `SampleRepo_SaveLoad_RoundTrip` | Sample 추가 → 새 Repo 로드 | count, 필드 값 일치 |
| T8-2 | `OrderRepo_SaveLoad_RoundTrip` | Order 추가 → 새 Repo 로드 | count, 필드 값 일치 |
| T8-3 | `SampleRepo_JsonKeys` | save 후 JSON 파싱 | 7개 키 모두 존재 확인 |
| T8-4 | `OrderRepo_JsonKeys` | save 후 JSON 파싱 | 9개 키 모두 존재 (`productionShortage`, `productionStartedAt` 포함) |
| T8-5 | `SampleRepo_EmptyOnNewPath` | 존재하지 않는 경로로 생성 | `count() == 0` |
| T8-6 | `OrderRepo_EmptyOnNewPath` | 존재하지 않는 경로로 생성 | `count() == 0` |
| T8-7 | `SampleRepo_UpdatePersists` | add → update(yield 변경) → 새 Repo 로드 | 변경된 yield 반영 |
| T8-8 | `OrderRepo_UpdatePersists` | add(RESERVED) → update(CONFIRMED) → 새 Repo 로드 | 상태 CONFIRMED |
| T8-9 | `OrderRepo_AllStatusRoundTrip` | 5가지 상태 각각 저장 → 로드 | fromString(toString(status)) 왕복 일치 |
| T8-10 | `SampleRepo_RemovePersists` | add 2개 → remove 1개 → 새 Repo 로드 | `count() == 1` |
| T8-11 | `OrderRepo_FifoPreservedAfterLoad` | A→B→C 순 add → 새 Repo 로드 | `findAll()` 반환 순서 A→B→C |

---

## 7. `.vcxproj` 업데이트 항목

**추가 `<ClCompile>` (Test — Release ExcludedFromBuild):**
```xml
<ClCompile Include="Test\PersistenceTest.cpp">
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
</ClCompile>
```

> `SampleRepository.cpp` / `OrderRepository.cpp` 는 기존 항목 유지 (새로 추가 불필요).

---

## 8. PRD 요구사항 매핑

| PRD 항목 | Phase 8 반영 여부 | 비고 |
|---------|-----------------|------|
| `data/samples.json` 저장 경로 | ✅ | PRD §7 |
| `data/orders.json` 저장 경로 | ✅ | PRD §7 |
| Repository 생성 시 `load()` 자동 실행 | ✅ | PRD §7 |
| 데이터 변경 후 `save()` 호출 | ✅ | PRD §7 |
| `data/` 디렉토리 없으면 자동 생성 | ✅ | PRD §7 |
| 최초 실행 시 더미 데이터 자동 생성 | ✅ | PRD §7 (DummyDataGenerator — count==0 시에만) |
| `productionShortage` / `productionStartedAt` JSON 직렬화 | ✅ | Phase 6 추가 필드 |
| TDD | ✅ | 선행 테스트 작성 (T8-1~T8-11) |

---

## 9. E2E 시나리오 검증 (직접 조작)

Phase 8 완료 후 아래 시나리오를 수동으로 수행하여 전체 흐름을 검증한다.

| # | 시나리오 | 검증 항목 |
|---|---------|-----------|
| E2E-A | 주문 → 승인(케이스 A) → 모니터링 → 출고 → 재시작 | 재시작 후 RELEASE 상태 및 감소된 reservedQty 유지 |
| E2E-B | 주문 → 승인(케이스 B) → 재시작 → 생산 완료 → 출고 → 재시작 | 재시작 후 pureQty 초과분 및 RELEASE 상태 유지 |
| E2E-C | 주문 → 거절 → 재시작 | REJECTED 상태 유지, 모니터링 집계 미포함 |
| E2E-D | A·B·C 케이스 B 순서 승인 → 재시작 → 생산라인 | 재시작 후 A→B→C FIFO 순서 보장 |
| E2E-E | 전체 재고 흐름 일관성 | `Σ pureQty + Σ reservedQty` = 초기 + 생산 완료량 - 출고량 |

---

## 10. Phase 8 완료 기준

| # | 검증 | 기준 |
|---|------|------|
| V8-1 | Debug 빌드 성공 | 경고·오류 0건 |
| V8-2 | PersistenceTest 단위 테스트 | T8-1~T8-11 (11개) 전체 통과 |
| V8-3 | 기존 단위 테스트 회귀 없음 | 전체 테스트 통과 수 유지 (기존 99 + 신규 11) |
| V8-4 | 재시작 후 데이터 유지 (직접 조작) | 앱 종료 → 재시작 → 이전 상태 동일 |
| V8-5 | 최초 실행 시 더미 데이터 생성 | `data/` 없는 상태에서 실행 → 5종 시료 자동 생성 |
| V8-6 | 재실행 시 더미 데이터 중복 없음 | 두 번째 실행 시 `count()` 동일 (DummyDataGenerator skip) |
| V8-7 | E2E-A~E 수동 검증 | 전체 비즈니스 흐름 정상 동작 |
| V8-8 | Release 빌드 성공 | 테스트 코드 제외 후 앱 빌드 정상 |
