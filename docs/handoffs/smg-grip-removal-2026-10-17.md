# 자체 SMG 그립 편집 기능 제거 — 새 DLL·PIE 검증 완료

2026-10-17 KST. 사용자가0.128의 자체 SMG 미리보기/그립 편집 기능이 필요 없다고 명시하고 관련 기능 제거를 요청했다. 에디터 저장·종료 확인 후 작업했다. **에셋·소스 제거, 사용자 Development Editor / Win64 빌드, 새 DLL의 클래스 부재·기존 AnimBP 재로드와 실제 Level_1 PIE 검증까지 완료했다.** Codex는 빌드·패키징·Git 스테이징/커밋/푸시를 실행하지 않았다. 사용자 물리 입력·미술적 수용은 자동 검사와 구분한다.

## 제거한 범위

전용 에셋4개를 Unreal Editor API로 삭제했다.

- `/Game/Developers/SMGGrip/L_HaruSMGGripPreview`
- `/Game/Developers/SMGGrip/BP_HaruSMGGripPreview`
- `/Game/DreamCatcher/Animations/Haru/Grip/ABP_HaruSMGGripPreview`
- `/Game/DreamCatcher/Animations/Haru/Grip/DA_HaruSMGGrip`

기존 에셋3개는 해당 기능의 연결만 해제해 저장했다.

