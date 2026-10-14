# DreamCatcher 프로젝트 마무리 작업 인계

2026-10-15 KST 기준. 다음 작업은 **메인화면·아웃게임 씬 추가와 UI 마무리**다. R14, Level_1 전환, 레거시 정리와 맵 복구는 현재 수용 범위에서 완료됐다. 완료한 이식·전체 검증을 처음부터 반복하지 않는다.

**새 채팅에서 읽을 순서**

1. 루트 `AGENTS.md`의 0.4 작업 규칙과 최신 0.114.
2. 이 문서.
3. `docs/handoffs/designer-asset-integration-2026-10-14.md` — 모델·재질·애니메이션·UI 작업 경로.
4. 필요한 기능만 `docs/specs/gas-lyra-migration.md`에서 확인한다. 과거 날짜의 빌드/승인/PIE 대기 기록을 현재 상태로 해석하지 않는다.
5. 정리나 복구 근거가 필요할 때만 `level1-production-cleanup-2026-10-14.md`, `level1-stash-conflict-recovery-2026-10-14.md`를 추가로 읽는다.

**기준 저장소와 현재 상태**

| 항목 | 기준 |
|---|---|
| 저장소 | `Shiny-Shine/DreamCatcher` |
| 로컬 작업 폴더 | `E:\Epic Games\UEProjects\DreamCatcher` |
| 엔진 | UE 5.8, `E:\Epic Games\UE_5.8` |
| 원본 Lyra 조사 위치 | `E:\Epic Games\UEProjects\LyraStarterGame` |
| 확인한 main | `13aa3b0f9b2c312dc303544625166d575cffb8f0` |
| 최근 병합 | PR #2, `codex/restore-level1-assets-20261014` → main, 2026-10-14 23:36 KST |
| 복구 커밋 | `349048a5994d762d42fa1cbbdde227a8bf22416e` |
| 사용자 확인 | 복구 브랜치 커밋·푸시·main 병합 완료, 디자이너 PC에서도 복구 성공 |
| 이번 인계 작업 시작 시 | 로컬 main과 origin/main 일치, 작업 폴더 clean |
| 이번 작업 범위 | 인계·상태 문서만 갱신. 새 기능 구현/에셋/Config/C++ 변경 없음 |

새 채팅에서는 그 사이 추가된 디자이너 커밋과 미커밋 변경을 먼저 읽기 전용으로 확인한다. 위 SHA를 강제로 재설정하거나 이전 작업 브랜치로 되돌리지 않는다. 이 인계 문서는 작성 직후 미커밋일 수 있으므로 보존한다.

**완료한 범위와 유지할 플레이 동작**

- GAS·Lyra 전환은 R14까지의 현재 승인 범위와 PC PIE 사용자 수용을 마쳤다. 원본 Inventory/QuickBar/Equipment, 장착·발사·재장전·조준·대시·사망, 일반 AI 이동/전투, 유한 스포너/인카운터, 보스, HUD·ESC 메뉴·결과 흐름이 연결돼 있다. 모든 장치·온라인·복제·패키징 검증까지 완료했다는 뜻은 아니다.
- 실제 플레이 기준은 디자이너 `Level_1`이다. 환경 배치를 유지하고 구형 플레이 흐름을 검증된 R13 흐름으로 교체했다. **일반 인카운터 2회 → 보스 → 성공 결과**, 플레이어 사망 시 실패 결과가 나온다.
- AI 이동용 Dynamic NavMesh가 연결돼 있고 실제 이동 및 사용자 이동 애니메이션 확인도 완료됐다. 과거의 정지 자세로 미끄러지던 문제를 미해결로 되돌리지 않는다.
- 처치 집계와 객체 정리가 분리돼 있으며 인카운터 완료·실패·소유 AI 정리 경로가 연결돼 있다. 플레이어/적 팀 구분도 현재 스테이지 경로에 있다.
- 플레이어·일반 적의 **자동 리스폰은 없는 것이 의도**다. 결과 화면의 명시적 재시작으로 새 판을 시작한다.
- 보스는 전용 Pawn·페이즈·직선/지면 패턴을 가진 제자리 보스다. 이동 비활성화는 현재 설계다. 전용 AnimBP는 미지정이므로 최종 보스 모션은 후속 연결 대상이다.
- HUD의 보스 체력·현재 페이즈 잘림 문제와 ESC 메뉴 동작은 해결됐고 사용자가 확인했다. 창 크기/해상도·DPI 변경 시 UI 정상 표시도 확인했다.
- **현재 결과/ESC 메뉴의 돌아가기는 같은 Level_1 재시작이다.** 사용자가 메인화면을 만들기 전까지 이렇게 두도록 정했다. 메인화면 추가 후 해당 목적지를 바꾸는 것이 다음 연결 작업이다.
- 실사용·공용 에셋과 회귀 fixture를 포함한 782개를 정식 경로로 옮겼고 승인된 테스트/레거시 묶음을 제거했다. 옛 `BP_DreamCatcherChracter`, `BP_DreamCatcherPlayerController`, 구형 게임모드/HUD, 테스트 플레이어 BP, 구형 플레이어 AnimBP 등은 삭제됐다. **활성 C++ Character/Controller 기반과 회귀 검사는 유지**했다.
- 마지막 잔여 정리 23개도 완료됐다. 공용 Lyra/캐릭터/환경 라이브러리, 디자이너 `Asset.umap` 등은 보존했다. 프로젝트 전체 미사용 에셋이 0이라는 의미는 아니다.
- PR #1의 입력 매핑 등록 조건·다발 탄환 빗나감 결과 리뷰 수정도 병합·PIE 수용 완료다.

