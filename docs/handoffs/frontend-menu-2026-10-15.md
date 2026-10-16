# 2D 메인메뉴 구현 인계 — 2026-10-15

현재 상태는 **사용자 C++ 빌드 성공, 신규 메뉴 에셋7개 생성·재로드·실행/복귀 연결과 실제 PIE 왕복 확인 완료, 사용자 화면·물리 입력 확인 대기**다. 초기 재진입 검사에서 발견한 탄약 위젯 오류는 별도 사용자 승인을 받아 해당 위젯1개만 보완했다. **일반 실행의 CommonLoadingScreen 활성화는 별도 미완료**이며 아래 제한을 따른다. R14, Level_1 전환·정리·복구를 재개하거나 전체 검증을 반복하지 않는다.

## 지금 이어서 할 일

사용자는 콘텐츠 브라우저에서 `/Game/DreamCatcher/Maps/L_DC_FrontEnd`를 열어 PIE로 화면과 마우스·키보드 조작을 확인한다. EditorStartupMap은 기존 Level_1을 유지했으므로 에디터를 켰을 때 자동으로 새 메뉴 맵이 열리지는 않는다. 기본 게임 실행 맵(GameDefaultMap)은 새 메뉴다.

- 스토리 카드 클릭/Enter → Level_1 진입.
- 설정 열기·뒤로, 종료 확인창 취소.
- ESC 메뉴의 복귀 → 새 메뉴. 결과의 `다시 시작` → Level_1 새 판, `메인화면으로` → 새 메뉴.
- 카드 삽화는 아직 미지정이고 최종 폰트·아트·연출 확인은 남아 있다. 새 화면의 해상도/포커스/물리 입력 수용을 자동 검사 완료로 대신하지 않는다.

추가 C++ 수정은 이번 후속에서 하지 않았으므로 재빌드를 요청할 이유가 없다. 커밋·푸시·병합·패키징은 계속 사용자 담당이다.

## 승인과 화면 범위

- 사용자는 3D 배경/이동 로비를 제외하고 첨부 `Slide 16_9 - 14.png`의 2D 카드·버튼 방식으로 진행하도록 정정했다. 후속으로 **프리 모드는 우선 건너뛰고 진행**하도록 승인했다.
- 첫 묶음은 스토리 카드·설정·종료, `메뉴 → Level_1 → 결과/ESC → 메뉴`다. 결과의 재시작은 Level_1 새 판 시작을 유지한다. 프리 모드 카드나 실행 기능은 추가하지 않는다.
- 별도 타이틀 페이지가 정해지지 않아 **모드 선택 구성을 실행 첫 화면으로 사용**하는 가정을 안내했다. 추가 페이지는 별도 범위다.
- 초안은 배치 방향의 참고 이미지다. 이미지 전체를 배경으로 붙이거나 그림을 잘라 최종 아트로 사용하지 않았다. 카드의 Artwork·번호·영문 라벨·제목·설명은 교체 가능한 속성이고 배치는 UMG 위젯 트리로 생성한다. 첫 생성안은 Artwork 미지정의 보라색 카드다. 최종 삽화·폰트·연출 완성 상태가 아니다.
- 빌드·패키징·화면 확인과 GitHub Desktop 브랜치/커밋/푸시/병합은 사용자 담당이다. 이번 작업은 로컬 main `819ac19`에서 시작했으며 **기존 Level_1.umap 미커밋 변경을 보존**했다. 직접 커밋·푸시·병합은 하지 않았으며 마지막 스테이징 상태는 아래에 기록했다.

## 확인한 원본과 적용 차이

원본 위치는 `E:/Epic Games/UEProjects/LyraStarterGame`이다. 아래 6개를 Unreal 내부 API로 읽어 부모·그래프 노드/핀 연결·CDO·직접 의존성을 확인했다. 에셋 저장/명시적 Compile 0, 대상 6개 파일 해시 불변, 명령줄 종료 0이다. 첫 시도는 보호된 ParentClass 직접 조회에서 중단했고 공개 `GetBlueprintParentClass` API로 교정한 두 번째 실행이 완료됐다.

