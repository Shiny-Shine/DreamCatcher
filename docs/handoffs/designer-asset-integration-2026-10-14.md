# DreamCatcher 디자이너 에셋 통합 안내

**2026-10-16 후속 0.123:** 현재 Pawn은 SKM_HARU, 보이는 무기는 B_Rifle의 **SMGVisual → SM_GUN_SMG**다. 실제/미리보기/픽업 표시를 교체했고 HARU의 Lyra 무기 본과 가상 본3개는 유지했다. [SMG 편집 위치와 총구·미리보기 안내](smg-visual-2026-10-16.md), [HARU 무기 본·재임포트 안내](haru-weapon-socket-2026-10-16.md)를 따른다. 아래 Manny 표는 초기 연결 기준이며 전체 HARU 리타깃 완료를 의미하지 않는다.

2026-10-14 정식 경로 정리와 사용자 PIE 확인을 마친 `Level_1` 기준이다. 캐릭터·무기·보스의 모델, 재질, 애니메이션을 현재 게임플레이에 연결할 때 사용할 에셋과 수정 위치를 정리한다.

**현재 플레이어와 일반 적은 같은 Pawn을 사용한다.** 공용 Pawn의 메시를 바꾸면 양쪽 외형이 함께 바뀐다. 서로 다른 모델을 사용할 경우 모델 제작과 함께 플레이어/적별 외형 적용 경로를 개발 담당자와 분리해야 한다. 보스는 별도 Pawn이다.

아래 `/Game/`은 Unreal 콘텐츠 브라우저의 `Content/`에 해당한다. 전체 에셋 경로를 기준으로 찾는다. 같은 파일명이 다른 폴더에 남아 있을 수 있다. FBX/Blender 등 모델 원본은 제작 도구에서 수정하고 Unreal로 Import/Reimport하며, Blueprint는 해당 모델을 게임에서 사용하는 연결 지점이다.

**작업별 시작 에셋**

| 작업 | 열 에셋 | 수정 위치와 영향 |
|---|---|---|
| 플레이어와 일반 적의 공통 외형 | `/Game/DreamCatcher/Blueprints/Player/BP_DC_LyraInventoryPawn` | Components의 `Mesh (CharacterMesh0)` → Skeletal Mesh Asset, Materials, Anim Class, 상대 Transform. 현재 양쪽에 공통 적용된다. |
| 공통 캐릭터 기반 | `/Game/DreamCatcher/Blueprints/Player/BP_DC_RiflePawn` | 위 Pawn의 부모. 입력·장비·체력·카메라 등 공통 구성이 있으므로 외형 교체는 먼저 실제 자식 Pawn에서 적용한다. 부모 수정은 자식의 override 여부도 확인한다. |
| 보스 외형 | `/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss` | `Mesh (CharacterMesh0)`의 메시·재질·Anim Class·상대 Transform. 일반 적/플레이어와 분리돼 있다. |
| 손에 든 무기 외형 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/B_Rifle` | **SMGVisual** 컴포넌트의 Static Mesh·재질·상대 Transform. 상속된 SkeletalMesh는 숨겨진 내부 드라이버이므로 외형 수정 대상으로 쓰지 않는다. |
| 총구·탄피 배출 기준점 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/SK_Rifle_SMGSocketDriver` | Mesh Socket의 Muzzle/ShellEject. SMGVisual의 위치·크기를 바꾸면 미리보기와 함께 재정렬한다. 내부 드라이버는 새 디자인 메시로 재임포트하지 않는다. |
| 무기를 손에 붙이는 위치 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/WID_Rifle` | Class Defaults → `ActorsToSpawn`의 `AttachSocket`, `AttachTransform`. 캐릭터 메시를 기준으로 부착한다. |
| 라이플 장착 중 캐릭터 동작 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/B_WeaponInstance_Rifle` | `EquippedAnimSet`, 장착 몽타주 등 캐릭터 애니메이션 연결. 무기 자체의 기계 동작은 별도 AnimBP다. |
| 무기 픽업 표시 | `/Game/DreamCatcher/Lyra/AI/ShooterCore/Weapons/Rifle/WeaponPickupData_Rifle` | `DisplayMesh`, `WeaponMeshOffset`, `WeaponMeshScale`. 손에 든 모델과 별도인 Static Mesh 표시다. 픽업을 배치·사용하는 경우 함께 맞춘다. |
| 환경·배치 | `/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1` | 환경 메시·재질·조명·장식. 현재 실제 플레이 맵이다. |

**캐릭터 모델과 애니메이션**

현재 캐릭터 연결은 다음과 같다.

`B_DC_StageUIExperience → DA_DC_LyraAIPawn → BP_DC_LyraInventoryPawn → BP_DC_RiflePawn`

