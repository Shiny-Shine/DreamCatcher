# 프로젝트 안정성 — 2026-11-17

## 현재 상태와 승인 범위

**최종 갱신 2026-11-18 / AGENTS0.151: 사용자 일반 Editor 빌드 후 장비 검사2건과 두 맵 반복 PIE 총4회 통과.** 실제 장착·해제·재장착·탄약 유지·월드 종료 정리에서 크래시는 재현되지 않았다. 사용자 수동 플레이와 프로젝트 전반의 안정성/최적화 완료를 의미하지 않는다.

사용자는 전반적 안정성 TODO1~5를 1차 승인하고, A~D(소유 관계 보완·초기 장착 조건·해당 회귀 검사·문서)를 최종 승인했다. 기존 기능·미커밋 변경을 보존하며 C++ 빌드는 사용자가 수행한다. 코드 준비 중에는 다른 채팅 `Game Animation Sample 추가 확인`과의 충돌을 피하기 위해 에디터를 실행하지 않았다. 후속 검증도 상대 세션의 조준/Traversal 검증 완료·idle와 Unreal 프로세스 부재를 확인한 뒤 순차 실행했다.

## 확인 근거와 한계

- 최신 실제 PIE 실패: 2026-11-17 21:46 KST, `/Game/DreamCatcher/GAS/Test/GASP/L_GASPMovementTest`.
- PawnExtension/Hero가 GameplayReady에 도달한 직후 장착 타이머에서 `TryEquipWhenReady → QuickBar.SetActiveSlotIndex → EquipItem → EquipmentList.AddEntry`가 실행됐다. 당시 소스75행 `OwnerComponent->GetOwner()->HasAuthority()`에서 주소 `0x1a4` 읽기 접근 위반이다.
- 근거: `Saved/Crashes/UECC-Windows-0868C9DB440AF569BA8784AA12137CA0_0000/CrashContext.runtime-xml` 및 같은 폴더 `DreamCatcher.log`의3615~3643행/오류 스택.
- 동일 장비 호출 스택·주소는 GASP 병합(11/5, e7ec1cc) 전에도 있다. 10/15 `UECC-Windows-817353E74D507E7047CA149096D8445B_0000`, 10/16 `UECC-Windows-7BD93B294F72A5B8AD47D09A7ED6BB90_0000`의 Level_1 PIE이며 후자는 같은 세션의 두 번째 PIE다.
- 원본 Lyra도 생성자에서 EquipmentList(this)를 설정하고 InitializeComponent에서 소유 참조를 다시 지정하지 않는다. DreamCatcher의 Blueprint SCS 장비 컴포넌트에도 이 경로가 사용된다.
- `OwnerComponent`는 NotReplicated이지만 UActorComponent의 DefaultToInstanced 때문에 현재 UHT 결과에는 InstancedReference가 포함된다. **정상 instancing에서는 self-reference가 자동 보정될 수 있으므로 템플릿 복사 자체가 항상 오류를 만든다고 주장하지 않는다.** 기존 정상 PIE 내보내기에도 올바른 소유 참조가 있다.
- 이번 보완은 소유 불변조건을 초기화 때 명시하고 잘못된 요청을 막는다. 역사적 크래시 당시 어느 객체의 참조가 왜 어긋났는지는 아직 미확정이다. 새 검사의 의도적 오류 주입은 그 간헐 조건의 재현을 대신하지 않는다.
- 후속 Native_A에서 **의도적 주입 전의 정상 사용자 지정 템플릿 복사도 이전 템플릿을 OwnerComponent로 유지하는 경우**를 실제 관찰했다(로그2422~2423행). InitializeComponent 이후 런타임 인스턴스 소유·정상 장착을 검사해 통과했다. 이 메커니즘 확인을 모든 역사적 PIE 실패의 단일 원인 확정으로 확대하지 않는다.
- 11/10 GASP 애니메이션 CacheTasksByKeyHash assert와 11/17 PythonScriptPlugin 크래시는 별도 자동화 프로세스 기록이다. 같은 애니메이션 assert는10/23 Inspector 작업에도 있다. 전체64개 CrashContext를 사용자 PIE64회로 집계하지 않는다.
- 최신 PIE 로그에는 Live Coding 초기화만 확인되며 실제 patch 적용 증거는 찾지 못했다. Live Coding이나 메모리 부족을 원인으로 확정하지 않는다.

## 작성한 코드와 보존한 계약