**실사용 에셋 경로**

`/Game/`은 콘텐츠 브라우저 `Content/`다. 아래는 현재 경로이며 과거 `GAS/Test`, `LyraMigration`, R11/R12/R13 테스트 맵 경로를 다시 활성화하지 않는다.

| 역할 | 현재 경로 |
|---|---|
| 플레이 맵 | `/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1` |
| 맵 전용 GameMode | `/Game/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode` |
| 스테이지 GameMode 부모 | `/Game/DreamCatcher/Blueprints/Core/BP_DC_StageRunGameMode` |
| Experience | `/Game/DreamCatcher/Experiences/B_DC_StageUIExperience` |
| 공용 PawnData | `/Game/DreamCatcher/Data/Pawns/DA_DC_LyraAIPawn` |
| 실제 플레이어·일반 봇 Pawn | `/Game/DreamCatcher/Blueprints/Player/BP_DC_LyraInventoryPawn` |
| Pawn 부모 | `/Game/DreamCatcher/Blueprints/Player/BP_DC_RiflePawn` |
| 플레이어 Controller | `/Game/DreamCatcher/Blueprints/Player/BP_DC_StageUIController` |
| Controller BP 부모 | `/Game/DreamCatcher/Blueprints/Player/BP_DC_LyraAIPlayerController` |
| 일반 적 스포너 | `/Game/DreamCatcher/Blueprints/AI/BP_DC_LyraFiniteSpawner` |
| 스테이지·인카운터 | `/Game/Blueprint/Stage/BP_StageDirector`, `/Game/Blueprint/Stage/BP_EncounterController` |
| 보스 | `/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss`, `/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_BossSpawner` |
| 보스 데이터 | `/Game/DreamCatcher/Data/Boss/DA_DC_Boss` |
| 라이플 | `/Game/DreamCatcher/Lyra/Weapons/Rifle` — `ID_Rifle`, `WID_Rifle`, `B_Rifle`, `B_WeaponInstance_Rifle`, 발사/재장전 GA |
| 캐릭터 기본 애니메이션 | `/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base` |
| 장착 동작 레이어 | `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers` |

Pawn 상속은 `BP_DC_LyraInventoryPawn → BP_DC_RiflePawn → ADreamCatcherCharacter`다. 플레이어와 일반 봇이 같은 PawnData/Pawn을 사용하므로 **플레이어 모델만 교체하려면 적용 경로 분리가 필요**하다. 별도 Pawn BP 두 개를 만드는 것만으로 생성 경로가 분리되지는 않는다.

디자이너의 `SM_GUN_SMG`/`Gun_Pistol` 등은 Static Mesh이고 장착 라이플은 Skeletal Mesh다. 모델 교체에는 리깅 또는 별도 표시 연결과 그립·총구·재장전 대응이 필요하다. 자세한 대상 필드는 디자이너 안내 문서를 따른다.

**메인화면과 UI 작업의 연결 지점**

현재 실행 가능한 메인화면·아웃게임 씬은 아직 만들지 않았다. 새 맵과 화면의 구성·버튼·3D 연출 범위는 다음 작업에서 정한다. 캐릭터 선택·성장·상점·세이브 슬롯·온라인 기능을 이번 요청에 포함된 것으로 임의 가정하지 않는다.

