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

| 작업 대상 | 주요 디렉터리 |
| --- | --- |
| 런타임 C++ | `Source/DreamCatcher/` 아래 `Character/`, `Player/`, `GameModes/`, `AI/`, `Stage/`, `UI/` |
| 블루프린트와 위젯 | `Content/DreamCatcher/Blueprints/` 아래 `AI/`, `Stage/`, `UI/` |
| 맵 | `Content/DreamCatcher/Maps/` 아래 `Story/`, `Frontend/`, `Tests/` |
| 입력과 게임 데이터 | `Content/DreamCatcher/Input/`, `Content/DreamCatcher/Data/` |
| 아트와 사운드 | `Content/DreamCatcher/` 아래 `Meshes/`, `Animations/`, `Textures/`, `Materials/`, `FX/`, `Audio/` |
| 에디터 제작·검사 스크립트 | `Scripts/Editor/` 아래 `AI/`, `Combat/`, `Stage/`, `UI/` |
| 프로젝트 설정 | `Config/` |

캐릭터, 플레이어 컨트롤러와 게임 모드는 각 역할 폴더의 C++ 클래스에서 정의합니다. [ADreamCatcherCharacter](Source/DreamCatcher/Character/DreamCatcherCharacter.h) · [ADreamCatcherPlayerController](Source/DreamCatcher/Player/DreamCatcherPlayerController.h) · [ADCLyraGameMode](Source/DreamCatcher/GameModes/DCLyraGameMode.h)

스테이지 진행을 수정할 때는 `Stage/DCStageDirector.h`의 `ADCStageDirector`, 이를 사용하는 `Blueprints/Stage/BP_StageDirector`, 배치를 확인할 `Maps/Story/Level_1`을 함께 봅니다. [스테이지 C++](Source/DreamCatcher/Stage) · [스테이지 블루프린트](Content/DreamCatcher/Blueprints/Stage) · [스토리 맵](Content/DreamCatcher/Maps/Story)

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

- Windows 64비트용 Unreal Engine 5.8
- Rider 또는 Visual Studio 기반 Unreal C++ 개발 환경
- Git LFS

### Blueprint Arrange 설치

프로젝트는 블루프린트 노드 정렬용 에디터 플러그인 `BlueprintArrange`를 사용합니다. 새 PC에서 프로젝트를 처음 열기 전에 다음 순서로 설치합니다.

