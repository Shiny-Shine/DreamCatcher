# DreamCatcher 프로젝트 통합 컨텍스트 및 Codex 작업 지침

> **리포지토리:** `Shiny-Shine/DreamCatcher`  
> **기본 브랜치:** `main`  
> **문서 역할:** 프로젝트 전체 컨텍스트, 개발 정책, 아키텍처, 구현 현황, Codex 작업 규칙  
> **최종 통합일:** 2026-07-24 KST  
> **권장 위치:** 리포지토리 루트 `/AGENTS.md`

---

## 0. 가장 먼저 읽을 내용

이 문서는 **DreamCatcher 프로젝트의 공통 기준 문서**입니다.

다음 자료를 종합해 작성했습니다.

- 현재 GitHub 리포지토리의 실제 코드와 설정
- 첨부된 `Dream Catcher 개발 최종 가이드`
- ChatGPT 프로젝트 내 세계관, 캐릭터, UI, 레벨, 개발 관련 대화
- 현재까지 확정된 게임플레이 및 시스템 결정
- Unreal Engine 협업 규칙
- UE 5.8 전환 작업
- Codex로 구현 작업을 넘기기 위한 운영 규칙

### 0.1 정보 충돌 시 우선순위

정보가 충돌하면 다음 순서를 따릅니다.

1. 사용자가 가장 최근에 명시한 요구사항
2. `docs/specs/` 아래의 기능별 명세 문서
3. 이 루트 `AGENTS.md`
4. 현재 소스 코드 및 프로젝트 설정
5. 오래된 README, 과거 문서, 이전 대화 내용

실제로 구현된 상태는 현재 소스 코드가 기준입니다.  
프로젝트가 앞으로 가야 할 방향은 이 문서와 기능별 명세가 기준입니다.

### 0.2 확정되지 않은 내용을 임의로 만들지 말 것

필요한 설계가 문서에 없을 경우:

- 큰 아키텍처를 임의로 선택하지 않습니다.
- 가장 작고 되돌리기 쉬운 구현을 선택합니다.
- 필요한 수치는 Blueprint 또는 DataAsset에서 조정할 수 있게 엽니다.
- 작업 완료 보고서에 어떤 가정을 했는지 명시합니다.

### 0.3 Unreal 바이너리 에셋 처리 원칙

다음 파일은 텍스트 코드처럼 직접 작성하거나 조작하지 않습니다.

- `.uasset`
- `.umap`
- `.ubulk`
- `.uexp`

Blueprint, UMG, Animation Blueprint, Input Action, 맵, 레벨 에셋 수정이 필요한 작업은 다음 순서로 처리합니다.

1. 필요한 C++ API와 이벤트 훅을 구현합니다.
2. Unreal Editor에서 해야 할 절차를 정확하게 작성합니다.
3. 수정해야 할 에셋 경로를 나열합니다.
4. 검증 기준을 작성합니다.

---

# 1. 프로젝트 정체성

## 1.1 프로젝트 한 줄 정의

**DreamCatcher는 짧고 밀도 높은 전투 세션과 캐릭터 연출을 중심으로 한 Unreal Engine 기반 3인칭 오버숄더 서브컬처 슈터입니다.**

기획 문구:

> 꿈과 현실의 경계에서 펼쳐지는 청량한 근미래 서브컬처 TPS 슈팅 게임.

## 1.2 팀 구성

프로젝트는 개발자와 디자이너 2명이 제작합니다.

### 개발자 담당

- C++ 게임플레이 아키텍처
- 입력 시스템
- 플레이어 상태 및 전투 규칙
- 체력, 데미지, 사망
- 적 행동 규칙
- 보스 로직
- 스포너, 인카운터, 스테이지 진행
- HUD 데이터 바인딩
- 결과, 보상, 강화, 저장 로직
- DataAsset 적용
- 빌드 및 기술 협업

### 디자이너 담당

- 캐릭터 및 배경 디자인
- 애니메이션 연결
- VFX, SFX 연결
- UI 레이아웃과 시각 디자인
- 레벨 블록아웃과 환경 배치
- 전투 가독성
- 컷신 자산
- Blueprint 연출
- 캐릭터 매력과 시각적 완성도

### 공동 결정 항목

- 무기 손맛
- 카메라 동작
- 회피 감각
- 적 공격 텔레그래프
- 전투 템포
- 보스 난이도
- 레벨 가독성
- 결과 및 보상 루프
- 최종 UI 사용성

## 1.3 핵심 감정

플레이어가 느껴야 할 핵심 감정:

- 신난다
- 시원하다
- 귀엽다
- 청량한 근미래 세계에 몰입된다
- 적 공격을 읽고 명확하게 대응할 수 있다

게임은 지나치게 어둡거나 군사적이거나 복잡한 방향으로 가지 않습니다.

## 1.4 참고 방향

프로젝트에서 참고한 주요 방향:

- **Snowbreak**: 3인칭 오버숄더 슈팅
- **Zenless Zone Zero**: 플랫하고 강한 그래픽 UI
- **No Straight Roads**: 에너지 있는 시청각 연출
- **Blue Archive**: 서브컬처 캐릭터 매력, 비율, 셀 채색

직접적인 모방이 아니라 방향 참고입니다.

---

# 2. 게임 범위

## 2.1 목표 플레이 세션

프로토타입 기준:

- 약 **5~8분**
- 짧지만 완결된 한 판
- 시작, 전개, 보스 클라이맥스, 결과가 있는 구조

## 2.2 핵심 플레이어 행동

현재 코어 액션은 의도적으로 작게 유지합니다.

- 이동
- 카메라 조작
- 기본 공격
- 조준
- 회피
- 궁극기

이 기능들의 손맛이 완성되기 전에는 대형 어빌리티 프레임워크를 추가하지 않습니다.

## 2.3 목표 스테이지 흐름

첫 수직 슬라이스 목표:

1. 시작
2. 인트로 컷신
3. 전투 1
4. 전투 2
5. 전환 컷신
6. 보스전
7. 결과

처음부터 큰 게임을 만드는 것이 목표가 아닙니다.  
짧지만 처음부터 끝까지 플레이 가능한 수직 슬라이스 하나를 완성하는 것이 우선입니다.

## 2.4 향후 메타 루프

전투 슬라이스가 안정된 이후:

1. 스테이지 선택
2. 로드아웃
3. 스테이지 플레이
4. 결과
5. 보상
6. 강화
7. 저장
8. 다음 플레이

메타 상태는 `GameMode`가 소유하지 않습니다.

---

# 3. 세계관

## 3.1 루시드 시티

주요 배경은 **루시드 시티(Lucid City)** 입니다.

꿈 기반 기술과 평범한 청춘의 일상이 공존하는 청량한 근미래 도시입니다.

시각적 특징:

- 깨끗한 도시 인프라
- 거대한 기업 또는 공공기관 건축
- 투명한 근미래 기술
- 꿈을 연상시키는 빛과 색
- 스트리트 문화
- 전투 동선이 명확한 공간
- 지나치게 어두운 사이버펑크가 아닌 밝고 청량한 분위기

## 3.2 꿈 에너지와 드림 코어

세계의 핵심 개념은 꿈을 에너지로 변환하는 기술입니다.

### 드림 코어

**드림 코어(Dream Core)** 는 꿈을 변환해 만들어지는 실용 에너지입니다.

도시의 기술과 인프라를 움직이는 중요한 에너지원입니다.

### 악몽 부산물

꿈을 에너지로 변환하는 과정에서는 위험한 부산물이 발생합니다.

이 부산물은:

- 악몽과 관련된 오염으로 나타납니다.
- 침식 또는 변질을 일으킵니다.
- 공간, 물체, 신호, 생명체에 영향을 줄 수 있습니다.
- 전문 처리 조직이 필요한 사건을 만듭니다.

