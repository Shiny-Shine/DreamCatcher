# R14 정리 및 Level_1 정식 전환 조사

**2026-10-15 현재:** 이 조사를 바탕으로 한 Level_1 전환·정식 경로 이관·레거시 정리 및 후속 맵 복구는 사용자 수용/main `13aa3b0` 반영까지 완료됐다. 새 재개 지점은 [마무리 작업 인계](project-finish-resume-2026-10-15.md)다. 아래 삭제/변경 제안·검증 대기는 당시 이력이며 같은 작업을 반복하지 않는다.

## 이력 — 잔여 플레이어 애니메이션·입력·블록아웃 정리 완료 / AGENTS 0.113

사용자 승인 후 옛 플레이어 AnimBP/레이어/롤5개, 옛 입력·Touch9개, Test_Map 블록아웃/전용 generated 메시9개를 추가 삭제했다. 최신 외부 참조0과 복구 근거를 확인했고22:49 새 프로세스에서 끊어진 참조0/Error·ensure0으로 검증했다. Level_1과 실사용 에셋·Source·Config·DLL은 그대로이며 기존 사용자 수용을 유지한다. 현재 입력 Action5개, 디자이너 Asset 맵과 연결 메시, 보스/실맵에서 사용하는 Mannequins 자료 및 회귀 검사는 보존한다. 추가 빌드·PIE 반복은 필요 없고 패키징은 계속 보류다. [정확한 정리 범위와 인계](level1-production-cleanup-2026-10-14.md)를 따른다. 프로젝트 전체 미사용 라이브러리가 0이라는 판정은 아니다.

---

## 최신 적용 — Level_1 R13 흐름·NavMesh 연결 / AGENTS 0.109

사용자는 첫 전환 묶음을 승인하고 **돌아가기를 같은 맵 재시작으로 임시 처리**하도록 정했다. 추후 메인화면이 만들어지면 ReturnMap을 그 맵으로 바꾼다. 아래 정리 후보 전체를 삭제하거나 R13 플레이어/무기/AI 자산을 옮기는 작업은 아직 하지 않았다.

| 변경 | 적용 내용 |
|---|---|
| `Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap` | 구형 `BP_EnemyCharacter_C_1` 및 Engine `PlayerStart_1` 제거/교체. Lyra 시작점, 일반전투2회→보스→완료의 StageDirector/Encounter/스포너, NavMesh 연결 |
| 새 `Content/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode.uasset` | R13 GameMode 복제본. Controller/Experience/HUD/팀/결과 기본값은 동일, ReturnMap만 Level_1. 기존 R13 BP는 변경하지 않음 |
| `Config/DefaultEngine.ini` | GameDefaultMap을 Level_1으로 지정. EditorStartupMap은 이미 Level_1이며 그대로 유지 |
| `Config/DefaultGame.ini` | 명시적 MapsToCook를 Level_1로 지정. 기존 Experience/ActionSet/Cue 검색·원본 자산은 유지. 실제 Cook/패키징은 미실행 |
| 자동화2파일 | `Scripts/Editor/level1_r13_flow_setup.py`, `level1_r13_flow_probe.py`. 승인된 맵/BP만 저장하며 원본 이식/전체 Blueprint 검사를 반복하지 않음 |

**환경 보존:** 원래4,644개 Outliner 액터 중 교체 대상2개를 제외한 환경4,642개의 클래스·위치/회전/크기를 저장 전후 및 새 프로세스 재로드에서 비교해 동일함을 확인했다. 환경 에셋/건물/장식/유효 BPP_NewMap은 수정하지 않았다. 복구 기준은 main의3665f3b 및 검증한 원본 Level_1 LFS payload이며 `before_A.json`에 기록했다.

**배치:** 기존 시작점 위치/회전/크기를 유지했다. 현재 저장된 바닥을 trace하고 AI 캡슐과 충돌하지 않는 위치를 확인하여 일반 스폰을(335,165,81), (635,165,81), 보스를(1035,165,73)cm에 배치했다. R13과 같은 시작 구역의 기능 연결이며, 디자이너가 추후 맵 전역의 전투 위치/간격을 조정할 수 있다. R13 원본의 일반전투2회→보스→Complete, 각 스포너1명, 자동 리스폰 없음, HUD/메뉴/결과 흐름을 유지한다.

**NavMesh:** 맵에서 충돌하는 환경4,251개의 합집합 bounds에500cm 여유를 주었다. 범위는 min=(-22982.94,-9661.08,-1780), max=(35270,27222.59,20012.91)cm, RuntimeGeneration=Dynamic이다. 지하 터널과 높은 환경도 범위에 포함한다. 전체 표면의 보행 가능/성능 수용까지 보증하는 것은 아니며, 전투 지점3개의 실제 AI 이동을 확인했다.

**자동검증 최종 결과:** 새 프로세스의 저장본 검증 `setup_Verify_A.json` 성공. `Level1Flow_D`는19:49 완료, 성공1/실패0/Error·ensure·Fatal0/프로세스0이다. 경고17은 Manny PoseAsset14개와 각 월드 초기화의 Recast/Crowd3개이며 별도로 유지한다.

- 실제 Experience/HUD/라이플 준비, 입력 주입 발사30→29, 견착 진입/Hip 복귀.
- AIController의 실제 MoveTo 요청3건(RequestSuccessful), 부분 경로 허용=false, 전투 지점3개 도달(종점100cm 이내, 구간 변위 합992.85cm). 이 동안 원본 BT를 일시 중지했다가 복구한 진단이며 자연 행동/시각은 사용자 확인과 구분한다.
- 진단 GAS 피해로 일반전투2회→보스/체력 HUD→클리어 결과와 객체 정리.
- 결과 상태의 공개 Exit API로 같은 Level_1 재로드, 새 실행/체력/이동 입력 복구(204.63cm).
- 원본 메뉴 ReturnButton→확인창 버튼 경로로 같은 Level_1을 다시 열고 정상 새 실행 확인.
- Unreal에서 읽은 실효 GameDefaultMap과 MapsToCook도 Level_1 일치. 사용자 SaveGames 변경0, 에셋 저장0, 보호 파일 변화0.

**검사 실패 이력은 보존한다:** PIE A/B의 Python NavigationSystem 정적 UFunction 호출은 CDO의 World 조회 ensure를 유발했다. A의 결과 UI private Content 조회도 실패했다. C는 SoftObjectPath를 str로 검사해 초기 assertion에 실패했다. 실제 AIController의 경로 추종과 기존 export_text 방식으로 교정한 D만 최종 통과 근거로 사용한다. 이 과정에서 게임 C++/에셋을 다시 바꾸거나 ensure를 허용하지 않았다.

근거는 `Saved/Diagnostics/Level1Integration/`의 `before_A.json`, `placement_A.json`, `setup_Apply_A.json`, `setup_Verify_A.json`, `pie_D.json`, `completion_A.json`, 그리고 `Saved/Automation/Level1Flow_D/index.json`이다. 완료한 R0~R14 원본 이식, 전체 Blueprint Compile, native 회귀를 재실행하지 않았다. **C++ 변경/빌드·패키징은0, DLL도 기존 검증본 유지**다.

**다음 사용자 확인:** Editor를 열어 Level_1에서 환경/시작 위치, 자연 AI 이동·전투/보스/HUD, 결과 및 Esc 메뉴의 돌아가기가 같은 맵을 새로 시작하는지 짧게 확인한다. 추가 빌드는 필요 없다. 확인 후 GitHub Desktop에서 새 작업 브랜치(예: `codex/level1-r13-flow`)에 현재 변경을 옮겨 커밋/푸시하며 main에 직접 커밋하지 않는다. 기존 테스트 맵/레거시 제거와 실사용 자산 정식 경로 이관은 후속 묶음이다.

---

## 조사 이력 — 2026-10-14 main 병합 후 정리 후보 / AGENTS 0.108

