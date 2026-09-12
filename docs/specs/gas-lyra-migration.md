# DreamCatcher GAS·Lyra 전환 명세

## 상태

- 결정 상태: 승인됨
- 이식 정책 갱신: 2026-09-10 KST, 사용자 승인
- 엔진 버전: Unreal Engine 5.8
- 기준 프로젝트: DreamCatcher
- 참고 프로젝트: Lyra Starter Game 5.8
- Lyra 로컬 경로: E:\Epic Games\UEProjects\LyraStarterGame
- 적용 방식: 이식 가능한 Lyra C++·Blueprint·데이터·시각 에셋을 최대한 원본 재사용하고, DreamCatcher 연결에 필요한 부분만 최소 수정
- 에셋 정책: Unreal Editor의 Migrate로 필요한 의존성을 함께 이전하고, 원본 레이아웃·애니메이션·머티리얼·곡선을 우선 보존
- 코드 정책: C++도 필요한 범위의 원본 이식을 우선하며, 재구현은 원본 재사용이 부적합한 이유를 확인한 경우에 한정
- 진행 방식: 사용자가 한 채팅에서 한 세부 단계씩 직접 구현. Codex는 읽기·검수·코드 및 Editor 절차 안내를 담당
- 문서 수정 권한: 이번 갱신은 사용자가 명시적으로 요청한 명세·계획 문서 수정이다. 이후 코드·Config·에셋을 직접 수정할 권한으로 확대하지 않는다
- 새 실행 계획: 이 문서의 `R0~R14`를 사용. 과거 `0~12단계`는 기존 기능 구현 이력으로 별도 보존

## 2026-09-10 확정 결정

1. 기존 1~6단계에서 직접 구현했더라도 원본 재사용·최소 수정 이식이 가능한 부분은 모두 교체 대상이다. 완료된 자체 구현을 보호하기 위한 예외는 두지 않는다.
2. `OnSpawn`은 자체 기반에 기능만 추가하지 않고, 원본 GameplayAbility·ASC와 필요한 초기화·보조 코드를 함께 이식한다.
3. 무기 UI를 현재의 Inventory 없는 구조에 맞추는 것을 최종 목표로 삼지 않는다. 원본 Inventory·Equipment·QuickBar와 아이템 연결을 먼저 이식한다.
4. 자체 최종 퍼짐 getter에 원본 Reticle만 맞추지 않는다. 원본 무기 계산·발사 소비부·Reticle·데이터를 함께 교체하여 각도·배율·화면 단위를 통일한다.
5. 회피의 기존 확정 규칙인 **무입력 전방 대시 / 이동 입력 방향 8방향 대시를 폐기**한다. 방향 선택·무입력 처리·이동 방식·몽타주·쿨다운·종료 처리는 로컬 Lyra `GA_Hero_Dash` 원본 확인 결과를 따른다.
6. Hip / Shoulder / Scope의 우클릭 입력 규칙은 유지한다. 유지하는 것은 사용자 조작 규칙이지 기존 C++·Blueprint 구현 자체가 아니다.
7. 기존 코드·에셋은 비교·회귀 검증 및 복구를 위해 임시 보존한다. 대체 검증이 끝나면 활성 경로에서 제외하고 참조 확인 후 제거한다. 새 경로와 옛 경로가 동시에 입력·데미지·Tick·연출을 처리하지 않게 한다.
8. 목표는 관련 기능과 의존성의 최대 원본 재사용이다. 전체 이식 가능성 검증이 완료됐다는 뜻은 아니며, 각 단계에서 필요한 원본이 추가 발견되면 목록에 추가한다.
9. Lyra 내용을 그대로 이식하지 못하거나 일부 수정·대체·보류·제외하려면, 해당 결정을 안내하기 전에 아래 `원본 그대로 이식하지 못하는 경우의 근거 설명 의무`에 따라 확인 가능한 근거와 대안을 제시한다. 근거 없이 불가능하다고 단정하거나 자체 구현으로 전환하지 않는다.

## 현재 구현 이력과 재개 지점

아래 완료 기록은 기존 사용자 보고 및 소스 점검에 따른 이력이다. 이번 문서 수정으로 빌드·PIE를 다시 검증한 것은 아니다.

| 항목 | 기존 작업 상태 | 이번 원본 이식에서의 취급 |
|---|---|---|
| 기존 0~6단계 | 기능 구현 완료 이력 | 원본 대체 검증은 별도. R1~R6에서 재검토·교체 |
| 기존 7-1 실제 퍼짐 기반 Reticle | 사용자 구현 완료 | 비교용 유지 후 R6~R7 원본으로 대체 |
| 기존 7-A Linked Anim Layer | Idle 연결까지만 완료 | 전체 완료 아님. R8에서 원본 계층으로 교체 |
| Lyra 크로스헤어 시각 에셋 | Migrate 진행됨 | 원본 Widget 및 UI C++ 기능 연동 완료와 구분 |
| 원본 UI C++·Reticle Widget 이식 | 중단 상태, 완료 아님 | R5~R6 의존성 준비 후 R7에서 재개 |
| `DCAbilityTagRelationshipMapping.h/.cpp` | 직전 채팅에서 절차만 안내. 2026-09-10 점검 시 대상 파일 없음 | 다음 구현 작업 R1-1. 다른 채팅에서 진행했는지 시작 전 재확인 |

다음 구현 채팅은 **R1-1 — Ability Tag Relationship Mapping 원본 두 파일 이식**부터 시작한다.
정책·계획 기록인 R0와 일부 C++ 대조는 수행했지만, 모든 원본 Blueprint 그래프와 런타임 감사가 끝난 것은 아니다.

## 목표

DreamCatcher의 기존 전투 구조를 Gameplay Ability System 기반으로 단계적으로 교체한다.

다음 시스템은 Lyra의 원본 코드와 에셋을 우선 검토하여 재사용·이식한다.
단순히 비슷한 기능을 새로 만드는 것이 아니라, 원본 동작을 보존하면서 DreamCatcher에 통합하는 것을 목표로 한다.

- Ability System Component
- Gameplay Ability
- Gameplay Effect
- AttributeSet
- Gameplay Tag
- AbilitySet
- PawnData
- PawnExtension / HeroComponent 초기화
- Input Tag
- Ability Tag Relationship Mapping
- AbilityCost / GameplayEffectContext / AbilitySystemGlobals
- Equipment
- Inventory / QuickBar / GameplayTagStack / Item Fragment
- WeaponInstance
- RangedWeaponInstance
- GameplayCue
- CameraMode
- 실제 탄 퍼짐 기반 Reticle
- HUD Host / UI 전용 C++ / Slate / 메시지 연결
- AnimInstance / CharacterMovement / AnimBP / Linked Anim Layer
- 발사·재장전·Dash Ability 및 연출 에셋
- 필요한 Experience·GameFeature·공통 모듈 의존성

## 핵심 원칙

- 기존 시스템은 이식한 대체 구현이 승인된 목표 동작을 만족하는지 검증하기 전까지 삭제하지 않는다. 폐기한 회피 규칙까지 동일하게 유지할 필요는 없다.
- 기존 플레이 가능한 맵과 Character Blueprint를 직접 실험 대상으로 사용하지 않는다.
- GAS 테스트 맵과 테스트 Character Blueprint에서 먼저 검증한다.
- C++는 규칙, 상태, 판정, 수명 관리를 담당한다.
- Blueprint는 애니메이션, VFX, SFX, Camera Shake, UI 연출을 담당한다.
- 위 역할 구분을 이유로 원본 Ability Blueprint의 발사·재장전·Dash 흐름을 C++로 다시 작성하지 않는다. 원본의 C++/Blueprint 책임 분할을 우선 보존한다. UMG 안에 데미지·보상 같은 게임 규칙을 새로 넣지는 않는다.
- 적용 우선순위는 **원본 그대로 재사용 → 최소 수정 이식 → 불가피한 부분만 직접 구현**이다.
- 이 원칙은 크로스헤어에만 한정하지 않고 GAS·Equipment·Camera·Animation·UI 등 전환 대상 전반에 적용한다.
- 기존의 “Lyra C++ 코드는 그대로 복사하지 않는다”는 직접 재구현 우선 원칙은 폐기한다. 필요한 범위의 원본 코드를 가져오고 검증된 계산·표시·상태 처리 로직을 최대한 유지한다.
- C++ 이식은 에셋 Migrate와 별도 작업이다. 모듈·플러그인·include·API 매크로·Reflection 타입·호출 대상의 의존성을 함께 확인한다.
- Lyra Blueprint는 부모 클래스와 내부 C++ 위젯·노드·구조체·태그·메시지 의존성을 확인한 뒤 원본 에셋을 우선 Migrate한다.
- 프로젝트 연결을 위해 클래스·함수·프로퍼티·패키지 참조를 바꿔야 하면 필요한 범위만 변경하고, 기존 직렬화 데이터와 애니메이션 바인딩을 보존한다.
- 재구현이 필요하면 원본 이식이 부적합한 이유, 대체할 범위, 원본과 달라지는 동작을 먼저 설명한다. 독립성을 이유로 곧바로 재작성하지 않는다.
- 사용자가 확정한 DreamCatcher 규칙(예: Hip / Shoulder / Scope 입력)은 유지한다. Lyra의 기본 게임 규칙까지 자동으로 대체하는 것은 아니다.
- 기존 구현도 원본 대체 대상에 포함한다. 원본을 기존 자체 API·계산·수명 구조에 맞추기 위해 다시 축소 구현하지 않는다. 과도기 연결 코드는 필요성과 제거 시점을 기록한다.
- 원본 코드·에셋의 저작권 및 라이선스 표기를 유지하고, 적용되는 배포 조건을 확인한다.
- `.uasset`과 `.umap`은 Unreal Editor 밖에서 복사하거나 수정하지 않는다.

