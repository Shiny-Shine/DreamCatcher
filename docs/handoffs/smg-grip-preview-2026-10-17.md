# HARU SMG 왼손 조정 환경 — 과거 구현 기록 / 이후 제거

**현재 상태:** 사용자가 이 자체 기능이 필요 없다고 요청해0.129에서 제거하고 사용자 재빌드·새 DLL·PIE 검증을 마쳤다. 아래 경로·버튼·사용법은 과거 기록이며 현재 사용할 수 없다. 최신 결과는 [제거 인계](smg-grip-removal-2026-10-17.md)를 따른다. 이 기능을 다시 생성하거나 현재 안내로 사용하지 않는다.

2026-10-17 KST. 승인 범위인 HARU/SMG 대기·조준 자세의 왼손 위치/회전 편집 환경을 저장하고 실제 Level_1에 연결했다. 사용자 빌드 2회, 저장·재로드, 에디터 자동 갱신과 실제 PIE 회귀가 통과했다. **사용자가 직접 그립을 조정하고 화면·조작감을 수용하는 단계가 남았다.** 새 환경의 사용자 확인을 완료로 기록하지 않는다.

사용자가 공용 IK에 저장한 위치 **(5,-0.5,-6)**, 회전 **(0,0,0)**을 초기 공유 값으로 유지했다. 이번 작업은 최종 손 접촉을 미술적으로 확정한 것이 아니라 그 위치/회전을 편하게 조정하는 환경을 만든 것이다. 오른손·무기 부착, HARU 본/가중치, 재장전·대시와 Level_1은 보존했다. Codex는 C++ 빌드·패키징·Git 스테이징/커밋/푸시를 실행하지 않았다.

## 사용하는 방법

1. 콘텐츠 브라우저에서 `/Game/Developers/SMGGrip/L_HaruSMGGripPreview`를 연다. Developers 폴더가 안 보이면 **콘텐츠 브라우저 오른쪽 위 톱니바퀴 → Content 영역 → Developers Content**를 켠다. UE5.8 새 UI는 Show 접두사를 생략하며 구형 UI 이름은 Show Developers Content다(설치된 ContentBrowserMenuUtils.cpp 확인). PIE를 실행하지 않고 레벨 뷰포트에서 작업한다.
2. Outliner의 **HARU SMG - select LeftHandTarget to adjust** 액터를 선택한다. Details 상단 컴포넌트 목록에서 **LeftHandTarget**을 선택한다.
3. **W: 이동, E: 회전**으로 초록색 목표점을 조정한다. Details의 상대 Location/Rotation에 숫자를 입력해도 된다. 작은 조정이 안 되면 이동/회전 스냅을 끄거나 간격을 낮춘다. Scale은1로 고정된다.
4. 액터 자체를 다시 선택하고 **SMG Grip → Preview Aiming (ADS)**를 켜거나 꺼 대기/조준 자세를 비교한다. 같은 위치·회전 설정을 두 자세에서 사용한다. 자세마다 별도 값을 저장하는 기능은 아니다.
5. 원하는 값이면 액터의 **Apply Grip to Game Settings** 버튼을 누른 뒤 **Save All**한다. 이때 공유 `DA_HaruSMGGrip`도 저장 대상에 포함한다. 이후 Level_1에서 같은 설정을 사용한다. 같은 HARU와 SMG를 쓰는 일반 AI에도 적용된다.
6. **Reload Saved Grip**은 현재 공유 DataAsset 값으로 편집 목표점을 되돌린다. Apply 이전에는 아직 공유 값이 바뀌지 않는다. Apply 이후 되돌리려면 Ctrl+Z로 Apply를 취소한 다음 Reload를 누른다. 이 버튼이 디스크 파일 자체를 다시 읽는 것은 아니다.

