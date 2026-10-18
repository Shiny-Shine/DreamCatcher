# SMG 탄창 외형 연결 — 원본 Magazine 본·재장전 유지

2026-10-18 KST. 사용자는 `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Gun_Magazine`의 탄창 에셋을 현재 무기에 연결하고 Lyra의 기존 탄창 연결·움직임을 유지하도록 승인했다. 실제 메시 **GUN_Magazine**을 확인해 연결했다. **저장·별도 재로드·실제 Level_1 재장전/발사/대시·외형 캡처 검증 완료**다. C++/Config/DLL 변경과 빌드·패키징·커밋/푸시는 하지 않았다. 추가 빌드는 필요 없다.

## 실제 변경

변경 에셋은 `/Game/DreamCatcher/Lyra/Weapons/Rifle/B_Rifle` 하나다.

기존 숨겨진 `SkeletalMesh` 아래에 엔진 기본 `StaticMeshComponent`인 **SMGMagazineVisual**을 추가했다.

- Static Mesh: `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Gun_Magazine/GUN_Magazine`
- Material: 해당 메시의 기존 `MI_Gun_Magazine`
- Parent Socket: **Magazine** — 기존 `SK_Rifle_SMGSocketDriver`의 원본 본
- Location: 약 `(-0.364458, 0.000012, -2.650894)`cm
- Rotation: `Roll 0, Pitch -2.96619, Yaw 0`°
- Scale: `(0.5,0.5,0.5)`
- Movable, 표시 활성, NoCollision, overlap/Nav 영향 비활성. 별도 물리 시뮬레이션은 사용하지 않는다.

탄창과 SMG의 메시 형상을 비교해 SMG 에셋 좌표에서 탄창 위치를 `(29,0,4)`로 맞췄다. 현재 SMGVisual의 Transform을 합성한 뒤 원본 Magazine 기준 자세에 대한 상대 Transform으로 변환했다. 탄창의 원본 정점·피벗·재질과 SMG 본체 위치/회전/크기를 수정하지 않았다. 현재 맞춤은 저장된 무기의 외형 캡처로 확인했으며, 캐릭터 손 접촉의 최종 미술적 수용은 사용자 확인 범위다.

## 원본 동작을 유지하는 방식

기존 `SK_Rifle_SMGSocketDriver`와 원본 `SK_Rifle`의 본7개(Root/Grip/Door/Trigger/Magazine/Barrel/Bolt)는 그대로다. 원본 `ABP_Weap_Rifle`, `AM_Weap_Rifle_Reload`, `Weap_Rifle_Reload`도 변경하지 않았다.

새 탄창은 원래 **Magazine 본의 이동·회전·스케일을 부모 부착으로 그대로 따른다.** 재장전 중 빠져나가며 작아졌다가 돌아오는 동작을 새 코드로 흉내 내지 않았다. 캐릭터 재장전 몽타주, 탄약 처리, 발사 그래프, 기존 SMGVisual/총구/WID/HARU/Level_1은 유지했다. 이벤트 그래프·Construction Script에 새 실행 노드를 추가하지 않았다.

원본 무기 몸체는 기존처럼 숨겨진 드라이버이고, 화면에는 SMGVisual과 SMGMagazineVisual이 표시된다. 노리쇠 등 다른 원본 부품 외형을 새 SMG에 추가한 것은 아니다.

## 편집과 범위

탄창 외형의 위치·회전·크기는 **B_Rifle → Components → SMGMagazineVisual → Transform**에서 조정한다. Parent Socket `Magazine`과 부모 SkeletalMesh는 유지한다. 본체의 SMGVisual만 나중에 움직이면 탄창 위치는 자동으로 같이 보정되지 않으므로 두 외형의 맞춤을 함께 확인한다.

이번 작업은 실제 B_Rifle과 그 Blueprint 미리보기 연결이다. **SKM_HARU의 기존 소켓 Preview Asset은 SM_GUN_SMG 단일 메시이므로 이 탄창 컴포넌트까지 표시하지 않는다.** HARU Skeleton과 픽업 표시 데이터는 변경하지 않았다.0.129에서 제거한 자체 SMG 그립 편집기·공유 설정·전용 맵도 재생성하지 않았다.

