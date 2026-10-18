# HARU Lyra 라이플 부착과 무기 본 복원

**전체 본 확장 후속 0.126:** 기존119개 본·무기/SMG 부착과 가상 본3개를 유지한 채 Lyra 누락89개를 추가했다. 현재 실제208개＋가상3개이며 HARU 전용 Post Process/비틀림 연결이 적용됐다. 아래122개 본 수와 테스트 코드는 이전 단계 기록이다. [최신 리깅 인계](haru-lyra-full-rig-2026-10-16.md)를 따른다.

**대시 오류 후속 0.124:** HARU의 본 구조는 유지하고 FastFeet_Haru 프로필과 대시 전용 사본/조건부 선택을 추가했다. [첫 대시 ensure 수정 인계](haru-dash-blend-fix-2026-10-16.md)를 참고한다. 아래0.122의 사격/이동 검사가 대시 프로필 호환까지 검증한 것은 아니다.

**외형 후속 0.123:** 무기 본·가상 본은 이 문서 상태를 유지한다. 보이는 무기와 weapon_r_preview의 모델/Transform은 이후 SM_GUN_SMG로 교체됐으므로 [SMG 연결 인계](smg-visual-2026-10-16.md)가 최신이다. 아래 SK_Rifle 미리보기 기록은 무기 본 복원 당시의 상태다.

## 최신 상태 0.122

사용자가 실제 무기 위치가 크게 어긋난다고 확인해 기준 자세뿐 아니라 **애니메이션되는 무기 본·양손 IK·무기와 미리보기 연결 전체**를 다시 비교했다. 아래 0.121의 고정 소켓 교정은 최종 해결이 아니며 다시 적용하지 않는다.

### 원인과 적용

- Lyra 원본 `weapon_r`은 `hand_r` 자식 본으로, 라이플 애니메이션에 별도 이동·회전 트랙이 있다. 대기 모션의 손 기준 위치는 약(-7.62,3.20,-0.95)cm, 회전은 Yaw 약175°다. 레퍼런스 포즈의(-1.16,1.32,-0.52)cm/−8.52°를 고정 소켓에 넣어서는 이를 재현할 수 없다. 앞선 조사에서 이 차이를 놓쳤다.
- HARU에는 이 본과 원본의 가상 본3개가 없었고, 손 본의 기준 축도 Manny와 달랐다. 기존 `weapon_r` 고정 소켓을 제거하고 `SKM_HARU`에 실제 무기 본을 추가했다. **HARU 손 축 차이를 반영해 무기 본의 기준 회전만 대응**시켜, 원본 애니메이션의 무기 방향을 재현한다. 기존118개 본의 위치·회전·계층은 유지한다.
- HARU Skeleton에 원본 그대로 `VB IK_Hand_R_chestSpace`(spine_05→hand_r), `VB IK_Hand_L_chestSpace`(spine_05→hand_l), `VB IK_Hand_L_weaponSpace`(weapon_r→hand_l)를 추가했다. 원본 `weapon_r_muzzle` 소켓도 복원했다. 이 소켓과 미리보기 소켓의 Force Always Animated가 켜져 있어 스킨 웨이트가 없는 무기 본도 평가된다.
- WID_Rifle의 AttachSocket=weapon_r, AttachTransform 위치0/Yaw−90°/크기1과 실제 B_Rifle의 SK_Rifle·ABP_Weap_Rifle·메시 상대 Transform은 이미 원본과 같아 유지했다. Pawn·맵·장비 C++·공용 AnimBP·원본 애니메이션은 수정하지 않았다.
- 기존 에디터 미리보기에는 실제 플레이 무기와 다른 `SM_GUN_SMG`가 붙어 있었다. 해당 미리보기 연결만 실제 `SK_Rifle`로 바꿨다. 추가한 `weapon_r_preview` 소켓은 weapon_r 본 아래에서 WID의 Yaw−90°를 적용하므로 **미리보기와 실제 장착 Transform이 일치**한다. SMG 디자인 에셋 자체는 보존했다.

### 변경 파일과 검증