Experience는 `/Game/DreamCatcher/Experiences/B_DC_StageUIExperience`, PawnData는 `/Game/DreamCatcher/Data/Pawns/DA_DC_LyraAIPawn`이다. PawnData 이름의 AI는 적 전용이라는 뜻이 아니다. GameMode의 기본 Pawn만 변경하거나 맵에 다른 캐릭터를 놓는 방식으로 현재 생성 경로를 교체하지 않는다.

| 구성 | 현재 에셋 |
|---|---|
| 캐릭터 Skeletal Mesh | `/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny` |
| Skeleton | `/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin` |
| Physics Asset | `/Game/Characters/Heroes/Mannequin/Rig/PA_Mannequin` |
| 기본 AnimBP | `/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base` |
| 라이플 동작 레이어 | `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers` |
| 태그로 선택하는 별도 동작 레이어 | `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers_Feminine` |
| 기본 몸 재질 | `/Game/Characters/Heroes/Mannequin/Materials/Instances/Manny/MI_Manny_01_Blue`, `/Game/Characters/Heroes/Mannequin/Materials/Instances/Manny/MI_Manny_02_Blue` |

현재 메시 상대 Transform은 위치 `(0, 0, -90)`, 회전 `(0, -90, 0)`, 크기 `(1, 1, 1)`이다. 새 모델의 원점과 방향이 다르면 기존 값에서 맞춘다. 외형 크기 조정을 위해 Actor 전체를 무조건 확대하면 이동·충돌·부착 위치까지 영향을 받으므로 메시 정렬과 캡슐/Physics Asset 조정을 구분한다.

1. 새 캐릭터를 **별도 Skeletal Mesh 에셋**으로 가져오고 재질을 구성한다. 기존 Manny에 덮어쓰기보다 적용 대상을 명시적으로 선택할 수 있다.
2. 현재 Skeleton과 호환되는 리깅이면 기존 AnimBP·라이플 레이어를 유지한 상태에서 새 메시를 연결해 확인한다. 같은 이름의 뼈만 있다고 호환이 보장되는 것은 아니다.
3. Skeleton이 다르면 이동 애니메이션뿐 아니라 장착·발사·재장전·회피·사망에 쓰는 애니메이션, AnimBP/연결 레이어, IK와 소켓을 함께 대응한다. 모델을 바꾸는 것만으로 기존 동작이 자동 연결되지는 않는다.
4. 재질은 Mesh 컴포넌트의 슬롯 override까지 확인한다. 새 메시의 기본 재질을 바꿔도 컴포넌트에 기존 재질이 지정돼 있으면 기존 외형이 남을 수 있다.
5. 플레이어와 일반 적을 다르게 보이게 만들 때는 각각의 모델·재질을 준비하고, 실제 생성 시 서로 다른 외형이 적용되도록 개발 담당자가 연결한다. 자식 BP 두 개를 만드는 것만으로 현재 공용 PawnData 경로가 분리되지는 않는다.

라이플을 들 때의 이동·조준 자세는 `ABP_Mannequin_Base`와 `ABP_RifleAnimLayers`가 함께 담당한다. 캐릭터의 Anim Class를 단순 이동용 AnimBP로 바꾸면 연결된 장비 애니메이션도 다시 구성해야 한다. `Feminine` 레이어는 스타일 태그로 선택되므로 모델 이름만 바꿔 자동 선택되는 것으로 간주하지 않는다.

**라이플 모델과 장착 연결**

현재 장비 연결은 `ID_Rifle → WID_Rifle → B_Rifle + B_WeaponInstance_Rifle`이다. 네 에셋은 모두 `/Game/DreamCatcher/Lyra/Weapons/Rifle`에 있다. 외형을 수정할 실제 액터는 이 폴더의 `B_Rifle`이다.