미리보기는 기본 대기/ADS 고정 자세다. 실제 플레이의 이동·조준 각도·몽타주·카메라·후속 리깅에 따른 전체 모습까지 동일한 화면을 재현하는 것은 아니다. **그립 위치/회전 설정과 IK 보정 경로를 공유**한다. 최종 접촉은 Level_1의 대기/조준·재장전·대시에서 확인한다. 손가락 개별 자세는 이번 편집 대상이 아니다.

왼손 목표점을 멀리 두면 팔 길이 제한 때문에 손이 목표에 닿지 않을 수 있다. 원래 Two Bone IK의 Allow Stretching=false를 유지했다. 실제 ADS 검사에서도 팔 길이55.02cm에 목표 거리60.36cm이면5.34cm가 남았고, 목표를 도달 범위 안으로 옮기면 오차가 해소됐다. 이 경우 목표를 가까이 옮겨 맞춘다.

오른손 기준인 `weapon_r`, 실제 `B_Rifle/SMGVisual`, `WID_Rifle`은 그대로다. 이후 무기 외형/부착 Transform을 별도로 바꾸면 이 미리보기의 `WeaponRelativeTransform`도 다시 동기화해야 한다. 이번 기능이 향후 모든 무기 편집까지 자동 추적하는 것은 아니다.

## 실제 저장한 에셋

| 역할 | 경로 |
|---|---|
| 실제/미리보기 공유 설정 | `/Game/DreamCatcher/Animations/Haru/Grip/DA_HaruSMGGrip` |
| 고정 자세·원본 손 IK 기반 미리보기 | `/Game/DreamCatcher/Animations/Haru/Grip/ABP_HaruSMGGripPreview` |
| 편집용 액터 | `/Game/Developers/SMGGrip/BP_HaruSMGGripPreview` |
| 편집용 맵 | `/Game/Developers/SMGGrip/L_HaruSMGGripPreview` |
| 기존 공용 레이어의 최소 확장 | `/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ABP_ItemAnimLayersBase` |
| 공유 설정을 사용하는 라이플 레이어 | `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers` |
| 공유 설정을 사용하는 여성 라이플 레이어 | 같은 폴더의 `ABP_RifleAnimLayers_Feminine` |

신규4개＋기존3개다. 계획했던 `Backup/ABP_ItemAnimLayersBase_BeforeGrip`은 저장하지 않았다. 원본 BP의 Property Access 문맥 때문에 복제 백업의 컴파일이 실패해, **사용자 수정이 포함된 정확한 원본 파일을 Git LFS 객체로 보존**했다. 활성 에셋을 과거 R8 버전으로 되돌리지 않았다.

HARU Mesh/Skeleton/가중치·보정 리깅, SMG/WID/B_Rifle/총구 드라이버, Level_1, 기존 애니메이션·몽타주, Config/uproject는 이번 범위에서 바꾸지 않았다. Git에 표시되는 이 파일들의 앞선0.124~0.127 변경은 유지한다.

## 원본 대비 연결과 코드

원래 `FullBody_SkeletalControls`의 Hand IK Retargeting → 가상 본 `VB IK_Hand_L_weaponSpace`를 `ik_hand_l`에 복사 → 오른손/왼손 Two Bone IK를 유지한다. 왼손 `hand_l`/목표 `ik_hand_l`, Bone Space, Take Rotation From Effector Space=true, 기존 `HandIK_Left_Alpha`와 Effector Location(5,-0.5,-6)은 그대로다.

공용 레이어의 native 부모만 `UAnimInstance` 하위인 `UDCSMGGripAnimInstance`로 확장하고, 왼손 IK 앞에 엔진 Transform(Modify) Bone1개＋Getter2개를 추가했다. 공용 부모의 설정은 비어 있고 라이플 레이어2개만 공유 DA를 지정한다. 실제 HARU 메시와 부착된 보이는 SMG가 맞을 때만 보정한다.