정확한 과학 원리, 공식 명칭, 정치적 의미는 아직 완전히 확정되지 않았습니다.  
별도 스토리 명세 없이 세부 설정을 임의로 확정하지 않습니다.

## 3.3 드림캐쳐 조직

DreamCatcher는 악몽 사건을 처리하는 중심 조직 또는 대응 부대입니다.

조직의 분위기:

- 공식적인 대형 기관
- 기업과 공공기관이 결합된 인상
- 악몽 처리 전문 조직
- 높은 위치의 간부와 젊은 현장 요원이 공존하는 구조

악몽처리반의 최종 공식 영문 부제는 아직 확정되지 않았습니다.

## 3.4 환경 스토리텔링

배경은 다음 내용을 전달해야 합니다.

- 꿈 기술로 움직이는 도시
- 겉으로는 정상적인 일상
- 그 아래 존재하는 악몽 위험
- 드림캐쳐 조직의 강한 존재감
- 전투 동선의 명확성
- 대규모 침식 사건의 흔적

모든 면에 세부 묘사를 가득 채우지 않습니다.

- 중심부는 대비, 디테일, 빛을 강하게 합니다.
- 주변부는 대비와 묘사를 줄입니다.
- 플레이어의 시선이 필요한 곳에 집중되게 합니다.

## 3.5 BIG BROTHER 보스 콘셉트

논의된 주요 보스 콘셉트:

**BIG BROTHER**

작업 문구:

> 수많은 신호 속에서 깨어난 관측자.

핵심 요소:

- 감시
- 관측
- 정보 노이즈
- 신호 왜곡
- 기계 눈
- 거대한 몸체 또는 머리
- 마천루 옥상 보스전
- 원형 판옵티콘형 스테이지
- 건물 위에서는 머리나 상체 일부만 보일 정도의 압도적 크기

단순한 CCTV 머리 로봇처럼 보이지 않도록 합니다.

- 분산된 눈
- 신호 구조
- 정보 왜곡
- 건축물과 연결된 규모
- 도시 전체를 감시하는 인상

별도 보스 명세가 나오기 전까지는 콘셉트 방향입니다.

---

# 4. 아트 방향

## 4.1 전체 톤

핵심 키워드:

- 청량함
- 청춘
- 근미래
- 서브컬처
- 스트리트 테크웨어
- 셀 채색
- 투명감
- 그래픽 대비
- 꿈
- 별
- 원형
- 신호
- 노이즈

게임은 지나치게 무겁고 진지한 분위기보다 활기 있고 접근하기 쉬운 방향을 유지합니다.

## 4.2 캐릭터 렌더링

선호 방향:

- 애니메이션 스타일
- 읽기 쉬운 실루엣
- 과도하지 않은 디테일
- 정돈된 셀 채색
- 실사보다 단순한 재질 표현
- 얼굴과 표정이 잘 보이는 구성
- 깨끗한 선과 형태
- 캐릭터 색이 배경 조명과 자연스럽게 섞이는 표현

콘셉트 아트 제작 원칙:

- 메인 캐릭터 비중을 키웁니다.
- 주변 대비를 줄입니다.
- 포즈를 배경과 자연스럽게 연결합니다.
- 과도한 회화적 노이즈를 피합니다.
- 서브컬처 게임 이미지처럼 읽기 쉬워야 합니다.

## 4.3 의상 방향

기본 방향:

- 스트리트 테크웨어
- 제복 요소
- 현실 군복처럼 무겁지 않은 전술 포인트
- 비대칭
- 스트랩, 홀스터, 장비를 디자인 포인트로 사용
- 흰색, 파란색, 보라색, 분홍색
- 제한적으로 어두운 색 사용

## 4.4 그래픽 모티프

반복적으로 사용할 요소:

- 드림캐쳐를 연상시키는 원과 방사형 구조
- 별
- 링
- 신호선
- 투명 패널
- 절제된 글리치 또는 노이즈
- 기관형 로고 시스템

실제 드림캐쳐 장식을 모든 곳에 붙이지 않습니다.  
상징은 간접적으로 사용합니다.

## 4.5 브랜드 방향

DreamCatcher 로고와 브랜딩은 다음을 지원해야 합니다.

- 공식 기관 또는 종합 기업 같은 신뢰감
- 깨끗한 타이포그래피
- 조직 및 부대 아이덴티티
- 청량한 청춘 분위기
- 공식성과 서브컬처 감성의 균형

굿즈와 기획서 페이지도 지나치게 어둡고 무겁게 만들지 않습니다.

---

# 5. 캐릭터 방향

캐릭터 이름, 최종 배경, MBTI는 별도 문서가 없으면 확정 상태가 아닙니다.

## 5.1 메인 캐릭터 A

성격 방향:

- 밝다
- 쾌활하다
- 에너지가 넘친다
- 행동이 빠르다
- 쉽게 주눅 들지 않는다
- 팀 분위기를 자연스럽게 끌어올린다
- 낙천적이다
- 팀의 추진력과 분위기 메이커 역할

밝은 성격이라는 이유로 무능하거나 단순한 캐릭터로 만들지 않습니다.

## 5.2 캐릭터 B

성격 방향:

- 겉으로는 시크하고 절제되어 있다
- 실제로는 상냥하고 배려심이 있다
- 감정 표현이 크지 않다
- 믿을 수 있다
- 날카로운 인상과 부드러운 행동의 대비가 매력

공식 설명에서는 “갭모에”라는 단어를 직접 쓰지 않습니다.

## 5.3 캐릭터 C

성격 방향:

- 나른하다
- 느긋하다
- 귀찮아하는 말투를 사용한다
- 의욕이 없어 보인다
- 실제 능력은 월등하다
- 필요할 때 매우 신뢰할 수 있다

단순히 게으른 개그 캐릭터로 소비하지 않습니다.

## 5.4 높으신 분 / 리더 캐릭터

일반 현장 팀원이 아니라 조직 내 높은 위치의 인물입니다.

성격 및 디자인 방향:

- 높은 직위
- 조용하고 신비로운 분위기
- 표정 변화가 적다
- 존댓말 사용
- 팀 전체를 깊이 이해한다
- 조직의 방향을 잡는다
- 성숙하고 안정된 존재감
- 매우 긴 생머리
- 정복 또는 의전복
- 흰색 베이스
- 보라·분홍 포인트
- 드림캐쳐를 은유한 머리 장식 또는 액세서리

기본적으로 악역처럼 보이게 만들지 않습니다.

---

# 6. 게임 모드와 화면 구조

## 6.1 스토리 / 기본 전투 모드

포함 요소:

- 전투 HUD
- 대화 및 스토리 UI
- 적 인카운터
- 스테이지 진행
- 보스전
- 결과 전환

## 6.2 프리 모드

기존 게임 화면을 재사용하되 전투 압박 요소를 줄입니다.

예상 차이:

- 적 표시 제거
- 보스 UI 제거
- 인카운터 진행 압박 제거
- 전투 전용 안내 최소화
- 이동과 배경 감상에 필요한 플레이어 상태 UI 유지

## 6.3 무한 모드

기본 전투 HUD에 다음 요소를 추가합니다.

- 현재 페이즈 또는 웨이브
- 점수
- 진행 난이도 상승
- 명확한 실패 및 결과 상태

별도의 완전히 다른 UI 언어를 만들지 않고 기존 전투 HUD를 확장합니다.

## 6.4 설정 UI

프로토타입 범위는 사운드 설정 중심입니다.

- 전체 음량
- 배경음
- 효과음
- 음성이 있을 경우 음성 볼륨

명확한 필요가 없으면 설정 메뉴 범위를 크게 늘리지 않습니다.

## 6.5 공통 UI

공통 기능:

- 확인
- 취소 / 뒤로
- 일시정지
- 알림
- 로딩
- 화면 전환
- 버튼 안내
- 모달 패널

모든 공통 UI는 하나의 그래픽 규칙을 공유해야 합니다.

---

# 7. UI / UX 방향

## 7.1 기준 해상도

UI 디자인 기준:

- **1920 × 1080**
- 16:9

절대 좌표만 사용하지 않고 Safe Margin과 해상도 스케일링을 고려합니다.

## 7.2 전투 HUD 기능 그룹

### 플레이어 상태

- 체력
- 궁극기 게이지
- 필요 시 상태 이상
- 조준 상태
- 크로스헤어

### 무기 상태

- 주무기를 크게 강조
- 보조무기는 낮은 우선순위
- 탄약 또는 발사 상태
- 현재 무기 정보

### 스킬

- 스킬 버튼
- 쿨다운
- 궁극기 상태
- 활성/비활성 상태 구분

### 적과 인카운터

- 적 표시
- 보스 체력
- 인카운터 또는 페이즈 정보
- 무한 모드 웨이브와 점수

### 진행 및 피드백

- 크로스헤어
- 히트마커
- 데미지 숫자
- 목표 또는 진행 알림
- 클리어 / 실패 화면 전환

## 7.3 대화 UI

대화 UI는 작은 전투 툴팁이 아니라 독립적인 전체 화면 연출로 취급합니다.

가능한 요소:

- 캐릭터 초상 또는 전신
- 화자 이름
- 대사
- 다음 / 스킵 / 자동 진행
- 배경 가독성 처리
- 게임 화면 복귀 전환

## 7.4 시각 스타일

UI 디자인 방향:

- 플랫한 그래픽 아이콘
- 반투명 패널
- 기획서 및 UI 쇼케이스에서는 밝은 배경
- 명확하고 절제된 포인트 색
- 강한 정보 위계
- 둥근 형태와 각진 형태를 의도적으로 혼합
- 꿈, 원, 별 모티프
- 불필요한 장식 픽토그램 최소화
- 타이틀과 본문 가독성 우선

## 7.5 UI 구현 원칙

UI는 상태를 표시하고 입력을 전달합니다.  
핵심 게임 규칙을 계산하지 않습니다.

UMG Blueprint 안에 넣지 말아야 할 것:

- 보상 계산
- 강화 계산
- 데미지 규칙
- 스테이지 진행 판정
- 저장 규칙

---

# 8. 현재 리포지토리 상태

## 8.1 리포지토리

- 리포지토리: `Shiny-Shine/DreamCatcher`
- 공개 리포지토리
- 기본 브랜치: `main`
- 마지막 확인 커밋: `11c8aeab7f84b306dce24729547edfc4fc448f92`
- 마지막 확인 날짜: 2026-07-22
- 마지막 확인 변경 파일: `Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap`
- 최근 작업은 Level 1 환경과 블록아웃에 집중되어 있습니다.

## 8.2 엔진 상태

현재 GitHub `main`의 `.uproject`는 다음 값을 사용합니다.

```json
"EngineAssociation": "5.7"
```

개발자 로컬에서는 **Unreal Engine 5.8 전환 작업**이 진행 중입니다.

다음을 구분해야 합니다.

- GitHub `main`은 아직 UE 5.7 상태일 수 있습니다.
- 로컬 작업본은 이미 UE 5.8일 수 있습니다.
- 엔진 전환은 별도 브랜치와 별도 커밋으로 처리해야 합니다.
- 디자이너용 바이너리는 정확히 같은 엔진 버전과 Build ID여야 합니다.

## 8.3 UE 5.8 Target 설정

현재 확인된 Target 파일은 다음 설정입니다.

```csharp
DefaultBuildSettings = BuildSettingsVersion.V6;
IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
```

UE 5.8로 전환할 때 두 파일을 수정합니다.

- `Source/DreamCatcher.Target.cs`
- `Source/DreamCatcherEditor.Target.cs`

권장 값:

```csharp
DefaultBuildSettings = BuildSettingsVersion.V7;
IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
```

정상적인 마이그레이션 문제 해결을 위해 다음 설정을 먼저 사용하지 않습니다.

```csharp
BuildEnvironment = TargetBuildEnvironment.Unique;
bOverrideBuildEnvironment = true;
```

이 설정은 일반적인 버전 불일치를 해결하기보다 공유 빌드 환경을 분리하거나 검사를 우회합니다.

## 8.4 Windows 빌드 필수 구성

UE 5.8 C++ 개발 환경:

- Visual Studio 2022 또는 Visual Studio Build Tools
- C++ 게임 개발 구성 요소
- MSVC x64/x86 빌드 도구
- Windows SDK
- `.NET Framework 4.8 SDK`
- `.NET Framework 4.8 Targeting Pack`

UnrealBuildTool 실행에 사용되는 최신 .NET과 `SwarmInterface`가 요구하는 `.NET Framework SDK`는 다른 구성 요소입니다.

---

# 9. 현재 소스 아키텍처

## 9.1 핵심 원칙

- **규칙과 상태:** C++
- **연출과 에셋 연결:** Blueprint
- **밸런스와 콘텐츠 값:** 점진적으로 DataAsset

## 9.2 플레이어 계층

### `ADreamCatcherCharacter`

현재 역할:

- 플레이어 본체
- 입력 수신
- 이동
- 카메라
- 오버숄더 시점
- 조준점 계산
- Health / Combat 이벤트 연결
- Blueprint 연출 훅 제공

현재 동작:

- 카메라 기준 이동
- 컨트롤러 Yaw를 따르는 슈터형 회전
- Spring Arm과 Follow Camera
- 기본 공격 입력
- 회피 입력
- 궁극기 입력
- 카메라 Line Trace 조준점 계산
- 총구 소켓 위치 계산
- 사망 시 입력과 이동 비활성화

관련 없는 시스템을 계속 Character에 추가하지 않습니다.

### `UDCHealthComponent`

현재 역할:

- 최대 체력
- 현재 체력
- 데미지
- 회복
- 리셋
- 체력 변경 이벤트
- 사망 이벤트

연출과 분리된 상태를 유지합니다.

### `UDCCombatComponent`

현재 역할:

- 자동 발사 타이머
- 회피 쿨다운
- 궁극기 게이지
- 궁극기 사용 가능 여부
- 전투 액션 요청 이벤트

향후 적절한 역할:

- 조준 상태
- 전투 액션 제한
- 발사 상태 규칙
- `WeaponComponent` 도입 전까지 무기 요청 전달

이 컴포넌트에서 직접 VFX를 생성하거나 캐릭터 애니메이션을 재생하지 않습니다.

## 9.3 컨트롤러와 HUD

### `ADreamCatcherPlayerController`

현재 역할:

- 로컬 입력 매핑 컨텍스트
- 입력 모드
- HUD 생성
- 현재 Pawn과 HUD 연결

### `UDCPlayerHUDWidget`

현재 역할:

- Character 관찰
- HealthComponent 구독
- CombatComponent 구독
- 컴포넌트 이벤트를 Blueprint UI 이벤트로 전달
- Pawn 교체 또는 파괴 시 바인딩 해제

현재 이벤트 기반 구조를 유지합니다.

## 9.4 게임 규칙

### `ADreamCatcherGameMode`

현재 역할:

- 플레이어 사망 이벤트 연결
- 일정 시간 후 현재 레벨 재시작

결과, 보상, 메타 상태를 모두 GameMode에 넣지 않습니다.

## 9.5 적 계층

### `ADCEnemyCharacter`

현재 동작:

- 기본 AIController
- 플레이어 타깃 탐색
- 플레이어 쪽 이동
- 사거리 근처에서 정지
- 타깃을 바라봄
- Line Trace 공격
- 데미지 적용
- Blueprint 공격 / 피격 / 사망 이벤트
- 일정 시간 후 파괴