- `/Game/UI/Menu/W_LyraFrontEnd`
- `/Game/UI/Menu/Experiences/W_ExperienceSelectionScreen`
- `/Game/UI/B_LyraFrontendStateComponent`
- `/Game/System/FrontEnd/B_LyraFrontEnd_Experience`
- `/Game/UI/FrontEnd/W_FrontEndHUDLayout`
- `/Game/UI/Menu/W_LyraExperienceTileButton`

로컬 근거는 `Saved/Diagnostics/Frontend/Source_B/report.json`과 같은 폴더의 T3D다. 이 조사는 원본 소스/그래프 확인이며 새 메뉴 런타임 검증이 아니다.

| 원본 | 재사용 및 이번 차이 |
|---|---|
| `LyraFrontendStateComponent.h/.cpp` | 이름/include/Experience 타입 대응 후 재사용. Experience 대기·사용자 초기화·CommonUI Menu push를 유지한다. PC 메뉴에는 별도 Press Start 화면이 없으므로 해당 클래스가 비어도 원본 로컬 사용자 초기화를 실행한다. 월드 종료 시 자신의 비동기 흐름/사용자·세션 delegate/소유 화면만 정리하도록 보강했다. 원본의 요청 세션 분기는 남아 있지만 온라인 UI·온라인 검증을 추가한 것은 아니다. |
| `ApplyFrontendPerfSettingsAction.h/.cpp` | 이름과 SettingsLocal 타입만 대응했다. 메뉴 진입/종료 시 카운트 기반 설정 적용·해제 본문은 원본과 같다. |
| 기존 UI 정책·루트·`DCButtonBase`·`W_LyraMenuButton` | 다시 이식하지 않고 현재 정식 경로를 재사용한다. 카드의 아트/라벨/배치와 선택 테두리는 초안에 맞는 신규 UMG 구성이다. |
| `W_LyraFrontEnd` / `W_ExperienceSelectionScreen` | 실제 그래프는 Credits/Replays/Logout, 온라인 Login/QuickPlay/Host/SessionBrowser를 포함한다. 사용자 요청의 단일 스토리 진입 화면과 달라 전체 화면을 복사·삭제해서 만들지 않고, 이미 이식된 버튼·ActivatableWidget·설정/확인창 기반 위에 새 화면을 구성한다. 원본 이식 불가 판정이 아니라 승인된 화면/기능 범위 차이다. |
| `B_LyraFrontEnd_Experience` | 원본은 Frontend State + 음악 컴포넌트, 성능 설정, 통계 위젯, 분할화면 액션을 포함한다. 별도 DreamCatcher Experience에는 이번 화면에 필요한 State/성능 설정만 연결한다. 메뉴 음악·통계·분할화면은 요청 범위에 추가하지 않았다. 원본 에셋은 수정하지 않는다. |
| 설정 | 외부 활성 화면은 `W_LyraSettingScreen`이다. `W_SettingsPanel`은 그 내부 패널이며 단독으로 Menu 스택에 넣지 않는다. Apply/Cancel/Back은 기존 화면의 동작을 유지한다. |
| 로딩 | 현재 프로젝트에 CommonLoadingScreen은 있으나 Config의 LoadingScreenWidget 지정은 없다. 신규 2D 로딩 위젯을 같은 수명 관리에 연결하도록 준비했다. 새 로딩 시스템은 만들지 않는다. |

## 작성한 코드

