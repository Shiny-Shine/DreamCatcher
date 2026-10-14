# main 병합 준비 — R14 완료 후

## 최신 — 잔여 플레이어 애니메이션·입력·블록아웃 정리 완료 / AGENTS 0.113

사용자 승인 후 옛 플레이어 AnimBP/레이어/롤5개, 옛 입력·Touch9개, Test_Map 블록아웃/전용 generated 메시9개를 추가 삭제했다. 최신 외부 참조0과 복구 근거를 확인했고22:49 새 프로세스에서 끊어진 참조0/Error·ensure0으로 검증했다. Level_1과 실사용 에셋·Source·Config·DLL은 그대로이며 기존 사용자 수용을 유지한다. 현재 입력 Action5개, 디자이너 Asset 맵과 연결 메시, 보스/실맵에서 사용하는 Mannequins 자료 및 회귀 검사는 보존한다. 추가 빌드·PIE 반복은 필요 없고 패키징은 계속 보류다. [정확한 정리 범위와 인계](level1-production-cleanup-2026-10-14.md)를 따른다. 프로젝트 전체 미사용 라이브러리가 0이라는 판정은 아니다.

---

## 병합 완료 / Level_1 전환 적용

사용자가 리뷰 수정본 PIE 정상 동작을 확인하고 `bc453e6`으로20파일을 커밋/푸시했다. 검증본과 커밋의 내용 일치를 확인한 뒤 사용자가 PR #1을 **2026-10-14 18:13 KST**에 병합했고, 로컬/원격 main은 **`3665f3ba690bab286784da7aae183b7485932cd0`**다. 아래 PR 갱신/병합 대기는 과거 이력이다. GAS-System도 main에 모두 포함됐음을 확인했으며 Codex가 브랜치를 삭제하거나 병합한 것은 아니다.

후속 조사와 사용자 승인 후 **디자이너 Level_1 환경을 보존하면서 R13 흐름·Dynamic NavMesh 연결**을 적용했다. 돌아가기는 사용자 결정대로 같은 Level_1 새 실행이며 추후 메인화면으로 바꾼다. 새 GameMode BP/기존 맵/Config2개를 변경했고19:49 최종 PIE 이동·전투·복귀 검증이 통과했다. C++/DLL 변경·빌드·패키징은 없고, 현재는 사용자 Level_1 PIE 확인과 새 작업 브랜치 커밋 대기다. 기존 테스트/레거시 제거는 후속이다. 상세는 [Level_1 인계](r14-reference-audit-2026-10-14.md) 최상단과 AGENTS0.109를 따른다.

## 통합 PR 검증 이력 — 자동검증 완료·당시 사용자 커밋/푸시 대기