**사용자 확정 방향:** 디자이너 `/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1`의 환경·건물·장식을 유지하고, 그 맵의 기존 플레이 흐름은 폐기한다. R13 테스트 맵에서 검증한 플레이어·입력·라이플·AI·스테이지·보스·HUD/메뉴/결과 흐름을 Level_1에 연결한다. 테스트 맵을 최종 플레이 맵으로 남기는 방향이 아니다.

기준 main은 PR #1 병합 커밋 `3665f3ba690bab286784da7aae183b7485932cd0`이다. 사용자는 리뷰 수정본 PIE 정상 동작과 커밋/푸시, 병합까지 직접 완료했다. R14 및 리뷰 검증 완료를 취소하거나 반복하지 않는다. 이번 추가 작업은 **조사와 변경안 정리**이며 C++·Config·에셋 수정/삭제, Blueprint Compile, PIE, 빌드/패키징, Git 조작은 하지 않았다.

### 실제 두 맵의 차이

| 항목 | 디자이너 Level_1 | 검증된 R13 L_DC_Level1UI |
|---|---|---|
| 에디터 Outliner 조회 액터 | 4,644 | 1,116 |
| GameMode | `/Game/Blueprint/BP_DreamCatcherGameMode` | `GAS/Test/R13/BP_DC_StageUIGameMode` |
| 플레이어 선택 | 구형 `BP_DreamCatcherChracter` / `BP_DreamCatcherPlayerController` | Experience → `DA_DC_LyraAIPawn` → `BP_DC_LyraInventoryPawn` |
| 현재 전투 배치 | 구형 `BP_EnemyCharacter_C_1` 1개 | 일반 Lyra 스포너2개＋보스 스포너1개 |
| 진행 배치 | StageDirector/EncounterController 없음 | StageDirector1개＋EncounterController3개, 일반전투2회→보스전→Complete |
| PlayerStart | Engine.PlayerStart1개 | DCLyraPlayerStart1개 |
| NavMeshBoundsVolume | 0개 | 1개 |
| 돌아가기 | 구형 맵 흐름 | R10 `L_DC_LyraFiniteEncounter`로 이동 |

LevelScriptActor도 Outliner API와 별도로 조회했지만 두 맵에서 발견되지 않았다. 따라서 이번 조회에서 별도 Level Blueprint 진행 로직은 확인되지 않았다. 열린 사용자 Editor의 미저장 상태는 조사하지 않았으며, 위 값은 저장된 맵 기준이다.

R13의 게임플레이 배치를 옮겨야 하며 **맵 전체 복제로 디자이너 환경을 덮어쓰지 않는다.** 현재 R13 스포너 좌표를 그대로 최종 배치로 확정하지 않는다. Level_1의 충돌·이동 가능 공간에 맞춰 시작 위치/일반전투/보스 위치와 NavMesh 영역을 정하고 연결해야 한다. 기존 자동 리스폰 없음, 명시적 재시작/결과 흐름은 유지한다.

### 정리 후보 분류

숫자는 경로/분류별 조사 수이며 서로 겹칠 수 있다. ‘참조 없음’은 저장된 package 참조와 확인한 소스/설정 범위의 결과다. 모든 동적 이름·태그 조회나 최종 Cook의 미사용 판정을 의미하지 않는다.

| 묶음 | 확인된 범위 | 처리 방향과 선행 조건 |
|---|---|---|
| GAS 테스트 에셋 | `Content/DreamCatcher/GAS/Test/`83개, 그중 맵8개 | 아래 실사용 자산을 정식 경로로 옮긴 뒤 과거 테스트/맵 제거. 폴더 전체 즉시 삭제 불가 |
| 실사용 경로에 섞인 테스트 폴더 | 맵 간 이동을 제외한 R13 게임플레이 참조에38개 | 31개는 플레이어/입력/AI/보스/UI 등 정식 경로 이관 후보. 나머지7개는 아래 테스트 부여/빈 Set 해제 후보 |
| 과거 GAS 테스트 BP/맵 | `Blueprints/AI/Test/BP_DC_GASTestTarget`, `Core/Test/BP_DC_GAS_TestGameMode`, `BP_DC_PlayerController_GAS_Test`, `Player/Test/BP_DC_Player_GAS_Test`, `Map/Test_Map/L_GAS_CombatTest` | 테스트 표적·구형 플레이어 묶음 제거 후보. DefaultPawnData의 구형 fallback 연결부터 교체 |
| 구형 전투/장비/UI 구현 | Combat·Health·Equipment·Weapon·Reticle·HUD·Camera·Enemy14계열28개 h/cpp와 대응 BP | 활성 Pawn/Controller의 잔존 컴포넌트·변수·타입 참조를 먼저 분리한 뒤 제거 |
| 별도 테스트 표적 | `AI/DCGASTestTarget.h/.cpp`, `Development/DCRifleIntegrationTarget.h/.cpp`, `BP_DC_RifleDeathTarget` | 구형 전투 테스트/라이플 테스트 맵과 함께 제거 후보 |
| C++ 회귀 검사 | `Source/DreamCatcher/Tests/`37파일(CPP26/H11), 헤더 선언44타입 | 확인한 외부 런타임 include/선언 타입 사용과 BP 부모 참조0. 과거 이식 검사는 정리 후보, 입력·발사·장비·스테이지·보스 핵심 회귀는 개발 전용으로 보존 권장. 일반 속성/함수 참조까지 전수 미사용이라고 확정한 것은 아님 |
| 템플릿 코드 | `Variant_Combat`42, `Variant_Platforming`8, `Variant_SideScrolling`26 =76파일 | 현재 저장 에셋의 native parent 및 다른 런타임 소스의 해당 타입 사용을 찾지 못함. 템플릿 정리 묶음 후보이며 Source include 경로도 함께 정리 |
| 옛 템플릿 경로 | `Content/ThirdPerson`5, `Variant_Combat`28, `Variant_Platforming`11, `Variant_SideScrolling`19 =63개 | 모두 redirector이고 목적지 `/Game/Start_Contents/`가 디스크/Registry에 없음. 디스크 referencer0. 깨진 템플릿 redirector 제거 후보 |
| 나머지 redirector | 전체114개, 디스크 referencer 없는 것은111개 | 위63개와 구분. 유효 목적지가 있는 redirector는 실제 참조 정리 후 Unreal에서 처리하며 일괄 raw 삭제하지 않음 |
| 이식 작업 스크립트 | `Scripts/Editor/`Python87개＋추적 중 pycache4개 | 일회성 이식/진단 레시피는 작업 종료 후 정리·보관. 이번 정식 경로 이관에 필요한 reader/helper는 마지막까지 유지. pycache4개는 우선 정리 후보 |
| Editor 이식 플러그인 | `Plugins/DCRifleMigrationTools`추적26파일, Editor 모듈1개 | 저장 에셋의 `/Script/DCRifleMigrationTools` package 의존0. 아직 에셋 조사/이관 스크립트가 사용하므로 실제 에셋 정리 후 `.uproject` 등록과 함께 제거 검토 |
| 수동 백업 소스 | 루트 `백업/`13파일 | 현재 빌드 Source 루트 밖의 옛 C++/Build.cs/uproject 사본. Git 병합 커밋을 복구 기준으로 두고 정리할 우선 후보 |
| 옛 Box_1 복사본 | `DreamCatcher_ENV/Prop/Box_1/`13개 | 외부 직접 참조는 R11/R12/R13 맵뿐. Level_1 및 맵을 제외한 새 게임플레이 참조0. 세 테스트 맵 폐기 후 제거 후보. 디자이너 `Prop/Mesh/Box_1`은 유지 |
| Saved 진단/생성물 | `Saved/Diagnostics`, `Saved/Automation`, 로그·DDC·Intermediate 등 | 레거시 기능과 구분. Git에서 무시되는 자료이며 최종 근거·복구 자료를 보관한 뒤 별도 용량 정리. 저장 게임·설정과 추적 중 Binaries를 일괄 삭제하지 않음 |