- `Source/DreamCatcher/UI/Frontend/DCLyraFrontendStateComponent.*`: 원본 메뉴 초기화 흐름과 위 종료 처리.
- `Source/DreamCatcher/UI/Frontend/DCApplyFrontendPerfSettingsAction.*`: 원본 메뉴 성능 설정 Action.
- `Source/DreamCatcher/GameModes/DCFrontendGameMode.*`: CommonPlayerController와 기본 PlayerState, Pawn/관전자 미생성, 스토리 목적지 검증·다음 tick 이동·중복 클릭 차단·이동 실패 시 재시도 허용.
- `Source/DreamCatcher/UI/Frontend/DCFrontendScreen.*`: UMG의 스토리/설정/종료 버튼 연결, 설정 화면의 소유 수명, 공통 종료 확인창. 첫 화면 ESC는 종료 확인이며 설정 화면 ESC는 기존 뒤로 동작이다.
- `Source/DreamCatcher/UI/Frontend/DCModeCardButton.*`: 기존 Lyra 버튼 기반의 편집 가능한 카드 데이터와 hover/focus 테두리.
- `Source/DreamCatcher/Tests/DCFrontendTestTypes.h`, `DCFrontendAutomationTests.cpp`: 실제 맵을 열지 않는 travel 중복/실패/재시도와 메뉴의 전투 생성 차단 검사 2건. **사용자 빌드 후 성공2/실패·보고서경고0**으로 통과했다.
- `DreamCatcher.uproject`, `Source/DreamCatcher/DreamCatcher.Build.cs`: 원본 흐름의 엔진 ControlFlows 플러그인/모듈 연결.
- `Source/DreamCatcher/UI/Stage/DCStageResultWidget.cpp`: 복귀 버튼 문구를 `메인화면으로`로 변경. 게임 규칙 변경 없음.

현재 게임 GameMode는 Experience의 PawnData가 비어 있으면 AssetManager의 전투 PawnData를 선택한다. 새 메뉴는 별도 GameMode에서 전투 PlayerState의 PawnData 수신 경로를 사용하지 않고, Pawn 클래스 선택과 재시작도 명시적으로 차단한다. 기존 게임 GameMode/Controller/PlayerState/PawnData는 수정하지 않았다.

## 에셋 자동화 — 생성·재로드·연결 완료

`Scripts/Editor/frontend_menu_setup.py`는 UE 5.8에 포함된 `UMGToolSet`의 공개 API를 별도 프로세스에서 사용한다. 새 Editor 플러그인이나 범용 이식 도구를 만들지 않았다. 선행 API 확인 후 실제 신규7개 생성/Compile/저장, 별도 프로세스 재로드, 기존 GameMode/Config 연결을 완료했다. 초기 생성2회는 Python API 노출 차이(텍스트 정렬 setter, GameFeature 타입명)로 **저장 전** 중단됐고 공개 속성/클래스 조회와 기존 배열 API로 교정했다. 실패 보고서는 보존했고 기존 파일은 불변이었다.

신규 경로는 정확히 7개다.

| 역할 | 경로 |
|---|---|
| 스토리 카드 | `/Game/DreamCatcher/UI/Frontend/W_DC_StoryModeCard` |
| 메인화면 | `/Game/DreamCatcher/UI/Frontend/W_DC_Frontend` |
| 로딩 표시 | `/Game/DreamCatcher/UI/Frontend/W_DC_LoadingScreen` |
| 원본 상태 컴포넌트의 프로젝트 설정 | `/Game/DreamCatcher/Blueprints/UI/BP_DC_FrontendStateComponent` |
| 메뉴 Experience | `/Game/DreamCatcher/Experiences/B_DC_FrontEndExperience` |
| 메뉴 GameMode | `/Game/DreamCatcher/Blueprints/Core/BP_DC_FrontEndGameMode` |
| UI 실행용 빈 맵 | `/Game/DreamCatcher/Maps/L_DC_FrontEnd` |

스크립트 단계는 `-DCFrontendMode=create|verify|connect`로 명시하며, 각 단계는 별도 unattended 명령줄 에디터에서 실행한다. `-EnablePlugins=UMGToolSet`은 해당 프로세스에만 적용하며 프로젝트 플러그인 목록에는 추가하지 않았다. 후속 실행은 **`-SCCProvider=None`을 명시**하고 실제 SourceControl 비활성 상태도 검사한다. `-NoSourceControl`만으로 자동 등록 방지를 가정하지 않는다.

