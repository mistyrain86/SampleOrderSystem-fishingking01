# POC.md

> **POC(Proof of Concept) 참고 자료입니다.**
> 아래 레포지토리들은 이 프로젝트의 구현 방향을 탐색하기 위해 미리 작성된 예제 코드입니다.
> 반드시 따라야 할 규범이 아니라, 아이디어와 구조를 참고하는 용도로 활용하세요.

---

## 1. MVC 스켈레톤 코드

**레포:** https://github.com/mistyrain86/ConsoleMVC-fishingking01

콘솔 기반 MVC 패턴의 기본 골격 구현체. 메모리 기반(in-memory) 데이터로 동작하며, 전체 폴더 구조와 Controller-View 분리 방식을 참고할 수 있다.

**참고할 구조:**
```
├── Model/
│   ├── Sample.h                  # 시료 도메인 모델
│   ├── SampleRepository.h/cpp    # 시료 데이터 접근 (Repository 패턴)
│   ├── Order.h                   # 주문 도메인 모델
│   ├── OrderRepository.h/cpp     # 주문 데이터 접근
│   └── OrderStatus.h             # 주문 상태 열거형
├── Controller/
│   ├── SampleController.h/cpp
│   ├── OrderController.h/cpp
│   ├── DashboardController.h/cpp
│   ├── ProductionController.h/cpp
│   └── ReleaseController.h/cpp
├── View/
│   ├── MainView.h/cpp
│   ├── SampleView.h/cpp
│   ├── OrderView.h/cpp
│   ├── DashboardView.h/cpp
│   ├── ProductionView.h/cpp
│   └── ReleaseView.h/cpp
└── Util/
    └── DummyDataGenerator.h/cpp
```

**핵심 아이디어:**
- 도메인(Sample, Order)별로 Controller + View + Repository를 각각 쌍으로 구성
- Repository 패턴으로 데이터 접근을 추상화하여 향후 저장소 교체 가능

---

## 2. 데이터 영속성 처리

**레포:** https://github.com/mistyrain86/DataPersistence-fishingking01

ConsoleMVC와 동일한 MVC 구조에 JSON 파일 기반 데이터 저장/로드 기능을 추가한 구현체. `nlohmann/json` 라이브러리(`json.hpp`)를 사용한다.

**참고할 포인트:**
- Repository 생성자에서 파일 경로를 받아 `load()` 자동 호출
- `save()` / `load()` 메서드로 JSON 직렬화/역직렬화 처리
- `data/samples.json`, `data/orders.json` 형태로 분리 저장
- `<filesystem>`으로 `data/` 디렉토리 자동 생성
- 최초 실행 시 더미 데이터 자동 생성 로직 포함

```cpp
// 참고 예시: Repository 생성 패턴
SampleRepository repo("data/samples.json");  // 생성 시 자동 load
repo.save();                                  // 변경 후 저장
```

---

## 3. 데이터 모니터링 Tool

**레포:** https://github.com/mistyrain86/DataMonitor-fishingking01

DataPersistence가 저장한 JSON 파일을 5초 주기로 읽어 콘솔 대시보드로 시각화하는 독립 실행형 모니터링 도구.

**참고할 포인트:**
- 메인 시스템과 **별도 프로세스**로 실행되며 JSON 파일만 공유
- 커맨드라인 인자로 데이터 경로 지정 가능 (기본값: `data/`)
- ANSI 색상 코드를 활용한 콘솔 시각화
- `MonitorController` 단일 컨트롤러 + `DashboardView`로 구성
- 5초 폴링 주기로 파일을 재읽어 변화 감지

```
실행 예시: DataMonitor.exe [data_path]
```

---

## 4. Dummy 데이터 생성 Tool

**레포:** https://github.com/mistyrain86/DummyDataGenerator-fishingking01

테스트·데모·성능 검증용 무작위 시료/주문 데이터를 생성하여 JSON으로 저장하는 독립형 유틸리티.

**참고할 포인트:**
- `std::mt19937` 난수 생성기 사용 → seed 고정 시 재현 가능한 데이터 생성
- `SampleGenerator`, `OrderGenerator`로 생성 로직 분리
- 생성된 JSON을 `data/` 경로에 저장하면 DataPersistence·DataMonitor와 바로 연동 가능
- `GeneratorView`로 생성 수량 등 사용자 입력 처리

```
실행 예시: DummyDataGenerator.exe
```

---

## 전체 POC 구성 흐름

```
[DummyDataGenerator]  →  data/*.json
                               ↓
                     [DataPersistence / 메인 앱]  ←→  data/*.json
                               ↓
                        [DataMonitor]  (5초 폴링)
```

네 레포는 독립적으로도 동작하지만, 위 흐름대로 조합하면 데이터 생성 → 관리 → 모니터링 파이프라인을 구성할 수 있다.

---

> **주의:** 이 POC 코드들은 탐색용으로 작성된 예제입니다. 실제 `SampleOrderSystem` 구현 시에는 `PRD.md` 요구사항을 우선 기준으로 삼고, 이 코드는 구조나 아이디어 참고 용도로만 활용하세요.
