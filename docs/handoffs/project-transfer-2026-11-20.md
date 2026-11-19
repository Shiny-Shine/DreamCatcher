# DreamCatcher 새 PC 개발 인수인계

2026-11-20 KST · [Shiny-Shine/DreamCatcher](https://github.com/Shiny-Shine/DreamCatcher) · 기준 브랜치 `main` · 기준 커밋 `77974f21de62b9e2af44e357bd4c66ede66732c1`

**새 PC에서 가장 먼저 할 일은 본 프로젝트와 팀 에셋 저장소를 함께 복원하는 것이다.** 정식 스토리는 프런트엔드에서 Level_1·2·3과 BIG BROTHER 전투를 거쳐 메뉴로 돌아오는 단계까지 연결됐다. 폴더 재배치는 완료됐다. 다음 게임플레이 작업은 별도 GASP 테스트맵에서 최신 조준·장착을 직접 확인한 뒤 권총·무기 교체 연결 범위를 검토하는 것이다.

이 문서의 링크는 체크아웃 안에서 상대 경로로 열린다. `/Game/…`은 Unreal 콘텐츠 브라우저에서 프로젝트의 `Content/…`에 해당한다. 11월 18일 이전 인계와 검사 로그에 적힌 경로는 당시 위치이며, 아래 표가 11월 20일의 편집 위치다.

## 1. 기존 PC에서 챙길 것

| 대상 | 이전 방법과 이유 |
|---|---|
| 본 저장소와 Git LFS | GitHub Desktop에서 전달할 커밋과 Push 상태를 확인한다. 11월 20일 조사 시작 시 작업 트리는 깨끗했고, 로컬 `main`과 로컬 원격 추적 브랜치 `origin/main`이 위 커밋을 가리켰다. 장비 안정성 코드·GASP 에셋·안정성 인계가 이미 추적된다. 이 인수인계 문서도 다음 전달 커밋에 포함한다. |
| **팀 저장소 `DreamCatcher-PrivateAssets`** | 본 프로젝트 옆에 둘 별도 저장소다. 생성 기록의 커밋은 `0f322827`, 브랜치는 `codex/blocking-starter-pack`이며 당시 등록된 원격은 0개였다. 해당 체크아웃과 숨김 `.git` 아래의 LFS 객체까지 확보한다. 공용 저장소에서 제외된 `Content/BlockingStarterPack`을 Level_1이 사용한다. |
| 검증·복구 근거 | 필요한 `Saved/Diagnostics`, `Saved/Automation`, `Saved/Logs`, `Saved/Crashes`, 복구할 Autosaves·캡처를 별도로 백업한다. 특히 `RoleLayout*`, `EditorScriptPortability`, `ProjectStability20261117`, `GASPADS_20261117_A`, `GASPTraversalCombat_20261117_A`가 이번 재개 근거다. |
| 사용자 설정 | 기존 키·설정을 이어가려면 `Saved/SaveGames/EnhancedInputUserSettings.sav`, `SharedGameSettings.sav`와 필요한 로컬 설정을 보존한다. |
| 로컬 Git 복구 이력 | 기존 PC에는 PV·Level_1 관련 stash 2개와 `refs/codex-recovery/level1-20261016/` 아래 복구 참조 4개가 있다. 일반 clone으로 이전되지 않는다. 이 이력까지 필요하면 기존 저장소의 숨김 `.git`과 LFS 저장소를 포함한 백업을 보존한다. 이미 복구한 Level_1은 현재 커밋을 사용한다. |
| 제작 원본·비교 자료 | 디자이너가 보관하는 FBX·PNG·Blender·PSD·원본 영상·음원과, 이식 비교에 필요한 Lyra 원본 프로젝트를 확보한다. 재임포트 조사 대상 117개 에셋에서 기록된 원본과 해시가 일치한 파일은 조사 범위에서 0개였다. 그중 HARU의 과거 FBX에는 UE에서 추가한 본·가중치가 없어 재임포트 전 제작 원본 반영이 필요하다. |

`Intermediate`, `DerivedDataCache`, IDE 캐시는 재생성할 수 있다. Win64 Editor DLL과 `.modules`, 타깃 파일은 디자이너와 동기화하는 추적 파일이므로 해당 커밋의 조합을 사용한다. [Git LFS 대상](../../.gitattributes) · [HARU 가중치·재임포트 인계](haru-lyra-weight-transfer-2026-10-17.md) · [Level_1 복구 기록](level1-merge-recovery-2026-10-16.md)

## 2. 새 Windows PC에서 처음 열기

1. **환경을 맞춘다.** 현재 PC는 UE **5.8.4**(CL `58902052`), Win64, DX12·SM6를 사용한다. 프로젝트의 EngineAssociation은 `5.8`이고 Game·Editor 타깃은 V7 / Unreal5_8을 사용한다. 성공한 사용자 빌드 로그의 도구 체인은 MSVC `14.50.35730`, Windows SDK `10.0.26100.0`이었다. C++ 게임 개발 도구와 Git LFS를 설치하고, Fab의 **Blueprint Arrange**를 같은 UE 5.8 엔진에 설치한다. 기존 PC의 플러그인 버전은 1.1.0이다. [프로젝트](../../DreamCatcher.uproject) · [VS 구성](../../.vsconfig) · [플러그인 설치 절차](../../README.md#blueprint-arrange-설치)
2. **두 저장소를 같은 상위 폴더에 둔다.** 본 저장소에서 전달받은 커밋을 체크아웃하고 LFS 실파일을 받는다. 새 clone에서는 `git lfs pull` 후 `git rev-parse HEAD`로 커밋을 확인한다. 이 문서를 담은 후속 커밋을 전달받았다면 그 커밋이 기준 커밋을 포함해야 한다.
3. **Editor를 열기 전에 팀 팩을 설치한다.** README의 PowerShell 절차가 팀 저장소에서 팩을 설치하고 파일별 SHA-256을 검사한다. 기준 트리 ID는 `20fea4282ab4f6a56d9fc7aeb4a8795a2cebc78a`, 팩은 **650파일 / 135,549,028바이트**다. 최종 위치는 `DreamCatcher/Content/BlockingStarterPack`이다. [팀 에셋 설치·검사](../../README.md#팀-에셋-설치)
4. **설정과 필요한 진단 자료를 복원한다.** Editor 도구 14개의 PC 고정 경로를 정리했다. 실행 파일 자동 탐색이 실패할 때만 `DC_UNREAL_EDITOR_CMD`에 해당 엔진의 `UnrealEditor-Cmd.exe` 절대 경로를 지정한다. 미션 HUD 제작에는 UE 엔진 소스와 `Level1Merge20261016/apply_merge.py`, 탄약 HUD 검사에는 과거 baseline, 영상 디코드 실패 검사에는 과거 보고서와 일치하는 원본 영상이 필요하다. [도구별 자료 목록](../../README.md#에디터-스크립트-경로와-진단-자료) · [경로 탐색 코드](../../Scripts/Editor/dc_editor_paths.py)
5. **사용자가 Editor를 열고 필요한 경우 일반 Editor 빌드를 한다.** 개발 PC의 빌드 타깃은 `DreamCatcherEditor / Win64 / Development`다. Editor를 열었을 때 기본 맵은 **Level_1**, 게임을 기본 설정으로 시작했을 때 첫 맵은 **L_DC_FrontEnd**다. 전역 GameMode는 `Engine.GameModeBase`이고 실제 플레이는 맵의 World Settings와 Experience가 결정한다. README의 ‘기본 프로젝트 설정’에는 과거 GameMode가 남아 있어 현재 값은 아래 Config를 따른다. [시작 설정](../../Config/DefaultEngine.ini) · [Cook·Experience 설정](../../Config/DefaultGame.ini)
6. **옮긴 환경에서 한 판을 확인한다.** 프런트엔드 PV와 메뉴 BGM → 난이도 선택 → Level_1에서 블록아웃 메시·HARU·장비·HUD → 조준·사격·재장전·무기 교체 → 다음 레벨·보스·클리어 영상·메뉴 복귀를 확인한다. 장비 크래시는 같은 Editor 세션의 두 번째 PIE에서도 발생했던 이력이 있으므로, Level_1과 GASP 테스트맵에서 각각 PIE 종료 후 한 번 더 시작해 초기 장착을 확인한다. 사용자 패키지는 별도 실행 결과로 기록한다.

## 3. 프로젝트와 팀

DreamCatcher는 **5~8분의 한 판**에 캐릭터 표현, 오버숄더 사격, 읽고 피할 수 있는 공격, 보스전과 결과를 담는 UE 기반 서브컬처 TPS다. 게임의 배경인 루시드 시티는 꿈에서 추출한 에너지 ‘드림 코어’를 사용한다. DreamCatcher 조직은 그 과정에서 생기는 악몽 오염에 대응한다. HARU가 현재 플레이어 제작의 중심이며, BIG BROTHER는 감시·기계 눈·신호 왜곡을 모티프로 한 보스다. 밝은 근미래 도시, 셀 채색, 스트리트 테크웨어, 원·별·신호선과 1920×1080 기준의 읽기 쉬운 HUD가 미술 방향이다. 최종 캐릭터 수·전체 스테이지 수·성장 경제는 기획 결정 사항으로 남아 있다. [세계관·제작 방향](../../AGENTS.md) · [게임 소개](../../README.md)

| 담당 | 책임 |
|---|---|
| 개발자·사용자 | C++ 아키텍처, 능력·피해·AI·보스·스테이지·데이터 연결, 빌드와 기술 통합. 일반 C++ 빌드·패키징 및 GitHub Desktop 커밋·푸시·병합을 수행한다. |
| 디자이너 | 캐릭터·환경 제작, 애니메이션, VFX/SFX, HUD 배치·스타일, 레벨 동선·배치, 컷신과 Blueprint 연출. 같은 맵·주요 BP를 편집할 때 담당자를 정한다. |
| 공동 | 사격·대시 손맛, 카메라, 적 공격 가독성, 보스 난이도, UI 사용성과 최종 시청각 수용을 결정한다. |
| Codex | 원본 조사, 승인된 코드·에셋 작업, Unreal 내부 자동화, 관련 검증·기록을 맡는다. 제작 인원은 사람 기준 2명이다. |

## 4. 기능을 수정할 때 찾아갈 곳

| 찾는 것 | 현재 위치와 사용법 |
|---|---|
| 실제 맵 | [Frontend](../../Content/DreamCatcher/Maps/Frontend) · [Story](../../Content/DreamCatcher/Maps/Story) · [GASP 및 이동 테스트](../../Content/DreamCatcher/Maps/Tests/Movement) · [환경 인스턴스·제작용 맵](../../Content/DreamCatcher/Maps/Environment). 프로젝트 맵은 14개이며 Cook 명시 목록은 프런트엔드와 스토리 3개다. |
| GameMode·스테이지 | [Core BP](../../Content/DreamCatcher/Blueprints/Core)에서 맵별 GameMode를, [Stage BP](../../Content/DreamCatcher/Blueprints/Stage)에서 `BP_StageDirector`와 `BP_EncounterController`를 연다. 배치 확인은 `Maps/Story/Level_1`에서 한다. |
| 초기화·능력·입력 | [Experience BP](../../Content/DreamCatcher/Blueprints/Experiences), [ActionSets](../../Content/DreamCatcher/Data/Experiences/ActionSets), [PawnData](../../Content/DreamCatcher/Data/Pawns), [게임플레이 BP](../../Content/DreamCatcher/Blueprints/Gameplay), [입력](../../Content/DreamCatcher/Input). Experience와 ActionSet은 Config에 등록한 폴더에서 검색한다. |
| 플레이어·무기 | [Player BP](../../Content/DreamCatcher/Blueprints/Player), [무기 BP](../../Content/DreamCatcher/Blueprints/Weapons), [Equipment](../../Content/DreamCatcher/Blueprints/Equipment), [무기 데이터](../../Content/DreamCatcher/Data/Weapons). GASP 플레이어는 `Blueprints/Player/Movement/BP_DC_GASPPlayer`다. |
| 적·보스·난이도 | [AI BP](../../Content/DreamCatcher/Blueprints/AI), [AI 행동 데이터](../../Content/DreamCatcher/AI), [보스 데이터](../../Content/DreamCatcher/Data/Boss), [난이도](../../Content/DreamCatcher/Data/Difficulty), [미션 데이터](../../Content/DreamCatcher/Data/Stage). |
| UI·영상 | [Frontend 위젯](../../Content/DreamCatcher/Blueprints/UI/Frontend), [HUD](../../Content/DreamCatcher/Blueprints/UI/HUD), [미디어 에셋](../../Content/DreamCatcher/Media), [실제 영상 파일](../../Content/Movies). `W_DC_ModeCard`는 세 모드 카드의 공통 위젯이다. |
| 캐릭터·연출 | [HARU 실사용 메시](../../Content/DreamCatcher/Meshes/Characters/Haru/SKM_HARU.uasset), [애니메이션](../../Content/DreamCatcher/Animations), [AnimBP·Notify](../../Content/DreamCatcher/Blueprints/Animation), [재질](../../Content/DreamCatcher/Materials), [텍스처](../../Content/DreamCatcher/Textures), [음향](../../Content/DreamCatcher/Audio), [효과](../../Content/DreamCatcher/FX). |
| 소스·도구 | [단일 런타임 모듈](../../Source/DreamCatcher), [C++ 검사](../../Source/DreamCatcher/Tests), [프로젝트 플러그인](../../Plugins), [Editor 스크립트](../../Scripts/Editor). 스크립트는 `AI/`, `Combat/`, `Stage/`, `UI/`로 찾는다. |

예전 `DreamCatcher_ENV/Map/Main/Level_1`과 `Blueprint/Stage/BP_StageDirector`는 각각 `DreamCatcher/Maps/Story/Level_1`, `DreamCatcher/Blueprints/Stage/BP_StageDirector`로 옮겼다. 기존 `Main_Charcher/Haru/SKM_HARU`는 최신 레지스트리에서 새 메시를 가리키는 Redirector다. 재임포트 호환용 Redirector 154개, 환경 월드 외부 액터 패키지 184개, 플러그인 마운트 `/DCStageUI`, 로컬 팀 팩, `/Game/Tests`의 NeverCook 검사 자료는 각각 보존 용도가 있다.

## 5. 런타임 구조

| 흐름 | 소유권과 수정 지점 |
|---|---|
| 기동 → 플레이어 생성 | `DCGameInstance`와 `DCAssetManager`가 기반을 준비한다. 맵의 GameMode와 Experience가 PawnData, ActionSet과 GameFeature를 결정한다. Pawn은 Spawned → DataAvailable → DataInitialized → GameplayReady 순으로 초기화된다. [GameMode](../../Source/DreamCatcher/GameModes/DCLyraGameMode.cpp) · [Experience](../../Source/DreamCatcher/GameModes/Experience/DCLyraExperienceManagerComponent.h) |
| 입력 → 능력 | PlayerState가 능력 시스템 컴포넌트(ASC)와 속성 집합을 소유한다. PawnExtension이 ASC와 현재 Pawn을 연결하고, Hero가 Enhanced Input·태그·조준 제스처를 능력에 전달한다. 사망·Possession 해제·입력 차단 시 입력과 조준 상태를 정리한다. [PlayerState](../../Source/DreamCatcher/Player/DCPlayerState.h) · [PawnExtension](../../Source/DreamCatcher/Character/DCPawnExtensionComponent.cpp) · [Hero](../../Source/DreamCatcher/Character/DCHeroComponent.cpp) |
| 장착 → 발사 → 피해 | Controller의 Inventory가 아이템·탄약을, QuickBar가 활성 슬롯을 관리한다. Pawn의 Equipment가 무기 액터와 AbilitySet을 부여·회수한다. 발사 능력이 명중 정보를 만들고 피해 계산에서 공격값·거리·부위·팀을 처리한다. HealthSet은 체력·아머를, GameplayCue·메시지는 피격 연출과 HUD를 연결한다. 파일명이 `DCLoadoutController`인 실제 클래스는 `ADCRifleIntegrationController`이며 생산 경로에서 사용한다. [Loadout](../../Source/DreamCatcher/Player/DCLoadoutController.cpp) · [Equipment](../../Source/DreamCatcher/Equipment/DCLyraEquipmentManagerComponent.cpp) · [피해 계산](../../Source/DreamCatcher/AbilitySystem/Executions/DCLyraDamageExecution.cpp) |
| 적 → 인카운터 → 다음 맵 | 일반 봇은 Lyra AIController·행동 트리·장비를 사용한다. 보스는 자체 ASC를 가진 GAS NPC와 보스 패턴·페이즈를 사용한다. Spawner가 생성·정리를, EncounterController가 전투 완료를, StageDirector와 StageRunGameMode가 진행·실패·재시작·여행을 맡는다. [AI](../../Source/DreamCatcher/AI) · [스테이지](../../Source/DreamCatcher/Stage) · [맵 이동](../../Source/DreamCatcher/GameModes/DCStageRunGameMode.cpp) |
| 상태 → 화면 | Experience가 UI 기능을 활성화한다. CommonUI·CommonGame·UIExtension·DCStageUI가 위젯, 레이어, 입력 포커스와 수명을 담당한다. 정적 배치·스타일·카드 애니메이션은 UMG Designer, 상태·데이터·미디어 수명은 C++에서 다룬다. [UI 구현](../../Source/DreamCatcher/UI) · [DCStageUI](../../Plugins/GameFeatures/DCStageUI) |
| 난이도와 저장 | GameInstance의 난이도 Subsystem이 판 시작 시 프리셋을 복사한다. 스토리 중 적용하는 피해·시작 탄약과 사용자 키·설정 저장은 별개다. 영구 스토리 진행도·보상·강화 저장은 향후 제작 범위다. [난이도 Subsystem](../../Source/DreamCatcher/System/DCDifficultySubsystem.h) |
| GASP 장애물 이동 | 테스트 Pawn에 붙인 `DCTraversalCombatComponent`가 동작 중 전투 입력을 억제하고, 기존 무기를 등에 부착했다가 원래 손·소켓·상대 Transform으로 복원한다. 몽타주 종료·중단·사망·Possession 해제 처리도 여기서 찾는다. [Traversal 구현](../../Source/DreamCatcher/Character/DCTraversalCombatComponent.cpp) |

## 6. 구현 이력과 플레이 기준

| 범위 | 인수한 결과 |
|---|---|
| R0~R4 | Lyra 기반 GAS·OnSpawn·Pawn 초기화·입력·체력·피해·치료·면역·사망·재시작을 연결했다. 카메라와 우클릭 조준은 Hip / Shoulder / Scope를 사용한다. Hip에서 짧게 눌렀다 떼면 Scope, 길게 누르면 Shoulder, 홀드를 떼면 Hip으로 복귀한다. |
| R5~R8 | Inventory·QuickBar·Equipment와 라이플을 연결했다. 원본 발사·퍼짐·반동·재장전·명중 표시·퀵슬롯, 애니메이션 레이어, 점프·웅크림 점프까지 진행했다. 현재 스토리 플레이어는 SMG와 권총을 사용한다. SMG 외형과 탄창은 라이플의 장착·소켓·재장전 동작을 따른다. |
| R9~R10 | 원본 Dash와 방어 효과·성공 이벤트, Q 입력·쿨다운형 캐릭터 고유 기술의 공통 기반이 있다. 일반 적은 이동·사격·탄약 소진·픽업·재장전·사망을 수행하며 유한 스포너와 인카운터가 전투를 끝낸다. 회피 성공 전용 시각·음향은 사용자 결정으로 제외했다. |
| R11~R14 | 스테이지 완료·실패·재시작·복귀, 2패턴 보스, Experience·GameFeature와 HUD·Esc·결과 화면을 연결했다. 10월 14~16일에 승인된 레거시 정리, 디자이너 main 병합, Level_1 실사용 경로 전환과 맵 복구를 마쳤다. |
| 10월 후속 콘텐츠 | HARU 리깅·가중치·무기 부착과 대시 Blend Out 오류를 보완했다. 목·머리 약점과 피격 숫자·효과음, 레이더·무기·미션·보스·결과 HUD를 연결했다. HP·아머·보스·미션·결과 HUD의 Designer 이관과 공통 모드 카드의 UMG 확대·복귀는 사용자 수용까지 완료했다. |
| 스토리·보스 | 프런트엔드 PV → 모드·난이도 선택 → Level_1 → Level_2 → Level_3 → BIG BROTHER → 클리어 영상·메뉴 복귀를 연결했다. BIG BROTHER는 고정 위치에서 플레이어를 바라보고 직선·지면 공격을 수행한다. 프리·챌린지 카드는 현재 ‘준비 중’ 안내를 연다. |
| 11월 GASP | Game Animation Sample의 전용 테스트 Pawn에서 이동·점프·장애물 통과와 SMG를 연결했다. 11월 17일 실제 PIE 입력 주입으로 100cm 장애물, 150cm 턱, 250cm 오르기와 무기 복원을 확인했다. Traversal이 조준을 취소하고, 유지 입력은 재개되지 않으며, 새 홀드·탭으로 Shoulder·Scope가 다시 동작했다. 조준 중 사격·수동 재장전과 FOV 80/70/40 복원도 확인했다. |
| 11월 장비 안정성 | 장비 초기화 때 목록의 소유 컴포넌트를 다시 연결하고 현재 Pawn의 Possession·장비 등록·GameplayReady를 확인한다. 11월 18일 native 검사 2건, GASP·Level_1 각각 첫 PIE와 재PIE 총 4회에서 장착→해제→재장착·탄약 30 유지·월드 종료 정리를 확인했다. 과거 간헐 크래시 원인 전체는 미확정이다. |

[GAS·Lyra 기능 명세 및 R0~R14 상세 이력](../specs/gas-lyra-migration.md) · [초기 장비 안정성 인계](project-stability-2026-11-17.md) · [피격 피드백](hit-zones-and-damage-feedback-2026-10-18.md) · [SMG 탄창](smg-magazine-2026-10-18.md)

난이도는 `Data/Difficulty/DA_DC_Difficulty_Easy`, `Normal`, `Hard`에서 조정하고 새 스토리를 시작해 확인한다. 아래는 11월 1일 검증 기록의 기본값이다. 실제 피해량은 거리·부위·방어구 적용 결과에 따라 달라진다.

| 스토리 난이도 | 일반 적 피해 | 보스 직선 / 지면 | 플레이어 SMG / 권총 피해 | SMG 장전 / 예비 | 권총 장전 / 예비 |
|---|---:|---:|---:|---:|---:|
| Easy | 7 | 24 / 35 | 50 / 80 | 30 / 180 | 12 / 360 |
| Normal | 10 | 34 / 50 | 40 / 64 | 30 / 120 | 12 / 240 |
| Hard | 15 | 44 / 65 | 32 / 52 | 20 / 60 | 8 / 120 |

세 난이도의 전투·재시작을 확인했고 Hard로 전체 레벨 체인을 검증했다. 탄창 용량은 SMG 30, 권총 12다. [난이도 검증 기록](../../Saved/Diagnostics/Difficulty20261101/FinalValidation_A.json)

## 7. 구조 재점검 결과와 검증 시점

| 시점·범위 | 확인한 내용 |
|---|---|
| 11월 19일 구조 통합 도중 | 스토리 PIE에서 메뉴 진입·조준·권총 교체·발사·재장전·클리어 영상 3회·보스 패턴을 관찰했다. native·UI 검사 90건에서 오류 0이었다. GASP에서는 이동·착지·낮은 장애물·복귀 관찰이 통과했지만 기존 PoseAsset·디버그 조회 경고로 상위 검사기의 수용 값은 false였다. [당시 실행 결과](../../Saved/Diagnostics/RoleLayout_20261118/Integrated_20261119/postboot_delivery.final-tests.json) |
| 11월 19~20일 후속 이동 | 원래 이동표 7,161건(Content 7,041·C++ 99·Editor 스크립트 21)을 처리한 뒤 중복된 역할 경로 357건과 마지막 12건을 추가 정리했다. 이 숫자는 단계별 건수다. 최종 12건의 참조자 13개를 확인했고 해당 이동의 구 주소는 0개였다. 환경 Packed Level은 외부 액터 패키지와 실제 Level_1 배치를 검증했다. [월드·Packed Level 검사](../../Saved/Diagnostics/RoleLayout_WorldPairProduction/completion.json) · [최종 12건 검사](../../Saved/Diagnostics/RoleLayout_Final12/asset_acceptance.json) |
| 11월 20일 마감 기록 | 빈 폴더 343개를 정리했다. 검사한 공용 Unreal 패키지 7,787개의 해시 차이 0, 프로젝트 참조 24,557건에서 누락 대상 0, 승인 이동 잔여 0이다. 별도 환경 식생 샘플 의존성과 재임포트 원본은 기록에 구분돼 있다. [마감 결과](../../Saved/Diagnostics/RoleLayout_Empty343/completion.json) |
| 이 문서 작성 시 재확인 | 기준 커밋에서 Content와 DCStageUI의 실제 `.umap`·`.uasset` 8,437개를 같은 커밋의 레지스트리 목록과 대조했다. 등록된 8,434개 경로가 전부 존재했고 차이 3개는 기존 예외인 팀 팩 예제 맵 2개와 DCStageUI 데이터 1개였다. Cook 맵 4개와 시작 설정의 실제 맵, 활성 텍스트 경로, C++ 내부 include, Editor Python 22개의 문법을 확인했다. 이번 재점검은 디스크·설정·기존 기록을 사용했다. 새 Editor 빌드·PIE·패키지 실행은 수행하지 않았다. |

이전 통합 빌드·PIE, 마지막 에셋 이동 후의 별도 로드·참조 검사, 새 PC에서의 플레이 수용은 각각 시점이 다르다. 기존 컴퓨터에서 확인한 성공 기록과 새 PC의 실제 결과를 함께 기록한다.

## 8. 남은 작업과 협업 방식

| 순서 | 담당과 다음 판단 |
|---|---|
| 1. 이전 수용 | 사용자가 새 PC에서 팀 팩, 게임 시작과 Editor 시작, 스토리·보스·클리어·재시작을 확인한다. GASP 테스트맵에서 SMG 손·총구 정렬, Shoulder/Scope 구도, 장애물 전후 재입력, 소리·조작감을 직접 확인한다. 기존 HUD·모드 카드 수용은 완료 이력으로 유지한다. |
| 2. 다음 게임플레이 | GASP 조준 수용 후 권총·무기 교체를 같은 테스트 Pawn에 연결할 범위를 검토한다. GASP의 대시·전체 HUD·정식 플레이어 전환은 적용 범위와 회귀 대상을 먼저 정한다. 개별 캐릭터 고유 기술의 실제 효과·쿨다운, 보스 연출·난이도는 기획·플레이 결과로 결정한다. |
| 3. 재현되는 문제 | GASP PoseAsset·발소리 import·반복 디버그 CVar 조회, Level_1 NavMesh origin/tile·종료 시 CrowdManager를 발생 시점과 실제 영향으로 구분한다. 물리 substepping·뼈 영향 수는 성능 측정 후 조정한다. 일반 실행의 로딩 화면과 Lyra 오디오 믹스 Subsystem에는 여전히 진단용 생성 조건이 있다. [로딩 관리자](../../Plugins/CommonLoadingScreen/Source/CommonLoadingScreen/Private/LoadingScreenManager.cpp) · [오디오 믹스](../../Source/DreamCatcher/Audio/DCLyraAudioMixEffectsSubsystem.cpp) |
| 4. 배포 | 사용자가 현재 구조로 Windows 패키징 및 실제 실행을 확인한다. Cook 대상 4맵, 영상 2종, Experience 탐색과 테스트 Experience 포함 여부, 설정·입력·종료를 확인한다. 이 커밋의 최종 패키지 수용, 온라인 복제·전체 장치 검증은 열려 있다. |
| 5. 장기 제작 | 프리·챌린지의 실제 규칙, 결과 보상·강화·해금·영구 진행 저장, 최종 성능 목표와 콘텐츠 범위를 정한다. |

작업 순서는 **요구사항 → TODO와 예상 동작 보고 → 1차 승인 → 항목별 파일·원본·차이·검증 검토 → 최종 확인 → 작성 → 검증 및 사용자 수용**이다. 코드·Config·에셋·문서 모두 적용한다. 최신 사용자 지시와 기능 명세를 먼저 읽고, 과거 이력의 작업 제안은 후속 완료 기록과 대조한다. [프로젝트 작업 규칙](../../AGENTS.md)

- Lyra는 원본 재사용 → 최소 수정 이식 → 필요한 부분 직접 구현 순서로 적용한다. 대체·보류 시 확인한 원본, 제약, 동작 차이와 복구 방법을 먼저 설명한다.
- 에셋 편집·이동·저장은 Unreal 내부 기능을 사용한다. 짧은 속성·배치 작업은 사용자 Editor 조작, 반복적이고 값이 확정된 작업은 승인된 범위에서 자동화한다. 두 팀원이 같은 맵·BP를 동시에 저장하지 않도록 조율한다.
- Codex가 직접 처리하는 것이 기본이다. 독립 작업이 두 개 이상일 때만 최대 세 서브에이전트로 병렬화한다. 화면 조작은 대상·범위가 명시된 질문창 등에서 사용자의 준비와 승인을 확인한다.
- 자동 검사는 변경한 동작과 영향을 받은 경로에 맞추고, 결과에는 실제 빌드·재로드·PIE·사용자 시각 수용 중 어느 단계까지 확인했는지 적는다. 과거의 임시 제작 스크립트는 퇴역한 것이 있어 실행 전에 현재 위치·입력·호출 여부를 확인한다.

다음 채팅에서는 이 문서, 루트 `AGENTS.md`의 0.4와 0.151, [안정성 인계](project-stability-2026-11-17.md)를 먼저 읽는다. 로컬 근거를 함께 옮겼다면 [GASP 조준](../../Saved/Diagnostics/GASPADS_20261117_A/Summary.json), [Traversal](../../Saved/Diagnostics/GASPTraversalCombat_20261117_A/RegressionFixSummary.json), [레벨 진행](../../Saved/Diagnostics/LevelProgression20261023/FinalValidation_A.json), [BIG BROTHER](../../Saved/Diagnostics/BigBrotherBoss20261024/FinalValidation_A.json) 기록에서 재개할 기능의 수용 범위를 확인한다.
