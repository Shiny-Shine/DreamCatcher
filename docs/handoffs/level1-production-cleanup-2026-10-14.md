# Level_1 정식 경로 전환 및 레거시 정리

## 최신 — 잔여 플레이어 애니메이션·입력·블록아웃 정리 완료 (0.113)

사용자가 남은 레거시 여부를 확인하고 추가 정리를 승인했다. 0.112는 당시 지정한 정리 묶음의 완료였으며 프로젝트 전체의 미사용 에셋이 0이라는 뜻이 아니다. 최신 참조를 다시 읽고 다음 **23개**를 Unreal 내부 API로 제거했다.

- 옛 플레이어 애니메이션5개: `ABP_DC_Player`, `ABP_DC_Player_GAS_Test`, `AM_Roll_Placeholder`, `ABP_DC_RifleAnimLayers`, `ALI_DC_WeaponAnimLayers`.
- 옛 입력9개: `/Game/Input`의 `IMC_Player`, `IMC_Default`, `IMC_MouseLook`, 미사용 Action `IA_Dodge`/`IA_Jump`/`IA_MouseLook`, 템플릿 Touch UI/Thumbstick/Interface3개. 정식 Lyra 경로의 같은 이름 Action은 삭제하지 않았다.
- `DreamCatcher_ENV/Map/Test_Map/Test_Map` 및 그 전용/고립 generated 메시8개. 이 맵은 2026-02-26 `Build Test Map 1-1` 커밋의 액터25개 블록아웃이며 GameMode override/외부 참조/실행 설정 연결이 없었다.

현재 입력에서 참조하는 `/Game/Input/Actions/IA_Aim`, `IA_Fire`, `IA_Look`, `IA_Move`, `IA_Ultimate`는 보존했다. 환경·캐릭터 자료를 참조하는 디자이너 `Asset.umap`과 연결 메시 `Box_34DB3452`, Config CoreRedirect 호환 경로의 `Struct_UIMessaging`도 보존했다. `Characters/Mannequins`에는 현재 보스 및 Level_1/Level_2가 참조하는 `SKM_Quinn_Simple` 등이 있으므로 폴더를 일괄 삭제하지 않았다. 공용 라이브러리와 회귀 검사 전체를 미사용으로 판단한 작업은 아니다.

외부 package 참조0 및 Source/Config/현재 Editor 스크립트의 대상 경로 문자열 참조0, 삭제 전 Git LFS 복구 payload23개를 확인했다. **22:49 새 프로세스 검증 성공**, 삭제 파일23개만 변경, 끊어진 참조0/Error·ensure0/보호 파일 변경0이다. 두 실사용 Pawn의 `SKM_Manny`/`ABP_Mannequin_Base` 및 현재 입력 데이터 로드도 확인했다. Level_1/실사용 에셋/Source/Config/DLL/사용자 저장/Git 인덱스는 그대로이므로 기존 플레이 수용을 유지하며 C++ 빌드/PIE 재실행은 하지 않았다.

정확한 목록은 [잔여 정리 목록](level1-residual-cleanup-2026-10-14.json), 근거는 `Saved/Diagnostics/ResidualLegacy/{before_A.json,inspect_A.json,manifest_A.json,cleanup_Delete_A.json,cleanup_Verify_A.json,closeout_A.json}`이다. 아래0.112 수용과0.111 검증은 유지한다. 패키징과 Git 커밋/푸시는 별도 상태다.

## 최신 상태 — Level_1 정식 전환·레거시 정리 및 사용자 수용 완료 (0.112)

사용자가 새 DLL 검증 후 **“정상 작동을 확인했어”**라고 최종 확인했다. 이에 현재 Level_1 전환, 실사용 정식 경로 이관, 승인된 테스트/레거시 정리 묶음을 **사용자 수용까지 완료**로 기록한다. 아래 자동검증 결과를 반복하지 않았으며 이번에는 완료 상태 문서만 갱신했다. 패키징은 기존 지시대로 별도 보류·미검증이다.

사용자 Development Editor / Win64 빌드는 **21:57 성공(8.75초)**했다. `DCCameraMode.h` include 누락 오류는 해결됐으며 DLL은 5,724,672 bytes, SHA256 `f863512bb8eb923a450e349f692f7e4eb21b618290ec9a1b3b8505d6710bffaf`다. 이번 검증에서 C++/Config/프로젝트 설정과 DLL은 변경하지 않았다.