현재 AI는 의도적으로 프로토타입 수준입니다.

### 현재 적 공격 문제

현재 공격 함수 안에서 데미지 처리와 연출 이벤트가 거의 동시에 실행됩니다.

공격 텔레그래프를 만들기 위해 다음 단계로 분리해야 합니다.

1. 공격 의도
2. 선딜
3. 공격 확정
4. 명중 판정
5. 후딜

## 9.6 인카운터 계층

### `ADCEnemySpawner`

현재 역할:

- 적 클래스
- 총 스폰 수
- 최대 생존 수
- 스폰 간격
- 사망 추적
- 완료 이벤트

### `ADCEncounterController`

현재 역할:

- 여러 스포너 활성화
- 남은 스포너 추적
- 인카운터 완료 이벤트

### `ADCStageDirector`

현재 페이즈:

- `Delay`
- `Encounter`
- `BossEncounter`
- `Complete`

현재 역할:

- 페이즈 순차 실행
- 인카운터 이벤트 바인딩
- 완료까지 진행
- Blueprint 스테이지 및 페이즈 이벤트

향후 필요에 따라 추가 가능한 페이즈:

- `Sequence`
- `Reward`
- `Result`

수직 슬라이스에서 실제 필요가 생기기 전에는 불필요하게 확장하지 않습니다.

---

# 10. 확인된 현재 파일 구조

## 10.1 코어 소스

```text
Source/DreamCatcher/
  DreamCatcherCharacter.h
  DreamCatcherCharacter.cpp
  DreamCatcherPlayerController.h
  DreamCatcherPlayerController.cpp
  DreamCatcherGameMode.h
  DreamCatcherGameMode.cpp
  DreamCatcher.Build.cs
```

## 10.2 컴포넌트

```text
Source/DreamCatcher/Components/
  DCHealthComponent.h
  DCHealthComponent.cpp
  DCCombatComponent.h
  DCCombatComponent.cpp
```

## 10.3 AI

```text
Source/DreamCatcher/AI/
  DCEnemyCharacter.h
  DCEnemyCharacter.cpp
  DCEnemySpawner.h
  DCEnemySpawner.cpp
```

## 10.4 스테이지

```text
Source/DreamCatcher/Stage/
  DCEncounterController.h
  DCEncounterController.cpp
  DCStageDirector.h
  DCStageDirector.cpp
```

## 10.5 UI

```text
Source/DreamCatcher/UI/
  DCPlayerHUDWidget.h
  DCPlayerHUDWidget.cpp
```

## 10.6 주요 Blueprint 에셋

```text
Content/Blueprint/
  BP_DreamCatcherGameMode.uasset
  BP_DreamCatcherPlayerController.uasset
  BP_DreamCatcherChracter.uasset

Content/Blueprint/UI/
  WBP_PlayerHUD.uasset

Content/Blueprint/AI/
  BP_EnemyCharacter.uasset
  BP_EnemySpawner.uasset

Content/Blueprint/Stage/
  BP_EncounterController.uasset
  BP_StageDirector.uasset
```

확인된 오탈자:

```text
BP_DreamCatcherChracter
```

Unreal Editor에서 참조와 Redirector를 확인하지 않고 임의로 이름을 변경하지 않습니다.

## 10.7 입력 에셋

```text
Content/Input/
  IMC_Player.uasset

Content/Input/Actions/
  IA_Move.uasset
  IA_Look.uasset
  IA_Fire.uasset
  IA_Dodge.uasset
  IA_Ultimate.uasset
```

추가 예정:

```text
IA_Aim.uasset
```

## 10.8 맵

```text
Content/DreamCatcher/DreamCatcher_ENV/Map/Main/
  Level_1.umap

Content/DreamCatcher/DreamCatcher_ENV/Map/Test_Map/
  Test_Map.umap
```

## 10.9 프로젝트 설정

```text
Config/DefaultGame.ini
Config/DefaultEngine.ini
DreamCatcher.uproject
Source/DreamCatcher.Target.cs
Source/DreamCatcherEditor.Target.cs
```

---

# 11. 현재 구현 상태

## 11.1 구현됨

- 플레이어 Character 기반
- 오버숄더 카메라
- 카메라 기준 이동
- HealthComponent
- CombatComponent
- 자동 발사 요청 타이밍
- 회피 쿨다운
- 궁극기 게이지 상태
- Enhanced Input
- PlayerController 입력 매핑
- HUD 생성
- 이벤트 기반 HUD 연결
- 플레이어 사망 및 레벨 재시작
- 기본 적 Character
- 적 이동 및 공격
- EnemySpawner
- EncounterController
- StageDirector
- Level 1 환경 및 블록아웃 작업
- Unreal 에셋 Git LFS
- 디자이너용 Win64 Editor 바이너리 동기화 구조

## 11.2 부분 구현

- 플레이어 공격 연출
- 실제 발사체 또는 명중 방식
- 플레이어 피격 피드백
- 적 공격 연출
- 적 공격 텔레그래프
- 궁극기 실제 효과
- 궁극기 게이지 획득 방식
- HUD 시각 완성도
- 실제 레벨 인카운터 배치
- 전체 스테이지 진행
- 컷신 연결

## 11.3 미구현 또는 확인되지 않음

- 전용 BossCharacter
- 보스 페이즈 컴포넌트
- 보스 패턴 시스템
- 보스 HUD
- 결과 UI 및 결과 상태
- 보상 시스템
- 강화 시스템
- 인벤토리
- SaveGame
- GameInstance 또는 MetaSubsystem
- DataAsset 기반 밸런스
- 스테이지 선택
- 미니맵
- 정식 WeaponComponent
- WeaponData
- HitReactionComponent
- 견착 / 스코프 조준 시스템
- 회피 무적 프레임
- 최종 데미지 숫자 디자인
- 완성된 대화 흐름
- 프리 모드
- 무한 모드

---

# 12. 확정된 조준 시스템 명세

이 항목은 현재 가장 최근에 확정된 우클릭 조작 규칙입니다.

## 12.1 입력 방식

우클릭 하나를 사용합니다.

### 짧게 클릭

- 기준 시간 전에 눌렀다 뗍니다.
- Scope 조준을 켜거나 끕니다.

### 길게 누르기

- 기준 시간을 넘길 때까지 유지합니다.
- Shoulder Aim으로 진입합니다.
- 버튼을 누르고 있는 동안 유지합니다.
- 버튼을 떼면 Shoulder Aim을 종료합니다.

### 초기 기준 시간

```text
0.18초
```

테스트 범위:

```text
0.16~0.22초
```

## 12.2 상태 우선순위

두 개의 의도 상태를 별도로 저장합니다.

```cpp
bool bScopeToggled;
bool bShoulderHeld;
```

최종 조준 모드 우선순위:

1. `bShoulderHeld`가 참이면 Shoulder
2. 그렇지 않고 `bScopeToggled`가 참이면 Scope
3. 둘 다 아니면 Hip

권장 enum:

```cpp
UENUM(BlueprintType)
enum class EDCAimMode : uint8
{
    Hip,
    Shoulder,
    Scope
};
```

## 12.3 상태 전이

```text
Hip
  짧은 클릭 -> Scope
  길게 누름 -> Shoulder
  길게 누른 뒤 해제 -> Hip

Scope
  짧은 클릭 -> Hip
  길게 누름 -> Shoulder
  길게 누른 뒤 해제 -> Scope
```

Scope가 켜진 상태에서 Shoulder가 일시적으로 우선 적용됩니다.  
Shoulder 해제 후 이전 Scope 상태로 돌아갑니다.

## 12.4 강제 해제 조건

다음 행동은 모든 조준 상태를 해제합니다.

- 회피
- 궁극기
- 사망
- Possession 해제
- 입력 취소
- 향후 전투 입력을 막는 컷신 상태