1. **create:** 기존 목적지·dirty 패키지가 있으면 중단. 신규 에셋만 생성/Compile/저장한다. 현 Level_1/GameMode/Config/DLL의 해시 불변을 확인한다.
2. **verify:** 새 프로세스에서 7개 재로드, 부모·필수 위젯·Experience/설정/맵 연결 확인. 에셋 저장 없음. 이후 단계용 해시 기록.
3. **connect:** verify의 에셋/DLL 해시와 현재 값이 같을 때만 기존 `BP_DC_Level1GameMode.ReturnMap`을 새 맵으로 변경한다. `DefaultEngine.ini.GameDefaultMap`, `DefaultGame.ini.MapsToCook` 추가와 CommonLoadingScreen 위젯 지정만 반영한다. `EditorStartupMap`은 Level_1 유지. Level_1.umap은 저장하지 않는다.

각 단계는 같은 이름의 기존 보고서가 있으면 덮어쓰지 않고 중단한다. 실패 보고서와 부분 생성물이 있으면 원인을 확인한 뒤 복구 방식을 정하고, 삭제·덮어쓰기로 재실행하지 않는다. 실제 실행 전 다른 에디터의 저장/미저장 상태와 프로세스 충돌을 다시 확인한다.

**이미 생성·연결됐으므로 create/connect를 다시 실행하지 않는다.** 현재 기존 에셋 수정은 `BP_DC_Level1GameMode.ReturnMap`과 아래 추가 승인된 탄약 위젯뿐이다. `DefaultEngine.ini.GameDefaultMap`은 새 메뉴, `DefaultGame.ini.MapsToCook`은 Level_1+새 메뉴이며 LoadingScreenWidget 경로도 등록했다. 메뉴 Experience는 기존 정식 스캔 폴더 아래라 스캔 범위를 추가하지 않았다.

## 재진입 탄약 위젯 보완 — 추가 승인 및 적용 완료

PIE B는 메뉴→스토리→ESC 복귀→재진입→결과 재시작→결과 복귀를 모두 실행했지만, 두 번째 진입의 장비 연결 전 `W_AmmoCounter_Rifle`에서 `InventoryInstance` Accessed None 오류2건이 발생해 전체 실패로 판정했다. 성공한 버튼 동작과 오류 없는 통과를 구분했다.

읽기 전용 그래프 확인에서 Event Tick→재질 갱신 경로가 유효성 검사 없이 MagazineAmmo를 읽고, OnWeaponInitialized 경로도 MagazineSize를 직접 읽는 것을 확인했다. 사용자가 **탄약 위젯1개 보완을 별도 승인**했다.

- 대상은 `/Game/DreamCatcher/Lyra/Weapons/Rifle/W_AmmoCounter_Rifle` 하나다.
- `Scripts/Editor/frontend_ammo_guard.py`로 Tick/무기 초기화 앞에 각각 `IsValid(InventoryInstance) → Branch`를 추가했다. 유효할 때는 기존 실행 흐름, 무효일 때만 해당 갱신을 건너뛴다.
- 추가4노드와 허용한6개 기존 핀의 연결 외 원본 노드/핀/기본값 동일, 위젯 CDO 기본값 동일을 검사했다. 표시 수치·태그·재질·무기 로직을 바꾸지 않았다.
- 해당1개만 Compile/저장한 뒤 별도 프로세스 재로드/Compile/그래프 비교를 통과했다. C++/DLL 변경 없음.
- 보완 후 PIE C에서 첫 진입·메뉴 후 재진입·결과 재시작 모두 실제 인벤토리 30/30과 Bar/Glow의 Rounds_Current/Rounds_Max/IsLowAmmo가 일치했다.

## 확인 근거와 남은 제한