- 직접 영향 Blueprint11개 재로드/Compile 통과. 검사한 삭제 native19종 등록0, Game 및 DCStageUI 마운트의 폐기 package 참조0, 실제 Pawn 컴포넌트에서 구형 장비/전투/카메라 부품0. 환경4642개 및 같은 맵 ReturnMap을 확인했다.
- 입력/탄환, 장비, GAS 적, 유한 Lyra 스포너, 스테이지, 보스 소유권 정리, 메뉴 회귀 **22건 성공/실패0**. 빈 optional 스포너의 즉시 완료를 확인하는 의도된 경고1건이 있다.
- 두 플레이어 BP에 삭제한 native CombatComponent의 serialized export가 남아 발생하던 로드 경고를 처리했다. `BP_DC_RiflePawn` 및 `BP_DC_LyraInventoryPawn`만 새 DLL로 재저장했으며 그래프·메시·AnimClass·캡슐·이동 수치·입력 설정을 비교해 유지했다. 별도 새 프로세스 재로드에서 해당 경고0, 예상 밖 에셋/보호 파일 변경0이다.
- 최종 **22:23 `Level1Flow_PostBuild_B` 실제 PIE 성공1(경고 포함)/실패0/Error·ensure0**. 발사, 견착/Hip 및 스코프/Hip 입력 전환, 웅크리기/해제 API, 실제 AI3지점 이동(1521.18cm), 일반전투2→보스/HUD→완료/정리, 결과/메뉴의 같은 맵 복귀와 이동 입력 복구를 통과했다.
- 추가로 실제 플레이어에게 진단 GAS 치명 피해를 적용해 실패 결과, Pawn/장비/AI 정리, 자동 리스폰 없이 대기, 명시적 재시작 후 체력/탄약30/입력/첫 인카운터 복구를 확인했다. 자연 조작감·애니메이션 화면 수용은 사용자 확인으로 구분한다.
- PIE 경고18은 기존 Manny PoseAsset14 및 월드 초기화 Crowd/Recast4이다. 캐릭터 리로드의 기존 LyraGame import 경고와 UBT의 DataRegistry 명시적 의존성 경고도 별도로 남긴다. 이번 Editor 빌드/실행은 통과했으며 패키징은 미실행이다.

`PostBuild_A`의 확장 사망 검사는 Pawn의 Possession 해제 직후 즉시 파괴를 요구해 실패했다. 실제 원본은 detach 뒤 `SetLifeSpan(0.1f)`로 파괴하므로 검사만 그 유예를 기다리도록 교정했다. C++ 동작 변경은 없으며 B만 최종 통과 근거다.

이번 추가 변경은 플레이어 BP2개, `Scripts/Editor/level1_gameplay_probe.py`의 영향 경로 검사 및 문서다. 추가 C++ 빌드는 필요 없다. 복구 캐시는 `pawn_resave_recovery_A.json`, 검사 근거는 `user_build_A.txt`, `postbuild_Assets_A.json`, `pawn_resave_Save_A.json`, `pawn_resave_Verify_A.json`, `postbuild_closeout_A.json` 및 `Saved/Automation/{Level1PostBuildCore_A,Level1Flow_PostBuild_B}/index.json`이다. Git 인덱스/사용자 SaveGames/맵은 보존했고 커밋·푸시·브랜치 조작은 하지 않았다.

## 이력 — 정리 적용 직후 C++ 빌드 대기

에셋 이관/정리 후 기존 검증 DLL로 `Level1Flow_Cleanup_C`가 **21:41 성공1(경고 포함)/실패0/Error·ensure0**으로 종료했다. 실제 AI MoveTo 3지점 도달(변위 합1289.85cm), 발사/견착/Hip, 일반전투2→보스/HUD→클리어/정리, 결과 및 메뉴 확인창의 같은 맵 재시작과 이동 복구를 확인했다. 경고17은 기존 Manny PoseAsset14와 월드 초기화 Recast/Crowd3이다.

그 뒤 구형 native/템플릿/도구를 정리했다. **새 C++의 UHT·컴파일·링크 및 새 DLL 기준 Blueprint/PIE는 아직 미검증**이다. Codex는 C++ 빌드/패키징을 실행하지 않았고 기존 게임 DLL 해시는 그대로다. 사용자가 Editor를 닫은 상태에서 Development Editor / Win64를 빌드하면 아래 재개 절차를 따른다.