별도 기능 명세에서 정책을 바꿀 수 있지만 의도적으로 결정해야 합니다.

## 12.5 책임 분리

### Character

- `IA_Aim` 입력 수신
- Hold Timer 시작 및 정리
- 짧은 클릭과 길게 누르기 판정
- 목표 카메라 프로필 관리
- 조준 감도 배율 적용

### CombatComponent

- 조준 의도와 최종 조준 상태 저장
- 조준 가능 여부 판정
- `OnAimModeChanged` 이벤트
- 현재 조준 모드 제공
- 회피, 궁극기 등 충돌 액션에서 조준 해제
- 향후 조준 모드별 탄 퍼짐 제공

### Blueprint / HUD / AnimBP

- 카메라 연출
- 견착 애니메이션
- Aim Offset
- 스코프 오버레이
- 크로스헤어 변화
- 사운드
- 화면 전환 효과

## 12.6 초기 카메라 값

초기 테스트값이며 최종 밸런스가 아닙니다.

| 상태 | FOV | Spring Arm | Socket Y | 감도 |
|---|---:|---:|---:|---:|
| Hip | 90 | 325 | 55 | 1.0 |
| Shoulder | 72 | 235 | 80 | 0.7 |
| Scope | 40 | 210 | 70 | 0.35 |

## 12.7 Scope 구현 방식

첫 수직 슬라이스에서는 다음 방식으로 구현합니다.

- 기존 플레이어 카메라 사용
- FOV 축소
- 감도 감소
- 스코프 마스크 / 오버레이
- 크로스헤어 전환

첫 구현부터 SceneCapture2D 방식 스코프를 만들지 않습니다.

이유:

- 정식 무기 시스템이 아직 없습니다.
- 렌더링 설정이 이미 무겁습니다.
- 첫 프로토타입에서 불필요한 성능 비용과 복잡도가 발생합니다.

## 12.8 Codex 수정 범위

예상 C++ 파일:

```text
Source/DreamCatcher/DreamCatcherCharacter.h
Source/DreamCatcher/DreamCatcherCharacter.cpp
Source/DreamCatcher/Components/DCCombatComponent.h
Source/DreamCatcher/Components/DCCombatComponent.cpp
Source/DreamCatcher/UI/DCPlayerHUDWidget.h
Source/DreamCatcher/UI/DCPlayerHUDWidget.cpp
```

수동 Unreal Editor 작업:

```text
Content/Input/Actions/IA_Aim.uasset
Content/Input/IMC_Player.uasset
Content/Blueprint/BP_DreamCatcherChracter.uasset
Content/Blueprint/UI/WBP_PlayerHUD.uasset
Animation Blueprint 에셋
```

---

# 13. 향후 권장 아키텍처

## 13.1 전투 계층

현재:

- `ADreamCatcherCharacter`
- `UDCHealthComponent`
- `UDCCombatComponent`

향후:

- `UDCWeaponComponent`
- `UDCHitReactionComponent`
- `IDCDamageable`

원칙:

- 상태, 쿨다운, 판정은 C++
- 애니메이션, 카메라 쉐이크, VFX, SFX는 Blueprint
- 반복 조정이 시작된 수치는 DataAsset

## 13.2 무기 계층

향후 `UDCWeaponComponent` 역할:

- 현재 장착 무기 데이터
- 발사 방식
- 발사 간격
- 탄 퍼짐
- 반동
- Projectile / Hitscan 선택
- 탄약과 재장전
- 무기 교체 상태

Character에 남길 역할:

- 입력
- 카메라 기준
- 조준 원점과 방향
- 무기 컴포넌트로 요청 전달

향후 `UDCWeaponData` 후보 값:

- 발사 모드
- 데미지
- 발사 간격
- Hip 탄 퍼짐
- Shoulder 탄 퍼짐
- Scope 탄 퍼짐
- Shoulder FOV
- Scope FOV
- 조준 중 이동 속도
- 조준 감도
- 전환 속도
- 스코프 UI 클래스

## 13.3 보스 계층

권장 구조:

- `ADCBossCharacter`
- `UDCBossPhaseComponent`
- `UDCBossData`
- `WBP_BossHUD`

첫 보스 최소 요구:

- 전용 체력 UI
- 구분되는 공격 패턴 2개
- 명확한 텔레그래프
- 체력 기준 페이즈 전환 1회
- 사망 시 스테이지 완료 신호

일반 적의 체력과 데미지만 높인 형태로 만들지 않습니다.

## 13.4 스테이지 계층

- GameMode: 레벨 규칙
- StageDirector: 순서
- EncounterController: 한 전투 구간
- EnemySpawner: 적 생성
- Sequence: 컷신
- Result: 플레이 종료

## 13.5 메타 계층

향후:

- `UDreamCatcherGameInstance`
- `UDCSaveGame`
- `UDCMetaSubsystem`
- Reward Service
- Upgrade Service
- StageRunResult 구조체

영구 상태는 GameMode가 아니라 이 계층에서 관리합니다.

## 13.6 데이터 계층

DataAsset 적용 순서:

1. 적 데이터
2. 무기 데이터
3. 플레이어 데이터
4. 보스 데이터
5. 스테이지 데이터
6. 보상 및 강화 데이터

실제 반복 수정이 시작된 데이터부터 옮깁니다.  
수직 슬라이스가 플레이 가능하기 전에 대규모 DataAsset 전환을 하지 않습니다.

---

# 14. 개발 로드맵

## 마일스톤 1 — 전투 프로토타입

목표:

- 읽기 쉽고 반응성 좋은 사격
- 의미 있는 회피
- 작동하는 궁극기 루프
- 기본 적 전투
- 기능적인 HUD

남은 핵심 작업:

- 총구 화염
- 트레이서
- 피격 이펙트
- 피격 사운드
- 히트마커
- 카메라 반응
- 적 선딜
- 회피 무적
- 궁극기 게이지 획득
- 조준 시스템

## 마일스톤 2 — 수직 슬라이스

목표:

- 인트로
- 전투 1
- 전투 2
- 전환
- 보스
- 결과

완료 기준:

> 개발자 개입 없이 게임 시작부터 결과 화면까지 도달할 수 있다.

## 마일스톤 3 — 보스전

목표:

- 전용 보스
- 페이즈 전환
- 패턴 시스템
- 보스 HUD
- 읽기 쉬운 텔레그래프
- 보스룸 연출

## 마일스톤 4 — 데이터 기반

목표:

- 자주 수정하는 수치를 DataAsset로 이동
- 디자이너가 C++ 수정 없이 밸런스 조정
- 기존 기본값 안전하게 이전

## 마일스톤 5 — 메타 루프

목표:

- 결과
- 보상
- 강화
- 저장
- 다음 판 반영

## 마일스톤 6 — 제작 기반 및 폴리싱

목표:

- 샘플 의존 축소
- 콘텐츠 구조 정리
- 아트 교체 가능한 구조
- 최종 UI 언어
- VFX / SFX 리듬
- 캐릭터 매력
- 성능 검증

---

# 15. 현재 우선순위

현재 권장 순서:

1. 전투 피드백
2. 조준 시스템
3. 적 공격 텔레그래프 분리
4. 회피 무적 및 피드백
5. Level 1 인카운터 안정화
6. 전체 스테이지 흐름
7. 최소 보스
8. 결과 화면
9. DataAsset 전환
10. 메타 루프

수직 슬라이스 전에는 다음을 우선하지 않습니다.

- 미니맵
- 대형 인벤토리
- GAS
- 멀티플레이
- 복잡한 AI 프레임워크

---

# 16. 기술 부채와 위험

## 16.1 Variant 샘플 코드

`DreamCatcher.Build.cs`에 다음 관련 경로와 의존성이 남아 있습니다.

