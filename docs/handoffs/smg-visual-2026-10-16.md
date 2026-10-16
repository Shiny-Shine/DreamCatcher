# SMG 실제 무기 외형과 미리보기 연결

사용자는 `SM_GUN_SMG`를 **실제 플레이와 HARU 메시 에디터 미리보기 모두**에 쓰도록 승인했다. 기존 라이플의 성능·입력·GAS·캐릭터 애니메이션을 유지하면서 보이는 모델과 총구 위치를 교체했다.

## 현재 연결과 편집 위치

| 목적 | 에셋과 위치 |
|---|---|
| 디자인 원본 | `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Gun_SMG/SM_GUN_SMG` — 원본 메시·재질·피벗은 수정하지 않았다. |
| 실제 무기 외형 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/B_Rifle` → 새 `SMGVisual` StaticMeshComponent |
| 외형의 상대 Transform | 위치 `(0,-0.5,-6)`cm, 회전 `(Roll0,Pitch0,Yaw90)`°, 크기 `(0.5,0.5,0.5)`. 원본 길이 약169cm를 플레이에서 약84.6cm로 표시한다. |
| 부착 기준 | WID_Rifle의 `weapon_r`/Yaw−90° 유지. HARU의 애니메이션되는 weapon_r 본·가상 본3개는 변경하지 않았다. |
| 총구·탄피 배출 | 새 `/Game/DreamCatcher/Lyra/Weapons/Rifle/SK_Rifle_SMGSocketDriver`의 Mesh Socket `Muzzle`, `ShellEject`, `ShellEject_0` |
| HARU 미리보기 | `하루리깅_Skeleton` → `weapon_r_preview`. SMGVisual 상대 Transform과 WID 부착 Transform을 합친 값으로 실제 외형과 일치한다. |
| 픽업 표시 | `/Game/DreamCatcher/Lyra/AI/ShooterCore/Weapons/Rifle/WeaponPickupData_Rifle`의 DisplayMesh/Scale/Offset. SMG로 바꾸고 기존 표시 중심을 유지했다. |

`B_Rifle`의 기존 `SkeletalMesh` 컴포넌트는 화면에 표시하지 않는다. 이 컴포넌트의 스켈레탈 타입·AnimBP·소켓을 기존 Lyra 발사/재장전 그래프가 직접 참조하므로, 원본 동작을 유지하는 내부 드라이버로 남겼다. `Visible=False`, 주/깊이 패스·그림자·CustomDepth 비활성, `AlwaysTickPoseAndRefreshBones`를 사용한다. 보이는 `SMGVisual`은 독립적으로 표시되고 충돌·겹침·Nav 영향은 꺼져 있다. `SK_Rifle_SMGSocketDriver`는 원본 SK_Rifle 사본에 메시 전용 소켓만 추가했으며, 공용 원본 SK_Rifle과 그 Skeleton은 보존했다.

총구는 원본 SMG 정점에서 측정한 로컬 `(97.641319,0,32.743649)`를 기준으로 맞췄다. 탄피 기준점은 우측 리시버의 초기 위치 `(23,5.2,32.743649)`이며 필요하면 드라이버의 ShellEject 소켓을 조정한다. 이 초기 위치와 외형 크기는 아트 최종 수용을 대신하지 않는다.

SMG에는 스켈레톤이 없으므로 **SMG 자체의 탄창 분리·노리쇠 모션은 없다.** 캐릭터의 발사·재장전 모션과 탄약 처리, 기존 발사 이펙트·사운드는 유지한다. 아이콘·표시명·연사 속도 등 무기 성능을 SMG라는 이름에 맞춰 임의 변경하지 않았다.

외형의 위치·크기·피벗을 다시 바꾸면 총구 소켓과 `weapon_r_preview`도 함께 맞춰야 한다. 내부 드라이버를 새 무기 모델로 다시 임포트하거나 제거하지 않는다. 실제 디자인 변경은 SM_GUN_SMG 또는 B_Rifle의 SMGVisual에서 시작한다.

## 처리와 검증

- 에셋 변경은 기존3개(B_Rifle, HARU Skeleton의 미리보기, WeaponPickupData_Rifle)와 새 드라이버1개다. HARU 메시·기존 본·원본 애니메이션·WID·공용 B_Weapon·맵·Config·디자인 원본은 유지한다.
- 에디터 전용 `Source/DreamCatcher/Editor/DCHaruRifleRigRepair.cpp`에 정확한 경로와 `-DCSMGVisualSetup`/unattended 조건을 요구하는 보조 명령을 추가했다. 원본 스켈레탈 소켓 계약을 유지하고 미리보기를 동기화하는 초기 설정용이며 자체 에셋 저장은 하지 않는다. 사용자 빌드 완료(DLL2026-10-16 03:33:04 KST, 5,865,472bytes). Codex 빌드·패키징은 실행하지 않았다.
- 저장 D와 별도 프로세스 재로드 D에서 실제 SMG/미리보기/총구 일치, 무기 그래프의 노드·핀·함수 참조 보존, 미리보기 밖 HARU Skeleton 보존, 픽업의 게임플레이 데이터 보존을 확인했다. 이후 내부 드라이버의 Visible도 비활성화한 최종 F를 저장했다.
- 실제 Level_1 PIE E 및 최종 숨김 저장본의 새 프로세스 PIE F에서 탄약30→발사29→재장전30, 원본 WeaponFire 이펙트 액터 생성, 이동 최고600cm/s와 총구 오차0.000003cm 미만을 확인했다. 최종 엔진 로그 Error/ensure0, 에셋·사용자 SaveGames 불변이다. 확인 화면은 `Saved/Diagnostics/SMGVisual/pie_F.png`이며 후방 플레이 시점이므로 손가락 접촉·아트 수용을 대신하지 않는다.
- 현재 Level_1 검사에는 해당 픽업 스포너가0개였으므로 픽업은 데이터 저장·재로드 확인이며, 실제 맵에서 획득 연출을 검증했다고 기록하지 않는다. 사용자 화면·그립·크기 수용 및 멀티플레이/패키징은 별도다.
- 스테이징·커밋·푸시는 하지 않았다. 기존3개 에셋은 LFS 캐시에 해시 검증해 보존했다.

근거: `Saved/Diagnostics/SMGVisual/{before_C.json,save_D.json,verify_D.json,visibility_save_F.json,pie_E.json,pie_F.json,pie_F.png,closeout_F.json}`, `Saved/Logs/SMGVisual-*.log`. 조사 A/B는 조회 결과와 보호 해시는 확보했지만 Python 종료 단계에서 충돌했으므로 프로세스 정상 종료로 기록하지 않는다. Save_C는 겹침 이벤트 설정 API 이름 문제로 저장 전에 중단됐다. PIE_D는 GAS 라이브러리의 Python 노출 이름 문제로 중단됐다. 해당 실패 과정의 에셋 저장은0이며 최종 성공 검사와 구분한다.