사용자 빌드에서 `DreamCatcherCharacter.cpp`의 `TSubclassOf<UDCCameraMode>` 유효성 검사에 필요한 완전한 타입 정의가 없어 C2027/C2672가 발생했다. 정리 중 누락한 `Camera/DCCameraMode.h` include 한 줄을 복원했다. 동작·에셋·Config 변경은 없으며 재빌드 결과는 대기 중이다.

## 승인과 보존 기준

사용자가 Level_1 PIE 정상 동작을 확인하고 실사용 에셋의 정식 경로 이관, 테스트 맵 및 레거시 정리를 승인했다. R0~R14 수용은 유지한다. Level_1의 환경 배치 4,642개, 일반 인카운터 2회 → 보스 → 완료, 자동 리스폰 없음, 결과/메뉴의 돌아가기 → 같은 Level_1 재시작을 보존한다. 빌드·패키징·자연 조작/화면 수용은 사용자 담당이다.

작업 기준은 main `3665f3ba690bab286784da7aae183b7485932cd0`과 사용자 수용된 미커밋 Level_1이다. Git stage/commit/branch/merge/push는 하지 않는다. 미커밋 Level_1 및 BP_DC_Level1GameMode는 Git LFS clean 필터로 복구 캐시만 확보했다. 인덱스 변경은 없었다. Unreal 바이너리는 Unreal 내부 이동·저장·삭제 API만 사용한다.

## 실사용 경로

- 플레이어/Controller: `/Game/DreamCatcher/Blueprints/Player`
- 일반 AI/보스: `/Game/DreamCatcher/Blueprints/AI`, `Data/Pawns`, `Data/Boss`
- GameMode: `/Game/DreamCatcher/Blueprints/Core`
- Experience/ActionSet: `/Game/DreamCatcher/Experiences`
- 프로젝트 GAS/Input/보스 FX: 각각 `DreamCatcher/GAS`, `Input`, `FX/Boss`
- Lyra 공용 라이브러리: `/Game/DreamCatcher/Lyra`; 라이플은 `Lyra/Weapons/Rifle`, HUD/메뉴는 `Lyra/UI/Stage`, 공용 UI는 `Lyra/UI/Shared`, 사망 Cue 묶음은 `Lyra/GameplayCues/Death`
- 라이플 발사 MetaSound: `/Game/DreamCatcher/Audio/Weapons/MSS_DC_Rifle_Fire`
- 보존한 장비 회귀 fixture 4개: `/Game/Tests/Equipment`, `DirectoriesToNeverCook`로 게임 패키지에서 제외

총 782개 이관 대상: 기존 Test 경로의 실사용 31개, LyraMigration 584개, 사망 Cue 의존 묶음 163개, 장비 회귀 데이터 4개다. basename을 유지해 Experience의 PrimaryAssetId가 바뀌지 않는다. 전체 Lyra 라이브러리의 미사용 여부를 추측해 삭제하지 않는다.

`DefaultGame.ini`의 PawnData fallback, Experience/ActionSet 스캔, GameplayCue/UI 설정과 Windows 입력 장치 설정, 두 기존 PackageRedirect의 목적지를 새 경로로 갱신했다. 전역 구형 GameMode fallback은 Engine.GameModeBase로 바꿨다. Level_1은 명시적인 BP_DC_Level1GameMode override를 유지한다. 다른 환경 맵에 StageDirector가 없는 상황에서 임의로 스테이지 실행을 시작하지 않는다.

## 의도된 정리 차이

- PawnData의 빈 R4 Set, TagFire/TagReload 검사 Set, SignatureSmoke Set을 해제한다. Q의 TEST 출력/검사용 2초 쿨다운은 제거되며, 실제 시그니처 효과를 새로 만들지 않는다. 시그니처 기반 BP와 입력 확장점은 유지한다.
- 실제 Pawn의 구형 EventGraph 47개 노드, 구형 EquipmentManager SCS 1개, 사용하지 않는 구형 피해 GE/견착·스코프 카메라 필드를 제거했다. QuickSlotInput/ChangeQuickbarSlot과 Lyra 장비·CharacterParts·메시·AnimClass 설정은 보존 검사했다.
- 구형 플레이용 맵 9개, 구형 플레이어/적/HUD/장비 및 테스트 데이터, 옛 Box_1 복사본을 제거한다. 디자이너의 `Prop/Mesh/Box_1`과 환경/건물/장식은 보존한다.
- 원본 GAS/Lyra/Stage/Boss 계층과 핵심 회귀 검사는 유지한다. 기존 플레이어 native 기반 자체를 삭제하는 작업이 아니다.