## 적용·검증 근거

진단 폴더: `Saved/Diagnostics/SMGMagazine20261018/`.

- `inspect_C.json`: GUN_Magazine이 StaticMesh임을 확인. 정점392/삼각형780. 원본/드라이버 본7개와 원본 재장전2.2초, Magazine의 이동/회전/스케일 변화를 조회했다. 에셋 저장0.
- `baseline_D.json`: 기존 B_Rifle을 Git LFS 객체로 정확히 보존하고 보호 에셋23개·Content/GameFeature 상태·Git index/HEAD·DLL 해시를 기록했다.
- `ui_prepare_F.json`: 기본 컴포넌트·메시·Transform·충돌 설정을 메모리에서 준비했다. Python에서 SCS의 Parent Socket 속성이 보호돼 있어 computer-use로 원래 에디터를 열었다. 도구 입력 시 소켓 선택 팝업이 닫혀, 사용자가 **Magazine 선택 → Compile → Save → 에디터 종료**를 완료했다고 확인했다. 신규 C++ 보조 기능은 추가하지 않았다.
- `save_F.json`: 사용자 저장 후 변경 파일이 B_Rifle1개임을 확인. 보호23개 불변. 이 기록 자체와 후속 재로드 검증을 구분한다.
- `verify_G2.json`: 새 프로세스 로드·Compile·실제 액터 생성에서 부모 SkeletalMesh/소켓 Magazine/메시 GUN_Magazine/표시·충돌 설정 확인. 위치 맞춤 오차 약0.00000056cm, 회전 오차0.00000069rad. 원래 무기 컴포넌트 주요 설정과 게임플레이 그래프 보존, 저장0, Error/ensure0.
- `pie_I.json`: 실제 Level_1 PIE에서 **646표본**의 탄창 위치·회전·스케일 추종 확인. 최대 위치 오차 약8.14e-10cm, 회전·스케일 오차0. 원본 `AM_Weap_Rifle_Reload`가 실행됐고 Magazine 본의 최대 이동208.30cm/최소 스케일0.1을 그대로 따랐다. 발사 탄약30→29, 원본 캐릭터 재장전과 탄약30 복구, 원본 대시/무기 부착 유지도 통과했다. 에셋 저장0, SaveGames 불변, Error/ensure0.
- `Saved/Automation/SMGMagazine_PIE_I/index.json`: 성공(경고 포함)1/실패0. 기존 NavMesh 원점 재생성·종료 시 CrowdManager/NavMesh 조회·검사 코드의 deprecated 속성 읽기 경고가 있다. 경고0으로 기록하지 않는다.
- `presentation_H.json`, `magazine_H.png`: 저장된 실제 무기를 임시 에디터 장면에서 렌더해 탄창 삽입구·외형을 확인했다. 새 맵 생성/저장0. 이 장면 캡처를 PIE라고 보고하지 않는다.
- `closeout_J.json`: B_Rifle1개만 변경, 보호23개·복구 원본·DLL·Git index/HEAD 보존, 최종 로그 Error/ensure0. 검사 프로세스 모두 종료했다.

초기 inspect_A/B와 memory_E/E2/E3는 Python 클래스명/속성 접근 제약 조사에서 중단됐고 저장0이다. verify_G는 텍스트 export에 같은 Construction Script 노드가 중복 출력되는 차이로 중단됐고, 동일 GUID/핀 중복을 정규화한 G2에서 원본과 일치했다. 실패한 검사를 성공으로 합산하지 않는다. `apply.py`의 memory/save 시도는 성공 경로가 아니며 현재 상태에서 재실행하지 않는다.

최종 B_Rifle SHA256: `009a808178aabe27ba1330e8ff4402da48ecb13a1b210a8cabf7b8e94fe7d19d`. 적용 전 원본 해시/복구 LFS 위치는 baseline_D를 따른다. 현재 연결은 완료됐으므로 추가 빌드나 동일 검증을 반복하지 않는다. 사용자 물리 입력·최종 화면 수용 및 온라인/패키징 검증은 별도다.
