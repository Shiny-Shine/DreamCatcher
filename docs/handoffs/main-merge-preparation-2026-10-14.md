# main 병합 준비 — R14 완료 후

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