1. `Source/DreamCatcher/Equipment/Lyra/DCLyraEquipmentManagerComponent.cpp`
   - InitializeComponent에서 EquipmentList.OwnerComponent를 실제 this로 재연결한다. 이전 참조가 다를 때만 Verbose 진단을 남긴다.
   - 템플릿·파괴 중 컴포넌트·목록 소유 불일치, 무효/파괴 중/권한 없는 소유 액터의 장착 요청은 경고와 nullptr로 거부한다. 거부 전에 목록에 엔트리를 추가하지 않는다.
   - ASC 조회에서 유효하지 않은 소유 액터를 처리한다. 정상 장비 생성·Ability 부여·QuickBar/Instigator·복제·해제 순서와 에셋 설정은 유지한다.
2. `Source/DreamCatcher/Player/DCLoadoutController.cpp`
   - 종료/파괴·Possession 불일치 시 대기 타이머를 정리한다.
   - 현재 Pawn과 BoundPawnExtension/장비의 소유 관계, 장비 등록·InitializeComponent 완료를 기존 GameplayReady 조건과 함께 확인한다.
   - 기존0.05초/100회 대기, 무기·아이템·탄약·피해 계산은 변경하지 않는다.
3. `Source/DreamCatcher/Tests/DCEquipmentAutomationTests.cpp`
   - 기존 `DreamCatcher.R5.Equipment.QuickBarLifecycle`은 유지한다.
   - `DreamCatcher.R5.Equipment.TemplateOwnerLifecycle` 추가. 임시 Game 월드·소유자 없는 템플릿에서 컴포넌트를 생성하고 초기화 전 참조를 기록한다. 두 번째 수명에는 오래된 템플릿 참조를 의도적으로 주입한다.
   - 실제 장착/해제/재생성, 올바른 Pawn outer, 목록 소유 불일치/파괴된 컴포넌트/소유 액터 없는 요청의 거부, 템플릿 엔트리 불변을 확인한다. 오류 경고5건은 의도한 거부 경로이며 기대 메시지로 지정했다.
   - 기존 임시 검사와 동일하게 `-DCR5IsolatedAutomation`이 필요하다. 에셋·CDO를 수정하지 않으며 네트워크 복제나 실제 PIE 검증으로 보고하지 않는다.

## 동시 작업·변경 범위

- 코드 준비 단계의 변경은 위 C++3개, 루트 AGENTS.md, 기능 명세, 이 인계 문서다. 후속 검증 중에는 C++·헤더/API·에셋·Config·플러그인을 변경하지 않았고 빌드·Git stage/commit/push/merge도 하지 않았다. 검증용 임시 관찰기1개는 사용 후 삭제했다.
- 미커밋 ASC2개·DCHero·DCTraversalCombat2개·Traversal 검사, GASP 플레이어/애니메이션/Experience/테스트 에셋, 기존 AC_TraversalLogic·SandboxCharacter_CMC_ABP·스켈레톤·DLL 변경은 다른 작업으로 보존한다. 현재 상태를 HEAD나 과거 진단 해시로 되돌리지 않는다.
- 다른 세션의 AC_TraversalLogic·조준 입력 보완을 포함한 전체 변경 목록을 이번 작업의 산출물로 표시하지 않는다. 해당 세션의 검사 성공을 이 코드 검증으로 대신하지 않고, 같은 새 DLL에서 장비 검사를 별도로 수행했다.
- 조사 에이전트2개 결과를 회수하고 완료 상태를 확인했다. 완전 종료 도구는 없어 추가 호출/대기를 중단했다. 의존적인 구현은 부모가 직접 순차 처리했다.

## 검증 결과와 다음 순서