## 자동화 이력과 근거

`Saved/Diagnostics/Level1Cleanup/`에 각 시도의 실패와 성공을 모두 보존한다.

- `promotion_recovery_A.json`: 에셋 854개 복구 payload 및 초기 보호 해시. 미커밋 수용본 2개 포함.
- `retirement_recovery_A.json`: 추가 구형/템플릿 94개 복구 payload.
- 이동 A~C: CDO 설정 확인창 및 Python 속성 접근/형식 문제로 저장 전 중단, 에셋 변경 0.
- 이동 D: Commandlet의 DataRegistry 미초기화와 `-nop4`만으로 Git provider가 꺼지지 않는 문제를 확인해 해당 프로세스만 중단. 신규 에셋 1개만 생성됐고 원본/인덱스는 보존. E에서 Unreal 삭제 API로 그 신규 파일을 회수했다.
- 이동 E: `-SCCProvider=None`과 UI CDO의 프로세스 전용 None override를 사용한 전체 Editor 이동. 781개 목적지는 저장됐으나 이동 콜백 중 구 경로 재로드/AnimGraph/ControlRig 오류가 있었으므로 성공으로 기록하지 않는다.
- 새 프로세스 재로드: 781개 모두 로드. 핵심 BP 31개의 원래 노드/핀/컴포넌트 비교. WeaponAudioFunctions의 신규 UObject 5개는 실제 그래프 멤버가 아닌 연결 없는 컴파일 확장 노드임을 별도 확인했다. 이를 무시하는 범위는 신규·비멤버·연결 0인 노드로 제한한다.
- `promotion_Resave_B.json`, `promotion_Resave_C.json`: 성공, 환경 4,642개 동일, 보호 파일/예상 밖 변경 0. Level_1 및 직접 참조 저장, DataRegistry 경로 갱신.
- `audio_promotion_A.json`: 삭제 직전 검사에서 실제 발사 Cue의 MetaSound 참조 1개를 발견, 삭제 목록에서 제외하고 정식 경로로 추가 이관. 프로세스 종료 0, Error/ensure 0.
- `retirement_Detach_A.json`: Pawn 정리 및 자식 Compile/저장 성공, 다른 그래프/메시/애니메이션 설정 유지.
- 삭제 A~F: 저장 전 보수적 타입/패키지 검사 및 위 MetaSound 참조 검사에서 중단. 실제 파일 삭제 0. 타입 비교는 표시 문자열의 메모리 주소를 제외하고 명시적 CoreRedirect를 반영하며, Blueprint 패키지 안의 본체/생성 클래스 redirector를 함께 다룬다.

## native 및 개발 잔재 정리

- 구형 Combat/Health/Equipment/Weapon/Reticle/HUD/Camera/Enemy14계열28파일, 구형 GameMode2, 테스트 표적4, 구형 Aim Ability2 = **36파일 제거**.
- 템플릿 Variant_Combat/Platforming/SideScrolling **76파일 및 Build.cs include 경로 제거**.
- ADreamCatcherCharacter/PlayerController 자체는 유지한다. ASC/Health 생명주기, crouch, 사망 완료 정리, 현재 GAS 조준 상태/감도와 원본 카메라 보조/ASC 입력 처리를 보존하고 옛 직접 발사·회피·HUD·SpringArm/FollowCamera·무기 모델·피해 어댑터만 제거했다. ACharacter의 native Tick은 유지한다.
- PawnData의 ShoulderCameraMode/ScopeCameraMode/DefaultWeaponDefinition과 Hero의 전환용 Legacy 플래그를 제거했다. 현재 Pawn은 이미 원본 Hero 입력/카메라 경로였으며, Hip/Shoulder/Scope 제스처 어댑터는 유지한다.
- 실사용 Controller2쌍은 `Source/DreamCatcher/Player/DCLoadoutController.*`, `DCLyraAIIntegrationController.*`로 옮겼다. serialized 클래스명/장착·해제 API는 유지하고, 함께 들어 있던 R5 테스트 GameMode만 제거했다.
- 유한 스포너의 구형 EnemyClass 경로만 제거했다. GAS 보스/Lyra 봇 생성, 사망 집계·객체 정리·실패/인카운터 완료 처리는 유지한다. 기존 미설정 optional 스포너의 즉시 완료 정책도 이번에 변경하지 않았다.
- C++ 회귀 검사37파일은 보존한다. 구형 Enemy 검사 부분만 제거하고 현재 GAS teardown 검사를 유지했다. 장비 fixture 경로와 메뉴 복귀 목적지를 새 경로로 갱신했다. `Test.R2.CostCharge`는 해당 회귀 데이터가 사용하므로 유지한다.
- 이식 전용 `DCRifleMigrationTools` 플러그인과 `.uproject` 등록 제거, `백업/`13파일 및 과거 Editor 스크립트/계약검사/추적 pycache 정리. 총130개 기존 추적 파일이 이 도구 묶음에서 제거된다. 게임용 Win64 DLL과 런타임 플러그인, 사용자 저장 파일은 보존했다.
- Editor 레시피97파일은 해시를 비교한 `Saved/Diagnostics/Level1Cleanup/retired_editor_recipes.zip`에 보존했다. 현재 유지할 재현 스크립트는 `Scripts/Editor/level1_gameplay_probe.py` 하나이며 이식 플러그인에 의존하지 않는다.