### 실제 PawnData에 남은 테스트 부여 7개

`DA_DC_LyraAIPawn`과 기존 `DA_DC_RiflePawn`의 AbilitySets에서 아래 연결을 확인했다. 클래스/그래프/부여 목록을 직접 읽었으며 새 PIE나 Compile은 실행하지 않았다.

1. `/Game/DreamCatcher/GAS/Test/AS_DC_R2_TagRelations`와 `GA_DC_R2_TagFire`, `GA_DC_R2_TagReload`: **3개**. 두 Ability는 입력 태그가 비어 있고 Commit/PrintString/WaitDelay/End 검사 그래프다. 현재 라이플의 실제 발사/재장전 Ability와 구분하여 Set 부여를 해제하고 제거할 후보다. 이름이 비슷한 원본 `/Game/LyraMigration/AbilitySystem/TagRelationships_ShooterHero`는 실전 태그 관계 데이터이므로 보존한다.
2. `/Game/DreamCatcher/GAS/Test/R9/Signature/AS_DC_SignatureSmoke`, `GA_DC_SignatureSmoke`, `GE_DC_SignatureCooldownSmoke`: **3개**. InputTag.Ultimate에 실제로 연결됐지만 효과는 TestCommitCount/TEST 출력·0.35초 대기·검증용2초 쿨다운뿐이다. 제거하면 현재 Q 입력의 테스트 반응이 없어지는 차이가 있다. 정식 시그니처의 실제 효과를 이번 정리에서 임의 제작하지 않으며 `/Game/DreamCatcher/GAS/Abilities/Signature/GA_DC_SignatureBase`와 입력 확장 기반은 별도 보존 후보다.
3. `/Game/DreamCatcher/GAS/Test/R4/AS_DC_PlayerPawnBase_R4`: **1개**. 현재 GrantedGameplayAbilities/Effects/Attributes가 모두 빈 Set이다. PawnData의 잔존 연결과 함께 제거 후보로 분류한다.

세 PawnData(현재 Lyra AI/플레이어용, 구형 Rifle용, AssetManager fallback용)의 중복/잔여 AbilitySet·카메라·장비 필드도 함께 정리해야 한다.

### 이름만 보고 지우면 안 되는 실사용 계층

- `Development/DCRifleIntegrationActors`의 **Controller**는 현재 Inventory/QuickBar/라이플 장착·해제를 담당한다. `DCLyraAIIntegrationController`는 그 자식이며 현재 R13 PlayerController의 native 기반이다. 이 파일에는 구형 R5 테스트 GameMode도 함께 있으므로 통째로 삭제하지 않고 실사용 Controller를 정식 Player 계층으로 분리/정리할 대상이다.
- `DreamCatcherCharacter`와 `DreamCatcherPlayerController`는 새 플레이어의 native 상속 기반이기도 하다. 구형 CombatComponent, FollowCamera/CameraBoom, HUD·입력 호환 분기 등 잔존 부분을 좁혀 제거해야 한다. 클래스 전체 삭제 대상이 아니다. 구형 `DreamCatcherGameMode`는 별도 `DCLyraGameMode`와 다르며 구형 BP/테스트 GameMode 폐기 후 제거 검토한다.
- `BP_StageDirector`, `BP_EncounterController`, `DCStageDirector`, `DCEncounterController`, `DCEnemySpawner`는 R13에서도 사용한다. **DCEnemySpawner의 EnemyClass 구형 분기**와 **GASEnemyClass 보스 분기**를 구분한다. `DCBossCharacter : DCGASEnemyCharacter`이므로 보스 기반인 GAS Enemy/Health/Death 계층은 보존한다.
- `/Game/LyraMigration/`584개 중 맵을 제외한 게임플레이 package 참조에458개가 들어간다. 나머지도 Config·AssetManager·UI/태그 검색으로 사용될 수 있다. 특히 `Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247`은 현재 실전 라이플이며 `UI/R13`은 현재 HUD/메뉴다. 이름이 Migration/Diagnostics라는 이유로 삭제할 수 없다. 필요한 묶음을 정식 경로로 옮기는 대상이다.
- CommonGame/CommonUI/CommonUser/GameSettings/UIExtension/GameplayMessageRouter/ModularGameplayActors 등 현재 런타임 플러그인과 디자이너용 Win64 DLL은 삭제 대상이 아니다. DCLyra 원본 계층과 Character/AI/Stage/Boss는 핵심 게임 코드다.

### 설정과 추가 결정이 필요한 연결

- `DefaultEngine.ini`의 GameDefaultMap은 R13, EditorStartupMap은 Level_1, GlobalDefaultGameMode는 구형 BP다. 새 Level_1 GameMode override와 기본 실행 설정을 함께 맞춘다.
- `DefaultGame.ini`의 MapsToCook2개는 R13/R10 복귀 테스트 맵이다. Experience/ActionSet AlwaysCook 스캔도 Test/R10·R13 및 LyraMigration/AI에 걸려 있다. 에셋 경로만 옮기거나 삭제해서는 안 된다.
- `DCAssetManager.DefaultPawnData`가 구형 `/Game/DreamCatcher/GAS/Pawn/DA_DC_PlayerPawn`이고, 이 데이터가 옛 테스트 플레이어·WID_DC_Rifle을 참조한다. fallback도 정식 PawnData로 교체해야 구형 장비/플레이어를 안전하게 제거할 수 있다.
- GameplayCueNotifyPaths에 실전 라이플의 Diagnostics 경로가 등록되어 있다. Cue/원본 클래스 CoreRedirect는 이관 후 실제 serialized 참조를 확인하여 최소 변경한다. `DefaultGameplayTags.ini`의 `Test.R2.CostCharge`는 회귀 검사에서도 사용하므로 해당 검사 처리와 함께 정리한다.
- `DefaultEditor.ini`의 `/Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap`, `DefaultEditorPerProjectUserSettings.ini`의 `/Game/ThirdPersonCPP`는 오래된 템플릿 편집 경로다. 정리 후보지만 이번에는 Config를 수정하지 않았다.
- **돌아가기 목적지는 아직 결정이 필요하다.** 현재 R13의 ReturnMap은 R10 테스트 맵이며, 이를 지우거나 null로 두면 메뉴/결과의 돌아가기가 동작하지 않는다. 등록된 맵에서 정식 로비/프런트엔드 맵은 확인되지 않았다. 테스트 맵 폐기 전에 정식 복귀 목적지 또는 임시 동작을 확정해야 하며, 버튼 삭제/기능 축소로 숨기지 않는다.

### 권장 실행 순서 — 아직 적용하지 않음

1. **Level_1의 플레이 흐름 교체 묶음:** 디자이너 환경 보존, 구형 적1개/기존 GameMode·PlayerStart 연결 교체, 검증된 Experience·Pawn/Controller·HUD 및 일반전투2→보스→결과 배치, NavMesh, 기본 실행/Cook/복귀 연결을 함께 다룬다. 실제 위치와 복귀 목적지를 확정하고 연결한 뒤 사용자 짧은 완주 확인을 받는다.
2. **실사용 에셋의 정식 경로 이관 + 활성 테스트 부여 정리:** 테스트 폴더의 실사용31개 및 라이플/UI 등 필요한 imported 자산을 이름/부모/기본값을 보존하며 Unreal API로 이동하고 참조/설정을 갱신한다. 위7개 테스트 흔적과 중복 PawnData를 해제·정리한다. 실제 기능 기반은 남긴다.
3. **레거시·과거 테스트 묶음 제거:** 구형 플레이어/적/HUD/장비·카메라의 호환 연결과 native 사용을 끊은 뒤 대응 BP/코드를 정리한다. 기존 GAS 테스트맵8개와 L_GAS_CombatTest, 테스트 표적, 옛 Box13개는 이관·복귀 전환 후 처리한다. Source 변경 후 C++ 빌드는 사용자 담당이다.
4. **개발 잔재 마감:** 필요한 핵심 회귀와 최근 근거를 보존하고, 오래된 테스트/이식 스크립트·플러그인·템플릿76파일/63redirector·백업13파일·pycache4개를 승인된 정확한 목록으로 정리한다. 전체 redirector 일괄 삭제나 Binaries/사용자 저장 설정 제거는 하지 않는다.

