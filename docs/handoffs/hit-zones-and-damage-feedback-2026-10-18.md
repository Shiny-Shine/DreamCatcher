# 부위별 피해·명중 표시·피격 피드백 — 2026-10-18

**A/B 적용·저장·재로드 및 실제 사격 자동 검사를 마쳤다. 다음은 사용자의 화면·소리 확인이며 추가 C++ 빌드는 필요 없다.** 시작 main은 `13447f7`이다. 완료한 HARU/SMG/탄창·원본 애니메이션·Level_1 검사는 반복하지 않는다. 아래 적용 전 조사 기록보다 이 현재 상태와 AGENTS 0.133이 우선한다.

## 현재 적용 상태

사용자 빌드 후 21:08:28 KST 갱신된 DLL에서 NumberPop 클래스3개와 요청 구조체가 로드됐다. 준비된 원본 기반 C++5파일과 Config2개는 이번 후속 작업에서 변경하지 않았다. Codex C++ 빌드·패키징·Git stage/commit/push/merge는 실행하지 않았다.

- HARU Physics Asset18Body: `neck_01`/`head`는 기존 `PhysMat_Player_WeakSpot`, 나머지는 `PhysMat_Player`. BodySetup과 DefaultInstance 양쪽에 지정했다.
- 보스: 현재 사용하는 템플릿 PA22Body를 `/Game/DreamCatcher/Physics/PA_DC_BossHitZones`로 Unreal 내부 복제했다. `neck_01`/`neck_02`/`head`는 약점이며 Boss BP의 Mesh.PhysicsAssetOverride만 사본으로 연결했다. 공용 템플릿은 보존했다.
- 원본 피격/숫자 누락48개: 엔진 `MigratePackages`로 정확한 목록만 추가한 뒤 엔진 `rename_assets`로 정식 폴더에 이동했다. Cue는 `/Game/DreamCatcher/Lyra/GameplayCues/DamageTaken/GCNL_Character_DamageTaken`, 나머지는 `/Game/DreamCatcher/Lyra/Feedback/DamageTaken/<원본 Game 상대 경로>`이다. 임시 원래 경로와 redirector는 남지 않았다.
- `/DCStageUI/DCStageUI`의 기존 AddComponents 목록에 `BP_DC_StageUIController`용 `B_NiagaraNumberPopComponent` 항목1개만 추가했다. 원본 Style→NS_DamageNumbers와 `DamageInfo` 연결을 검증했다. Client=True/Server=False이며 메뉴/AI에는 추가하지 않는다.

기존 에셋 변경은 **HARU PA/Boss BP/DCStageUI 3개**, 신규는 **피격48+보스 PA1=49개**다. Level_1, HARU Mesh/Skeleton, B_Rifle의 SMG/탄창/애니메이션 참조, 기존 공유220개, 원본 Lyra 프로젝트, Git HEAD/index, 사용자 SaveGames는 보존했다.

## 실제 통합에서 보완한 사운드 참조

첫 실제 몸통 사격에서 새 `sfx_Character_DamageGiven_nl_meta`가 원본 RandomStereo 내부 클래스 키를 찾지 못해 MetaSound 재생 오류가 발생했다. 공용 `/Game/Audio/MetaSounds/sfx_RandomStereo_nl_meta`의 전체 export를 비교한 결과 **RootGraph ClassName만** 원본 `C7725AA14BADE5EB7F9F54894DB8BE03`에서 현재 `5A9AD0844AAAEB75E9111BAC4F8BFFC3`로 달랐다. 파일의 기존 AssetClassID 캐시는 옛 값이었다.

공용 에셋을 덮어쓰지 않고 **이번에 추가한 명중/약점/처치/피격/약점 피격 MetaSound5개**의 RootMetasoundDocument 의존 클래스 식별자만 Unreal 구조체 속성 API로 대응했다. 저장 전 전체 문서 비교에서 그 식별자1개 외 입력/파형/그래프 변화가 없었다. 후속 실제 일반·약점 발사에서 MetaSound 오류가 사라졌다. 별도 C++나 새 공용 사운드 복제본을 추가하지 않았다.