- `/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ABP_ItemAnimLayersBase`: native 부모를 `UDCSMGGripAnimInstance`에서 원래 `UAnimInstance`로 복원. `FullBody_SkeletalControls`의 추가 Modify Bone1개/Getter2개를 제거하고 원래 오른손 IK 출력→왼손 IK 입력으로 연결했다.
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers`
- 같은 폴더 `ABP_RifleAnimLayers_Feminine`: 공유 그립 설정 참조를 제거하고 원래 부모의 동작을 사용한다.

소스7파일과 최초 생성용 스크립트를 삭제했다.

- `Source/DreamCatcher/Animation/Grip/DCSMGGripSettings.h`
- 같은 폴더 `DCSMGGripAnimInstance.h/.cpp`
- 같은 폴더 `DCSMGGripPreviewActor.h/.cpp`
- `Source/DreamCatcher/Editor/DCSMGGripEditorSetup.cpp`
- `Source/DreamCatcher/Tests/DCSMGGripAutomationTests.cpp`
- `Scripts/Editor/smg_grip_editor_setup.py`

Build.cs에서는 이 도구를 위해 추가했던 에디터 전용 `AnimGraph`, `AnimGraphRuntime` 의존성2개만 제거했다. 기존 다른 에디터 보조 코드와 의존성은 유지한다. 저장소에 남은 과거0.128 문서와 Saved 진단은 이력이며 활성 기능이 아니다.

## 유지한 상태와 동작 차이

- 기존 Unreal 스켈레탈 메시 에디터의 소켓/Preview Asset 기능과 `weapon_r_preview`는 유지한다. 사용자가 저장한 최신 소켓·무기 편집을 이전 해시로 덮어쓰지 않았다.
- HARU 본·가중치·보정 리깅, 실제 B_Rifle/SMGVisual, WID_Rifle 부착, 총구 드라이버, 기존 몽타주, Level_1과 Config는 유지한다.0.126/0.127 리깅·가중치 작업은 제거 대상이 아니다.
- 기존 공용 왼손 Two Bone IK에 직접 저장돼 있던 Effector Location **(5,-0.5,-6)**, Alpha·재장전 커브·본 설정을 보존한다.
- 자체 DA에 사용자가 적용했던 **위치(-4,-2.5,0), 회전(Pitch0,Yaw-3.5,Roll5)**은 삭제 전 기록했다. **이 추가 보정은 제거되므로 기존 공용 IK의 자세로 돌아간다.** 요청하지 않은 다른 보정 방식으로 값을 옮겨 넣지 않았다.
- Apply Grip to Game Settings, Reload Saved Grip, 전용 조정 맵 및 에디터 Tick은 제거된다. 이후 무기 위치 조정은 원래 스켈레탈 메시 소켓 미리보기와 실제 무기 에셋을 사용한다. 소켓 미리보기 값이 자동으로 실제 무기에 동기화되는 기능을 새로 추가하지 않았다.

## 보존과 검증

근거 폴더는 `Saved/Diagnostics/SMGGripRemoval20261017/`다.

- `baseline_A.json`: 제거 전7개 에셋을 정확한 Git LFS 객체로 보존했다. 각 파일의 SHA256/크기/복구 객체 경로, 보호 에셋12개, Content/GameFeature 파일 상태, Git index/HEAD, 소스 해시를 기록했다. 코드·Build.cs·최초 스크립트9개는 같은 폴더의 `*.before.txt`에 보존했다. 바이너리 파일을 일반 파일 I/O로 복사·변조하지 않았다.
- `memory_B5.json`: 저장0 상태에서 제거 검사 성공. 전체 그래프의 노드·핀 값/연결, FullBody_SkeletalControls의 기존 애니메이션 노드 구조체, CDO의 기존 설정을 비교했다. 의도한3노드 제거/1연결 복원 외 차이0. 컴파일이 재생성하는 LinkID/SourceLinkID와 일부 생성 속성 번호·출력 순서만 정규화했다.
- `save_C.json`: 위 기존 AnimBP3개만 저장. 보호 에셋/예상 밖 파일 변경0, Error/ensure0.
- `delete_D.json`: 별도 프로세스에서 저장본을 재로드·컴파일해 전용 클래스/설정 참조가 사라진 것을 확인했다. 외부 참조0인 전용4개만 `EditorAssetSubsystem.delete_loaded_assets`로 삭제했다. 보호 에셋/예상 밖 파일 변경0, Error/ensure0.
- `verify_E2.json`: 삭제 후 다시 연 프로세스에서 기존 AnimBP3개 로드/컴파일, `/Game`과 `/DCStageUI` 에셋 레지스트리의 삭제 에셋/전용 부모 잔여 참조0 확인. 에셋 저장0, 보호/예상 밖 변경0, Error/ensure0. 첫 verify_E는 삭제된 패키지의 None 참조 반환을 빈 목록으로 처리하기 전 중단됐으며 저장0이었다.
- `closeout_F.json`: 변경 에셋은 기존3개/삭제4개로 정확히 한정, 보호12개/복구 LFS7개 검증, 소스7개/스크립트1개 제거와 Build.cs 변경 범위 확인. Git index/HEAD 보존·diff --check 통과, 검사 에디터 프로세스0 확인. 기존 DLL 시각은04:04:39 KST이며 사용자 재빌드 전이다.
- 사용자 재빌드는 **15:47:48 KST DLL 갱신, UBT 성공51.73초**, 크기5,879,808바이트다. `verify_G.json`에서 DCSMGGripSettings/AnimInstance/PreviewActor3개가 실제로 등록되지 않음을 확인했다. 기존 AnimBP3개 로드/컴파일 성공, 삭제 에셋·전용 부모 잔여 참조0, 보호/예상 밖 변경0.
- `pie_H.json`의 실제 Level_1 PIE에서 원래 ABP_RifleAnimLayers 실행·HARU/SMG 장착, 발사 탄약30→29, 원본 재장전 몽타주와 탄약30 복구, 원본 대시 몽타주·최대속도1850cm/s·무기 부착 유지를 확인했다. 에셋 저장0, SaveGames 불변, Error/ensure0. 검사 전용 오프스크린 애니메이션 Tick 옵션은 복원했다.
- `Saved/Automation/SMGGripRemoval_PIE_H/index.json`은 성공(경고 포함)1/실패0이다. 경고는 맵 NavMesh 원점 재생성, 종료 시 CrowdManager/NavMesh 조회, 검사 스크립트의 deprecated skeletal_mesh 속성 읽기다. 경고0으로 보고하지 않는다. 원본 PoseAsset의 기존 로드 경고도 이번 제거 범위에서 수정하지 않았다.
- `closeout_I.json`은 새 DLL SHA256 `56e14eef6fa13691f2e388ba1bc2d3f0e7e458f7c4ad9896000f034dcb2406b5`, 전체 PIE 로그 Error/ensure0, 보호12개/복구 LFS7개·삭제4개/소스8개·Git index/HEAD 보존을 확인한다. 검사 프로세스 모두 종료. 소스8개는 C++7개＋Python 생성 스크립트1개다.
- 최초 메모리 검사B~B4는 검사 스크립트의 노드 속성 차이, 컴파일 인덱스/출력 순서 및 삭제 후 메모리에 남은 비참조 노드의 텍스트 export를 구분하는 과정에서 중단했다. 저장·삭제0이었다. B5만 최종 메모리 성공 근거다. 새 프로세스 재로드에서 실제 잔여 참조0을 따로 확인했다.

## 현재 재개 기준

제거와 새 DLL 검증은 완료됐다. 추가 빌드나 같은 자동 검사를 반복하지 않는다. 원래 스켈레탈 메시 소켓/Preview Asset 편집을 사용하며,0.128 최초 생성 스크립트를 복원·재실행하지 않는다. 사용자가 직접 확인한 화면/조작감은 별도로 기록한다. 과거 전체 GAS/맵/리깅 검증도 반복하지 않는다.

무기 소켓 값의 실제 플레이 동기화는 직전 대화에서 방법만 설명한 상태이며, 이번 제거 요청에 포함해 임의로 적용하지 않는다.
