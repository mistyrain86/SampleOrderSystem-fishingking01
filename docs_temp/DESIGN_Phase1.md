# DESIGN_Phase1.md — 프로젝트 골격 + UI 뼈대

> **Phase 1 목표:**  
> 앱이 실행되면 스플래시 화면이 뜨고, 시스템 현황 요약과 함께 메인 메뉴에서 각 View로 이동 가능한 **동작하는 껍데기**를 완성한다.  
> 이 Phase가 끝나면 사용자가 앱을 직접 실행하고 메뉴 6개를 탐색할 수 있다.

---

## 1. 목표 및 범위

### 포함 (In Scope)
- `SampleOrderSystem` 메인 프로젝트 + `SampleOrderSystemTest` 테스트 프로젝트 구성
- `Model / Controller / View / Util` 폴더 구조
- `json.hpp` 단일 헤더 추가
- `SplashView` — S-Semi 배너 + "Press Enter to Start..."
- `MainView` — 시스템 현황 요약 섹션 + 6개 메뉴 루프
- 스텁 View 6종 (`SampleView`, `OrderView`, `ApprovalView`, `MonitoringView`, `ProductionView`, `ReleaseView`)
- `main.cpp` 진입점
- Sanity 테스트 1개

### 제외 (Out of Scope)
- 실제 비즈니스 로직 및 Controller 구현
- 도메인 모델 (Sample, Order 구조체)
- 데이터 저장/로드

---

## 2. 폴더 및 파일 구조

```
SampleOrderSystem/
├── main.cpp
├── Model/                      # (빈 폴더, Phase 2에서 채워짐)
├── Controller/                 # (빈 폴더, Phase 4~에서 채워짐)
├── View/
│   ├── SplashView.h / .cpp
│   ├── MainView.h / .cpp
│   ├── SampleView.h / .cpp       # 스텁
│   ├── OrderView.h / .cpp        # 스텁  [2] 시료 주문
│   ├── ApprovalView.h / .cpp     # 스텁  [3] 주문 승인/거절
│   ├── MonitoringView.h / .cpp   # 스텁  [4] 모니터링
│   ├── ProductionView.h / .cpp   # 스텁  [5] 생산라인 조회
│   └── ReleaseView.h / .cpp      # 스텁  [6] 출고 처리
└── Util/
    └── json.hpp

SampleOrderSystemTest/
├── SampleOrderSystemTest.vcxproj
└── SanityTest.cpp
```

---

## 3. 클래스 설계

### 3-1. `SplashView`

```cpp
// View/SplashView.h
class SplashView {
public:
    void show() const;
};
```

**동작 흐름:**
1. 콘솔에 S-Semi 배너 출력
2. "Press Enter to Start..." 출력
3. `std::cin.get()` — Enter 입력 대기
4. 콘솔 화면 클리어 (`system("cls")`)

**출력 형태:**
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

---

### 3-2. `MainView`

```cpp
// View/MainView.h
class MainView {
public:
    void run();
private:
    void printHeader() const;   // 시스템 현황 요약 출력
    void printMenu() const;     // 메뉴 항목 출력
    int  readChoice() const;
    void dispatch(int choice);
};
```

**동작 흐름:**
```
loop:
  printHeader()   ← 시스템 현황 (현재는 0으로 하드코딩)
  printMenu()
  readChoice()
    → [1] SampleView::show()
    → [2] OrderView::show()
    → [3] ApprovalView::show()
    → [4] MonitoringView::show()
    → [5] ProductionView::show()
    → [6] ReleaseView::show()
    → [0] 루프 종료
    → 기타 → "잘못된 선택입니다." 재입력 유도
```

**출력 형태 (PDF p.11 기반):**
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

> **Phase 1 한정:** 시스템 현황 수치는 모두 0 하드코딩.  
> Phase 4에서 SampleController 연동 → 등록 시료/총 재고 갱신  
> Phase 5에서 OrderController 연동 → 전체 주문 수 갱신  
> Phase 6에서 ProductionController 연동 → 생산라인 대기 수 갱신

---

### 3-3. 스텁 View 6종

각 스텁 View는 단일 `show()` 메서드를 가지며 화면 헤더, "구현 예정" 메시지를 출력한 뒤 Enter 대기 후 복귀한다.

| 파일 | 클래스 | 메뉴 번호 | 완성 Phase |
|------|--------|----------|-----------|
| `SampleView` | `SampleView` | [1] 시료 관리 | Phase 4 |
| `OrderView` | `OrderView` | [2] 시료 주문 | Phase 5 |
| `ApprovalView` | `ApprovalView` | [3] 주문 승인/거절 | Phase 5 |
| `MonitoringView` | `MonitoringView` | [4] 모니터링 | Phase 7 |
| `ProductionView` | `ProductionView` | [5] 생산라인 조회 | Phase 6 |
| `ReleaseView` | `ReleaseView` | [6] 출고 처리 | Phase 6 |

**공통 스텁 패턴:**
```cpp
void XxxView::show() {
    std::cout << "\n================================================================\n";
    std::cout << "  [N] XXX\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "  구현 예정입니다. (Phase N)\n\n";
    std::cout << "  Enter를 눌러 메인 메뉴로 돌아갑니다...";
    std::cin.ignore();
    std::cin.get();
}
```

---

### 3-4. `main.cpp`