1. Epic 계정의 Fab 라이브러리에 **Blueprint Arrange**를 추가합니다. [Fab 상품 페이지](https://www.fab.com/listings/af6a5d41-480d-4b03-ac84-8b446126dc59)
2. Epic Games Launcher의 **Unreal Engine → 라이브러리 → Fab 라이브러리**에서 **Blueprint Arrange → 엔진에 설치(Install to Engine)**를 선택하고 프로젝트를 열 UE 5.8에 설치합니다. [Epic 플러그인 설치 안내](https://dev.epicgames.com/documentation/unreal-engine/working-with-plugins-in-unreal-engine)
3. 팀 에셋 설치를 마치고 프로젝트를 연 뒤 **편집(Edit) → 플러그인(Plugins)**에서 `Blueprint Arrange`를 검색해 활성 상태를 확인합니다.

### 팀 에셋 설치

`Level_1`은 팀용 로컬 저장소 `DreamCatcher-PrivateAssets`의 `Content/BlockingStarterPack`을 사용합니다. 에셋 저장소와 LFS 객체를 받아 프로젝트와 같은 상위 폴더에 둡니다. 현재 팩은 650개 파일, 135,549,028바이트입니다. 에디터를 열기 전에 [DreamCatcher.uproject](DreamCatcher.uproject)가 있는 폴더에서 PowerShell로 실행합니다.

```powershell
$project = (Get-Item -LiteralPath '.\DreamCatcher.uproject' -ErrorAction Stop).DirectoryName
$assets = (Resolve-Path -LiteralPath '..\DreamCatcher-PrivateAssets' -ErrorAction Stop).Path
$pack = 'Content/BlockingStarterPack'
$tree = git -C $assets rev-parse "HEAD:$pack"
if ($tree -ne '20fea4282ab4f6a56d9fc7aeb4a8795a2cebc78a') { throw '팀 에셋 저장소의 팩 버전을 확인하세요.' }
git -C $assets lfs checkout "$pack/**"
if ($LASTEXITCODE -ne 0) { throw '팀 저장소에서 LFS 파일을 복원하세요.' }
$manifest = (git -C $assets lfs ls-files --json --include="$pack/**" HEAD | Out-String | ConvertFrom-Json).files
if ($manifest.Count -ne 650) { throw 'Git LFS 목록에 팩 파일 650개가 필요합니다.' }

function Assert-BlockingPack($root) {
    $files = @(Get-ChildItem -LiteralPath (Join-Path $root $pack) -Recurse -File -Force -ErrorAction Stop)
    if ($files.Count -ne 650) { throw "$root 폴더의 팩 파일 수를 확인하세요." }
    foreach ($entry in $manifest) {
        $file = Join-Path $root $entry.name
        if ((Get-FileHash -LiteralPath $file -Algorithm SHA256 -ErrorAction Stop).Hash -ne $entry.oid) { throw "$file 파일이 LFS 원본과 다릅니다." }
    }
}
Assert-BlockingPack $assets
$destination = Join-Path $project $pack
if (-not (Test-Path -LiteralPath $destination)) {
    git -C $assets diff --cached --quiet HEAD -- $pack
    if ($LASTEXITCODE -ne 0) { throw '팀 에셋 저장소의 팩에 스테이징된 변경이 있습니다.' }
    $prefix = $project.Replace('\', '/') + '/'
    for ($i = 0; $i -lt $manifest.Count; $i += 50) {
        $end = [Math]::Min($i + 49, $manifest.Count - 1)
        $paths = $manifest[$i..$end].name
        git -C $assets checkout-index "--prefix=$prefix" -- $paths
        if ($LASTEXITCODE -ne 0) { throw '팩 설치가 완료되지 않았습니다. 대상 폴더를 확인하세요.' }
    }
}
Assert-BlockingPack $project
```

기존 팩 폴더는 검증만 수행합니다. 설치가 끝나면 콘텐츠 브라우저에서 `/Game/BlockingStarterPack/Meshes/Primitives/SM_Cube`를 열고, `/Game/DreamCatcher/Maps/Story/Level_1`에서 블록아웃 메시가 표시되는지 확인합니다.

### 프로젝트 열기

1. `DreamCatcher.uproject`를 엽니다.
2. 필요하면 프로젝트 파일을 갱신합니다.
3. C++ 프로젝트를 빌드합니다.
4. Unreal Editor에서 프로젝트를 실행합니다.

### 에디터 스크립트 경로와 진단 자료

`Scripts/Editor/`에서 에디터 실행 명령을 생성하는 도구는 스크립트가 있는 체크아웃과 프로젝트에 연결된 UE 5.8 설치를 기준으로 경로를 찾습니다. 별도 엔진 설치를 사용할 때는 도구를 실행하는 셸의 환경 변수 `DC_UNREAL_EDITOR_CMD`에 해당 설치의 `Engine/Binaries/Win64/UnrealEditor-Cmd.exe` 전체 경로를 지정합니다.

다음 도구를 새 PC에서 실행하려면 해당 검사에 사용한 자료도 준비합니다. `Saved/Diagnostics/`는 Git에서 제외된 로컬 폴더입니다.

| 작업 | 필요한 자료 |
| --- | --- |
| 미션 HUD 제작 도구 `mission_hud_setup.py` | 해당 UE 5.8 설치의 엔진 소스(`Engine/Source`)와 `Saved/Diagnostics/Level1Merge20261016/apply_merge.py` |
| 탄약 HUD 검사 `weapon_ammo_hud_gameplay_probe.py` | `Saved/Diagnostics/WeaponAmmo20261020/Before_A/baseline.json` |
| 스테이지 진행 검사 `level_progression_gameplay_probe.py`의 `decode_error` 사례 | `Saved/Diagnostics/LevelProgression20261023/Apply_A/report.json`과 그 기록의 SHA-256에 일치하는 당시 원본 영상 |

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