## 자동 확인 결과

| 범위 | 확인 결과 | 근거 |
|---|---|---|
| 부위 재질/보스 연결 | 적용 전후 및 새 프로세스에서 도형·본·제약 보존, 일반/약점 재질과 보스 override 일치 | `physics_apply_G.json`, `physics_verify_L.json` |
| 원본 이식/재로드 | 원본·기존 공유 파일 보존,48개 로드/관련 BP Compile, 누락/이전48개 경로 의존0, NumberPop/Style/시스템/StageUI 연결 유지 | `feedback_migrate_H.json`, `feedback_integrate_J.json`, `feedback_verify_L.json` |
| Cue 그래프 | 연결된 원본 그래프의 핀/실행 연결 보존. Redirect 표시명·미연결 숨김 핀 GUID 차이와 원본의 미연결 확장 노드2개를 구분 | `cue_authored_graph_comparison_N.json` |
| HARU 실제 발사 | 몸통 `spine_02`12, 머리 `head`18, 빗나감0 | `pie_Q.json` |
| 보스 실제 발사 | 몸통 `spine_03`12, 머리 `head`18 | `pie_Q.json` |
| 탄약/피드백 실행 | 각 사격1발 소비. 실제 NumberPop 호출4회, NS_DamageNumbers 활성 및 명중 위치 오차0.009cm 미만 | `pie_Q.json`, `pie_Q.log` |
| 로그/파일 | 최종 PIE Error/ensure/Fatal0. Automation 성공1(기존 경고 포함)/실패0. 기존 사용자 파일·SaveGames 변화0 | `Saved/Automation/HitZones_PIE_Q/index.json`, `asset_scope_Q.json` |

위 JSON/log는 `Saved/Diagnostics/HitZones20261018/` 아래다. PIE는 현재 Level_1 Experience/장비/HUD를 사용하고 런타임에서 AI·이동을 멈춘 뒤 공중의 같은 거리로 캐릭터를 배치했다. 보스는 현재 BP를 런타임에 추가했다. 실제 입력 주입으로 발사했으며 가짜 HitResult/SetByCaller 피해를 사용하지 않았다. 발사 직전 동일 무기 채널의 본/재질 trace와 실제 체력·탄약·Cue 결과를 함께 기록했다. 전체 스테이지/보스 패턴/복제 회귀를 다시 완료했다고 해석하지 않는다.

Niagara는 한 프레임에 `DamageInfo`를 소비하므로 Python 후속 tick에서는 raw 배열을 직접 포착하지 못했다. **원본 약점 판정→치명타 bool→숫자 부호 코드/그래프 연결과 실제 숫자 시스템 실행을 확인한 것이며, 렌더링된 치명타 색/크기·마커·소리 청취 완료는 아니다.** 일반/약점의 별도 캐릭터 스파크 템플릿은 원본에 없어 공통 NS_ImactSparksCharacter를 유지한다.

공유220개 비교는 텍스트/텍스처 설정·픽셀 기준158개 일치,62개는 메타데이터/이전 이식·재생성 차이를 기록했다.220개 전체의 시각/동작 동등성을 확정하지 않았다. 이번 연결에 필요한 사운드 클래스 키 차이는 위 방식으로 해결하고 실제 Cue 실행을 확인했다. 앞선 HDR 텍스처 PNG export 중단, 검사 파일명/보호 getter/플러그인 마운트 오류, 예전 HUD 조회와 raw 배열 관측 실패는 최종 성공으로 합산하지 않는다. 부분 대용량 텍스처 텍스트 출력2.79GB만 정리했고 진단 로그는 남겼다.

## 사용자 확인 및 다음 작업

1. Editor를 새로 열고 Level_1에서 일반 적과 보스의 몸·머리를 각각 단발로 맞힌다. 같은 근거리 기준12/18과 약점 히트마커·숫자 표시를 확인한다.
2. 일반/약점 명중음과 캐릭터 스파크, 적에게 맞을 때 피격 화면·사운드를 확인한다. 처치음·원본 히트 리액션의 시각/청취 수용도 여기에 포함한다.
3. 결과에 문제가 없으면 사용자가 GitHub Desktop 커밋/푸시/병합을 진행한다. 패키징 역시 사용자 담당이다.