각 실제 변경 묶음은 정확한 파일·에셋·Config 범위를 먼저 제시한다. 현재 요청은 후보 조사이며 위 대량 이동/삭제를 이번 조사에서 실행한 것이 아니다.

### 조사 근거와 한계

- `Saved/Diagnostics/Level1Cleanup/assets_A/`: package3,786개 metadata/dependencies/referencers, 선택 BP36개, PawnData3개, 저장 맵3개. BP22개는 기존 R14 스냅샷과 파일 해시가 일치해 재사용했고14개만 새로 읽었다.
- `flow_details_A.json`: 테스트 부여/현재 입력/두 GameMode 및 숨겨진 LevelScriptActor·NavMesh 추가 확인. 저장/Compile0, ensure0.
- `classification_A.json`: GAS/Test83개·LyraMigration584개·옛 Box13개·legacy native 소비자·정식 이관 의존 경로의 전체 목록.
- `text_A.json`: 추적 파일과 코드 그룹별 목록/크기, native 선언/소비자, Source/Config의 문자열 경로.
- `redirector_candidates_A.json`: 전체114개 및 목적지가 없는 템플릿63개. `Content/Start_Contents`는 디스크/Registry 모두 부재다.
- 에셋 파일3,788개의 크기/수정 시각과 보호 소스·Config·DLL·Git index 불변. 두 Unreal 조회 모두 종료0/Error·ensure·Fatal0. 원본 Lyra script import 및 Manny PoseAsset/NavMesh 관련 경고는 삭제 성공/실패와 구분한다.
- `/Content/BlockingStarterPack/`은 Git 무시 대상이며 로컬에만 있는 파일도 포함한다. 두 pack 맵은 디스크에 있지만 Registry에 없었다. 이 미검증 외부 팩을 정리 대상으로 임의 확정하지 않는다.
- package closure는 실행/Cook 목록이 아니다. PrimaryAssetId, 태그/문자열 로딩, C++ native 타입·Blueprint 핀 참조는 실제 삭제 직전 해당 범위에서 다시 확인해야 한다. 이번 조사에는 새 플레이어/맵 활성 변경이나 패키징 검증이 없다.

---

## 아래는 R14 완료 당시 이력

기준일: 2026-10-14 KST. 브랜치: `GAS-System`. R13 승인 범위의 사용자 수용은 완료 상태를 유지한다.

## 이력: R14 완료 / 디자이너 main 병합 준비 — AGENTS 0.106

사용자가 **창 크기를 바꿔도 UI가 정상 작동함을 확인**했고 R14 완료 정리를 요청했다. 이에 패키징을 제외한 현재 GAS-System의 R14 정리·검증·사용자 수용을 완료로 기록한다. 구체적인 화면 크기/OS DPI 수치를 보고받은 것은 아니므로 모든 해상도·장치 검증으로 확대하지 않는다. 패키징은 사용자 지시에 따라 보류·미검증이고 기존 경고29 및 기존 플레이어/맵 호환 계층 보존도 유지한다.

완료 근거는 2A 에셋1수정/3삭제·사용자 PIE 수용, 2B 구형 C++2파일 제거·사용자 빌드/원본 Death 연결 검증, 최종 Blueprint316개 SkipSave Compile 통과, 이번 UI 창 크기 변경 수용이다. 완료한 이식·검사를 반복하지 않았다.

다음은 [main 병합 준비 인계](main-merge-preparation-2026-10-14.md)를 따른다. 디자이너의 작업은 모두 푸시됐다고 사용자 확인했고 원격/로컬 main은 ebb169c다. 동일 수정 경로 충돌은0이지만 Box_1 경로 이동에 따른 기존 R11~R13 참조 보존이 필요하다. 현재 R13/R14 미커밋 작업의 checkpoint도 먼저 필요하다. 이번에는 조사/문서만 진행했고 Git stage/commit/checkout/merge/push, 브랜치 생성·에셋 변경·C++ 빌드/패키징은 하지 않았다.

---

## 패키징 보류·R14 후 main 병합 / 최종 Compile 통과 이력 — AGENTS 0.105

사용자는 **패키징 과정은 건너뛰고, R14가 끝난 뒤 디자이너가 작업 중인 main과 병합**한다고 명시했다. 이 최신 지시가 이전의 ‘지금 Windows 패키징 대기’보다 우선한다. 패키징은 현재 R14 마감에서 분리된 **보류·미검증** 항목이며 성공으로 표시하지 않는다. 병합은 R14 이후 단계이고 이번에 fetch/checkout/merge를 실행하거나 타인의 작업을 변경하지 않았다. 병합 방향과 원격 최신/미커밋 상태는 실제 병합 단계에서 확인한다.

현재 소스/에셋 정리와 관련 검증은 완료 상태다. R14-2A는 사용자 PIE 수용 완료, R14-2B는 사용자 Editor 빌드와 원본 Death 연결 검증 완료다. 이를 반복하지 않았다. 남아 있는 구형 Combat/Equipment/Health/HUD/Camera 계층은 기존 플레이어·맵의 확인된 호환 의존 때문에 보존한다. R13의 원본 입력 경로 선택, 원본장비1/구형장비0, 원본 발사/사망·HUD 경로 검증을 유지한다. 모든 구형 코드가 프로젝트에서 사라졌다는 뜻은 아니며 잔존 비활성 계산의 성능 비용도 이번에 측정하지 않았다.

### 패키징을 제외한 최종 확인

새 DLL 기준으로 `/Game`에 저장된 Blueprint/UMG/AnimBP/ControlRigBlueprint 자산 **316개를 엔진 SkipSave Compile API로1회 확인**했다. `Scripts/Editor/r14_final_blueprint_check.py`, `Saved/Diagnostics/R14/final_blueprints_A.json`, `Saved/Logs/R14FinalBlueprints-A.log`가 근거다.

- **성공316/실패0, Error·ensure·Fatal0, 종료0.** C++ 빌드·에셋 저장·Cook·패키징·PIE0.
- 에셋 파일 상태, Source/Config·uproject·DLL·Git index 불변.
- Warning29개는 Manny PoseAsset14＋Quinn PoseAsset14＋B_LyraGameInstance의 이전 `/Script/LyraGame` package 조회1이다. 경고를 없앴다고 보고하거나 단순 재저장으로 덮어쓰지 않았다.
- 검사 범위는 등록된 Blueprint 계열 자산이다. 맵 내부 LevelScript BP·외부 플러그인 자산 전체, 모든 해상도/장치/복제, 패키지 실행의 새 검증은 아니다. 기존 실제 맵 PIE/사용자 수용은 별도 근거로 유지한다.

최종 정리 근거 `premerge_closeout_A.json`. R13 GameDefaultMap/두 MapsToCook 설정은 이미 준비된 상태를 유지했다. 패키징을 보류하라는 지시를 Config 원복 지시로 해석하지 않았다. main 병합 후 실제 디자이너 맵의 최종 실행/복귀 경로를 별도로 결정한다.

### 남은 R14 사용자 확인

사용자는 **해상도·DPI/창 크기 변경 확인은 아직 전**이라고 답했다. 따라서 R14 전체 완료로 표시하지 않는다. 남은 것은 창 크기를 바꿔도 HUD·보스 체력/페이즈·Esc 메뉴·결과 버튼이 잘리거나 겹치지 않는지 확인하는 항목이다. 예를 들어 1280×720과1920×1080에서 확인한다. 발사·조준·사망·완주 및 이식 검사를 다시 요구하지 않는다.