## 원본 확인과 변경 검증

- 작업 전에 실제 Lyra 파일·클래스·함수·에셋 경로와 엔진 버전을 확인한다. 문서 설명이나 기억만으로 구현을 추정하지 않는다.
- C++ 기본값, Blueprint에서 덮어쓴 값, 실제 런타임 값을 구분한다. 곡선의 키 좌표·보간 방식·단위도 확인 없이 임의로 정하지 않는다.
- 바이너리 에셋의 이름·참조 문자열만 확인한 상태를 Blueprint 그래프 연결이나 실행 동작까지 검증한 것으로 보고하지 않는다.
- 원본 유지 부분, 프로젝트 통합 때문에 수정한 부분, 아직 확인하지 못한 부분을 구분해 보고한다.
- 기존 구현과 Lyra의 차이를 발견하면 먼저 원본 동작과 설정을 확인한다. 원본에 없는 추가 시스템을 필요한 기능으로 단정하지 않는다.
- 이식 후에는 빌드, 에셋 로드·Compile, 참조 유효성, 런타임 기능·연출을 각각 검증한다. 파일 복사나 빌드 성공만으로 완료 처리하지 않는다.
- 정책 변경만으로 기능 이식 완료를 선언하지 않는다. 과거 단계 번호와 이번 승인된 R 단계 번호를 구분한다.
- 원본 코드가 상호 의존하면 빌드 가능한 통합 묶음으로 이식한다. 뒤 단계에 적힌 의존성을 앞당기는 것은 가능하지만, 이를 이유로 원본 기능을 임의 삭제하거나 빈 구현으로 완료 처리하지 않는다.
- 공식 문서와 설치된 버전의 코드가 다르면 차이를 기록한다. 원본에도 결함이 있을 수 있으므로 최소 수정의 근거와 재현·검증 결과를 남긴다.

## 원본 그대로 이식하지 못하는 경우의 근거 설명 의무

이 규칙은 R1~R14 전체의 C++·Blueprint·데이터·시각 에셋·모듈/플러그인에 적용한다.
원본을 수정하거나 자체 구현으로 대체하는 경우뿐 아니라, 일부 기능을 생략·보류·제외하는 경우에도 적용한다.
사용자가 직접 작업하기 전에 판단할 수 있도록 해당 단계의 채팅에서 먼저 설명하고, 단계 인계에도 남긴다.

### 반드시 설명할 내용

1. **대상과 원본 역할:** 이식하려는 원본의 정확한 파일·클래스·함수 또는 에셋 경로·그래프·설정 항목과 그 기능을 명시한다.
2. **확인한 근거:** 실제 소스 위치와 관련 코드, 확인한 Blueprint 연결/기본값, 빌드·로드·런타임 오류 및 재현 조건 중 해당하는 근거를 제시한다. 관련 공식 문서가 있으면 URL과 해당 항목도 함께 제시한다. 실행하지 않은 테스트나 보지 못한 그래프는 근거로 주장하지 않는다.
3. **막히는 이유와 영향:** 원본이 요구하는 의존성·타입·수명·태그·데이터와 현재 프로젝트의 차이를 연결해 설명한다. 그대로 가져오면 어떤 컴파일 오류·참조 누락·동작 불일치가 발생하는지, 코드 분석상 예상인지 실제 재현 결과인지 구분한다.
4. **제약의 종류:** 기술적/버전 제약, 선행 의존성 미준비, 사용자 규칙과의 충돌, 범위·비용에 따른 보류, 단순 미검증을 구분한다. 라이선스 제약을 이유로 들면 적용되는 공식 조건과 출처·대상 범위를 명시한다.
5. **원본을 살리는 대안:** 필요한 의존성을 함께 이식하거나 앞당기는 방법, 참조·이름·연결부만 최소 변경하는 방법을 먼저 검토한다. 이 방법으로 해결되지 않는 이유까지 확인한 경우에만 해당 부분의 재구현·제외를 제안한다.
6. **변경 범위와 동작 차이:** 원본 그대로 유지할 부분, 수정/대체/누락할 부분, 결과적으로 달라지는 기능·수치·데이터 계약과 비교 검증 방법을 명시한다. 일정·비용 추정에는 근거와 불확실성을 표시한다.
7. **결정 및 재개 조건:** 권장안, 남은 미검증 항목, 추가 확인 방법, 보류 시 재개 조건을 남긴다. 사용자 요구 동작이나 승인 범위를 바꾸는 대안은 사용자 확인 전 확정·구현하지 않는다.

### 판단 시 금지할 것

- “Lyra 전용이다”, “복잡하다”, “의존성이 많다”, “현재 구조와 다르다”는 설명만으로 이식 불가·제외·재구현을 결정하지 않는다. 구체적인 의존성 경로와 해결 가능성을 제시한다.
- 아직 Inventory나 부모 클래스가 없다는 사실을 영구적인 이식 불가능 사유로 취급하지 않는다. 선행 작업으로 해결 가능한지 먼저 확인한다.
- 확인 도구나 자료가 없어 검증하지 못한 상태는 **미검증/판단 보류**로 표시한다. 원본에 기능이 없거나 이식할 수 없다고 단정하지 않는다. 필요한 소스·그래프·로그·Editor 확인 항목을 요청한다.
- 검색에서 찾지 못했다는 사실만으로 기능 부재를 확정하지 않는다. 조사한 원본 범위와 아직 확인하지 못한 범위를 밝힌다.
- 기존 자체 구현을 유지하는 편이 편리하다는 이유로 원본 재사용 가능성을 배제하지 않는다.

### 보고 양식

```text
작업 ID / 대상 원본:
원본 역할 / 그대로 가져오려던 범위:
확인 근거: 파일·함수 또는 그래프/설정·로그·공식 문서
확인 수준: 소스 분석 / 그래프 확인 / 실제 재현 / 미검증
그대로 이식할 때의 제약과 예상 또는 재현된 영향:
분류: 기술 제약 / 선행 의존성 / 사용자 규칙 / 범위·비용 / 미검증
검토한 원본 보존 대안과 각 대안의 제약:
권장안 / 원본 유지 부분 / 최소 수정·대체·제외 부분:
원본과 달라지는 동작 / 비교 검증 기준:
사용자 확인이 필요한 결정 / 보류 항목의 재개 조건:
```

단순한 모듈·클래스·패키지 이름 변경은 원본 → 대상 변환표와 변경 이유, 동작 변경 유무로 간결하게 설명할 수 있다.
이 경우에도 기능 삭제나 계산 변경을 이름 변경에 섞지 않는다.

## 플레이어 ASC 소유 구조

플레이어의 Ability System Component는 PlayerState가 소유한다.

아래 DC 명칭은 현재 프로젝트 대응 이름이다. 최종 클래스·프로퍼티 이름은 원본 이식 단계의 참조 변환표로 확정하며, 새 자체 클래스를 다시 설계하라는 의미가 아니다.

ADCPlayerState

- UDCAbilitySystemComponent
- UDCHealthSet
- UDCCombatSet
- UDCResourceSet

ADreamCatcherCharacter

- PlayerState ASC의 Avatar
- 이동과 카메라의 실제 Pawn
- DCPawnExtensionComponent
- 원본 HeroComponent에 대응하는 초기화·입력·Ability Camera 연결
- DCHealthComponent
- DCEquipmentManagerComponent
- DCCameraComponent

일반 적과 보스는 Pawn 교체가 필요하지 않으므로 Character가 ASC를 직접 소유할 수 있다.
이는 DreamCatcher 적용 후보이며, Lyra 봇의 소유 구조와 같다고 단정하지 않는다. R10에서 기존 적 수명과 원본 의존성을 확인하고 확정한다.