| 구성 | 현재 에셋 또는 필드 | 디자이너 작업 |
|---|---|---|
| 손에 든 무기 메시 | `/Game/Weapons/Rifle/Mesh/SK_Rifle` | 새 Skeletal Mesh를 `B_Rifle.SkeletalMesh`에 연결한다. |
| 무기 Skeleton | `/Game/Weapons/Rifle/Mesh/SK_Rifle_Skeleton` | 재장전·발사에 사용되는 본/피벗과 새 모델의 대응을 확인한다. |
| 무기 재질 | `/Game/Weapons/Rifle/Materials/MI_Weapon_Rifle` | 새 재질을 연결하고 컴포넌트 Materials override도 확인한다. |
| 무기 자체 AnimBP | `/Game/Weapons/Rifle/Animations/ABP_Weap_Rifle` | 무기 메시의 본 애니메이션용이다. 캐릭터 이동 AnimBP와 구분한다. |
| 캐릭터 발사·재장전 몽타주 | `/Game/Weapons/Rifle/Animations/AM_MM_Rifle_Fire`, `/Game/Weapons/Rifle/Animations/AM_MM_Rifle_Reload` | 손·어깨·탄창 위치를 새 모델에 맞춘다. 기존 이벤트·슬롯·노티파이와 게임플레이 완료 연결을 보존한다. |
| 무기 발사·재장전 몽타주 | `/Game/Weapons/Rifle/Animations/AM_Weap_Rifle_Fire`, `/Game/Weapons/Rifle/Animations/AM_Weap_Rifle_Reload` | 총 자체의 부품 움직임을 맞춘다. |
| 캐릭터 부착 | `WID_Rifle → ActorsToSpawn → AttachSocket / AttachTransform` | 현재 부착 대상으로 지정된 소켓이 새 캐릭터에도 존재하는지 확인하고, 그립 위치·회전·크기를 조정한다. |
| 총구 기준 | `B_Weapon` 그래프의 `Muzzle` 소켓 조회 | 새 무기에서도 `Muzzle` 이름과 총구 위치·방향을 맞춘다. 발사 이펙트·탄 궤적의 기준에 사용된다. |
| 픽업 표시 메시 | `/Game/DreamCatcher/Lyra/AI/Game/Weapons/Rifle/Mesh/SM_Rifle` | `WeaponPickupData_Rifle.DisplayMesh`에서 새 Static Mesh로 교체한다. 장착 메시 교체와 별도다. |

현재 라이플의 컴포넌트 Transform은 위치·회전 0, 크기 1이다. 무기 원본의 피벗과 축을 먼저 맞춘 뒤 장착 offset을 조정한다. 본·소켓이 다른 무기에서는 재장전, 탄피 배출, 왼손 위치까지 확인한다. 기존 공통 부모 `B_Weapon`의 발사·효과 그래프를 외형 교체 때문에 지우거나 새 입력 로직으로 대체하지 않는다.

저장소에 있는 디자이너 무기 모델은 다음처럼 **Static Mesh**다.

| 기존 디자인 에셋 | 현재 형식 |
|---|---|
| `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Gun_SMG/SM_GUN_SMG` | Static Mesh |
| `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Gun_Pistol/Gun_Pistol` | Static Mesh |
| `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Mesh/Gun/SM_Dummy_Gun_MPX` | Static Mesh |
| `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Mesh/Gun/SM_Dummy_Gun_Pistol` | Static Mesh |

이 Static Mesh들은 환경 소품이나 픽업 표시로 사용할 수 있지만 `B_Rifle`의 Skeletal Mesh 필드에는 그대로 지정할 수 없다. 현재 장착·재장전 구조에 맞는 Skeletal Mesh로 리깅하거나, Static Mesh 외형을 사용하는 별도 표시 연결을 개발 담당자가 구성해야 한다. 후자를 선택해도 총구·그립·이펙트 기준과 재장전 표현은 맞춰야 한다. 단순 외형 교체는 SMG/권총 전용 발사 방식이나 성능을 새로 구현하는 작업과 구분한다.

**보스 모델과 공격 연출**

`BP_DC_Boss`의 현재 메시가 `/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`이며 Anim Class는 미지정이다. 메시 상대 위치는 `(0, 0, -85)`, 회전은 `(0, -90, 0)`이다. 보스는 현재 제자리에서 패턴을 실행하는 구조이며 C++에서 이동을 비활성화한다.

- 모델·재질·대기 AnimBP는 `BP_DC_Boss`의 Mesh에 연결한다. 기존 ASC·Health·BossPhase 컴포넌트와 보스 부모 클래스는 유지한다.
- 체력·페이즈 전환·패턴 사이 간격과 예고/명중 재질은 `/Game/DreamCatcher/Data/Boss/DA_DC_Boss`에서 설정한다. 외형만 변경할 때는 전투 수치를 함께 바꾸지 않는다.
- 예고 재질은 `/Game/DreamCatcher/FX/Boss/M_DC_BossWarning`, 명중 재질은 `/Game/DreamCatcher/FX/Boss/M_DC_BossImpact`다. 데이터의 `WarningMaterial`, `ImpactMaterial` 연결을 사용한다.
- 직선 공격은 `/Game/DreamCatcher/GAS/Abilities/Boss/GA_DC_BossLineShot`, 지면 공격은 `/Game/DreamCatcher/GAS/Abilities/Boss/GA_DC_BossGroundStrike`다. 현재 공격 모션을 넣을 공통 Montage 필드가 완성돼 있는 구조는 아니다. 공격/전환/피격/사망 모션을 받으면 패턴 시점에 연결하는 개발 작업이 필요하다.
- 현재 바닥 원·직선 예고의 형상은 C++ `DCBossTelegraph`가 생성한다. 재질 변경은 위 에셋으로 가능하고, Niagara·데칼 기반의 새 예고 연출은 생성·종료 시점 연결도 함께 작업한다.