이 확인이 끝나면 패키징을 제외한 현재 GAS-System의 R14 마감을 정리하고 **다음 단계에서 main 병합 준비**로 넘어간다. 패키징/패키지 실행, 모든 장치/복제, 디자이너 실맵 통합 후 검증은 별도 상태로 남긴다. 새 삭제/호환 계층 제거·Config 변경은 이번 작업에서 하지 않았다.

---

## R14-2B 검증 완료 / R13 패키지 경로 준비 이력 — AGENTS 0.104

사용자가 R14-2B 빌드 완료를 알렸다. UBT 로그의 **2026-10-14 02:50:12 KST 시작, 32.46초 성공**과 DLL **02:50:44 / 6,625,792 bytes / SHA256 c4730a1a…** 갱신을 확인했다. `native_death_user_build_A.txt`, `native_death_build_verified_A.json`이 근거다. Codex는 빌드하지 않았다.

`native_death_verify_Verify_A.json` / `R14NativeDeath-Verify_A.log`: 새 프로세스에서 구형 `DCGameplayAbility_Death` 미등록, 원본 `DCLyraGameplayAbility_Death` 등록과 GA_Hero_Death·GA_DC_EnemyDeath·실제 Lyra Pawn·Boss **4개 로드/Compile 통과**. 종료0/Error·ensure·Fatal0, 기존 PoseAsset 등 Warning은 남아 있다. 저장0/에셋 상태 및 보호 소스·Config 변화0. **R14-2B 승인 범위는 빌드/연결 검증까지 완료**다. 2A 사용자 수용도 유지하며 해당 PIE/이식/정리를 반복하지 않는다.

### 사용자 선택에 따른 다음 작업

사용자는 다음 패키지 검증의 시작점을 **‘검증된 R13 맵으로 우선 준비’**로 선택했다. 이를 위해 정확히 Config2개만 변경했다.

| 파일 | 이번 변경 |
|---|---|
| `Config/DefaultEngine.ini` | GameDefaultMap을 `/Game/DreamCatcher/GAS/Test/R13/L_DC_Level1UI.L_DC_Level1UI`로 변경 |
| `Config/DefaultGame.ini` | `[/Script/UnrealEd.ProjectPackagingSettings]`에 MapsToCook로 R13 시작 맵과 기존 R10 복귀 맵2개 추가 |

Cook 맵 경로는 `/Game/DreamCatcher/GAS/Test/R13/L_DC_Level1UI`, `/Game/DreamCatcher/GAS/Test/R10/LyraEncounter/L_DC_LyraFiniteEncounter`다. EditorStartupMap은 기존 Level_1, 전역 기본 GameMode/기존 GameInstance는 유지했다. R13 월드의 기존 GameMode override가 StageUIGameMode를 선택한다. 기존 ReturnMap도 R10 그대로다. 게임 C++·맵/플레이어 에셋·uproject·DLL·Git index를 수정하지 않았다.

UE5.8 ProjectPackagingSettings는 native 모듈이 DeveloperToolSettings여도 `OverrideConfigSection`에서 `/Script/UnrealEd.ProjectPackagingSettings`를 사용한다. 엔진 Cook 코드는 명령줄 등에 별도 맵 목록이 없으면 MapsToCook를 읽는다. 프로젝트의 기존 Experience/ActionSet AlwaysCook 스캔과 DCStageUI 플러그인 활성화는 그대로 유지했다. 모든 `/Game`을 강제 Cook하거나 기존 테스트/디자이너 자산을 삭제하지 않았다.

`r14_package_config_verify.py` / `package_config_Verify_A.json`은 새 Unreal 프로세스에서 아래 **유효 설정과 저장된 연결만** 확인했다.

- GameDefaultMap=R13, EditorStartupMap=기존 Level_1, 전역 GameMode/GameInstance 보존.
- MapsToCook에 두 맵 존재, bCookAll=false, NeverCook 목록 비어 있음. 기존 AlwaysCook 폴더 `/NNEDenoiser`는 유지.
- R13 월드의 StageUI GameMode, R13 Experience ID, bUseExperienceUI=true, 기존 R10 ReturnMap 일치.
- R13을 포함한 Experience/ActionSet AlwaysCook 규칙2개 및 DCStageUI enabled 확인.
- 종료0/Error·ensure·Fatal0, 에셋 저장0/전체 에셋 상태 불변, 변경 파일은 위 Config2개뿐.

최종 근거 `package_prepare_summary_A.json`, 이번 변경만 담은 `package_config_changes_A.patch`. **Cook/패키징/패키지 실행 성공이 아니며 아직 수행하지 않았다.** 추가 Editor C++ 빌드는 필요 없다.

### 다음 사용자 작업

1. 변경된 Config를 읽도록 Editor를 다시 열고 **Windows Development**로 패키징한다. 패키징은 계속 사용자 담당이다. 별도 Launch/Cook 프로필에서 맵 목록을 덮어쓰고 있다면 위 두 맵이 포함돼야 한다.
2. 패키지 실행 파일을 별도 맵 인자 없이 실행해 R13으로 시작하는지 확인한다.
3. 최초 장착·HUD/메뉴, 전투→보스→결과, 사망 후 명시적 재시작, 돌아가기→R10 전환을 확인한다. 이는 패키지 환경 검증이며 기존 PIE 수용을 취소하거나 이식을 반복하는 것이 아니다.

패키징 실패 시 사용자 로그를 근거로 필요한 변경 범위를 좁힌다. 현재는 **사용자 패키징/실행 결과 대기**다. 디자이너 최신 맵/브랜치 통합, 보존 중인 구형 호환 계층의 추가 전환, 최종 제출 패키지와 R14 전체 완료는 별도 잔여 범위다. 로컬 main의 최신 저장 커밋은 `9b6f363`(10월9일)이며 디자이너의 원격 최신/미커밋 상태를 보증하지 않는다. 이번에 fetch/병합은 하지 않았다.

---

## R14-2A 사용자 수용 / R14-2B 코드 제거·빌드 대기 이력 — AGENTS 0.103

사용자는 **R14-2A를 PIE에서 확인했다**고 명시했고, 제안한 R14-2B C++2파일 제거도 승인했다. 2A는 자동 회귀와 사용자 수용까지 완료다. 기존 이식/에셋 삭제/완료한 PIE를 반복하지 않는다.

승인된 `Source/DreamCatcher/AbilitySystem/Abilities/DCGameplayAbility_Death.h`와 `.cpp`를 제거했다(45＋112=157줄). 제거 직전 제안 해시와 일치했으며, 다른 Source/Config/Plugin의 정확한 타입·include·스크립트 클래스 경로 참조가 없었다. 제거 후 동일 범위 참조0을 재확인했다. **현재 원본 `DCLyraGameplayAbility_Death`와 기존 Health/캐릭터·맵·에셋·Config·태그는 유지**했다. 이를 대체하는 새 코드나 Redirect를 추가하지 않았다.

정적 근거는 `Saved/Diagnostics/R14/native_death_before_A.json`, `native_death_removed_A.json`이다. 실제 변경은 승인된 소스2개 삭제뿐이며 다른 보호 파일 **784개** 해시와 Content/Feature 에셋 파일 **2,897개** 크기·수정시각은 불변이다. DLL/Git index/uproject도 유지했다. 상태 문서와 빌드 후 확인 스크립트는 추가/갱신했다. Codex의 C++ 빌드·패키징·Editor 실행·Computer Use·Git 조작은0이다.

**다음은 사용자 Development Editor / Win64 빌드1회**다. native 클래스 제거이므로 Editor를 닫고 빌드한다. 현재 DLL은 아직 제거 전 바이너리이며 클래스 제거의 UHT/링크/실제 로드 성공을 정적 검사로 대체하지 않는다.