## 입력 구조

이동·시점의 Native Input과 Ability Input Tag 전달은 원본 InputConfig·InputComponent·HeroComponent·PlayerController의 호출 관계를 이식한다.
기존 Input Action과 키 배치는 필요하면 재사용하되, 원본 AbilitySet이 사용하는 태그와 정확히 연결한다.

- 원본 Input Tag, Ability Asset Tag, 실행 중 소유 태그, Gameplay Event, UI 메시지 태그를 서로 다른 용도로 취급한다.
- 기존 `InputTag.Weapon.Fire`, `InputTag.Aim`, `InputTag.Dodge`는 현재 구현 식별자이며 원본에 강제할 최종 태그가 아니다.
- 이식 시 기존 태그 → 원본 태그 또는 유지할 프로젝트 태그의 매핑과 InputConfig·AbilitySet·BP·AnimBP·UI의 소비처를 함께 기록한다.
- Press / Release / Canceled를 검증하고, 기존 Character 입력과 원본 Hero 입력이 중복 바인딩되지 않게 한다.

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

현재 조준 상태는 다음 Gameplay Tag로 표현한다.

- State.Aim.Shoulder
- State.Aim.Scope

Hip은 두 태그가 모두 없는 상태다.
원본 ADS 카메라·애니메이션·태그에 연결할 때 사용자 입력 규칙은 보존한다. 태그 이름이나 상태 구현을 바꾸면 관련 소비처를 함께 이전한다.

## 회피 원본 적용과 방어 효과의 구분

- 원본 대상: `Plugins/GameFeatures/ShooterCore/Content/Game/Dash/GA_Hero_Dash.uasset` 및 실제로 참조하는 부모·효과·몽타주·Cue·UI·입력 데이터.
- 경로 존재 확인과 그래프/기본값/PIE 확인을 구분한다. 무입력 처리, 방향 계산, Root Motion, 쿨다운 시작, 취소·종료 시점은 그래프와 실행 결과를 확인해 기록한다.
- 기존 LaunchCharacter 대시와 8방향 분기 코드를 보존하기 위해 원본 Dash를 바꾸지 않는다.
- 회피 무적/데미지 차단과 성공 피드백은 별도 작업이다. Lyra Dash가 이를 제공한다고 가정하지 않는다.
- 먼저 원본 Dash 동작을 검증하고, 원본에 방어 효과가 없다면 R9-2에서 DreamCatcher 추가 규칙으로 구분해 구현한다. 기존 방향 규칙을 다시 도입하지 않는다.

## 발사 구조

자체 `GA_DC_RifleFire`의 고정된 10단계 처리 순서를 목표로 유지하지 않는다.
다음 원본을 함께 이식한다.

- `LyraGameplayAbility_FromEquipment`: Ability SourceObject에서 장비를 얻고 장비의 Instigator에서 Inventory 아이템을 얻는 경로.
- `LyraGameplayAbility_RangedWeapon`: 타기팅, 탄도 Trace, TargetData 처리, Commit, 퍼짐 증가, Blueprint 콜백.
- 원본 발사 Ability Blueprint 계층과 라이플 자식: 발사 간격, Montage, Damage GE 및 GameplayCue 연결.
- `LyraWeaponStateComponent`와 원본 TargetData 타입: 무기 갱신 및 명중 표시 데이터 경로.
- 원본 AbilityCost와 Inventory 태그 스택: 탄약 비용 및 관련 상태.

발사 시각 갱신, 비용 Commit, 데미지, Heat 증가, Cue의 정확한 실행 위치·순서는 실제 원본 C++와 BP 그래프를 근거로 안내한다.
원본의 카메라 타기팅과 현재의 총구 가림 검사는 같은 구현이라고 가정하지 않는다. 벽·엄폐·근거리 테스트로 차이를 기록하고, 별도 규칙 추가가 필요하면 이유를 설명한다.
네트워크 확인 메시지가 존재한다는 이유로 완전한 서버 재판정·부정행위 방어가 구현됐다고 보고하지 않는다.

## 탄 퍼짐과 크로스헤어

원본 `LyraRangedWeaponInstance`의 Heat·Spread 곡선, 상태별 배율, 첫발 정확도와 회복 처리를 이식한다.
원본 무기 계산·발사 소비부·Reticle·무기 데이터는 한 계약으로 교체한다.

- 원본의 `GetCalculatedSpreadAngle()`과 `GetCalculatedSpreadAngleMultiplier()`를 함께 사용하는 계약을 기준으로 한다.
- 최종 퍼짐은 기본 퍼짐각 × 유효 배율이다. 첫발 정확도 적용도 원본 무기 규칙에서 처리한다.
- 현재 DC `GetCurrentSpreadAngle()`은 이미 최종값이다. 이를 원본 기본 각도로 연결하고 배율을 다시 곱하지 않는다.
- 전체 원뿔 각도와 반각, 도와 라디안, 화면 픽셀과 UMG/Slate 좌표를 구분한다. 원본 데이터 계약을 기존 getter에 맞추기 위해 바꾸지 않는다.
- 상태 배율이 참조하는 원본 CameraMode 태그·블렌드 정보까지 연결한다. 임의의 Shoulder/Scope 배율을 원본 값이라고 소개하지 않는다.
- 무기 Tick 소유처는 원본 경로로 통일한다. 기존 Equipment Tick과 WeaponState Tick이 중복으로 Heat를 회복시키지 않게 한다.
- 기존 NormalizedSpread, 임의 Offset 배율, 자체 Heat·첫발 정확도 구현은 대체 검증 후 제거한다.

### Reticle 원본 이식 범위

크로스헤어는 Lyra의 원본 시각 에셋·Widget Blueprint·UI 전용 C++를 최대한 보존하여 이식한다.
기존 DreamCatcher의 단순 이미지 기반 Reticle은 대체 검증이 끝날 때까지 유지한다.

원본 재사용 대상:

- `W_Reticle_Rifle`, `W_Reticle_Pistol`, `W_Reticle_Shotgun`의 레이아웃과 UMG 애니메이션
- `AimDownSights`, `Elimination` 등 원본 위젯 내부의 연출과 바인딩
- `M_UI_Base_ReticleBuilder`, `MI_UI_Reticles_*` 및 종속 머티리얼 함수·Curve·Curve Atlas·Texture
- `LyraReticleWidgetBase`의 반경 계산, `CircumferenceMarkerWidget` / `SCircumferenceMarkerWidget`의 배치·그리기 로직
- `HitMarkerConfirmationWidget` / `SHitMarkerConfirmationWidget`의 명중 표시·페이드 로직
- `W_WeaponReticleHost`, `W_Reticle_AmmoBar`, `W_Temp_ReticleDot` 등 관련 위젯도 의존성을 검토하여 이전
- `LyraWeaponUserInterface`, `InventoryFragment_ReticleConfig` 및 Inventory 아이템에서 Reticle을 선택하는 원본 연결

목록에 있는 이름·참조는 그래프 검증 완료를 뜻하지 않는다. 위젯별 생성·애니메이션 재생·메시지 구독·해제는 R7에서 직접 확인한다.

DreamCatcher에 맞춰 최소 수정할 연결:

- 원본 Inventory·Equipment·QuickBar와 무기 조회를 연결하고 위젯 생성·교체·해제를 검증
- 기존 조준 태그 및 Scope 표시 정책과 원본 조준 애니메이션의 연결
- 실제 명중·데미지·약점·처치 데이터 전달
- `LyraWeaponStateComponent`의 데이터 공급 및 팀·네트워크 의존성 처리
- Lyra 전용 부모·구조체·메시지·패키지 참조와 필요한 Core Redirects

Inventory가 없다는 이유로 원본 Host의 Instigator 조건만 제거하거나 아무 객체를 넣어 우회하지 않는다.
원본 Inventory 연결이 선행되도록 R5를 먼저 수행한다. 예외적인 임시 연결이 필요하면 최종 구조와 구분하고 제거 단계를 기록한다.

호환 클래스와 참조 변환을 준비하기 전에, 부모나 내부 위젯 타입이 누락된 원본 Widget Blueprint를 열어 저장하지 않는다.
탄약·약점·처치 신호가 미구현이면 관련 UI 에셋의 이전과 기능 연동 완료를 구분한다.
좌표·DPI 보정, 이미지 경계 배치, 조준·명중·처치 애니메이션을 원본과 비교 검증한다.
첫발 정확도·퍼짐 회복은 무기 규칙이며, 별도 UI 펄스 연출의 존재 여부와 혼동하지 않는다.

## 카메라 반동

초기 구현에서는 별도의 전용 카메라 반동 컴포넌트를 만들지 않는다.