이 작업 뒤 C++ 변경은 없으므로 재빌드를 요청하지 않는다. 동작/화면 문제가 보고되면 해당 재현 범위만 보완하며 완료한 R14·맵 복구·전체 마이그레이션·탄창 검증으로 돌아가지 않는다.

일회성 자동화: `Scripts/Editor/hit_zone_physics_setup.py`, `damage_feedback_assets.py`, `damage_feedback_metasound_link.py`, `hit_zone_gameplay_probe.py`. 쓰기 스크립트는 정확한 목록, SourceControl 비활성, 기존 파일 보호와 출력 경로 충돌 방지를 사용한다. 기존 성공 작업을 그대로 재실행할 필요는 없다.

## 적용 전 확인한 원인

### 피해 배율과 히트마커 자체는 이미 있다

- 현재 `B_WeaponInstance_Rifle.MaterialDamageMultiplier`는 원본과 동일한 `Gameplay.Zone.WeakSpot → 1.5`다.
- `DCLyraGameplayAbility_RangedWeapon::WeaponTrace`는 실제 무기 채널에 complex trace를 하고 `bReturnPhysicalMaterial=true`를 사용한다.
- `DCLyraDamageExecution`은 기본 피해 × 거리 감쇠 × 물리 재질 배율 × 팀 허용 여부를 계산한다. `DCLyraRangedWeaponInstance::GetPhysicalMaterialAttenuation`은 `DCPhysicalMaterialWithTags.Tags`와 위 배율표를 조회하며 일치 태그가 없으면1.0이다.
- `DCLyraWeaponStateComponent`도 같은 HitResult.PhysMaterial의 `Gameplay.Zone.*` 태그를 히트마커에 전달한다. 현재 `W_Reticle_Rifle`에는 약점 전용 `MI_UI_Reticles_HitMarkerConfirmation_Critical` 브러시가 원본 그대로 연결돼 있다.
- 따라서 머리·팔·다리라는 본 이름만으로 배율/마커가 달라지는 구조가 아니다. 원본 라이플의 현재 설정은 **일반1.0 / 약점1.5**이며, 부위마다 별도 임의 배율을 확정하지 않는다.

### 현재 캐릭터의 물리 재질 연결이 빠져 있다

| 대상 | 현재 실제 메시/Physics Asset | 확인 내용 |
|---|---|---|
| 공용 플레이어/일반 적 Pawn | `SKM_HARU` → `/Game/DreamCatcher/Main_Charcher/Haru/하루리깅_PhysicsAsset` | Body18개. `neck_01`, `head`를 포함하지만 부위별 PhysMaterial/DefaultInstance.PhysMaterialOverride 지정이 없고 메시 컴포넌트 전체 override도 없다. |
| 보스 | `SKM_Quinn_Simple` → `/Game/Characters/Mannequins/Rigs/PA_Mannequin` | Body22개. 부위별 물리 재질 지정이 없다. |
| 원본 Lyra 및 보존된 원본 PA | `/Game/Characters/Heroes/Mannequin/Rig/PA_Mannequin` | Body22개. 일반은 `PhysMat_Player`, `neck_01`/`neck_02`/`head`는 `PhysMat_Player_WeakSpot`을 PhysMaterial과 DefaultInstance override에 지정한다. |

두 물리 재질은 이미 DreamCatcher에 존재하고 정상 로드된다.

- `/Game/Characters/Heroes/PhysMat_Player`: `DCPhysicalMaterialWithTags`, Character 표면, 태그 없음.
- `/Game/Characters/Heroes/PhysMat_Player_WeakSpot`: 같은 클래스/표면, `Gameplay.Zone.WeakSpot` 태그.

