# DreamCatcher GAS·Lyra 전환 명세

## 상태

- 결정 상태: 승인됨
- 엔진 버전: Unreal Engine 5.8
- 기준 프로젝트: DreamCatcher
- 참고 프로젝트: Lyra Starter Game 5.8
- Lyra 로컬 경로: E:\Epic Games\UEProjects\LyraStarterGame
- 적용 방식: Lyra 구조를 참고해 DreamCatcher용으로 직접 구현
- 에셋 정책: 독립적으로 사용할 수 있는 Lyra 에셋은 의존성을 확인한 뒤 최대한 Migrate

## 목표

DreamCatcher의 기존 전투 구조를 Gameplay Ability System 기반으로 단계적으로 교체한다.

다음 시스템은 Lyra의 구조를 참고한다.

- Ability System Component
- Gameplay Ability
- Gameplay Effect
- AttributeSet
- Gameplay Tag
- AbilitySet
- PawnData
- Input Tag
- Ability Tag Relationship Mapping
- Equipment
- WeaponInstance
- RangedWeaponInstance
- GameplayCue
- CameraMode
- 실제 탄 퍼짐 기반 Reticle

## 핵심 원칙

- 기존 시스템은 GAS 구현이 같은 기능을 검증하기 전까지 삭제하지 않는다.
- 기존 플레이 가능한 맵과 Character Blueprint를 직접 실험 대상으로 사용하지 않는다.
- GAS 테스트 맵과 테스트 Character Blueprint에서 먼저 검증한다.
- C++는 규칙, 상태, 판정, 수명 관리를 담당한다.
- Blueprint는 애니메이션, VFX, SFX, Camera Shake, UI 연출을 담당한다.
- Lyra C++ 코드는 그대로 복사하지 않는다.
- Lyra Blueprint는 부모 클래스와 의존성을 확인한 뒤 Migrate 또는 재구성한다.
- `.uasset`과 `.umap`은 Unreal Editor 밖에서 복사하거나 수정하지 않는다.

## 플레이어 ASC 소유 구조

플레이어의 Ability System Component는 PlayerState가 소유한다.

ADCPlayerState
- UDCAbilitySystemComponent
- UDCHealthSet
- UDCCombatSet
- UDCResourceSet

ADreamCatcherCharacter
- PlayerState ASC의 Avatar
- 이동과 카메라의 실제 Pawn
- DCPawnExtensionComponent
- DCHealthComponent
- DCEquipmentManagerComponent
- DCCameraComponent

일반 적과 보스는 Pawn 교체가 필요하지 않으므로 Character가 ASC를 직접 소유할 수 있다.

## 입력 구조

IA_Move와 IA_Look은 일반 C++ 입력으로 유지한다.

전투 입력은 Input Tag를 통해 Ability System으로 전달한다.

IA_Fire
→ InputTag.Weapon.Fire
→ DCAbilitySystemComponent
→ GA_DC_RifleFire

IA_Aim
→ InputTag.Aim
→ GA_DC_Aim

IA_Dodge
→ InputTag.Dodge
→ GA_DC_Dodge

IA_Ultimate
→ InputTag.Ultimate
→ GA_DC_Ultimate

## 확정된 조준 명세

Hip에서 짧게 누르고 해제
→ Scope

Hip에서 우클릭을 일정 시간 이상 유지
→ Shoulder
→ 해제
→ Hip

Scope에서 우클릭 누름
→ 즉시 Hip
→ 이후 버튼을 떼어도 변화 없음

조준 상태는 다음 Gameplay Tag로 표현한다.

- State.Aim.Shoulder
- State.Aim.Scope

Hip은 두 태그가 모두 없는 상태다.

## 발사 구조

GA_DC_RifleFire
1. 현재 장착된 DCRangedWeaponInstance 조회
2. 발사 가능 여부 확인
3. 현재 실제 퍼짐각 조회
4. 카메라 기준 조준점 계산
5. 총구 가림 검사
6. TargetData 생성
7. Ability Commit
8. Damage GameplayEffect 적용
9. WeaponInstance의 Heat와 Spread 증가
10. GameplayCue.Weapon.Rifle.Fire 실행

## 탄 퍼짐과 크로스헤어

DCRangedWeaponInstance가 실제 탄 퍼짐을 소유한다.

동일한 CurrentSpreadAngle을 다음 두 곳에서 사용한다.

- 실제 탄도 계산
- 크로스헤어의 화면 반경 계산

기존 NormalizedSpread와 임의 Offset 배율은 새 Reticle 구조가 검증된 후 제거한다.

## 카메라 반동

초기 구현에서는 별도의 전용 카메라 반동 컴포넌트를 만들지 않는다.

Lyra의 다음 에셋을 Migrate해서 사용한다.

- /Game/Feedback/CameraShakes/CS_Weapon_Fire_Rifle
- /Game/Feedback/CameraShakes/CS_Weapon_Fire_Pistol

DreamCatcher에서 다음 GameplayCue를 만든다.

- GCN_DC_Weapon_Rifle_Fire
- GCN_DC_Weapon_Pistol_Fire

GameplayCue가 Camera Shake, 발사 사운드, Muzzle Flash를 실행한다.