Lyra의 다음 에셋을 Migrate해서 사용한다.

- /Game/Feedback/CameraShakes/CS_Weapon_Fire_Rifle
- /Game/Feedback/CameraShakes/CS_Weapon_Fire_Pistol

발사 GameplayCue도 원본을 이식한다. 현재 자체 `GCN_DC_Weapon_Rifle_Fire`는 교체 대상이다.
라이플은 `Plugins/GameFeatures/ShooterCore/Content/Weapons/Rifle/GCN_Weapon_Rifle_Fire.uasset`를 시작점으로 실제 부모·카메라 쉐이크·사운드/MetaSound·Niagara·무기 Actor·함수/매크로 의존성을 확인한다.
피스톨 등 추가 무기는 해당 단계에서 실제 원본 경로와 참조를 확인한다.

GameplayCue가 Camera Shake, 발사 사운드, Muzzle Flash 등을 실행하는 원본 설정을 보존한다.
에셋만 복사하고 Cue의 대상·로컬 실행·파라미터·중복 재생을 확인하지 않은 상태를 반동 이식 완료로 처리하지 않는다.

기존 PendingRecoil과 UpdateCameraRecoil은 GAS 사격이 검증된 뒤 제거한다.

## Equipment 구조

원본 Inventory와 Equipment의 역할 및 수명을 함께 이식한다.

1. Inventory Item Definition / Fragment가 아이템 구성·초기 스탯·장착 정의·Reticle 설정을 제공한다.
2. Inventory Item Instance가 소유 아이템의 런타임 태그 스택 등을 보관한다.
3. QuickBar가 활성 슬롯의 아이템을 Equipment로 장착하고 아이템을 Instigator로 연결한다.
4. Equipment Definition / Manager / Instance가 장착 수명, AbilitySet 부여·회수, Actor 생성·부착을 처리한다.
5. WeaponInstance / RangedWeaponInstance가 원본의 무기 런타임 데이터와 계산을 제공한다.
6. 발사·재장전 Ability, 원본 UI가 같은 장비·아이템 데이터를 사용한다.

원본 `LyraEquipmentDefinition`은 UObject 기반 클래스이며, 이를 자체 PrimaryDataAsset 구조로 바꾸는 것을 전제하지 않는다.
연사 간격·Camera Shake·애니메이션 설정의 소유 위치를 기존 DC 클래스에 강제로 맞추지 않는다. 실제 원본 C++·Ability BP·Cue·무기 BP의 프로퍼티 위치를 유지한다.
라이플 하나로 먼저 통합 검증하고, 같은 원본 구조로 필요한 추가 무기를 확장한다.
여기서 필요한 최소 Inventory는 승인된 선행 의존성이다. 대형 인벤토리 UI·아이템 경제·저장은 별도 후속 범위다.

## 기존 Gameplay Tag 목록 — 이식 시 매핑 대상

아래는 기존 명세의 DC 태그 목록이며 존재·사용 여부를 각 단계에서 다시 확인한다.
원본 Ability·AnimBP·UI를 이 목록에 맞춰 일괄 수정하라는 목표가 아니다. 원본 태그를 우선 유지하고, 사용자 조작·프로젝트 규칙에 필요한 차이만 명시적으로 연결한다.
특히 `State.Dodging`과 `Cooldown.Dodge`를 Lyra Dash의 원본 태그라고 단정하지 않는다.

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

기존 GAS 재구현도 Legacy 교체 대상에 포함한다. 제거는 아래 검증을 마친 기능별로 수행하고 R14에서 잔여 참조를 종합 점검한다.

| 기존 시스템 | 새 시스템 | 제거 시점 |
|---|---|---|
| 자체 DCGameplayAbility / ASC / AbilitySet | Lyra 원본 기반 클래스 및 보조 타입 | OnSpawn·입력·취소·부여/회수 검증 후 |
| 자체 PawnExtension / 입력 초기화 | 원본 PawnExtension·Hero·Input 연결 | 초기화·Pawn 교체·입력 중복 검증 후 |
| 자체 CameraMode / 충돌 처리 | 원본 카메라 스택·ThirdPerson | 벽·카메라 전환·조준 규칙 검증 후 |
| 자체 DCEquipment / 기본 무기 직접 장착 | 원본 Inventory·QuickBar·Equipment | 아이템 연결·장착 수명·Ability 회수 검증 후 |
| 자체 DCRangedWeaponInstance / Ranged Ability | 원본 무기 계산·발사 C++·Ability BP | 퍼짐·탄약·발사·데미지 검증 후 |
| 자체 DC Reticle / Idle 전용 Linked Layer | 원본 Reticle / AnimBP·Linked Layer | R7·R8 대응 기능 검증 후 |
| UDCCombatComponent 사격 | 원본 발사 Ability 이식본 | R6 검증 후 |
| UDCCombatComponent 회피 / 자체 LaunchCharacter 대시 | 원본 GA_Hero_Dash 이식본 | R9-1 원본 동작 검증 후. 방어 효과는 R9-2에서 별도 검증 |
| UDCCombatComponent 궁극기 | GA_DC_Ultimate | ResourceSet 연동 후 |
| FDCWeaponHandlingProfile | 원본 무기·Ability·Cue 데이터 | 데이터 소유처 및 동작 검증 후 |
| Character WeaponMesh | Equipment WeaponActor | 장비 부착 검증 후 |
| Character 사격 Trace | 원본 RangedWeapon Ability 이식본 | TargetData·엄폐 동작 검증 후 |
| ApplyPointDamage | Damage GameplayEffect | 플레이어와 적 데미지 검증 후 |
| 자체 DCHealthSet·DamageExecution·HealthComponent 체력 저장 | 원본 Attribute·Execution·Health 연결 | 플레이어·적·HUD·사망 검증 후 |
| EDCAimMode | State.Aim Gameplay Tag | Camera와 AnimBP 전환 후 |
| PendingRecoil | Lyra Camera Shake | GameplayCue 검증 후 |
| NormalizedSpread | 실제 퍼짐 기반 Reticle | 크로스헤어 검증 후 |
| Enemy TryFireAtTarget | Enemy Attack Ability | 텔레그래프 검증 후 |

## Lyra 코드·에셋 이식 정책

### 선택 기준

아래 목록은 고정된 복사 가능·불가능 분류가 아니라 우선 검토할 대상이다.
이미 직접 구현한 기능이 있더라도 원본 재사용·최소 수정 이식이 가능하면 교체를 기본 방향으로 한다.
특정 클래스가 Lyra 전용이라는 사실만으로 이식 대상에서 제외하지 않는다.

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

### 원본 이식 후 최소 수정 후보

- GameplayCue Notify
- Animation Blueprint, Linked Anim Layer, Animation Layer Interface
- Reticle Widget, HUD Host 및 관련 UI 전용 C++·Slate 구현
- Gameplay Ability, AbilitySet, Equipment Definition, WeaponInstance Blueprint, PawnData
- CameraMode, Camera Curve
- 위 에셋의 부모·데이터 공급·수명 관리를 담당하는 C++ 클래스

### 직접 구현이 필요한 경우

아래 사유는 자동으로 재구현을 허용하는 면제 항목이 아니다. 위 근거 설명 의무를 충족하고 원본 보존 대안을 먼저 검토한다.

- Lyra에 대응 기능이 없거나, 사용자가 확정한 DreamCatcher 규칙과 다른 부분
- 의존성 검토 결과 원본 이식에 작업 범위를 크게 벗어나는 변경이 필요하고, 작은 연결 코드로 해결할 수 없는 부분
- 원본의 결함·버전 비호환 등 수정 이유가 실제 확인된 부분

직접 구현은 해당 부분에 한정한다. 예를 들어 명중 데이터 공급처가 다르다는 이유로,
재사용 가능한 히트마커 그리기·페이드 코드까지 다시 작성하지 않는다.

### 범위 제한

- 최대한 이식한다는 원칙은 의존성 검토 없이 Lyra C++·ShooterCore·ShooterMaps 전체를 복사한다는 뜻이 아니다.
- 현재 기능과 관계없는 FrontEnd·온라인·팀·스코어·Bot 시스템까지 자동으로 가져오지 않는다. 다만 Damage·명중 UI 등에서 실제로 필요한 팀 판정 의존성을 검토 없이 삭제하지 않는다.
- 필요한 엔진·플러그인 모듈(CommonUI 등)은 의존성을 확인한 뒤 사용할 수 있다. 원본을 재사용하려고 필요한 작은 의존성과 전체 프레임워크 도입을 구분한다.
- 필요한 의존성이 현재 작업 범위를 크게 확장하면 이유와 대안을 제시하고 사용자와 범위를 확정한다.
- 불필요한 전체 이름 변경·폴더 이동·정리 리팩터링은 이식 작업에 섞지 않는다.