- 첫 C2027/C2672는 검사 CPP의 `GameFramework/SpectatorPawn.h` include 추가로 해결됐다. 사용자 빌드 성공을 확인했고 DLL은 **2026-10-15 18:26:28 KST, 5,848,576 bytes**, SHA256 `dd465a67a2a6c8211f7a20dde16ca0cfd9682cb854fc9330c33fdca095a9b429`다. Codex는 C++ 빌드/패키징을 실행하지 않았다.
- native2건: `Saved/Automation/Frontend_A/index.json`, 성공2/실패·보고서경고0.
- 생성/재로드/연결: `Saved/Diagnostics/Frontend/create.json`, `verify.json`, `connect.json`, 모두 complete=true. 각각 에셋 저장7/0/1개.
- 탄약 보완: 같은 폴더 `ammo_guard_apply.json`, `ammo_guard_verify.json`, complete=true, 에셋 저장1/0개, 보호 파일 불변.
- 실제 PIE: `Scripts/Editor/frontend_menu_probe.py`, 같은 폴더 `pie_C.json`과 `pie_C.log`. **complete=true, 실패0, Error0, ensure0, 에셋 저장0, 사용자 SaveGames 불변**이다. 검사 도중 성공/실패 판정에 필요한 결과 화면은 `FailStage()`로 유도했다. 전투/보스 전체 클리어를 다시 플레이한 검사가 아니다.
- 기존 Level_1의 사용자 미커밋 파일은 작업 전후 SHA256 `c075fae64f043905b7adcd853d03682a0908bd7b014b4d96a7ddf7eae78e42a7`로 불변이다. 새 메뉴/설정과 보완 탄약 위젯 등 보호 파일도 PIE에서 저장되지 않았다.
- 최초 PIE A는 로딩 관리자가 항상 존재한다는 검사 가정 때문에 중단됐고, B는 위 실제 탄약 오류로 실패했다. 두 실패 보고서를 보존하며 C의 결과로 구분한다.
- 기존 Manny PoseAsset, Navigation/Crowd 초기화 등 경고는 해당 실행 로그에 남는다. 모든 장치·패키징·온라인/복제·물리 키보드/마우스·최종 아트/해상도 수용까지 완료한 것은 아니다.
- 전체 PIE 종료 보고서 `Saved/Automation/FrontendPIE_C/index.json`은 **2026-10-15 19:08:48 KST, 성공1(경고 포함)/실패0**이다. 경고20건은 기존 PoseAsset14/Nav3/CrowdFollowing3이며 전체 종료 로그에서도 Error0/ensure0을 확인했다.
- 마지막 Git 상태에서 신규 위젯/BP6개는 스테이징돼 있고 신규 맵은 untracked다. 에디터 에셋 저장 과정의 자동 등록 가능성이 있어 후속 스크립트에 위 SourceControl 비활성 확인을 추가했다. 원인 확정이나 사용자 작업 여부를 단정하지 않고 인덱스를 임의 되돌리지 않았다. 직접 Git 변경 명령·커밋·푸시·병합은 실행하지 않았다. GitHub Desktop에서 새 맵/에셋/설정/소스/문서를 함께 검토한다.

### 일반 실행의 로딩 표시 — 별도 미완료

`Plugins/CommonLoadingScreen/Source/CommonLoadingScreen/Private/LoadingScreenManager.cpp::ShouldCreateSubsystem()`에는 `DCLyraRuntimeDiagnostics::IsEnabled()` 조건이 남아 있다. 해당 조건은 **Editor + 비Commandlet + unattended + 명시적 진단 플래그**일 때만 참이며, 일반 PIE와 패키징 빌드에서는 거짓이다. 이번 일반 실행 조건의 PIE에서는 관리자가 없었고, 이를 검사 결과에 `disabled_by_existing_diagnostic_gate`, `loading_screen_verified=false`로 기록했다.

신규 로딩 위젯과 Config 등록은 준비됐지만 **실제 일반 실행의 로딩 화면이 활성화됐다는 뜻은 아니다.** 메뉴 왕복은 이 조건 변경 없이 통과했다. 이 전역 플러그인 조건의 활성 경로 전환은 이번7개/탄약1개 에셋 승인 범위로 조용히 확대하지 않았다. 필요하면 CommonLoadingScreen에 한정한 별도 변경안을 설명하고 사용자 C++ 빌드를 거쳐 진행한다. 다른 오디오/설정 진단 조건을 일괄 해제하지 않는다.