기존 Effector를 E, 공유 위치/회전을 L/R이라고 하면 새 목표 보정은 `Translation = L - R.RotateVector(E)`, `Rotation = R`이다. 기존 IK가 이후 E를 더하므로 초기 L=E/R=0에서는 항등 보정이다. 미리보기는 평가된 `ik_hand_l`에서 이 보정의 역변환으로 기준을 복원해 목표점의 자기 누적을 막는다.

재장전/대시는 기존 `Event.Movement.Reload`/`Event.Movement.Dash` 태그와 지정 몽타주 인스턴스를 확인해 새 보정만0으로 한다. `Montage_IsActive`가 블렌드 아웃 시작 후 false가 되는 엔진 동작을 확인했으므로, 종료 전 `MontageInstances` 목록까지 확인한다. 기존 DisableLHandIK 커브와 동작은 유지한다.

- `Source/DreamCatcher/Animation/Grip/DCSMGGripSettings.h`: 공유 설정.
- 같은 폴더 `DCSMGGripAnimInstance.h/.cpp`: 게임 스레드 스냅샷·범위·몽타주 보호.
- 같은 폴더 `DCSMGGripPreviewActor.h/.cpp`: 에디터 전용 액터·초록 목표점·Apply/Reload. Apply는 FScopedTransaction과 dirty 표시를 사용하고 디스크 저장은 Save All에 맡긴다.
- `Source/DreamCatcher/Editor/DCSMGGripEditorSetup.cpp`: unattended＋`-DCSMGGripSetup`에서 정확한 대상만 구성하는 `DC.SMGGrip.PrepareGraphs`. 자체 저장 없음.
- `Source/DreamCatcher/Tests/DCSMGGripAutomationTests.cpp`: 오프셋·회전·미리보기 기준 복원 검사1건.
- `Source/DreamCatcher/DreamCatcher.Build.cs`: 에디터 대상에만 AnimGraph/AnimGraphRuntime 의존성 추가.

에디터의 일반 Actor Tick은 활성화돼 있어도 해당 뷰포트에서 실행되지 않는 경우를 실제 재현했다. 두 번째 빌드에서는 PreviewActor.cpp에 `FTickableEditorObject`를 추가해 Editor 월드의 이 액터만 고정 포즈를 갱신한다. 메시의 자체 에디터 Tick은 꺼 중복 평가를 막고, 컴파일 중인 메시/종료 중인 월드는 건너뛴다. 실제 게임용 그립 코드는 이 두 번째 수정에서 바꾸지 않았다.

## 검증 근거

진단 폴더는 `Saved/Diagnostics/SMGGripEditor20261017/`, 로그는 `Saved/Logs/SMGGrip-*.log`다.

| 검사 | 결과와 근거 |
|---|---|
| 첫 사용자 빌드 | 03:04:26 KST DLL 갱신, UBT 성공71.89초 |
| C++ 계산 검사 | `Saved/Automation/SMGGrip_C/index.json`, 성공1/실패0/경고0 |
| 메모리 준비 | `memory_J.json` 성공. 기존 그래프·오른손/무기 보존, 저장0 |
| 저장 | `save_O.json` 성공. 위7개만 저장, 대기/ADS에서 도달 가능한 목표의 위치·회전 일치 |
| 실제 Level_1 PIE | `pie_X.json` complete=true. 초기 기존 경로 대비 본 이동 최대0.000061cm. 공유값 변경 시 왼손0.7586cm 이동, 보호 본/무기 차이 최대0.000039cm. HARU/보이는 SMG 조건 확인 |
| 실제 재장전·대시 | 같은 PIE X에서 재장전230표본/블렌드 아웃33표본, 대시71표본/블렌드 아웃25표본 추가 보정0. 원래 설정·임시 Tick 옵션 복원, 에셋 저장0, SaveGames 불변, Error/ensure0 |
| 두 번째 사용자 빌드 | 04:04:39 KST DLL 갱신, UBT 성공6.05초. PreviewActor.cpp의 에디터 자동 갱신만 수정 |
| 새 DLL 에디터 검사 | `preview_Y.json` complete=true. 명시적 Refresh 없이 왼손1cm 이동·회전 일치, 오른손/무기 유지, Apply/Undo/Redo/Reload, 대기/ADS 전환. Error/ensure0, 에셋 저장0 |
| 화면 정리 저장 | `polish_Z1.json` 성공. 새 조정 맵만 저장해 조명 아이콘을 캐릭터 밖으로 옮기고 가까운 초기 시점 설정 |
| 최종 저장본 재로드 | `final_AA.json` complete=true. 저장된 조명·시점·초기 공유 값, 미적용 편집값의 ADS/대기 왕복 보존 확인. Error/ensure0, 에셋 저장0 |
| 마감 보존 확인 | `closeout_AB.json` complete=true. 보호 에셋12개·최종7개 해시·원본 공용 BP LFS·Git index/HEAD 보존, Python AST와 git diff --check 통과. 검사 프로세스 모두 종료 |