### 이식 작업 순서

1. 원본 코드·Blueprint 그래프·데이터와 종속 코드·에셋을 확인한다.
2. 원본 유지 범위와 최소 수정 범위, 검증 기준을 정한다. 그대로 이식하지 못하는 부분은 근거·원인·원본 보존 대안·동작 차이를 먼저 설명하고 필요한 사용자 결정을 받는다.
3. 독립적인 시각·데이터 에셋은 Editor의 Migrate로 필요한 의존성과 함께 이전한다.
4. C++ 의존 에셋은 대상 클래스·모듈·참조 변환을 먼저 준비하고 원본 에셋을 이전한다.
5. DreamCatcher의 입력·상태·데이터 공급 연결만 필요한 만큼 수정한다.
6. 원본과 동일 조건에서 비교하고, 의도적인 차이와 미검증 항목을 기록한다.
7. 대체 기능이 검증된 뒤 기존 구현을 정리한다.

## 과거 단계별 진행 상태 — 기존 기능 구현 이력

이 표의 체크는 원본 이식 완료를 의미하지 않는다. 이후 작업 순서와 새 완료 판정은 아래 R 계획을 사용한다.

- [x] 0단계: 명세와 GAS 테스트 에셋 준비
- [x] 1단계: GAS Foundation
- [x] 2단계: PawnData와 Input Tag
- [x] 3단계: Attribute, Damage, Death
- [x] 4단계: Aim과 CameraMode
- [x] 5단계: Equipment와 WeaponInstance
- [x] 6단계: Ranged Fire와 GameplayCue
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

## 새 실행 계획 — R0~R14

2026-09-10 사용자 요청에 따라 원본 최대 재사용을 기준으로 재편한 실행 계획이다.
기존 1~6단계의 재구현도 포함하며, 기존 7-1 이후 작업만 이어 붙이는 계획이 아니다.
`R`은 과거 단계와 구분하기 위한 새 작업 식별자다. 각 `R번호-세부번호`를 사이드 채팅의 작업 단위로 사용한다.

### 의존성과 완료 판정 규칙

- 원본 소스·에셋 → 이식 대상 → 필요한 의존성 → 변경 이유 → 검증 방법을 해당 단계 시작 시 작성한다.
- 원본 그대로 이식하지 못하는 항목마다 위 근거 설명 의무를 적용한다. 단계의 권장안과 인계 기록에 설명이 없는 수정·대체·보류·제외 항목을 남기지 않는다.
- 아래 순서는 기능 검증 순서다. 파일을 반드시 번호 순으로만 복사한다는 뜻이 아니다.
- GameplayAbility·ASC·Hero·PawnExtension·PlayerState·Camera·AnimInstance 등 상호 참조는 실제 호출 관계를 따라 함께 빌드 가능한 묶음으로 준비한다.
- 예를 들어 원본 GameplayAbility의 카메라 API 때문에 Hero/Camera가 필요하면 R2 통합 묶음에 해당 원본 C++를 앞당긴다. R4는 해당 카메라의 에셋·조작 연결 및 검증 단계가 된다.
- Health 타입, Inventory 비용, 발사 AbilitySet의 재장전 Ability, 발사 Montage 등도 같은 방식으로 앞당길 수 있다. 세부 범위와 이유를 기록하고, 뒷단계 기능까지 완료했다고 표시하지 않는다.
- 필요한 의존성의 범위가 예상보다 커지면 원본 이식과 최소 연결 수정의 구체적 비용을 제시한다. 자동으로 대형 프레임워크를 도입하거나 자체 구현으로 회귀하지 않는다.
- 한 세부 단계는 가능하면 빌드 가능한 단위로 끝낸다. 강하게 연결된 파일은 함께 안내하며, 빈 함수·누락 부모·기능 삭제로 컴파일만 통과한 상태를 완료로 취급하지 않는다.
- 각 대체 기능이 검증되면 해당 옛 경로를 비활성화하고 참조를 정리한다. R14는 모든 삭제를 미루는 단계가 아니라 잔여 Legacy의 최종 감사다.

### 전체 순서 및 현재 상태

| ID | 작업 | 완료 시 확인할 결과 | 현재 상태 |
|---|---|---|---|
| R0 | 정책·현재 상태·작업 단위 기록 | 원본 교체 범위와 다음 작업이 문서로 고정 | 문서 갱신 완료, 전체 원본 감사는 아님 |
| R1 | GAS 보조 타입 및 의존성 준비 | 원본 기반 클래스 이식에 필요한 타입·모듈 목록과 독립 파일 준비 | 미착수. R1-1 절차만 안내됨 |
| R2 | GAS·Pawn 초기화·입력 공통 기반 교체 | 원본 ASC/Ability/AbilitySet, OnSpawn, 초기화·입력 동작 | 미착수 |
| R3 | 체력·데미지·사망 교체 | 원본 Attribute/Context/Execution/Health/Death 경로 동작 | 미착수 |
| R4 | 원본 카메라·조준 연결 | 원본 카메라와 확정 우클릭 규칙의 통합 | 미착수 |
| R5 | Inventory·QuickBar·Equipment·라이플 데이터 | 실제 Inventory 아이템에서 장비와 UI 데이터가 연결 | 미착수 |
| R6 | 원본 퍼짐·발사·GameplayCue·반동 | 원본 무기 계산·탄약 비용·발사·연출 동작 | 미착수 |
| R7 | 원본 Reticle·HUD·명중 표시 | 원본 Widget/C++/Slate가 같은 무기·아이템을 표시 | 기존 자체 기능·시각 에셋은 있음. 원본 통합 미완료 |
| R8 | 전체 AnimBP·Linked Layer·재장전 | Idle 이외 이동·조준·발사·재장전·무기 계층 연결 | 기존 Idle 연결만 있음. 원본 통합 미완료 |
| R9 | Lyra Dash·회피 방어·궁극기 | 원본 Dash 검증 후 방어 효과와 프로젝트 궁극기 분리 연동 | 미착수 |
| R10 | 적 GAS·공격 텔레그래프 | 공격 의도부터 후딜까지 중단 가능한 전투 흐름 | 미착수 |
| R11 | Level 1 빈 흐름 완주 | 전투 1 → 전투 2 → 보스 Placeholder → 결과 Placeholder | 새 전투 경로 통합 미검증 |
| R12 | 최소 보스 | 패턴 2개·페이즈 전환·HUD·사망 완료 신호 | 미착수 |
| R13 | Experience·필요 무기/공통 기능 후속 이식 | 실제 필요한 나머지 원본 기능의 통합 | 선행 의존성 외 미착수 |
| R14 | Legacy·참조·회귀 최종 정리 | 새 구조만으로 시작부터 결과까지 동작 | 미착수 |

### R0 — 기준 기록

- 원본 재사용 정책, 세 가지 명시적 교체 결정, Dash 규칙 폐기를 이 명세에 반영한다.
- 기존 기능 완료와 원본 대체 완료를 분리한다.
- 각 작업 시작 시 브랜치·dirty 파일·엔진 버전·원본 위치·이전 채팅 인계 기록을 재확인한다.
- 이번 문서 갱신은 R1 이후의 구현·빌드·PIE 완료를 의미하지 않는다.

### R1 — GAS 보조 타입과 의존성 준비

**세부 작업**

- R1-1: `LyraAbilityTagRelationshipMapping.h/.cpp`를 복사해 필요한 타입·파일 이름만 변경한다. 기존 안내의 목적지는 `Source/DreamCatcher/AbilitySystem/DCAbilityTagRelationshipMapping.h/.cpp`이며 현재 생성 여부부터 재확인한다.
- R1-2: 원본 AbilitySourceInterface·GameplayEffectContext·AbilitySystemGlobals 및 PhysicalMaterialWithTags·TargetData 관련 의존성을 확인하고, 독립적으로 이식 가능한 묶음을 준비한다. Context 할당 Config와 기존 호출부 전환은 활성화 전에 함께 검증한다.
- R1-3: AbilityCost·실패 메시지·Gameplay Tag·로그 및 필요한 모듈/플러그인 의존성을 확인한다. `LyraAbilityCost` 기본 클래스는 로컬 원본에서 헤더에 구현되어 있으므로 없는 CPP를 만들거나 복사 대상으로 제시하지 않는다.
- R1-4: R2의 ASC·GameplayAbility 상호 의존성 표를 확정한다. GlobalAbilitySystem, AssetManager/GameData의 DynamicTag GameplayEffect 공급, GameplayMessageRuntime, Hero/Camera, AnimInstance, Character/PlayerController 호출부를 누락하지 않는다.

**검증**