- 에셋2개: `Content/DreamCatcher/Main_Charcher/Haru/SKM_HARU.uasset`, `Content/DreamCatcher/Main_Charcher/Haru/하루리깅_Skeleton.uasset`.
- `Source/DreamCatcher/Editor/DCHaruRifleRigRepair.cpp`: Python에 노출되지 않은 엔진 API로 원본 이름의 가상 본과 미리보기를 연결하는 제한된 에디터 전용 명령. 정확한 HARU 경로와 원본 구조, unattended 및 `-DCHaruRigRepair`가 모두 맞아야 실행하며 자체 저장은 하지 않는다. 게임 빌드에는 포함되지 않는다. 사용자 빌드 완료, DLL 2026-10-16 03:02:56 KST/5,857,792bytes 확인. Codex는 C++ 빌드·패키징을 실행하지 않았다.
- 저장과 새 프로세스 재로드에서 대기·ADS·여성형 대기·앉기·전진 걷기·재장전6개, Raw/Compressed2종, 시점2개 총24건씩 통과했다. 무기 방향 원본 일치, 가상 본3개 존재, 실제 B_Rifle과 미리보기 Transform 일치를 확인했다.
- GeometryScript로 읽은 정점22,044개·삼각형41,241개 전체 위치/인덱스/스킨 웨이트 해시 `7393878a5dcb41d36819ffbf8bc02ab8b0c2fdd81127d0d9410025fe9ffff4a4`가 변경 전·저장 전·재로드에서 동일하다. 재질·모프 이름 목록·Skeleton 모프 커브 메타데이터도 보존됐다. 이 값은 DynamicMesh 조회 기준이며 FBX/SourceModels의 정점 집계와 구분한다.
- 저장은 위2개 에셋뿐이며 다른 콘텐츠·코드·Config·DLL 보호 검사는 통과했다. 수정 전 사용자 메시/Skeleton은 LFS 캐시에 해시 검증해 보존했다. 스테이징·커밋·푸시는 하지 않았다.
- 실제 Level_1 PIE J에서 대기·이동8개 표본, 최대속도600cm/s의 라이플 추종 검사가 통과했다. weapon_r은 기준 자세 대비 약146~179°의 애니메이션 회전이 적용됐고 매 프레임 실제 무기와 미리보기 Transform을 비교했다. 최종 엔진 로그 Error/ensure0, 에셋과 사용자 SaveGames 불변이다. 확인 PNG에서 손에 붙은 전방 라이플을 확인했지만 후방 시점이므로 손가락·왼손 접촉의 정밀 수용은 사용자 화면 확인에 남긴다. 전체 HARU 리타깃·모든 LOD·멀티플레이 검증 완료로 확대하지 않는다.

### 디자이너 확인 위치

`SKM_HARU`의 Skeleton Tree에서 `hand_r → weapon_r`은 이제 **본**이다. `weapon_r_preview`는 실제 라이플의 −90° 부착을 반영하는 미리보기 소켓이다. 라이플 애니메이션을 재생하며 확인하고, 무기 위치를 바꾸려고 옛 고정 weapon_r 소켓을 다시 만들지 않는다. 새 모델을 FBX로 재임포트할 때도 weapon_r 본이 보존돼야 하며, 소스 FBX에 해당 본이 없으면 다시 확인이 필요하다. 이번 작업은 원본 FBX를 수정하지 않았다.

근거: `Saved/Diagnostics/HaruSocketRepair/grip_audit_A.json`, `grip_bone_probe_C.json`, `grip_before_F.json`, `grip_save_G.json`, `grip_verify_G.json`, `grip_pie_J.json`, `grip_pie_J.png`, `grip_closeout_J.json`, `Saved/Logs/HaruGrip-*.log`. BoneProbe_B는 모프 조회 API 오류로 변경 전 중단, Save_F는 제거된 소켓의 미참조 객체까지 T3D에 나타나 검사에서 중단했으며 둘 다 저장0이다. 최종 G는 실제 소켓 배열을 검증했다. PIE G/H/I는 검사 스크립트 API 호출 오류로 중단했으며 성공으로 기록하지 않는다. J는 별도 카메라 없이 기존 플레이 화면에서 검사를 완료했다.

## 이전 0.121 기록 — 레퍼런스 포즈 고정 소켓의 한계

사용자가 SKM_HARU의 hand_r 아래에 weapon_r을 만들고, 방금 만든 소켓은 잘못된 것 같으니 지운 뒤 다시 만들어 달라고 요청했다. WID_Rifle의 기존 Yaw −90° 보정도 고려하도록 지시했다.