정식 경로 전체 대응은 [에셋 경로 목록](level1-asset-paths-2026-10-14.json), 삭제 대상은 [정리 목록](level1-retirement-2026-10-14.json)에 남긴다. 테스트 맵/구형 에셋98개와 잔존 사본·redirector178개를 합쳐 **276패키지를 Unreal API로 삭제**했고, 그 외782개는 이관했다. 추가로 제작 자산을 미사용이라고 추측해 삭제하지 않았다.

## 중간 실패의 후속 판정

- 삭제 G 정상 완료: 남은 대상 파일0, 외부 참조0, 예상 밖 에셋/보호 파일 변경0.
- `production_verify_A.json`: 782개 목적지 로드, Game 경로 폐기 참조0, 오류가 보고됐던 AnimBP2개 Compile와 MaterialFunction6개 expression 존재 확인, 환경4642개 동일. 이후 실제 PIE에서 발견한 플러그인 마운트 참조는 아래에서 별도로 보완했다.
- `/DCStageUI/DCStageUI`의 알림/Registry 참조3개는 A 저장 후 새 프로세스 B에서 모두 새 경로로 확인했다. A의 저장 직후 Registry 조회는 이전 캐시를 읽어 assertion 실패했으며, 실제 데이터는 저장됐다. `feature_paths_B.json`이 새 프로세스 검증 근거다. 기존 GameFeature의 Action 구성/클라이언트·서버 적용 플래그는 유지한다.
- Nav 갱신 A는 ExecutePythonScript 자동 종료로 저장 전 끝났다. B/C는 저장/환경 보존만 통과했고 AsyncLoadLock(0x20)으로 생성은 수행되지 않았다. D는 저장 전용 프로세스에만 `bWaitForAsyncLoadingBeforeBuildingNavigationAutomatically=False`를 적용하고 에셋 준비 후 RebuildNavigation을 실행했다. AgentRadius35/AgentHeight144/Dynamic/볼륨은 유지했고, 영구 Config는 바꾸지 않았다.
- PIE Cleanup_A/B는 실패 이력으로 보존한다. 최종 **Cleanup_C는 위 프로세스 옵션 없이 일반 프로젝트 설정으로** 모든 이동·전투·복귀를 통과했다. 따라서 Nav 저장만으로 이동 성공을 주장하지 않는다.

## 다음 — 사용자 검토·커밋

1. 사용자 최종 Level_1 확인은 완료됐다. 추가 빌드나 같은 검증을 반복할 필요는 없다.
2. 사용자가 GitHub Desktop에서 작업 브랜치의 변경을 검토·커밋/푸시한다. 정식 경로 및 삭제 JSON 목록을 검토 기준으로 사용하며 main 직접 커밋을 피한다. 이번 완료 기록은 Codex의 Git 작업 승인이 아니다.
3. 패키징은 별도 보류 상태를 유지한다. 후속 기능 작업은 새 변경 범위를 정한 뒤 진행한다.

패키징은 사용자 지시대로 계속 보류·미검증이다. 보호된 기준 게임 DLL SHA256은 `d5b30577e3aa9c5fc585a8a30abf96b87f979824fdff2ea835485a39f4aea099`다. 복구/검증/정적 검사 근거는 `promotion_recovery_A.json`, `retirement_recovery_A.json`, `retirement_Delete_G.json`, `production_verify_A.json`, `feature_paths_B.json`, `navigation_refresh_D.json`, `pie_Cleanup_C.json`, `native_cleanup_A.json`, `editor_tools_cleanup_A.json`에 있다.