- R1-1은 신규 UCLASS/USTRUCT 빌드 및 Data Asset 클래스 선택창 노출까지 확인한다. 아직 ASC와 연결하지 않으므로 게임 동작 변화가 없는 것이 정상이다.
- 원본 함수 본문과 달라진 부분 및 이유를 기록한다. 원본에 없는 기본값이나 정책을 추가하지 않는다.
- 다른 원본 클래스가 필요한 파일은 R2 통합 묶음으로 이월할 수 있다. 이월 파일은 완료 처리하지 않는다.

### R2 — 원본 GAS·Pawn 초기화·입력 공통 기반

**대체 대상**

현재 DCGameplayAbility·DCAbilitySystemComponent·DCAbilitySet, 자체 Pawn 초기화 및 입력 전달 경로.

**세부 작업**

- R2-1: 원본 GameplayAbility·ASC·AbilitySet와 R1에서 확정한 의존성을 함께 이식한다. 기존 Blueprint의 부모·프로퍼티·태그 참조 변환을 준비한 뒤 활성 경로를 바꾼다.
- R2-2: PlayerState의 ASC 소유, PawnData, PawnExtension, HeroComponent, Character/PlayerController의 연계와 InitState 체계를 이식한다. 필요한 Camera·AnimInstance·Health C++ 타입은 함께 준비한다.
- R2-3: InputConfig·InputComponent·Hero 입력 바인딩과 PlayerController의 Ability 입력 처리를 연결한다. 원본 Input Action·IMC·태그·AbilitySet 중 재사용할 데이터를 확인한다.
- R2-4: 원본 Tag Relationship Mapping을 PawnData/ASC에 연결하고 Ability 활성화·취소·부여·회수 수명을 검증한다.

**검증**

- OnSpawn Ability가 유효한 Avatar 연결 시와 Avatar 준비 후 새로 부여될 때 각각 원본 실행 정책에 맞게 활성화된다.
- OnInputTriggered / WhileInputActive, Press / Release / Canceled, 입력 차단과 실행 그룹·태그 관계가 동작한다.
- Pawn 소멸·교체·재시작 후 중복 부여, 이전 Avatar 참조, 남은 Held 입력·Delegate·태그가 없다.
- PawnData AbilitySet의 부여 위치·수명을 원본과 대조한다. 현재 PawnExtension의 부여/회수 코드를 그대로 유지하면서 원본 PlayerState에서도 중복 부여하지 않는다.
- 원본 GameplayAbility의 EffectContext 타입 요구를 만족한다. 부모 클래스 교체만으로 런타임 `check` 실패를 남기지 않는다.

### R3 — 체력·데미지·사망

**세부 작업**

- R3-1: 원본 AttributeSet·CombatSet·HealthSet·HealthComponent와 Death Ability를 이식한다. C++ 기반이 R2에서 앞당겨졌다면 여기서는 원본 GE·Ability·HUD 소비처 연결을 수행한다.
- R3-2: 원본 Damage/Heal Execution, EffectContext 및 AbilitySource를 연결한다. 거리·물리 머티리얼 배율과 데미지 허용 판정의 공급처를 확인한다.
- R3-3: 테스트 대상의 피격·회복·사망·리셋과 이전 HealthComponent 기반 적 사이의 과도기 경계를 검증한다.

**검증**

- GE가 Damage/Healing을 거쳐 Health에 반영되고, 사망 이벤트가 한 번만 발생한다.
- 원본 팀 판정 의존성을 제거한 채 허용 배율이 0으로 남아 데미지가 사라지는 일이 없다. 필요한 팀 기능 재사용 또는 최소 프로젝트 판정 연결의 이유를 기록한다.
- DamageImmunity, 사망 중 입력·장비 정리, Pawn 교체 후 체력 초기화가 일관된다.
- 궁극기 ResourceSet은 프로젝트 고유 자원으로 분류한다. Lyra에 동일 궁극기 구현이 있다고 가정하지 않는다.

### R4 — 원본 카메라와 사용자 조준 규칙

**세부 작업**

- R4-1: 원본 CameraComponent·CameraMode/Stack·ThirdPerson·PenetrationAvoidanceFeeler와 필요한 CameraManager/Assist 연결을 이식·검증한다. R2에서 옮긴 C++는 중복 작성하지 않는다.
- R4-2: 원본 CameraMode Blueprint·곡선 및 `GA_ADS`와 부모의 실제 그래프를 확인해 이식한다.
- R4-3: Hip 짧은 클릭 Scope / Hip Hold Shoulder / Scope 누름 즉시 Hip 규칙을 원본 기반 카메라·Ability에 최소 연결한다. 기존 자체 구현은 별도 보호하지 않는다.

**검증**

- 원본 카메라의 모드 소유·해제, 블렌드 정보, 벽 가림 방지, 감도와 FOV가 정상이다.
- Scope에서 Release가 재진입을 만들지 않고, 강제 취소·사망·Possession 해제 때 조준 상태와 카메라가 정리된다.
- 원본 무기 배율이 조회할 카메라 태그·블렌드 값을 제공한다. 원본 ADS와 다른 부분은 사용자 입력 규칙에 필요한 차이로 기록한다.

### R5 — Inventory·QuickBar·Equipment와 라이플

**세부 작업**

- R5-1: 원본 GameplayTagStack, Inventory Item Definition/Instance/Manager와 필요한 Fragment를 이식한다. SetStats·EquippableItem·ReticleConfig 등 실제 라이플/UI 의존성을 확인한다.
- R5-2: 원본 Equipment Definition/Instance/Manager·WeaponInstance·QuickBar를 이식하고, 원본 아이템 → 슬롯 → 장착 → Instigator 연결을 구성한다.
- R5-3: 원본 라이플 `ID_Rifle`, `WID_Rifle`, `AbilitySet_ShooterRifle`, `B_WeaponInstance_Rifle`, `B_Rifle`와 필요한 종속 에셋을 이식한다. 파일명의 의미만으로 부모 타입을 추정하지 않는다.

**검증**

- 실제 Inventory Item Instance가 장비의 Instigator가 되고, 원본 FromEquipment Ability·ReticleConfig 조회가 같은 아이템을 찾는다.
- 장착 시 Actor 부착 및 Ability 부여, 해제 시 Actor/Ability 회수, 재장착 시 아이템 상태 유지 범위를 원본과 비교한다.
- 기본 무기 직접 장착 경로가 중복 실행되지 않는다. 아이템·장비·ASC 수명이 분리된다.
- R6의 Ability 클래스/에셋이 필요한 라이플 데이터는 의존성 준비 후에만 열어 저장한다. 에셋 이전과 완전한 발사 가능 상태를 구분한다.

### R6 — 원본 퍼짐·발사·GameplayCue·카메라 반동

**세부 작업**

- R6-1: 원본 RangedWeaponInstance의 Heat/Spread 곡선·배율·첫발 정확도·회복과 무기 데이터 계약을 이식한다. WeaponStateComponent의 무기 Tick 경로도 연결한다.
- R6-2: 원본 FromEquipment·RangedWeapon Ability·TargetData·ItemTagStack 비용과 라이플 발사 BP 계층을 이식한다. AbilitySet이 요구하는 재장전/상시 Ability, Montage·슬롯 의존성은 필요 시 R8에서 앞당긴다.
- R6-3: 원본 Damage GE·발사 Cue·Camera Shake·사운드/MetaSound·Niagara·무기 Actor 의존성을 이식한다. 자체 Cue를 새로 설계하는 단계가 아니다.
- R6-4: 발사 간격·탄약 소비·명중·표면 효과·엄폐·카메라 연출을 원본과 동일 조건에서 비교한다.

**검증**

- 실제 발사가 원본의 기본 퍼짐 × 유효 배율을 사용하고, 배율 중복·Heat 이중 Tick·비용 이중 소비·중복 Cue가 없다.
- 원본 Blueprint 설정값과 곡선을 그대로 확인해 사용한다. 기존 DC 테스트 수치를 원본 수치라고 이전하지 않는다.
- 로컬 원본의 `LastFireTime` / `TimeLastFired` 갱신 불일치와 퍼짐 회복 지연을 재현 테스트한다. 수정이 필요하면 원본과의 최소 차이로 기록한다.
- 카메라 반동은 원본 Cue/Shake 설정으로 검증하고, 기존 PendingRecoil 경로를 중복 적용하지 않는다.
- 서버 확인 메시지와 실제 판정 검증 범위를 구분한다. 이번 계획 자체가 멀티플레이 완성·보안 검증을 승인하는 것은 아니다.

### R7 — 원본 Reticle·HUD·명중 표시 재개

**선행 조건:** R5의 Inventory 연결과 R6의 무기 데이터/명중 경로. 시각 에셋이 이미 있다는 이유로 생략하지 않는다.

**세부 작업**