`preview_Y_hipfire.png`와 `preview_Y_ads.png`에서 실제 HARU·SMG·초록 목표점을 확인했다. 사용자 물리 입력/미술적 수용, 전체 애니메이션·온라인·패키징 검증과 구분한다. 두 번째 빌드 뒤 완료된 게임 경로 검사X를 불필요하게 반복하지 않았다.

검사 과정에서 고친 것은 복제 백업의 PropertyAccess 제약, 복사된 AnimNode 구조체와 별개인 노출 핀 기본값, Python reflected struct의 실시간 참조, 임시 PIE 속성 변경 알림에 의한 컴포넌트 재구성, 에디터 일반 Tick 미실행이다. 실패한 실행을 성공으로 합산하지 않는다. 최종 성공 근거는 위 표다. 화면 정리 첫 실행Z는 명령 인자 인용 오류로 스크립트 시작 전 중단됐고 저장 없이 해당 검사 프로세스만 종료했다.

## 보존·복구와 재개

- 작업 전 보호 해시15개와 Git index 항목/HEAD는 `baseline_B.json`에 있다. 그중 승인된 공용/라이플 레이어3개 외12개는 유지한다.
- 사용자 수정 공용 BP 원본은 SHA256 `10b9acef067965b57f4a0151062c5052f31872a0cca22d095bea9195f8b35a40`, 크기2,808,731바이트다. `.git/lfs/objects/10/b9/10b9acef067965b57f4a0151062c5052f31872a0cca22d095bea9195f8b35a40`에 보존했고 `original_lfs_G.json`에서 검증했다. 스테이징이나 기존 파일 교체 없이 git lfs clean으로 정확한 원본을 보존한 것이다.
- 최종7개 에셋 해시는 `polish_Z1.json`의 `saved_hashes`다. 원본 BP 파일 보존 근거와 최종 수정본을 혼동하지 않는다.
- `Scripts/Editor/smg_grip_editor_setup.py`는 최초 생성용이다. 사전 해시와 신규 경로 부재를 요구하므로 **현재 저장 완료 상태에서 재실행하지 않는다**. 완료된 리깅·가중치 전이·GAS/맵 통합도 반복하지 않는다.
- 공유 값을 초기 상태로 돌리려면 DA의 위치(5,-0.5,-6), 회전0으로 설정하고 미리보기 Reload를 누른다. 기능 자체를 제거하려면 승인된3개 레이어/새 코드/설정 의존성을 함께 검토한다. 보존 BP만 무작정 덮어써 다른 새 편집을 지우지 않는다.
- 다음은 사용자 조정 맵 사용과 Level_1의 손 접촉·대기/조준·재장전/대시 확인이다. 추가 C++ 빌드는 필요 없다. 근육 가중치의 미술적 수용은0.127 문서의 별도 범위다.