빌드 완료 후 `Scripts/Editor/r14_native_death_verify.py`를 새 commandlet에서 `-DCR14NativeDeath=Verify_A`, 로그 `Saved/Logs/R14NativeDeath-Verify_A.log`로 실행한다. 스크립트는 삭제 전 DLL 해시와 다름/삭제2소스 부재/활성 원본 소스 보존을 먼저 요구한다. 구형 native 클래스 미등록, 원본 Death 클래스 등록, GA_Hero_Death·GA_DC_EnemyDeath·실제 Lyra Pawn·Boss4개 로드/Compile을 확인하며 저장0으로 제한한다. 현재는 Python AST 확인만 통과했고 **Unreal 실행은 아직 하지 않았다.** 기존4에셋 정리나 전체 Blueprint/PIE 이식을 반복하지 않는다.

이 단계는 빌드 대기이며 R14-2B 검증 완료가 아니다. 잔존 Combat/Equipment 및 기존 플레이어 호환 계층, 시작/복귀 맵·Cook·디자이너 통합·최종 패키징 등 R14 전체 잔여 범위는 그대로다.

---

## 사용자 시각 확인 대기 / R14-2B 변경안 준비 이력 — AGENTS 0.102

사용자는 R14-2A 이후의 발사·조준·사망 연출 PIE를 **아직 확인하지 않았다**고 명시했다. 2A 자동 회귀 성공은 유지하되 사용자 수용 완료로 표시하지 않는다. 이번 후속 요청에서는 독립적인 다음 정리 후보 조사와 변경안 준비만 수행했다. 게임 소스·Config·에셋/맵 수정·삭제, Editor 실행·재검사·빌드·패키징·Git 변경은0이다.

다음 **R14-2B 제안은 아래 C++2파일 제거만**이다. 아직 삭제 승인·적용 전이다.

- `Source/DreamCatcher/AbilitySystem/Abilities/DCGameplayAbility_Death.h`
- `Source/DreamCatcher/AbilitySystem/Abilities/DCGameplayAbility_Death.cpp`

두 파일은 `UDCHealthComponent`를 호출하는 이전 사망 Ability다. 다른 Source/Config/Plugin의 정확한 타입·include·스크립트 클래스 경로 참조는0이며, source 해시는 앞선 정리 전 기준과 같다. 유일한 자식 GA_DC_Death는 승인된 2A에서 삭제됐다. 기존 Blueprint 타입 조사318개에서 이 구형 클래스의 함수/cast/핀 참조는0이었다. 그 조사에서 실패했던 오래된 Cue도2A에서 제거됐으므로 같은 전체 조사를 반복하지 않았다. 이것은 모든 동적 참조가 새로 검증됐다는 뜻은 아니다.

현재 적/보스는 `DCLyraGameplayAbility_Death`를 사용하고, 원본 Lyra Death의 CoreRedirect도 이 원본 기반 클래스를 가리킨다. **DCLyraGameplayAbility_Death·DCHealthComponent·캐릭터·맵·Config·태그는 변경안에 포함하지 않는다.** 의도한 플레이 동작은 유지하고 쓰이지 않는 native 클래스 등록/컴파일 대상만 줄인다. 새 기능 구현이나 원본 사망 동작 대체는 없다.

검토용 패치는 `Saved/Diagnostics/R14/native_death_proposal_A.patch`, 근거는 `native_death_proposal_A.json`에 있다. 실제 두 소스는 그대로다. 승인 후 대상2파일만 제거하고 사용자가 Development Editor/Win64 빌드를1회 수행한다. 이후 새 DLL에서 원본 클래스 등록과 현재 Death Blueprint/관련 부모 로드·Compile을 확인한다. 제거된 타입의 잔여 참조가 발견되면 해당 참조를 숨기기 위한 Redirect를 추가하지 않고 원인을 확인한다. 완료된 이식/전체 테스트 반복과 패키징은 이 묶음에 포함하지 않는다.

---

## R14-2A 승인 범위 적용 및 자동 회귀 통과 이력 — AGENTS 0.101

사용자가 아래 에셋4개 변경안을 승인했고 열린 Editor를 저장·종료했다고 확인했다. **노드2개 제거/부모 Pawn 저장1개 및 미사용 에셋3개 삭제를 완료**했다. 추가 C++ 빌드는 필요 없다. 다음은 기존 R13 맵에서 짧게 발사·견착/복귀·사망 연출을 사용자 확인하는 것이다. 이번 자동 검사는 물리 입력·화면/음향 수용을 대신하지 않는다.

- 복구 기준: 커밋 `7a10c75b0b2af4be2f15f7c104c1bbfe82c4bbe9`의 대상4개 LFS 포인터와 현재 파일 SHA/크기가 일치하고, **로컬 LFS payload4개도 존재·해시 일치**함을 확인했다. `cleanup_recovery_A.json`에 경로/객체 ID/해시를 남겼다. 새 바이너리 복사나 Git 변경은 하지 않았다. 커밋 blob은 LFS 포인터이므로 포인터 자체와 uasset을 동일 바이트로 비교한 초기 receipt 시도는 실패했으며 변경0이었다.
- `cleanup_Apply_B.json`: BP_DC_RiflePawn의 EventGraph에서 승인한 `K2Node_CallFunction_8`/`K2Node_VariableGet_0`만 제거하고 저장했다. 나머지 그래프/핀·부모/컴포넌트·메시/AnimClass·Hero 설정 동일성 확인. 이 실행은 후속 삭제 확인에서 중단되어 complete=false이며, **Pawn 저장 성공과 전체 작업 완료를 구분**한다. 저장된 Pawn을 재수정하지 않았다.
- `cleanup_Delete_B.json`: GA_DC_Death, CM_DC_Hip, 비활성 `/Game/LyraMigration/Death/GCNL_Death`를 Unreal Editor API로 삭제했다. 삭제 직전 참조0/원본해시/dirty 상태를 확인했고 실제 파일과 Registry에서3개 모두 없어졌다. 다른 에셋 변화0. 깨진 Cue의 삭제를 위한 로드에서 이미 확인한 Compile 오류2개만 발생했고 대상 제거 뒤 새 프로세스 검사는 오류0이었다.
- 엔진 삭제 차이: 기본 `EditorAssetSubsystem.delete_asset`은 commandlet과 별도 Editor에서 모두 메모리/Registry 제거 성공을 반환했으나 파일을 남겼다. UE5.8 `ObjectTools.cpp`의 기본 참조 조사와 비교용 기존 알고리즘을 확인하여 **삭제 프로세스에만 `Editor.UseLegacyGetReferencersForDeletion=1`**을 적용했다. 외부 참조0/해시/대상 제한 검사는 유지했다. 이 경로에서 실제 파일 제거까지 통과했고 종료 전0으로 복원했다. Config/엔진 소스 변경0. 세부 내부 참조 차이의 근본 원인을 완전히 입증했다는 의미는 아니다.
- `cleanup_Verify_A.json`/`R14Cleanup-Verify_A.log`: 새 프로세스에서 수정 Pawn, 실제 자식 Pawn, 기존 GAS 테스트 플레이어, 활성 플레이어/적 Death Ability, 활성 Death Cue, 원본 기본 Camera **7개 재로드/Compile 통과**. 나머지 그래프/컴포넌트 동일, 수정 Pawn의 구형 WID_DC_Rifle 직접 의존 제거, 삭제3개 부재 확인. 추가 저장0, Error/ensure/Fatal0.
- `cleanup_pie_A.json`/`Saved/Automation/R14Cleanup_A/index.json`: **실제 R13 PIE 성공1/실패0/오류0/경고15**. 경고는 기존 Manny PoseAsset14＋Recast1이다. 원본 장비1/구형 장비0, 탄약30→29, 견착 진입→Hip 복귀, 원본 Death 시작3.909초→종료11.913초→Pawn 제거12.031초, 실패 결과와 QuickBar 장비 정리를 확인했다. 입력 주입·검사용 면역 및 SelfDestruct 태그의 피해 GE를 사용했고 물리 조작/픽셀/음향/복제 검증은 아니다.
- 최종 근거 `cleanup_summary_A.json`: 정확히 에셋1수정/3삭제, 기존 게임 C++·Config·기타 에셋·맵·DLL·Git index 보존. C++ 빌드/패키징·Computer Use·Git stage/commit/merge0. 새 파일은 제한된 에셋 recipe/PIE probe와 진단·상태 문서다.