| 연결 | 현재 위치와 동작 |
|---|---|
| UI 정책/루트 | `/Game/DreamCatcher/Lyra/UI/Shared/Foundation/B_LyraUIPolicy`, `/Game/DreamCatcher/Lyra/UI/Shared/Foundation/W_OverallUILayout` |
| HUD 레이아웃 | `/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout` |
| ESC 메뉴 | `/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_LyraGameMenu` |
| 설정 패널 | `/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Settings/W_SettingsPanel` |
| HUD/알림 등록 | `/Game/DreamCatcher/Experiences/ActionSets/LAS_DC_StageHUD`, `/Game/DreamCatcher/Experiences/ActionSets/LAS_DC_StageNotifications` |
| UI GameFeature | `Plugins/GameFeatures/DCStageUI/Content/DCStageUI.uasset` |
| UI 시스템 | `Source/DreamCatcher/UI/Subsystem/DCUIManagerSubsystem.*`, `Source/DreamCatcher/System/DCGameInstance.*` |
| 스테이지 복귀/재시작 | `Source/DreamCatcher/GameModes/DCStageRunGameMode.*` |
| 메뉴 복귀·입력 차단/복구 | `Source/DreamCatcher/UI/Stage/DCStageUIComponent.*`, `DCStageUIBlueprintLibrary.*` |
| 페이즈·결과 표시 | `Source/DreamCatcher/UI/Stage/DCStageOverlayWidget.*`, `DCStageResultLayer.*`, `DCStageResultWidget.*` |
| 보스 체력 | `Source/DreamCatcher/UI/Boss/DCBossHealthWidget.*` — 현재 C++ 위젯 |

`RequestRestartStage()`는 현재 맵을 다시 열고, `RequestExitStage()`는 GameMode의 `ReturnMap`을 연다. `RequestExitStageFromMenu()`는 진행 중인 판을 종료한 뒤 같은 복귀 경로를 호출한다. 결과 및 ESC 메뉴의 정리/입력 복구를 유지하면서 `BP_DC_Level1GameMode.ReturnMap`을 새 메인화면 맵으로 바꾸는 연결을 검토한다. 현재 여행 처리는 standalone에 한정되며 별도의 온라인 구조로 확대하지 않는다.

현재 Config는 다음과 같다.

- `Config/DefaultEngine.ini`: `GameDefaultMap`과 `EditorStartupMap`은 모두 Level_1. `GlobalDefaultGameMode`는 `Engine.GameModeBase`, `GameInstanceClass`는 `DCGameInstance`다. 메인화면의 실행/편집 시작 맵 변경 여부를 구분해서 제안한다.
- 같은 파일의 `GameUserSettingsClassName=/Script/DreamCatcher.DCLyraSettingsLocal`은 Experience/설정 메뉴의 기존 필수 연결이다. 기본 엔진 클래스로 돌리지 않는다.
- `Config/DefaultGame.ini`: Level_1을 `MapsToCook`에 등록했고 `/Game/Tests`는 NeverCook이다. UI 정책과 Experience/ActionSet 스캔도 정식 경로를 사용한다. 새 메뉴 맵/Experience를 만들면 필요한 등록을 승인 범위에 포함한다. Cook 목록 준비와 실제 패키징 실행은 구분한다.

**다음 작업 제안 순서**

1. 최신 Git 상태와 위 연결 지점을 확인한다. 기존 CommonUI/설정 메뉴와 원본 Lyra에서 재사용할 수 있는 메인화면 구성을 조사해 변경안을 먼저 설명한다. 새 기능 작업 브랜치는 `codex/` 접두사를 사용하며 main에 직접 커밋하지 않는다.
2. 메인화면·아웃게임 씬의 역할과 필요한 버튼/연출을 사용자와 정한다. 새 맵/UI, 기존 메뉴 복귀, 시작 맵/등록 Config 등 연관된 코드·에셋 범위를 한 묶음으로 제시한다. 이번 인계 요청 자체는 그 구현 승인까지 포함하지 않는다.
3. 승인 후 `메인화면 → Level_1 → 결과 또는 ESC 메뉴 → 메인화면` 흐름을 연결한다. 결과의 재시작은 Level_1 새 판 시작으로 유지한다. 메인화면에서 스테이지나 봇이 자동 실행되지 않도록 초기화 범위를 구분한다.
4. 디자이너 아트와 UI 수정을 연결한다. 기존 설정 Apply/Cancel, 메뉴/모달 포커스, 마우스 커서와 이동·발사 입력 차단/해제, 보스/결과 표시 수명, 화면 크기 대응을 보존한다.
5. 변경 영향 범위만 자동검사하고 사용자는 필요한 C++ 빌드 및 짧은 화면·조작 확인을 한다. 실제 패키징은 사용자가 다시 진행하겠다고 할 때 수행한다.

**작업 규칙과 이미 완료한 검증**