- `Variant_Combat`
- `Variant_Platforming`
- `Variant_SideScrolling`
- `StateTreeModule`
- `GameplayStateTreeModule`

영향:

- 빌드 범위 증가
- 실제 프로젝트 구조와 샘플 구조 혼동
- 유지보수 어려움

정책:

- 실제 참조 여부를 확인하기 전에는 무작정 삭제하지 않습니다.
- 새 DreamCatcher 시스템을 Variant 구조 위에 만들지 않습니다.
- 프로젝트 구조가 안정된 뒤 별도 작업으로 제거합니다.

## 16.2 Content 루트 불일치

현재 프로젝트 콘텐츠가 여러 루트에 나뉘어 있습니다.

```text
Content/Blueprint/
Content/DreamCatcher/
Content/Input/
```

장기 목표:

```text
Content/DreamCatcher/
  Blueprints/
  Data/
  Input/
  Maps/
  Animations/
  FX/
  Audio/
  Sequences/
  Placeholder/
```

개발 중에는 대규모 에셋 이동을 하지 않습니다.

## 16.3 Blueprint 이름 오탈자

```text
BP_DreamCatcherChracter
```

Unreal Editor에서만 이름을 수정하고 Redirector와 참조를 검증합니다.

## 16.4 렌더링 비용

현재 프로젝트 설정에 포함된 고비용 기능:

- Lumen Hardware Ray Tracing
- Ray Tracing
- Path Tracing
- Nanite
- Substrate

가능한 영향:

- 셰이더 컴파일 증가
- 반복 작업 속도 저하
- 팀원 PC 성능 차이
- SceneCapture 기반 기능 추가 비용

작업 속도가 지나치게 느려지면 개발용 렌더링 프로파일을 고려합니다.

## 16.5 회피 제한

현재 회피는 `LaunchCharacter`와 쿨다운 중심입니다.

확인되지 않은 것:

- 무적 프레임
- 데미지 무시
- 성공 피드백
- 애니메이션 기반 이동

의미 있는 회피를 위해 실제 방어 효과가 필요합니다.

## 16.6 적 텔레그래프 제한

현재 적 공격은 공격 함수에서 바로 Line Trace와 데미지를 처리합니다.

목표 구조:

```text
공격 의도 -> 선딜 -> 확정 -> 명중 -> 후딜
```

## 16.7 `.gitignore` 위험

확인된 `.gitignore`에는 다음 규칙이 있습니다.

```text
/Content/Characters/
```

이미 추적된 파일은 유지되지만 새 캐릭터 에셋이 Git에 나타나지 않을 수 있습니다.

생산용 캐릭터 에셋을 해당 경로에 추가하기 전 규칙을 검토합니다.

## 16.8 IDE 메타데이터

`.idea` 개인 작업 파일이 리포지토리에 포함된 이력이 있습니다.

별도 정리 작업에서:

- 적절한 ignore 규칙 추가
- 이미 추적된 개인 workspace 파일 제거

## 16.9 main 직접 커밋

현재 PR과 Issue 기반 작업 흐름이 약합니다.

위험:

- 바이너리 충돌
- 리뷰 부족
- 엔진 전환과 기능 변경 혼합
- 회귀 확인 어려움

가벼운 브랜치와 PR 흐름을 도입합니다.

---

# 17. Git 및 협업 규칙

## 17.1 브랜치 규칙

Codex 작업을 `main`에 직접 커밋하지 않습니다.

예시:

```text
feature/aim-system
feature/enemy-telegraph
feature/boss-foundation
fix/ue58-target-settings
docs/project-context
```

한 브랜치에는 하나의 기능 또는 하나의 마이그레이션만 포함합니다.

## 17.2 커밋 메시지

의도를 설명하는 메시지를 사용합니다.

권장:

```text
feat: add aim input state machine
fix: update target settings for UE 5.8
refactor: separate enemy attack telegraph from damage
docs: add DreamCatcher project context
```

지양:

```text
작업 1
Level 1 1-2
수정
```

맵 작업은 변경 구역을 명시합니다.

```text
level: revise Level 1 encounter-2 rooftop route
```

## 17.3 바이너리 에셋 담당자

`.umap`, `.uasset`은 일반적인 텍스트 병합이 어렵습니다.

작업 전 한 명이 다음 에셋의 임시 소유권을 가집니다.

- 맵
- 주요 Widget Blueprint
- Character Blueprint
- Animation Blueprint

두 사람이 같은 바이너리 에셋을 동시에 수정하지 않습니다.

## 17.4 Git LFS

현재 LFS 대상:

- `.uasset`
- `.umap`
- `.ubulk`
- `.uexp`
- `.utoc`
- `.ucas`
- `.fbx`
- `.blend`
- `.psd`
- `.exr`
- `.wav`
- `.mp3`
- `.mp4`

LFS 규칙을 임의로 제거하지 않습니다.

## 17.5 디자이너용 바이너리 동기화

디자이너가 Visual Studio 전체를 설치하지 않고 C++ 클래스를 사용할 수 있도록 Win64 Editor 바이너리를 동기화하는 방향을 사용합니다.

대상:

```text
Binaries/Win64/
Plugins/**/Binaries/Win64/
```

주의:

- 개발자와 디자이너가 같은 엔진 버전을 사용해야 합니다.
- `UnrealEditor-DreamCatcher.dll`과 `UnrealEditor.modules`가 같은 빌드여야 합니다.
- C++ 변경 후 개발자는 다시 빌드하고 일치하는 바이너리를 커밋해야 합니다.
- UE 5.7 바이너리와 UE 5.8 프로젝트를 섞지 않습니다.

---

# 18. Codex 작업 규칙

## 18.1 작업 전 확인

Codex는 작업 전에 다음을 수행합니다.

1. 이 파일을 읽습니다.
2. 현재 브랜치를 확인합니다.
3. 관련 파일을 확인합니다.
4. 현재 프로젝트가 UE 5.7인지 UE 5.8인지 확인합니다.
5. 직접 수정할 수 없는 바이너리 Editor 작업을 구분합니다.
6. 수정할 소스 파일 범위를 명시합니다.

## 18.2 아키텍처 원칙

선호:

- 책임이 명확한 작은 컴포넌트
- 이벤트 기반 UI
- Blueprint 연출 훅
- 조정 가능한 수치
- 되돌리기 쉬운 변경
- 명확한 상태 전이
- 기존 이름 규칙 유지

명시적 요청 없이는 피할 것:

- GAS
- 멀티플레이 복제 아키텍처
- 대형 Ability / Attribute 프레임워크
- Behavior Tree 중심 구조 전환
- 과도한 Subsystem 추가
- 전체 폴더 재구성
- 대량 에셋 이름 변경
- Variant 샘플 구조로 교체

## 18.3 C++ 규칙

- Unreal 이름 규칙을 따릅니다.
- UnrealHeaderTool 호환 선언을 유지합니다.
- 헤더에서는 가능한 경우 Forward Declaration을 사용합니다.
- 필요한 헤더는 명시적으로 Include합니다.
- 우연한 Include 순서에 의존하지 않습니다.
- 반환형이 있는 함수의 모든 경로를 확인합니다.
- Timer와 Dynamic Delegate를 정리합니다.
- UObject 수명에 적절한 `UPROPERTY`, `TObjectPtr`, Weak Pointer를 사용합니다.
- 디자이너가 필요한 항목만 Blueprint에 노출합니다.
- 연출 전용 로직을 저수준 컴포넌트에 넣지 않습니다.
- Timer 또는 Event로 해결 가능한 경우 Tick을 추가하지 않습니다.
- Tick이 필요하면 최소한의 작업만 수행합니다.

## 18.4 Blueprint 전달 규칙

수동 Editor 작업이 필요하면 반드시 다음을 보고합니다.

