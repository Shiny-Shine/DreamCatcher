# Dream Catcher

Dream Catcher는 `언리얼 엔진 5 기반 3인칭 오버숄더 서브컬쳐 슈터` 프로젝트입니다.  
짧고 밀도 높은 전투 세션 안에서 캐릭터성과 타격감, 연출 중심의 플레이 경험을 만드는 것을 목표로 합니다.

## 프로젝트 개요

- 장르: 3인칭 오버숄더 서브컬쳐 슈터
- 엔진: Unreal Engine 5
- 팀 구성: 2인
  - 개발자: 게임플레이 코드, 시스템, 아키텍처
  - 디자이너: 아트, 애니메이션, VFX/SFX, UI, 레벨 연출
- 목표 플레이 세션: 5~8분

Dream Catcher가 플레이어에게 주고 싶은 감정은 아래와 같습니다.

- 내가 그 세계 안에 들어와 있는 느낌
- 신난다
- 시원하다
- 귀엽다

## 핵심 기획 방향

### 1. 캐릭터 중심의 전투 경험

플레이어는 3인칭 오버숄더 시점에서 캐릭터를 조작하며 아래 전투 루프를 경험합니다.

- 기본 사격
- 회피
- 궁극기

중요한 것은 단순히 적을 맞추는 것이 아니라,  
`보는 재미`, `쏘는 손맛`, `캐릭터의 매력`, `읽기 쉬운 전투 연출`을 함께 만드는 것입니다.

### 2. 짧지만 완결감 있는 한 판 구조

한 번의 플레이는 아래 흐름을 기준으로 설계합니다.

1. 시작
2. 컷신
3. 전투 1
4. 전투 2
5. 컷신
6. 보스전
7. 결과

짧은 시간 안에 한 판의 기승전결을 완성하는 것이 목표입니다.

### 3. 반복 플레이를 위한 메타 루프

전투 바깥에서는 아래 구조를 계획하고 있습니다.

- 스테이지 선택
- 보상 획득
- 강화
- 장비 확장

## 참고한 게임과 차별점

Dream Catcher는 아래 작품들에서 영감을 받았습니다.

- 스노우 브레이크
- NSR
- 젠레스 존 제로

다만 Dream Catcher는 아래 방향에 더 집중합니다.

- 더 짧고 응축된 전투 세션
- 강한 오버숄더 슈팅 감각
- 소규모 팀이 빠르게 반복 개발 가능한 구조
- 대규모 콘텐츠보다 캐릭터 연출과 전투 피드백 중심의 경험

## 코어 시스템

### 전투

- 3인칭 사격
- 회피
- 궁극기 게이지 및 스킬
- 일반 적 인카운터
- 보스전

### 성장

- 스킬 업그레이드
- 장비 확장

### 메타

- 스테이지 선택
- 보상
- 강화

### UI / UX

- 체력바
- 궁극기 게이지
- 미니맵
- 인벤토리
- 결과 / 강화 UI

## 현재 프로젝트 상태

현재 프로젝트는 `전투 수직 슬라이스 / 전투 프로토타입` 단계에 있습니다.

현재 구현되었거나 일부 구현된 항목:

- 플레이어 캐릭터 기본 구조
- 체력 컴포넌트
- 전투 컴포넌트
- Enhanced Input 기반 입력 연결
- PlayerController와 HUD 바인딩 흐름
- 일반 적 캐릭터
- 적 스포너
- 인카운터 컨트롤러
- 스테이지 디렉터
- 플레이어 사망 시 레벨 재시작 흐름

다음 주요 목표:

- 전투 피드백 강화
- 보스 전용 구조 추가
- DataAsset 기반 밸런스 구조
- 결과 / 보상 / 강화 루프
- 저장 / 메타 진행 구조
- UI 확장

## 기술 방향

Dream Catcher는 아래 하이브리드 방식으로 개발합니다.

- C++: 게임 규칙, 상태, 시스템 구조
- Blueprint: 연출, 애니메이션, VFX/SFX 연결, UI, 디자이너 반복 작업
- DataAsset: 밸런스 값과 콘텐츠 정의

### 상위 아키텍처

- 전투 계층
  - Character, Health, Combat, 이후 Weapon
- 스테이지 계층
  - Spawner, Encounter, Stage 흐름, Boss 진행