- 사용자 담당: C++ 빌드, 패키징, 조작감·화면·연출 확인, GitHub Desktop 커밋·푸시·병합. Codex는 이번 인계에서 새 채팅 생성·메시지 전송·Git 조작을 하지 않는다.
- Codex 담당: 원본 조사, 승인된 코드/Config, Unreal 내부 에셋 자동화와 영향 검사. `원본 재사용 → 최소 수정 이식 → 불가피한 부분만 직접 구현` 정책 유지.
- 에셋 덮어쓰기/삭제/참조 교체는 구체적 범위를 설명해 승인받는다. `.uasset`/`.umap`을 텍스트로 편집하거나 헤더를 치환하지 않는다. 저장 전 에디터 잠금·미저장 변경을 확인하며 사용자 에디터를 임의 종료하지 않는다.
- 이미 완료한 검증: 10월14일 21:57 사용자 Editor 빌드, 영향 BP11개 검사, 핵심 회귀22건, Level_1 실제 PIE 이동/발사/조준/사망/정리/인카운터/보스/복귀, 사용자 최종 정상 확인. 이어 잔여23개 정리 검사 및 복구 파일/신규10개 에셋 로드도 통과했다.
- 현재 DLL SHA256: `f863512bb8eb923a450e349f692f7e4eb21b618290ec9a1b3b8505d6710bffaf`. 인계 시 맵/재질/DLL 해시만 확인했으며 완료된 엔진/PIE 검증을 재실행하지 않았다.
- 알려진 경계: 기존 Manny PoseAsset와 Nav/Crowd 초기화 경고, DataRegistry 명시적 의존성 경고 등은 과거 인계에 남아 있다. 새 경로에서 재현되는 영향이 있을 때 다룬다. 패키징·전체 장치·온라인/복제 검증을 완료로 확대하지 않는다.
- `/Saved/Diagnostics`, `/Saved/Automation`, 로그는 이 PC의 로컬 근거이며 Git에서 공유되지 않을 수 있다. 정식 인계 문서와 경로 JSON을 재개 기준으로 삼는다. 유지된 실제 검사 스크립트는 `Scripts/Editor/level1_gameplay_probe.py`다. 정리로 제거한 이식 도구를 다시 만들거나 설치하는 작업부터 시작하지 않는다.

**복구 사건의 최종 판정**

23:08 병합 `57200f8`은 정상 정리본에 환경 에셋17개만 더한 상태였다. 뒤의 `7f53325`에서 Stash 충돌 텍스트가 Level_1과 MI_Concrete_3의 실제 LFS payload로 올라가 에셋이 읽히지 않았다. 이 두 파일만 정상본으로 되돌렸고 신규 디자인10개를 유지했다. 사용자가 `349048a`를 커밋·푸시하고 PR #2를 main에 병합했으며 **디자이너도 복구 성공을 확인**했다. 현재 맵 복구·병합은 종료된 작업이므로 다시 복구 브랜치를 만들지 않는다.

충돌 텍스트가 가리키던 디자이너 별도 Stash 원본2개의 확보는 확인되지 않았다. 당시 원격 LFS가 404였고 이 PC에 원본이 없었다. 사용자의 복구 성공 확인을 그 별도 원본까지 되찾았다는 뜻으로 확대하지 않는다. 식별 정보는 `level1-stash-conflict-recovery-2026-10-14.md`에 보존돼 있다. 기존 Stash/백업을 건드리지 않고 후속 UI 작업을 진행하며, 새 단서나 요청이 없으면 이 조사를 다시 시작하지 않는다.

**새 채팅 첫 메시지**

```text
DreamCatcher의 AGENTS.md 최신 0.114와
docs/handoffs/project-finish-resume-2026-10-15.md,
docs/handoffs/designer-asset-integration-2026-10-14.md를 읽고 이어서 진행해줘.

R14, 디자이너 Level_1의 R13 플레이 흐름·NavMesh 전환, 정식 경로 이관과
레거시 정리, 맵/재질 복구는 완료했고 PR #2가 main에 병합됐어.
디자이너도 복구 성공을 확인했어. 완료한 검증·마이그레이션·복구를 반복하지 마.

이제 메인화면 및 아웃게임 씬 추가, UI 수정 등 마무리 작업을 할 거야.
기존 Level_1 환경·플레이어·전투·AI·보스·새 디자인 에셋은 유지해줘.
현재 돌아가기는 Level_1 재시작이고, 메인화면 추가 후 그곳으로 돌아가게 연결해야 해.
먼저 현재 상태와 재사용 가능한 Lyra/CommonUI 구성을 확인하고,
필요한 화면 구성·변경 범위를 설명해줘. 구현은 그 작업 묶음을 승인한 뒤 진행해줘.
빌드·패키징·화면 확인과 GitHub Desktop 커밋·푸시·병합은 내가 할게.
```