```cpp
#include <windows.h>
#include "View/SplashView.h"
#include "View/MainView.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    SplashView splash;
    splash.show();

    MainView mainView;
    mainView.run();

    return 0;
}
```

---

## 4. 테스트 설계

### 4-1. 테스트 프로젝트 구성

| 항목 | 값 |
|------|----|
| 프로젝트명 | `SampleOrderSystemTest` |
| 출력 형식 | 콘솔 애플리케이션 |
| NuGet 참조 | `gmock.1.11.0` |
| 추가 포함 경로 | `..\SampleOrderSystem` |
| 링크 대상 | gmock.lib, gtest.lib (NuGet 자동 처리) |

### 4-2. Sanity 테스트

**파일:** `SampleOrderSystemTest/SanityTest.cpp`

```cpp
#include <gtest/gtest.h>

TEST(Sanity, AlwaysPass) {
    EXPECT_EQ(1 + 1, 2);
}
```

**목적:** 테스트 프로젝트 빌드 및 gmock 링크가 정상임을 확인한다.

> **TDD 원칙 적용 근거:**  
> Phase 1은 순수 UI 뼈대(출력 전용)로 단위 테스트 대상 로직이 없다.  
> Sanity 테스트를 먼저 작성하여 "테스트가 실패 → 빌드 성공 → 테스트 통과" 사이클을 최소 충족한다.  
> 실제 비즈니스 로직 TDD는 Phase 3(Repository)부터 적용된다.

---

## 5. `.vcxproj` 업데이트 규칙

새 파일 추가 시 `SampleOrderSystem.vcxproj`와 `.vcxproj.filters`를 함께 업데이트한다.

**`.vcxproj` 추가 패턴:**
```xml
<ItemGroup>
  <ClInclude Include="View\SplashView.h" />
  <ClInclude Include="View\MainView.h" />
  <ClInclude Include="View\SampleView.h" />
  <ClInclude Include="View\OrderView.h" />
  <ClInclude Include="View\ApprovalView.h" />
  <ClInclude Include="View\MonitoringView.h" />
  <ClInclude Include="View\ProductionView.h" />
  <ClInclude Include="View\ReleaseView.h" />
</ItemGroup>
<ItemGroup>
  <ClCompile Include="View\SplashView.cpp" />
  <ClCompile Include="View\MainView.cpp" />
  <ClCompile Include="View\SampleView.cpp" />
  <ClCompile Include="View\OrderView.cpp" />
  <ClCompile Include="View\ApprovalView.cpp" />
  <ClCompile Include="View\MonitoringView.cpp" />
  <ClCompile Include="View\ProductionView.cpp" />
  <ClCompile Include="View\ReleaseView.cpp" />
  <ClCompile Include="main.cpp" />
</ItemGroup>
```

**`.vcxproj.filters` 추가 패턴:**
```xml
<Filter Include="View" />
<Filter Include="Util" />

<ClInclude Include="View\SplashView.h">
  <Filter>View</Filter>
</ClInclude>
<!-- 나머지 View 헤더들도 동일하게 -->

<None Include="Util\json.hpp">
  <Filter>Util</Filter>
</None>
```

---

## 6. PRD 요구사항 매핑

| PRD 항목 | Phase 1 반영 여부 | 비고 |
|---------|-----------------|------|
| 콘솔 애플리케이션 | ✅ | Visual Studio Console App |
| C++20 | ✅ | `LanguageStandard: stdcpp20` |
| Unicode / UTF-8 | ✅ | `SetConsoleOutputCP(CP_UTF8)` |
| [1] 시료 관리 메뉴 | 🔲 스텁 | Phase 4에서 완성 |
| [2] 시료 주문 메뉴 | 🔲 스텁 | Phase 5에서 완성 |
| [3] 주문 승인/거절 메뉴 | 🔲 스텁 | Phase 5에서 완성 |
| [4] 모니터링 메뉴 | 🔲 스텁 | Phase 7에서 완성 |
| [5] 생산라인 조회 메뉴 | 🔲 스텁 | Phase 6에서 완성 |
| [6] 출고 처리 메뉴 | 🔲 스텁 | Phase 6에서 완성 |
| 메인 메뉴 시스템 현황 요약 | 🔲 0으로 하드코딩 | Phase 4~6에서 실제 연동 |
| TDD | ✅ | Sanity 테스트 선행 작성 후 구현 |

---

## 7. Phase 1 완료 기준 (PLAN.md V1 기준)

| # | 검증 | 기준 |
|---|------|------|
| V1-1 | Debug\|x64 빌드 성공 | 경고·오류 0건 |
| V1-2 | 테스트 프로젝트 빌드 성공 | gmock 링크 오류 없음 |
| V1-3 | 더미 테스트 실행 | `[  PASSED  ] 1 test` |
| V1-4 | 앱 실행 시 스플래시 화면 출력 | 배너·"Press Enter" 표시 |
| V1-5 | Enter 후 메인 메뉴 진입 | 시스템 현황 섹션 + 6개 메뉴 항목 표시 |
| V1-6 | 각 메뉴 선택 시 스텁 화면 출력 | "구현 예정" 메시지 + 메인 메뉴 복귀 |
| V1-7 | `[0]` 선택 시 정상 종료 | 프로세스 종료 확인 |
| V1-8 | 잘못된 번호 입력 | 오류 메시지 + 재입력 유도 |
| V1-9 | 한글 깨짐 없음 | `SetConsoleOutputCP(CP_UTF8)` 적용 확인 |
