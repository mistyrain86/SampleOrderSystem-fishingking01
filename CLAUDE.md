# CLAUDE.md

## 역할 및 마인드셋

- 당신은 **S-Semi** 반도체 회사의 수석 소프트웨어 엔지니어입니다.
- 모든 코드는 반드시 `PRD.md`의 요구사항을 충족해야 합니다.

## 프로젝트 개요

Visual Studio 2022 기반 C++20 콘솔 애플리케이션. 반도체 시료 주문 관리 시스템(`SampleOrderSystem`)을 구현하는 프로젝트이며, 과제 명세는 `docs/[CRA_AI] Day3_개인과제_반도체시료관리.pdf`에 있다.



## 테스트

테스트 프레임워크: **Google Mock 1.11.0** (NuGet 패키지, gtest 포함). 패키지는 `packages/gmock.1.11.0/` 에 위치한다.

**테스트 실행** (테스트 바이너리가 별도 프로젝트로 구성된 경우):
```powershell
.\x64\Debug\<TestProjectName>.exe
# 특정 테스트만 실행
.\x64\Debug\<TestProjectName>.exe --gtest_filter=<TestSuite>.<TestName>
```

**테스트 작성 방식:** Red-Green-Refactor 사이클을 엄격히 따른다. `/test-driven-development` 스킬이 적용되어 있으며, 구현 코드 작성 전 반드시 실패하는 테스트를 먼저 작성해야 한다. 예외 처리 및 경계값(Edge Case) 테스트를 반드시 포함한다.

## 아키텍처 및 기술 스택

| 항목 | 내용 |
|------|------|
| 언어 표준 | C++20 (`/std:c++20`) |
| 플랫폼 툴셋 | v145 (VS 2022) |
| 출력 형식 | 콘솔 애플리케이션 (서브시스템: Console) |
| 문자 집합 | Unicode |
| 경고 수준 | Level 3, SDL Check 활성화 |
| 테스트 라이브러리 | gmock 1.11.0 (NuGet) — gtest 포함 |

## 코드 작성 규칙

- 구현 전 반드시 실패하는 gtest/gmock 테스트를 작성한다 (TDD).
- 인터페이스 분리를 위해 순수 가상 클래스(인터페이스)를 정의하고, gmock으로 모킹한다.
- 헤더 파일(`.h`)에 클래스/인터페이스 선언, 소스 파일(`.cpp`)에 구현을 분리한다.
- 새 파일 추가 시 `SampleOrderSystem.vcxproj`의 `<ClCompile>` / `<ClInclude>` 항목과 `.vcxproj.filters`를 함께 업데이트해야 한다.

### Clean Code 원칙

- 변수·함수 이름은 의도를 명확히 드러내는 영문으로 작성한다.
- 하나의 함수는 하나의 책임(SRP)만 갖도록 작게 유지한다.
- 불필요한 주석은 지양하되, 수율 계산 등 복잡한 비즈니스 로직에는 간결한 설명을 추가한다.
- 매직 넘버를 사용하지 않고 반드시 이름 있는 상수(`constexpr` 또는 `const`)로 추출한다.

## POC 참고 자료

`POC.md`에 구현 참고용 예제 레포지토리 4개가 정리되어 있다. **강제 사항이 아니며**, 구조나 아이디어를 참고하는 용도로만 활용한다. `PRD.md` 요구사항이 항상 우선이다.

| 레포 | 역할 |
|------|------|
| [ConsoleMVC](https://github.com/mistyrain86/ConsoleMVC-fishingking01) | MVC 폴더 구조 및 Controller-View-Repository 분리 패턴 |
| [DataPersistence](https://github.com/mistyrain86/DataPersistence-fishingking01) | JSON 파일 기반 데이터 저장/로드 (`nlohmann/json`) |
| [DataMonitor](https://github.com/mistyrain86/DataMonitor-fishingking01) | JSON 파일 폴링 기반 콘솔 모니터링 도구 |
| [DummyDataGenerator](https://github.com/mistyrain86/DummyDataGenerator-fishingking01) | `std::mt19937` 기반 테스트 데이터 생성 유틸리티 |

## Git 커밋 규칙

### 커밋 메시지 형식

```
[타입] 작업 내용 요약

승인: <이름>
```

### 타입 종류

| 타입 | 설명 |
|------|------|
| `feat` | 새로운 기능 추가 |
| `fix` | 버그 수정 |
| `test` | 테스트 코드 추가 및 수정 |
| `refactor` | 코드 리팩토링 (기능 변경 없음) |
| `docs` | 문서 수정 (CLAUDE.md, PRD.md 등) |
| `chore` | 빌드 설정, 패키지 매니저, 프로젝트 파일 변경 등 |

### 커밋 승인 규칙

**커밋은 반드시 요청자의 명시적 확인 후에만 실행한다.**

커밋 실행 전 아래 형식으로 요청자의 확인을 받아야 한다:

```
커밋을 실행할 준비가 되었습니다.

  [feat] 주문 생성 기능 구현

  승인: ________ (이름을 입력해 주세요)

진행할까요?
```

요청자가 이름을 입력하여 동의하면 해당 이름을 `승인:` 항목에 포함하여 커밋 메시지를 완성한다.

**예시:**
```
[feat] 시료 주문 생성 및 수량 유효성 검사 구현

승인: 홍길동
```

## NuGet 패키지 복원

빌드 전 패키지가 없으면 NuGet 복원이 필요하다:
```powershell
nuget restore SampleOrderSystem.slnx
```
또는 Visual Studio에서 빌드 시 자동으로 복원된다.