- R7-1: 원본 LyraReticleWidgetBase·LyraWeaponUserInterface·CircumferenceMarkerWidget/Slate·HitMarkerConfirmationWidget/Slate 및 필요한 CommonUI·메시지 의존성을 이식한다.
- R7-2: 원본 W_Reticle_Rifle·W_WeaponReticleHost·W_Reticle_AmmoBar·필요한 보조 Widget을 호환 타입과 참조 변환 준비 후 Migrate한다. 기존에 가져온 시각 에셋은 중복 덮어쓰지 않고 참조·수정 여부부터 확인한다.
- R7-3: 원본 레이아웃·UMG 애니메이션·머티리얼 파라미터를 유지하면서 Aim/Scope·탄약·명중·처치 신호를 연결한다. 원본 WeaponStateComponent의 실제 데미지/팀 판정 연계는 검증 범위를 명시한다.

**검증**

- 발사와 Reticle이 같은 무기·최종 퍼짐 계약을 사용하고, 해상도·DPI·FOV 변경 시 반경/배치가 맞는다.
- 첫발 정확도, 조준, 연사, 명중, 처치가 각각 올바른 데이터로 표시된다. 미구현 신호를 가짜 성공 이벤트로 대체하지 않는다.
- 무기 해제·교체·사망·Pawn 교체 때 위젯과 구독이 정리되고, 옛 Reticle과 중복 표시되지 않는다.
- 기존 7-1의 자체 구현 완료와 이번 R7 원본 이식 완료를 별도로 기록한다.

### R8 — 전체 애니메이션 계층과 재장전

**세부 작업**

- R8-1: 원본 LyraAnimInstance·CharacterMovement의 애니메이션 지원 API와 `ABP_Mannequin_Base`, `ALI_ItemAnimLayers`, `ABP_ItemAnimLayersBase`, 라이플 Linked Layer를 의존성과 함께 이식한다.
- R8-2: 원본 이동·정지·방향 전환·점프/낙하·조준·발사 계층, Aim Offset, IK/Control Rig와 필요한 플러그인을 검증한다. 스켈레톤·소켓·슬롯 불일치 시 필요한 Retarget/참조 변경만 수행한다.
- R8-3: 원본 라이플 재장전 및 자동 재장전 Ability 계층, 비용/탄약 태그 스택, Montage·Notify·이벤트를 연결한다. R6에서 준비한 의존성을 다시 작성하지 않는다.

**검증**

- 기존 7-A의 Idle 연결만으로 전체 완료 처리하지 않는다. 이동·공중·조준·발사·장착/해제의 원본 Layer 연결을 확인한다.
- 재장전 완료·취소·사망·무기 교체에서 탄약 증감이 한 번만 적용되고 발사 차단 태그가 남지 않는다.
- 장비가 링크한 Layer를 올바르게 해제하고, 실제 무기 상태와 애니메이션이 일치한다.

### R9 — Lyra Dash, 방어 효과, 궁극기

**세부 작업**

- R9-1: 원본 GA_Hero_Dash의 부모·태그·입력·몽타주·Root Motion·쿨다운·Cue·UI를 검수하고 이식한다. 무입력·방향 선택 규칙은 원본을 따른다.
- R9-2: 원본의 실제 방어 기능 유무를 확인한다. 부족한 경우에만 무적/데미지 차단·시작/종료 이벤트·성공 피드백을 프로젝트 추가 기능으로 구성하고 별도 검증한다.
- R9-3: 궁극기는 현재 확정된 기획과 원본에 재사용 가능한 Ability·비용·Effect·Cue·UI를 먼저 대조한다. 전용 효과·충전/소모·쿨다운 등 미확정 규칙은 사용자와 확정한 뒤 필요한 부분만 구현한다.

**검증**

- 폐기한 무입력 전방/8방향 자체 대시가 다시 실행되지 않는다. 원본과 입력 없음·지상·공중·벽·취소 상황을 비교한다.
- 조준·발사·점프 등 상호작용과 쿨다운 시작/종료가 그래프 확인 결과와 일치한다.
- 방어 성공은 실제 공격 차단 근거가 있는 경우에만 발생하며, 종료·사망 후 무적 태그가 남지 않는다.
- 궁극기 설계가 미확정이면 해당 세부 작업만 대기 상태로 기록한다. 원본에 동일한 궁극기가 존재한다고 가정하지 않는다.

### R10 — 적 GAS와 공격 텔레그래프

**세부 작업**

- R10-1: 현재 적의 체력·사망·Spawner 구독을 원본 GAS 전투 경로로 이전한다. ASC 소유 위치와 AI 실행 주체는 실제 프로젝트 요구로 확정한다.
- R10-2: 재사용할 원본 공격·AbilityTask·Montage·GameplayCue를 확인하고, 의도 → 선딜 → 명중 구간 → 후딜과 중단 처리를 연결한다.
- R10-3: 예고 VFX·애니메이션·사운드를 가능한 원본 자산으로 연결한다. DreamCatcher에만 필요한 텔레그래프 규칙은 원본 제공 기능과 구분한다.

**검증**

- 공격 예고 전에 즉시 데미지가 들어가지 않고, 선딜 중 사망·취소·타깃 상실·장애물 상황에서 예정된 공격이 올바르게 정리된다.
- 적 사망 집계와 인카운터 완료가 중복 발생하지 않는다.
- 이 작업을 이유로 Bot·대형 Behavior Tree 프레임워크 전체를 자동 도입하지 않는다.

### R11 — 실제 보스 전 빈 스테이지 흐름 완주

**세부 작업**

- R11-1: 새 플레이어/적 전투 경로를 Level 1의 전투 1·전투 2에 연결한다.
- R11-2: 보스 Placeholder와 결과 Placeholder를 연결하여 먼저 한 판을 끝낸다.
- R11-3: 재시작·실패·레벨 이동 시 ASC·장비·HUD·인카운터 상태를 검증한다.

**검증**

- `전투 1 → 전투 2 → 보스 Placeholder → 결과 Placeholder`를 Editor 수동 개입 없이 완주한다.
- 원본 GamePhase·UI·메시지 중 쓸 수 있는 부분을 검토하되, Lyra에 DreamCatcher 스테이지 순서가 그대로 구현되어 있다고 가정하지 않는다.
- 기존 StageDirector/Encounter/Spawner도 비교 대상이다. 직접 대응하는 원본이 없으면 프로젝트 흐름 역할로 유지·최소 수정하며 이유를 기록한다.
- 실제 보스 모델·완성 애니메이션이 없어도 이 단계를 완료할 수 있어야 한다.

### R12 — 최소 보스

- R12-1: 원본 GAS·Health·Cue·Animation·HUD 기반 중 재사용 가능한 부분으로 보스 전용 연결을 구성한다.
- R12-2: 구분되는 패턴 2개, 읽을 수 있는 텔레그래프, 체력 기반 페이즈 전환 1회, 보스 체력 UI를 구현한다.
- R12-3: 사망 신호로 R11의 Placeholder를 실제 보스전으로 교체하고 결과까지 검증한다.

Lyra에 동일한 보스 패턴 시스템이 있다고 단정하지 않는다. 원본에 대응하지 않는 보스 규칙은 프로젝트 고유 구현으로 명시한다.

### R13 — Experience 및 남은 원본 기능 확장

- R13-1: 기존 후속 범위인 Experience·GameFeature 로딩과 PawnData/Ability/UI 조립 중 실제 필요한 부분을 검토·이식한다. R2 등에서 필수 의존성으로 앞당긴 부분은 여기서 중복 구현하지 않는다.
- R13-2: 사용하기로 한 추가 무기와 공통 HUD·입력·연출 에셋도 원본 Inventory/Equipment/Ability/Layer 구조로 확장한다. 최종 무기 종류·수를 임의 확정하지 않는다.
- R13-3: 이식 목록에서 빠진 재사용 가능 코드·BP·데이터·시각 에셋을 재감사한다. 보류·제외 항목에는 이유와 재개 조건을 남긴다.

대형 인벤토리 UI·보상 경제·저장·온라인 FrontEnd는 자동 포함하지 않는다. 의존성 때문에 필요한 작은 기능과 별도 게임 확장을 구분한다.
각 도입 후 기존 수직 슬라이스와 선택한 새 기능을 모두 검증한다.

### R14 — Legacy·참조·회귀 최종 정리

- R14-1: 이미 대체 검증한 기존 자체 GAS·Combat·Health·Camera·Equipment·Spread·Reticle·Dash·Animation 경로의 남은 참조를 점검한다.
- R14-2: 사용자가 승인한 제거 범위에서 코드와 에셋을 정리한다. BP 부모·Config·태그·Redirect·패키지 참조를 확인하고 에셋 삭제는 Editor에서 수행한다.
- R14-3: 전체 Editor 빌드, Blueprint Compile, 레벨 완주·사망·재시작·무기 교체·조준·회피·해상도 테스트와 배포용 패키징 검증을 수행한다.