**확인과 최종 변경**

- 최초 HARU 검사에서는 weapon_r 본/소켓이 없었지만, 사용자 작업 후에는 `하루리깅_Skeleton`에 이름 weapon_r, 부모 hand_r, 위치/회전0, 크기1인 소켓1개가 있었다. 이름·부모는 이미 정상이었다. HARU 메시 자체에는 별도 Mesh Socket이 없었다.
- 원본 Manny의 weapon_r은 hand_r 자식 **본**이다. 원본 레퍼런스 포즈의 hand_r→weapon_r 변환을 가져와 HARU 소켓의 기준 오프셋으로 사용했다.
- 자동화의 삭제·재생성 시도는 에디터의 SET 명령 제한으로 **저장 전에 중단**됐다. 파일 변화0이다. 이 제한을 우회하지 않았으며, 지원되는 `SetSocketParent`/`SetSocketLocalTransform` API로 기존 소켓을 다시 설정했다. **기존 소켓 객체를 삭제·재생성한 것은 아니다.**
- 변경 파일은 `Content/DreamCatcher/Main_Charcher/Haru/하루리깅_Skeleton.uasset` **1개**다. 사용자 SKM_HARU 메시 변경, 다른 Skeleton 데이터와 기존 메뉴/맵/애니메이션 작업을 유지했다.

| 설정 | 최종 값 |
|---|---|
| Socket Name | `weapon_r` |
| Parent Bone | `hand_r` |
| Relative Location | X −1.163383, Y 1.317055, Z −0.519517 cm |
| Relative Rotation | Pitch 15.808370°, Yaw −8.517089°, Roll −11.617612° |
| Relative Scale | 1, 1, 1 |
| WID_Rifle.AttachTransform | 기존 위치0/크기1/**Yaw −90° 유지** |

소켓 회전과 WID_Rifle 회전은 함께 적용된다. 소켓에 단순히 +90°를 넣어 WID의 −90°를 상쇄하는 방식으로 처리하지 않았다. 원본 Manny의 손 기준 라이플 변환을 재현하는 것으로 확인했다.

**검증과 한계**

- 별도 프로세스 저장 후 새 프로세스 재로드 성공. 소켓 밖 Skeleton 전체 텍스트 export 해시가 동일하다. 메시와 WID_Rifle 및 다른 에셋/코드/Config/DLL, 기존 staged 항목 내용은 불변이다.
- 임시 Editor 월드에서 실제 B_Rifle 클래스와 런타임 장비 코드의 상대 Transform/KeepRelative 부착 규칙을 사용했다. 손 기준 위치·회전이 원본 기준과 일치했고, HARU 액터를 이동 `(150,100,75)` 및 Yaw35° 회전시켜도 동일한 손 기준 변환을 유지했다.
- 최종 Save_D/Verify_D의 Error/ensure/Fatal0이다. 기존 Manny PoseAsset 경고는 이번 소켓 변경과 별개다.
- 위 검사는 **PIE나 화면 확인이 아니다.** 실제 플레이에서 손을 따라오는지, 손가락/왼손의 정밀 위치가 맞는지는 사용자 화면 확인이 남아 있다. 정적 소켓을 추가·교정한 것이며 원본의 애니메이션되는 weapon_r 본이나 HARU 리타깃 구성을 새로 만든 작업은 아니다.
- C++ 빌드·패키징은 실행하지 않았고 추가 빌드가 필요 없다. 커밋·푸시는 하지 않았다.

사용자 최초 소켓을 포함한 HARU 메시/Skeleton의 작업 전 상태는 Git LFS 캐시에 해시 검증해 보존했다. 근거는 `Saved/Diagnostics/HaruSocketRepair/{inspect_B.json,before_C.json,save_D.json,verify_D.json,closeout_D.json}` 및 `Saved/Logs/HaruSocketRepair-*.log`다. Inspect_A의 텍스트 인코딩 오류와 Save_C의 미지원 명령 실패는 모두 저장0인 이력이다.

현재 Pawn의 메시가 SKM_HARU이고 Anim Class가 ABP_Mannequin_Base인 점은 조사에서 확인했다. HARU 전용 애니메이션 호환/리타깃 검증은 이 소켓 수정의 범위 밖이며 완료로 기록하지 않는다.