보스 모델만 교체해도 전용 공격 모션이 자동 재생되는 것은 아니다. 모션 길이와 실제 예고·피해 시점을 맞추는 작업을 별도로 전달한다.

**총기 효과와 UI 아트**

| 작업 | 연결 지점 |
|---|---|
| 총구 섬광 | `/Game/Effects/Particles/Weapons/NS_WeaponFire_MuzzleFlash_Rifle` 및 `B_Rifle`의 효과 설정 |
| 탄 궤적·탄피 | `/Game/Effects/Particles/Weapons/NS_WeaponFire_Tracer`, `/Game/Effects/Particles/Weapons/NS_WeaponFire_ShellEject` |
| 발사 효과 호출 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/GCN_Weapon_Rifle_Fire` |
| 현재 발사 사운드 | `/Game/DreamCatcher/Audio/Weapons/MSS_DC_Rifle_Fire` |
| 조준점 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/W_Reticle_Rifle` |
| 탄약 표시 | `/Game/DreamCatcher/Lyra/Weapons/Rifle/W_AmmoCounter_Rifle` |
| HUD 전체 배치 | `/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout` |
| ESC 메뉴 | `/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_LyraGameMenu` |

위 효과·위젯은 공용 참조가 있으므로 특정 무기만 바꾸려면 별도 효과/재질 사본을 연결한다. UI 아트를 바꿀 때 기존 위젯 이름·이벤트·데이터 연결과 조준점의 PlayerState 검사 가드를 보존한다. 보스 체력·현재 진행·결과 UI는 현재 `Source/DreamCatcher/UI/Boss`와 `Source/DreamCatcher/UI/Stage`의 C++ 위젯이므로 그 구조 변경은 개발 담당자와 진행한다.

**환경 배치와 통합 순서**

환경 모델·재질·조명은 `Level_1`에 반영한다. 현재 GameMode는 `/Game/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode`다. 맵의 StageDirector, EncounterController, 일반 적/보스 스포너, PlayerStart, NavMeshBoundsVolume과 그 참조를 유지한다. 벽·바닥을 바꾸면 충돌과 NavMesh 경로도 확인한다. 캐릭터 체격이나 캡슐까지 바꾸는 경우 AI 통과 폭과 Nav Agent 설정도 개발 담당자와 맞춘다.

1. 플레이어·일반 적·보스·무기별 새 모델, 재질, Skeleton/Physics Asset, 애니메이션 경로를 정한다. 서로 다른 플레이어/적 모델은 적용 경로 분리 작업도 함께 전달한다.
2. 한 종류씩 새 에셋을 Import하고 위의 실제 사용 BP에 연결한다. 공용 원본 메시·재질을 덮어쓰는 대신 새 에셋을 연결하면 영향 범위를 관리하기 쉽다. 에셋 이동·복사·이름 변경은 Unreal 콘텐츠 브라우저에서 처리한다.
3. 해당 변경에 필요한 짧은 PIE 확인을 한다. 캐릭터는 이동·조준·장착·재장전·사망, 무기는 그립·총구·탄피·픽업 표시, 보스는 예고와 실제 공격 시점, 환경은 충돌·AI 이동을 본다. 이미 수용한 변경 없는 기능을 일괄 재검증할 필요는 없다.
4. 모델·텍스처·재질·Skeleton·Physics Asset·애니메이션과 수정한 BP를 하나의 작업 묶음으로 전달한다. 같은 BP나 `Level_1.umap`을 두 사람이 동시에 저장하지 않도록 담당을 나눈다. C++ 변경이 필요한 항목은 개발 담당자가 처리하고 빌드·패키징은 사용자가 진행한다.

디자이너 전달 목록에는 **적용 대상, Unreal 에셋 경로, Skeleton 호환 여부, 재질 슬롯, 필요한 모션, 무기 그립·총구 위치**를 적는다. 새 디자인 모델의 호환성과 화면 결과는 실제 모델을 연결한 뒤 확인한다.

이 안내의 현재 연결 관계는 정식 경로 이관 기록, 실사용 Pawn 재저장/잔여 정리 검사, 현재 장비·GameMode·보스 소스로 확인했다. 이번 작업은 안내 문서 작성이며 게임 에셋·소스·Config 변경은 없다.