2026-10-14 사용자가 GitHub Desktop에서 checkpoint → `integrate-main-20261014` 생성 → main 병합 → 옛 Box_1 보존 → publish를 직접 마쳤다. PR은 [#1](https://github.com/Shiny-Shine/DreamCatcher/pull/1), base=`main`, head=`integrate-main-20261014`다. 아래 최초 준비 기록의 ‘미실행’은 이력이며 현재 상태가 아니다.

- 고정 main: `ebb169cdf2952a84644821778dc04a4fb3515f86`.
- 사용자 checkpoint: `82bfcfcd27ef93b1c90de05a40c183b4f2fa8d5e`.
- 사용자 통합/Box 보존: `3e8f1a5`, `8b1a76c`.
- 사용자 폐기 배치 정리 커밋/원격 head: `5dfb0e61cca5eec81c523a667a36a8892a9a2b85`.
- main 최종 병합·패키징은 아직 하지 않았다. 패키징은 사용자 지시로 보류한다.

통합 자동검사는 디자이너 LFS951개 및 옛 Box13개 해시, 변경 BP8개 Compile, Box26개 로드, 맵9개 로드, R13 실제 PIE 발사·조준·사망/장비 정리까지 통과했다. PIE 경고15(Manny14＋RecastNavMesh 초기화1)는 기존 경계로 기록한다. `Building_Samples`의 `NewMap1`과 `Level_1`의 `RoadLine_IS`는 원격 main에도 없었고 사용자가 폐기 배치 제거를 승인했다. 정확한 LevelInstance 각1개 제거 후 새 프로세스로 누락 참조0/나머지 배치·유효 BPP_NewMap 보존을 확인했다. 근거 `Saved/Diagnostics/MergeValidation/integration_summary_A.json`; 완료한 이식·전체 Blueprint/맵 검사를 재실행하지 않는다.

### 승인된 PR 리뷰 수정

리뷰 [입력 매핑](https://github.com/Shiny-Shine/DreamCatcher/pull/1#discussion_r4267782549), [탄환 결과](https://github.com/Shiny-Shine/DreamCatcher/pull/1#discussion_r4267782558)를 확인했고 사용자가 수정 진행을 승인했다.

1. `DCHeroComponent.cpp`: 원본의 `bRegisterWithSettings` 조건 안에 있던 `AddMappingContext`와 옵션 설정을 조건 밖으로 이동한다. 설정 등록 여부가 실제 입력 활성화까지 막지 않도록 한다. 현재 검증된 플레이어 매핑5개는 모두 true이므로 기존 경로는 동일하다.
2. `DCLyraGameplayAbility_RangedWeapon.cpp`: 탄환 시작 시 `NumHitsBeforeBullet`을 기록하고 그 탄환의 결과가 추가되지 않았을 때만 fallback 결과를 보충한다. 앞 탄환이 이미 결과를 추가했거나 앞 탄환도 빗나갔더라도 각 탄환의 끝점이 남는다. 명중 결과 Append·중복 제거·피해·퍼짐은 변경하지 않는다. 현재 라이플의 `BulletsPerCartridge=1`은 유지한다.

로컬 Lyra 원본 `Source/LyraGame/Character/LyraHeroComponent.cpp:256` 및 `Weapons/LyraGameplayAbility_RangedWeapon.cpp:426`에도 기존 조건문이 존재한다. 이번 변경은 사용자 승인한 두 예외조건의 원본 대비 보강이다.

회귀 검사는 새 `Source/DreamCatcher/Tests/DCMergeReviewTestTypes.h`/`DCMergeReviewAutomationTests.cpp`의 임시 객체만 사용한다. 입력 검사는 실제 Hero 초기화와 Enhanced Input subsystem의 활성/등록/우선순위를 확인한다. 무기 검사는 실제 trace 경로와 임시 Box 충돌을 사용하며 기존 virtual trace-channel 훅으로 탄환별 명중/빗나감을 결정한다. 실제 게임 코드에 테스트용 trace 분기를 추가하지 않는다. 저장된 Blueprint·맵·설정은 변경하지 않는다.

### 16:44 사용자 빌드 및 16:45~16:47 검사 결과

- 첫 빌드 C2146은 검사 CPP의 `ON_SCOPE_EXIT` 뒤 세미콜론 누락이었으며 교정했다. 후속 사용자 빌드는 **16:43:51 시작·9.36초 성공**, DLL **16:44:00,6,636,544bytes,SHA256 `57733499785ce28d6ab9c4f0766cdf8f09b088d94f8ad6edde739d3d47872076`**다. BuildId55116800. 기존 DataRegistry 의존 선언 경고는 남기며 Config를 바꾸지 않았다. 사용자 빌드의 플러그인 DLL 변경은 보존한다.
- `Saved/Automation/MergeReview_A/index.json`: **성공1/실패1**. `PerBulletMissResults`는 단발/전부 명중/전부 빗나감/혼합6조합 모두 성공, 경고0. `InputMappingRegistration`은 테스트 LocalPlayer를 GameInstance 아래 생성해 엔진 `UCLASS(Within=Engine)` 규칙 위반 ensure가 발생했다. 게임 입력 코드의 assertion 실패로 해석하지 않되, 검사 전체는 실패로 유지한다.
- 테스트 CPP의 생성 Outer만 **`GEngine`으로 교정하고 `Engine/Engine.h`를 추가**했다. 원인 근거는 엔진 `Engine/Classes/Engine/LocalPlayer.h:168`. 게임 코드2파일/에셋/Config 수정은 추가로 없다. 이 교정본은 사용자 재빌드/실행 대기다.
- 현재 DLL의 R13 실제 PIE 영향 회귀는 **16:47 성공/프로세스0/Error·ensure0**. 장착/발사30→29/견착/Hip 복귀/사망·Pawn·장비 정리/실패 결과를 확인했다. 기존 레시피 `r14_cleanup_pie_probe.py -DCR14Probe=Review_A` 재사용, 에셋 저장0이다. 보고서 경고16은 Manny PoseAsset14＋RecastNavMesh1＋긴 프레임 이동 시뮬레이션 제한1이며 물리 입력/시각 수용 검증은 아니다.
- 근거 `Saved/Diagnostics/MergeValidation/pr1_review_checks_A.json`, `Saved/Automation/MergeReviewPIE_A/index.json`, `Saved/Diagnostics/R14/cleanup_pie_Review_A.json`. 보호 파일693개/에셋3788개 비교에서 의도한 테스트 CPP 교정 외 소스·Config·에셋·사용자 저장 설정·DLL·Git index 변화0이다.

### 16:50~16:51 입력 검사 교정본 검증 완료

- 사용자 증분 빌드 **16:50:02 시작·8.97초 성공**. DLL **16:50:10,6,636,544bytes,SHA256 `d5b30577e3aa9c5fc585a8a30abf96b87f979824fdff2ea835485a39f4aea099`**. 교정된 테스트 CPP가 실제 Compile/Link됐음을 UBT 로그에서 확인했다. 현재 증분 빌드로 DLL/PDB와 Git index의 파일 상태 정보도 갱신됐으며 staged 변경은 없다.
- `/Engine/Maps/Entry`를 명시한 별도 unattended Editor에서 **입력 검사1건만 실행해16:51:23 성공/보고서 경고0/실패0, Error·ensure·Fatal0, 종료0**. 근거 `Saved/Automation/MergeReviewInput_B/index.json`, `Saved/Logs/MergeReviewInput-B.log`. 시작 시 기존 B_LyraGameInstance의 `/Script/LyraGame` 경고1은 검사 경고와 구분한다.
- 탄환 검사와 R13 `Review_A` PIE는 관련 코드 불변을 확인하여 이전 성공 결과를 재사용했고 반복하지 않았다. 에셋/Config/저장 설정과 테스트 코드 외 소스의 이전 보호 기록도 유지됐다. 종합 `Saved/Diagnostics/MergeValidation/pr1_review_closeout_B.json`을 따른다.

### 다음 실행 — 사용자 짧은 확인 후 기존 PR 갱신

- **추가 C++ 수정·재빌드 요청은 없다.** 리뷰2건의 자동검증이 완료됐다. 사용자는 R13에서 이동·조준·발사 및 통합 맵 외형을 짧게 확인한다. 이미 확인한 전체 게임 진행·보스·메뉴를 처음부터 반복할 필요는 없다.
- GitHub Desktop의 **`integrate-main-20261014`**에서 코드·회귀 검사·최신 문서·사용자 빌드 DLL을 함께 커밋하고 **Push origin**한다. 권장 메시지: `fix: activate input mappings and preserve per-bullet miss results`. 수정 전 원격 head는5dfb0e6이며 기존 PR #1이 갱신된다. 새 PR은 필요 없다. 현재 변경 목록은 `Saved/Diagnostics/MergeValidation/pr1_commit_candidates_B.json`에 정리한다.
- PR의 두 리뷰가 수정 커밋을 가리키는지 확인한 뒤 사용자가 해결 표시/병합을 진행한다. Codex는 PR 댓글·리뷰 Resolve·Git stage/commit/push/merge를 실행하지 않았다. 사용자의 패키징 보류 지시는 유지한다.
- 통합 맵의 외형·배치 확인과 PR 검토 후 사용자가 main 반영을 진행한다. 디자이너 Level_1의 기존 GameMode와 R13 테스트 맵은 유지하며 게임플레이 재배치는 별도 단계다.

---

## 범위와 현재 상태

2026-10-14 사용자가 창 크기 변경 시 UI 정상 동작을 확인했다. **패키징을 제외한 현재 GAS-System의 R14 정리·검증·사용자 수용을 완료**로 기록한다. 패키징은 사용자 지시로 보류·미검증이다. 기존 호환 계층을 전부 삭제했다는 의미는 아니다.

이번 요청은 **병합 준비**다. 실제 stage/commit/checkout/merge/push, 브랜치·worktree 생성, 에셋 참조 교체, C++ 빌드/패키징은 하지 않았다. 디자이너는 병합 대상 작업을 main에 모두 커밋·푸시했다고 사용자가 확인했다. 누구에게도 메시지를 보내지 않았다.

## 고정한 비교 기준

| 기준 | 커밋 |
|---|---|
| 현재 GAS-System / 원격 GAS-System | `7a10c75b0b2af4be2f15f7c104c1bbfe82c4bbe9` — 10월13일03:39 KST |
| 로컬 main / 원격 main | `ebb169cdf2952a84644821778dc04a4fb3515f86` — 10월13일22:26 KST |
| 공통 조상 | `50ef6ac698897f531f418e0f41beb648954e4e41` |

GitHub 읽기 전용 compare를 양방향으로 조회하여 원격/로컬 기준 일치를 확인했다. 원격 compare의 파일 목록은300개로 제한되어 있으므로 전체 파일 분석에는 **로컬 Git tree/diff**를 사용했다. fetch가 없어도 비교 대상 커밋 객체는 이미 로컬에 있다. 실제 실행 직전에 원격 SHA가 바뀌지 않았는지 다시 확인한다.

- main 전용 커밋23개, GAS-System 전용 커밋18개.
- 공통 조상 대비 main 변경964경로: 추가891/수정60/삭제13. Source/Config/uproject 변경0.
- GAS의 저장된 변경2,211경로. 여기에 현재 미커밋 변경도 포함하여 교차 비교했다.
- **동시에 변경된 동일 경로0, 예상 대소문자/파일-디렉터리 충돌0.** 실제 merge 실행 결과나 에셋/게임플레이 호환성을 보증하는 판정은 아니다.
- 디자이너 변경 맵6개: `Instance/NewMap`, `Map/Building_Samples`, `Map/Main/Level_1`, `Level_2`, `Level_3`, `Map/Test_Map/Asset` (`Content/DreamCatcher/DreamCatcher_ENV/` 아래).

근거: `Saved/Diagnostics/MergePrep/remote_heads_A.json`, `preparation_A.json`, `designer_delta.json`, `gas_committed_delta.json`.

## 먼저 보존해야 할 미커밋 작업

최초 준비 스냅샷 기준 **추적 중 변경28개＋미추적 파일484개=512개**, staged0이다. R13/R14의 새 소스·UI/맵·GameSettings/DCStageUI 플러그인과 빌드 산출물이 아직 GAS HEAD에 없다. 따라서 현재 HEAD만 병합하면 R14 완료본이 포함되지 않는다. 이번 인계 문서 등 후속 문서 추가로 최종 개수는 달라질 수 있으며 실제 checkpoint 직전에 목록을 갱신한다.

`checkpoint_candidates.json`에는 전체 경로를 분류해 두었다. 승인 후 커밋 후보는 기존 변경과 새 R13/R14 코드·에셋·Config·필요 플러그인·Editor 바이너리·실행 레시피·문서다. `git add -A`로 무관한 파일을 포함하지 않고 고정 목록을 사용한다.

- 제외할 새 생성 캐시: `Scripts/Editor/__pycache__/r13_stage_ui_setup.cpython-311.pyc`. 이번에 삭제하지 않았다.
- 기존 추적 중 Python cache3개는 별도 정리 요청 없이 제거하지 않는다.
- 프로젝트, GameSettings, DCRifleMigrationTools의 `UnrealEditor.modules` BuildId는 모두 **55116800**이다. 현재 검증된 DLL/관련 모듈 파일을 소스와 함께 보존한다. 폴더 이름이 Binaries라는 이유로 제외하지 않는다(AGENTS17.5).
- `.gitignore`의 BlockingStarterPack 및 기타 기존 제외 정책은 그대로다. Git 병합이 무시된 공용 팩을 전송해 주는 것은 아니며, 새 작업 환경의 해당 의존은 확인이 필요하다. 이번 작업에서 강제 추가/무시 규칙 변경은 하지 않았다.

## Git 충돌0과 별개인 Box_1 참조 문제

main은 아래13개를 `Prop/Box_1/...`에서 **`Prop/Mesh/Box_1/...`의 같은 이름**으로 재배치했다. Git에서는 옛13경로 삭제와 새 경로 추가로 보인다. 새 파일의 내부 데이터·시각 동등성을 확인한 것은 아니다.

| 옛 폴더 | 에셋 이름 |
|---|---|
| `Content/DreamCatcher/DreamCatcher_ENV/Prop/Box_1/Material/` | MI_BOX, MI_Decal, M_Box_1, M_Box_Decal, T_DecalSet_A, T_DecalSet_B, T_DecalSet_N, T_Trim_A, T_Trim_B, T_Trim_M, T_Trim_N |
| `Content/DreamCatcher/DreamCatcher_ENV/Prop/Box_1/StaticMesh/` | SM_BOX_001, SM_BOX_Decallmesh_001 |

앞선 저장된 R14 참조 근거에서 R13 의존 집합에 남아 있는 것은5개다: **MI_BOX, MI_Decal, T_DecalSet_B, SM_BOX_001, SM_BOX_Decallmesh_001**. 특히 MI_BOX와 두 메시3개는 R11/R12/R13 맵에 직접 참조된다. 그 외8개를 미사용 확정한 것은 아니며 여기서는 Box_1 묶음 보존 범위를 제안한다. 상세 참조와 새 경로는 `designer_deletion_dependencies_A.json`에 있다. 이 참조 근거 이후 R14 정리는 Box_1/맵을 변경하지 않았다.

**권장 병합 처리:** 디자이너의 새 `Prop/Mesh/Box_1`은 그대로 반영하고, 기존 테스트/검증 맵이 쓰는 옛 Box_1 묶음13개는 GAS checkpoint의 정확한 버전으로 통합 브랜치에 보존한다. main의 삭제13건을 무조건 수용하지 않는다. main 브랜치 자체나 디자이너 맵을 이 준비 단계에서 수정하지 않는다.

대안은 기존 맵/메시/재질 참조를 새 경로로 Editor에서 교체하는 것이다. 이는 더 넓은 에셋 저장과 데이터/시각 비교가 필요하므로 이번 첫 병합안에 넣지 않는다. 바이너리 충돌에 ours/theirs를 일괄 적용하거나 uasset을 텍스트 치환하지 않는다. 통합 후 새 경로 동등성을 확인하고 옛 경로를 없애려면 별도 범위로 제시한다.

## LFS 준비 상태

main에서 추가/수정된 LFS 포인터는 **951개**, 해당 로컬 payload는 조사 시점 모두 없었다. 필요한 고유 데이터는 **635,035,336 bytes(약606MiB)**다. 포인터 객체만으로는 실제 에셋을 Editor에서 검증할 수 없다.

원격 main 커밋을 고정한 LFS fetch가 실제 병합 단계에 필요하다. 이번에는 포인터/로컬 파일 존재·크기만 읽었고 **다운로드하지 않았다**. local cache는 `E:/Epic Games/UEProjects/DreamCatcher/.git/lfs/objects`다. sandbox의 최초 LFS 경로 조회는 접근 거부였고, 권한 있는 읽기 전용 조회로 실제 경로를 확인했다. LFS fetch와 checkout은 raw uasset 편집이나 임의 바이너리 복사와 구분한다.

## 제안하는 실행 순서 — 아직 실행 승인 전

1. 현재 validated R13/R14 작업과 완료 문서를 정확한 목록으로 **GAS-System checkpoint 커밋**에 저장한다. 새 Python cache1개는 제외하고 기존 사용자 변경은 보존한다. 예상 커밋명: `chore: checkpoint validated R13 UI and R14 cleanup`.
2. checkpoint에서 **`codex/integrate-main-20261014` 통합 브랜치**를 만든다. main에 직접 커밋하거나 디자이너 branch를 먼저 움직이지 않는다. 실제 작업 디렉터리/필요한 격리 방식은 실행 전에 현재 Editor·worktree 상태를 확인해 선택한다.
3. 고정 main `ebb169c`의 LFS 데이터를 가져온 뒤 통합 브랜치에서 main을 병합한다. 완료 커밋 전에 위 Box_1 옛13자산을 checkpoint 기준으로 보존하여 기존 R11/R12/R13 참조를 지킨다. Source/Config 변경 충돌0이라는 사전 결과와 달라지면 원인을 확인하며 자동으로 범위를 확대하지 않는다.
4. 단계적 확인: 누락 LFS/깨진 참조/실제 충돌 여부→새 디자이너 맵과 기존 R13·공용 자산 로드/필요 Blueprint Compile→병합 영향이 있는 맵의 사용자 시각 확인. 이번 main은 C++ 변경0이므로 소스 변경이 추가로 생기지 않으면 불필요한 재빌드를 요구하지 않는다. 필요해진 C++ 빌드는 사용자 담당이다.
5. 통합 결과를 검토한 뒤 최종 main 반영 방법/방향과 커밋·푸시/PR을 확정한다. **이번 준비 또는 향후 통합 브랜치 생성 승인을 main push/강제 push 승인으로 해석하지 않는다.**

Git 통합만으로 디자이너 최신 Level_1에 R13 Experience/스포너·보스·HUD 흐름을 자동 배치하지는 않는다. R13 사본은 보존되며 디자이너 맵의 게임플레이 활성 연결은 별도 적용·검증 범위다. 현재 R13 GameDefaultMap/기존 R10 ReturnMap 설정도 그대로 유지하고, 최종 플레이 맵 확정 때 변경안을 제시한다. 패키징은 계속 보류한다.

## 다음 승인 범위

위 순서의 **checkpoint 커밋＋별도 통합 브랜치＋고정 main/LFS 병합＋Box_1 옛13자산 보존 및 관련 검증**을 한 묶음으로 제안한다. 실제 Source/Config/에셋 추가 수정, 디자이너 원본 맵 참조 교체, main 반영·push/PR, 패키징은 여기서 포괄 승인받는 범위가 아니다. AGENTS0.4의 명시적 브랜치 병합 범위와17.1의 main 직접 커밋 금지 원칙을 따른다.