재개 시 apply/delete를 다시 실행하지 않는다. 자동 회귀는 완료했고 사용자 시각 확인만 남았다. 구형 Death native2파일, 잔존 Combat/Equipment 컴포넌트, CoreRedirect/태그/기존 재질 redirector, 시작/복귀 맵·Cook·디자이너 병합·최종 패키징은 이번 승인 밖이며 그대로다. R14 전체 완료는 아니다.

추가 실행 이력: Apply_A는 삭제 노드 UObject가 GC 전에도 shared reader에 보이는 탓에 저장 전에 비교를 거부했다(변경0). 실제 graph membership에서 두 노드 부재를 확인하고 비교에서도 정확히 두 객체만 제외하도록 교정했다. Apply_B의 unexpected_asset_changes1은 Windows 경로 구분자 비교 문제로 승인된 Pawn1개였으며 다른 자산 수정이 아니었다. Delete_A는 API=true/Registry 제거에도 실제 파일이 남아 중단했고, Delete_B가 최종 파일 삭제 성공이다. 실패 보고서는 덮어쓰지 않았다.

---

## 범위와 판정

사용자가 승인한 R14-1 읽기 전용 조사와 정리 후보 분류를 수행했다. 게임 C++·Config·에셋을 수정/삭제하지 않았다. 사용자 빌드·패키징, Computer Use, Git stage/commit/merge는 실행하지 않았다. 기존 실제 에셋 이식이나 완주 검사를 반복하지 않았다.

- 저장된 `/Game`, `/DCStageUI` AssetRegistry 항목 **2,898개**, R13 명시적 시작점5개의 hard/soft package 의존 **1,291개**를 조사했다. 의존 목록 내 metadata 누락0. 이는 현재 실행 중인 UObject 목록이나 최종 Cook 목록이 아니다.
- 관련 Blueprint **26개**의 부모·그래프/핀 연결·선택 CDO·컴포넌트를 읽었다. PawnData **3개**, 보존 대상 맵 **14개**의 저장된 의존을 확인했다. 별도 PIE나 수동 입력 검사는 하지 않았다.
- 기존 native 타입 조회기로 프로젝트 Blueprint 계열 **319개**를 조사했다. **318개 조회 성공, 오래된 Death Cue1개는 실패**했다. 실패 자산에서 엔진의 로드 시 Compile 오류2개가 발생했으므로 전체 Blueprint 무오류/Compile 성공으로 표시하지 않는다. 명시적 Compile·저장은0, ensure0, 조사 대상 파일 해시 변화0이다.
- `references_D`의 보호 SHA 상태 **642개**는 조사 종료 후 재대조에서도 불변이다. 해당 실행의 전체 Content/Feature 파일 크기·수정시각 변화0. 타입 조사도 대상 Blueprint 파일 해시 변화0이다.

근거: `Saved/Diagnostics/R14/references_D/`, `classification_D.json`, `type_references_A.json`, `candidate_references_A.json`. 주 조사 스크립트는 `Scripts/Editor/r14_reference_audit.py`다. 기존 graph/AssetRegistry 조회와 `DCRifleMigrationLibrary.InspectBlueprintTypeReferences`를 재사용했고 새 C++/범용 검사 도구를 추가하지 않았다.

## R13의 실제 선택값과 잔존 참조

```text
L_DC_Level1UI
  → BP_DC_StageUIGameMode (bUseExperienceUI=true)
  → B_DC_StageUIExperience (PrimaryAsset ID로 선택)
  → DA_DC_LyraAIPawn
  → BP_DC_LyraInventoryPawn
  → BP_DC_RiflePawn (부모)
```

- Hero는 `bUseLegacyPlayerInput=false`, `bUseLegacyCameraMode=false`, `bUseOriginalADSInputRouting=true`다. 현재 PawnData의 `DefaultWeaponDefinition=None`이고 기존 원본 ID_Rifle/Inventory/QuickBar/Equipment 경로를 사용한다. `DefaultCameraMode`는 `/Game/Characters/Cameras/CM_ThirdPerson`이다.
- Pawn에는 **DCCombatComponent와 구형 DCEquipmentManagerComponent가 상속으로 남아 있다.** 원본 DCLyraEquipmentManager도 별도로 존재한다. 비어 있는 구형 장비 컴포넌트의 존재와 이중 장착/발사를 구분한다. 이번 조사에서 이중 발사·이중 피해를 재현했다고 주장하지 않는다.
- 소스상 `DCCombatComponent::TickComponent`는 자체 퍼짐 갱신을 수행한다. 현재 발사 퍼짐은 원본 WeaponState→DCLyraRangedWeaponInstance 경로다. 구형 Tick의 실측 비용·런타임 소비자는 이번에 측정하지 않았으므로 즉시 비활성화하지 않는다.
- R13 Controller의 `HUDWidgetClass=None`, `WeaponReticleHostClass=None`이고 StageUI는 Experience 경로를 사용한다. 기존 `DCStageResultWidget`은 R11/R12에 필요하며 StageOverlay 내부 표시에도 재사용된다. 임시 이름만 보고 삭제하지 않는다.
- Config의 `GameplayCueNotifyPaths`는 일반 hard/soft 의존 그래프 밖에서 Cue를 로드한다. 실제 보존 대상은 `/Game/GameplayCueNotifies/DCMigration/Death/GCNL_Death`, `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247`, Dash/AI Cue 경로다. `Diagnostics`나 `Test`라는 폴더 이름은 삭제 근거가 아니다.

## 분류 목록

| 항목 | 판정 | 근거와 후속 조건 |
|---|---|---|
| DCAbilitySystemComponent·DCAbilitySet·DCHealthSet·DCHeroComponent·DCCameraComponent/Mode·DCAnimInstance 등 | 현재 사용, 보존 | 원본 기반 대응 계층에 DC 이름이 사용된다. Lyra 접두어 유무로 구형 판정하지 않는다. |
| DCCombatComponent 및 DreamCatcherCharacter의 Legacy 입력/반동 코드 | 현재 Pawn에 잔존 + 기존 플레이어 호환, 보존 | native 생성과 기존 BP 의존이 있다. 현재 Hero 선택값은 원본 경로다. 별도 실행/소비자 확인 없이 클래스나 이벤트를 제거하지 않는다. |
| 구형 Equipment/Weapon 계층, WID_DC_Rifle·B_DC_RifleInstance·GA_DC_RifleFire | 기존 GAS 테스트 플레이어에 필요, 보존 | DA_DC_PlayerPawn은 DefaultWeaponDefinition=WID_DC_Rifle. BP_DC_Player_GAS_Test의 장착/퍼짐/해제 그래프와 DCGASTestTarget도 사용한다. |
| DCReticleWidgetBase·DCPlayerHUDWidget 및 기존 HUD BP | 기존 플레이어/맵 호환, 보존 | WBP_DC_PlayerHUD_GAS_Test→WBP_DC_RifleReticle, 기존 WBP_PlayerHUD/Controller의 의존이 있다. |
| DCHealthComponent·DCEnemyCharacter | 기존 적/스포너 호환, 보존 | 기존 EnemyCharacter와 유한 스포너의 fallback 및 테스트 의존을 유지해야 한다. 현재 보스는 DCGASEnemyCharacter/DCLyraHealth 기반이다. |
| DCCameraMode_ThirdPerson, CM_DC_Shoulder·CM_DC_Scope | 보존 | PawnData3개가 Shoulder/Scope를 참조한다. R13 Hero가 현재 이 필드를 선택하지 않더라도 기존 플레이어의 호환 참조를 해소하기 전 삭제하지 않는다. |
| CM_DC_Hip | 제거 후보 | AssetRegistry referencer0, Source/Config/Plugin 텍스트 경로·이름 참조0. 현재 기본 카메라는 원본 CM_ThirdPerson이다. |
| GA_DC_Death | 제거 후보 | 유일한 구형 Death Ability BP이며 referencer0, 텍스트 경로·이름 참조0. 현재 플레이어/적은 원본 기반 GA_Hero_Death/GA_DC_EnemyDeath다. |
| DCGameplayAbility_Death.h/.cpp | 후속 native 제거 후보, 이번 제안에서 보류 | 다른 Source/Config 소비자0, 성공한318 BP 타입 조회에서 함수/cast/핀 참조0이다. 그러나 아래 실패1개를 포함한 전체 타입 검사가 완전 통과한 상태는 아니므로 먼저 에셋 정리 후 native 삭제를 확정한다. |
| /Game/LyraMigration/Death/GCNL_Death | 제거 후보, 오류가 있는 비활성 사본 | referencer0, 텍스트 참조0, Config Cue 검색 폴더 밖. 로드 시 invalid macro graph/obsolete Skeletal Mesh Component pin 오류2개. 활성 `/Game/GameplayCueNotifies/DCMigration/Death/GCNL_Death`와 경로가 다르다. |
| CoreRedirect·GameplayTag·기존 redirector | 보존 | 클래스명/태그/PrimaryAsset ID 기반 동적 조회를 package referencer0만으로 배제할 수 없다. 삭제 근거가 확정되지 않았다. |