- 메타 계층
  - Stage Select, Reward, Upgrade, Save 데이터
- UI 계층
  - HUD, Result, Progression, Inventory
- 데이터 계층
  - 캐릭터 / 무기 / 적 / 보스 / 스테이지 / 보상 수치

## 저장소 구조

현재 주요 디렉터리는 아래와 같습니다.

```text
Source/DreamCatcher
  DreamCatcherCharacter.*
  DreamCatcherPlayerController.*
  DreamCatcherGameMode.*
  Components/
  AI/
  Stage/
  UI/

Content/
  Blueprint/
  Input/
  DreamCatcher/

Config/
```

현재 코드베이스의 핵심 런타임 클래스:

- `DreamCatcherCharacter`
- `DCHealthComponent`
- `DCCombatComponent`
- `DreamCatcherPlayerController`
- `DreamCatcherGameMode`
- `DCPlayerHUDWidget`
- `DCEnemyCharacter`
- `DCEnemySpawner`
- `DCEncounterController`
- `DCStageDirector`

## 개발 원칙

Dream Catcher는 아래 원칙을 기준으로 개발합니다.

- 게임 규칙은 C++에 둡니다.
- 연출과 반복 수정이 필요한 부분은 Blueprint에 둡니다.
- 시스템은 책임 단위로 분리합니다.
- 먼저 플레이 가능한 수직 슬라이스를 만듭니다.
- 전투 루프가 재미있어진 뒤 보스, 데이터, 메타를 확장합니다.

소규모 팀에서는 구조를 과도하게 복잡하게 만드는 것보다,  
명확한 책임 분리와 빠른 반복이 더 중요하다고 판단하고 있습니다.

## 실행 방법

### 요구 사항

- Unreal Engine 5
- Rider 또는 Visual Studio 기반 Unreal C++ 개발 환경

### 프로젝트 열기

1. `DreamCatcher.uproject`를 엽니다.
2. 필요하면 프로젝트 파일을 갱신합니다.
3. C++ 프로젝트를 빌드합니다.
4. Unreal Editor에서 프로젝트를 실행합니다.

### 기본 프로젝트 설정

현재 프로젝트는 아래를 기준으로 동작합니다.

- 기본 시작 맵: `Level_1`
- 기본 GameMode: `BP_DreamCatcherGameMode`

에디터 설정이나 로컬 오버라이드가 다를 경우, 아래를 확인하십시오.

- `World Settings -> GameMode Override`
- 레벨 안에 `PlayerStart` 존재 여부
- Character / Controller / HUD 블루프린트 연결 상태

## 로드맵

### Milestone 1. Combat Prototype

- 플레이어 전투
- 적 전투
- HUD 기본 구조
- 회피 / 궁극기 루프

### Milestone 2. Vertical Slice

- 컷신과 전투 흐름
- 2개의 전투 인카운터
- 보스전
- 결과 화면

### Milestone 3. Meta Loop Prototype

- 스테이지 선택
- 보상 루프
- 강화 루프
- 저장 / 진행 상태 유지

### Milestone 4. Production Base

- DataAsset 기반 밸런스 구조
- 콘텐츠 폴더 정리
- 확장 가능한 보스 / 성장 구조

## 팀 작업 방향

### 개발자 중심 작업

- 게임플레이 시스템
- 아키텍처 설계
- 전투 규칙
- AI / 인카운터 흐름
- 메타 진행 로직
- 저장 시스템

### 디자이너 중심 작업

- 캐릭터 표현
- 애니메이션
- VFX / SFX
- UI 스타일
- 레벨 연출과 전투 가독성
- 컷신 연출

## 프로젝트 방향성

Dream Catcher는 처음부터 거대한 시스템 중심 슈터를 목표로 하지 않습니다.

이 프로젝트는 아래 순서를 기준으로 개발합니다.

1. 전투가 읽히고 재미있게 느껴지게 만든다
2. 짧지만 완결된 한 판을 플레이 가능하게 만든다
3. 보스전과 성장 구조를 붙인다
4. 루프가 재미있어진 뒤에 콘텐츠를 확장한다

즉, 이 저장소의 핵심은 “많은 시스템”이 아니라  
`작지만 밀도 있는 전투 경험을 정확한 구조 위에 쌓아 가는 것`입니다.