기존 PendingRecoil과 UpdateCameraRecoil은 GAS 사격이 검증된 뒤 제거한다.

## Equipment 구조

DCEquipmentDefinition
- EquipmentInstance 클래스
- 장착 시 부여할 AbilitySet
- 생성할 Weapon Actor
- 부착 소켓
- 부착 Transform

DCWeaponInstance
- 무기 공통 런타임 상태
- 마지막 발사 시간
- Camera Shake
- 애니메이션 설정

DCRangedWeaponInstance
- 연사 속도
- 최대 사거리
- Heat
- 탄 퍼짐
- 퍼짐 회복
- 이동 배율
- 공중 배율
- 조준 배율
- 거리별 데미지 배율

## 기본 Gameplay Tag

### Input

- InputTag.Jump
- InputTag.Aim
- InputTag.Dodge
- InputTag.Ultimate
- InputTag.Weapon.Fire
- InputTag.Weapon.Reload

### Ability

- Ability.Action.Aim
- Ability.Action.Dodge
- Ability.Action.Ultimate
- Ability.Action.WeaponFire
- Ability.Action.Reload
- Ability.Action.Death

### State

- State.Aim.Shoulder
- State.Aim.Scope
- State.Dodging
- State.Firing
- State.Reloading
- State.Dead
- State.Attack.Intent
- State.Attack.Windup
- State.Attack.Active
- State.Attack.Recovery

### Gameplay

- Gameplay.DamageImmunity

### Event

- GameplayEvent.Death
- GameplayEvent.Dodge.Success

### Cooldown

- Cooldown.Dodge
- Cooldown.Ultimate

## 기존 시스템 교체표

| 기존 시스템 | 새 시스템 | 제거 시점 |
|---|---|---|
| UDCCombatComponent 사격 | GA_DC_RifleFire | GAS 사격 검증 후 |
| UDCCombatComponent 회피 | GA_DC_Dodge | 무적과 쿨다운 검증 후 |
| UDCCombatComponent 궁극기 | GA_DC_Ultimate | ResourceSet 연동 후 |
| FDCWeaponHandlingProfile | DCRangedWeaponInstance | 무기 데이터 이전 후 |
| Character WeaponMesh | Equipment WeaponActor | 장비 부착 검증 후 |
| Character 사격 Trace | DCGameplayAbility_RangedWeapon | TargetData 검증 후 |
| ApplyPointDamage | Damage GameplayEffect | 플레이어와 적 데미지 검증 후 |
| UDCHealthComponent 체력 저장 | DCHealthSet | HUD와 죽음 연동 후 |
| EDCAimMode | State.Aim Gameplay Tag | Camera와 AnimBP 전환 후 |
| PendingRecoil | Lyra Camera Shake | GameplayCue 검증 후 |
| NormalizedSpread | 실제 퍼짐 기반 Reticle | 크로스헤어 검증 후 |
| Enemy TryFireAtTarget | Enemy Attack Ability | 텔레그래프 검증 후 |

## Lyra 에셋 Migrate 정책

### 직접 Migrate 후보

- Camera Shake
- SoundWave
- MetaSound
- Attenuation
- Niagara
- Weapon Mesh
- Weapon Material
- Weapon Animation
- Animation Montage
- Curve
- Haptic Effect
- UI Texture
- UI Material

### Migrate 후 수정 후보

- GameplayCue Notify
- 일부 Animation Blueprint
- 일부 Reticle Widget
- 일부 Camera Curve

### 구조만 참고해 재생성

- Gameplay Ability
- AbilitySet
- Equipment Definition
- WeaponInstance Blueprint
- PawnData
- CameraMode
- Lyra Reticle Widget
- Lyra HUD Host

### 전체 복사 금지

- ShooterCore 전체
- ShooterMaps 전체
- Lyra C++ 전체
- 온라인 시스템
- 팀 및 스코어 시스템
- FrontEnd
- CommonUI 전체
- Bot 전체

## 단계별 진행 상태

- [x] 0단계: 명세와 GAS 테스트 에셋 준비
- [x] 1단계: GAS Foundation
- [x] 2단계: PawnData와 Input Tag
- [x] 3단계: Attribute, Damage, Death
- [x] 4단계: Aim과 CameraMode
- [x] 5단계: Equipment와 WeaponInstance
- [ ] 6단계: Ranged Fire와 GameplayCue
- [ ] 7단계: Reticle과 HUD
- [ ] 8단계: Dodge와 Ultimate
- [ ] 9단계: Enemy Attack Telegraph
- [ ] 10단계: Boss와 Level 1
- [ ] 11단계: Inventory와 Experience
- [ ] 12단계: Legacy 제거

## 0단계 완료 조건

- AGENTS.md가 GAS 도입을 승인된 방향으로 설명한다.
- 이 명세 문서가 저장되어 있다.
- GAS 테스트 맵이 존재한다.
- GAS 테스트 Character Blueprint가 존재한다.
- 테스트 맵이 테스트 Character Blueprint를 생성한다.
- 기존 Test_Map과 기존 플레이어 Blueprint가 수정되지 않는다.