참조0은 조사 시점의 저장 데이터 기준이다. 사용자의 열려 있는 Editor의 미저장 변경과 디자이너 쪽 미커밋 변경은 보지 못했다. 실제 삭제 직전에 해당 자산의 해시/참조/dirty 상태를 다시 확인해야 한다.

## 실행되지 않는 구형 라이플 참조

R13에 구형 무기 의존이 들어오는 경로를 확인했다.

```text
B_DC_StageUIExperience → DA_DC_LyraAIPawn → BP_DC_LyraInventoryPawn
→ BP_DC_RiflePawn → WID_DC_Rifle → AS_DC_Rifle → GA_DC_RifleFire
```

`/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn:EventGraph`에서:

- `K2Node_CallFunction_8` (`EquipItem`)은 execute/then/ReturnValue 연결이 모두0이다. `EquipmentDefinition` 기본 핀 값만 WID_DC_Rifle을 보유한다.
- `K2Node_VariableGet_0` (`Get EquipmentManager`)의 출력은 위 노드 self에만 연결된다.
- 조사한 이 BP 그래프에서 WID_DC_Rifle 리터럴은 이 핀1개뿐이다.

**이 두 노드만 제거하는 것이 가장 작은 실제 정리안**이다. 원본 Equipment/발사/조준·입력/애니메이션과 컴포넌트는 유지한다. R13까지 상속되는 부모 BP를 저장하므로 변경 전 그래프/해시 확인, 제거 후 나머지 그래프 동일성·Compile·R13에서 구형 WID 의존 경로가 줄었는지 확인한다. 남아 있는 구형 장비 진단 그래프나 키 입력을 포괄 삭제하지 않는다.

## Redirector 기록 교정

R13의 기존 이식 계획에서 중간 경로였던295개를 다시 조회했다. **현재 on-disk 파일0, AssetRegistry 항목0**이다. 따라서 이전 인계의 ‘R13 중간 redirector를 유지했다’는 표현은 현재 잔존 상태의 근거로 사용할 수 없다. 이번 조사에서 삭제한 것은 아니다. 과거 메모리 객체/중간 경로 이력을 현재 파일 잔존과 혼동하지 않는다.

프로젝트 전체에는 기존 redirector97개가 있으며 R13 정적 의존에 포함되는1개도 있다. 이들은 위295개와 다르며 일괄 정리하지 않는다. `candidate_references_A.json`에 실제 목적지와 referencer를 기록했다.

## R14-2A 제안 이력 — 이후 사용자 승인 및 위 적용 결과로 갱신

**에셋4개에 한정**한다.

1. `Content/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn.uasset`: 위 실행되지 않는 노드2개만 제거하고 Compile/저장.
2. `Content/DreamCatcher/GAS/Abilities/GA_DC_Death.uasset`: Unreal API로 제거.
3. `Content/DreamCatcher/Camera/Modes/CM_DC_Hip.uasset`: Unreal API로 제거.
4. `Content/LyraMigration/Death/GCNL_Death.uasset`: 오류가 있는 비활성 사본만 Unreal API로 제거.

원본과의 기능 차이는 없다. 1은 실행선 없는 편집 잔재, 2~4는 사용 경로가 확인되지 않은 이전 구현/실패 사본이다. 활성 원본 Death/Camera/Cue와 기존 플레이어의 실행 그래프는 유지한다. 3개 삭제 직전 참조·해시·dirty 검사를 통과하지 못하면 해당 삭제를 수행하지 않는다. 복구용 원본은 Unreal 내부 복제/내보내기 또는 이미 확인된 버전 관리 상태로 확보하고, 외부 파일 I/O로 바이너리를 직접 복사하지 않는다. 복구용 사본이 정식 Content/Cook에 남아 미사용 의존을 다시 넣지 않도록 범위를 명시한다.

검증은 대상 Blueprint 및 활성 Death Cue의 로드/Compile, BP 부모·그래프/원본 장착 경로 보존, 삭제 참조 없음, R13에 필요한 의존·Cue 경로 보존이다. 변경된 장착/사망/카메라 범위의 짧은 회귀만 수행하며 전체 마이그레이션을 반복하지 않는다. C++·Config·맵 수정이 없으므로 **이 묶음 자체에는 추가 C++ 빌드가 필요 없다.** 사용자 확인은 기존 플레이어의 장착/발사, 사망 연출, 카메라 확인으로 한정한다.

구형 Death native2파일과 잔존 컴포넌트 정리는 이후 승인 범위다. 현재 기본 시작 맵/ReturnMap/Cook 설정, 디자이너 브랜치 통합, 최종 사용자 빌드·패키징도 이번 에셋4개 제안에 포함하지 않는다. R14 전체 완료가 아니다.

## 실패 이력과 재사용

- `references_A`: UClass를 Python isinstance의 type으로 전달한 진단 스크립트 오류. 저장0.
- `references_B`: 이미 최종 경로로 이식된 메뉴에 중간 경로가 있다고 가정하여 missing referencer 반환값을 처리하지 못함. 저장0.
- `references_C`: 클래스 경로 구조체의 표시 문자열 때문에 PawnData/World 분류0. complete 필드가 true여도 그 두 범위를 검증한 결과가 아니다.
- `references_D`: 클래스 경로의 package/name 필드를 사용하여 PawnData3/보존맵14 보완. 26개 BP 결과는 C의 보호 해시 불변을 확인하고 재사용. 주 조사 종료0/Error·ensure·Fatal0, 저장0.
- `type_references_A`: 명시적 Compile은 하지 않았지만 로드 시 오래된 Cue1개 Compile 오류2개, 프로세스 종료1.318개 타입 조회와 실패1개를 함께 보존한다. 전체 성공이라고 보고하지 않는다.
- `candidate_references_A`: 후보3개와 Config Cue 폴더/실제 사용 redirector의 registry 조회. 에셋 로드0/저장0/종료0. Source/Config/Plugin 텍스트 참조 결과도 수집했다.

재개 시 위 완료한 조회를 무조건 재실행하지 않는다. R14-2A가 승인되면 변경 대상과 삭제 직전 안전 조건만 최신 상태로 재확인한다.