- 사용자 DLL/PDB: 2026-11-17 23:36:23 KST. DLL6,939,648 bytes, SHA256 `89d01932d3b681aa20a6a5a5bb04112a268b99b85f6ba4b96178cbb1dc1b55c5`. 이후 C++ 변경0·추가 빌드 불필요.
- `Saved/Diagnostics/ProjectStability20261117/Native_A.log`와 `Native_A_Automation/index.json`: 장비2건 Success, 테스트 경고/실패0, 종료0. 의도한 거부5건은 기대 메시지로 검증했다.
- `GASP_E.json/.log` 및 `GASP_E_Automation/`: GASP 테스트맵 첫 PIE/재실행2회. `Level1_A.json/.log` 및 `Level1_A_Automation/`: Level_1 첫 PIE/재실행2회. 각 맵은 별도 프로세스에서 `Project.Maps.PIE -TestLoops=2`로 실행해 같은 에디터 프로세스에서 PIE 종료 후 다시 시작했다. 레벨 OpenLevel만으로 재실행을 대신하지 않았다.
- 최종4회 모두 실제 Pawn·장비·QuickBar/Inventory 아이템 연결, 명시적 해제/무기 액터 파괴, 새 장비 재장착·탄약30 유지, EndPIE 이후 무기 액터 부재를 확인했다. 내부 목록 소유 참조는 native 검사/EquipItem 진입 조건으로 확인하고 Python은 공개 런타임 소유 관계를 관찰했다.
- 최종 Native_A/GASP_E/Level1_A는 Error/ensure/Fatal0, 종료0. **경고0 실행은 아니다.** 원본 경고(Automation 재출력 제외)는 GASP16줄·Level_1 21줄이다. GASP의 PoseAsset/import·디버그 CVar 반복 조회와 Level_1의 NavMesh origin/tile 불일치가 남는다. CrowdManager 경고는 두 회차 모두 BeginTearingDown 직후 발생했다. NavMesh 전체 이관/AI 경로를 다시 수정·검증한 것은 아니다.
- Automation JSON은 마지막 pass 결과를 담아 GASP warnings0, Level_1 warnings2다. 첫 pass 경고를 포함한 로그를 함께 보존하며 마지막 JSON만으로 전체 경고0을 주장하지 않는다.
- `build_baseline.json`의 보호 대상30개 파일과 Git 스테이징이 검증 후 동일하다. 별도 UserDir/GameUserSettingsINI와 NoAutoSave/NoSaveConfig를 사용했고 Content/Config 저장0이다. 새 코드나 에셋 변경 없이 종료했다.
- 초기 관찰기 시도 A는 Python 자동 종료로 PIE 시작 전 종료했다. B는 Controller Python 함수명, C는 보호된 내부 속성 조회, D는 EndPIE GC 후 무효 wrapper 조회로 실패했다. 로그/실패 JSON을 보존했고 관찰기만 교정했다. 최종 E와 Level1_A만 완료로 표시한다.
- 임시 `equipment_pie_probe.py`는 호출·import 참조와 프로세스 종료를 확인해 삭제했다. `final_result.json`에 관찰기 SHA256, 성공/실패 시도, 보호 확인, 검증 한계를 기록했다. 로그·관찰 JSON·Automation 보고서는 남긴다.
- 다음은 사용자 화면·조작감 확인과 별도 승인할 후속 최적화다. 동일 자동 검사나 완료된 마이그레이션을 반복하지 않는다. 재발하면 소유 참조 진단과 맵/첫 실행·재실행 순서를 대조하고 새 근거에 따라 수정 범위를 제시한다.

## 후속 최적화 후보 — 이번 수정 제외

- `Config/DefaultEngine.ini`: GASP 병합이 bSubstepping=True/MaxSubsteps=16 및 unlimited bone influences를 추가했다. 실제 물리/애니메이션 비용과 HARU·Traversal·ragdoll 영향을 측정하기 전 값을 변경하지 않는다.
- `Config/DefaultGame.ini`: Experiences 경로 AlwaysCook에 B_DC_GASPMovementTest가 포함될 수 있다. 실제 manage/soft/hard 의존성과 테스트맵 배포 의도 확인 후 좁은 Cook 규칙을 별도 승인한다. GAS/Test 전체 제외는 금지한다.
- PoseAsset 원본 불일치·MotionMatch 검색 생략·FootstepEffectTagModifier의 LyraGame import 경고는 사용 경로와 지속 여부를 확인한다. 장비 크래시 원인으로 단정하거나 애니메이션/발소리 에셋을 삭제하지 않는다.
- GASP_E에서 `DDCvar.DrawCharacterDebugShapes`의 FindConsoleObject 호출500회 성능 경고도 관찰했다. 실제 호출 그래프와 비용 확인 전 제거/캐싱이나 Debug 플러그인 삭제를 하지 않는다. Level_1 NavMesh origin/tile 경고와 종료 중 CrowdManager 경고는 발생 시점에 맞춰 별도 검토한다.
- Mover/ChaosMover/샘플/LiveLink 등의 삭제 가능성은 아직 미확정이다. 현재 Tick/레이더/HUD 구현도 이미 조건부 갱신이 있어 단순히 이름·파일 수·Tick 가능 여부만으로 제거하지 않는다.
- 향후 기능용 애니메이션과 현재 미커밋 GASP 확장 의존성을 보존한다. 현 시점의 확정 삭제 목록은0개다.