HARU/보스의 Mesh는 무기 trace를 막고 Capsule은 해당 trace를 무시하도록 연결돼 있다. 새로운 부위별 피해 계산 C++를 만드는 것보다 **현재 사용하는 Physics Asset의 각 Body에 기존 원본 재질을 연결**하는 것이 먼저다. 현재 결과는 코드·CDO·Physics Asset export 및 임시 Editor 액터의 실제 컴포넌트 설정 확인이다. 실제 사격의 BoneName/PhysMaterial/체력 차이를 측정한 PIE 결과로 확대하지 않는다.

### 원본 캐릭터 피격 Cue는 별도로 누락돼 있다

원본 `/Game/GameplayCueNotifies/GCNL_Character_DamageTaken`은 현재 프로젝트에 없다. `GameplayCue.Character.DamageTaken` 태그 자체는 Config에 이미 있지만 이 태그를 처리할 원본 Cue와 관련 피드백 에셋은 별개다.

원본 그래프/기본값에서 확인한 동작:

- `EvaluateWeakSpot`은 HitResult의 PhysicalMaterialWithTags에서 약점 태그를 조회한다.
- 공격자의 일반/약점 명중음, 피해자의 일반/약점 피격음을 각각 Select한다. 처치음 분기도 별도로 있다.
- 피해 숫자 요청의 `bIsCriticalDamage`에 약점 판정을 전달한다. 원본 Niagara NumberPop 컴포넌트는 이를 Niagara 데이터에 담아 일반/치명타 표시를 구분한다.
- 기본 캐릭터 피격 파티클은 `NS_ImactSparksCharacter`이며 카메라 피격 표시·카메라 흔들림·진동도 포함한다. **일반/약점별로 서로 다른 캐릭터 파티클 템플릿을 선택하는 분기는 이 Cue 기본값에서 확인하지 못했다.** 사운드/숫자/히트마커의 차이와 구분한다.
- `B_WeaponImpacts`와 `GCN_Weapon_Impact`의 벽/유리 등 표면 효과는 별도 경로다. 원본과 현재 `B_WeaponImpacts`는 Concrete/Glass 분기를 갖고 Character/Default 시스템은 None이다. 이 경로만으로 누락된 캐릭터 DamageTaken Cue를 대신하지 않는다.

Cue와 원본 `B_NiagaraNumberPopComponent`의 `/Game` 의존성 합집합은268개이며 현재 경로가 없는 후보는48개다(캐릭터 피드백40＋NumberPop8). 기존220개는 재사용 후보이며 파일이 존재한다는 이유만으로 모든 데이터가 원본과 같다고 판정하지 않는다. 원본 Cue의 Lyra native 참조 중 Health/VerbMessage/PhysicalMaterial 타입은 이미 대응이 있고, **NumberPop 컴포넌트/요청 구조체/스타일 계층은 현재 소스에 없다.**

## 승인된 변경 묶음

### A. 원본 기준 약점 판정·히트마커 연결

1. 기존 HARU Physics Asset1개에 원본 물리 재질을 지정한다. 기존 `neck_01`/`head` Body는 약점, 나머지는 일반으로 설정한다. 공용 HARU를 쓰는 플레이어/봇 모두에 적용되는 범위다.
2. 보스가 쓰는 현재 Physics Asset을 Unreal 내부에서 새 `/Game/DreamCatcher/Physics/PA_DC_BossHitZones`로 복제하고 기존 Body22개의 재질만 지정한다. 목2개/머리는 약점, 나머지는 일반이다.
3. 기존 `/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss`의 Mesh.PhysicsAssetOverride만 새 사본으로 연결한다. 공용 템플릿 Physics Asset은 수정하지 않는다.

기존 Lyra PA를 보스/HARU에 통째로 바꾸는 대안은 사용하지 않는 안이다. HARU는 Body18/원본22로 구성이 다르고, 보스 템플릿과 원본 Lyra PA도 발 박스 높이·허벅지/종아리 캡슐 치수 등이 실제로 다르다. **현재 충돌 도형·본 연결·제약을 유지하면서 재질/태그만 재사용**한다. 피해 공식/배율표/Reticle 그래프를 재작성하지 않는다.