- 정확한 에셋 경로
- 부모 클래스
- 할당할 프로퍼티
- 구현할 이벤트
- 노드 연결 순서
- 예상 실행 결과
- 검증 방법
- Redirector 또는 참조 위험

## 18.5 작업 범위 통제

관련 없는 파일을 수정하지 않습니다.

특히 다음을 포함하지 않습니다.

- `.idea`
- `Saved`
- `Intermediate`
- 관련 없는 `.uasset`
- 관련 없는 `.umap`
- 불필요하게 재생성된 대량 에셋
- 기능 작업과 무관한 엔진 전환 변경

## 18.6 작업 완료 보고서

모든 Codex 작업 완료 보고에는 다음을 포함합니다.

1. 요약
2. 변경 파일
3. 구현된 동작
4. 사용한 가정
5. 빌드 결과
6. 테스트 내용
7. Unreal Editor 수동 작업
8. 알려진 제한
9. 다음 권장 작업

---

# 19. 빌드 및 검증

## 19.1 엔진 전환 후 클린 빌드

Unreal Editor와 IDE를 종료한 뒤 생성 폴더를 삭제합니다.

삭제 가능:

```text
Binaries
Intermediate
.vs
```

`Saved`는 로컬 상태를 초기화할 때만 선택적으로 삭제합니다.

삭제 금지:

```text
Content
Config
Source
Plugins
DreamCatcher.uproject
```

## 19.2 UE 5.8 Editor 빌드 예시

로컬 경로에 맞게 수정합니다.

```bat
"<UE_5.8>\Engine\Build\BatchFiles\Build.bat" ^
DreamCatcherEditor Win64 Development ^
-Project="<Project>\DreamCatcher.uproject" ^
-WaitMutex -NoHotReloadFromIDE
```

## 19.3 검증 단계

### 1단계 — 정적 확인

- 선언 구조 확인
- Include 확인
- Reflection Macro 확인
- Delegate Signature 확인
- Timer와 소유권 확인

### 2단계 — C++ 빌드

- `DreamCatcherEditor Win64 Development` 성공
- 새 Warning-as-Error 없음
- Generated Code 성공

### 3단계 — Editor 확인

- 프로젝트 실행
- C++ 부모 Blueprint 로드
- Missing Class 경고 없음
- Blueprint Compile Error 없음
- 필요한 프로퍼티 할당 가능

### 4단계 — 게임플레이 확인

- 대상 맵 실행
- 기능 동작이 요구사항과 일치
- 상태가 올바르게 초기화
- 입력 잠김 없음
- Delegate 또는 Timer 잔여 없음
- 재시작과 레벨 전환 정상

## 19.4 바이너리 에셋 한계

C++ 빌드 성공만으로 다음을 보장하지 않습니다.

- Blueprint 그래프 연결
- Input Action 할당
- Widget 레이아웃
- 애니메이션 연결
- 레벨 Actor 배치

항상 Editor 검증을 별도로 작성합니다.

---

# 20. 향후 폴더 구조

당장 재구성하지 않고 장기 목표로 사용합니다.

## 20.1 Source

```text
Source/DreamCatcher/
  Core/
  Player/
  Components/
  AI/
  Stage/
  UI/
  Meta/
  Data/
```

## 20.2 Content

```text
Content/DreamCatcher/
  Blueprints/
    Core/
    Player/
    AI/
    Stage/
    UI/
  Data/
    Characters/
    Weapons/
    Enemies/
    Bosses/
    Stages/
    Rewards/
    Upgrades/
  Input/
  Maps/
    Prototype/
    Combat/
    Boss/
  Animations/
  FX/
  Audio/
  Sequences/
  Placeholder/
```

## 20.3 에셋 이동 조건

다음 조건에서만 에셋을 이동합니다.

- 수직 슬라이스가 안정됨
- 참조 구조를 이해함
- 두 팀원이 동의함
- Unreal Editor에서 이동함
- Redirector 정리
- 게임플레이 변경과 분리된 커밋

---

# 21. 작업 루틴

## 21.1 개발자 일일 루틴

1. 오늘 만들 기능 하나를 정합니다.
2. 기능의 규칙과 상태를 정의합니다.
3. C++를 구현합니다.
4. 빌드합니다.
5. Blueprint 훅을 엽니다.
6. Test Map에서 검증합니다.
7. 프로젝트를 항상 플레이 가능한 상태로 유지합니다.
8. 다음 시스템 전에 막는 버그를 해결합니다.
9. 하나의 의도로 커밋합니다.

## 21.2 디자이너 일일 루틴

1. 오늘 작업할 Blueprint 또는 레벨 구역을 정합니다.
2. 메시, 애니메이션, VFX, SFX, UI를 연결합니다.
3. 아름다움보다 가독성을 먼저 봅니다.
4. 적 공격 의도가 보이는지 확인합니다.
5. 피격 결과가 이해되는지 확인합니다.
6. 피드백을 다음으로 구분해 기록합니다.
   - 버그
   - 가독성 문제
   - 손맛 문제
   - 시각 폴리싱

## 21.3 주간 루틴

- 최소 한 번 전체 플레이 빌드 확인
- 현재 가능한 흐름 전체 플레이
- 구현 상태 문서 갱신
- 버그와 재미 문제 분리
- 바이너리 에셋 소유권 확인
- 다음 주 핵심 위험 하나 선택
- 큰 시스템 여러 개 동시 시작 금지

---

# 22. 기능 완료 기준

함수가 존재한다고 기능이 완성된 것이 아닙니다.

## 22.1 사격

완료 기준:

- 입력 반응이 빠름
- 발사가 보임
- 명중 지점이 읽힘
- 명중과 빗나감 구분
- 사운드가 타이밍을 보조
- 카메라 반응이 과하지 않음
- 수치 조정 가능
- 사망 또는 상태 중단 시 발사 종료

## 22.2 회피

완료 기준:

- 시작과 끝이 보임
- 실제 방어 효과가 있음
- 쿨다운 이해 가능
- 이동 제어 가능
- 충돌 문제 확인
- 피격 중 상호작용 확인

## 22.3 적 공격

완료 기준:

- 데미지 전에 공격 의도가 보임
- 대응 시간이 공정함
- 피격 방향 이해 가능
- 장애물이 공격을 막음
- 후딜이 있어 난사처럼 보이지 않음

## 22.4 조준 시스템

완료 기준:

- 짧은 클릭이 Scope를 안정적으로 토글
- 길게 누르면 Shoulder Aim 시작
- 해제 시 Hip 또는 이전 Scope로 복귀
- 클릭과 Hold가 중복 실행되지 않음
- 강제 취소 시 상태 초기화
- 카메라 보간 안정
- 감도 정상 전환
- HUD와 애니메이션이 실제 상태 반영

## 22.5 인카운터

완료 기준:

- 스포너가 한 번만 시작
- 모든 적 사망 집계
- 완료 이벤트 한 번만 발생
- 잘못된 스포너 또는 클래스 때문에 진행 막힘 없음
- StageDirector 진행
- 재시작 후 이전 상태가 남지 않음

## 22.6 수직 슬라이스

시작부터 결과까지 Unreal Editor에서 수동 개입 없이 플레이할 수 있어야 합니다.

---

# 23. Codex 작업 명세 템플릿

```md
# 작업명

## 목표

플레이어 또는 개발자가 확인할 수 있는 결과를 작성합니다.

## 배경

`AGENTS.md`와 기능별 명세를 참조합니다.

## 수정 범위

변경 가능한 파일과 시스템을 명시합니다.

## 필수 동작

결정적인 요구사항을 작성합니다.

## 제외 범위

추가하면 안 되는 기능을 작성합니다.

## Blueprint 전달

필요한 수동 에셋 작업을 작성합니다.

## 완료 기준

관찰 가능한 성공 / 실패 기준을 작성합니다.

## 검증

- 정적 확인
- 빌드
- Editor 확인
- 게임플레이 테스트

## 완료 보고

- 변경 파일
- 구현 내용
- 가정
- 테스트 결과
- 수동 Editor 작업
- 제한 사항
```