같은 입력/데미지/Heat/카메라 반동을 처리하는 경로가 하나로 정리되어야 한다.
기존 미커밋 변경이나 무관한 에셋을 일괄 삭제하지 않는다. Redirect는 낡아 보인다는 이유만으로 제거하지 않는다.

## 원본 검수 근거와 미검증 항목

다음 경로는 위 `Lyra 로컬 경로`를 기준으로 한 원본 위치다. 파일 존재·일부 C++ 대조와 Blueprint 그래프 검증을 구분한다.

| 확인 대상 | 원본 경로 | 현재 근거/주의 |
|---|---|---|
| OnSpawn·Ability 기반 | `Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp`, `Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.cpp` | C++ 실행 경로 확인. DC에는 enum만 있음 |
| 태그 관계표 | `Source/LyraGame/AbilitySystem/LyraAbilityTagRelationshipMapping.h`, `Source/LyraGame/AbilitySystem/LyraAbilityTagRelationshipMapping.cpp` | 두 파일 본문 확인. R1-1 원본 |
| Pawn 초기화 | `Source/LyraGame/Character/LyraPawnExtensionComponent.h`, `Source/LyraGame/Character/LyraHeroComponent.cpp`, `Source/LyraGame/Player/LyraPlayerState.cpp` | InitState·PlayerState 부여와 현재 자체 수명 차이 확인 |
| EffectContext | `Source/LyraGame/AbilitySystem/LyraAbilitySystemGlobals.cpp`, `Source/LyraGame/AbilitySystem/LyraGameplayEffectContext.h` | 원본 Ability가 확장 Context를 요구. Config/호출부를 함께 준비 |
| 무기 아이템 연결 | `Source/LyraGame/Equipment/LyraQuickBarComponent.cpp`, `Source/LyraGame/UI/Weapons/LyraWeaponUserInterface.cpp` | QuickBar의 Instigator 할당과 UI의 non-null 조건 확인 |
| 퍼짐 계약 | `Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp`, `Source/LyraGame/UI/Weapons/LyraReticleWidgetBase.cpp` | 원본 기본각×배율, DC 최종각 반환 및 화면 좌표 차이 확인 |
| 무기 갱신 | `Source/LyraGame/Weapons/LyraWeaponStateComponent.cpp` | 원본 Tick 존재. DC Equipment Tick과 중복 금지 |
| 회복 지연 | `Source/LyraGame/Weapons/LyraRangedWeaponInstance.h`, `Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp`, `Source/LyraGame/Weapons/LyraWeaponInstance.cpp` | LastFireTime은 초기화/읽기만 발견, 발사 갱신은 TimeLastFired. 런타임 재현은 미검증 |
| 명중 확인 | `Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp` | 로컬 소스에 `bIsTargetDataValid = true`가 있음. 완전한 서버 검증이라고 단정 금지 |
| 라이플 발사·Cue | `Plugins/GameFeatures/ShooterCore/Content/Weapons/Rifle/GA_Weapon_Fire_Rifle_Auto.uasset`, `Plugins/GameFeatures/ShooterCore/Content/Weapons/Rifle/GCN_Weapon_Rifle_Fire.uasset` | 파일 존재 확인. 정확한 그래프·CDO 값·실행 비교는 R6에서 수행 |
| Reticle Host | `Plugins/GameFeatures/ShooterCore/Content/UserInterface/HUD/W_WeaponReticleHost.uasset` | 파일 존재 확인. 내부 생성/정리 그래프는 R7에서 수행 |
| Dash | `Plugins/GameFeatures/ShooterCore/Content/Game/Dash/GA_Hero_Dash.uasset` | 파일 존재 확인. 원본 방향·무입력·방어 효과는 R9에서 그래프/PIE 확인 |
| 애니메이션 계층 | `Content/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base.uasset`, `Content/Characters/Heroes/Mannequin/Animations/LinkedLayers/ABP_ItemAnimLayersBase.uasset`, `Content/Characters/Heroes/Mannequin/Animations/LinkedLayers/ALI_ItemAnimLayers.uasset`, `Content/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers.uasset` | 파일 존재 확인. 전체 그래프·플러그인·스켈레톤 검증은 R8에서 수행 |

원본 파일은 버전 갱신으로 바뀔 수 있으므로 단계 시작 시 다시 확인한다. 표의 기존 점검은 해당 단계 완료 판정의 대체물이 아니다.

## 공식 문서 참고

- GAS 확장, OnSpawn, 태그 관계, Ability 비용의 개념은 [Abilities in Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)를 참고한다. 구체적인 동작은 설치된 원본 코드/그래프와 대조한다.
- Inventory와 Equipment의 역할·수명 구분은 [Lyra Inventory and Equipment](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-inventory-and-equipment-in-unreal-engine)를 참고한다.
- Pawn 기능들의 초기화 상태 연계는 [Game Framework Component Manager](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-framework-component-manager-in-unreal-engine)를 참고한다.

## 사이드 채팅 운영 및 인계

### 시작 규칙

1. 루트 AGENTS.md와 이 명세를 읽는다. 별도 채팅의 완료 보고 없이 앞 단계가 끝났다고 추정하지 않는다.
2. 해당 R 세부 단계의 실제 파일·빌드/Editor 검증 기록을 확인하고 미완료 선행 작업이 있으면 먼저 설명한다.
3. 사용자에게는 존댓말로, Unreal 초심자가 직접 따라 할 수 있는 C++ 복사/최소 수정·파일 위치·Editor 노드 연결·검증 절차를 채팅으로 제공한다.
4. 기본적으로 프로젝트 파일은 직접 수정하지 않는다. 사용자가 문서 갱신만 요청하면 문서만, 소스 수정을 별도로 요청하면 그 승인 범위만 수정한다.
5. 확인 못 한 Blueprint 그래프·기본값은 추정하지 말고, 필요한 Editor 확인 절차나 사용자 스크린샷을 요청한다.
6. 한 채팅에서는 선택한 세부 단계만 진행한다. 큰 상호 의존 묶음이면 범위를 먼저 밝히고 빌드 가능한 단위로 안내한다. 병렬 채팅에서 같은 파일·에셋을 동시에 작업하지 않는다.
7. 원본을 그대로 가져오지 못하는 부분은 해당 변경 안내 전에 원본 근거·구체적 제약·대안·동작 차이를 설명한다. 확인 부족이면 미검증으로 남기고 다음 확인 절차를 제시한다.

### 다음 채팅에 붙여 넣을 시작 문구

```text
DreamCatcher의 AGENTS.md와 docs/specs/gas-lyra-migration.md를 읽고
R1-1 — Ability Tag Relationship Mapping 원본 두 파일 이식을 진행하겠습니다.
기존에 안내만 받았으므로 실제 파일 존재 여부부터 확인해 주세요.
Lyra 원본 재사용 → 최소 수정 이식 → 불가피한 부분만 직접 구현 원칙을 적용해 주세요.
기존 자체 구현은 교체 대상이며, 프로젝트 파일은 직접 수정하지 말고 제가 작업할 절차를 설명해 주세요.
원본 경로, 바꿀 이름, 의존성, 빌드/Editor 검증 기준과 미검증 내용을 구분해 주세요.
원본을 그대로 이식하지 못하는 부분은 실제 근거와 이유, 원본을 보존할 대안, 달라지는 동작을 먼저 설명해 주세요.
이번에는 이 세부 단계까지만 진행해 주세요.
```

### 단계 종료 인계 양식

```text
작업 ID / 상태: R?-? / 안내만·구현 중·검증 대기·완료 중 하나
브랜치 / 확인한 프로젝트·엔진 버전:
원본 파일·에셋 / 대상 파일·에셋:
원본 유지 범위:
최소 변경 내용과 이유 / 원본 대비 의도적인 동작 차이:
그대로 이식하지 못한 항목 / 확인 근거 / 제약 분류 / 검토한 대안 / 결정·재개 조건:
선행 의존성 / 앞당겨 수행한 다른 단계의 일부:
실제로 사용자가 변경한 파일 / 기존 dirty 변경과의 구분:
빌드 결과 / Editor Compile / PIE 비교 결과: 각각 실행 여부와 근거
남아 있는 참조·임시 연결·미검증 항목:
기존 경로 비활성화·제거 여부와 복구 기준:
다음 작업 ID 및 시작 조건:
```

소스 존재만으로 사용자 빌드 성공을 추정하지 않는다. 완료 상태와 체크리스트를 파일에 반영하려면 문서 수정 승인 범위를 확인한다.