A의 변경 대상은 기존 HARU Physics Asset, 신규 보스 Physics Asset 사본, 기존 보스 BP의 연결1개다. HARU Mesh/Skeleton/스킨 가중치/SMG/탄창/발사·재장전·대시/Level_1은 보존한다.

### B. 원본 피격 Cue와 피해 숫자 표시 연결

- 원본 NumberPop 기반2파일, NiagaraText2파일, Niagara 스타일 헤더1개 **총5개 C++ 파일**을 이름/include/모듈 API/로그 대응만 해서 이식한다. 원본 Cue가 호출하는 숫자 표시 경로를 빈 대체 클래스로 숨기거나 그래프에서 제거하지 않는다.
- 위3개 클래스와 요청 구조체의 Redirect, 별도 Cue 경로 스캔을 Config에 등록한다. 이미 있는 DamageTaken/WeakSpot 태그는 중복 추가하지 않는다.
- 원본 Cue 및 관련 누락48개 후보를 별도 정식 폴더에 이식하고, 필요한 기존 공유 의존성은 비교 후 재사용한다. 기존 공유 에셋을 일괄 덮어쓰지 않는다. 원본 자산은 보존하며 추가 제약/새 의존성으로 범위가 달라지면 변경 전에 설명한다.
- 원본 NumberPop BP/스타일/숫자 Niagara를 연결하고, `DCStageUI`의 기존 컴포넌트 등록 액션에 현재 플레이어 Controller용 NumberPop만 추가한다. 메뉴/AI Controller에는 추가하지 않는다.
- 일반/약점 명중음·피격음·치명타 숫자 구분과 기존 캐릭터 피격 파티클/피격 화면을 원본 방식으로 복원한다. 별도 헤드샷 파티클을 새로 디자인하는 작업은 포함하지 않는다.

A/B는 사용자 승인과 C++ 빌드 이후 위 현재 상태까지 적용했다. 아래 최초 검증 계획과 실제 확인 범위·남은 사용자 수용을 구분한다.

## 검증 계획

- 수정 전후 Body 도형/본/제약과 보호 에셋 불변, 허용된 재질·연결만 변경됐는지 확인.
- 같은 거리의 실제 발사에서 몸/머리의 HitResult BoneName·PhysMaterial·태그, 소비 탄약, 실제 체력 감소와1.5 배율을 함께 기록. 임시 약점 재질을 가짜 표적에 붙였던 과거 검사를 현재 적의 본별 검사로 대신하지 않음.
- 실제 약점 히트마커, 일반/약점 피격 Cue/사운드/숫자 구분을 확인. 정상 명중·빗나감·사망 후 종료와 메시 없는 전환 프레임 등 관련 영향만 검사.
- 사용자에게 남길 확인은 화면/소리/조작감, 필요 C++ 빌드 및 패키징이다. 이미 완료된 모델/탄창/마이그레이션 전체를 재검증하지 않는다.

## 로컬 조사 근거

`Saved/Diagnostics/HitZones20261018/`의 `target_B/report.json`, `source_A/report.json`, `source_feedback_A/report.json`, `source_numberpop_A/report.json` 및 T3D/그래프 기록. 게임 에셋 저장0, 명시적 Blueprint Compile0. 첫 target_A는 Python에 없는 SkeletalMeshComponent.GetPhysicsAsset 조회에서 중단됐고, 실제 override/mesh 속성 조회로 교정한 B가 성공했다. 보호된 SkeletalBodySetups 배열은 직접 열지 않고 전체 텍스트 export로 Body 설정을 확인했다. 실패를 성공으로 합산하지 않는다.

원본/대상: `E:/Epic Games/UEProjects/LyraStarterGame`, `E:/Epic Games/UEProjects/DreamCatcher`. 추가 의존성 후보 목록은 같은 이름의 JSON manifest에 보존한다. **최신 상태는 문서 상단과 AGENTS 0.133이다. 사용자 빌드·에셋 적용·제한된 실제 사격 자동 검사를 마쳤고 화면·소리 수용이 남아 있다.**