### 조준 시스템 예시

```md
# 작업: 조준 입력 상태 머신

## 목표

짧은 우클릭 Scope 토글과 길게 누르는 Shoulder Aim을 구현합니다.

## 수정 범위

- DreamCatcherCharacter
- DCCombatComponent
- 필요한 경우 DCPlayerHUDWidget

## 필수 동작

- 짧은 우클릭은 Scope 토글
- Hold 기준 기본값 0.18초
- 기준 시간 이후 Shoulder 활성화
- 우클릭 해제 시 Shoulder 종료
- Shoulder 종료 후 이전 Scope 상태 복구
- 회피, 궁극기, 사망, 입력 취소 시 상태 정리

## 제외 범위

- WeaponComponent
- SceneCapture 스코프
- 바이너리 Input Action 직접 생성
- AnimBP 수정
- 최종 UI 아트

## Blueprint 전달

IA_Aim, IMC_Player, Character Blueprint, HUD, AnimBP 연결 절차를 작성합니다.
```

---

# 24. ChatGPT, GitHub, Codex 작업 흐름

ChatGPT 프로젝트의 모든 대화가 Codex에 자동으로 전달되지는 않습니다.

GitHub 문서를 공통 기준으로 사용합니다.

```text
ChatGPT 프로젝트
  -> 기획 및 아키텍처 결정
  -> AGENTS.md 또는 docs/specs/<기능>.md 갱신
  -> Codex가 리포지토리 읽기
  -> 기능 브랜치에서 코드 수정
  -> 개발자 Diff 검토
  -> Unreal Editor 수동 연결
  -> 게임플레이 검증
  -> Merge
  -> 프로젝트 상태 문서 갱신
```

## 24.1 ChatGPT에 둘 내용

- 논의
- 대안
- 시각 아이디어
- 초기 세계관
- 디자인 피드백
- 기능 정의

## 24.2 GitHub Markdown에 둘 내용

- 확정 결정
- 아키텍처
- 구현 요구사항
- 현재 상태
- 완료 기준
- 빌드 규칙
- Editor 절차
- 알려진 위험

## 24.3 Codex 작업 범위

- 소스 분석
- C++ 구현
- Config 변경
- 빌드 오류 수정
- 테스트
- 범위가 명확한 리팩터링
- Diff 검토
- Blueprint 전달 문서

---

# 25. 명시적 승인 없이 하지 말 것

- GAS 도입
- 멀티플레이 복제 설계
- 대형 Behavior Tree 프레임워크
- 수직 슬라이스 전에 큰 인벤토리 개발
- 레벨 구조 확정 전 미니맵 개발
- 전체 Content 폴더 일괄 이동
- 대량 에셋 이름 변경
- Unreal Editor 밖에서 바이너리 에셋 수정
- UMG 안에 게임 규칙 계산
- 모든 기능을 Character에 추가
- 보스를 체력 높은 일반 적으로 구현
- 첫 스코프를 SceneCapture2D로 구현
- 정상적인 빌드 전환 문제를 강제 Override로 숨김
- 엔진 전환과 게임플레이 기능을 한 커밋에 혼합
- `main` 직접 커밋
- 두 사람이 같은 `.umap` 동시 수정

---

# 26. 프로젝트 용어

## DreamCatcher

프로젝트 제목이자 악몽 사건을 처리하는 중심 조직 또는 부대.

## Lucid City

근미래 도시 배경.

## Dream Core

꿈을 변환해 만든 에너지.

## 악몽 오염

Dream Core 생산 부산물로 발생하는 악몽 현상과 침식.

## Shoulder Aim

우클릭을 기준 시간 이상 유지했을 때 활성화되는 3인칭 견착 조준.

## Scope

우클릭 짧은 클릭으로 토글되는 확대 조준.

## Hip

기본 비조준 상태.

## Encounter

하나 이상의 EnemySpawner를 실행하고 적 처치 후 완료되는 전투 구간.

## StageDirector

스테이지 페이즈 순서를 실행하는 Actor.

## Vertical Slice

전투, 보스, 결과까지 포함하는 짧고 완결된 대표 플레이 세션.

---

# 27. 현재 확정되지 않은 항목

다음은 현재 자료만으로 완전히 확정할 수 없습니다.

- 최종 공식 스토리 시놉시스
- 최종 캐릭터 이름
- 최종 MBTI
- 최종 플레이어블 캐릭터 수
- 최종 무기 목록
- 최종 보스 목록
- Dream Core의 세부 과학 원리
- 루시드 시티의 정치 구조
- DreamCatcher 조직의 최종 영문 부제
- 최종 스테이지 수
- 최종 성장 경제
- 최종 재화 종류
- 최종 인벤토리 범위
- 현재 논의된 액션 외 전체 조작법
- 모든 HUD 요소의 최종 위치
- 최종 오디오 자산 목록
- 최종 타깃 하드웨어 및 성능 예산
- UE 5.8 전환을 최종 확정할지 여부

별도 결정 없이 확정 설정으로 만들지 않습니다.

---

# 28. 다음 권장 작업 패키지

## 패키지 A — UE 5.8 전환 안정화

- 두 Target 파일 V7 / Unreal5_8 적용
- 클린 빌드
- 새로운 기본 설정으로 드러난 실제 컴파일 오류 수정
- 프로젝트 실행
- Blueprint 부모 클래스 확인
- 디자이너용 바이너리 재빌드
- 엔진 전환만 별도 커밋

## 패키지 B — 조준 시스템

- C++ 입력 상태 머신
- CombatComponent 조준 상태
- Blueprint 이벤트
- IA_Aim 및 IMC_Player 연결 문서
- 카메라, HUD, AnimBP 연결
- 클릭 / Hold 기준 테스트

## 패키지 C — 적 공격 텔레그래프

- 공격 의도, 선딜, 명중, 후딜 분리
- 연출 이벤트
- 즉시 데미지 제거
- 시야 및 중단 테스트

## 패키지 D — 회피 방어

- 명확한 무적 또는 데미지 차단 상태
- 회피 상태 이벤트
- 충돌 및 사망 상호작용
- 지속시간과 쿨다운 조정

## 패키지 E — Level 1 수직 슬라이스 연결

- 인카운터 구역 확정
- EncounterController와 Spawner 배치
- StageDirector 연결
- 보스 진입 Placeholder
- 결과 Placeholder
- 재시작 검증

## 패키지 F — 최소 보스 기반

- 전용 보스 클래스
- 페이즈 컴포넌트
- 공격 패턴 2개
- 보스 HUD 이벤트
- 사망 완료 신호

---

# 29. 최종 프로젝트 원칙

DreamCatcher는 잘못된 구조 위에 있는 프로젝트가 아닙니다.

현재는 작은 팀에 적절한 구조 위에 아직 완성되지 않은 기능이 쌓여 있는 상태입니다.

프로젝트는 다음 순서로 완성합니다.

- 현재 전투를 읽기 쉽고 시원하게 만듭니다.
- 처음부터 끝까지 플레이 가능한 한 판을 완성합니다.
- 보스를 일반 적과 분리합니다.
- 실제 반복 조정이 필요한 데이터를 DataAsset로 옮깁니다.
- 전투가 재미있다는 것이 확인된 뒤 메타 루프를 추가합니다.

성공 기준은 아키텍처의 복잡성이 아닙니다.

> 현재 구조를 안정적이고, 신나고, 읽기 쉽고, 완결된 DreamCatcher 경험으로 완성하는 것이 성공 기준입니다.
