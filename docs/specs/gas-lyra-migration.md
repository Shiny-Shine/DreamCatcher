# DreamCatcher GAS·Lyra 전환 명세

## 상태

- 결정 상태: 승인됨
- 이식 정책 갱신: 2026-09-10 KST, 사용자 승인
- 진행 현황 갱신: 2026-10-09. **R1 감사 보완2건 통과:** 실패태그5개 원본 매핑 복구와 DefaultGameData 단일 재저장 후 비에디터 GameData/Integration 맵 로드·종료0, 별도 재로드 및 기존 회귀8건 통과. AssetManager C++나 R6/재장전 규칙은 변경하지 않았다. 다음은 R6 실제 회복지연 재현/최소 변경안이며 R2/3/4/5의 남은 통합 회귀와 R7 착수 보류는 유지한다. 사운드 사용자 통과·R 현재 미재현/관찰 상태도 유지한다.
- 엔진 버전: Unreal Engine 5.8
- 기준 프로젝트: DreamCatcher
- 참고 프로젝트: Lyra Starter Game 5.8
- Lyra 로컬 경로: E:\Epic Games\UEProjects\LyraStarterGame
- 적용 방식: 이식 가능한 Lyra C++·Blueprint·데이터·시각 에셋을 최대한 원본 재사용하고, DreamCatcher 연결에 필요한 부분만 최소 수정
- 에셋 정책: Unreal Editor의 Migrate로 필요한 의존성을 함께 이전하고, 원본 레이아웃·애니메이션·머티리얼·곡선을 우선 보존
- 코드 정책: C++도 필요한 범위의 원본 이식을 우선하며, 재구현은 원본 재사용이 부적합한 이유를 확인한 경우에 한정
- 진행 방식: 변경안·원본 대비 차이 제시 → 연관된 코드·에셋 작업 묶음 승인 → Codex 처리·자동 검사 → 사용자 C++ 빌드·패키징 및 짧은 플레이 확인
- 직접 수정 권한: Unreal 내부 에셋 자동화는 사용자가 허용했다. 코드·Config는 승인된 작업 묶음 범위만 수정하며, 문서만 갱신하라는 요청은 문서만 수행한다. C++ 빌드·패키징은 사용자 담당이다
- 새 실행 계획: 이 문서의 `R0~R14`를 사용. 과거 `0~12단계`는 기존 기능 구현 이력으로 별도 보존

## 2026-10-08 추가 결정 — 실제 구현 우선

사용자는 완료 상태 확인과 함께 “이제부터는 추가 도구 확장보다 실제 작업 진행을 우선”하도록 명시했다. 이전의 세분화된 도구/선행검사 확대보다 이 운영 원칙을 우선한다.

1. 다음 우선 결과물은 **R5-3 라이플 실에셋 이식＋R6 테스트 플레이어의 장착·발사·재장전·해제 연결**이다. R7/HUD 전체와 R13/Experience 전체 완료를 무조건 선행 조건으로 삼지 않는다. 실제 로드/Compile/실행을 막는 부분만 근거를 남기고 포함한다.
2. 기존 자동화와 검사는 재사용한다. 새 범용 도구나 감사기 확장은 실제 진행을 막는 재현된 문제에 필요한 최소 수정일 때 별도 승인받는다. 에셋 이식·설정을 수행하는 제한된 일회성 자동화와 짧은 수동 Editor 절차를 비교해 작업시간을 줄인다.
3. 연관 코드·에셋·필수 검증을 한 묶음으로 설명·승인받고, 사용자의 빌드와 짧은 플레이 확인 왕복을 줄인다. 매번 독립 검사 수 증가를 기능 완료로 보고하지 않는다.
4. 원본 재사용/변경 전 근거·제약·대안·동작 차이 설명, 미검증 명시, 빌드·패키징 사용자 담당, 기존 변경 보호와 승인 경계는 유지한다. 후속 검증으로 분리해도 기능을 삭제하거나 R0~R14 완료 목표를 축소하지 않는다.

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

## 2026-10-05 승인 — 자동화 운영 및 마감 기준

과거의 한 단계씩 수동으로 구현하는 방식에서, 사용자 작업을 줄이는 **승인된 기능 묶음 + Unreal 내부 자동화** 방식으로 전환한다. 원본 최대 재사용 정책과 변경 전 근거 설명 의무는 바뀌지 않는다.

### 역할과 권한

- **사용자:** C++ 빌드, 패키징, 조작감·화면·연출을 직접 확인한다. Codex는 별도 지시 없이는 C++ 빌드·패키징을 실행하지 않는다.
- **Codex:** 원본 조사, 승인된 코드 작업, Unreal 내부 에셋 생성·설정·검사 자동화를 담당한다.
- 작업 전 `AGENTS.md`와 이 명세를 먼저 읽고, 현재 상태와 승인 범위를 확인한다.
- 각 묶음의 대상 파일·에셋, 원본 근거·제약·대안·동작 차이, 검사 범위를 먼저 제시한다. 승인된 범위 안에서 처리·자동 검사한 뒤 사용자에게 빌드와 필요한 플레이 확인을 전달한다.
- 연관된 R 세부 단계의 의존성을 묶을 수 있다. 단순히 승인 횟수를 줄이려고 무관한 기능까지 포함하거나, 뒤 단계의 미검증 기능을 완료 처리하지 않는다.
- “완료 상태 확인”, “다음 단계 진행”을 새로운 코드·Config 수정 범위에 대한 포괄 승인으로 해석하지 않는다. 원본과 다른 동작, 기능 보류·제외, 삭제·덮어쓰기·대량 참조 교체·실제 병합은 해당 변경 범위의 승인을 확인한다.
- Unreal 내부 API를 통해 생성·설정·Compile·저장한다. `.uasset` 등을 에디터 밖에서 일반 바이너리 파일로 복사·변조하지 않는다. 에디터와 자동화 프로세스가 같은 에셋을 동시에 저장하지 않게 한다.
- Python/Editor Utility/Commandlet 실행과 Blueprint Compile은 **C++ 빌드·패키징과 구분**한다. 에셋 검사는 저장 후 별도 프로세스 재로드까지 확인하고, 자동화하지 못한 부분만 수동 Editor 절차로 전달한다.
- 임시 에디터 월드의 자동 검사, PIE, 네트워크 복제, 시각·조작감 검증은 서로 다른 근거다. 하나의 성공으로 나머지를 완료 처리하지 않는다.

### 마감 범위와 체크포인트

- **2026-10-16까지 마이그레이션 작업을 마치고 디자이너 브랜치와 병합**하는 것이 목표다. **R9~R12의 신규 제작도 포함**하며, 이를 17~20일로 넘기거나 임의로 범위를 축소하지 않는다.
- **2026-10-17~20:** 아웃게임 UI 등 게임의 완성도와 프로젝트 제출 마감을 진행한다.
- 사용자는 매일 **3~5시간**, 많은 날은 **10시간** 작업 가능하다. 10월 5일 시점의 단순 가용 시간은 약 **36~60시간 + 집중 작업일의 추가 시간**이며, 매일 10시간을 쓸 수 있다고 가정하지 않는다.
- 다음 표는 **완료 약속이 아닌 지연 판단 기준**이다. 미달 시 전체 일정에 미치는 영향을 즉시 알리고, 사용자 승인 없이 범위를 축소하거나 검증을 생략하지 않는다.

| 날짜 (KST) | 판단 기준 |
|---|---|
| 2026-10-08 | 원본 장비·발사·재장전 핵심 흐름 확보 |
| 2026-10-12 | 신규 제작을 포함한 스테이지·보스·결과 흐름 실행 |
| 2026-10-14 | 기능 동결, 통합·오류 수정에 집중 |
| 2026-10-15 | 디자이너 변경 통합 및 병합 후보 검증 |
| 2026-10-16 | 최종 병합과 사용자 빌드·패키징 검증 완료 목표 |

### 브랜치 협업과 확인 한계

- 개발 브랜치는 `GAS-System`, 디자이너 작업 브랜치는 `main`이다. 사용자 설명상 GAS 테스트 맵과 디자이너 맵은 분리되어 있다.
- 2026-10-05에 **로컬에 저장된 커밋**을 비교했다: `main`/`origin/main`은 `ddc65ba`(10월 2일), `GAS-System`은 `205ca1b`(10월 5일), 공통 조상은 `50ef6ac698897f531f418e0f41beb648954e4e41`이다.
- 이 기준에서 양쪽이 동일하게 수정한 파일 경로는 **0개**였다. 원격 최신 상태를 새로 가져와 확인한 결과가 아니며, 디자이너 PC의 미커밋 변경도 포함하지 않는다.
- 파일 경로가 겹치지 않아도 Blueprint·머티리얼·Config 참조, 부모 클래스, 게임플레이 호환성은 별도로 검증해야 한다. 향후 실제 맵 통합 때 작업 범위도 다시 조정한다.
- **실제 병합은 아직 하지 않았다.** 16일에 처음 병합하지 말고 디자이너의 중간 커밋을 받아 통합 검증할 시점을 잡는다. 로컬 스냅샷만으로 병합 안전을 확정하지 않는다.

## 현재 구현 이력과 재개 지점

### 2026-10-05 현재 인계 — R5-1 기본 자동 검사 통과

아래는 R5-1까지의 완료 이력이다. 이후 R5-2의 빌드·독립 자동 검사까지 진행했으므로 **아래 16:31 KST 검증 인계**를 최신 재개 기준으로 사용한다. 승인되지 않은 다음 코드 묶음을 먼저 수정하지 않는다.

| 항목 | 확인된 범위 | 완료와 구분할 내용 |
|---|---|---|
| R4-2 원본 ADS | 원본 이식본의 Left Alt 테스트, 카메라·속도·감도·사운드 복구에 대한 사용자 확인. 줌 MetaSound 의존성 재저장 후 재시작 검증 | 모든 플랫폼·UI·시각 수치의 정밀 검증은 아님 |
| R4-3 PC 입력·종료 | 원본 ADS 그래프를 유지하고 클릭/Hold를 논리 입력으로 전달. Scope·Shoulder 정상 종료, Scope 중 사망 취소, 재시작 후 재바인딩, 입력 차단 ON/OFF 6쌍, 두 모드의 UnPossess 후 Hip 복귀 확인 | 게임패드 Aim Assist·터치 UI는 보류. 이 기록은 모든 장치에 대한 R4 전체 완료가 아님 |
| R5-1 기본 C++ | `Source/DreamCatcher/Inventory/`에 Definition·Instance·Manager·SetStats의 `.h/.cpp` 총 8개 추가. 기존 GameplayTagStack 재사용. 원본과 이름·필요 include·설명 주석 외 실행 코드 대조 | 나머지 Fragment·Equipment·QuickBar·원본 라이플 소비처 연결은 미완료 |
| 사용자 C++ 빌드 | 2026-10-05 02:29 KST의 `DreamCatcherEditor Win64 Development`, `Result: Succeeded` | 에디터 게임플레이·복제 성공을 뜻하지 않음 |
| R5-1 에셋 자동화 | Unreal 내부 생성·설정·Compile·저장 후 **별도 프로세스 재로드**. 아이템 생성, 초기 StatTags 7, 증가 후 10, 감소 후 6, 조회, 제거 후 빈 목록 확인 | **임시 에디터 월드 검사**이며 PIE·복제·Equipment·QuickBar 연결은 미검증 |

**자동화 산출물과 근거**

- 스크립트: `Scripts/Editor/r5_inventory_automation.py`
- 테스트 에셋: `/Game/DreamCatcher/GAS/Test/R5/Automation/ID_DC_R5_InventorySmoke`
- 실제 파일: `Content/DreamCatcher/GAS/Test/R5/Automation/ID_DC_R5_InventorySmoke.uasset`
- 로그: `Saved/Logs/R5InventoryAutomation-verify.log`, 2026-10-05 **03:18 KST**(본문 UTC `2026.10.04-18.18.13`). `smoke_pass`, `mode=verify`, `cpp_build=false`, `packaging=false`, `PIE=false`를 확인했다. 로그 줄 번호 대신 실행 시각·표식을 대조한다.
- 테스트용 스택은 기존 `Test.R2.CostCharge` 태그를 **새 Inventory Item Instance 안에서만** 사용한다. 실제 플레이어 비용이나 밸런스를 바꾼 것이 아니다.
- 모드는 `audit`(기본값, 쓰기 없음), `prepare`(지정된 신규 에셋만 생성·저장), `verify`(저장된 에셋 재로드·검사)다. 기존 대상 에셋이 있으면 기대값을 검증하고 덮어쓰지 않는다.
- 실제 검사는 별도 `UnrealEditor-Cmd` Python commandlet에서 수행했다. 프로세스 한정 `-DDC-ForceMemoryCache`로 초기 캐시 문제를 피했으며, 프로젝트 DDC 설정을 변경한 것은 아니다.
- 이 자동화 작업에서 추가한 것은 위 스크립트와 신규 테스트 에셋이다. 기존 코드·Config·게임 에셋은 변경하지 않았고, Codex가 C++ 빌드·패키징을 실행하지 않았다.

**원본 제한 및 남은 작업**

- 원본의 `AddItemInstance` 경로는 `unimplemented()`이며 그대로 보존했다. 동작하는 `AddItemDefinition` 경로로 검증했고, 미구현 경로를 호출하거나 완료된 기능으로 보고하지 않는다.
- 원본 `CanAddItemDefinition`의 용량·중복 검사 TODO, Instance 개수 기준의 `GetTotalItemCountByDefinition` 및 전체 Entry를 제거하는 `ConsumeItemsByDefinition` 의미를 임의 변경하지 않았다.
- EquippableItem·QuickBarIcon·PickupIcon·ReticleConfig 등의 후속 Fragment와 Equipment·WeaponInstance·QuickBar 의존성을 검토·연결해야 한다. 현재 자체 장비·발사 코드는 대체 검증 전까지 비교·복구용이며 원본 통합 완료가 아니다.
- 다음 묶음에서는 승인 전 원본 차이·대상 경로·의존성·자동 검사와 사용자 빌드/PIE 범위를 제시한다. 위 Inventory 생성·StatTags·제거 검사를 위해 사용자가 수동 Print String 그래프를 다시 만들 필요는 없다.
- 멀티플레이 복제, 실제 장비 Actor/Ability 부여·회수, 실사용 Pawn/Controller 연결, 모든 시각·입력 장치 검증은 별도다. 기존 LevelScript 입력 경고나 과거 Live Coding 오류도 해당 시점의 기록으로 구분한다.

### 2026-10-05 R5-2 코드 준비 이력 — 아래 후속 검증 결과와 함께 읽을 것

**승인된 방향:** 원본 계층을 별도 이름으로 이식하고 독립 검증한 뒤 활성 경로를 교체한다. 활성 경로의 회귀 검증까지 완료되면 대응하는 기존 자체 구현을 제거한다. 기존 구현을 최종 구현으로 보호하거나, 검증 전에 먼저 삭제하지 않는다.

**이번 코드 범위 (원본 기반 10쌍 / 20개 파일)**

- `Source/DreamCatcher/Equipment/Lyra/`: `DCLyraEquipmentDefinition`, `DCLyraEquipmentInstance`, `DCLyraEquipmentManagerComponent`, `DCLyraQuickBarComponent`, `DCLyraGameplayAbility_FromEquipment`의 `.h/.cpp`.
- `Source/DreamCatcher/Weapon/Lyra/DCLyraWeaponInstance.h/.cpp`.
- `Source/DreamCatcher/Cosmetics/DCLyraCosmeticAnimationTypes.h/.cpp`.
- `Source/DreamCatcher/Inventory/`: `DCInventoryFragment_EquippableItem`, `DCInventoryFragment_QuickBarIcon`, `DCInventoryFragment_PickupIcon`의 `.h/.cpp`.
- 원본 근거: `LyraStarterGame/Source/LyraGame/Equipment/`, `Weapons/LyraWeaponInstance.*`, `Cosmetics/LyraCosmeticAnimationTypes.*`, `Inventory/InventoryFragment_*.{h,cpp}` 중 위 대응 파일. Epic 저작권 표기를 유지했다.
- 변경은 Reflection/파일 이름, 모듈 API 매크로, include 경로·명시적 include와 기존 원본 이식 타입 연결이다. Inventory는 `UDCInventory*`, GAS는 기존 원본 기반 `UDCAbilitySet`·`UDCAbilitySystemComponent`·`UDCGameplayAbility`, 사망 알림은 `UDCLyraHealthComponent`를 사용한다. 이름 치환과 include·주석·공백을 제외한 토큰 대조에서 **20개 모두 일치**했다. 빌드 성공을 뜻하지 않는다.

**원본 동작과 제약 보존**

- 기존 자체 `DCEquipment*`는 `bool SpawnEquipmentActors`, 자체 장비 목록·Tick·현재 무기 getter·ASC 해제 연결을 사용한다. 원본의 `void SpawnEquipmentActors`, PawnComponent/FastArray, 장비 수명 계약과 달라 같은 타입을 덮어쓰지 않았다. 별도 원본 타입은 이 차이를 호환용 가짜 API로 숨기지 않는다.
- Ability 부여 → Actor 생성 → OnEquipped, 해제 시 OnUnequipped → Ability 회수 → Actor 파괴를 유지한다. QuickBar의 아이템 Instigator 설정은 원본처럼 `EquipItem` 반환 **후**다. OnEquipped/OnSpawn 시점부터 아이템 조회가 가능하다고 가정하지 않는다.
- 원본 메시지 태그 `Lyra.QuickBar.Message.SlotsChanged`·`Lyra.QuickBar.Message.ActiveIndexChanged`, Weapon의 `UneuippedAnimSet` 프로퍼티 철자, 장비의 빈 `OnRep_Instigator`도 보존했다. 원본 `RemoveItemFromSlot`은 활성 인덱스를 -1로 만들지만 SlotsChanged만 방송하며 이 차이를 임의 수정하지 않았다.
- `ReticleConfig`는 원본 ReticleWidget/RangedWeapon 타입과 함께 준비해야 한다. 기존 자체 Reticle 타입으로 대체하지 않았다. R6~R7 의존성 준비 후 재개할 항목이며 이식 불가 판정이 아니다.
- 기존 PawnData 기본 무기 자동 장착, PlayerState 소유 ASC/PawnExtension 해제 순서, 기존 HUD·발사 소비처 연결은 **미변경·미검증**이다. 실사용 Pawn에서 두 장비 경로가 동시에 자동 장착되지 않도록 후속 활성 전환 시 검증한다.

**독립 검증 준비 (아직 실행하지 않음)**

- `Source/DreamCatcher/Tests/DCEquipmentAutomationTests.cpp`: `DreamCatcher.R5.Equipment.QuickBarLifecycle`. UE 5.8 `FTestWorldWrapper`로 GameInstance·메시지 Subsystem이 있는 임시 Game 월드를 사용한다. 기존 임시 Editor 월드 Inventory 검사와 다르며, PIE/네트워크 검사는 아니다.
- 테스트 Pawn/Controller는 엔진 기본 타입이고 ASC는 테스트 Pawn에만 붙인다. 실제 플레이어의 PlayerState 소유 ASC 구조를 바꾼 것이 아니다. 장착·부착, 실제 Inventory Instigator, FromEquipment SourceObject/아이템 조회, 슬롯 순환·잘못된 인덱스·중복 선택, 아이템 Stat 유지, 해제와 Manager 제거 시 Actor/Ability 회수를 검사하도록 작성했다.
- `Scripts/Editor/r5_equipment_automation.py`: `audit`(기본, 읽기), `prepare`(신규 생성·저장), `verify`(재로드 검사). 기존 에셋은 기대값 검사만 하며 덮어쓰거나 삭제하지 않는다. Python 구문 검사는 통과했지만 Unreal API 실행은 미검증이다.
- 생성 예정 경로는 `/Game/DreamCatcher/GAS/Test/R5/Automation/Equipment/`의 `GA_DC_R5_EquipmentSmoke`, `AS_DC_R5_EquipmentSmoke`, `WID_DC_R5_EquipmentSmoke`, `ID_DC_R5_EquipmentSmoke` **4개만**이다. 이번에는 아직 생성하지 않았다. 빈 SkeletalMeshActor로 부착 수명만 검사하며, 원본 라이플 외형·발사·연출을 대체하는 에셋이 아니다.
- 테스트는 별도 `UnrealEditor-Cmd` 프로세스의 `-DCR5IsolatedAutomation` 플래그가 없으면 실행을 거부한다. GameInstance가 프로세스 전역 delegate를 사용하는 영향 때문에 사용자의 작업 중인 Editor 세션에서는 실행하지 않는다.

**다음 절차:** 사용자가 에디터를 닫고 `DreamCatcherEditor Win64 Development` C++ 빌드 → Codex가 별도 Python commandlet에서 `prepare` → 별도 프로세스 `verify` → 별도 프로세스에서 위 C++ Automation Test 실행. Codex는 C++ 빌드·패키징을 실행하지 않는다. 통과 로그를 확보한 뒤 R5-3/R6 원본 라이플 의존성 및 활성 경로 교체안을 제시한다. 복제, 실제 Pawn 교체·사망·ASC 해제 순서, 시각·장치 피드백과 활성 경로 회귀는 독립 테스트 통과와 별도로 검증한다.

### 2026-10-05 16:31 KST 후속 검증 — R5-2 독립 검사 통과

**완료 범위와 근거**

- 사용자 C++ 빌드: `C:/Users/Min/AppData/Local/UnrealBuildTool/Log.txt`, 시작 16:19:53 KST, 종료 16:20:35 KST, `DreamCatcherEditor Win64 Development`, `Result: Succeeded`. 신규 장비 계층과 `DCEquipmentAutomationTests.cpp` 컴파일·DLL 링크를 확인했다. Codex가 빌드·패키징한 것이 아니다.
- 에셋 준비: `Saved/Logs/R5EquipmentAutomation-prepare.log`, **16:29:25 KST** 최종 성공. 위 허용 경로의 GA·AS·WID·ID 4개를 Unreal 내부 API로 생성·저장했다. 먼저 생성된 GA는 후속 재실행에서 검증만 하고 보존했다.
- 별도 프로세스 재로드: `Saved/Logs/R5EquipmentAutomation-verify.log`, **16:29:58 KST**, 4개 모두 `fixture_validated`, `mode=verify`, 프로세스 종료 코드 0. 원본 부모 타입·AbilitySet·WeaponInstance·EquippableItem·초기 Stat 7을 확인했다.
- 독립 런타임 검사: `Saved/Logs/R5EquipmentAutomation-runtime.log`, **16:31:34 KST**, `Test Completed. Result={Success}`. `Saved/Automation/R5Equipment/index.json`에 `succeeded=1`, `failed=0`, `succeededWithWarnings=0`, 해당 테스트 `warnings=0`, `errors=0`. 프로세스 종료 코드 0.
- 확인한 동작: 기본 3슬롯, 아이템 생성, 장비 Actor 생성·Character Mesh 부착·상대 위치, 실제 아이템 Instigator, Ability SourceObject 및 FromEquipment 조회, 동일·잘못된 슬롯 선택 시 중복 없음, 빈 슬롯 건너뛰기, 재장착 시 아이템 Stat 6 유지, 해제·Manager 파괴 시 Actor/Ability 회수, Inventory 제거.

**실행 중 수정한 범위**

- `Scripts/Editor/r5_equipment_automation.py`만 교정했다. `UBlueprint.ParentClass`는 Python 직접 조회가 보호되어 있어 UE 5.8의 `GetBlueprintParentClass` API를 사용한다.
- `FDCAbilitySet_GameplayAbility`의 `EditDefaultsOnly` 필드는 Python의 분리된 구조체 setter로 수정할 수 없었다. Unreal의 `import_text`로 구조체 기본값을 만든 뒤 새 DataAsset에 할당·검증했다. C++ 프로퍼티 제한을 완화하지 않았다.
- GameplayTag Python 래퍼의 객체 비교를 Unreal `export_text` 값 비교로 바꿨다. 게임 태그나 입력 설정은 변경하지 않았다.
- 샌드박스 첫 실행에서는 캐시 접근 오류가 있어 권한 승인된 별도 프로세스로 실행했다. 프로젝트 캐시 Config를 바꾸지 않았다. 초기 스크립트 실패는 최종 성공 로그와 구분한다.
- 이번 후속 검증에서는 C++·Config·기존 게임 에셋·Git 인덱스를 수정하지 않았다. DLL 변경은 사용자 빌드 결과이며, 새 검증 에셋 4개와 자동화 스크립트 수정 및 문서 갱신이 Codex 작업 범위다.

**미검증 / 다음 변경안**

- 통과 범위는 임시 Game 월드의 단일 권한 실행이다. 실제 PlayerState 소유 ASC·PawnExtension 해제 순서, 실사용 Pawn/Controller 교체, PIE, 클라이언트 복제, 라이플 외형·사운드·발사·장치 피드백은 미검증이다. 기존 활성 경로는 아직 교체·삭제하지 않았다. R5 전체 완료가 아니다.
- 다음은 **R5-3 원본 라이플 의존성을 위한 R6 코드와 Reticle 선행 타입**이다. 원본 `LyraRangedWeaponInstance`, `LyraGameplayAbility_RangedWeapon`, `LyraWeaponStateComponent`, `LyraAbilityCost_ItemTagStack`, `LyraReticleWidgetBase`, `InventoryFragment_ReticleConfig`를 별도 원본 계층으로 연결하는 변경안을 제시한다. R7의 Reticle 기반 타입을 앞당기는 것이며 HUD 전체 완료로 취급하지 않는다.
- 실제 확인 근거: 원본 RangedWeapon은 WeaponInstance 상속과 Camera BlendInfo를 사용하고, WeaponStateComponent가 현재 장착 무기의 Tick을 호출한다. ItemTagStack 비용은 FromEquipment의 실제 Inventory Item을 소비한다. ReticleConfig는 원본 ReticleWidget 클래스를 저장하고, 원본 Widget은 WeaponInstance/InventoryInstance와 원본 퍼짐 조회를 사용한다. 기존 자체 API에 강제로 맞추지 않는다.
- 충돌 선행 확인: 원본 발사는 `Lyra_TraceChannel_Weapon`(`ECC_GameTraceChannel2`)를 사용한다. 현재 Config에는 AimAssist용 5번 채널만 선언되어 있어, 원본 충돌 채널·프로필과 대상 메시의 응답을 후속 설정 범위로 제시해야 한다. 원본은 무기 trace가 Physics Asset을 맞히고 capsule trace를 별도 채널로 구분한다. 현재 사격과 명중 동작이 동일하다고 가정하지 않는다.
- 원본 라이플 BP 그래프·데이터·전체 종속 에셋과 충돌 응답은 추가 검수가 필요하다. C++ 기반 준비 전 무리하게 로드·저장하거나 임의 수치/그래프로 재제작하지 않는다. **다음 신규 코드·Config 변경은 아직 적용하지 않았으며 승인 대기**다.

### 2026-10-05 후속 코드 준비 이력 — 아래 16:53 검증 결과와 함께 읽을 것

사용자가 앞서 제시한 선행 코드 묶음의 진행을 승인했다. 기존 활성 경로를 교체하지 않고 아래 원본 계층을 별도로 추가했다. C++ 빌드·패키징은 실행하지 않았다.

**원본 → 대상 (6쌍 + 상수 헤더 = 13개 파일)**

| Lyra 원본 | DreamCatcher 대상 (`Source/DreamCatcher/` 아래) |
|---|---|
| `Weapons/LyraRangedWeaponInstance.*` | `Weapon/Lyra/DCLyraRangedWeaponInstance.*` |
| `Weapons/LyraGameplayAbility_RangedWeapon.*` | `Weapon/Lyra/DCLyraGameplayAbility_RangedWeapon.*` |
| `Weapons/LyraWeaponStateComponent.*` | `Weapon/Lyra/DCLyraWeaponStateComponent.*` |
| `AbilitySystem/Abilities/LyraAbilityCost_ItemTagStack.*` | `AbilitySystem/Abilities/DCLyraAbilityCost_ItemTagStack.*` |
| `UI/Weapons/LyraReticleWidgetBase.*` | `UI/Weapons/Lyra/DCLyraReticleWidgetBase.*` |
| `Weapons/InventoryFragment_ReticleConfig.*` | `Weapon/Lyra/DCInventoryFragment_ReticleConfig.*` |
| `Physics/LyraCollisionChannels.h` | `Physics/DCLyraCollisionChannels.h` |

- 기존 원본 이식본 `DCLyraWeaponInstance`·FromEquipment·EquipmentManager·Inventory와 연결했다. Camera는 `UDCCameraComponent`, Team은 `UDCTeamSubsystem`, AbilitySource/TargetData는 기존 `IDCAbilitySourceInterface`/`FDCGameplayAbilityTargetData_SingleTargetHit`, 물리 머티리얼은 `UDCPhysicalMaterialWithTags`, 로그는 `LogDCAbilitySystem`을 재사용한다.
- 이름·include 경로/명시적 include·주석·공백을 제외한 원본 토큰 대조에서 **13개 모두 일치**했다. 자유 함수와 console 변수 namespace는 이름 충돌 방지용 `DCLyra` 이름을 쓰며, `lyra.Weapon.*` console 명령 문자열과 GameplayTag 문자열은 원본 그대로다. Epic 저작권 표기도 유지했다.
- 원본의 열/퍼짐 곡선·상태 배율·첫발 정확도·거리/물리 머티리얼 감쇠, TargetData 생성·예측 창·콜백/해제, WeaponState의 무기 Tick·명중 표시 흐름, 아이템 비용의 수량 버림·권한 측 차감·실패 태그, Reticle의 화면 픽셀 반경 계산을 보존했다.
- 기존 자체 발사/Reticle API에 맞추는 어댑터를 추가하지 않았다. 원본 TargetData 이후의 효과 적용과 발사 간격·재장전은 원본 Blueprint 책임으로 남겼다. 원본의 총구 위치 보정·벽 근처 타기팅 TODO와 TargetData 검증 수준도 변경하지 않았다.
- `DCLyraCollisionChannels.h`는 원본 1~4번 상수만 정의한다. **충돌 채널/프로필을 Config에 등록한 것이 아니며** 현재 사격 채널을 바꾸지 않는다. 원본 라이플 실사용 전에 채널·물리 에셋·벽/엄폐 응답을 별도 연결/검증한다.

**새 독립 검사 준비 — 아직 컴파일·실행하지 않음**

- 파일: `Source/DreamCatcher/Tests/DCWeaponFoundationAutomationTests.cpp`. 원본 게임 코드가 아니라 프로젝트 검증 코드다. 기존 R5 테스트 파일과 에셋을 수정하지 않는다.
- `DreamCatcher.R6.Weapon.SpreadAndAttenuation`: 원본 C++ 기본값, 합성 열 곡선의 장착 중간값·발사 증가·냉각·최솟값, 첫발 정확도 배율, 거리 곡선과 두 물리 태그 배율의 곱을 검사한다.
- `DreamCatcher.R6.Weapon.ItemTagCost`: 임시 ASC/FromEquipment/Inventory Item으로 수량 2.9의 버림(2), 7→5→3→1 차감, 부족 시 실패 태그·추가 차감 없음, 연결 아이템이 없을 때 실패를 검사한다.
- 합성 곡선·수량은 **테스트 전용**이며 원본 라이플 BP 값으로 추정하거나 실제 에셋에 저장한 것이 아니다. 테스트는 별도 프로세스의 `-DCWeaponIsolatedAutomation` 플래그가 필요하다.
- 실제 발사 타기팅, 재장전, 화면 투영·Reticle 시각, 조준/웅크리기/낙하 전이, 서버/클라이언트 예측·복제, PlayerState 소유 ASC 통합까지 검증하는 테스트가 아니다.

**원본 소스에서 발견한 추가 확인 항목**

- 원본 `LyraRangedWeaponInstance::UpdateSpread()`는 private `LastFireTime`을 읽지만 해당 원본 `.h/.cpp`에서는 초기값 0.0 외 갱신을 찾지 못했다. 발사 Ability가 호출하는 부모 `LyraWeaponInstance::UpdateFiringTime()`은 별개 변수 `TimeLastFired`를 갱신한다.
- 이 구조는 그대로 보존했다. 위 합성 냉각 검사는 기본 지연 0만 다루므로 **실제 라이플의 발사 후 회복 지연을 검증한 것으로 해석하지 않는다.** 후속 절차는 원본 `B_WeaponInstance_Rifle`의 지연 값 확인 → 발사 전후 두 시간 경로/냉각 재현 → 필요 시 원본 보존 대안과 실제 동작 차이를 설명하고 수정 승인 받기다.

**재개 절차:** 사용자 `DreamCatcherEditor Win64 Development` 빌드 → Codex가 새 클래스 로드와 독립 검사 2건 및 R5 수명 검사의 회귀 실행 → 원본 라이플 Blueprint/종속 에셋·충돌 설정 사전 검수 → 승인된 에셋/Config 연결 작업. 현재 단계는 선행 코드 준비이지 R5-3 라이플, R6 발사, R7 HUD 완료가 아니다. 기존 구현 삭제·활성 경로 전환·에셋 저장·Build.cs/Config 변경은 이번에 하지 않았다.

### 2026-10-05 16:53~17:00 KST — 선행 검사 통과, 원본 라이플 조사

**이번 완료 근거**

- 사용자 빌드: UBT `Log.txt`의 16:51:00~16:51:22 KST, `DreamCatcherEditor Win64 Development`, `Result: Succeeded`. 신규 RangedWeapon·발사·WeaponState·비용·Reticle·Fragment와 `DCWeaponFoundationAutomationTests.cpp` 컴파일 및 DLL 링크를 확인했다.
- `Saved/Logs/R6WeaponFoundation-runtime.log`, 16:53:47 KST에 `QuickBarLifecycle`, `ItemTagCost`, `SpreadAndAttenuation` 모두 `Result={Success}`. `Saved/Automation/R6WeaponFoundation/index.json`: 성공 3, 실패 0, 경고 동반 성공 0, 각 테스트 경고·오류 0. 프로세스 종료 코드 0.
- 이 결과는 합성 곡선/데이터 기반 독립 테스트와 기존 R5 장비 수명 회귀다. 원본 라이플 BP 실행, TargetData 실제 명중, 재장전·자동 재장전, 화면 투영·조작감, 실사용 PlayerState/ASC 연결, 네트워크 복제는 미검증이다.
- 이번에 C++·Config·원본/대상 게임 에셋·Git 인덱스를 변경하지 않았다. 새 읽기 전용 조사 스크립트 `Scripts/Editor/r5_rifle_source_audit.py`와 이 문서의 인계 갱신만 추가했다. 빌드·패키징은 실행하지 않았다.

**원본 데이터·그래프 조사**

- 원본 Lyra의 별도 Python commandlet으로 수행했다. 원본 Build ID는 엔진과 일치했으며, Python/EditorScriptingUtilities는 프로세스 옵션으로만 활성화했다. `.uproject` 변경·에셋 Compile/Save·Migrate는 호출하지 않았다.
- 근거: `E:/Epic Games/UEProjects/LyraStarterGame/Saved/Logs/R5RifleSourceAudit.log`, 최종 실행 **17:00:53 KST**, `DC_RIFLE_AUDIT complete`, 에셋 쓰기 0. 부모/선택 프로퍼티 조회 14개, 그래프 25개·노드 510개에서 핀 값과 연결을 읽었고, 최종 실행의 그래프/노드 읽기 실패는 0이다. 에셋 종속성 전체의 폐쇄 집합이나 모든 시각 동작까지 검증한 것은 아니다.
- `Graph.Nodes`는 Python 직접 프로퍼티 접근이 거부되어, 공개 `ObjectIterator`로 로드된 노드의 Graph Outer를 대조하고 BlueprintEditorLibrary/GraphPin API로 핀을 읽었다. 접근 플래그나 그래프를 수정하지 않았다. 최초 조회 실패 로그와 최종 성공을 구분한다.
- `ID_Rifle`: EquippableItem·QuickBarIcon·SetStats·PickupIcon·ReticleConfig의 5개 Fragment. MagazineSize **30**, MagazineAmmo **30**, SpareAmmo **60**. ReticleWidgets는 `W_Reticle_Rifle`와 `W_AmmoCounter_Rifle`이며 두 위젯의 네이티브 부모는 원본 LyraReticleWidgetBase다.
- `WID_Rifle`: `B_WeaponInstance_Rifle`, `AbilitySet_ShooterRifle`, `B_Rifle`, `weapon_r` 소켓과 원본 부착 Transform을 참조한다.
- `AbilitySet_ShooterRifle`: 자동 발사(`InputTag.Weapon.FireAuto`), 재장전(`InputTag.Weapon.Reload`), 무입력 태그의 `GA_Weapon_AutoReload` 3개 Ability. 자동 재장전은 **OnSpawn / LocalOnly**, PollInterval **0.25**, TimeSinceActivityToReload **약 0.66**이다.
- 실제 자동 라이플 자식 CDO: `FireDelayTimeSecs` **약 0.12**, `AutoRate=1`, 비용 **MagazineAmmo 1**, `bOnlyApplyCostOnHit=false`, `GE_Damage_RifleAuto`. 부모 GA_Weapon_Fire의 기본 0.10 값을 자식의 실제 값으로 혼동하지 않는다.
- `B_WeaponInstance_Rifle`: `SpreadRecoveryCooldownDelay` **약 0.15**, SweepRadius **5.5**, MaxDamageRange **25000**, FirstShotAccuracy 허용. 곡선 키·외삽 설정은 로그의 전체 Unreal 구조체 내보내기 값을 보존하며, C++ 기본값이나 임의 직선으로 재작성하지 않는다. 앞서 발견한 두 발사 시간 변수 경로 문제는 이 비영 지연 값으로 원본 런타임 재현이 필요하다.
- 재장전 부모 그래프는 `GameplayEvent.ReloadDone`을 `WaitGameplayEvent`로 기다리고 권한 측 `ReloadAmmoIntoMagazine`에서 실제 아이템 스택을 옮긴다. 단순 Delay로 대체하지 않는다. 몽타주/Notify 의존성 확인과 실제 실행은 후속이다.

**원본 조사 후 제시한 변경안 — CharacterParts 코드의 후속 적용은 아래 인계 참고**

1. 원본 `Cosmetics/LyraCharacterPartTypes.h`, `LyraPawnComponent_CharacterParts.h/.cpp` **3개 파일**을 별도 `DCLyra` 이름으로 이식한다. 기존 `DCLyraCosmeticAnimationTypes`를 재사용한다. R8의 외형/애니메이션 선행 타입을 앞당기는 것이며, 현재 캐릭터 외형이나 메시를 교체하는 작업은 아니다.
2. 근거: 원본 `B_WeaponInstance_Base.DetermineCosmeticTags`는 `LyraPawnComponent_CharacterParts`를 조회하고 `GetCombinedTags(Cosmetic.AnimationStyle)`로 애니메이션 스타일을 선택한다. 현재 프로젝트에는 이 타입이 없어 해당 참조를 그대로 가져오면 타입 해결이 불완전하다. 이는 선행 타입 미준비이지 이식 불가가 아니다.
3. 원본 보존 대안은 해당 컴포넌트를 재사용하는 것이다. 태그 조회 제거/상수 대체는 동적 스타일 선택을 잃고, 원본 Hero 전체를 플레이어로 교체하는 것은 현재 승인 범위를 크게 확장하므로 선택하지 않는다. 원본 컴포넌트 추가만으로 실제 Pawn에 자동 부착하거나 외형 파트를 생성하지 않는다.
4. 별도로 이식본 `B_WeaponInstance_Base`의 장비 해제 그래프를 최소 연결할 필요가 있다. 원본은 `GetTypedPawn(B_Hero_ShooterMannequin)` → 원본 HealthComponent → IsDeadOrDying를 사용한다. DreamCatcher Pawn은 그 BP의 자식이 아니므로 타입 조회가 실패할 수 있다. 대상 Character에 같은 이름의 `UDCLyraHealthComponent`가 있음을 확인했다. 원본 에셋을 보존한 채 이식본의 PawnType만 DreamCatcher Character 계층으로 연결하는 안을 검토한다. 사망 시 장비 해제 애니메이션 처리 규칙을 바꾸려는 것이 아니며 컴파일/사망·해제 회귀 검증이 필요하다.
5. 이후 원본 라이플·부모 BP·AutoReload·Reticle/AmmoCounter·몽타주/Notify·Cue의 종속성 및 리다이렉트/충돌 Config 범위를 확정하고 에셋 이식을 승인받는다. 현재 14개 조사만으로 모든 종속 에셋의 이식 준비가 끝났다고 판단하지 않는다. 기존 플레이어 자동 장착·HUD·발사 경로는 여전히 보존한다.

### 2026-10-05 CharacterParts 코드 준비 이력 — 아래 후속 검증 결과 참고

사용자가 CharacterParts 원본 코드 3개 파일 이식을 승인했다. R5-3 라이플 Blueprint 의존성을 위해 R8의 선행 타입 일부를 준비한 것이며, 현재 플레이어 외형 교체나 R8 완료를 의미하지 않는다.

- 원본: `LyraStarterGame/Source/LyraGame/Cosmetics/LyraCharacterPartTypes.h`, `LyraPawnComponent_CharacterParts.h/.cpp`.
- 대상: `Source/DreamCatcher/Cosmetics/DCLyraCharacterPartTypes.h`, `DCLyraPawnComponent_CharacterParts.h/.cpp`.
- 최소 변경: 클래스·구조체·delegate·enum·파일 이름을 `DCLyra`로 분리하고, 기존 `FDCLyraAnimBodyStyleSelectionSet`으로 연결했다. `ChildActorComponent`, `SceneComponent`, `SkeletalMesh`, `PhysicsAsset`, `World` 등의 직접 사용 타입은 명시적 include를 추가했다. Epic 저작권 표기를 유지했다.
- 보존한 원본: FastArray 데이터와 복제 콜백, 파트 핸들, 권한 측 추가/제거, dedicated server의 외형 Actor 생성 제외, ChildActor 부착·충돌 설정·Tick 선행 조건, 태그 인터페이스 조회/접두어 필터, BodyMeshes 선택·변경 delegate, EndPlay 정리. 원본 게임 로직을 단순 태그 getter로 축소하지 않았다.
- 정적 검사: 이름 치환과 include·주석·공백을 제외한 원본 코드 토큰이 **3개 파일 모두 일치**했고, generated 헤더가 각 헤더의 마지막 include인지 확인했다. 컴파일·UHT·실행 성공을 의미하지 않는다.
- **중요한 원본 동작:** `BroadcastChanged()`는 `BodyMeshes.SelectBestBodyStyle()` 결과를 `SetSkeletalMesh`에 그대로 전달한다. 규칙과 DefaultMesh가 비어 있으면 null 메시가 선택될 수 있다. 이 동작을 임의 수정하지 않았으며 현재 실제 Pawn에는 컴포넌트를 부착하거나 파트를 추가하지 않았다. 향후 외형 연결 전에 기본 메시·물리 에셋과 파트 데이터부터 검증한다.
- 기존 소스·Build.cs·Config·에셋·스크립트는 변경하지 않았다. 의존 모듈 `ModularGameplay`·`NetCore`와 기존 Cosmetics 타입을 재사용했다. 원본 Hero 타입 참조의 Blueprint 변경, 리다이렉트와 라이플 이식, 활성 경로 교체·기존 구현 삭제는 아직 하지 않았다.
- C++ 빌드·패키징·새 클래스 로드·런타임·클라이언트 복제는 이번에 실행하지 않았다. 이전 16:53 테스트 통과를 이 새 코드의 검증 결과로 취급하지 않는다.

**다음 절차:** 사용자 `DreamCatcherEditor Win64 Development` 빌드 → Codex가 신규 Reflection 타입/클래스 로드와 기존 R5/R6 검사 3건의 회귀 확인 → CharacterParts의 실제 파트/태그/메시 수명 및 복제 검사 범위를 구분해 준비 → 원본 라이플 에셋·Hero 최소 연결·Config 변경안을 승인받고 진행. 지금 사용자가 수동으로 Pawn 컴포넌트나 Blueprint 노드를 추가할 필요는 없다.

### 2026-10-05 17:23~17:30 KST — CharacterParts 준비 확인과 이식 경로 조사

**확인된 완료 범위**

- 사용자 UBT 로그: 시작 **17:23:05 KST**, 약 26.69초, `DreamCatcherEditor Win64 Development`, CharacterParts CPP와 모듈 컴파일·DLL 링크 후 `Result: Succeeded`. Codex는 빌드·패키징하지 않았다.
- `Saved/Logs/R5CharacterParts-audit.log`, **17:26:53 KST**, 새 컴포넌트 클래스와 `DCLyraCharacterPart`, `DCLyraCharacterPartHandle`, `DCLyraAppliedCharacterPartEntry`, `DCLyraCharacterPartList`, `EDCLyraCharacterCustomizationCollisionMode` 로드 성공. CDO에서 `GetCombinedTags`가 빈 컨테이너, `GetCharacterPartActors`가 빈 목록임을 확인했다. 에셋 쓰기·Pawn 부착·파트 생성 없음, 종료 코드 0.
- `Scripts/Editor/r5_equipment_automation.py`의 읽기 전용 audit에 위 검사를 추가했다. 기존 에셋 생성 allowlist 4개는 늘리지 않았다. 이 결과는 CharacterParts의 실제 파트 추가/제거·태그 제공 Actor·메시 선택·복제 검사가 아니다.
- `Saved/Logs/R5CharacterParts-regression.log`, **17:28:24 KST**, 기존 `QuickBarLifecycle`, `ItemTagCost`, `SpreadAndAttenuation` 회귀 성공. `Saved/Automation/R5CharacterPartsRegression/index.json`에 성공 3, 실패·경고 0, 프로세스 종료 코드 0. 이 3건은 새로운 CharacterParts 수명 테스트가 아니다.
- C++·Config·원본/대상 에셋·Git 인덱스는 이번 확인 작업에서 변경하지 않았다. 검사/조사 스크립트와 두 기준 문서만 갱신했다. 사용자에게 추가 빌드나 수동 Pawn/Blueprint 연결을 요구하지 않는다.

**읽기 전용 종속성/충돌 조사**

- `r5_rifle_source_audit.py -DCDependencyPlan`은 AssetRegistry의 hard/soft 패키지 참조만 순회한다. 원본 Hero 참조를 삭제하거나 종속성을 임의 제외하지 않았으며, Engine/Script 패키지는 외부 제공 항목으로 별도 집계한다. 에셋 저장·복사·Migrate를 호출하지 않는다.
- 근거: 원본 프로젝트 `Saved/Logs/R5RifleDependencyPlan.log`, **17:30:49 KST**, 조사 패키지 **1,434**: Game 1,140 / ShooterCore 29 / Niagara 207 / ControlRig 56 / AnimationLocomotionLibrary 1 / AudioModulation 1. 별도 Engine/Script 참조 76개. 엔진 플러그인 콘텐츠를 전부 프로젝트로 복사해야 한다는 뜻이 아니다.
- 현재 DreamCatcher의 같은 `/Game` 경로에 존재하는 파일은 **116개**다. 예: SK_Rifle, MI_Weapon_Rifle, 라이플 muzzle flash Niagara, Reticle 머티리얼. 경로 존재만 확인했으며 바이트 동일성·사용자 수정 유무까지 확인한 것은 아니다. 덮어쓰기/동일 에셋 재사용 판단 근거로 사용하지 않는다.
- 이 조사는 **최종 마이그레이션 목록이 아니다.** 원본 Hero 참조로 이어지는 종속성도 포함하고, GameplayCue 태그를 통한 동적 탐색은 완전하지 않다. 특히 `/Game/GameplayCueNotifies/GCN_Weapon_Impact`가 파일로 존재함을 확인했지만 해당 실행의 패키지 참조 목록에는 포함되지 않았다. Cue 등록/태그 매칭을 확인해 명시적 후보로 추가해야 한다.

**다음 변경안 — 아직 실행하지 않음**

1. 원본 프로젝트와 DreamCatcher의 **신규 `/Game/LyraMigration/Rifle` 전용 폴더**를 후보로 사용한다. 실제 시작 전 폴더 존재·저장 충돌을 재확인하며, 기존 원본과 대상 에셋을 덮어쓰거나 이동·삭제·Consolidate하지 않는다.
2. 원본 복사본에서 `B_WeaponInstance_Base`의 특정 Hero 조회를 네이티브 Character 계층으로 최소 연결하고 종속성을 다시 계산한다. 원본에서는 LyraCharacter, 대상에서는 기존 ClassRedirect를 통해 DreamCatcherCharacter가 되는 경로를 우선 검토한다. HealthComponent/사망 검사 규칙은 유지하며 원본 원위치 에셋은 변경하지 않는다. 복사본의 컴파일과 참조 차이를 확인한 뒤 최종 복사 목록을 확정한다.
3. 라이플 Item/Equipment/AbilitySet, 발사·재장전·AutoReload 및 부모 BP, Reticle/AmmoCounter, 필요한 몽타주/Notify·Cue·시각 의존성을 Unreal 내부 API로 이식하는 작업안을 제시한다. 기존 에셋과 경로가 겹치면 조용히 덮어쓰거나 건너뛰어 혼합하지 않는다.
4. 준비된 DC/DCLyra 타입으로 읽기 위한 **선별 Class/Struct/Enum/Function 리다이렉트와 누락 GameplayTag**가 후속 Config 변경 범위다. 이전 단계의 리다이렉트를 통째로 교체하지 않는다. 실제 추가 항목을 확인하고 승인된 범위에서만 적용한다.
5. 이번 에셋 준비 묶음의 목표는 이식본 로드·Blueprint Compile·부모/곡선/태그/참조·별도 프로세스 재로드 검사다. 실제 PawnData/무기 자동 장착·입력·HUD·CharacterParts 부착·충돌 프로필 전환과 기존 구현 제거는 포함하지 않는다. 그 연결은 준비된 에셋 검증 후 별도 회귀 범위로 제시한다.

### 2026-10-05 17:44~18:05 KST — 승인된 에셋 준비 시도, 이식 미완료

사용자가 신규 전용 폴더 이식과 필요한 리다이렉트/태그 Config 작업 묶음을 승인했다. C++ 추가 구현·빌드·패키징·기존 에셋 덮어쓰기·활성 장착 전환·기존 구현 삭제는 이 승인에 포함하지 않는다.

**준비된 부분**

- 신규 스크립트 `Scripts/Editor/r5_rifle_stage.py`로 원본 프로젝트 `/Game/LyraMigration/Rifle/Seed`에 원본 Item/Equipment/AbilitySet, 무기 부모/자식, 발사/재장전/AutoReload, Reticle/AmmoCounter, Fire/Impact Cue의 핵심 복사본 16개를 만들었다. 기존 원본 경로 에셋은 저장하지 않았다.
- 복사본 `B_WeaponInstance_Base`의 EventGraph에서 `GetTypedPawn`의 PawnType 핀 **1개**를 `B_Hero_ShooterMannequin_C`에서 `/Script/LyraGame.LyraCharacter`로 바꾸고 Compile/Save를 통과했다. HealthComponent/IsDeadOrDying 흐름은 유지했다. 대상 프로젝트에는 기존 LyraCharacter→DreamCatcherCharacter 리다이렉트가 있다.
- AbilitySet·WID·ID 참조를 준비용 복사본으로 연결했고, 새 프로세스 재로드/종속성 조회에서 원본 Hero BP 참조가 사라졌음을 확인했다. 이때 프로젝트 패키지 969, 엔진 플러그인 포함 1,228개였다. 최종 배포/이식 목록이 아니며 뒤의 B_Rifle 복구 이후 재계산해야 한다.
- 처음 저장 직후 종속성 레지스트리가 None을 반환해 조회 단계가 실패했다. 새 프로세스 `R5RifleStage-plan.log`에서는 조회가 정상화됐다. 이 조회 실패와 이후 저장 충돌은 별개다.

**발견한 손실과 복구 방향**

- `B_Rifle` 복사본을 복제된 Actor 부모로 직접 Reparent한 뒤, 원래 의존하던 SK_Rifle/MI_Weapon_Rifle/ABP_Weap_Rifle 참조가 목록에서 빠졌다. 이 엔진 경로에서 상속 SCS 컴포넌트 override가 보존되지 않는 증거다. 데이터-only 무기 수치와 Actor 컴포넌트 override를 동일하게 취급하지 않는다.
- 기존 원본은 변경하지 않고 **`Seed/Preserved/B_Rifle`**을 원본 부모를 유지한 새 복사본으로 만들었으며, 준비용 WID는 이 경로로 연결했다. 최초 `Seed/B_Rifle`은 실패 비교용 잔여물로 남아 있고 실사용/이식 대상이 아니다. Seed 폴더에는 이 잔여물을 포함해 17개 파일이 있다.
- 이후 전체 부모/종속성 참조 치환은 Unreal Advanced Copy에 맡기는 방식으로 시도했다. 원본 Actor 설정을 C++/BP에서 수작업 재제작하거나 시각 기능을 제거하지 않았다.

**종속성 복사 결과 — 모두 미완료**

| 로그 / 원본 프로젝트 신규 출력 폴더 | 결과 |
|---|---|
| `R5RifleStage-copy.log` / `Rifle/Content` | 일반 Advanced Copy는 성공 콜백을 반환했으나 후속 개별 저장 중 접근 위반 충돌. 6개 파일만 저장됨 |
| `R5RifleStage-payload.log` / `Rifle/Payload` | 프로세스 한정 `AssetTools.UseHeaderPatchingAdvancedCopy=1` 시험. 다수 패키지에 `Unknown section`; 83개 파일만 생성, 프로세스 종료 코드 1. 당시 스크립트 complete/성공 콜백은 완료 근거로 사용하면 안 됨 |
| `R5RifleStage-prepared.log` / `Rifle/Prepared` | 일반 복사 결과의 객체를 유지하고 971개를 일괄 저장하도록 재시도. 747개 파일 저장 후 접근 위반 충돌(종료 코드 3). 마지막 저장 로그는 `Audio/Blueprints/B_MusicManagerComponent_Base`이나 근본 원인 확정은 아님 |

- 부분 출력은 모두 **원본 Lyra 프로젝트의 신규 `Content/LyraMigration/Rifle/` 아래**다. Content/Payload/Prepared/Seed를 합쳐 853개 파일, 약 1.24 GB이며 삭제하지 않았다. 747개나 성공 콜백만 보고 완전한 에셋 집합으로 간주하지 않는다. 원본 재사용이 불가능하다는 판정도 아니다.
- 저장 로그 787건에서 허용된 `/Game/LyraMigration/Rifle/` 밖의 Save Package 호출은 확인되지 않았다. 확인한 원본 B_WeaponInstance_Base/ID_Rifle 파일 수정 시각도 기존 값이다. 파일명/개수 검사만으로 모든 내용의 동일성을 검증했다는 뜻은 아니다.
- **DreamCatcher의 `Content/LyraMigration/Rifle`는 아직 생성되지 않았으며 Migrate를 실행하지 않았다. Config와 C++도 이번 시도에서 변경하지 않았다.** 사용자 빌드·패키징을 대신 실행하지 않았다.

**추가로 확인한 네이티브 연결 제약 (복사 충돌과 별개)**

1. 원본 B_Weapon 그래프는 `ObserveTeamColors`를 사용한다. 원본은 `Teams/AsyncAction_ObserveTeamColors.h/.cpp`의 `UCancellableAsyncAction`으로 현재 팀을 즉시 알리고 이후 팀·표시 에셋 변경을 관찰한다. DreamCatcher에는 이 클래스가 없다. 다음 권장안은 기존 DCTeam 타입에 최소 연결해 **원본 2개 파일을 이식**하는 것이다. 노드를 삭제하거나 색상을 고정하면 원본 동적 팀 색상 동작이 달라지므로 선택하지 않는다.
2. 원본 W_Reticle_Rifle의 Elimination 메시지 필터는 `LyraPlayerState.GetLyraPlayerController()`를 호출한다. 원본 함수는 `Cast<ALyraPlayerController>(GetOwner())`이며, 현재 DCPlayerState에 대응 Blueprint getter가 없다. 원본과 같은 DC 전용 Controller cast getter 추가를 권장한다. 일반 APlayerState.GetPlayerController로 치환하면 반환 타입 범위가 넓어지므로 동일 동작이라고 단정하지 않는다.
3. 위 C++ 의존성 추가는 현재 에셋·Config 묶음 범위 밖이라 **적용하지 않았다.** 이식본 참조가 해결되려면 별도 코드 승인·사용자 빌드가 필요하다. 이 추가가 Advanced Copy 저장 충돌까지 해결한다고 주장하지 않는다.

**안전한 재개 조건**

- 먼저 부분 결과를 식별하고 보존한다. `copy` 모드는 기존 출력 폴더가 있으면 중단하도록 되어 있으며, 승인 없이 폴더를 삭제하거나 덮어써서 재시도하지 않는다.
- 원본 추가 C++ 2개 파일 + PlayerState getter의 승인 여부를 확인한다. 복사 충돌은 특정 패키지/저장 순서/부모 참조를 분리해 진단하며, Editor 전용 보조 코드가 필요하면 범위를 먼저 설명하고 별도 승인받는다.
- 최종 패키지 집합·원본 값·부모/컴포넌트 참조·저장 후 별도 프로세스 재로드가 검증되기 전에는 DreamCatcher Migrate/Config 적용/활성 경로 전환을 진행하지 않는다. 이번 단계는 **부분 준비, 차단 원인 확인**이며 완료가 아니다.

### 2026-10-05 TeamColor 코드 준비 및 오류 수정 이력 — 아래 후속 검증 참고

사용자가 앞서 제시한 추가 C++ 묶음을 승인했다. 이 변경은 원본 Blueprint의 네이티브 의존성 준비이며, 에셋 복사 저장 충돌 수정은 아니다.

**구현 범위**

- 원본 `LyraStarterGame/Source/LyraGame/Teams/AsyncAction_ObserveTeamColors.h/.cpp` → `Source/DreamCatcher/Teams/DCLyraAsyncAction_ObserveTeamColors.h/.cpp`.
- 클래스/파일/delegate 이름을 분리하고, 기존 `IDCTeamAgentInterface`·`UDCTeamStatics`·`UDCTeamSubsystem`·`UDCTeamDisplayAsset`에 최소 연결했다. Epic 저작권, GameInstance 등록, 초기 알림, 팀/표시 에셋 변경 관찰과 원본 취소 처리 코드를 유지했다. 이름 치환과 include·주석·공백을 제외한 코드 대조에서 2개 파일 모두 원본과 일치했다.
- `Source/DreamCatcher/Player/DCPlayerState.h/.cpp`: 원본 `ALyraPlayerState::GetLyraPlayerController()`에 대응하는 BlueprintCallable `GetDCPlayerController()` 추가. 구현은 `Cast<ADreamCatcherPlayerController>(GetOwner())`이며, 전용 Controller가 아닌 Owner에는 nullptr을 반환한다. 기존 ASC·PawnData·팀·StatTags 동작은 변경하지 않았다.
- `Source/DreamCatcher/Tests/DCTeamColorAutomationTests.cpp`: `DreamCatcher.R5.TeamColor.ObserverAndController` 검사 준비. 임시 Game 월드에서 null/generic/project Controller Owner 구분, null agent 처리, 팀/표시 에셋 delegate 구독과 1→2→NoTeam 전환, NoTeam 상태에서 Cancel 시 팀 delegate 해제, 팀 인터페이스 없는 Actor의 종료 경로를 확인하도록 작성했다.
- 테스트는 `-DCR5IsolatedAutomation`을 지정한 별도 프로세스에서만 허용한다. 실제 Pawn Possess/HUD/게임플레이 BeginPlay를 실행하지 않으며, 팀 색상 시각 출력·클라이언트 복제·실제 BP 연결·팀에 속한 상태에서 Cancel하는 경우까지 검증하는 것은 아니다. 원본 취소 코드를 임의 보강하지 않았다.

**검증과 다음 단계**

- 정적 원본 대조와 diff/공백 검사는 통과했다. **C++ 빌드·UHT·새 검사 실행은 아직 하지 않았다.** 과거 3건의 성공을 이번 변경의 회귀 성공으로 간주하지 않는다.
- Config·에셋·Build.cs·스크립트·Git 인덱스는 변경하지 않았다. 신규 관찰 노드 2개·기존 PlayerState 2개 수정·테스트 1개와 문서 갱신이 이번 범위다.
- 향후 리다이렉트 연결 대상은 `/Script/LyraGame.AsyncAction_ObserveTeamColors` → `/Script/DreamCatcher.DCLyraAsyncAction_ObserveTeamColors`, 기존 LyraPlayerState Controller getter → DCPlayerState.GetDCPlayerController다. 이 Config 항목은 아직 적용하지 않았다.
- 사용자 빌드 후 Codex가 새 타입/함수 로드와 신규 검사 1건 + 기존 R5/R6 검사 3건을 실행한다. 에셋 복사 충돌과 부분 결과는 이전 인계대로 보존하고 별도 진단한다. 이 코드 추가만으로 복사 충돌이나 원본 라이플 이식이 완료된 것이 아니다.

**후속 사용자 빌드 오류 및 수정 — 재빌드 대기**

- 사용자 첨부 로그에서 `DCTeamColorAutomationTests.cpp`의 `IsBoundToObject` 호출 6곳에 C2039가 발생했다. `TestTrue/TestFalse` 인수 오류 C2661은 이 잘못된 표현식에 따른 연쇄 오류다. UBT 결과도 `Failed (OtherCompilationError)`이며 빌드 성공으로 기록하지 않는다.
- Codex가 작성한 테스트에서 단일 `TScriptDelegate` API를 동적 multicast에 잘못 적용했다. 로컬 UE 5.8 `ScriptDelegates.h`의 `TMulticastScriptDelegate::GetAllObjects()`와 `TDynamicMulticastDelegate`의 public 상속을 확인하고, 6곳을 `GetAllObjects().Contains(Observer.Get())`로 교정했다. 관찰 객체가 구독 중인지 확인하는 검사 목적은 같다.
- 변경 범위는 테스트 파일과 인계 문서뿐이다. 원본 관찰 노드·PlayerState getter·Config·에셋은 이번 오류 수정에서 변경하지 않았으며, C++ 빌드나 검사를 실행하지 않았다. 사용자가 다시 빌드한 뒤 새 로그를 확인한다.

### 2026-10-05 18:29~18:41 KST — 4건 검사 통과와 복사 분리 진단

**사용자 빌드 / 자동 검사**

- UBT `Log.txt`: **18:29:02 KST** 시작, 6.19초 후 `Result: Succeeded`. 수정된 `DCTeamColorAutomationTests.cpp` 컴파일과 DreamCatcher DLL 링크를 확인했다. 이전 C2039/C2661 실패와 구분한다.
- `Saved/Logs/R5TeamColor-regression.log`, **18:31:02 KST**: `QuickBarLifecycle`, `ObserverAndController`, `ItemTagCost`, `SpreadAndAttenuation` 모두 Success. `Saved/Automation/R5TeamColorRegression/index.json`: 성공 **4**, 실패·경고 동반 성공 **0**, 각 테스트 경고·오류 **0**, 프로세스 종료 코드 0.
- TeamColor 검증 범위는 전용 Controller Owner cast 및 구독 경로다. 실제 색상 연출·팀이 설정된 채 Cancel·BP 통합·클라이언트 복제까지 완료된 것은 아니다. 라이플 이식 전체 완료나 복사 충돌 해결도 아니다.

**개별 복사 진단 (기존 승인된 신규 Rifle 폴더 범위)**

- 신규 스크립트 `Scripts/Editor/r5_rifle_copy_diagnostic.py`. 원본 `/Game/Audio/Blueprints/B_MusicManagerComponent_Base`와 `/Game/Audio/Blueprints/WeaponAudioMacros`만 각각 `/Game/LyraMigration/Rifle/Diagnostics/SingleAsset/`에 복사했다. 기존 경로가 있으면 덮어쓰지 않고 중단한다. 원본 Save·Consolidate·Migrate·C++ 빌드·패키징은 호출하지 않는다.
- 원본 프로젝트 `Saved/Logs/R5RifleCopyDiagnostic-single.log`: **18:38:25 KST**까지 두 복사본의 저장 성공, 종료 코드 0. `R5RifleCopyDiagnostic-reload.log`: **18:41:48 KST**까지 별도 프로세스 재로드 성공, 종료 코드 0.
- 진단 로그에는 기존 원본/Death 이식본 사이의 `GameplayCue.Character.Death` 중복 등록 경고가 있다. 이 두 에셋 저장 실패로 해석하지 않으며, 별도 문제로 보존한다.
- 이 결과로 두 에셋이 단독 저장/재로드에서도 항상 실패한다는 가설은 지지되지 않는다. 대량 참조 치환·복사 집합·저장 순서를 분리해서 살펴봐야 한다. **근본 원인은 아직 미확정**이며 두 파일을 제외하거나 음악/오디오 기능을 제거하는 근거가 아니다.
- 기존 Seed/Content/Payload/Prepared 부분 출력은 건드리지 않았다. 이번 진단 파일 2개만 추가했으며, DreamCatcher의 `Content/LyraMigration/Rifle`는 여전히 없다. C++·Config·기존 게임 에셋도 이번 확인 작업에서 수정하지 않았다.

**당시 제안 — Editor 전용 보조 도구 (후속 승인·구현 상태는 아래 최신 인계 참고)**

- 로컬 UE 5.8 `IAssetTools.h`에서 Python에 공개된 `BeginAdvancedCopyPackages`는 내부에서 `bShouldCheckForDependencies=true`로 전체 의존성을 확장한다. 명시적 Source→Destination 맵을 받는 `AdvancedCopyPackages` 오버로드와 destination 검증 API는 C++ public API이지만 UFUNCTION이 아니어서 현재 Python 경로에서는 직접 호출할 수 없다.
- Unreal이 제공하는 이 API를 호출하는 **작은 Editor 전용 플러그인/BlueprintFunctionLibrary**를 DreamCatcher 프로젝트 안에 추가하는 안을 권장한다. 엔진 소스/게임 런타임을 바꾸거나 에셋 바이너리를 자체 편집하는 방식이 아니다. 원본 기능 삭제·축소도 아니다.
- 도구는 명시적 패키지 목록, 신규 `/Game/LyraMigration/Rifle/Diagnostics` 하위 목적지, 기존 목적지 거부, dirty 원본 거부/미저장, 작업 전후 경로·결과 기록을 강제한다. 먼저 소수 패키지만으로 복사·참조 치환·저장을 분리 검증하고, 통과한 범위에서 확장한다. 대량 부분 폴더를 자동 삭제·덮어쓰는 기능을 넣지 않는다.
- 예상 변경은 Editor 전용 `.uplugin`/모듈/보조 라이브러리와 필요한 프로젝트 플러그인 등록이다. 별도 C++ 작업 승인과 사용자 빌드가 필요하며, 아직 파일을 생성하지 않았다. 기존 `PluginManager.cpp`의 명령줄 `-PLUGIN=` 지원을 확인해 원본 프로젝트를 수정하지 않고 같은 엔진의 도구를 로드하는 경로를 검토할 수 있다.
- 수동 대량 복사는 같은 엔진 경로를 사용하므로 안전한 해결책으로 보장하지 않는다. 도구를 만들어도 충돌이 반드시 해결된다는 뜻은 아니며, 진단 범위를 통제하기 위한 수단이다.

### 2026-10-05 후속 승인 — 명시적 패키지 복사 보조 도구 코드 준비 이력

**범위 및 실제 변경**

- 사용자가 위 Editor 전용 도구 제안에 “진행하자”로 승인했다. 코드 작성만 수행했으며 **C++ 빌드·패키징·에셋 복사·Editor 실행은 이번 작업에서 하지 않았다.**
- 신규 `Plugins/DCRifleMigrationTools/`: `.uplugin`, `DCRifleMigrationTools.Build.cs`, 기본 모듈 구현, `Public/DCRifleMigrationLibrary.h`, `Private/DCRifleMigrationLibrary.cpp`, `Private/DCRifleMigrationValidation.h`, `Private/Tests/DCRifleMigrationValidationTests.cpp` 총 7개 파일.
- `DreamCatcher.uproject`에 플러그인 등록 1건을 추가했다. 모듈 유형 `Editor`, 프로젝트 등록 `TargetAllowList=[Editor]`, 콘텐츠 없음, 자동 실행 없음. DreamCatcher/Lyra 게임 런타임 모듈에 대한 의존성을 넣지 않았다. 기존 `Source`·`Config`·`Scripts/Editor` 파일 327개는 작업 전후 SHA-256 비교에서 변경이 없었다.

**원본 근거·제약·선택한 방식**

- 로컬 UE 5.8 `AssetTools/Private/AssetTools.cpp`의 명시적 `AdvancedCopyPackages(TMap<...>, ...)` 경로를 사용한다. 엔진이 복제·참조 치환·Blueprint Compile·목적지 저장을 수행하며, 자체 바이너리 수정이나 엔진 소스 수정은 없다.
- `BeginAdvancedCopyPackages`의 자동 종속성 확장과 dirty 패키지 저장 경로를 피하고, 지정된 맵만 복사 대상으로 전달한다. 단, **원본 로딩 중 종속 에셋은 여전히 메모리에 로드될 수 있다.** 이 도구만으로 종속성 전체 이전이 완료되는 것은 아니다.
- 같은 엔진 구현에서 `ISourceControlModule::IsEnabled() || bForceAutosave`이면 저장하고 소스 컨트롤 자동 추가도 할 수 있음을 확인했다. 따라서 메모리 진단 보존 및 저장소 무변경을 위해 소스 컨트롤이 활성 상태면 작업을 거부한다. 사용자 설정을 도구가 임의 변경하지 않는다.
- 기존 Python 진입점을 그대로 사용하면 소규모 명시적 집합을 통제할 수 없고, 이미 실패한 헤더 치환 방식이나 의존성 제외는 원본 보존 대안이 아니다. 이번 도구는 원본 게임 동작을 바꾸지 않는 **진단용 연결 코드**다. 메모리 복사와 저장 포함 복사를 별도 새 프로세스/실행명으로 비교한다.

**작성된 안전 제한과 결과의 의미 — 실행 검증 전**

- `ValidateCopyPlan`은 사전 검사만, `CopyPackageSubset`은 검사 후 명시적 맵만 네이티브 API에 전달한다. 입력은 파일/오브젝트 경로가 아닌 long package name이다.
- 출발지는 마운트된 `/Game/` 또는 `/ShooterCore/`의 기존 `.uasset`만, 목적지는 새 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/<RunName>/` 하위만 허용한다. 한 번에 **1~16개**, RunName은 ASCII 영숫자/밑줄 1~48자. 중복·대소문자 목적지 별칭·잘못된 경로·예약 파일 이름·기존 목적지 폴더/메모리 패키지/동반 파일·맵을 거부한다.
- 실행 조건: LyraStarterGame 또는 DreamCatcher의 **게임 스레드·unattended commandlet**, `-DCRifleCopyDiagnostics`, 소스 컨트롤 비활성(`-SCCProvider=None -nop4`), dirty 패키지 없음, `AssetTools.UseHeaderPatchingAdvancedCopy=0`. 활성 에디터나 PIE에서 호출하지 않는다.
- `bSaveCopies=false`는 메모리 진단, `true`는 목적지 저장 포함이다. 명시적 원본 `.uasset/.uexp/.ubulk/.uptnl`의 존재 여부 및 MD5 전후 값을 로그에 남기고 대조한다. **로드된 모든 간접 의존성의 해시 검증은 아니다.** 결과에 네이티브 반환값·원본 유지·목적지 메모리/파일 상태를 분리한다.
- 성공 반환도 별도 프로세스 재로드·Blueprint 오류·참조 완결성·PIE·복제 성공을 뜻하지 않는다. 네이티브 오류 로그를 따로 검사한다. 충돌 시 결과 반환/사후 해시 검사가 불가능할 수 있고 일부 파일이 남을 수 있다. 기존 부분 결과의 자동 삭제·덮어쓰기·복구를 하지 않으며 새 실행명으로만 재시도한다.

**검증 상태와 다음 절차**

- 로컬 엔진 헤더/구현 API 대조, `.uplugin/.uproject` JSON 파싱, Editor 전용 등록, `git diff --check` 정적 확인을 수행했다. **신규 코드의 UHT/C++ 빌드·로드·동작은 미검증**이다.
- `DreamCatcher.MigrationTools.ExplicitCopy.NameGuards` 테스트를 작성했다. 유효한 1/16개 맵, 잘못된 실행명, 경로 탈출/오브젝트·파일 경로, 중복 목적지, 예약 이름, 17개 초과 거부를 검사하며 에셋 생성·저장은 하지 않는다. **아직 실행하지 않았다.** dirty/소스 컨트롤/기존 파일 거부 등 전체 preflight 런타임 검증을 대신하지 않는다.
- 사용자: 에디터 종료 후 `DreamCatcherEditor / Development Editor / Win64` 빌드. 새 플러그인 파일이 IDE에 보이지 않으면 프로젝트 파일을 재생성할 수 있다. 게임 코드 연결이나 Blueprint 수동 노드 작업은 필요 없다.
- Codex 재개: 빌드 성공 확인 → 새 모듈/Reflection 로드 및 NameGuards 실행 → 쓰기 없는 `ValidateCopyPlan` 확인 → 2개 안팎의 명시적 집합으로 메모리/저장/새 프로세스 재로드와 참조 치환 비교. 원본 프로젝트는 `-PLUGIN=<DCRifleMigrationTools.uplugin 절대경로>`와 필요한 플러그인 활성화로 로드 가능한지 먼저 확인한다. 이 외부 로드 방법도 실제 실행 전이며, 원본 `.uproject`나 엔진 플러그인 폴더를 선제 수정하지 않는다.
- 현재는 **R5-3 라이플 에셋 준비의 복사 충돌 진단 / R6 선행 코드만 일부 검증** 상태다. R5-3 완료·활성 무기 교체·기존 구현 제거는 아직 아니다. 10/8 핵심 장비·발사·재장전 체크포인트의 선행 문제이므로, 소규모 검사 결과에 따라 일정 영향을 다시 판단한다.

### 2026-10-05 19:35~20:00 KST — 도구 검증 통과, 상속 컴포넌트 소실 분리 확인

**사용자 빌드 및 도구 검사**

- UBT `C:/Users/Min/AppData/Local/UnrealBuildTool/Log.txt`: **19:35:24 KST** 시작, `DCRifleMigrationLibrary.cpp`, 모듈, UHT 생성 코드, `DCRifleMigrationValidationTests.cpp` 컴파일 및 DLL 링크 후 **Succeeded**, 10.40초. 새 DLL/엔진의 BuildId는 `55116800`으로 일치했다. 이번 턴에서 Codex는 C++ 빌드·패키징을 실행하지 않았다.
- `Saved/Logs/R5ExplicitCopyNameGuards.log`, **19:38:44 KST**: `DreamCatcher.MigrationTools.ExplicitCopy.NameGuards` 성공. `Saved/Automation/R5ExplicitCopyNameGuards/index.json` 성공 1, 실패·경고 0, 프로세스 종료 코드 0. 게임플레이/복제 검사가 아니라 이름·개수 제한 검사다.
- 신규 스크립트 `Scripts/Editor/r5_rifle_explicit_copy.py`: 고정 진단 묶음 `audio` 또는 `weapon_actor`, `preflight/memory/save/verify/inspect` 모드만 제공한다. 복사/저장은 C++ bridge의 Unreal 내부 API만 사용하며 원본 Save, Migrate, C++ 빌드, 패키징, 파일 덮어쓰기/삭제를 호출하지 않는다.
- 원본 `.uproject`를 수정하지 않고 `-PLUGIN=<DreamCatcher/Plugins/DCRifleMigrationTools/DCRifleMigrationTools.uplugin>`와 `-EnablePlugins=DCRifleMigrationTools,PythonScriptPlugin,EditorScriptingUtilities`로 새 모듈을 로드했다.
- 최초 preflight의 C++ 판정은 통과했으나 Python 반환을 tuple로 가정해 스크립트가 실패했다. 실제 reflection 문서의 `str or None` 규약에 맞춰 **스크립트만** 수정했고, `R5ExplicitCopy-preflight2.log` **19:44:36 KST**에서 사전 검사·스크립트 전체 성공(에셋 쓰기 0, 종료 0)을 확인했다. 최초 실패 로그와 구분한다.

**오디오 2개: 소규모 메모리/저장/재로드 성공**

- 원본: `/Game/Audio/Blueprints/B_MusicManagerComponent_Base`, `/Game/Audio/Blueprints/WeaponAudioMacros`.
- 원본 프로젝트 `Saved/Logs/R5ExplicitCopy-memory.log`, **19:46:08 KST**: 명시적 2개 메모리 복사 및 Blueprint Compile 성공, `sources_unchanged=1`, `saved=0`, 종료 0. `AudioMemory_1945` 폴더가 디스크에 생성되지 않은 것도 확인했다.
- `R5ExplicitCopy-save.log`, **19:47:11 KST**: 새 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/AudioSave_1947/`에 2개 저장, `sources_unchanged=1`, `destinations_verified=1`, 종료 0.
- `R5ExplicitCopy-reload.log`, **19:48:27 KST**: 별도 프로세스에서 두 복사본 로드·Compile·종속성 조회 성공, 종료 0. 기존 목적지를 다시 preflight하면 거부하는 것도 확인했다. 두 에셋 사이에는 직접 package 참조가 없어, 이 결과만으로 부모/자식 참조 치환을 검증했다고 볼 수 없다.
- 원본 프로젝트의 기존 `GameplayCue.Character.Death` 중복 등록 경고는 계속 존재한다. NameGuards의 경고 0과 이 commandlet 전체가 무경고라는 주장을 혼동하지 않는다.

**실제 무기 부모/자식: 저장 성공이지만 원본 동등성 실패**

- 원본 `/Game/Weapons/B_Weapon`, `/ShooterCore/Weapons/Rifle/B_Rifle`만 새 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/WeaponActorSave_1951/`에 명시적으로 복사했다. `R5ExplicitCopy-weapon-save.log`, **19:51:38 KST**: 두 파일 저장·Compile 및 원본 해시 유지 성공, `B_Rifle`의 부모도 새 `B_Weapon_C`로 치환, 프로세스 종료 0.
- `R5ExplicitCopy-weapon-reload.log`, **19:55:44 KST**: 새 프로세스 로드·Compile은 성공. 부모 `B_Weapon`의 package 참조 집합은 일치했지만 자식 `B_Rifle`에는 원본의 `/Game/Weapons/Rifle/Mesh/SK_Rifle`, `/Game/Weapons/Rifle/Materials/MI_Weapon_Rifle`, `/Game/Weapons/Rifle/Animations/ABP_Weap_Rifle` **3개가 누락**돼 자동 비교가 의도적으로 실패(종료 1)했다. 정상 이식본이 아니며 활성 적용 금지다.
- `R5ExplicitCopy-weapon-components.log`, **20:00:01 KST**: 원본과 복사본의 로드된 SkeletalMeshComponent 템플릿을 읽기만 했다. 원본 자식 `B_Rifle_C:SkeletalMesh_GEN_VARIABLE`에는 SK_Rifle, ABP_Weap_Rifle_C, MI_Weapon_Rifle가 설정돼 있으나, 복사본 자식의 템플릿 목록은 비어 있었다. 부모 양쪽의 템플릿은 메시/AnimClass 없음·재질 목록 비어 있음으로 동일했다. **자식 상속 오버라이드 소실**을 package 참조 이외에도 확인했다. 에셋 쓰기 0, 종료 0.
- 근거가 확인된 문제는 이 2개 복사 집합에서의 상속 값 소실이다. 이전 971개 대량 저장 접근 위반의 정확한 원인·같은 원인 여부는 아직 미확정이며, 소규모 성공만으로 대량 충돌 해결을 선언하지 않는다.

**당시 변경안 — 후속 승인/코드 준비는 아래 최신 인계 참고**

- 원본 근거: `B_Rifle`의 메시·AnimClass·재질은 원본 자식 override 템플릿에 있다. 엔진 `InheritableComponentHandler.cpp::ValidateTemplates/IsRecordValid`는 override의 OwnerClass·ComponentKey·상속 관계를 검사해 유효하지 않은 레코드/템플릿을 제거한다. 이 특정 내부 함수가 이번 손실의 발생 지점인지까지 스택으로 확인한 것은 아니다.
- 구체적 제약: 현재 `AdvancedCopyPackages` 호출은 부모 참조를 바꾸지만 해당 자식 override 값은 보존되지 않았다. 성공 반환·저장·Compile만으로는 검출되지 않는다. 이 상태에서 라이플을 이전하면 원본 총 메시/재질/애니메이션을 잃을 수 있다.
- 원본 보존 대안: (1) 원본 부모/경로를 그대로 유지하는 복사는 참조 제한 및 기존 경로 충돌을 다시 다뤄야 하므로 이번에 활성화하지 않는다. (2) Unreal의 컴포넌트 override 생성·속성 복사 API로 **새 부모의 동일 SCS 컴포넌트에 원본 자식 override 전체를 보존/복원하고 속성을 비교**하는 Editor 보조 도구 확장을 권장한다. 눈에 보인 세 참조만 임의 하드코딩해 나머지 원본 값을 생략하지 않는다.
- 추가 근거: `SubobjectData.cpp::GetObjectForBlueprint`는 이름과 달리 `CreateOverridenComponentTemplate`를 호출할 수 있다. 읽기 전용 조사에서는 이 API를 사용하지 않았다. 실제 복원 작업에서만 신규 복사본을 대상으로 사용하는 방안을 검토한다.
- 예상 변경 범위: `DCRifleMigrationTools`의 Editor 라이브러리/검사와 진단 스크립트. 새 실행 폴더에서만 시험하고 원본·기존 실패본·게임 C++·Config·활성 무기는 유지한다. 의도적인 게임 동작 차이는 없으며 **경로/부모만 이식본으로 치환하고 컴포넌트 값은 원본과 동일하게 유지**하는 것이 합격 기준이다. 속성 비교·별도 프로세스 재로드·최종 메시 시각 확인 전에는 완료로 취급하지 않는다.
- 이 보강 C++는 사용자 승인 후 작성하고 사용자가 빌드한다. 필요 API 접근과 실제 보존 성공 여부는 아직 미검증이다. 이번 검증 과정에서는 기존 C++를 추가 수정하지 않았다.
- 이번 생성물은 원본 프로젝트의 진단 에셋 **4개**(오디오 2, 무기 Actor 2), 새 Python 스크립트와 검사 로그다. 기존 부분 폴더를 삭제/덮어쓰지 않았고 DreamCatcher `Content/LyraMigration/Rifle`는 아직 없다. 현재는 **R5-3 진행 중**, R6 실사용 발사·재장전·R7 HUD 연결로 넘어가지 않는다.

### 2026-10-05 후속 승인 — SCS override 보존/복원 코드 준비, 빌드 대기

- 사용자가 위 보강안을 승인했다. 이번에는 **코드와 검사 준비만** 수행했으며 C++ 빌드·패키징·Unreal 프로세스·에셋 복사/수정은 실행하지 않았다. 앞선 19:35 빌드 및 NameGuards 성공은 이번 보강 코드의 검증 결과가 아니다.
- 변경: Editor 플러그인의 `DCRifleMigrationLibrary.h/.cpp`, 신규 `Private/DCRifleInheritedOverrides.h/.cpp`, 신규 `Private/Tests/DCRifleInheritedOverrideTests.cpp`, `Scripts/Editor/r5_rifle_explicit_copy.py`. Build.cs·uproject 추가 변경, 게임 C++·Config·바이너리 에셋 변경은 없다.

**구현한 동작 — 실행 전**

1. `CopyPackageSubset`의 `bPreserveInheritedOverrides=true`일 때만 보존기를 켠다. 기본값 false는 이전 문제 재현/대조용 네이티브 경로를 유지한다. Python 진단 스크립트에서는 `-DCRiflePreserveOverrides`를 명시한다.
2. 네이티브 복사 전에 원본 override의 SCS 소유 Blueprint·GUID·변수명, 임시 템플릿 복사, 복제 대상 속성의 불변 비교 데이터를 캡처한다. 원본을 자동 Compile·저장하지 않고, 로딩으로 dirty 패키지가 생겨도 복사 전 재검사에서 중단한다.
3. 보존 모드에서는 native Advanced Copy를 **메모리 모드**로 실행한다. 복사 부모의 동일 GUID/변수명과 부모·자식 관계를 확인한 뒤 `CreateOverridenComponentTemplate` 및 `UEngine::CopyPropertiesForUnrelatedObjects`로 새 자식의 override 값을 복원한다. 부모부터 Compile하고 후속 자식은 현재 클래스/핸들러를 다시 찾는다. 원본 객체 교체 알림·원본 참조 일괄 치환은 하지 않는다.
4. 원본 엔진의 `FComponentComparisonHelper`와 동일한 `ShouldDuplicateValue()` 기준으로 속성을 선택한다. 비일시적 내부 객체의 속성·클래스와 record 수, native 속성 비교 훅도 검사한다. 메시/재질/AnimClass 세 항목만 하드코딩하지 않는다. UObject 외형 경로는 비교용으로 정규화하고 **예상 원본 참조만 이식 경로로 번역**한다. 실제 복사본의 남은 원본 참조까지 정규화해 오류를 숨기지 않는다.
5. Compile 후 속성/record 비교·원본 파일 해시 검사가 모두 성공해야 명시적 신규 목적지 패키지만 저장한다. 저장 직전 기존 파일·동반 파일·소스 컨트롤 활성 상태를 다시 거부하고, 저장 후 원본 해시도 재검사한다. 비교 실패 시 이유·속성명·일부 값이 결과에 남는다. 프로세스 충돌이나 저장 도중 실패의 부분 출력은 자동 삭제하지 않는다.
6. `CompareInheritedOverrides`는 기존 복사본을 수정·저장하지 않고 원본과 비교한다. fresh-process `verify` 모드에서 package 참조 검사와 함께 호출하며, Rifle 그룹은 예상 override 수가 0이면 통과시키지 않는다.

**범위·제약**

- 게임 동작을 바꾸는 보강이 아니라 복사 시 원본 설정 손실을 방지하는 Editor 진단용 처리다. 기존 16개 제한, 새 실행 폴더, commandlet/unattended/소스 컨트롤/dirty 보호는 유지한다. 이미 생성된 `WeaponActorSave_1951`의 손상 복사본을 덮어쓰지 않는다.
- 현재 보존기는 **동일 asset leaf name, GUID/변수명으로 식별 가능한 SCS override, native 컴포넌트 클래스**를 요구한다. UCS 키·템플릿 누락·Blueprint로 정의한 컴포넌트 클래스·이름 변경 등은 지원 여부가 미검증이므로 복사 전에 이유를 남기고 거부한다. 해당 Lyra 기능이 이식 불가능하다는 판정이나 원본 범위 삭제가 아니다. 이런 입력이 발견되면 원본 보존 경로를 추가 조사한다.
- 비교에서 transient/deprecated 등 엔진의 복제 비교 기준 밖 속성은 제외한다. 수치/참조 보존과 모든 시각/런타임 동작의 동등성을 혼동하지 않는다. 임의로 차이를 허용하거나 필요한 참조를 제외해 검사를 통과시키지 않는다.

**검사 준비·재개 순서**

- 새 `DreamCatcher.MigrationTools.ExplicitCopy.InheritedOverrides`: 디스크 저장/실제 에셋 로드 없이 transient 부모·자식 Blueprint를 만들어 원본 override 캡처 → 복사본의 누락 검출 → 복원/Compile 후 비교 → 값 변조 검출 → 원본 비영향을 검사하도록 작성했다. `-unattended -DCRifleCopyDiagnostics`가 필요하다. 실제 Advanced Copy·Rifle 저장/재로드를 대신하는 테스트가 아니며 **아직 실행하지 않았다**.
- 로컬 엔진 API 대조, Python AST 문법 검사, `git diff --check`를 수행했다. 기존 `Source`·`Config` 322개 파일의 작업 전후 SHA-256도 동일했다. **UHT/C++ 컴파일·신규 자동 테스트·실제 라이플 보존 성공은 미검증**이다.
- 사용자: 에디터 종료 후 `DreamCatcherEditor / Development Editor / Win64` 빌드. Codex: 빌드 로그 확인 → 별도 프로세스에서 NameGuards + InheritedOverrides → 기존 손상 진단본을 비교 API가 실패로 판정하는지 확인 → 새 `WeaponActorPreserve_<고유실행명>` 폴더에서 보존 모드 메모리/저장 → 새 프로세스 재로드/부모·package·전체 override 비교. 시각 확인과 최종 이식은 그 다음이다.
- 현재 **R5-3 진행 중 / 보강 코드 빌드 대기**. R6 실사용 발사·재장전, R7 HUD, 기존 구현 제거는 아직 진행하지 않는다.

**후속 사용자 빌드 오류 교정:** `DCRifleInheritedOverrideTests.cpp`의 `PKG_Transient`가 UE 5.8에 정의되지 않아 C2065가 발생했다. 로컬 `ObjectMacros.h`에서 `RF_Transient`가 UObject의 저장 제외 플래그임을 확인하고, 테스트 패키지 생성의 `SetPackageFlags(PKG_Transient)`를 `SetFlags(RF_Transient)`로 교정했다. 게임 코드·에셋 변경은 없으며 **수정 후 사용자 재빌드 및 자동 테스트는 아직 미검증**이다. Codex는 빌드를 실행하지 않았다.

### 2026-10-05 20:31~20:34 KST — 사용자 빌드 성공, 속성 캡처 오류 교정

- UBT `Log.txt`: **20:31:25 KST** 시작, 테스트 컴파일 및 `UnrealEditor-DCRifleMigrationTools.dll` 링크 후 `Result: Succeeded`, 5.09초. DLL 시각 20:31:29 KST. 앞선 PKG_Transient 컴파일 오류는 통과했다.
- `Saved/Logs/R5InheritedOverrideChecks.log`, **20:34:01 KST** 및 `Saved/Automation/R5InheritedOverrideChecks/index.json`: 성공 **1**(NameGuards), 실패 **1**(InheritedOverrides), 경고 **0**. Unreal 프로세스 종료 코드는 0이지만 테스트 보고서에는 실패가 있으므로 전체 검증 성공이 아니다.
- 실패 지점: 원본 transient fixture의 override 캡처, `Cannot export component property: AnimBlueprintGeneratedClass`. 복원·저장 단계에 도달하기 전이며 실제 라이플 에셋 복사를 실행하지 않았다.
- 로컬 엔진 `CoreUObject/Private/UObject/Property.cpp::FProperty::ExportText_Direct`에서 `Data == Delta`이면 전체 출력하고, 기본값과 같아 내보낼 차이가 없을 때도 false를 반환함을 확인했다. 기존 도구는 Delta=nullptr로 전달한 뒤 false를 읽기 오류로 오해했다. 원본 에셋 손상이나 해당 속성 이식 불가가 아니다.
- 기존 승인된 보존 도구의 좁은 결함 교정으로 `DCRifleInheritedOverrides.cpp::ReadProperties`의 `ExportText_InContainer(Index, Value, Object, nullptr, Object, PPF_None)`를 `ExportText_InContainer(Index, Value, Object, Object, Object, PPF_None)`로 변경했다. null/기본값을 포함한 전체 출력이 목적이며 속성 제외·검사 완화로 우회하지 않는다.
- **이 수정 이후 C++ 빌드·테스트는 실행하지 않았다.** 사용자 재빌드 후 NameGuards/InheritedOverrides를 다시 실행하고, 두 검사 통과 후에만 기존 손상본 비교 및 새 라이플 진단 복사/재로드를 진행한다. 현재 R5-3 보존 도구 검증 중이며 R6/R7 활성 연결은 아직 아니다.

### 2026-10-05 20:39~20:45 KST — 재빌드 성공, 스냅샷 생성 충돌 교정

- UBT `Log.txt`: **20:39:12 KST** 시작, `Result: Succeeded`, 5.96초. DLL 시각 20:39:17 KST. ExportText 전체 출력 수정이 포함된 사용자 빌드다.
- `Saved/Logs/R5InheritedOverrideChecks2.log`: **20:42:57~59 KST**, InheritedOverrides 실행이 이전 ReadProperties 실패 지점은 지났으나 `FInheritedOverrideState::Capture`의 `DuplicateObject`(당시 cpp 172행)에서 `EXCEPTION_ACCESS_VIOLATION reading 0x38`로 종료됐다. 완성된 index.json 보고서는 없고 NameGuards도 이 실행에서는 도달하지 못했다. 이전 실행의 NameGuards 성공 이력과 구분한다.
- 원본 프로젝트 `Saved/Logs/R5Override-compare-original.log`, **20:45:31 KST**: 실제 B_Weapon/B_Rifle와 기존 `WeaponActorSave_1951`을 `compare` 모드로 읽기만 했지만 같은 Capture 스냅샷 위치에서 충돌했다. 임시 테스트 fixture만의 문제라고 볼 수 없다. **실제 에셋 복사·복원·저장 호출에는 도달하지 않았다.**
- 원본 2개 `.uasset`의 MD5는 이전 `R5ExplicitCopy-weapon-save.log`의 복사 전 해시와 일치한다. 새 진단 에셋/실행 폴더는 없고 DreamCatcher `Content/LyraMigration/Rifle`도 없다. 실패본과 원본은 보존했다.
- 원인/수정 근거: `FObjectDuplicationParameters`의 기본 FlagMask는 `RF_InheritableComponentTemplate`를 포함한다. 도구는 원본의 상속 컴포넌트를 UClass가 아닌 `GetTransientPackage()` 아래로 이 플래그를 유지해 복제하고 있었다. 로컬 `BlueprintSupport.cpp::FDeferredObjInitializationHelper::DeferObjectInitializerIfNeeded`는 이 플래그가 있으면 Outer를 UClass로 캐스팅한 뒤 SuperClass에 접근한다. 이 소유 객체 계약 위반을 확인했다. 심볼이 없는 엔진 내부 모든 프레임이나 과거 대량 저장 충돌까지 같은 원인으로 확정한 것은 아니다.
- 승인된 보존기 안의 좁은 교정: `DCRifleInheritedOverrides.cpp::Capture`의 스냅샷 생성을 `FObjectDuplicationParameters` + `StaticDuplicateObjectEx`로 변경했다. 로컬 `ActorConstruction.cpp::AActor::CreateComponentFromTemplate`처럼 **생성 전** Archetype/InheritedTemplate/Transactional/WasLoaded/Public 플래그를 제외하고, 메모리 스냅샷에 불필요한 Standalone도 제외한다. `PPF_DuplicateVerbatim`으로 텍스트 ID를 보존하고 생성 후 루트 스냅샷에만 RF_Transient를 설정한다. 중첩 객체에 RF_Transient를 일괄 강제하지 않는다.
- 이는 임시 데이터 사본의 객체 식별 플래그만 교정한 것이다. 원본과 실제 목적지의 상속 템플릿 플래그·메시/재질/AnimClass 등 값은 변경하지 않는다. 전체 속성 비교·원본 해시·저장 제한을 그대로 유지한다. 검사를 건너뛰거나 값을 제외하지 않는다.
- **이번 교정 이후 빌드·테스트는 실행하지 않았다.** 사용자 재빌드 → 동일 자동 테스트 2건 → 기존 손상본 비교가 충돌 없이 불일치로 보고되는지 확인 → 새 보존 모드 복사/저장/재로드 순서로 다시 검증한다. 여전히 **R5-3 도구 검증 중**이며 실제 라이플 이식 완료가 아니다.

### 2026-10-05 20:56~21:30 KST — 보존 도구 검사 통과, 로드 유발 dirty 보호 정책 검토

**통과한 범위**

- UBT `Log.txt`: **20:56:21 KST** 시작, `Result: Succeeded`, 5.85초. DLL 시각 20:56:26 KST. 스냅샷 FlagMask 교정 후 사용자 빌드다.
- `Saved/Logs/R5InheritedOverrideChecks3.log`, **21:07:32 KST**, `Saved/Automation/R5InheritedOverrideChecks3/index.json`: InheritedOverrides + NameGuards **성공 2, 실패·경고 0**, 프로세스 종료 0. 임시 Blueprint에서 누락 검출·복원/Compile 후 속성 일치·값 변조 검출·원본 비영향을 확인했다. 실제 Rifle 복제/저장 성공을 뜻하지 않는다.
- 원본 프로젝트 `Saved/Logs/R5Override-compare-fixed.log`, **21:24:21 KST**: 기존 `WeaponActorSave_1951` 비교가 충돌 없이 완료됐다. 예상 override 1, 비교된 override 0, mismatch로 올바르게 보고했다. 비교 결과 false는 이 손상본에 대한 **기대한 음성 판정**이며, 도구 프로세스는 정상 종료 0. 원본/진단본 수정·저장 없음.

**실제 Rifle 시도의 중단 지점**

- `R5Override-preserve-memory.log`, **21:27:09 KST**: 초기 preflight 통과와 원본 override 1개 캡처 후, 재검사에서 `/Game/Effects/Particles/Weapons/NS_WeaponFire`가 dirty라서 거부했다. `copy_invoked=false`, `saved_to_disk=false`. 결과의 다른 검증 bool=false는 해당 단계 미실행의 기본값이며 원본 디스크 변경 증거가 아니다.
- 세 단계 실행은 메모리 단계의 종료 1에서 중단했다. 계획했던 `WeaponActorPreserveMemory_2125`, `WeaponActorPreserveSave_2125` 폴더는 없으며 save/reload 로그도 없다. **실제 메모리 복사·보존 복원·저장·재로드는 아직 실행/검증되지 않았다.**
- C++는 이번 확인에서 변경하지 않았다. `r5_rifle_explicit_copy.py`에 읽기 전용 `dirty_audit` 모드만 추가했다. 패키지 저장, dirty 플래그 해제, 보호 검사 우회는 하지 않는다.

**읽기 전용 재현 근거**

- `R5Override-dirty-audit.log`, **21:30:27~30 KST**, 종료 0: 시작 시 dirty 목록 `[]`; B_Weapon을 단순 로드한 후 2개, B_Rifle을 단순 로드한 후 다음 4개가 dirty가 됐다. 두 명시적 원본 Blueprint 자체는 dirty가 아니었다. Snapshot/Capture나 복사를 호출하지 않아도 발생했다.
  - `/Game/Effects/Particles/Weapons/NS_WeaponFire`
  - `/Game/Effects/Particles/Weapons/NS_WeaponFire_MuzzleFlash_Rifle`
  - `/Game/Effects/Particles/Weapons/Emitters/NE_MuzzleFlashCards`
  - `/Game/Effects/Particles/Weapons/Emitters/NE_MuzzleFlashStarBurst`
- 따라서 이 실행에서의 dirty는 사용자가 편집 중인 에디터에서 넘겨받은 변경이 아니라 새 commandlet의 로딩 중 생성된 메모리 상태다. 어떤 Niagara 내부 갱신 함수가 표시했는지까지는 미검증이며 원본 기능 오류나 이식 불가로 단정하지 않는다. 원본 B_Weapon/B_Rifle 디스크 MD5는 이전 진단과 동일했다.
- 원본 프로젝트에 기존 Death Cue 중복 경고는 남아 있다. 도구 자동 테스트 2건의 경고 0과 원본 commandlet 전체의 경고를 구분한다.

**당시 변경안 — 후속 승인/코드 준비는 아래 최신 인계 참고**

- 현재 전체 dirty 거부 재검사는 로딩 중 바뀐 종속 패키지도 사용자 미저장 변경과 동일하게 거부하므로 실제 Rifle 보존 진단을 시작할 수 없다. 보호 검사를 없애거나 dirty를 강제로 지우는 방식은 선택하지 않는다.
- 대안 A: 원본 Niagara 4개를 의도적으로 재저장한 뒤 다시 검사한다. 원본 파일 자체가 바뀌므로 별도 원본 변경 승인·백업/검증이 필요하며 이번에는 수행하지 않는다.
- 권장 대안 B: **처음부터 있던 dirty, 복사 대상 Blueprint의 dirty, 명시적 목록 밖 dirty는 계속 거부**한다. 별도 opt-in 진단 모드에서 위 4개만 로드 전에 기록·해시하고, Capture 이후 새로 dirty가 된 패키지가 그 목록 안이며 디스크 파일/동반 파일 해시가 불변인 경우에만 진행하도록 보강한다. 원본 dirty 상태는 해제하지 않고, 저장 대상은 검증된 신규 목적지 패키지로만 제한하며 작업 후에도 원본 해시를 재확인한다. 이번 2개 Rifle Actor 진단 범위로 한정한다.
- 변경 범위는 Editor 도구의 preflight/후속 검사, 해당 정책 테스트 및 진단 스크립트다. 게임 C++·Config·원본 Niagara 내용·활성 무기 설정은 바꾸지 않는다. 의도적인 게임 동작 차이는 없지만, **안전 검사에서 로딩 유발 미저장 종속성 4개를 구분해 취급하는 차이**가 있으므로 사전 승인이 필요하다.
- 승인 후 코드 작업 → 사용자 빌드 → 정책 테스트와 기존 2건 회귀 → 메모리/저장/재로드 순으로 재개한다. 현재는 **R5-3 진행 중**이며 이 한정 정책의 구현·동작 및 실제 Rifle 보존 결과는 미검증이다.

### 2026-10-05 후속 승인 — 고정 load-dirty 정책 코드 준비, 사용자 빌드 대기

- 사용자가 위 안전 정책 보강안을 승인했다. **코드·검사 준비만** 수행했으며 C++ 빌드·패키징·Unreal 프로세스·에셋 복사/저장은 이번 작업에서 실행하지 않았다. 이전 21:07 테스트 성공을 새 코드 검증으로 취급하지 않는다.
- 변경 파일: Editor 플러그인의 `DCRifleMigrationLibrary.h/.cpp`, 신규 `Private/DCRifleLoadDirtyPolicy.h/.cpp`, 신규 `Private/Tests/DCRifleLoadDirtyPolicyTests.cpp`, `Scripts/Editor/r5_rifle_explicit_copy.py`. 기존 상속 복원 구현과 테스트, Build.cs·uproject, 게임 C++·Config·에셋은 변경하지 않았다.

**구현한 제한과 단계**

- 기본 동작은 기존 strict 검사다. `CopyPackageSubset`의 추가 bool `bAllowKnownLoadDirtyDependencies=true`와 `-DCRifleAllowKnownLoadDirty`가 모두 필요하며, **LyraStarterGame + 상속 보존 모드 + 원본 B_Weapon/B_Rifle 정확히 2개**로 제한한다. 임의 허용 패키지 목록을 Python에서 전달할 수 없다.
- 첫 preflight는 예외 없이 dirty 전체를 거부한다. 정책 시작 시에도 dirty가 없어야 한다. 초기 dirty 기록이 있으면 나중에 표시가 사라졌더라도 허용하지 않는다.
- 이전 읽기 전용 재현에서 확인한 Niagara 4개를 고정 목록으로 사용한다. 각 `.uasset`와 선택적 `.uexp/.ubulk/.uptnl`의 존재 여부·MD5를 **Capture/원본 로드 전에** 기록한다. 없는 필수 파일, 읽을 수 없는 파일은 즉시 거부한다.
- Capture 후에는 고정 목록 안에서 새로 dirty가 된 종속 패키지만 허용하고, 명시적 원본 Blueprint 또는 목록 밖 패키지가 dirty이면 거부한다. 고정 목록 전체의 파일과 동반 파일을 재검사하므로, 변경·생성·삭제·읽기 실패도 거부한다. 원본 메모리의 dirty 플래그는 해제하지 않는다.
- 복사/복원 후와 저장 직전·저장 시도 후에도 같은 검사를 수행한다. 복사 후에는 호출의 정확한 목적지 패키지 2개만 정상적인 새 dirty 출력으로 인정한다. 같은 진단 폴더의 다른 패키지는 허용하지 않는다. 저장 대상 배열은 이 신규 목적지들뿐이며 원본이나 Niagara 4개를 추가하지 않는다. 원본 Blueprint 파일 해시 검사도 기존대로 유지한다.
- 결과에는 `KnownLoadDirtyPolicyRequested`, `KnownLoadDirtyPolicyVerified`, 관측된 `ObservedLoadDirtyDependencies`를 추가했다. `LogDCRifleLoadDirty`에 단계별 파일 해시/부재와 미저장 패키지를 기록한다. 새 결과가 성공이어도 fresh-process 재로드·참조/속성 동등성·시각·실제 발사 검증은 별도다.
- 이 변경은 **진단 안전 정책의 한정된 예외**다. Niagara 데이터 수정, 원본 재저장, 상태 표시 강제 해제, 원본 기능 삭제, 활성 무기 전환을 구현한 것이 아니다. 새 dirty 패키지가 발견되면 자동으로 목록을 넓히지 않는다.

**검사 준비 및 재개**

- 새 `DreamCatcher.MigrationTools.ExplicitCopy.LoadDirtyPolicy` 테스트는 순수 값/해시 상태 판정만 수행한다. 올바른 프로젝트/2개 소스/보존 모드, 고정 목록 4개, 초기 dirty 거부, 목록 밖·원본 dirty 거부, 복사 전 목적지 dirty 거부, 복사 후 정확한 목적지만 허용, 파일 변경/생성/삭제/읽기 실패 거부를 검사한다. 패키지·에셋·월드 생성이나 실제 파일 쓰기는 없으며 **아직 실행하지 않았다**. 실제 commandlet의 파일 I/O 및 dirty 관측 통합까지 검증한 테스트는 아니다.
- 로컬 엔진 API 대조, Python AST 문법 검사, `git diff --check`를 수행했다. `Source`·`Config` 322개 파일은 작업 전후 SHA-256이 동일하다. UHT/C++ 컴파일·새 테스트·실제 정책 실행은 미검증이다.
- Python 스크립트는 복사 전에 새 API 인자가 반영됐는지 확인해 구버전 DLL 사용을 거부한다. 허용 옵션은 `weapon_actor` 그룹과 `-DCRiflePreserveOverrides` 조합에서만 사용한다.
- 사용자: 에디터 종료 후 `DreamCatcherEditor / Development Editor / Win64` 빌드. Codex: 빌드 성공 확인 → `-unattended -DCRifleCopyDiagnostics`로 MigrationTools 테스트 **3건**(기존 2 + LoadDirtyPolicy) → 새 고유 실행명의 weapon_actor 메모리 단계부터 `-DCRifleAllowKnownLoadDirty`와 `-DCRiflePreserveOverrides`를 함께 지정 → 성공한 경우에만 새 저장 폴더/별도 프로세스 재로드. 기존 `WeaponActorSave_1951`은 덮어쓰지 않는다.
- 현재 **R5-3 정책 보강 코드 준비 / 사용자 빌드 대기**다. 실제 라이플 보존 복사·이식·발사·HUD·복제 완료는 아니다.

### 2026-10-05 21:53~22:17 KST — 실제 Rifle Actor 보존 성공, 다음 핵심 묶음 조사

**빌드・자동 검사**

- UBT `Log.txt`: **21:53:11 KST** 시작, 새 정책/라이브러리/UHT/테스트를 포함해 `Result: Succeeded`, 10.47초. DLL 시각 21:53:20 KST. 사용자 빌드이며 Codex는 빌드・패키징을 실행하지 않았다.
- `Saved/Logs/R5LoadDirtyPolicyChecks.log`, **21:54:55 KST**, `Saved/Automation/R5LoadDirtyPolicyChecks/index.json`: InheritedOverrides, LoadDirtyPolicy, NameGuards **3건 성공, 실패・경고 0**, 종료 0.

**실제 원본 무기 Actor 2개 보존 검사**

- 원본 프로젝트 `Saved/Logs/R5RiflePolicy-memory.log`, **21:56:38 KST**: `B_Weapon`/`B_Rifle` 메모리 복사, 상속 설정 복원・Compile・전체 비교 성공. dirty 정책도 통과했다. `WeaponActorPreserveMemory_2155` 폴더는 생성되지 않았다.
- `R5RiflePolicy-save.log`, **21:56:54 KST**: 새 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/WeaponActorPreserveSave_2155/`에 **B_Weapon.uasset(445,198바이트), B_Rifle.uasset(17,199바이트)** 저장. 기존 경로는 덮어쓰지 않았다. `succeeded=true`, `source_files_unchanged=true`, `inherited_overrides_verified=true`, `known_load_dirty_policy_verified=true`.
- `R5RiflePolicy-reload.log`, **21:57:09 KST**: 별도 프로세스에서 재로드/Compile 및 원본 대비 package 참조 비교 성공(`missing=[]`, `unexpected=[]`). B_Rifle의 부모는 새 B_Weapon_C이며 SK_Rifle・MI_Weapon_Rifle・ABP_Weap_Rifle 참조가 모두 유지됐다. override는 **expected=1, compared=1, match=1**. 메모리→저장→재로드 순차 실행 전체 종료 0.
- 기존 목적지를 다시 preflight하면 거부하는 검사도 유지됐다. 그 `Preflight rejected` 로그는 이번에 의도한 덮어쓰기 방지 확인이지 재로드 실패가 아니다.
- 작업 후 디스크에서 원본 Blueprint 2개와 허용 Niagara 4개를 재확인했다. `.uasset/.uexp/.ubulk/.uptnl` **24개 존재/해시 상태 중 차이 0**, 실제 존재 파일 6개. 원본 저장・dirty 해제 없음. 원본 프로젝트의 기존 Death Cue 중복 경고는 별도로 남아 있다.
- **완료 범위는 원본 Lyra 안의 Actor 부모/자식 2개 보존 검사**다. 실제 시각 출력・게임플레이・발사・재장전・멀티플레이 및 DreamCatcher 클래스 연결은 미검증이며 전체 R5-3 완료가 아니다. 과거 971개 대량 저장 충돌의 정확한 원인을 해결했다고 선언하지 않는다.

**다음 묶음의 읽기 전용 조사**

- 스크립트에 `rifle_core_audit` 고정 목록을 추가했다. 이 그룹은 **dirty_audit 모드만 허용**하며 복사/저장 호출에는 사용할 수 없다. C++・Config는 이번 확인에서 변경하지 않았다.
- 원본 `Saved/Logs/R5RifleCore-dirty-audit.log`, **22:17:41 KST**, 종료 0: 다음 원본 핵심 **16개** 모두 정상 로드. 명시적 원본의 dirty는 0, 로드 후 dirty 종속성은 기존 Niagara 4개 그대로다.
  - WeaponInstance: `/ShooterCore/Weapons/B_WeaponInstance_Base`, `/ShooterCore/Weapons/Rifle/B_WeaponInstance_Rifle`
  - Actor: `/Game/Weapons/B_Weapon`, `/ShooterCore/Weapons/Rifle/B_Rifle`
  - 발사: `/Game/Weapons/GA_Weapon_Fire`, `/ShooterCore/Weapons/Rifle/GA_Weapon_Fire_Rifle_Auto`
  - 재장전: `/Game/Weapons/GA_Weapon_ReloadMagazine`, `/ShooterCore/Weapons/Rifle/GA_Weapon_Reload_Rifle`, `/Game/Weapons/GA_Weapon_AutoReload`
  - UI/데이터: `/ShooterCore/Weapons/Rifle/`의 `W_Reticle_Rifle`, `W_AmmoCounter_Rifle`, `AbilitySet_ShooterRifle`, `WID_Rifle`, `ID_Rifle`
  - Cue: `/ShooterCore/Weapons/Rifle/GCN_Weapon_Rifle_Fire`, `/Game/GameplayCueNotifies/GCN_Weapon_Impact`
- 원본 루트 package 참조 집합: **1,483개** = Game 1,189 + ShooterCore 29 + Niagara 207 + ControlRig 56 + AudioModulation 1 + AnimationLocomotionLibrary 1. Engine/Script 외부 참조 76, registry 조회 불가 0. **원본 Hero가 여전히 포함**돼 있다. 이전에 최소 수정했던 Seed가 아닌 원본 루트 조사이므로, 이 숫자를 최종 Migrate/파일 복사 개수로 사용하지 않는다. 동적 GameplayCue 태그 등 모든 런타임 참조를 완전하게 증명하는 목록도 아니다.

**당시 제안 — 고정 핵심 16개 준비 (후속 승인・작성 상태는 아래 참고)**

- 현재 정책은 정확히 B_Weapon/B_Rifle 2개만 허용하므로 16개 묶음을 복사하려면 코드 범위 변경 승인이 필요하다. 임의 16개나 더 큰 묶음을 허용하지 않고, 위 정확한 목록만 추가하는 변경안을 제안한다.
- 유지할 보호: 신규 실행 폴더/기존 목적지 거부, 최대 16개, 초기 dirty・명시적 원본 dirty・목록 밖 dirty 거부, Niagara 고정 4개 및 전체 명시적 원본 해시, 목적지 전용 저장, 상속 설정 비교. 이번 조사에서 새 dirty가 없었다는 이유로 다른 미검증 패키지를 허용하지 않는다.
- 승인할 작업 묶음: 제한된 Editor 정책/테스트와 핵심 준비 자동화 → 사용자 빌드 → 새 고유 폴더에 핵심 복사/내부 부모・아이템/장비/AbilitySet 참조 검증 → 이미 승인된 B_WeaponInstance_Base의 Hero 타입 핀 최소 연결을 새 복사본에 재적용하고 Compile/재로드 비교 → 그 복사본에서 종속성/대상 충돌 목록 재산출. 원본・기존 실패/성공 진단본은 수정하지 않는다.
- 원본 근거/동작 차이: Actor 상속 설정은 이번에 원본과 일치함을 검증했다. 기존 17:44 Seed의 Hero 핀 변경은 원본 Hero Blueprint의 구체 타입 대신 해당 경로가 사용하는 native LyraCharacter 기반으로 연결하는 최소 변경이었다. 새 핵심 묶음도 이 한정 차이를 기록하고 나머지 원본 데이터/그래프를 유지해야 한다. 불명확한 추가 연결은 미검증으로 남기고 먼저 설명한다.
- 아직 대량 종속성 복사, DreamCatcher Migrate/Config 적용, 활성 PawnData・QuickBar・발사/HUD 전환, 기존 구현 삭제는 수행하지 않는다. 원본 경로 그대로 복사하면 대상 충돌/원본 프로젝트 결합 문제가 남으므로 이번 새 준비 묶음에서 참조를 확인하는 것이 권장 대안이다.
- 현재 **R5-3 진행 중: 무기 Actor 상속 보존 하위 검증 완료 → 핵심 16개 준비 묶음 승인 대기**. 이 다음 범위의 C++는 아직 변경하지 않았다.

### 2026-10-05 후속 승인 — 핵심 16개 정책・준비 자동화 코드 작성

- 사용자가 위 코드/자동화 묶음을 승인했다. 이번에는 **코드와 검사 준비만** 수행했으며 C++ 빌드・패키징・Unreal 프로세스・에셋 복사/변경은 실행하지 않았다.
- 변경: Editor 정책/라이브러리 `.h/.cpp` 및 기존 LoadDirtyPolicy 테스트, 신규 `Scripts/Editor/r5_rifle_core_stage.py`, `r5_rifle_core_contract.py`, `tests/test_r5_rifle_core_contract.py`. 기존 상속 보존/복원 구현, 이전 Actor 진단 스크립트, 게임 C++・Config・Build.cs・uproject・에셋은 그대로다.

**C++ 범위**

- `ApprovedRifleCorePackages()`에 앞서 조사한 정확한 16개를 고정했다. 정책은 기존 Actor 2개 또는 이 16개 전체 집합만 허용한다. 15개 부분 집합, 같은 개수의 다른 목록, 17개 확장, 다른 프로젝트/보존 비활성은 거부한다. 기존 목적지・이름・dirty・파일 해시 보호는 유지한다.
- Python은 읽기 전용 getter `GetApprovedRifleCorePackages()`와 `GetKnownRifleLoadDirtyPackages()`로 현재 빌드의 목록을 받는다. Niagara 허용 목록은 기존 4개 그대로이며 스크립트에서 추가할 수 없다. 공개 getter가 없는 구버전 DLL은 새 준비 스크립트가 거부한다.
- 기존 `LoadDirtyPolicy` 테스트에 정확한 16개 허용/부분・대체・추가 목록 거부/프로젝트・보존 모드 제한을 추가했다. MigrationTools 테스트 종류는 기존과 같은 **3건**이며 새 코드로 아직 실행하지 않았다.

**신규 준비 스크립트의 계획된 실행**

- 별도 원본 Lyra Python commandlet 전용. `-DCRifleCoreRun=RifleCore_<고유이름>`을 명시한다. 목적지는 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_<이름>/`이다. 기본 `preflight`는 저장하지 않는다.
- `memory`: C++ 보호 검사로 정확히 16개를 메모리에만 복사 → 원본 대비 부모・주요 데이터/Fragment/ItemTagStack 비용・그래프 노드/핀 기본값/연결 비교 → 새 B_WeaponInstance_Base의 `GetTypedPawn.PawnType` 입력 1개를 기존 승인대로 원본 Hero Blueprint 타입에서 `/Script/LyraGame.LyraCharacter`로 변경 → Compile/같은 비교 및 SCS override 비교. 저장은 하지 않는다.
- `prepare`: 위 메모리 절차가 통과한 경우에만 **같은 호출에서 새로 만든 16개**를 Unreal `SaveLoadedAssets`로 저장한다. 소스 컨트롤 비활성, 초기 dirty 없음, 원본 dirty 없음, 신규 목적지 외 dirty는 고정 Niagara 4개만, 기존 목적지/동반 파일 없음, 원본 핵심 16개+Niagara 4개 SHA-256/존재 상태 불변을 저장 직전과 작업 후 확인한다. dirty 플래그는 해제하지 않는다. 부분 저장 실패는 보존하며 재개/덮어쓰기 모드는 없다.
- `verify`/`plan`: 별도 프로세스에서 저장된 복사본 16개 로드・Compile・부모/선택 데이터/그래프 구조/직접 package 참조/SCS override 비교 → 종속 package 집합과 DreamCatcher 기존 Game 경로 충돌 목록을 읽기 전용으로 산출한다. 원본 및 진단본 디스크 해시도 검사하며 저장은 하지 않는다.
- 복사 실행에는 `-DCRifleCopyDiagnostics -DCRiflePreserveOverrides -DCRifleAllowKnownLoadDirty -unattended -SCCProvider=None -nop4`를 사용한다. SourceControl/EditorAssetSubsystem/Python 플러그인은 기존 명령줄 활성화/외부 `-PLUGIN=` 방식으로 사용하며 원본 `.uproject`를 바꾸지 않는다.

**검증 범위/차이**

- 의도한 차이는 이미 승인된 Hero 타입 입력 핀 한 개와 묶음 내부 참조의 새 경로 치환이다. 그래프 비교는 노드 클래스/제목, 핀 이름/방향/기본값/연결을 검사한다. 모든 핀 타입 메타데이터・Widget Designer 시각 계층・모든 CDO 속성의 전수 비교는 아니며, 선택 데이터와 SCS override 비교 및 실제 시각/게임플레이 검증을 구분한다.
- 선택 데이터는 ID의 Fragment, WID의 InstanceType/AbilitySets/ActorsToSpawn, AbilitySet의 부여 목록, 라이플 퍼짐/애니메이션 데이터, GA의 정책/비용/태그 등이다. 알 수 없는 Fragment 종류나 조회 실패/차이는 저장 전 실패로 남긴다. 값을 임의로 변경하거나 검사를 자동으로 생략하지 않는다.
- 메모리 단계의 AssetRegistry 새 package 종속성 조회는 지연될 수 있어, 정확한 package 참조 판정은 fresh-process verify에서 한다. 직접 Hero package 예외는 수정한 Base 에셋에만 적용한다. 그래프·참조가 추가로 달라지면 자동 허용하지 않고 원인을 확인한다.
- 종속성 계획은 원본 핵심 package로 다시 연결되는 외부 참조, 원본 Hero 도달 여부, 조회 불가, 대상 기존 경로를 보고한다. 16개 준비본이 외부 종속성까지 독립 이식된 완성본이라는 뜻이 아니다. 외부 Macro/Widget/시각 에셋, 동적 Cue 연결 등은 후속 검증 대상이며 대량 복사・Config・활성 경로 교체는 이번 범위가 아니다.

**수행한 검사와 다음 절차**

- Python 순수 테스트 **5건 통과**: 새 실행명/16개 경계, package 참조 번역 경계, Hero 종속성 예외 범위, Hero 핀 한 개 변경/원본 불변, dirty 보호. Unreal을 import하거나 에셋/파일을 생성하지 않는 테스트다. 준비 runner의 Python AST 문법 검사, `git diff --check`, C++ 고정 16개와 이전 읽기 전용 조사 목록 일치도 확인했다.
- `Source`・`Config` 322개 파일의 SHA-256은 작업 전후 동일하다. **신규 UHT/C++ 컴파일・확장된 C++ 테스트・Unreal 16개 복사/핀 변경/저장/재로드는 아직 미검증**이다.
- 재개: 사용자 `DreamCatcherEditor / Development Editor / Win64` 빌드 → Codex가 MigrationTools 3건 회귀 → core 스크립트 preflight → 새 `RifleCore_Memory_*` 메모리 단계 → 통과하면 다른 새 `RifleCore_Prepared_*` 이름으로 prepare → 새 프로세스 verify/plan. 실패하면 다음 쓰기 단계는 중단한다. 기존 Actor 성공본/실패본・Seed・Prepared 등은 덮어쓰지 않는다.
- 현재 **R5-3 핵심 16개 준비 코드 완료 / 사용자 빌드 대기**. R5-3 전체, R6 실제 발사, R7 HUD, PIE/복제 검증 완료는 아니다.

### 2026-10-06 인계 — 10월 5일 23:36~23:59 검증, core16 저장 전 중단

**완료 범위와 실패 지점**

- 사용자 UBT 로그 `C:/Users/Min/AppData/Local/UnrealBuildTool/Log.txt`: **23:36:45 KST 시작, 13.57초, Result: Succeeded**. 보조 도구 DLL의 갱신 시각은 23:36:58이다. Codex가 빌드・패키징한 것이 아니다.
- `Saved/Logs/R5Core16PolicyChecks.log`와 `Saved/Automation/R5Core16PolicyChecks/index.json`: **23:38:34 KST**, `InheritedOverrides`, `LoadDirtyPolicy`, `NameGuards` **3건 성공, 실패・경고 0**, 프로세스 0. 독립 도구 테스트이며 실제 core 묶음 복사 성공은 아니다.
- 원본 Lyra `Saved/Logs/R5Core16-preflight.log`: **23:40:25 KST 사전 검사 통과**, 에셋 쓰기 0.
- 원본 Lyra `Saved/Logs/R5Core16-memory.log`: **23:40:58 KST**, 실행명 `RifleCore_Memory_2340`. 엔진 Advanced Copy 반환값은 true였지만 보조 도구의 Compile/보호 검사 결과는 **success=false**다. 반환값만으로 복사 성공을 판정하지 않는다.
- 오류 1: 새 `GCN_Weapon_Rifle_Fire`의 `Lyra Get Weapon` 매크로 확장 출력 `As B Weapon`과 `Target`에서 **B Weapon Object Reference is not compatible with B Weapon Object Reference**. 원본/복사본 클래스 참조가 일치하지 않는 경로다.
- 오류 2: 허용 목록 밖 원본 `/ShooterCore/Weapons/Pistol/B_Pistol` dirty. 같은 실행에서 원본 Pistol의 Actor・WeaponInstance・Fire・Reload BP가 자동 재컴파일된 로그도 확인했다. Pistol을 Niagara load-dirty 예외에 추가하지 않는다.
- **저장, Hero 핀 변경, 후속 prepare, fresh-process verify/plan은 실행하지 않았다.** 실패한 메모리 프로세스는 종료했으며 `RifleCore_Memory_2340`/`RifleCore_Prepared_2340` 에셋 폴더와 prepare/reload 로그는 생성되지 않았다. DreamCatcher의 `Content/LyraMigration/Rifle`도 아직 없다.
- 로그의 복사 전 원본 fingerprint와 디스크를 대조한 결과 core16+Niagara4의 `.uasset/.uexp/.ubulk/.uptnl` **80개 존재/해시 상태 차이 0**이다. Pistol은 이 사전 해시 집합에 없으므로 이 수치로 Pistol 전수 불변을 주장하지 않는다. 원본 에셋 저장은 호출하지 않았다.

**원본 조사와 엔진 근거**

- 신규 `Scripts/Editor/r5_rifle_reference_audit.py`는 원본 Lyra Python commandlet에 한정된 읽기 전용 조사다. 그래프/노드/핀, AssetRegistry 직접 종속성, 원본 Pistol 부모를 조회하며 Compile・copy・save는 호출하지 않는다. 최종 로그는 원본 `Saved/Logs/R5Core16-reference-audit2.log`, **23:58:54~23:59:01 KST**, 종료 0, `asset_writes=0`, `compile=false`, `copy=false`다.
- `GCN_Weapon_Rifle_Fire.OnBurst`에서 `Lyra Get Weapon` 매크로 사용을 확인했다. `/Game/Audio/Blueprints/WeaponAudioMacros`의 `LyraGetWeapon` 그래프에는 `Cast To B_Weapon`이 있고 `/Game/Weapons/B_Weapon`을 참조한다. `/ShooterCore/System/Audio/WeaponAudioFunctions.SetWeaponSoundParams`는 같은 라이브러리의 `Lyra Get Weapon Ammo`를 사용한다. 두 라이브러리는 core16 밖이므로 원본과 복사된 무기 타입의 의존 경계를 함께 다뤄야 한다.
- Python은 `MacroGraphReference`와 핀의 `PinSubCategoryObject` 보호 필드 읽기를 거부했다. 제한을 우회하지 않았다. 노드 제목/출력 표시 타입/등록된 package 의존성은 확인했지만 **실제 매크로 그래프 포인터와 핀 subtype 객체 경로의 정확한 대응은 공개 C++ API로 추가 확인해야 한다**.
- 엔진 `Engine/Source/Developer/AssetTools/Private/AssetTools.cpp`의 full `AdvancedCopyPackages`는 명시적 원본만 제외 집합에 넣고 `ObjectTools::ConsolidateObjects`를 호출한다. `Engine/Source/Editor/UnrealEd/Private/ObjectTools.cpp`의 BP 자식 처리에는 `GetDerivedClasses`로 찾은 원본 자식 중 제외 집합 밖인 항목에 `Modify`, `ParentClass` 교체, Compile을 실행하는 경로가 있다. 그 분기에는 새 목적지 포함 집합에 한정하는 검사가 없다. 삭제 비활성 인자로 호출해도 이 경로는 별개다.
- 위 소스 경로는 실패 로그의 원본 Pistol 자동 Compile/dirty와 부합한다. 실패 프로세스에서 Pistol의 변경된 ParentClass 값을 직접 캡처한 것은 아니므로 모든 메모리 변경을 실측한 것으로 보고하지 않는다. 별도 읽기 전용 재로드에서는 원본 Pistol 4개 부모가 각각 원본 B_Weapon/WeaponInstance_Base/GA_Weapon_Fire/GA_Weapon_ReloadMagazine을 가리킴을 확인했다.

**당시 변경안 — 후속 승인/작성 상태는 아래 인계 참고**

1. 기존 core16에 위 **원본 WeaponAudioMacros・WeaponAudioFunctions 2개를 포함하는 후보 18개**의 그래프/타입 의존 경계를 공개 C++ getter로 확인한다. 추가 의존성이 발견되면 임의로 대량 복사하지 않고 설명한다. 매크로를 펼쳐 재제작하거나 원본 기능을 삭제하지 않는다.
2. Editor 보조 도구를 엔진의 하위 복사・참조 갱신 API를 사용하는 **새 복사본 한정 처리**로 보강하는 방안을 검증한다. 명시적 원본뿐 아니라 로드된 미선택 원본 자식도 재부모화/Compile 대상에서 보호한다. 엔진 소스 자체는 수정하지 않는다. 구체 API 조합은 원본 자식 보호 테스트로 확정하며, 아직 해결됐다고 단정하지 않는다.
3. 테스트에 미선택 형제 BP의 부모/dirty/파일 불변과 매크로・함수의 복사본 타입 일치를 추가한다. 기존 override・이름・소스 컨트롤・파일 해시・새 목적지 제한은 유지한다. 사용자 C++ 빌드 후 메모리 검사부터 실행하고, 모두 통과해야 새 에셋 저장・별도 재로드로 간다.
4. 대안은 별도 스테이징 프로젝트에 원본 package 경로를 유지하는 방법이나, 대상 기존 경로와의 충돌・후속 병합 범위를 새로 확인해야 한다. 매크로 펼치기/자체 함수 대체는 원본 그래프 구조를 바꾸므로 우선하지 않는다. 선택안의 의도된 동작 차이는 **게임 규칙 변경이 아니라 새 복사본 내부 참조의 일관성과 원본 편집 방지**이며 런타임 동등성은 별도 검증이다.

- 이번 확인 작업에서는 읽기 전용 조사 스크립트와 상태 문서만 추가/갱신했다. 새 C++・게임 Config・활성 경로・Git 인덱스는 수정하지 않았고 C++ 빌드・패키징도 실행하지 않았다. 당장 사용자에게 다시 빌드를 요청할 변경은 없다.
- 현재 **R5-3 에셋 준비 진행 중**, 새 도구/의존성 변경 묶음 승인 대기다. 이전 Actor 2개 보존 성공은 유지되지만 전체 라이플 이식・활성 플레이어・PIE・발사/재장전・복제는 미완료다. 이 조사로 과거 971개 대량 복사/저장 충돌의 원인까지 확정하거나 해결했다고 주장하지 않는다.

### 2026-10-06 후속 승인 — 목적지 한정 복사・closure18 코드 준비

사용자가 앞선 Editor 복사 도구 보강 묶음을 승인했다. 이번에는 코드/스크립트와 정적 검사만 수행했으며 **C++ 빌드・패키징・Unreal 프로세스・에셋 복사는 실행하지 않았다**.

**변경 파일과 원본 근거**

- `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/Private/DCRifleScopedCopy.h/.cpp` 신규: 엔진 `ObjectTools::DuplicateSingleObject`와 `FArchiveReplaceObjectAndStructPropertyRef`를 사용한다. 후자는 엔진 `ForceReplaceReferences`도 사용하는 UObject/FField 치환 API지만, 이 도구는 전역 ForceReplace/Consolidate/AdvancedCopy를 호출하지 않는다. 이번 실행에서 만든 정확한 패키지의 객체만 순회하고 외부 객체 직렬화 진입을 거부한다. 미해결 soft object path도 정확한 package 경계로 치환하며 임의 문자열/바이너리 헤더는 수정하지 않는다.
- `DCRifleMigrationLibrary.h/.cpp`, `DCRifleLoadDirtyPolicy.h/.cpp`, `DCRifleInheritedOverrides.cpp` 갱신: 새로운 엔진 개별 복사 경로, 원본 보호/저장 게이트, 공개 타입 조회, 고정 closure18 정책을 연결한다. 명시적 Compile에는 `SkipSave`를 사용한다.
- 모듈 Build.cs에 Editor 전용 `BlueprintGraph`, `KismetCompiler` 의존성을 추가했다. 실제 로컬 5.8 헤더의 공개 MacroInstance getter, DynamicCast/CallFunction/Pin 타입을 조회하기 위한 것이며 게임 런타임 모듈・엔진 소스・uproject・Config는 변경하지 않았다.
- `Private/Tests/DCRifleScopedCopyTests.cpp` 신규, 기존 LoadDirtyPolicy/NameGuards 테스트 확장. 기존 InheritedOverrides 테스트는 유지한다.
- `Scripts/Editor/r5_rifle_core_contract.py`, `r5_rifle_core_stage.py`, `tests/test_r5_rifle_core_contract.py` 갱신. 16개 원본 getter는 이전 조사/비교용으로 유지하며 새 준비 스크립트는 별도 closure18 getter를 요구한다. 구버전 DLL로 조용히 실행하지 않는다.

**고정 범위・보존 계약**

- 원본 핵심 16개 + `/Game/Audio/Blueprints/WeaponAudioMacros` + `/ShooterCore/System/Audio/WeaponAudioFunctions` **18개 전체 집합**만 일반 16개 상한의 추가 예외로 허용한다. 임의 17/18개, closure18+추가 에셋, 보존/파일 감시 비활성은 거부한다. 상호 의존 타입이 실제로 일치하는지는 검사로 확인해야 하며, 18개가 전체 시각/오디오 종속성 완성본이라는 뜻은 아니다.
- 원본 Pistol Actor・WeaponInstance・Fire・Reload 4개를 복사 전 파일 감시/로드 대상으로 추가했다. **Niagara dirty 허용 목록은 기존 4개 그대로**다. Pistol을 저장하거나 dirty 해제하지 않는다.
- 개별 복사 전에 로드된 모든 BP의 부모, Generated/Skeleton 클래스 객체, Compile 상태, dirty 상태, authored graph 타입 참조, 실제 패키지/동반 파일 해시를 캡처한다. 개별 복사/Compile/기존 override 복원 및 저장 게이트에서 불변을 확인한다. 모든 UObject 속성의 전수 비교를 뜻하지 않는다.
- `ReadTypeReferences`/`InspectBlueprintTypeReferences`는 공개 API로 매크로 graph/owner, 함수 member owner, cast target, 핀 subtype/member owner/map value subtype을 조회한다. 컴파일러 임시 확장 그래프는 비교하지 않고 `UBlueprint::GetAllGraphs`의 작성 그래프를 사용한다. 원본과 새 경로로 번역된 복사본을 대조하고 차이를 보고한다. 보호 필드 flag를 완화하거나 reflection 우회로 읽지 않는다.
- 원본 부모/형제는 명시적으로 재부모화하거나 Compile하지 않는다. 엔진 개별 복사의 내부 Compile 등 간접 영향은 원본 보호 검사에서 검출해야 하므로 **실제 라이플에서 원본 무변경이 검증됐다고 아직 선언하지 않는다**.
- 엔진 `PostDuplicateBlueprint`도 내부 컴파일을 수행하는 것을 확인했다. `Save on Compile`이 `Never`가 아니면 메모리 진단 전 거부하고 사용자 설정은 바꾸지 않는다. SourceControl 비활성・전용 unattended commandlet・새 목적지/동반 파일 없음・맵/외부 패키지 거부를 유지한다. BP 정의 클래스로 생성된 별도 데이터 인스턴스처럼 새 경로가 아직 지원하지 않는 자산은 실패로 보고한다.
- 의도된 게임 동작 변경은 없다. 원본 라이브러리 그래프를 펼치거나 자체 함수로 대체하지 않고 복사본 내부 참조만 맞춘다. 기존 승인된 Hero 입력 핀 변경은 여전히 새 Base 복사본에만 적용한다. Hero 핀 변경 후 추가 타입 차이가 발견돼도 자동 예외로 숨기지 않고 로그/원본 근거로 확인한다.

**자동화・검증 결과와 남은 절차**

- core runner에 읽기 전용 `inspect` 모드를 추가했다. 새 공개 getter로 원본 18개 및 보호 형제 BP의 타입을 기록하고, 파일/dirty 검사를 수행한다. 실제 실행은 사용자 빌드 후다.
- memory/prepare는 C++의 `scoped_copy_used`, `original_blueprints_unchanged`, 기존 override/load-dirty 검사 성공을 모두 요구한다. 이후 Python 비교/Hero 변경/저장 구간도 원본 18개+형제4의 타입/클래스 identity/상태와 원본 파일을 재검사한다. 새 복사본만 저장하며 별도 verify/plan에서 참조/데이터/타입과 원본 파일을 다시 확인한다.
- 새 C++ `ScopedReferences` 테스트는 임시 Parent/Rifle/미선택 Pistol, typed 매크로와 매크로 사용 함수 라이브러리, 함수 호출 노드를 만든다. 복사본 부모/매크로/함수/핀 참조 일치, 원본 형제의 부모・생성 클래스・dirty 유지, 파일 생성 없음, 기존 목적지 거부를 검사하고 잘못된 매크로 참조/원본 부모 변경을 의도적으로 만들어 검출 여부를 검사한다. 테스트 자신의 임시 패키지에만 clean 기준을 설정하며 원본 게임 패키지에는 이 동작을 적용하지 않는다.
- **실행한 검사:** 순수 Python unittest **6건 통과**, 변경 Python 3개 AST 문법 검사, `git diff --check` 통과. Unreal 파일 생성/복사/저장이나 C++ 컴파일은 실행하지 않았다.
- **미검증:** UHT/C++ 빌드, 확장된 MigrationTools **4건** 실행, 실제 라이플 18개 타입 조회・메모리 복사・저장・재로드, Hero 변경 후 타입 일치, 원본 형제 보호 실증, PIE/복제. 과거 3건 통과 로그는 새 코드의 통과 증거가 아니다.
- 다음: 사용자 **DreamCatcherEditor / Development Editor / Win64 빌드** → Codex의 `DreamCatcher.MigrationTools.ExplicitCopy` 4건 → 별도 원본 Lyra `inspect`/preflight → 새 실행명 memory → 통과하면 다른 새 실행명 prepare → 별도 verify/plan. 어느 단계든 실패하면 저장/후속 이식은 중단하고 근거를 확인한다. 사용자가 Blueprint 노드를 다시 만들 필요는 없다.
- 현재 **R5-3 보강 코드 준비 완료・사용자 빌드 대기**다. 기존 성공/실패 진단 에셋, 활성 무기, 게임 C++/Config, 사용자 staged 변경, Git 인덱스는 이번에 수정하지 않았다.

### 2026-10-06 00:55~00:59 KST — 빌드 성공, 타입 조회 완료, 검사 조건 교정 필요

**실행 결과**

- 사용자 UBT 로그: `C:/Users/Min/AppData/Local/UnrealBuildTool/Log.txt`, **00:54:34 시작・39.10초・Result: Succeeded**. 새 UHT 코드와 ScopedCopy/ScopedCopyTests CPP, DLL 링크를 확인했다. DLL 갱신은 **00:55:13 KST**다. Codex는 빌드・패키징하지 않았다.
- `Saved/Logs/R5ScopedPolicyChecks.log`, `Saved/Automation/R5ScopedPolicyChecks/index.json`: **00:57:16 KST 성공 2/실패 2**, 경고 동반 성공 0. InheritedOverrides/LoadDirtyPolicy는 통과, NameGuards/ScopedReferences는 실패했다. 테스트 프로세스 종료 코드는 0이었으므로 **종료 코드만으로 성공 판정하면 안 된다**.
- NameGuards: `Exact approved eighteen accepted` 실패. 순수 이름 검사 함수 `ValidateNames`가 `IsValidLongPackageName`을 호출해 등록된 마운트까지 검사한다. DreamCatcher에는 `/ShooterCore` 마운트가 없고 원본 Lyra에는 있으므로 테스트 환경에 종속됐다. 로컬 엔진 `PackageName.cpp`의 `IsValidTextForLongPackageName`(문자열 형식)과 `IsValidLongPackageName`(형식+마운트) 구현을 대조했다. 실제 원본 프로젝트의 같은 18개 사전 검사는 아래와 같이 통과했다.
- ScopedReferences: **fixture 생성 전에 실행 조건에서 실패**했다. 등록은 `EditorContext`지만 본문은 `IsRunningCommandlet()`을 요구한다. 기존 `UnrealEditor-Cmd -ExecCmds=Automation RunTests ...`는 headless Editor Automation이며 Commandlet이 아니다. 엔진 `AutomationTest.cpp`는 EditorContext와 CommandletContext를 별도로 선택한다. 두 프로젝트의 현재 SaveOnCompile은 `SoC_Never`이며 이번 실패를 자동 저장 설정 문제로 취급하지 않는다.
- 저장/복사를 차단한 상태에서 독립적인 원본 조회만 수행했다. 원본 `Saved/Logs/R5Closure18-inspect.log`: **00:58:56 preflight 통과・00:59:07 inspect 완료**, 프로세스 0, `asset_writes=0`, 명시적 Compile/copy 없음. 핵심18 + Niagara4 + 보호 Pistol4의 파일/동반 파일 **104개 상태 불변**을 확인했다. 원본 BP21개는 모두 dirty 0이다.
- 공개 C++ 조회에서 `GCN_Weapon_Rifle_Fire.OnBurst`의 매크로 graph가 `/Game/Audio/Blueprints/WeaponAudioMacros.WeaponAudioMacros:LyraGetWeapon`, 함수 소유 클래스가 원본 WeaponAudioFunctions인 것을 확인했다. 매크로의 실제 cast target은 `/Game/Weapons/B_Weapon.B_Weapon_C`이며 `WeaponAudioFunctions.SetWeaponSoundParams`는 같은 라이브러리의 `LyraGetWeaponAmmo`를 참조한다. 이전 Python 보호 필드 미조회였던 이 범위는 이제 실제 조회 근거가 있다. 복사본 타입 일치까지 검증한 것은 아니다.

**당시 교정안 — 후속 승인/코드 적용은 아래 기록 참고**

1. `ValidateNames`의 원본 경로는 엔진의 텍스트 형식 검사와 기존 `/Game`/`/ShooterCore` 허용 범위를 사용해 마운트 유무와 분리한다. 실제 복사 preflight에서는 마운트/원본 파일 존재 검사를 계속 수행한다. 허용 루트・개수・경로 경계・덮어쓰기 보호는 완화하지 않는다.
2. 임시 `ScopedReferences` 테스트만 등록된 Editor Automation과 일치하도록 별도 unattended/headless 프로세스 및 명시적 진단 플래그를 요구한다. 실에셋 복사 API의 `IsRunningCommandlet()` 보호를 제거하지 않는다. 대안인 전용 Commandlet 테스트 runner 신설은 추가 도구가 필요해 이번의 작은 교정보다 범위가 크다.
3. 실패 시 구체적인 경로 검사 이유를 테스트 출력에 남긴다. 원본 게임 그래프/타입・18개 목록・게임 규칙은 변경하지 않는다. 이는 원본 이식 방식 변경이 아니라 Codex가 추가한 테스트/검사 조건의 오류 교정이다.
4. 승인 후 교정 → 사용자 C++ 재빌드 → MigrationTools 4건 통과 확인 → 새 실행명 memory → 통과 시 prepare/별도 verify. 아직 ScopedReferences 본체와 실제 18개 복사 경로가 실행되지 않았으므로 추가 오류가 없다고 단정하지 않는다.

이번 확인에서는 상태 문서만 갱신했다. C++・스크립트・에셋・Config・Git 인덱스는 수정하지 않았으며 memory/prepare/verify나 활성 무기 교체도 실행하지 않았다. 현재 **R5-3 진행 중・검사 조건 교정안 승인 대기**다.

### 2026-10-06 후속 승인 — 이름/마운트・테스트 실행 조건 교정 완료

사용자가 앞서 제안한 두 검사 조건의 코드 수정을 승인했다. **기존 Editor 플러그인의 CPP 3개와 상태 문서만 수정**했으며 C++ 빌드・패키징・Unreal 실행・에셋 작업은 하지 않았다.

- `Private/DCRifleMigrationLibrary.cpp`: `ValidateNames`에서 원본/목적지의 문자열 형식을 `FPackageName::IsValidTextForLongPackageName`으로 검사한다. `/Game`・`/ShooterCore` 허용 원본, 신규 진단 실행 폴더 경계, 개수/중복/파일명 제한은 그대로다. 실제 `Preflight`에서 양쪽 경로의 `IsValidLongPackageName` 검사와 원본 `.uasset` 존재/마운트 경로 해석/기존 목적지 거부/해시 검사를 수행한다. 순수 형식 검사 통과만으로 복사할 수 있게 만든 것이 아니다.
- `Private/Tests/DCRifleMigrationValidationTests.cpp`: `/ShooterCore` 이름을 플러그인 마운트/실제 에셋 존재와 독립적으로 검사하는 항목을 추가했다. 허용되지 않은 플러그인, 비슷한 접두사의 루트, 원본 경로 traversal은 거부한다. 성공 예상 검사 실패 시 `ValidateNames`의 구체적인 거부 이유를 출력한다.
- `Private/Tests/DCRifleScopedCopyTests.cpp`: EditorContext 등록과 실행 조건을 맞췄다. **Editor・비 Commandlet・게임 스레드・비 PIE・unattended・nullrhi・DCRifleCopyDiagnostics・SaveOnCompile=Never**가 필요하다. 임시 테스트에만 적용하며 실에셋 복사 API의 `CheckExecutionContext`/Commandlet 요구와 저장 보호는 변경하지 않았다.
- 원본 근거는 이전 조사에서 대조한 UE 5.8 `PackageName.h/.cpp`의 텍스트/마운트 검사 구분과 `AutomationTest.cpp`의 EditorContext/CommandletContext 구분이다. `CoreGlobals.h`의 Editor/PIE 상태 선언도 확인했다. Lyra 게임 그래프/동작・18개 목록・dirty 예외・복사/참조 치환 구현 자체는 이번 수정 범위가 아니다.
- **검증:** `git diff --check` 통과, 기존 순수 Python 계약 테스트 6건 회귀 통과. `Source`/`Config`/도구 Source **337개 파일**의 전후 SHA-256 대조에서 위 CPP 3개만 변경됐고 나머지 변경/삭제는 없다. Python 테스트는 수정한 C++의 실행 성공 증거가 아니다.
- **미검증:** 이번 CPP 재빌드, NameGuards/ScopedReferences를 포함한 MigrationTools 4건 재실행, 실제 라이플 메모리 복사/저장/재로드. 이전 00:57의 2/4 결과를 수정 후 통과로 바꾸지 않는다. Blueprint 노드 수동 수정은 필요하지 않다.
- **다음:** 사용자 `DreamCatcherEditor / Development Editor / Win64` 재빌드 → Codex가 별도 headless Editor에서 MigrationTools 4건 실행(JSON 성공/실패 수 확인, 종료 코드만 보지 않음) → 모두 통과한 뒤 원본 Lyra의 새 실행명 memory → 통과 시 다른 새 실행명 prepare → 별도 verify/plan. 현재 **R5-3 교정 코드 완료・사용자 재빌드 대기**다.

### 2026-10-06 01:43~01:46 KST — 3건 통과, 임시 복사 목적지 오류 확인

- 사용자 UBT 로그: **01:43:41 KST 시작・8.91초・Result: Succeeded**, 직전 교정한 CPP 3개와 DLL 링크 성공. DLL 갱신은 **01:43:49**다. Codex가 빌드한 것이 아니다.
- `Saved/Logs/R5ScopedPolicyChecks-fixed.log`, `Saved/Automation/R5ScopedPolicyChecksFixed/index.json`: **01:46:40 KST**, 성공 **3**, 실패 **1**, 경고 동반 성공 0. InheritedOverrides/LoadDirtyPolicy/NameGuards는 모두 성공했고 ScopedReferences만 실패했다. 프로세스 종료 0을 테스트 전체 성공으로 취급하지 않는다.
- 이번에는 ScopedReferences의 실행 조건을 통과해 임시 Parent/Rifle/Pistol/매크로/함수 BP 생성/Compile까지 실행했다. 원본 보호 baseline은 로드된 BP 15개・파일 상태 24개를 캡처했지만, 첫 번째 개별 복사 `BP_Rifle`에서 **/Temp 목적지가 유효한 writable root가 아니라는 엔진 메시지**로 중단됐다. 참조 치환/복사본 Compile/최종 원본 불변 검사는 아직 통과하지 않았다.
- 근거: 로그 2315~2316의 `Cannot duplicate object: BP_Rifle` / `Path does not start with a valid root`; 엔진 `PackageName.cpp`는 `TempRootPath`를 `EMountFlags::ReadOnly`로 등록하고 `ObjectTools::DuplicateSingleObject`는 목적지를 `IsValidLongPackageName(..., bIncludeReadOnlyRoots=false)`로 검사한다. 테스트 fixture의 `RF_Transient`와 목적지 mount의 쓰기 가능성은 서로 다른 조건이다.
- 원본 라이플 에셋의 memory/prepare/verify는 실행하지 않았고 테스트 에셋 저장도 호출하지 않았다. 새 테스트 출력 폴더는 확인되지 않았으며 DreamCatcher `Content/LyraMigration/Rifle`도 여전히 없다. 이전 원본 타입 조회 및 파일104개 보존 결과는 별도 성공 이력이다.

**당시 최소 변경안 — 후속 승인/적용은 아래 참고**

- `Private/Tests/DCRifleScopedCopyTests.cpp`의 fixture를 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/ScopedFixture_<GUID>/` 아래 새 메모리 패키지로 구성한다. 생산 에셋을 넣거나 저장하는 것이 아니라 엔진 복사 API가 허용하는 마운트를 사용하는 테스트다. 임시 플래그・Compile 자동 저장 Never・새 목적지 제한・원본/복사본 파일 미생성 검사는 유지한다.
- `/Temp`를 writable로 재등록하거나 엔진의 writable-root 검사 및 실에셋 복사 보호를 제거하는 대안은 사용하지 않는다. 테스트만 저수준 복사로 우회하면 실제로 사용하는 `DuplicateSingleObject` 경로를 검증하지 못하므로 같은 엔진 API를 유지한다. 게임 동작/원본 라이브러리 구조 변경은 없다.
- 이 경로 오류는 Codex가 만든 테스트 구성의 오류다. 승인 후 **테스트 CPP 1개 교정 → 사용자 재빌드 → 4건 재검사 → 모두 통과 시 실제 closure18 memory/prepare/verify** 순서다. 이후 단계에서 추가 문제가 없다고 아직 단정하지 않는다.
- 이번 확인에서는 상태 문서만 갱신했고 C++・스크립트・Config・Git 인덱스는 수정하지 않았다. 현재 **R5-3 진행 중・임시 테스트 목적지 교정안 승인 대기**다.

### 2026-10-06 후속 승인 — 임시 테스트 fixture 경로 교정 완료

- 승인 범위에 따라 `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/Private/Tests/DCRifleScopedCopyTests.cpp` **1개만 코드 변경**했다. fixture 루트는 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/ScopedFixture_<GUID>/`이며 실행마다 새 GUID를 사용한다. `/Source`에 테스트 원본, `/Copy`에 테스트 복사본을 구성한다. 아직 이 경로의 패키지/폴더를 실행으로 생성한 것은 아니다.
- BP를 만들기 전에 `IsValidLongPackageName(..., false, &RootReason)`으로 writable-root를 확인하고 실패 이유를 출력한다. 이는 이전 테스트의 잘못된 `/Temp` 목적지를 바로잡는 것으로, 엔진 mount를 재등록하거나 실에셋 복사 코드를 변경하지 않는다.
- 원본 fixture 패키지의 `RF_Transient`, 무인/headless/non-PIE 실행, `SaveOnCompile=Never`, 명시적 Compile의 `SkipSave`, 기존 목적지 거부와 원본/복사본 파일 미생성 assertion을 유지했다. 새 저장 호출은 없다. 원본 Lyra 게임 동작・매크로・라이플 에셋・Config・스크립트・모듈 설정은 변경하지 않았다.
- **검증:** 정적 검토와 `git diff --check` 통과. `Source`/`Config`/도구 Source **337개 파일**의 SHA-256 전후 비교에서 위 CPP 1개만 변경, 삭제 0. C++ 빌드・테스트 실행・Unreal/에셋 작업・패키징은 하지 않았다. 이전 3/4 결과를 수정 후 전체 성공으로 표시하지 않는다.
- **재개:** 사용자 `DreamCatcherEditor / Development Editor / Win64` 재빌드 → MigrationTools 4건 재검사 → 모두 통과할 때만 원본 closure18 memory → 별도 prepare → fresh-process verify/plan. 수동 Blueprint 노드 변경은 없다. 현재 **R5-3 테스트 경로 코드 교정 완료・재빌드/실행 검증 대기**다.

### 2026-10-06 02:11~02:13 KST — 도구 4건 통과, 실제 메모리 검사에서 실행 프레임/타입 차이 확인

**완료한 검사**

- 사용자 UBT 로그: **02:11:01 KST 시작・6.52초・Result: Succeeded**, ScopedCopyTests 컴파일/DLL 링크, DLL 갱신 **02:11:07**. Codex는 빌드・패키징하지 않았다.
- `Saved/Logs/R5ScopedPolicyChecks-gameroot.log` 및 `Saved/Automation/R5ScopedPolicyChecksGameRoot/index.json`: **02:12:41**, InheritedOverrides/LoadDirtyPolicy/NameGuards/ScopedReferences **4건 성공, 실패・경고 0**. 임시 parent/child/sibling/typed macro/function fixture의 참조 일치와 원본 상태 유지, 파일 미생성, 의도적인 불일치/기존 목적지 거부가 통과했다. fixture 폴더도 디스크에 없다.
- 이후 승인된 실제 원본 18개를 별도 Lyra Python commandlet에서 **메모리에만** 복사했다. 로그 `Saved/Logs/R5Closure18-memory.log`, 실행명 `RifleCore_Memory_0213`이다. 보호 대상 Pistol4와 Niagara4도 감시했다.

**전체 성공이 아닌 이유**

- **02:13:48** `PointerToUberGraphFrame->UberGraphFunctionKey == BPGC->UberGraphFunctionKey` ensure 발생. 대상은 새 `B_WeaponInstance_Rifle`의 CDO이며 엔진 메시지는 새 `B_WeaponInstance_Base`와 실행 프레임 key가 달라 참조 순회가 안전하지 않다고 한다. 콜스택은 `RemapCopies`의 첫 archive 참조 치환을 가리킨다. 이 ensure를 단순 경고로 무시하거나 삭제해서 진행하면 안 된다.
- 현재 보조 도구는 ensure를 실패 게이트에 포함하지 않아 **02:13:53 native result success=true**를 반환했다. 원본 BP guard(71 BP/284 file states), source/dirty 검사, override 1개 비교와 Hero 변경 전 **18개 주요 데이터・그래프・타입 검사**는 통과했지만 이 부분 결과가 엔진 오류를 상쇄하지 않는다. 전체 copy 성공으로 기록하지 않는다.
- **02:13:54** 기존 승인된 새 Base의 Hero 입력 타입 핀 1개를 LyraCharacter로 바꾼 뒤, **02:13:55** 타입 비교가 아래 정확히 한 subtype 차이를 거부했다.
  - `EventGraph.K2Node_CallFunction_1|ReturnValue1|Type`
  - 원본 기대값 `/ShooterCore/Game/B_Hero_ShooterMannequin.B_Hero_ShooterMannequin_C`
  - 실제 `/Script/LyraGame.LyraCharacter`
- Python이 오류로 종료했고 프로세스는 **종료1**, `complete` 표식은 없다. 원본/목적지 저장, prepare, fresh-process verify/plan은 실행하지 않았다. 새 `RifleCore_Memory_0213` 디스크 폴더는 없다. 실행 전 로그의 fingerprint와 종료 후 디스크를 직접 대조해 **104개 파일/동반 파일 존재・MD5 차이0**을 확인했다.

**원본/엔진 근거와 당시 변경안 — 후속 승인/적용은 아래 참고**

1. 엔진 `Class.cpp`의 `UStruct` 직렬화는 `Ar << SuperStruct`를 포함하고, 현재 도구는 새 generated class/CDO까지 archive로 치환한다. `BlueprintGeneratedClass.cpp::AddReferencedObjectsInUbergraphFrame`은 인스턴스의 frame key와 현재 class의 key 일치를 검사한다. 이 소스 경로와 실제 콜스택은 **부모 클래스 연결만 먼저 바뀌고 실행 메모리 수명 처리가 뒤따르지 못하는 문제**를 가리킨다. 모든 내부 포인터 전이를 디버거로 실측한 것은 아니며 구체 교정의 성공은 아직 미검증이다.
2. 새 복사본 부모 교체는 엔진의 재부모화/Compile 수명 절차를 사용하고, 기존 generated class의 상속 관계를 먼저 일반 참조처럼 치환하지 않도록 순서를 보강하는 안을 제안한다. 로컬 `UBlueprintEditorLibrary::ReparentBlueprint`는 ParentClass 변경, SCS root 검증, RefreshAllNodes, SkipSave 및 reinstancing 관련 옵션으로 Compile을 수행한다. 이 API 자체도 계층 변경 시 데이터 손실 가능성을 경고하므로 기존 원본 속성/그래프/override/형제 보호 비교를 유지하고 **새 복사본에만** 적용한다.
3. 부모 EventGraph의 실제 실행 프레임이 존재하는 상속 fixture를 추가한다. 현재 단순 fixture 4건 통과만으로 실제 라이플 경로를 보장하지 않는다. `FDebug::GetNumEnsureFailures()` 등 엔진 진단을 작업 전후 확인해 ensure 발생 시 다음 치환/복원/저장 단계로 넘어가지 않도록 한다. 현재처럼 return=true만 보고 진행하지 않는다.
4. 원본 `Source/LyraGame/Equipment/LyraEquipmentInstance.h`의 `GetTypedPawn`은 **`meta=(DeterminesOutputType=PawnType)`**이다. 따라서 승인된 입력 타입 변경에 따른 반환 핀 subtype 변화에는 원본 근거가 있다. Python 기대값은 patched Base의 실제 GetTypedPawn 노드/ReturnValue 핀 **1개에 한정**해 Hero→LyraCharacter를 허용하고, 변경 전/다른 노드/다른 에셋/추가 타입 차이는 계속 거부하는 테스트를 함께 작성하는 안이다. 모든 Hero 참조를 일괄 치환하거나 검사를 제거하지 않는다.
5. 대안은 원본 package 경로를 유지하는 별도 스테이징이지만 기존 대상 경로와 충돌 검토가 필요하다. 원본 EventGraph를 삭제/단순화하거나 engine ensure를 끄는 방법, 원본 Pistol에 영향을 주었던 전역 consolidate 복귀는 사용하지 않는다. 의도된 게임 규칙 변화는 없으며 복사/검사 도구의 수명 처리와 기존 승인된 핀 변경의 기대값을 바로잡는 범위다.

이번에는 문서만 갱신했다. C++・스크립트・Config・Git 인덱스와 활성 게임 경로는 변경하지 않았다. 현재 **R5-3 실제 에셋 준비 진행 중・위 보강 묶음 승인 대기**이며, 승인 후 코드/회귀 fixture 작성 → 사용자 빌드 → 자동 검사 → 새 메모리 검사에서 **ensure 0・모든 비교 통과**를 확인해야 저장 단계로 진행한다. 이전 대량 저장 충돌까지 해결됐다고 판단하지 않는다.

### 2026-10-06 후속 승인 — 부모 수명 처리・엔진 진단 차단・Hero subtype 기대값 보강

사용자가 앞선 보강 묶음을 승인했다. 아래는 **코드/검사 준비 완료**이며, 새 C++ 빌드・Unreal 실행・에셋 복사/저장・패키징은 하지 않았다.

**복사/컴파일 수명 처리**

- `Private/DCRifleScopedCopy.cpp`: 모든 개별 복사 후, 새 BP의 부모가 복사 묶음에 포함된 원본 BP이면 **부모부터 엔진 `UBlueprintEditorLibrary::ReparentBlueprint`로 전환/Compile**한다. 일반 archive 치환보다 먼저 실행하고, 생성 클래스의 실제 superclass・Compile 상태・원본 BP 불변을 검사한다. 새 BP에만 적용하며 원본 부모/형제는 변경하지 않는다.
- archive가 새 클래스/CDO의 SuperStruct를 직접 바꿔 실행 프레임과 어긋나게 하지 않는다. 치환 맵에 아직 남아 있는 SuperStruct를 발견하면 순회/저장을 실패로 남긴다. `CLASS_NewerVersionExists`인 퇴역 클래스와 그 소유 객체/CDO는 컴파일러 수명 처리에 맡기고 편집하지 않는다. 기존 package 범위 검사/참조/속성/override 비교는 유지한다.
- `DCRifleMigrationTools.Build.cs`에 **Editor 전용 BlueprintEditorLibrary** 모듈 의존성을 추가했다. 원본 엔진 API를 호출하기 위한 것이며 엔진 코드・게임 런타임 모듈・Config・uproject는 변경하지 않았다. 실제 계층 전환이 복잡한 라이플 기본값을 완전히 유지하는지는 새 실행으로 검증해야 한다.

**엔진 진단 실패/저장 차단**

- 신규 `Private/DCRifleDiagnosticGuard.h/.cpp`: `FDebug::GetNumEnsureFailures()`와 scope 내 Error/Fatal 로그를 관찰한다. 엔진 로그/카운터를 숨기거나 초기화하지 않는다. 이전 ensure가 있는 프로세스도 거부한다. Warning을 Error로 오인하지 않지만 실제 에셋 경고는 로그에 남기며 별도로 검수한다.
- `DCRifleMigrationLibrary.h/.cpp`, `DCRifleInheritedOverrides.cpp`, ScopedCopy에 guard를 연결했다. 개별 로드/복사/재부모화/객체 참조 치환/override 복원/Compile 및 저장 직전・직후에 검사한다. 실행 중인 엔진 함수 내부를 강제로 중단하는 것은 아니며 **반환 후 다음 객체/수정 단계로 넘어가지 않도록** 한다. 저장 중 발생한 오류는 실패/부분 출력으로 남기며 완료로 간주하지 않는다.
- 결과의 `engine_diagnostics_clean`이 true여야 성공/저장이 가능하다. Blueprint/Python 공개 `GetEngineEnsureFailureCount`를 추가하고 runner에서도 native copy, Hero 핀 편집/Compile, bundle 비교, 저장 직전/직후/완료에 ensure0을 요구한다. Python Compile의 반환값과 기존 원본/파일 검사도 유지한다. 구버전 DLL에는 이 getter가 없으므로 runner가 재빌드를 요구한다.

**좁은 Hero subtype 기대값**

- `Scripts/Editor/r5_rifle_core_contract.py`에 `expected_type_references`를 추가했다. `patched=true`인 원본 Base만 대상으로, 원본의 **EventGraph.K2Node_CallFunction_1**, 클래스 **K2Node_CallFunction**, 제목 **GetTypedPawn**, 원본 PawnType 값과 ReturnValue 존재를 확인한다.
- 그 뒤 **`EventGraph.K2Node_CallFunction_1|ReturnValue1|Type` 한 항목만** 원본 Hero→LyraCharacter 기대값으로 변경한다. 원본 GetTypedPawn의 DeterminesOutputType 설정과 02:13 로그로 확인된 차이이며 전체 Hero 참조 치환이 아니다. 원본 목록은 수정하지 않고, 다른 에셋/변경 전/다른 핀/추가 차이와 누락/중복은 계속 실패로 검출한다.
- `r5_rifle_core_stage.py`는 이 기대값과 원본 graph를 함께 대조한다. 기존 승인된 입력 핀 편집 자체나 게임 동작은 추가로 바꾸지 않는다.

**회귀 검사와 수행 결과**

- `Private/Tests/DCRifleScopedCopyTests.cpp`: 기존 fixture의 부모에 **CustomEvent→Delay→PrintString**, 지연 실행을 넘는 문자열 입력을 연결했다. 부모 UberGraphFunction/frame property와 자식 CDO의 실제 persistent frame 존재를 검사하고 복사 후 frame 존재/key(validation 활성 시), 자식 기본 Tags 값, 원본 형제/참조/파일 보호를 확인하도록 했다. 이벤트를 실제 게임 월드에서 실행하는 검사는 아니다.
- 엔진 ReparentBlueprint의 알려진 `class hierarchy is changing, there could be possible data loss!` **Warning 정확히 1건**만 이 fixture에서 기대한다. 오류/ensure 또는 다른 경고를 일괄 무시하지 않으며 기본값/프레임/원본 보존 검사는 별도로 통과해야 한다.
- 신규 `Private/Tests/DCRifleDiagnosticGuardTests.cpp`의 **EngineDiagnosticGate**는 ensure/error 상태 거부와 오류 latch를 검사한다. 가짜 로그를 observer에 직접 전달하므로 실제 엔진 ensure를 발생시키거나 프로세스 카운터를 오염시키지 않는다. MigrationTools는 기존4개+신규1개 **총5건**이며 새 코드로 아직 실행하지 않았다.
- 변경 파일은 위 Editor 도구/헤더/Build.cs/검사와 Python contract/runner/test, 상태 문서다. source/hash 비교에서 게임 `Source`/`Config`는 유지됐으며 기존 staged 변경・활성 경로・원본 에셋・Git 인덱스는 건드리지 않았다.
- **실행한 검사:** Python unittest **10건 통과**, 변경 Python3개 AST 문법 검사, `git diff --check` 통과. DLL은 이전 사용자 빌드(02:11:07) 그대로다. **UHT/C++ 빌드・C++5건・실제18개 메모리/저장/재로드・PIE/복제는 미검증**이다.
- **다음:** 사용자 `DreamCatcherEditor / Development Editor / Win64` 재빌드 → 별도 headless Editor MigrationTools **5건** → 원본 Lyra 새 memory 실행에서 **ensure0/진단 clean/전체 비교 통과** → 다른 새 실행명의 prepare → fresh-process verify/plan. 기존 실패 실행을 덮어쓰지 않는다. 현재 **R5-3 보강 코드 완료・사용자 빌드 대기**다.

### 2026-10-06 02:42~02:58 KST — 핵심18개 준비본 저장・재로드 성공, 다음 의존성 계획

**완료 근거**

- 사용자 UBT 로그: **02:42:31 시작・19.48초・Result: Succeeded**, 새 UHT/진단 guard/수명 처리/테스트 CPP 및 DLL 링크 확인, DLL **02:42:50** 갱신. Codex는 C++ 빌드・패키징하지 않았다.
- `Saved/Logs/R5LifecycleChecks.log`, `Saved/Automation/R5LifecycleChecks/index.json`: **02:44:04**, EngineDiagnosticGate/InheritedOverrides/LoadDirtyPolicy/NameGuards/ScopedReferences **5건 성공, 실패・보고서 경고0**. 알려진 재부모화 계층 경고1건은 fixture에서만 기대했고, 실행 프레임 존재/key 및 기본값 보존・원본 형제 보호・파일 미생성・진단 차단을 검사했다.
- 원본 Lyra `Saved/Logs/R5Closure18-lifecycle-memory.log`: **02:45:24 complete**, `mode=memory`, `ensure_failures=0`, `new_saved_assets=0`, 종료0. 원본 상태 guard, 상속 override1개 및 승인된 Hero 변경 전후의 18개 데이터/그래프/타입 비교가 통과했다. 이전 UberGraphFrame 오류가 재현되지 않았다.
- 원본 `Saved/Logs/R5Closure18-lifecycle-prepare.log`: **02:46:46 complete**, `mode=prepare`, **new_saved_assets=18**, `original_saves=0`, ensure0, 종료0. Unreal 내부 API로 해당 새 실행의 복사본만 저장했다.
- 새 저장 경로: **`/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/`**. 실제 디렉터리는 `E:/Epic Games/UEProjects/LyraStarterGame/Content/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/`. 지정된 core16+원본 audio macro/function2의 `.uasset` **18개**만 존재한다. 이전 Seed/Prepared/실패 진단본을 덮어쓰거나 삭제하지 않았다.
- 원본 `Saved/Logs/R5Closure18-lifecycle-reload.log`: **02:48:47 complete**, 별도 프로세스 `mode=verify`, asset writes0, 종료0. **18개 직접 package 참조 비교 모두 missing=[]/unexpected=[]**, 주요 데이터/그래프/타입・상속 override expected1/compared1/match1 통과. 원본 및 동반 파일 **104개**, 저장본 및 동반 파일 **72개** 상태 불변을 확인했다. 재로드 후 원본 BP 상태/타입과 ensure0 조건도 통과했다.
- 실제 원본 프로세스에는 엔진 재부모화 안내 Warning4건과 기존 원본 Death Cue 중복 Warning이 기록됐다. 자동 테스트 보고서의 경고0과 혼동하지 않는다. ensure/Error는 없으며 비교 범위는 선택한 데이터/graph/type/override이지 모든 UObject 속성・시각・런타임 전수 검증은 아니다.

**다음 단계의 읽기 전용 조사 결과**

- verify의 dependency plan은 **1228개 package**다: Game967(핵심 준비본18 포함), ShooterCore2, Niagara201, ControlRig56, AudioModulation1, AnimationLocomotionLibrary1. Engine/Script70은 별도로 집계했고 조회 불가0, **원본 핵심18 참조0・원본 Hero 도달=false**다. 1228개가 전부 새로 복사해야 할 파일 수라는 뜻은 아니다.
- 신규 `Scripts/Editor/r5_rifle_dependency_plan.py`를 원본 Lyra 별도 commandlet에서 실행했다. AssetRegistry의 직접 참조와 최단 참조 경로, 대상 파일의 SHA-256만 읽는다. **명시적 에셋 로드/Compile/복사/저장/Migrate 없음**, 로그 `Saved/Logs/R5Closure18-dependency-plan.log`, complete/asset_writes0/종료0. 구문 검사도 통과했다.
- 남은 ShooterCore 참조 2개:
  1. 준비본 `W_Reticle_Rifle` → `/ShooterCore/Input/Abilities/Struct_UIMessaging`
  2. 준비본 `GA_Weapon_Fire_Rifle_Auto` → `/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto`
- DreamCatcher에 기존 `/Game` 경로 **114개**가 겹친다(Audio, Effects, UI Reticle, 무기 Mesh/Material/Texture 등). `.uasset` 및 동반 파일 비교에서 byte-identical0・차이114를 확인했다. 저장 이력/메타데이터/버전 등에 따른 차이일 수 있어 **게임 값/참조가 실제로 다르다거나 덮어써야 한다는 결론은 아니다**. 아직 기존 파일을 덮어쓰거나 무조건 재사용하지 않았다.
- DreamCatcher에는 `/Game/LyraMigration/ADS/Struct_UIMessaging`가 이미 있다. 새 Reticle과 기존 ADS가 같은 메시지 타입을 사용할 수 있는지 원본/이식본 구조와 연결을 먼저 대조해야 한다. `GE_Damage_RifleAuto`의 대상 동명 파일은 확인되지 않았다. 이름만으로 기존 다른 Damage GE를 같은 것으로 간주하지 않는다.
- 현재 DefaultEngine의 일부 R3/R4 Redirect는 준비돼 있지만 이번 무기/장비/Reticle 핵심 native 타입의 필요한 추가 Redirect 목록은 아직 적용하지 않았다. 실제 이식 전 원본 타입/함수/구조체를 대응시켜 변경안을 제시한다. 충돌 채널/실사용 Pawn/장비/발사 연결도 아직 다음 작업이다.

**완료 범위와 재개 원칙**

- 완료는 **R5-3의 핵심18개 원본 기반 준비본 생성・저장・재로드 하위 검증**이다. DreamCatcher의 `Content/LyraMigration/Rifle`은 여전히 없으며, 실제 Migrate・활성 무기 교체・PIE・발사/재장전・복제는 미완료다. R5/R6 전체 완료로 보고하지 않는다.
- 이번 직접 변경은 위 원본 프로젝트 신규 진단 에셋18개, 읽기 전용 의존성 조사 스크립트, 상태 문서다. 게임/도구 C++・Config・uproject・Git 인덱스는 수정하지 않았다. 원본 라이플/공용 에셋 저장 및 기존 진단본 삭제는 하지 않았다.
- 다음은 **114개 공용 의존성의 실제 값/참조 비교, 남은 ShooterCore2개와 기존 ADS struct 연결, native Redirect/충돌 없는 이전 목록 조사**다. 안전한 읽기 전용 조사는 이어갈 수 있지만, 기존 에셋 덮어쓰기・새 Config/코드・대량 참조 교체/이전은 구체적인 범위와 원본 차이를 설명하고 승인을 확인한다. 이번 결과 뒤 사용자에게 다시 빌드나 수동 Blueprint 노드 작성을 요청할 필요는 없다.

### 2026-10-06 14:45~15:58 KST — 공용114개 읽기 전용 비교 및 다음 준비 묶음

**진행 범위/보존 결과**

- 이전 핵심18개 저장/재로드 성공을 재확인했고, 이번에는 기존 에셋의 실제 속성/참조와 native 연결 제약을 조사했다. C++・Config・uproject・게임 에셋・활성 경로・Git 인덱스는 수정하지 않았으며 빌드/패키징/명시적 Blueprint Compile/에셋 저장은 실행하지 않았다.
- 신규 `Scripts/Editor/r5_rifle_shared_audit.py`: 정확한 공용114개와 지정된 보조 에셋만 조회하고 Unreal 표준 `ObjectExporterT3D`, 공개 Python 속성 및 지원되는 Texture exporter로 **비교 보고서만** 생성한다. 보호 필드 flag를 바꾸거나 읽기 제한을 우회하지 않는다. 읽지 못한/deprecated 필드는 명시한다.
- 신규 `Scripts/Editor/r5_compare_shared_audit.py`: 두 프로세스의 로그/보고서를 오프라인 비교한다. 이미 설정된 ClassRedirect만 원본 표기에 적용하고, 관측된 **Unbound multicast delegate의 프로세스 주소만** 양쪽에서 정규화한다. GUID나 임의 native 데이터를 지워 같게 만들지 않는다. ZIP 보존된 대형 보고서도 읽을 수 있다.
- `r5_rifle_dependency_plan.py`에 AssetRegistry의 parent/native-parent metadata 조회를 추가했다. GetAsset/GetClass나 에셋 로드 없이 필요 native 타입을 조사한다.
- 최종 공용 비교 로그: 원본/대상의 `Saved/Logs/R5SharedAudit-full2.log`. 원본 **14:57:25 KST**, 대상 **14:57:44**, 각각 종료0/complete・ensure0・asset_writes0. 원본116에셋/동반 **464개**, 대상115에셋/동반 **460개** 파일 상태가 유지됐다. 원본 Niagara3개에 load-dirty가 있었으나 저장/dirty 해제하지 않았고 파일 불변을 확인했다. 대상 dirty0, 로드 실패0이다.
- 메시에 대한 별도 `R5SharedAudit-mesh.log`도 양쪽 종료0/저장0/ensure0으로 완료했다. 최종 원본 Reticle 조회 `R5SharedAudit-reticle.log`는 **15:58:21 KST**, 에셋/동반12개 불변, 저장0/ensure0이다.

**초기 진단 실패와 도구 교정 이력**

- 첫 대상 조회는 `/Game/Audio/AttenuationPresets/TempITDSpatializationSourceSettings` 로드에 실패했다. 로그의 `Failed to find script package /Script/Spatialization` 및 `ITDSpatializationSourceSettings class does not exist`가 근거다. 이후 **진단 명령줄에만** Spatialization을 활성화해 조회했다. 프로젝트 파일/전역 오디오 설정은 바꾸지 않았으므로 실제 게임/패키징에서의 플러그인 준비는 별도다.
- 첫 원본 전체 조회는 지원하지 않는 텍스처 형식에 TGA exporter를 명시해 `SupportsTexture(Texture)` assert로 중단됐다. 해당 실행은 완료 검사 통과가 아니다. 엔진 `UExporter::RunAssetExportTask`는 명시된 exporter의 class만 확인하지만 자동 `FindExporter`는 SupportsObject/SupportsTexture까지 확인한다. 조사 스크립트를 자동 선택 및 TGA→DDS→HDR 지원 확인 방식으로 교정해 최종 전체 조회를 완료했다. 엔진 검사/에셋 형식을 변경하거나 원본을 저장하지 않았다.

**비교 결과 — 동등성 범위에 주의**

| 검사 | 결과 | 해석 제한 |
|---|---:|---|
| 로드된 공용 에셋 | 114/114 양쪽 | 진단 프로세스의 Spatialization 활성화 조건 |
| 조회 가능한 공개 속성 | 113/114 일치 | 모든 native/bulk/editor-only 속성 전수 검사는 아님 |
| 표준 텍스트 export | 88/114 일치 | 나머지 메타데이터/캐시/버전 차이를 무조건 동작 차이로 보지 않음 |
| Texture2D 픽셀 export | 12/12 동일 SHA256 | TGA/DDS 등 지원된 원본 이미지 내보내기 기준, 실제 화면 검증은 별도 |
| 직접 package 참조 집합 | 100/114 일치 | 나머지14개는 추가 검토 필요, 목록 차이만으로 삭제/교체하지 않음 |

- 최초 공개 속성 비교의 9개 차이 중 SoundSubmix8개는 모두 `on_submix_recorded_file_done`의 **동일한 Unbound 상태・서로 다른 메모리 주소**였다. 이를 에셋 설정 차이로 보고하지 않았다. 남은 SK_Rifle의 `source_models`는 원본 cached TriangleCount/VertexCount/Bounds가 미계산값인 반면 대상에는 캐시 수치가 있다.
- 실제 editor subsystem 조회에서 양쪽 SK_Rifle는 **LOD1개・render vertex11255・section1**로 일치한다. 원본/대상 빌드/감소 설정도 조회된 범위에서는 같지만, vertex/index/skin weight/physics 전체 데이터를 비교한 것은 아니므로 메시 완전 동등성은 미검증이다.
- 참조 차이에는 머티리얼3개의 확장된 함수 참조, Niagara9개의 DataHierarchyEditor/일부 ENiagaraExpansionMode 차이, CameraShake의 GameplayCameras→EngineCameras 모듈 표기, Damage 부모의 LyraGame→DreamCatcher native 경로가 포함된다. 이 때문에 기존114개를 일괄 덮어쓰거나 무조건 재사용하지 않는다.

**남은 ShooterCore2개와 native 제약**

- 원본/기존 ADS의 `Struct_UIMessaging`은 `ON`(bool, False), `Controller`(Engine.PlayerController, None)의 멤버 설명과 **멤버 GUID까지 일치**한다. 그러나 struct GUID는 원본 `1D6423D04594B93C6E61E996ECD5FB8C`, 기존 ADS `F3AA7B2C417585FCF3416C8E9C931B93`로 다르고 대상에는 PropertyIDs metadata가 추가돼 있다. 같은 UObject/메시지 타입이라고 가정하지 않는다. **기존 ADS 구조체를 단일 타입으로 쓰는 참조 연결안 + 새 Reticle Compile/메시지 검증**을 후속 제안하며 아직 교체하지 않았다.
- 원본 `GE_Damage_RifleAuto`는 Instant, LyraDamageExecution, Source/BaseDamage snapshot의 **ScalableFloat 보정12**, DamageTaken Cue 및 Basic/Instant/Rifle 태그를 사용한다. 12가 감쇠/속성 등을 포함한 최종 피해량이라는 뜻은 아니다. 기존 SetByCaller 테스트 GE와 같다고 취급하거나 손으로 재작성하지 않는다.
- `W_Reticle_Rifle`은 `CircumferenceMarkerWidget`(CrossHairs/SetRadius)을 사용하나 대상에 해당 UMG/Slate 타입이 없다. 원본 `UI/Weapons/CircumferenceMarkerWidget.h/.cpp`, `SCircumferenceMarkerWidget.h/.cpp` **4파일**을 직접 확인했다. 대상 Build.cs에는 UMG/Slate/SlateCore가 이미 있다.
- 원본 Reticle의 WidgetTree/생성 클래스 CrossHairs 실제 값은 **OutsideRadius=true, Radius24**다. 원본 Slate 생성자/Construct에는 `bReticleCornerOutsideSpreadRadius`의 명시적 초기화가 없고 UMG Rebuild/Synchronize도 이를 전달하지 않지만, OnPaint 계산은 이 값을 읽는다. 원본과의 픽셀 단위 동일성이 보장된 상태가 아니다.
- `WeaponAudioFunctions.SendWeaponFire`는 GetGameState→Cast To LyraGameState→B_MusicManagerComponent_Base 조회/Receive Weapon Fire 흐름을 가진다. 현재 대상에 LyraGameState 대응 클래스가 없으며 원본 GameState 생성자는 ExperienceManagerComponent를 만든다. 원본 ExperienceManager는 ExperienceDefinition/ActionSet/Manager, AssetManager, SettingsLocal 등도 사용한다. 빈 GameState 대체/임의 cast 일반화나 원본 기능 삭제는 승인 없이 하지 않는다. 전체 원본 의존 묶음은 다음 범위를 정하기 위한 조사 대상이다.
- AssetRegistry native metadata에서 B_LyraGameInstance 및 ABP_Mannequin_Base도 의존성에 포함됨을 확인했다. DCGameInstance/DCAnimInstance의 존재만으로 원본 native API 전체 호환을 가정하지 않는다. 그래프 타입 조회는 class/struct 일부만 포착하므로 native Redirect 목록도 아직 전수 완료가 아니다.

**당시 제안 — 아래 후속 승인으로 1~2번 코드/설정 적용, 3번은 별도 범위 확정 필요**

1. 원본 Circumference UMG/Slate **4파일**을 별도 DCLyra 계층으로 이식하고 최소 자동 검사를 추가한다. 이름/include/module 연결 외 계산/브러시/각도/스케일은 유지한다. 위 옵션은 **명시 초기값과 UMG→Slate 값 전달만 최소 보강**하는 안을 먼저 설명한다. 원본을 무수정 복사하면 값 전달/초기화 문제를 그대로 남기므로 권장하지 않는다. 보강 후 표시가 에셋의 OutsideRadius 옵션을 따르도록 하는 차이가 있고, 원본의 실제 화면과 동일한지는 사용자 시각 검증으로 남긴다.
2. 해당 클래스/struct Redirect와 Spatialization 플러그인의 정식 활성화를 준비한다. 전역 오디오 장치/믹서 선택이나 음량 변경은 포함하지 않는다. 기존114개 덮어쓰기/삭제, 전체 Migrate, 활성 Pawn/무기 전환은 이 묶음에서 하지 않는다.
3. GameState/Experience/관련 GameInstance 의존성은 원본 보존 범위를 별도로 확정한다. 규모가 크다는 이유만으로 이식 불가/생략 처리하지 않는다. UI 준비 선행은 R7 일부를 R5-3에서 수행하는 것이며 R7 전체 완료가 아니다.

**보고서 관리/현재 상태**

- 표준 text export가 texture 내부 데이터까지 크게 출력해 진단 보고서가 약9.7GB 생성됐다. 이번 생성분을 `Saved/Diagnostics/R5SharedAudit_20261006.zip`(**2,451,315,359 bytes**)에 보존하고, 큰 파일36개는 ZIP entry의 길이/SHA256이 원본 보고서와 일치함을 확인한 뒤 정리했다(원본 보고서9,561,878,995 bytes). ZIP에서 복원 가능하며 **.uasset/.umap/프로젝트 소스는 삭제하지 않았다**. 작은 보고서/로그와 비교 스크립트는 유지했고 비교기는 ZIP fallback을 지원한다.
- 이 조사 종료 당시 Python 조사/비교/registry 스크립트 AST3개 및 `git diff --check`를 통과했고, Editor 보조 도구 DLL은 사용자 빌드02:42:50 그대로였다. 당시 새 C++/설정은 미적용이었으며 아래 후속 작업으로 현재 사용자 재빌드가 필요한 상태로 바뀌었다.
- 이 조사 종료 당시 **R5-3 공용 의존성 읽기 전용 비교 완료・준비 묶음 승인 대기**였다. 현재 코드 상태는 바로 아래 후속 승인 기록을 따른다. 전체 공용 에셋 동등성, GameState/Experience 연결, 대상 라이플 이식/활성 연결/PIE/복제는 여전히 미완료다.

### 2026-10-06 후속 승인 — Circumference UI 선행 코드/설정 완료, 빌드 대기

**범위와 원본 대비 차이**

- 사용자 `진행하자` 승인으로 위 제안 **1~2번만** 처리했다. R5-3의 라이플 의존성 해결을 위한 R7 일부 선행 준비이며 R5/R7 전체 완료가 아니다.
- 원본 `Source/LyraGame/UI/Weapons/CircumferenceMarkerWidget.h/.cpp`, `SCircumferenceMarkerWidget.h/.cpp`를 `Source/DreamCatcher/UI/Weapons/Lyra/` 아래 **DCLyraCircumferenceMarkerWidget / SDCLyraCircumferenceMarkerWidget** 이름으로 이식했다. reflected entry는 `FDCLyraCircumferenceMarkerEntry`다. 원본 저작권・속성명・SetRadius API를 유지했다.
- 원본 메서드의 배치/회전/OnPaint/ComputeDesiredSize/반경/목록 갱신 계산은 그대로다. 특히 OutsideRadius 계산의 양 축 모두 `ImageSize.X * 0.5`를 쓰는 원본 코드도 임의 변경하지 않았다. 원본 Rifle의 Radius24/OutsideRadius=true를 클래스 기본값으로 강제하지 않았고, 클래스 Radius48 기본값 및 에셋별 설정 구조를 보존했다.
- 승인된 최소 보강은 **OutsideRadius의 명시적 false 초기값, Slate 생성 인자 및 setter, UMG Rebuild/Synchronize 값 전달**이다. 원본의 초기화/전달 누락을 그대로 복제하는 대안 대신 이 경로를 적용했다. 이후 원본 에셋 옵션true가 실제 표시 계산에 반영되며, 원본의 미초기화 Slate 실행 결과와의 픽셀 동일성은 보장하지 않는다. 실제 시각 검증은 사용자 확인으로 남긴다.
- 최소 include/module export와 transient 테스트용 friend 접근자를 추가했다. 테스트 접근자는 Blueprint/게임플레이 API가 아니며 생산 에셋/전역 설정을 수정하지 않는다. 이미 UMG/Slate/SlateCore 의존성이 있으므로 Build.cs는 변경하지 않았다.
- `Config/DefaultEngine.ini`에 원본 `CircumferenceMarkerWidget` 클래스와 `CircumferenceMarkerEntry` struct의 Redirect **2개만** 추가했다. `DreamCatcher.uproject`의 Spatialization을 Enabled=true로 등록했다. `/Script/Spatialization.ITDSpatializationSourceSettings` 원본 의존성을 준비하는 설정이며 **전역 spatializer 선택・믹서・음량 변경이 아니다**.

**검사 코드 / 아직 실행 전**

`Source/DreamCatcher/Tests/DCCircumferenceAutomationTests.cpp`에 다음 **3건**을 작성했다.

| 테스트 (`DreamCatcher.R5.Reticle.Circumference.*`) | 확인 대상 |
|---|---|
| SlateGeometry | false 기본값/true 생성 인자, 사방 배치, 원본 이미지 중심 회전, OutsideRadius 켜기/끄기, HUDScale, 비정방형 브러시의 원본 X폭 사용, 원하는 크기 |
| WidgetLifecycle | transient UMG 생성, 생성 전후 SetRadius, MarkerList/OutsideRadius 전달, Synchronize, 외부 참조 해제 후 Release/rebuild의 현재값 유지 |
| DependencyRegistration | Config 클래스/struct Redirect와 native soft-path fixup 후 타입 로드, Spatialization source-settings 클래스 로드 |

- EditorContext 검사이며 저장 에셋/월드/활성 HUD/오디오 설정을 변경하지 않는다. 소스 LyraGame 모듈이 없는 대상에서 raw soft-path load가 먼저 없는 패키지를 시도하는 로컬 엔진 동작을 확인해, 검사에서는 **CoreRedirect fixup 후 로드**한다. 실제 라이플 BP의 import/Compile 검증을 대신하지 않는다.
- 정적 검증: 타입 이름만 정규화한 **원본 메서드7개 본문 일치**, `.uproject` JSON 유효/Spatialization 중복 없음, `git diff --check` 통과. 이 결과는 C++ 컴파일/링크나 자동 검사 실행 성공이 아니다.
- **빌드・패키징・에디터 실행・에셋 저장은 하지 않았다.** 기존 코드/스테이징/미커밋 변경은 보존했다. GameState/Experience, 기존114개 공용 에셋, ADS 구조체 연결, 전체 Migrate, 활성 Pawn/무기 전환은 미변경이다.

**다음 확인 절차**

1. 사용자가 에디터를 종료하고 **Development Editor / Win64 C++ 빌드**한다. 새 reflected 타입이 있으므로 기존 DLL만으로 검사하지 않는다.
2. 빌드 확인 후 Codex가 별도 새 에디터 프로세스에서 위 검사3건을 실행한다. **진단용 `-EnablePlugins=Spatialization`을 주지 않고** 정식 `.uproject` 설정으로 타입 등록을 확인한다.
3. 기존 `/Game/Audio/AttenuationPresets/TempITDSpatializationSourceSettings`의 읽기 전용 재로드와 관련 회귀 결과를 확인한다. 직접 에셋 참조/소리・Reticle 화면・PIE・복제는 각각 별도 검증으로 유지한다.
4. 이후 GameState/Experience/관련 GameInstance 등 원본 의존 묶음의 정확한 변경 범위를 제시하고 승인을 확인한다. 규모만으로 원본을 생략하거나 빈 GameState를 넣지 않는다.

### 2026-10-06 16:32 KST 후속 검증 — UI 의존성 통과, Experience 선행 묶음 제안

**이번 검증 결과**

- 사용자 UBT 로그: **16:25:32 시작・25.76초・Result: Succeeded**. UHT5개 생성 파일과 Circumference2개 CPP/검사 CPP가 컴파일되고 `UnrealEditor-DreamCatcher.dll`이 **16:25:57, 3,326,464 bytes**로 갱신됐다. Codex는 C++ 빌드/패키징을 실행하지 않았다.
- 별도 headless Editor의 **16:28:14 `Saved/Automation/R5CircumferenceChecks/index.json`**: DependencyRegistration/SlateGeometry/WidgetLifecycle **성공3・실패/보고서 경고0**. 실제 표시/픽셀/PIE 검증이 아니다. 이 실행의 콘솔 로그는 지정한 상대 log 인수가 분리되어 `Saved/Logs/DreamCatcher.log`에 기록됐으므로 고정 근거는 별도 JSON 보고서를 사용한다.
- 별도 Python commandlet의 **16:30:45 `Saved/Logs/R5CircumferenceDependencies.log`**: 포팅 UMG/entry와 Spatialization native 타입, `/Game/Audio/AttenuationPresets/TempITDSpatializationSourceSettings`의 실제 클래스 로드 통과. 진단용 `-EnablePlugins=Spatialization` 없이 `.uproject` 설정만 사용했다. 파일/동반4개 상태 불변(동반 파일 부재 상태 포함), asset_writes0・dirty0・ensure0・종료0이다. 사운드 청취/오디오 출력/패키징 검증은 아니다.
- 재실행 가능한 `Scripts/Editor/r5_circumference_dependency_verify.py`를 추가했다. 대상 프로젝트/정확한1개 에셋 경로 고정, Spatialization 명령줄 override 거부, 파일 해시 대조 및 dirty/ensure 검사이며 save/Compile/copy API를 사용하지 않는다.
- **16:32:06 `Saved/Automation/R5UIPrerequisiteRegression/index.json`**: Equipment.QuickBarLifecycle・TeamColor.ObserverAndController・Weapon.ItemTagCost・Weapon.SpreadAndAttenuation **성공4・실패/보고서 경고0**. 임시 월드/합성 데이터 범위이며 실제 Pawn/네트워크/시각 검증과 구분한다.
- 이번 확인에서 게임 C++/Config/기존 에셋/활성 경로/Git 인덱스는 미변경이다. 사용자 빌드 결과를 확인하고 독립 검사만 실행했으며 신규 코드 묶음은 적용하지 않았다.

**다음 변경 안내 전 원본 근거・제약・대안**

1. 원본 `GameModes/LyraGameState.cpp` 생성자는 ASC와 **ExperienceManagerComponent를 실제 생성**한다. 오디오 그래프의 Cast를 통과시키려고 빈 GameState를 만들거나 임의 일반 Cast로 바꾸는 것은 원본 보존이 아니다.
2. `LyraExperienceManagerComponent.h`는 `ILoadingProcessInterface`를 상속한다. 원본 CommonLoadingScreen은 대상 Plugins에 아직 없으며, `LoadingScreenManager.cpp::ShouldCreateSubsystem`은 비전용서버 GameInstance에서 생성하고 Tick으로 로딩 상태/viewport를 관리한다. 플러그인 활성화가 타입 등록만 하는 것으로 취급되지 않도록 실제 영향 검증이 필요하다.
3. `LyraExperienceManagerComponent.cpp::OnExperienceFullLoadCompleted`는 로드 완료 알림 후 `LyraSettingsLocal::OnExperienceLoaded`를 호출한다. 원본 `LyraSettingsLocal.cpp:537,547,1221`에서 ApplyNonResolutionSettings까지 이어지고 오디오 ControlBus 볼륨・사용자 설정 등을 적용한다. 대상에는 DCSettingsShared가 있지만 SettingsLocal은 없으며 두 타입을 동일하게 취급하지 않는다. 연관 Performance/Audio/PlatformEmulation 의존성의 전체 폐쇄 집합은 아직 미검증이다.
4. **대안:** 전체 GameState/Experience/LoadingScreen/SettingsLocal을 한 번에 이식할 수 있는지는 추가 원본 감사와 전역 설정 변경 승인이 필요하다. 원본 설정 호출을 삭제/no-op 처리하는 축소안은 제안하지 않는다. 먼저 독립 가능한 데이터/요청 관리 계층을 옮기면 활성 게임을 유지하며 원본 재사용 준비를 진행할 수 있다. 전체 기능을 제외하는 것이 아니라 검증/승인 경계를 나누는 것이다.

**당시 승인 제안 — 아래 후속 승인으로 6파일/검사 코드 반영**

- `Source/DreamCatcher/GameModes/Lyra/DCLyraExperienceDefinition.h/.cpp`
- `Source/DreamCatcher/GameModes/Lyra/DCLyraExperienceActionSet.h/.cpp`
- `Source/DreamCatcher/GameModes/Lyra/DCLyraExperienceManager.h/.cpp`
- 대응하는 독립 자동 검사 CPP: 데이터 기본값/Action 검증, 엔진 subsystem의 중복 요청・마지막 요청 해제 분기. 실제 GameFeature 플러그인 활성화/PIE 동시 실행과 구분한다.
- **변경 차이:** 이름/include/module을 대상에 연결하고 원본 DefaultPawnData 포인터를 이미 이식한 `UDCPawnData`에 대응한다. 데이터 검증/asset bundle 수집/요청 카운트 로직은 보존한다. EngineSubsystem은 타입 등록 후 존재할 수 있으나 경험 로드나 플러그인 활성화를 자동 시작하는 코드는 추가하지 않는다.
- **제외:** 기존 GameMode/GameState 교체, ExperienceManagerComponent/SettingsLocal/LoadingScreen 활성화, CoreRedirect/PrimaryAssetTypesToScan 등 Config, 실제 Experience 에셋, 공용114개 덮어쓰기와 라이플 Migrate. 새 클래스 이름에 따른 PrimaryAssetType・원본 AssetManager scan 경로 연결은 후속 실에셋 단계에서 별도로 검증한다.
- 이 묶음은 R5-3의 음악/라이플 native 의존성 때문에 R13 일부를 앞당기는 준비다. **GameState/Experience/R13 전체 완료가 아니며**, 현재 플레이어 초기화・음악・로딩 화면은 그대로다. 다음 사용자 승인 후 코드 작업하고 빌드는 사용자에게 넘긴다.

### 2026-10-06 후속 승인 — Experience 기반6파일/검사 코드 완료, 사용자 빌드 대기

**실제 반영 범위**

- 사용자의 `진행하자` 승인으로 위 6파일과 `Source/DreamCatcher/Tests/DCExperienceFoundationAutomationTests.cpp`만 추가했다. `GameModes/Lyra/`의 원본 대응 클래스3종이며 활성 GameState를 대신하는 빈 클래스나 SettingsLocal 호출을 삭제한 ManagerComponent는 만들지 않았다.
- 원본 `LyraExperienceDefinition/ActionSet/Manager` h/cpp와 타입명/include/module export 차이를 정규화한 **6파일 전체 비교가 일치**했다. Definition의 DefaultPawnData는 이미 이식된 `UDCPawnData`로 대응하고 Epic 저작권, 원본 데이터 validation, Blueprint 재상속 제한, action bundle 수집, PIE plugin 요청 카운트/마지막 해제 판단을 보존했다.
- 공개 타입의 DLL 연결 및 Definition CPP의 PawnData include만 보강했다. 기존 GameFeatures 의존성을 사용하므로 Build.cs 변경은 없다. 새 Manager는 원본 EngineSubsystem이지만 실제 Experience 로드/GameFeature 활성화 코드를 추가하거나 기존 GameMode에 연결하지 않았다.
- 엔진 `UPrimaryDataAsset::GetPrimaryAssetId`는 native instance에 클래스명을 쓰므로 새 PrimaryAssetType은 **DCLyraExperienceDefinition / DCLyraExperienceActionSet**이다. 이를 원본 LyraExperience 이름과 같다고 보고하지 않으며 Config scan/Redirect/실제 BP 자산의 ID 연결은 후속 실에셋 범위로 남긴다. 임의 ID override는 추가하지 않았다.

**작성한 검사 — 빌드/실행 전**

테스트 prefix: `DreamCatcher.R5.ExperienceFoundation`

| 검사 | 이번 코드의 검사 범위 |
|---|---|
| DataContracts | transient 원본 기본값, PawnData/ActionSet 타입 연결, native PrimaryAssetId/CDO 구분, null Action2개 및 자식 Action의 오류 전달/정상 validation |
| ActionBundles | 기존 엔진 AddComponents Action을 생성만 하고 활성화하지 않음. Client/Server bundle 수집, 전환 시 이전 bundle 제거, null Action skip, Actions 제거 후 bundle 초기화 |
| PluginRequestArbitration | GUID 합성 키A의 요청3개와 B1개를 교차 해제, 마지막 요청만 해제 허용, 같은 키의 다음 주기. 실제 GameFeaturesSubsystem 호출 없음 |

- 테스트 전용 reflected 클래스를 추가하지 않고 구체 엔진 Action을 재사용한다. bundle은 inherited reflected AssetBundleData를 읽어 검사하며 생산 CDO/에셋을 수정하지 않는다.
- 요청 카운트 검사는 원본 API가 프로세스 EngineSubsystem을 사용하므로 **별도 unattended UnrealEditor-Cmd 프로세스＋`-DCR5ExperienceIsolatedAutomation`** 없이는 거부한다. 기존 map 전체를 초기화하는 `OnPlayInEditorBegun`은 호출하지 않고 합성 키의 요청/해제를 균형 있게 실행한다.
- native 기본값/분기 코드 검증이며 실제 plugin 활성화/비활성화, 여러 PIE의 동시 실행, 비Editor 분기, Blueprint 상속/Compile, 원본 Experience 에셋 import/PrimaryAsset scan/비동기 로드는 **미검증**이다.

**변경 보호/다음 절차**

- Config/DefaultEngine.ini, DreamCatcher.uproject, DreamCatcher.Build.cs, DreamCatcherGameMode h/cpp **5파일 SHA256 불변**을 작업 전후 확인했다. 기존 사용자 미커밋/스테이징 변경은 보존했다. 원본/대상 바이너리 에셋・활성 Pawn/무기・GameState・기존 플레이어 초기화 경로는 미변경이다.
- `git diff --check`와 원본6파일 정규화 비교는 통과했지만 **UHT/C++ 컴파일/링크・자동 검사3건 실행은 아직 미검증**이다. Codex는 C++ 빌드/패키징/에디터 실행을 하지 않았다.
- 사용자가 에디터 종료 후 **Development Editor / Win64 빌드**한다. 빌드 로그/DLL 갱신 확인 뒤 Codex가 위 prefix3건과 기존 회귀를 새 프로세스에서 실행하고 보고서 실패/경고/ensure를 확인한다.
- 다음 연결 범위는 여전히 원본 ExperienceManagerComponent・CommonLoadingScreen・SettingsLocal/관련 성능・오디오・GameState이며 별도 조사/승인 대상이다. 이 준비를 R13 전체 또는 R5 라이플 활성 연결 완료로 표시하지 않는다.

### 2026-10-06 16:48 KST Experience 독립/회귀 검증 통과와 다음 설정 데이터 제안

**빌드 및 검사 근거**

- 사용자 UBT 로그는 **16:47:10 시작・Result: Succeeded・17.85초**이며 `UnrealEditor-DreamCatcher.dll`은 **16:47:28 / 3,387,392 bytes**로 갱신됐다. 원본 기반 Experience CPP3개와 검사 CPP의 컴파일/링크를 확인했다. Codex는 C++ 빌드/패키징을 실행하지 않았다.
- 별도 unattended Editor에서 `-DCR5ExperienceIsolatedAutomation`, 기존 R5/Weapon 검사 opt-in을 지정하여 실행했다. **16:48:52 `Saved/Automation/R5ExperienceFoundationChecks/index.json`**은 성공10・실패0・보고서 경고0・notRun0이다. 프로세스 종료0만으로 성공을 판단하지 않고 개별 결과를 읽었다.
- 신규 `ActionBundles`, `DataContracts`, `PluginRequestArbitration`3건 및 기존 Circumference3건, Equipment/QuickBar1건, TeamColor1건, Weapon2건이 모두 통과했다. `Saved/Logs/R5ExperienceFoundationChecks.log`에서 ensure failure/Error/Fatal0도 확인했다.
- 이번 확인은 transient native 데이터・에셋 번들 수집・요청 카운트 분기와 기존 독립 회귀 범위다. **실제 Experience 비동기 로드/플러그인 활성화・동시PIE・BP 재상속/Compile・원본 import/PrimaryAsset scan・활성 GameState/라이플/복제는 미검증**이다.
- 게임 C++/Config/uproject/에셋/Git 인덱스를 바꾸지 않았다. Config/uproject/Build.cs/GameMode h/cpp의 해시도 이전 보호값과 같았다. 이번에는 문서만 갱신하며 추가 사용자 빌드는 필요하지 않다.

**추가 원본 조사 — 단순 타입 등록으로 볼 수 없는 부분**

- `Settings/LyraSettingsLocal.cpp:423`의 Get은 `GEngine->GetGameUserSettings()`를 **CastChecked**한다. 원본은 `DefaultEngine.ini`의 GameUserSettingsClassName으로 클래스를 선택하며, 대상에는 이 선택 설정/SettingsLocal 타입이 아직 없다. 클래스만 추가한 것으로 전역 설정 경로를 완료 처리할 수 없다.
- 동일 CPP는 PerformanceStatTypes/PerformanceSettings/AudioSettings를 직접 include한다. 이 데이터 타입은 재사용 가능한 원본 기반이며 먼저 준비할 수 있다. AudioMixEffectsSubsystem/PlatformEmulationSettings도 연결되지만 해당 계층의 실제 효과와 준비 상태를 구분한다.
- `Audio/LyraAudioMixEffectsSubsystem.cpp:37,225,352`에서 Game/PIE 월드에 생성되고 OnWorldBeginPlay에 기본/User mix 및 HDR/LDR 처리를 적용함을 확인했다. PostInitialize는 LoadingScreenManager 표시 delegate에도 연결된다. 이름을 별도로 만드는 것만으로 런타임 영향이 격리되지는 않는다.
- CommonLoadingScreen `LoadingScreenManager.cpp:150`은 비전용서버 GameInstance에 생성된다. `ShowLoadingScreen`은 위젯 생성/입력 처리 등록/성능 설정 변경을 하며, 원본 입력 processor는 **Editor에서는 입력을 소비하지 않고 비Editor에서 소비**한다. 위젯 로드 실패 시 Error를 기록하고 Throbber로 fallback한다.
- 원본 `Config/DefaultGame.ini`는 `/Game/UI/Foundation/LoadingScreen/W_LoadingScreen_Host.W_LoadingScreen_Host_C`와 `ForceTickLoadingScreenEvenInEditor=False`를 지정한다. 플러그인만 켜고 위젯/설정 참조를 확인하지 않은 상태를 원본 로딩 화면 이식 완료로 보지 않는다. 대상 위젯의 그래프/의존성 검증은 아직 하지 않았다.
- 대안은 이 전체 런타임 계층과 Config/실에셋을 한 묶음으로 전환하는 것이지만 전역 오디오/로딩/사용자 설정 영향 범위의 승인과 검증이 필요하다. 호출을 삭제하거나 placeholder로 대체하는 축소안은 적용하지 않는다. 아래는 그 연결 전 데이터 준비이며 전체 기능 보류를 이식 불가로 단정하지 않는다.

**당시 제안 — 아래 후속 승인으로 데이터5파일/검사 코드 반영**

1. `Source/DreamCatcher/Performance/DCLyraPerformanceStatTypes.h` (원본 enum 값/순서와 enum range).
2. `Source/DreamCatcher/Performance/DCLyraPerformanceSettings.h/.cpp` (원본 성능 설정 구조/플랫폼 옵션/기본 FPS 후보/표시 가능 stat 목록).
3. `Source/DreamCatcher/Audio/DCLyraAudioSettings.h/.cpp` (원본 ControlBus/Mix soft-reference 및 HDR/LDR chain 데이터).
4. 독립 자동 검사 CPP: 원본 C++ 기본값・성능 목록・enum 범위・오디오 참조 타입/빈 기본값 등. 실제 오디오 출력/벤치마크/DeviceProfile 적용은 하지 않는다.

- 이름/include/module 연결 외 원본 데이터 구조와 C++ 기본값을 유지한다. **새 타입만으로 원본 Config값이 이전되는 것은 아니므로** Config 섹션/실제 오디오 경로 연결은 후속으로 남긴다.
- 변경 제외: SettingsLocal 자체/전역 GameUserSettings 교체, AudioMixEffectsSubsystem, PlatformEmulationSettings, CommonLoadingScreen 플러그인 활성화, GameState/ExperienceManagerComponent, Config/Build.cs/uproject/에셋/활성 Pawn/무기. 검사에 예상 밖 모듈 의존성이 필요하면 먼저 범위를 설명한다.
- 이 데이터5파일/검사 묶음의 사용자 승인 후 작업한다. 빌드는 사용자 담당이며 현재 단계는 **R5-3/R13 선행 준비**다.

### 2026-10-06 후속 승인 — Performance/Audio 데이터5파일 코드 완료, 빌드 대기

**실제 반영 범위와 원본 대비 차이**

- 사용자의 `진행하자` 승인에 따라 `Source/DreamCatcher/Performance/DCLyraPerformanceStatTypes.h`, `DCLyraPerformanceSettings.h/.cpp`, `Source/DreamCatcher/Audio/DCLyraAudioSettings.h/.cpp` **5파일**을 추가했다. 원본 Epic 저작권과 데이터 선언/생성자/플랫폼 getter를 유지했다.
- 원본 `LyraPerformanceStatTypes`, `LyraPerformanceSettings`, `LyraAudioSettings`의 로컬 소스 전체를 읽고 이름/include/module export만 대응했다. Audio의 editor DisplayName도 새 클래스명에 대응하며, 원본5파일 전체의 정규화 비교가 일치했다. enum 값/순서・메타데이터의 Engine/AudioModulation 클래스 필터・soft-reference/chain 필드・기본 배열 수치는 보존했다.
- 원본 Desktop FPS 후보는 **30,60,120,144,160,165,180,200,240,360**, Mobile 후보는 **20,30,45,60,90,120**이다. 이는 설정 후보 데이터이지 현재 FPS 적용값이 아니다. 기본 표시 가능 stat은 Count를 제외한18개다.
- Audio C++ 기본값은 bus/mix soft path8개가 비어 있고 HDR/LDR chain 배열도 비어 있다. **원본 INI가 지정하는 실제 에셋 값까지 이식한 상태는 아니다.** 새 이름의 Game config 섹션/실에셋 경로는 아직 연결하지 않았고 원본 Config를 자동 복사/덮어쓰지 않았다.
- 원본 `PerPlatformSettings.Initialize`는 Editor에서 플랫폼 설정 관리자에 플랫폼별 데이터 객체를 준비한다. 이를 원본대로 유지하지만 DeviceProfile 선택/프레임 제한/벤치마크/오디오 적용은 호출하지 않는다. 기존 Engine/DeveloperSettings/GameplayTags 의존성으로 준비되므로 Build.cs는 미변경이다.

**작성한 독립 검사 — 아직 빌드/실행 전**

`Source/DreamCatcher/Tests/DCSettingsFoundationAutomationTests.cpp`, prefix **DreamCatcher.R5.SettingsFoundation**:

| 검사 | 작성 범위 |
|---|---|
| PerformanceEnums | 표시 모드4개・frame pacing 모드3개・stat18개와 Count의 원본 이름/ordinal/선언 순서, enum range가 Count를 제외하는지 |
| PerformanceDefaults | transient 설정의 Desktop/Mobile FPS 후보 목록, DesktopStyle/지원 flag/빈 profile 기본값, stat18개 그룹과 빈 query, variant 기본값, Game config 유형 |
| AudioSchemaAndDefaults | soft path8개 속성/Config/Edit flag/원본 AllowedClasses metadata/빈 기본값, HDR/LDR array entry 타입, Submix/Effect의 Engine typed soft reference 구조 |

- 실제 에셋 로드/오디오 재생・믹스・HDR/LDR 처리・설정 저장・자동 벤치마크・DeviceProfile/FPS 적용은 검사하지 않으며 관련 API를 호출하지 않는다. 생산 CDO를 수정하지 않고 transient 설정과 reflected 스키마만 확인한다.
- UHT/C++ 컴파일/링크, 새 자동 검사 실행, 플랫폼별 INI override, 실제 사용자 설정・소리・로딩 화면은 **미검증**이다. 원본5파일 정규화 비교와 diff/새 파일의 정적 검사는 통과했지만 실행 성공을 뜻하지 않는다.

**변경 보호와 다음 확인**

- Config/DefaultEngine.ini・DefaultGame.ini, DreamCatcher.uproject, DreamCatcher.Build.cs, 기존 GameMode/LocalPlayer h/cpp **8개 SHA256 불변**을 작업 전후 확인했다. 기존 미커밋/스테이징 변경은 보존했다. 원본/대상 바이너리 에셋과 활성 Pawn/무기/전역 사용자 설정은 미변경이다.
- AGENTS와 이 명세를 코드 완료/빌드 대기로 갱신했다. Codex는 C++ 빌드/패키징/에디터 실행을 하지 않았다.
- 사용자가 에디터를 종료하고 **Development Editor / Win64 빌드**한다. 새 DLL 확인 뒤 Codex가 위3건과 이전10건의 회귀를 별도 프로세스에서 실행한다.
- SettingsLocal・AudioMixEffectsSubsystem・PlatformEmulationSettings・CommonLoadingScreen・ExperienceManagerComponent/GameState와 실제 Config/위젯/오디오 참조 연결은 후속 원본 조사/변경안 승인 범위다. 이 데이터 준비만으로 전체 로딩/오디오/Experience 이식 완료로 표시하지 않는다.

### 2026-10-06 17:10 KST 설정 데이터 검증 통과 / 실행 계층 묶음 제안

**확인된 결과**

- 사용자 UBT 빌드: **17:09:33 시작・17.79초・Result: Succeeded**. UHT7개 생성 파일 및 Audio/Performance/검사 CPP 컴파일과 링크를 확인했다. DLL은 **17:09:50 / 3,470,336 bytes**로 갱신됐다. Codex는 C++ 빌드/패키징을 실행하지 않았다.
- **17:10:59 `Saved/Automation/R5SettingsFoundationChecks/index.json`**: SettingsFoundation3건＋이전10건 **성공13・실패/보고서 경고/notRun0**. `Saved/Logs/R5SettingsFoundationChecks.log` ensure failure/Error/Fatal0, 별도 unattended Editor 프로세스 종료0이다.
- 신규 PerformanceEnums/PerformanceDefaults/AudioSchemaAndDefaults가 모두 통과했다. 원본 C++ 기본값과 데이터 구조의 독립 검사이며 **실제 원본 INI/soft-path 자산 연결・오디오 출력・FPS 적용・DeviceProfile・실제 Experience/라이플/PIE/복제는 미검증**이다.
- 게임 C++/INI/uproject/Build.cs/에셋/인덱스는 이번 확인에서 수정하지 않았다. 보호 대상8개 해시는 이전 값과 같았다. 상태 문서만 갱신했으며 지금 추가 사용자 빌드는 필요하지 않다.

**다음 변경을 위한 추가 근거・구체적 제약**

- 남은 직접 의존성은 SettingsLocal→AudioMixEffectsSubsystem→CommonLoadingScreen 및 SettingsLocal→PlatformEmulationSettings, ExperienceManagerComponent→SettingsLocal/LoadingProcessInterface이다. GameState 생성자가 ExperienceManagerComponent를 생성한다. 준비된 데이터/Experience6파일을 다시 쓰지 않고 이 실행 계층을 연결할 수 있도록 다음 묶음으로 다룬다.
- `AudioMixEffectsSubsystem::ShouldCreateSubsystem/OnWorldBeginPlay`는 Game/PIE에서 자동 생성되어 기본/User mix 및 HDR/LDR chain을 적용한다. `LoadingScreenManager::ShouldCreateSubsystem`도 비전용서버 GameInstance에 생성된다. 새 타입명만으로 기존 플레이와 독립되는 것은 아니다.
- `PlatformEmulationSettings.cpp::PostInitProperties`는 `ApplySettings()`를 호출하고, ApplySettings는 `UCommonUIVisibilitySubsystem::SetDebugVisibilityConditions` 및 조건부 `SetEditorSimulatedPlatform`을 호출한다. 데이터를 추가하는 것과 달리 Editor 전역 상태를 갱신하는 경로다.
- 대상 `Content/UI/Foundation/LoadingScreen/W_LoadingScreen_Host.uasset`은 없으며 Content/Plugins의 `*LoadingScreen*.uasset` 파일명 조사에도 결과가 없다. 다른 이름의 동등 위젯 존재/동작은 미검증이다. 원본 LoadingScreenManager는 원본 위젯 로드 실패 시 Error＋Throbber fallback이며 이를 원본 연출 이식 완료로 볼 수 없다.
- SettingsLocal::Get은 전역 GameUserSettings에 CastChecked를 사용한다. 대상의 전역 설정 클래스는 아직 전환하지 않았으므로 실제 Experience 완료/설정 재적용 경로를 지금 활성화했다고 보고할 수 없다.

**당시 설명한 원본 보존 대안과 임시 차이 — 아래 후속 승인으로 코드 반영**

1. 원본 소스 그대로 즉시 활성화하려면 전역 GameUserSettings 선택, 로딩 위젯과 audio/config 자산, 기존 GameMode/LocalPlayer 소비처까지 함께 전환/검증해야 한다. 가능성을 배제하지 않지만 현재 승인 범위와 준비 상태를 넘는다.
2. 원본 내장 `-NoLoadingScreen`은 비Shipping에서 화면 표시만 억제한다. AudioMix와 PlatformEmulation 전역 상태까지 격리하는 수단이 아니며 완전한 대안으로 취급하지 않는다.
3. **권장:** 원본 본체를 함께 이식하되 **원본에 없는 임시 진단 opt-in**을 `LoadingScreenManager::ShouldCreateSubsystem`, `DCLyraAudioMixEffectsSubsystem::ShouldCreateSubsystem`, `DCLyraPlatformEmulationSettings::ApplySettings` 입구에 둔다. 일반 실행은 비활성, 별도 **unattended Editor＋`-DCLyraRuntimeDiagnostics`**에서만 해당 원본 경로를 허용한다. 이름/include/module/API 연결 외 나머지 본체/설정 호출을 삭제・no-op 대체하지 않는다.
4. 동작 차이는 **정상 실행에서 이 신규 자동 처리가 시작되지 않는다는 것**이다. 최종 구현으로 남길 조건이 아니며, 실에셋/설정 준비 후 별도 승인된 활성 경로 전환 때 임시 조건을 제거하고 정상 PIE/시각/오디오/입력 회귀를 검증한다. 비활성 상태를 기능 완료로 표시하지 않는다.
5. 이 조건 외 예상하지 못한 native 의존성/API 제약은 이식 불가가 아니라 미검증으로 기록하고, 새 변경/범위 확대 전에 근거와 대안을 설명한다.

**당시 승인 묶음 / 아래 후속 작업 이력 참조**

| 원본 기반 부분 | 대상/규모 |
|---|---|
| LyraSettingsLocal | Settings/DCLyraSettingsLocal.h/.cpp, 2파일 |
| LyraAudioMixEffectsSubsystem | Audio/DCLyraAudioMixEffectsSubsystem.h/.cpp, 2파일 |
| LyraPlatformEmulationSettings | Development/DCLyraPlatformEmulationSettings.h/.cpp, 2파일 |
| LyraExperienceManagerComponent | GameModes/Lyra/DCLyraExperienceManagerComponent.h/.cpp, 2파일 |
| LyraGameState | GameModes/Lyra/DCLyraGameState.h/.cpp, 2파일 |
| CommonLoadingScreen | Plugins/CommonLoadingScreen의 원본 소스8개＋Build.cs＋.uplugin, 10파일. 원본 Binaries/Intermediate/바이너리 에셋은 복사하지 않음 |

- 위 **원본 기반20파일** 외 독립 검사/필요한 최소 진단 보조 코드, DreamCatcher.Build.cs의 ApplicationCore/CommonLoadingScreen/AudioMixer/AudioModulation 의존성 및 DreamCatcher.uproject의 플러그인 등록을 포함하는 제안이다. 추가 모듈은 필요 근거 확인 후 안내한다.
- 기존 ASC/AssetManager/메시지/로그 및 이번까지 이식한 데이터/Experience 타입에 최소 연결한다. 활성 GameMode/LocalPlayer와 INI(GameUserSettingsClassName・PrimaryAsset scan/Redirect 포함), 실에셋/활성 Pawn/무기/음악 설정은 변경하지 않는다.
- 검사는 기본 비활성 상태 및 별도 진단 프로세스의 클래스/서브시스템 수명・LoadingProcess 등록/해제・GameState ASC/Experience 초기 상태를 대상으로 한다. 실제 원본 위젯・오디오 믹스・완료 Experience를 실행하기 전에는 별도 에셋/설정 승인을 확인한다. 기존13건 회귀와 글로벌 처리의 의도치 않은 시작 여부도 확인한다.
- 이 묶음의 임시 동작 차이까지 사용자 승인 후 코드 작업한다. 빌드/패키징은 사용자 담당이며 현재 단계는 R5-3/R13 선행 준비다.

### 2026-10-06 후속 승인 — 실행 계층20파일 코드 완료, 빌드 대기

**반영 범위**

- 사용자 `진행하자` 승인으로 위 SettingsLocal/AudioMix/PlatformEmulation/ExperienceManagerComponent/GameState h/cpp **10파일**과 CommonLoadingScreen 플러그인의 원본 소스8개＋Build.cs＋descriptor **10파일**을 추가했다. 원본 프로젝트의 Binaries/Intermediate/바이너리 에셋은 복사하지 않았다.
- Game 클래스는 DCLyra 별도 이름, 기존 ASC/AssetManager/LocalPlayer/PlayerState/메시지/로그 참조는 이미 포팅된 DC 타입으로 대응했다. `GetLyraAbilitySystemComponent` 같은 원본 GameState Blueprint API명은 유지했다. 원본 CVar 문자열과 실제 동작 수식/상태 전이는 보존했다.
- `.uproject`에 CommonLoadingScreen Enabled=true, DreamCatcher.Build.cs에 public ApplicationCore/CommonLoadingScreen 및 private AudioMixer/AudioModulation 의존성만 추가했다. 원본 plugin 모듈 유형 RuntimeNoCommandlet/PreEarlyLoadingScreen은 유지했다.
- SettingsLocal의 명시적 NativeGameplayTags/CsvProfiler include와 module export 등 최소 연결을 보강했다. INI/GameUserSettingsClassName, 기존 GameMode/LocalPlayer 및 에셋/활성 Pawn/무기는 변경하지 않았다. 따라서 새 SettingsLocal::Get은 기존 전역 설정을 가리키는 정상 실행에서 호출하면 안 되며, 실제 선택은 후속 승인된 전환에서 한다.

**승인된 임시 진단 조건의 실제 구현**

- `Plugins/CommonLoadingScreen/Source/CommonLoadingScreen/Public/DCLyraRuntimeDiagnostics.h`를 추가했다. 순수 컨텍스트 판단과 현재 프로세스 getter로 구성되며 **WITH_EDITOR＋GIsEditor＋비Commandlet＋unattended＋-DCLyraRuntimeDiagnostics**일 때만 true다. 패키징/일반 실행에서도 기본false이며 최종 기능 동작이 아니다.
- 입구3곳: LoadingScreenManager.ShouldCreateSubsystem, DCLyraAudioMixEffectsSubsystem.ShouldCreateSubsystem, DCLyraPlatformEmulationSettings.ApplySettings. false이면 본체에 진입하지 않고, true이면 원본 본체를 실행한다. 그 외 원본 설정/오디오/Experience 적용 함수는 삭제/no-op 처리하지 않았다.
- 등록된 타입/설정 데이터/CVar의 존재와 실제 자동 처리 활성화를 구분한다. 정상 활성 전환 때 임시조건 제거와 원본 UI/오디오/설정/입력 회귀가 필요하다. 활성 전환용 임의 설정 flag나 원본 기능을 대체하는 구현은 추가하지 않았다.
- LoadingScreenManager.h에는 transient 검사의 등록 목록 개수를 읽는 friend 접근자만 추가했다. Blueprint/게임플레이 debug API나 상태 변경용 우회는 아니다.

**독립 검사4건 작성 — 아직 실행 전**

`Source/DreamCatcher/Tests/DCLyraRuntimeFoundationAutomationTests.cpp`, prefix **DreamCatcher.R5.RuntimeFoundation**:

| 검사 | 범위 |
|---|---|
| DiagnosticGate | 16개 컨텍스트 조합 중 Editor/비Commandlet/unattended/opt-in 조합만 허용, 실제 프로세스 판단 일치 |
| SettingsDefaults | 새 CDO의 gamma/FPS/volume/replay/플랫폼 기본값 읽기, 기존 전역 GameUserSettings 객체와 gamma 미변경 확인. 새 Get/Load/Set/Apply/Save 호출 없음 |
| SubsystemsAndLoadingTask | 기본/진단 모드별 LoadingScreen・AudioMix 생성 여부, 원본 factory의 task 생성/이유 변경/등록・해제, GameInstance shutdown 후 제거/틱 종료 |
| GameStateInitialState | transient GameMode 인스턴스에서만 새 GameState 선택, ASC owner/avatar/replication 및 Experience의 미선택/로딩 요구 초기 상태 |

- 상태 관련3건은 **별도 unattended UnrealEditor-Cmd＋-DCR5RuntimeIsolatedAutomation**을 요구한다. `-run=pythonscript` 같은 Commandlet 경로와 사용자 interactive Editor에서는 거부한다. DiagnosticGate는 순수 판정 검사다.
- 임시 게임 월드를 만들기 전에 새 AudioSettings의 모든 실제 에셋 참조/chain이 비어 있는지 검사한다. 이후 GameInstance viewport가 없음을 확인해 위젯/입력 표시가 시작되지 않게 한다. 오디오 자산이 설정된 후에는 이 fixture로 실제 믹스를 활성화하지 않고 거부한다.
- 기본 모드는 RuntimeDiagnostics flag 없이, 진단 모드는 flag를 넣어 각각 새 프로세스에서 실행한다. 동일 프로세스의 명령줄/전역 조건을 중간에 바꾸는 방식은 쓰지 않는다. 이전13건 회귀도 함께 확인한다.
- 실제 Experience 선택/완료・SettingsLocal 전역 교체・위젯 표시/오디오 출력・동시PIE/네트워크/원본 asset import는 미검증이며 이번 fixture가 검증했다고 보고하지 않는다.

**정적 확인 / 남은 확인**

- 타입/include/module 대응과 위 guard3곳/test friend1곳을 제외한 **원본기반20파일 본문 정규화 비교 일치**, 보호 대상 INI/GameMode/LocalPlayer6파일 SHA256 불변, plugin/project JSON 유효/등록 중복 없음, diff/새 파일 정적 검사를 확인했다. 실제 UHT/C++ 링크/실행 성공은 아니다.
- 원본 LoadingScreenManager는 PreLoadMapWithContext에 등록하지만 Deinitialize에서 PreLoadMap을 해제한다. 확인된 이름 차이를 이번 범위에서 임의 수정하지 않았으며, **맵 전환 delegate 수명/완전 정리는 후속 진단 필요**다. 이번 shutdown/틱/등록 목록 검사와 구분한다.
- Codex는 빌드/패키징/에디터 실행을 하지 않았다. 작업 중 plugin Intermediate에 생성 주체를 확인하지 않은 Definitions 파일이 관측됐다. 이를 생성/복사하는 명령은 실행하지 않았으며, DLL/UBT 빌드 결과는 여전히 이전17:09 빌드였다.
- 다음은 사용자 **Development Editor / Win64 빌드** 확인 → 두 모드 신규4건 및 이전13건 회귀다. 새 컴파일/실행 제약은 실제 결과로 판단하고 예상 밖 변경이 필요하면 근거/범위를 먼저 설명한다.

### 2026-10-06 23:34 KST 실행 계층 검증 — 기본 통과, 진단1건 실패

**실행 근거**

- 사용자 UBT 빌드는 **23:28:48 시작・Result: Succeeded・52.93초**. 게임 DLL은 **23:29:40 / 3,724,288 bytes**, CommonLoadingScreen DLL은 **23:28:59 / 174,592 bytes**다. Codex는 C++ 빌드/패키징을 실행하지 않았다.
- **23:31:33** `Saved/Automation/R5RuntimeFoundationDefault/index.json`: 기본 진단 비활성 모드 **성공17・실패0・보고서 경고0**.
- **23:34:35** `Saved/Automation/R5RuntimeFoundationEnabled/index.json`: `-DCLyraRuntimeDiagnostics` 활성 모드 **성공16・실패1・보고서 경고0**. 둘 다 독립 unattended Editor이며 **프로세스 종료0이라도 실패1건을 완료로 간주하지 않는다**. 양쪽 로그의 ensure/Fatal은0이다.
- 진단 실패는 `DreamCatcher.R5.RuntimeFoundation.SubsystemsAndLoadingTask`의 `Expected 'Loading manager stops ticking on deinitialize' to be true.`1개다. 실제 assertion은 `Source/DreamCatcher/Tests/DCLyraRuntimeFoundationAutomationTests.cpp:189`이며 보고서의191은 실패 이벤트의 보고 위치다.
- 같은 실행의 나머지16건은 성공했다. task 생성/등록/이유 변경/해제와 GameInstance subsystem 제거 등 다른 assertion에서 오류는 보고되지 않았지만, 전체 종료/실제 맵 전환 동작이 검증 완료된 것은 아니다.

**실패 원인 — Codex 검사 의미 해석 오류**

- `Engine/Source/Runtime/Engine/Public/Tickable.h:96~106`은 `GetTickableTickType()`을 **첫 Tick 전에 초기 정책을 정하는 virtual 함수**로 설명한다. 현재 엔진 등록 상태를 반환하는 accessor가 아니다.
- `Tickable.cpp::FTickableGameObject::SetTickableTickType`은 `FTickableStatics::SetTickTypeForTickableObject`를 호출하고, Never는 대기/활성 등록 목록에서 객체를 제거한다. getter의 반환값을 바꾸지 않는다.
- 원본/대상 `LoadingScreenManager::GetTickableTickType`은 non-template 인스턴스에서 항상 Conditional을 반환한다. Deinitialize가 SetTickableTickType(Never)를 호출해도 그 getter가 Never가 될 것으로 기대한 테스트는 잘못됐다.
- 따라서 이 실패는 **실제 manager가 계속 Tick된다는 증거가 아니다**. 원본 getter를 바꾸거나 실패를 expected error로 처리하는 방식으로 테스트를 통과시키지 않는다. Tick 등록 계약을 적합한 관찰값으로 다시 검증해야 한다.

**별도 원본 근거 — 이벤트 해제 불일치**

- 원본과 대상의 Initialize는 `FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject`를 사용한다. 반면 Deinitialize는 다른 delegate인 `PreLoadMap.RemoveAll(this)`를 호출한다. 대상 현재 위치는 LoadingScreenManager.cpp:132/145다.
- `MulticastDelegateBase.h::IsBoundToObject`는 native multicast의 객체 결합 여부를 공개 API로 확인할 수 있다. 객체를 검사 동안 유지한 채 초기화/종료 전후를 검증할 수 있다.
- 이 불일치는 **소스로 확인한 결과**이며, 이번17건은 해당 delegate의 잔존을 직접 검사하지 않았다. 원본을 그대로 두는 대안은 shutdown 시 동일 이벤트의 명시적 해제를 보장하지 못한다. 원본에 등록한 이벤트와 해제 대상을 맞추는 최소 수정을 권장한다.

**당시 승인 제안 — 아래 후속 승인으로2CPP 교정 반영**

1. `Tests/DCLyraRuntimeFoundationAutomationTests.cpp`: 잘못된 getter assertion을 교정한다. 독립 tick probe에서 실제 엔진 등록→Tick 호출→Never→미호출 계약을 관찰하고, 실제 manager의 task/subsystem/delegate 수명 검증과 구분한다. getter 자체를 강제로 변경하거나 검사를 expected failure로 약화하지 않는다. probe는 별도의 임시 World에 한정해 사용자 월드/다른 전역 tick을 실행하지 않도록 설계한다.
2. `Plugins/CommonLoadingScreen/Source/CommonLoadingScreen/Private/LoadingScreenManager.cpp`: PreLoadMap.RemoveAll을 **PreLoadMapWithContext.RemoveAll(this)**로 맞추는1줄 수정. 등록/해제 전후 IsBoundToObject 회귀도1번 검사에 포함한다. 정상 로딩 화면/오디오/설정 처리와 임시 진단 조건은 변경하지 않는다.
3. 원본 대비 동작 차이는 **종료된 manager의 해당 callback을 종료 시점에 제거**하는 것이다. 실제 맵 전환・렌더링・입력의 전체 회귀는 여전히 별도이며, 두 검사 의미를 혼동하지 않는다.
4. 승인 후 코드만 수정하고 사용자 재빌드 결과를 받은 뒤 기본/진단 두 모드를 다시 검사한다. 결과가 통과하기 전 새 에셋 이전/활성 경로 전환으로 넘어가지 않는다.

이번 확인에서는 게임 C++/INI/uproject/기존 에셋/인덱스를 변경하지 않았으며, 보호 대상6개 해시는 이전 값과 같았다. AGENTS와 명세에 실패/원인/교정 제안만 기록했다. 현재 단계는 **R5-3/R13 선행 검증 미완료**다.

### 2026-10-06 후속 승인 — Tick/delegate2CPP 교정 완료, 재빌드 대기

**수정 내용**

- 사용자의 `진행하자` 승인으로 `Plugins/CommonLoadingScreen/Source/CommonLoadingScreen/Private/LoadingScreenManager.cpp`의 해제 대상을 **PreLoadMapWithContext.RemoveAll(this)**로 맞췄다. 작업 전 파일과 비교해 production 변경이 이1줄뿐임을 확인했다. 원본의 getter/SetTickableTickType(Never)/표시/오디오/임시 진단 조건은 바꾸지 않았다.
- `Source/DreamCatcher/Tests/DCLyraRuntimeFoundationAutomationTests.cpp`에서 잘못된 manager getter assertion을 제거하고, 초기화 후 PreLoadMapWithContext/PostLoadMapWithWorld에 연결됐는지와 shutdown 후 두 연결이 제거됐는지 공개 IsBoundToObject API로 확인한다. manager는 TStrongObjectPtr로 유지하므로 weak object의 GC만으로 해제 검사가 통과하지 않는다.
- 새 `DreamCatcher.R5.RuntimeFoundation.TickRegistration`은 **비UObject probe**로 엔진의 실제 SetTickableTickType/TickObjects API 계약을 검사한다. 새로운 reflected 생산 타입/설정/헤더 API는 추가하지 않았다.
- probe는 별도 unattended Editor/격리 검사 flag에서만 실행한다. FTestWorldWrapper의 **EWorldType::Inactive** 월드(단일 새 non-null World 포인터, GameInstance 없음)로 Tick dispatch를 한정한다. nullptr 전역-world dispatch나 사용자 월드/actor Tick・BeginPlay는 호출하지 않는다. probe가 월드보다 먼저 소멸하여 등록을 방어적으로 해제한다.

**새 Tick 검사 순서**

1. Never로 생성 → 호출0.
2. Conditional 등록 후 첫 dispatch 전 Never → 대기 등록이 취소되어 호출0.
3. Conditional 등록 → dispatch2회에 실제 호출1/2로 증가(양성 대조).
4. Never 해제 → getter는 여전히 Conditional이지만 dispatch2회 후 호출2 유지.
5. 재등록 → 호출3, 재해제 → 호출3 유지.

이 검사는 **엔진 등록 API 계약**을 관찰한다. 실제 loading manager의 화면 Tick/맵 이동을 관찰했다고 표현하지 않으며, 실제 manager의 task/subsystem/delegate 수명 검사는 별도로 유지한다. 실패를 expected error로 처리하거나 원본 getter를 Never로 바꿔 통과시키지 않았다.

**정적 결과와 재개 지점**

- RuntimeFoundation 검사 수는4→**5건**, 이전13건과 함께 **기본/진단 각18건**을 재실행할 예정이다.
- INI2개/uproject/Build.cs/기존 GameMode/LocalPlayer h/cpp/진단 helper **9개 SHA256 불변**을 작업 전후 확인했다. 기존 사용자 변경・에셋・활성 경로・Git 인덱스는 보존했다. AGENTS/명세 상태 기록 외 코드 변경 대상은 위2CPP뿐이다.
- production1줄 변경 확인, 새 코드 구조/API 대조와 `git diff --check`는 통과했다. **재빌드/검사 실행은 아직 미검증**이며 이전 진단1실패 보고서를 성공으로 바꾸지 않았다. Codex는 빌드/패키징/에디터 실행을 하지 않았다.
- 다음은 사용자 **Development Editor / Win64 빌드** 확인 후 기본 모드와 `-DCLyraRuntimeDiagnostics` 진단 모드를 각각 새 unattended Editor에서 검사한다. 상태 관련 검사의 `-DCR5RuntimeIsolatedAutomation` 등 기존 opt-in도 유지한다.
- 실제 맵 이동/원본 로딩 위젯・음향 출력・전역 SettingsLocal/Experience 완료・라이플/복제는 여전히 미검증이다. 이 선행 검증이 통과하기 전 신규 에셋 이전/활성 전환은 하지 않는다.

### 2026-10-07 00:30 KST Tick/delegate 재검증 통과 / 라이플 native 연결 감사 제안

**최신 검증 근거**

- 사용자 UBT 재빌드: **00:21:04 시작・Result: Succeeded・7.68초**. 교정한 LoadingScreenManager.cpp와 DCLyraRuntimeFoundationAutomationTests.cpp를 컴파일했고 게임 DLL은 **00:21:11 / 3,730,432 bytes**, plugin DLL은 **00:21:10 / 174,592 bytes**로 갱신됐다. Codex는 빌드/패키징을 실행하지 않았다.
- **00:27:16 KST** `Saved/Automation/R5RuntimeFoundationFixedDefault/index.json`: 기본 비활성 모드 **18성공**.
- **00:30:40 KST** `Saved/Automation/R5RuntimeFoundationFixedEnabled/index.json`: 진단 활성 모드 **18성공**. 동일18종을 두 모드에서 **총36회** 실행한 결과이지 서로 다른36종이 아니다.
- 두 보고서 실패/경고/notRun0, 동명 `Saved/Logs/*.log`의 ensure/Fatal0, 프로세스 종료0이다. Source TickRegistration 및 실제 manager PreLoadMapWithContext/PostLoadMapWithWorld 초기화/종료 binding 검사도 포함해 통과했다.
- 이전 `R5RuntimeFoundationDefault`/`Enabled`의17건 및 진단1실패 보고서는 보존했다. 이번 새 결과는 코드 교정 후 새 DLL/별도 프로세스에서 얻은 결과다.
- 임시 진단 조건을 제거하거나 정상 플레이에 켠 것은 아니다. **실제 로딩 위젯/맵 이동/음향/전역 SettingsLocal/완료 Experience/라이플/PIE/복제는 여전히 별도 검증**이다. 이번에는 기존 게임 C++/Config/에셋/인덱스를 수정하지 않고 검사・읽기 전용 조사・상태 문서 갱신만 했다.

**라이플 이식 준비로 복귀하며 확인한 연결 공백**

- 원본 `R5Closure18-inspect.log`, `R5Closure18-lifecycle-reload.log`, `R5Closure18-native-plan.log`에서 관측된 `/Script/LyraGame.*` native 경로 합집합은 **28종**이다. 현재 DefaultEngine.ini의 정확한 Class/Struct/Enum Redirect와 대조하면 **7종 등록・21종 미등록**이다. nested fragment/구조체/CDO의 모든 타입을 수집한 전수 결과는 아니다.
- 준비된 C++ 클래스가 존재해도 원본 module/type 경로를 자동 해석하지는 않는다. Inventory/Equipment/Weapon/QuickBar/Reticle/CharacterParts/GameState 등은 실제 필요 경로와 멤버 서명을 확인한 후 명시적으로 연결해야 한다.
- 현재 `DCRifleScopedCopy.cpp::ReadTypeReferences`는 FunctionOwner/매크로 owner・cast・pin subtype은 읽지만 함수 member 이름/전체 인자・반환 구조를 기록하지 않는다. 이 보고서만으로 기능적 호환이나 정확한 함수 Redirect까지 확정할 수 없다.
- 실제 소스 예: 원본 PlayerState의 GetLyraPlayerController는 대상 GetDCPlayerController로 이식됐다. DCGameInstance는 원본 클래스와 헤더 API가 같지 않고 일부 초기화/공유 설정 경로만 포팅된 상태다. native28종 중 이식 전 경로를 일괄 Redirect하여 부분 구현을 완전 호환으로 간주하지 않는다.
- 남은 ShooterCore의 Struct_UIMessaging과 GE_Damage_RifleAuto, 공용 Game114개 충돌은 기존 조사 상태 그대로다. 구조체 identity나 Source/BaseDamage 보정12를 임의 테스트 타입/GE로 대체하지 않고, 덮어쓰기/전체 Migrate는 아직 승인・실행하지 않았다.

**승인 제안 이력 — 읽기 전용 연결 감사 도구/검사 묶음, 아래 후속 승인으로 코드 작성**

- `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/Public/DCRifleMigrationLibrary.h`: 별도 native contract 조회 API 선언.
- `.../Private/DCRifleNativeContractAudit.cpp`: BP의 참조 함수 이름/소유자와 native class/struct에서 **리플렉션에 노출된 함수・프로퍼티**의 인자・반환・상속/참조 구조를 읽는 구현. 비반영 C++ 메서드/함수 본문까지 수집했다고 주장하지 않으며 값 적용이나 함수 실행은 하지 않는다.
- `.../Private/Tests/DCRifleNativeContractAuditTests.cpp`: synthetic 타입/그래프의 조회・실패 처리/무변경 계약 검사. 실제 원본 에셋 호환을 fixture 성공으로 대신하지 않는다.
- `Scripts/Editor/r5_rifle_native_contract_audit.py`: 검증된 Prepared core18＋남은 ShooterCore2＋`/Game/B_LyraGameInstance`, `/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base`2의 **22개** 및 필요한 native 타입을 원본/대상에서 조회・비교한다. 실제 대상에 이식되지 않은 BP는 있다고 가정하지 않고 native 대응 후보만 조회한다.
- 조회 결과를 신규 Saved 진단 보고서로 남기고 에셋/동반 파일 상태・dirty・ensure를 확인한다. 기존 Save/Compile/Copy API와 분리하며 기존 copier 일반16개/정확한18개 정책・원본 보호를 넓히지 않는다.
- **이번 묶음 제외:** 게임 C++ 기능 교체, Config Redirect 적용, 에셋 저장/Compile/복사/마이그레이션, 기존114개 덮어쓰기, 활성 GameMode/Pawn/무기 전환, 임시 진단 조건 제거. 연결 후보/누락/차이를 먼저 보고하고 그 다음 변경안을 승인받는다.
- 구조적 서명 일치도 전체 기능 동등성의 증명은 아니다. 원본 그대로 연결할 수 없는 부분은 확인 근거・구체적 제약・대안・동작 차이를 제시하고, 확인이 부족하면 미검증으로 둔다.
- 현재 추가 빌드는 필요하지 않으며, 이 도구 코드 묶음 승인 후 작성하고 사용자 빌드를 거쳐 양쪽 읽기 전용 감사를 실행한다. 현재 위치는 **R5-3 라이플 이식 준비**, R13 실행 계층은 독립 검증 통과/활성 미전환이다.

### 2026-10-07 native 연결 감사 도구 코드 작성 / 사용자 빌드 대기

**승인 범위와 구현**

- 위 제안에 대한 사용자 “진행하자” 승인으로 Editor plugin header1・CPP2・Python1을 작성했다. `DCRifleMigrationLibrary.h`의 별도 보고서 USTRUCT2개 및 `InspectAssetNativeContract`/`InspectNativeTypeContract`, `Private/DCRifleNativeContractAudit.cpp`, `Private/Tests/DCRifleNativeContractAuditTests.cpp`, `Scripts/Editor/r5_rifle_native_contract_audit.py`가 대상이다. 게임 코드/Build.cs/Config 변경은 없다.
- 엔진의 이미 로드된 리플렉션과 authored BP 그래프를 조회한다. 함수명/소유자・인자 순서/반환/flags, 프로퍼티의 배열/set/map/참조/struct/enum・delegate 인자 서명, 상속/interface, call/variable/cast 노드와 핀 타입을 기록한다. 값 적용・CDO 생성・함수 실행・Compile・저장 API를 호출하지 않는다. BP의 부모/생성 클래스/상태 및 package dirty 전후를 확인한다.
- 실에셋 API는 기존 unattended commandlet/SourceControl 비활성/SaveOnCompile Never 조건과 `-DCRifleNativeContractAudit`를 요구한다. 허용 에셋은 **Prepared_0247의 정확한18개＋ShooterCore2＋GameInstance/AnimBP2=22개**다. 기존 copier의 일반16/정확한18개 정책과 결과 형식은 변경하지 않았다.
- 별도 synthetic Editor 검사는 unattended/nullrhi/no PIE/SaveOnCompile Never 및 `-DCRifleNativeContractAudit -DCRifleNativeContractTests`를 요구하고, 에셋 API의 입력을 GetTransientPackage 소속 fixture로 제한한다. native API는 이미 등록된 `/Script` class/struct/enum만 받는다.
- API당16384행/BP8192노드/중첩12 및 스크립트 native128타입 한도를 둔다. 누락・미해석・미지원 타입/한도 초과는 불완전 보고서로 남기며 호환으로 처리하지 않는다. 모든 K2 노드 종류/비반영 C++ 메서드/함수 본문/기본값/런타임 동등성까지 검사하는 도구가 아니다.
- Python은 원본에서 승인22개를 로드하고 참조된 LyraGame/ShooterCoreRuntime native를 읽는다. 대상에서는 **BP 에셋을 로드하지 않고** 이미 등록된 대응 후보 native만 조회한다. 모듈의 암묵적 로드나 미확인 클래스명 추측은 하지 않는다.
- 후보 이름 대응과 **이미 등록된** 함수 Redirect만 보고서 비교에 정규화한다. 미등록 PlayerState 함수 이름 변경 후보는 표시만 하며 Config에 적용하지 않는다. `reflected_shape_match`도 전체 기능 동등성을 뜻하지 않고, flag/서명 차이도 즉시 동작 불가로 단정하지 않는다.
- 출력은 새 `Saved/Diagnostics/R5NativeContracts/<Run>/source.json`, `target.json`, `comparison.json`뿐이며 기존 파일 덮어쓰기를 거부한다. 원본을 먼저 실행한 뒤 같은 run으로 대상을 실행한다. 실에셋/원본/형제/알려진 load-dirty package 및 동반 파일 해시, dirty와 ensure를 확인하고, 기존4개 외 dirty는 허용하지 않는다. 런타임 진단 활성 플래그는 이번 조회에서 거부한다.

**작성 시 확인과 미검증**

- bundled Python의 `--self-test` **4건 성공**: 정확한 선택22개/중복 거부, native 경로 정확 치환, 불완전 보고서의 호환 판정 금지, 기존 함수 Redirect 정규화와 인자명 유지. AST/정적 검사와 `git diff --check`도 통과했다.
- Config2/uproject/게임・Editor Build.cs/기존 복사CPP2/진단helper의 **8개 SHA256 불변**을 확인했다. 사용자 staged/dirty 변경・에셋・활성 경로・Git 인덱스는 보존했다.
- 신규 C++ 검사 **3건**은 NativeSchema(함수/인자/enum/delegate), BlueprintReadOnly(노드/멤버/반환/무변경 및 미해석 실패), RejectedInputs다. **UHT/C++ 빌드・이3건 실행・실제 원본/대상 감사는 아직 미검증**이다. 작성 완료를 실제 호환 확인으로 기록하지 않는다. Codex는 빌드/패키징/에디터 실행을 하지 않았다.
- 다음은 사용자 **Development Editor / Win64 빌드** 확인 → 새 DLL의 synthetic3건/기존 MigrationTools 회귀 → 별도 guarded commandlet의 source22/target native 감사다. 그 결과로 정확한 Redirect/호환 변경안을 먼저 제시한다. 원본 그대로 연결할 수 없는 부분은 근거・제약・원본 보존 대안・동작 차이를 제시하고 승인받는다.
- 현재 위치는 **R5-3 라이플 이식 준비**이며 R13 실행 계층 독립 검증 통과/활성 미전환이다. 전체 Migrate・기존114개 덮어쓰기・라이플 활성 전환・PIE/복제는 이번 작업에 포함되지 않는다.

### 2026-10-07 native 감사 C4456 교정 / 사용자 재빌드 대기

- 사용자 첨부 빌드 로그에서 `DCRifleNativeContractAudit.cpp` 102~115행의 C4456을 확인했다. 앞선 if 조건의 지역변수 P가 else-if 범위에도 존재하는 상태에서 동일 이름을 반복 선언한 도구 코드 오류다. 원본 Lyra 기능/에셋 호환 실패가 아니다.
- 같은 CPP의15개 property 분기에 ArrayProperty/SetProperty/MapProperty 등 서로 다른 변수명을 적용했다. 검사 로직/서명/분기 순서/출력 문자열은 동일하다. 경고 비활성화나 Build.cs/컴파일 정책 변경은 하지 않았다.
- 이름을 역치환하여 작업 전후 전체 CPP 내용이 동일한지 정적 확인했고 `git diff --check`를 통과했다. 코드 수정 대상은 이 CPP1개뿐이며 AGENTS/명세에는 실패와 교정 상태를 기록했다. Codex는 빌드/패키징/에디터를 실행하지 않았다.
- **사용자 재빌드 성공・신규 C++3건・실제 원본/대상 감사는 아직 미검증**이다. 다음은 Development Editor/Win64 재빌드 결과 확인 후 기존 자동 검사/읽기 전용 감사 절차 재개다. 단계는 R5-3 라이플 이식 준비로 유지한다.

### 2026-10-07 01:41 KST native 감사 빌드 확인 / 검사7성공・1실패

- 사용자 UBT 재빌드는 **01:39:37 시작・Result: Succeeded・6.53초**이며 교정한 DCRifleNativeContractAudit.cpp를 컴파일했다. Editor 도구 DLL은 **01:39:43 / 508,928 bytes**로 갱신됐다. Codex가 빌드한 것이 아니다.
- 새 DLL로 별도 unattended/nullrhi Editor에서 전체 MigrationTools를 실행했다. `Saved/Automation/R5NativeContractChecks/index.json` **01:41:41 KST** 결과는 **성공7/실패1/notRun0**이다. 기존 ExplicitCopy5건 및 신규 BlueprintReadOnly/NativeSchema2건은 경고/오류0으로 통과했다.
- **RejectedInputs 실패:** 테스트 CPP128행 `NewObject<UObject>(GetTransientPackage(), NAME_None, RF_Transient)`에서 추상 클래스를 생성했다. `Object.h:97`의 UCLASS(Abstract)와 `UObjectGlobals.cpp:3295`의 추상 생성 ensure, 실행 로그 callstack이 일치한다. 이는 Codex가 작성한 테스트 fixture 오류이며 원본 라이플이나 감사 API의 실제 연결 실패는 아니다. 이 검사에 경고2/오류26개의 로그가 기록됐지만 공통 원인은 ensure1건이다. 프로세스 종료0을 검사 성공으로 취급하지 않는다.
- **다음 교정안 / 아직 미적용:** `Private/Tests/DCRifleNativeContractAuditTests.cpp`1개에서 이미 include된 비추상 `UEdGraph`를 transient fixture로 생성하고 유효한 생성 결과를 확인한다. 목적은 “객체 인스턴스는 native 타입으로 받지 않는다”는 기존 거부 검사를 그대로 수행하는 것이다. 엔진 추상 클래스 생성 제한을 완화하거나 ensure를 예상 성공으로 처리하지 않는다. native 감사 구현/복사 정책/게임 동작 차이는 없다. 더 큰 새 테스트 UCLASS 추가 없이 기존 구체 타입을 재사용한다.
- 사용자 승인 후 테스트만 교정→사용자 Development Editor/Win64 재빌드→새 보고서로 전체8건 재검사→원본22개/대상 native 읽기 전용 감사로 진행한다. **이번 실제 source/target 감사는 미실행**이며 전체 native 호환을 완료로 표시하지 않는다.
- 이번 변경은 상태 문서뿐이다. C++/Config/에셋/활성 경로/진단 조건/인덱스는 그대로이고 보호 대상9개 SHA256 불변을 확인했다. 빌드/패키징은 실행하지 않았다. 실패 보고서는 보존한다. 단계는 R5-3 라이플 이식 준비다.

### 2026-10-07 RejectedInputs fixture 교정 완료 / 사용자 재빌드 대기

- 사용자 승인에 따라 `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/Private/Tests/DCRifleNativeContractAuditTests.cpp`1개만 교정했다. `TStrongObjectPtr<UEdGraph>`와 `NewObject<UEdGraph>(GetTransientPackage(), NAME_None, RF_Transient)`로 유효한 구체 객체를 만들고, TestNotNull 실패 시 즉시 false를 반환한다. 필요한 EdGraph 헤더는 이미 포함돼 있어 include/모듈 추가는 없다.
- 기존 null asset/null native type/객체 인스턴스/실제 script package 거부 assertion4개와 실행 조건은 그대로다. 감사 구현・엔진 추상 클래스 정책・ensure 처리・복사 제한・게임 동작은 바꾸지 않았다. 생성 실패를 native 타입 거부 성공으로 오인하지 않도록 했다.
- 승인한 fixture 구문 외 전체 CPP 내용 동일, 보호 대상9개(감사 API/헤더/Build/스크립트/Config2/uproject/인덱스/실패 보고서) SHA256 불변, `git diff --check` 통과를 정적으로 확인했다. 이 확인은 컴파일/실행 검증이 아니다.
- Codex는 빌드/패키징/에디터 실행을 하지 않았다. **사용자 재빌드와 전체8건 재실행은 대기**이며 `R5NativeContractChecks`의7성공/1실패 보고서를 보존한다. 다음은 사용자 Development Editor/Win64 빌드 확인 후 새 보고서로8건을 재검사하고, 통과하면 source22/target native 읽기 전용 감사다.

### 2026-10-07 01:52 KST native 감사 도구8건 통과 / source・target 보고서 확보

**실행 근거와 확인 범위**

- 사용자 UBT 재빌드는 **01:48:11 시작・Result: Succeeded・5.95초**, 도구 DLL은 **01:48:17 / 508,928 bytes**다. `Saved/Automation/R5NativeContractChecksFixed/index.json` **01:50:05 KST** 전체8건 성공, 실패/경고/오류/notRun0 및 동명 로그 ensure/Fatal0을 확인했다. 이전7성공/1실패 보고서는 보존했다.
- 별도 commandlet에서 원본/대상을 순서대로 읽었다. `Saved/Logs/R5NativeContractSource.log` **01:51:47**, `R5NativeContractTarget.log` **01:52:37** complete/종료0/에셋 쓰기0/ensure0이다. source 쪽의 기존 Death cue 중복 경고는 `R5Closure18-lifecycle-reload.log`에서도 확인되며 이번 조회가 무경고였다는 의미는 아니다. 해당 cue를 변경/삭제하지 않았다.
- 보고서 경로는 `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261007_0150/{source,target,comparison}.json`이다. 원본 에셋은22개, 에셋에서 직접 관측된 LyraGame native는34종, 상속/함수/프로퍼티 타입을 재귀 조회한 합계는84종이다. 84종 모두 신규 이식 대상이라는 뜻은 아니다. 대상은 이미 등록된 후보59종만 조회했고 BP를 명시적으로 로드하지 않았다.
- 원시 반영 구조 비교는 **일치33종 / 차이26종 / 후보 미등록25종**이다. 후보 이름과 기존 함수 Redirect만 정규화한 결과이며 값/비반영 C++/실제 게임 동작 동등성의 증명이 아니다.
- 원본 선택 에셋・원본 core・형제・알려진 Niagara의 파일/동반 상태 **192개 불변**을 실행 중 및 프로세스 종료 후 재확인했다. dirty는 기존 허용된 Niagara4개뿐이며 저장/mark-clean은 하지 않았다. 대상 코드/헤더/스크립트/Config/uproject/인덱스 보호8개 SHA256도 불변이다. 이번 수정은 상태 문서뿐이며 C++ 빌드/패키징은 실행하지 않았다.

**불완전한 항목과 원인**

- source22 중19개는 조회 완료, **W_Reticle_Rifle・W_AmmoCounter_Rifle・WeaponAudioFunctions3개는 미검증 메시지를 남겼다.** MakeStruct/BreakStruct/SetFieldsInStruct6개 노드를 일반 변수로 읽어 `VariableReference=None`을 누락 변수로 판단했다. 엔진의 `K2Node_StructOperation.h`에서 UK2Node_StructOperation이 UK2Node_Variable를 상속하고 별도 StructType을 소유함을 확인했다. 실제 원본 그래프의 깨진 참조라고 판정하지 않는다.
- native25종의 후보 미등록은 이식 코드 부재25종이라는 뜻이 아니다. 아래20개 대응 선언은 대상 소스에서 확인했지만 감사 Python 후보 목록에 빠져 있었다. 아직 추가/실행 비교하지 않았다.

| 원본 LyraGame 타입 | 소스로 확인한 DreamCatcher 후보 |
|---|---|
| LyraAbilitySystemComponent | DCAbilitySystemComponent |
| LyraCameraComponent | DCCameraComponent |
| LyraCameraModeStack | DCCameraModeStack |
| LyraEquipmentList | DCLyraEquipmentList |
| LyraAppliedEquipmentEntry | DCLyraAppliedEquipmentEntry |
| LyraExperienceManagerComponent | DCLyraExperienceManagerComponent |
| LyraExperienceDefinition | DCLyraExperienceDefinition |
| LyraExperienceActionSet | DCLyraExperienceActionSet |
| ELyraDeathState | EDCLyraDeathState |
| LyraAttributeSet | DCAttributeSet |
| LyraHeroComponent | DCHeroComponent |
| InputMappingContextAndPriority | InputMappingContextAndPriority |
| ECharacterCustomizationCollisionMode | EDCLyraCharacterCustomizationCollisionMode |
| LyraAppliedCharacterPartEntry | DCLyraAppliedCharacterPartEntry |
| LyraPawnData | DCPawnData |
| LyraInputConfig | DCInputConfig |
| LyraInputAction | DCInputAction |
| LyraPawnExtensionComponent | DCPawnExtensionComponent |
| LyraCameraAssistInterface | DCCameraAssistInterface |
| LyraAbilitySourceInterface | DCAbilitySourceInterface |

- 나머지 **HitMarkerConfirmationWidget・LyraHUD・ELyraPlayerConnectionType・LyraReplicatedAcceleration・SharedRepMovement5종은 이번 대상 소스 검색에서 대응 미확인**이다. 이름을 추정하거나 자체 타입으로 대체하지 않으며, 필요한 실제 사용 경로와 원본 이식 범위를 따로 확인한다.
- 차이26종에는 미등록 참조 타입/함수·delegate 이름뿐 아니라 `CLASS_ReplicationDataIsSetUp(0x800)` 같은 로드 상태 flag, MinimalAPI/RequiredAPI 노출 정책이 포함된다. 원시 flag 차이를 곧바로 기능 불일치로 보거나 무조건 마스킹하지 않는다.
- Character/PlayerController/PlayerState의 부모・interface・멤버 차이는 별도로 존재한다. DCGameInstance는 반영 멤버 차이가 class flag뿐이어도 원본의 비반영 GetPrimaryPlayerController/세션/Shutdown/암호화 override 전체를 포팅한 것이 아니다. 원본/대상 헤더를 대조했으며 일괄 native Redirect로 완전 이식을 선언하지 않는다.

**다음 변경안 / 아직 미적용・승인 필요**

- `Private/DCRifleNativeContractAudit.cpp`: MakeStruct/BreakStruct/SetFieldsInStruct 계열을 일반 변수와 구분하고 실제 StructType/핀/반영 멤버를 읽는다. 모든 UK2Node_Variable 실패를 건너뛰는 방식은 쓰지 않으며 실제 누락 변수/struct는 계속 미검증으로 처리한다.
- `Private/Tests/DCRifleNativeContractAuditTests.cpp`: 위 struct operation과 실제 잘못된 변수/누락 struct의 구분, 노드/dirty 무변경 회귀를 추가한다. 기존8건과 실패 보고서는 유지한다.
- `Scripts/Editor/r5_rifle_native_contract_audit.py`: 소스로 확인한20개만 명시적 조회/비교 후보에 추가하고 순수 계약 검사를 보강한다. raw 차이를 보존하며 기존 함수/delegate/클래스 flag 차이를 임의 성공으로 정규화하지 않는다. 대응 미확인5종은 미검증으로 남긴다.
- 이3파일은 검사 정확도와 조회 후보 보강이며 Lyra 게임 동작의 수정/대체가 아니다. 대안으로 현재 보고서를 불완전한 채 유지할 수 있지만 누락 후보와 노드 분류 때문에 정확한 Redirect 범위를 확정하기 어렵다. 다음 사용자 승인→코드 수정→사용자 빌드→도구 회귀와 새 source/target 보고서 검증 순서다.
- **제외:** Config Redirect 적용, 게임 C++ 이식/교체, 미확인5타입 신설, 기존114개 덮어쓰기, Migrate/에셋 저장/참조 치환, 활성 경로 변경. 현재 R5-3 라이플 준비이며 R13 독립 검사/실제 활성 완료를 구분한다.

### 2026-10-07 struct 노드・후보20종 보강 코드 작성 / 사용자 빌드 대기

- 위3파일 변경안에 대한 사용자 승인으로 `Private/DCRifleNativeContractAudit.cpp`, `Private/Tests/DCRifleNativeContractAuditTests.cpp`, `Scripts/Editor/r5_rifle_native_contract_audit.py`만 수정했다. 헤더 API/모듈 의존성/게임 C++/Config/에셋은 변경하지 않았다.
- 감사CPP는 UK2Node_MakeStruct/UK2Node_BreakStruct 계열(전자를 상속한 SetFieldsInStruct 포함)을 일반 변수보다 먼저 분류한다. StructType의 경로와 반영 멤버 이름/타입/flags/순서를 `node_struct_operation`/`struct_member` 행으로 남기고 기존 pin 조회를 유지한다. StructType이 없으면 cached pin에서 복구하지 않고 미검증 메시지를 남긴다. 다른 StructMember/Variable 노드는 기존 멤버 해석을 유지한다. 값 적용/실행/Compile/저장/수정 API를 추가하지 않았다.
- C++ `NativeContract.StructOperations`1건을 추가했다. transient Actor BP와 기존 보고서 USTRUCT를 사용해 Make/Break/SetFields의 정상 조회, 일반 bHidden 변수 조회, 각 StructType을 비웠을 때의 거부, 실제 존재하지 않는 변수 거부를 검사한다. 호출 전후 graph node/pin 배열・dirty・Blueprint 상태/생성 클래스 유지도 확인하도록 작성했다. 새 UCLASS/USTRUCT나 게임 모듈 의존성을 만들지 않았다.
- Python `REVIEWED_20261007_CANDIDATES`에는 위 표의20종만 추가했다. 이전 목록의 원본에 없는 ELyraCharacterCustomizationCollisionMode 철자는 실제 조회된 ECharacterCustomizationCollisionMode로 교정했다. 대상은 EDCLyraCharacterCustomizationCollisionMode 그대로다. 후보5종 미확인 상태, 기존 함수 Redirect 정규화 범위, 원시 flags/delegate 이름 비교를 유지한다. 이는 조회 후보이며 Config Redirect가 아니다.
- `--self-test`는 기존4건＋후보20개 경계・미확인5종 미추정・raw flags/delegate 차이 보존・구조 일치와 동작 동등성 구분4건으로 **합계8건 통과**했다. AST/정적 검사와 `git diff --check`도 통과했다. 보호11개(Config2/uproject/인덱스/헤더/Build/기존 copierCPP2/이전보고서3) SHA256 불변을 확인했다.
- **C++ 재빌드・신규 검사 실행・실제 source/target 재감사는 아직 미검증**이다. Codex는 빌드/패키징/에디터를 실행하지 않았다. 이전 전체8건 통과와 원시33일치/26차이/25미등록 보고서를 새 보강 코드의 결과로 바꾸지 않는다.
- 다음은 사용자 Development Editor/Win64 빌드 확인 → 기존8＋신규1의 **전체9건**을 새 보고서로 검사 → 새 run 이름으로 source22/target native 감사다. 새 타입이 관측되면 임의 후보를 만들지 않고 미검증 목록에 남긴다. 실제 Redirect/이식/활성 교체는 감사 결과에 근거한 별도 변경안 승인 후 진행한다.

### 2026-10-07 02:36 KST struct 노드・후보 보강 검증 완료 / HitMarker 선행 이식 제안

**최신 검증 결과**

- 사용자 UBT 빌드: **02:32:46 시작・Result: Succeeded・9.40초**, 도구 DLL **02:32:55 / 524,800 bytes**. `Saved/Automation/R5NativeStructChecks/index.json` **02:34:08 KST**에 기존8＋StructOperations1의 **전체9건 성공・실패/경고/오류/notRun0**, 동명 로그 ensure/Fatal0을 확인했다. Codex는 C++ 빌드/패키징을 실행하지 않았다.
- `Saved/Logs/R5NativeStructSource.log` **02:35:16**, `R5NativeStructTarget.log` **02:36:40** complete/종료0/asset writes0/ensure0이다. `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261007_0235/{source,target,comparison}.json`을 새로 만들었고 이전 보고서는 보존했다.
- 원본22개 모두 bounded 조회 성공으로, 이전3개 에셋의 StructOperation 오분류 메시지는 해소됐다. source native84종/target79종, 반영 구조 비교는 **52일치/27차이/5대응 미확인**이다. 에셋의 모든 저장값・C++ 본문・실제 화면/실행/복제 동등성의 증명이 아니다.
- 원본 파일/동반192개는 프로세스 종료 후 재대조해 불변이고, 대상 Config/코드/스크립트/프로젝트/인덱스 보호8개도 SHA256 불변이다. 원본 dirty는 기존 허용 Niagara4개뿐이며 저장하지 않았다. 이전부터 있던 Death cue 중복 경고도 남아 있으므로 전체 조회를 무경고로 표시하지 않는다. 이번 수정은 상태 문서뿐이다.

**다음 직접 의존성과 확인 근거**

- source 보고서는 `W_Reticle_Rifle:WidgetTree.HitMarkerConfirmations`와 생성 클래스의 대응 WidgetTree 객체가 `/Script/LyraGame.HitMarkerConfirmationWidget`임을 기록한다. 대응 미확인5종 중 이 타입은 실제 라이플 UI의 직접 의존성이다.
- 나머지 LyraHUD/ELyraPlayerConnectionType/LyraReplicatedAcceleration/SharedRepMovement4종은 현재22개 에셋 직접참조 목록에는 없고 각각 PlayerController/PlayerState/Character의 반영 멤버 조회에서 확장됐다. 이 사실로 최종 불필요/제외를 결정하지 않는다. 플레이어 원본 교체 및 전체 직렬화 의존성 점검은 별도로 남는다.
- 원본 `Source/LyraGame/UI/Weapons/HitMarkerConfirmationWidget.{h,cpp}`, `SHitMarkerConfirmationWidget.{h,cpp}`4개를 읽었다. UMG wrapper는 UWidget이며 Slate는 일반 APlayerController에서 LyraWeaponStateComponent를 찾고 화면 위치/zone과 마지막 명중 경과시간을 조회한다. 대상 `DCLyraWeaponStateComponent.h`에 FDCLyraScreenSpaceHitLocation/GetLastWeaponDamageScreenLocations/GetTimeSinceLastHitNotification이 이미 있다. 추가 PlayerController 대체 API가 필요한 구조는 아니다.
- 원본 그대로 유지할 기능: 지역별 marker brush 선택, 일반 명중 marker, 색/알파 계산, 화면→로컬 좌표 변환, 경과시간에 따른 opacity 감소, 기본 표시시간0.4초, DesiredSize100×100, UMG HitTestInvisible/volatile 설정과 Slate 자원 해제다. 기본값은 원본 C++에서 확인했으며 실제 W_Reticle 인스턴스의 모든 시각 값 검증을 대신하지 않는다.

**원본 대비 최소 보정 제안 — 아직 적용하지 않음**

- 원본 UMG `HitMarkerConfirmationWidget.cpp:36`은 `.HitNotifyDuration(this->HitNotifyDuration)`을 넘기지만 Slate `SHitMarkerConfirmationWidget.cpp::Construct`는 해당 인자를 읽지 않는다. Slate 멤버는 h의0.4f로 시작하고 Tick은 이 멤버로만 감쇠한다. 따라서 소스상으로 전달 기간이 사용되지 않는다. 이번에 별도 화면 실험으로 재현한 것은 아니다.
- 원본을 문자 그대로 유지하는 대안도 가능하지만, UMG의 노출된 표시시간을 달리 지정해도 실제 Slate 시간에는 반영되지 않는다. 이식을 불가능하다고 판단한 것이 아니라 명시적 최소 보정을 제안한다.
- 제안하는 차이는 **Construct에서 전달된 HitNotifyDuration을 멤버에 저장**하는 것뿐이다. 기본0.4초는 유지되고 생성 시 사용자 지정 기간도 적용된다. 프레임별 동적 attribute 재바인딩/새 감쇠 알고리즘/추가 시각 효과는 넣지 않는다. 이 보정을 포함한 묶음 승인을 받은 뒤에만 수정한다.

**다음 승인 범위 — R5-3에서 R7 직접 의존성 선행, 총7파일 예정**

- 새 원본 기반 게임 코드4개: `Source/DreamCatcher/UI/Weapons/Lyra/DCLyraHitMarkerConfirmationWidget.{h,cpp}`, `SDCLyraHitMarkerConfirmationWidget.{h,cpp}`. 저작권 유지, 접두사/모듈・include/WeaponState 타입 이름과 필요한 명시 include를 바꾸고 위 기간 전달 보정만 추가한다. 기존 Slate/UMG/SlateCore 모듈이 있어 Build.cs 변경은 현재 필요하지 않다.
- 새 검사1개 파일: `Source/DreamCatcher/Tests/DCHitMarkerAutomationTests.cpp`. 기본값・UMG→Slate 기간/brush 전달・context 없는 수명/자원 해제・Redirect 로드를 검사한다. 검사에 필요한 최소 읽기 전용 접근은 Slate 헤더 안에서 제한하고 실제 HUD/화면/네트워크 동등성은 성공으로 간주하지 않는다.
- `Config/DefaultEngine.ini`: **ClassRedirect1개만** `/Script/LyraGame.HitMarkerConfirmationWidget` → `/Script/DreamCatcher.DCLyraHitMarkerConfirmationWidget`로 추가한다. 기존 설정/Redirect를 덮어쓰지 않는다.
- `Scripts/Editor/r5_rifle_native_contract_audit.py`: 위1타입의 확인된 후보 및 미확인 목록 관련 순수 검사를 갱신한다. 나머지4타입을 추정 매핑하지 않는다.
- 제외: 기존 에셋 저장/복사/마이그레이션, Reticle 활성 연결, WidgetTree 수정, 실제 PlayerController에 WeaponState 추가, 전체 native Redirect/부분 Character・PlayerState・GameInstance를 완전 호환으로 간주하는 전환. W_Reticle의 GetLyraPlayerController→GetDCPlayerController 함수명 차이와 부분 플레이어 계층도 후속 정확한 연결 검증 대상으로 유지한다.
- 승인→코드/한정 Config 작업→사용자 Development Editor/Win64 빌드→새 독립 검사와 기존9건 회귀・native 재조회→후속 실제 에셋 연결 순서다. R7 전체/라이플 발사/명중 시각/복제는 미완료이며 빌드/패키징은 사용자 담당이다.

### 2026-10-07 HitMarker 원본 이식 코드 작성 / 사용자 빌드 대기

- 사용자 승인에 따라 위7파일을 작업했다. 신규 게임 파일은 `Source/DreamCatcher/UI/Weapons/Lyra/DCLyraHitMarkerConfirmationWidget.{h,cpp}`, `SDCLyraHitMarkerConfirmationWidget.{h,cpp}`, `Source/DreamCatcher/Tests/DCHitMarkerAutomationTests.cpp`5개이고, 기존 수정 파일은 `Config/DefaultEngine.ini`와 `Scripts/Editor/r5_rifle_native_contract_audit.py`2개다. 상태 문서는 별도로 갱신했다.
- 원본4파일 저작권과 구조를 유지했다. 이름/파일명・generated include・WeaponState/ScreenSpaceHitLocation 타입을 대상 이름으로 연결했고 명시적 SlateBrush/PlayerController/DrawElements include, UMG 클래스 export를 추가했다. Slate 헤더의 friend는 개발 자동 검사에서만 존재하는 읽기 전용 test accessor용이다. 게임 public/debug API나 새 입력/UI 연결은 만들지 않았다.
- 승인된 동작 보정은 Construct의 **`HitNotifyDuration = InArgs._HitNotifyDuration.Get(0.4f);`** 한 줄이다. 원본0.4초 기본값과 unset fallback을 유지하고 생성 시 전달된 사용자 값을 반영한다. 프레임별 attribute 갱신이나 SynchronizeProperties 확장은 하지 않았다. 기본 brush/zone 선택・좌표 변환・원본 opacity 계산・DesiredSize100×100・HitTestInvisible/volatile・자원 해제는 유지했다.
- 원본4개와 허용된 이름/경로/include/export/friend/기간 전달 변환을 대조했고 **OnPaint/ComputeDesiredSize/Tick 본문은 타입명 치환 외 동일**함을 정적으로 확인했다. 이 텍스트 대조는 시각/실행 동등성 검증이 아니다.
- 신규 자동 검사 prefix는 `DreamCatcher.R5.Reticle.HitMarker`다. **SlateArguments**는 기본/unset/지정/0 기간・brush/zone map/색 전달・context 없는 Tick을 검사한다. **WidgetLifecycle**은 transient native UWidget의 기간/brush 전달과 context 없음・release/rebuild를 검사한다. **DependencyRegistration**은 정확한 native ClassRedirect와 soft path fixup/로드를 검사한다. 기존 위젯/월드/PlayerController/태그 등록/에셋을 변경하지 않는 fixture이며 실제 명중・지역별 시각・화면 위치・복제 성공을 대신하지 않는다.
- Config는 HitMarker ClassRedirect1개와 설명 주석만 추가했다. 추가 블록1개를 제외한 정규화 내용의 SHA256이 작업 전과 같음을 확인했다. 클래스/함수 일괄 Redirect나 기존 설정 덮어쓰기는 하지 않았다.
- Python은 HitMarker 후보1개를 추가하고 기존 미확인5종 검사를4종으로 바꿨으며, 클래스가 런타임에 등록되지 않았으면 후보가 있어도 미검증이라는 검사를 추가했다. **순수9건/AST/diff 검사 통과**다. 이식 코드 존재만으로 native 클래스가 로드된다고 가정하지 않는다.
- 원본4파일/기존 WeaponState2파일/Build.cs/uproject/DefaultGame/인덱스/감사CPP/기존보고서의 보호12개 SHA256 불변을 확인했다. Build.cs・기존 게임 구현・에셋・활성 HUD・기존 보고서는 유지했다. C++ 빌드/패키징/에디터 실행은 하지 않았다.
- **C++ 신규3건・native 실제 로드/감사・화면/명중/PIE/복제는 아직 미검증**이다. 다음은 사용자 Development Editor/Win64 빌드 확인 후 **HitMarker3＋도구9＋기존Circumference3** 회귀와 새 native 감사다. 대상 BP를 자동 저장/교체하거나 전체 이식을 완료로 표시하지 않는다.

### 2026-10-07 03:07 KST HitMarker 독립 검증 통과 / 핵심 native 이름 연결 제안

**최신 결과와 한계**

- 사용자 UBT 빌드는 **02:59:56 시작・Result: Succeeded・22.41초**, 게임 DLL은 **03:00:17 / 3,774,464 bytes**다. Codex는 빌드/패키징을 실행하지 않았다.
- `Saved/Automation/R5HitMarkerChecks/index.json` **03:01:48 KST**에 HitMarker3/도구9/Circumference3 **총15건 성공, 실패/경고/오류/notRun0**, 동명 로그 ensure/Fatal0을 확인했다. 기본/unset/지정기간과 context 없는 Tick, brush/zone 전달, UMG 자원 해제/재생성, native Redirect 로드까지의 독립 검증이다. 실제 명중 화면/zone 시각/복제는 아직 아니다.
- `Saved/Logs/R5HitMarkerNativeSource.log` **03:04:22**, `R5HitMarkerNativeTarget.log` **03:07:22** complete/종료0/asset writes0/ensure0이다. 새 `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261007_0302/{source,target,comparison}.json`에서 source22개/타입84종과 target80종을 조회했다. 원시비교는 **52일치/28차이/4후보 미확인**이다.
- HitMarker의 원시차이는 소스 class flags0x30e000a0와 대상0x30f000a0, 즉 명시 export의 CLASS_RequiredAPI(0x100000)1개다. 나머지 반영 멤버는 이름 대응 후 일치하지만 승인된 생성기간 보정과 실제 화면의 동등성까지 반영 비교로 검증했다는 뜻은 아니다.
- LyraHUD/ELyraPlayerConnectionType/LyraReplicatedAcceleration/SharedRepMovement4종과 부분 플레이어/GameInstance 계층은 여전히 별도 점검 대상이다. PlayerState의 실제 GetLyraPlayerController 이름 연결도 이번에 해결하지 않았다.
- 원본/동반192개는 프로세스 종료 후 대조에서도 불변이고 대상 보호12개(Config/코드/스크립트/프로젝트/인덱스)도 SHA256 불변이다. 원본은 기존 허용 Niagara4개 load-dirty와 이전 Death cue 중복 경고를 유지하며 저장하지 않았다. 이번 코드/Config/에셋/활성 경로 변경은 없고 상태 문서만 갱신했다.

**다음 제안 — 아직 미적용・수정2파일**

- 아래30종은 source/target native 보고서에 모두 존재하고 조회가 완료됐으며, 원본 `/Script/LyraGame.*`에 대한 해당 Class/StructRedirect는 아직 없다. 대상 코드를 새로 만드는 것이 아니라 준비된 원본 기반 타입으로 기존 이름을 해석하게 하는 단계다.
- 다수가 원시 구조 일치이며 일부는 CLASS_ReplicationDataIsSetUp 로드상태, 상속한 ASC getter 이름, 대상ASC의 기존 CancelAimInputAndState/ClearAimState 추가 함수에서 차이가 난다. 이를 전체 동등성으로 해석하지 않는다. 타입 등록만으로 BP Compile/직렬화 값 보존/PIE/발사/복제까지 완료되지 않는다.
- 대안은 Blueprint와 데이터 참조를 개별 수정하는 것이지만 에셋 쓰기 범위가 커진다. 이번 제안은 한정 Redirect로 로드 시 이름을 해석하고 원본 게임 알고리즘 및 기존 CPP 본문은 유지한다. 기존 원본 이름 참조의 로드 해석은 바뀌지만 자동 저장/재부모화/활성 교체는 하지 않는다.
- **파일2개:** `Config/DefaultEngine.ini`에 Class21/Struct9/Function2의 **32개 정확한 Redirect**와 설명을 추가하고, `Source/DreamCatcher/Tests/DCRifleNativeRegistrationTests.cpp`를 새로 작성해 정확한 대상・class/struct 종류・soft path fixup 후 로드・ASC getter 이름/반환형과 관련 상속 관계를 검사한다. raw 원본 경로를 먼저 로드하여 누락 LyraGame 모듈 로드를 유발하지 않는다. 기존 동일키는 같은 값인지 확인하고 충돌 값을 덮어쓰지 않는다.
- 다음 표의 원본 접두사는 `/Script/LyraGame.`, 대상 접두사는 `/Script/DreamCatcher.`다. **이 표 밖 타입/패키지 전체 Redirect는 승인 범위가 아니다.**

| 종류 | 원본 타입 | 대상 타입 |
|---|---|---|
| Class | LyraInventoryItemDefinition | DCInventoryItemDefinition |
| Class | LyraInventoryItemInstance | DCInventoryItemInstance |
| Class | LyraInventoryItemFragment | DCInventoryItemFragment |
| Class | InventoryFragment_EquippableItem | DCInventoryFragment_EquippableItem |
| Class | InventoryFragment_PickupIcon | DCInventoryFragment_PickupIcon |
| Class | InventoryFragment_QuickBarIcon | DCInventoryFragment_QuickBarIcon |
| Class | InventoryFragment_ReticleConfig | DCInventoryFragment_ReticleConfig |
| Class | InventoryFragment_SetStats | DCInventoryFragment_SetStats |
| Class | LyraEquipmentDefinition | DCLyraEquipmentDefinition |
| Class | LyraEquipmentInstance | DCLyraEquipmentInstance |
| Class | LyraEquipmentManagerComponent | DCLyraEquipmentManagerComponent |
| Class | LyraQuickBarComponent | DCLyraQuickBarComponent |
| Class | LyraGameplayAbility_FromEquipment | DCLyraGameplayAbility_FromEquipment |
| Class | LyraWeaponInstance | DCLyraWeaponInstance |
| Class | LyraRangedWeaponInstance | DCLyraRangedWeaponInstance |
| Class | LyraGameplayAbility_RangedWeapon | DCLyraGameplayAbility_RangedWeapon |
| Class | LyraAbilityCost_ItemTagStack | DCLyraAbilityCost_ItemTagStack |
| Class | LyraReticleWidgetBase | DCLyraReticleWidgetBase |
| Class | LyraAbilitySet | DCAbilitySet |
| Class | LyraAbilityCost | DCAbilityCost |
| Class | LyraAbilitySystemComponent | DCAbilitySystemComponent |
| Struct | GameplayTagStack | GameplayTagStack |
| Struct | GameplayTagStackContainer | GameplayTagStackContainer |
| Struct | LyraEquipmentActorToSpawn | DCLyraEquipmentActorToSpawn |
| Struct | LyraEquipmentList | DCLyraEquipmentList |
| Struct | LyraAppliedEquipmentEntry | DCLyraAppliedEquipmentEntry |
| Struct | LyraAbilitySet_GameplayAbility | DCAbilitySet_GameplayAbility |
| Struct | LyraAbilitySet_GameplayEffect | DCAbilitySet_GameplayEffect |
| Struct | LyraAbilitySet_AttributeSet | DCAbilitySet_AttributeSet |
| Struct | LyraAbilitySet_GrantedHandles | DCAbilitySet_GrantedHandles |

- 함수 연결은 동일 getter의2개 이전 경로다. 목적지는 둘 다 `/Script/DreamCatcher.DCGameplayAbility.GetDCAbilitySystemComponentFromActorInfo`이며 이전 경로는 `/Script/LyraGame.LyraGameplayAbility.GetLyraAbilitySystemComponentFromActorInfo` 및 클래스명 변환 후 `/Script/DreamCatcher.DCGameplayAbility.GetLyraAbilitySystemComponentFromActorInfo`다.
- 원본/대상 GameplayAbility.cpp의 getter를 직접 확인했다. 양쪽 모두 CurrentActorInfo가 있으면 ASC를 프로젝트 타입으로 Cast하고 없으면 nullptr를 반환한다. 함수/반환 flags와 타입 대응도 같고 이름만 다르다. 새 wrapper나 함수 본문 변경은 하지 않는다.
- **제외:** PlayerState/GameInstance/GameState/AnimInstance/CharacterParts/Health/Team 등 나머지 이름 연결, 미확인4종 임의 매핑, 현재 Character/Controller의 기존 Redirect 변경, getter 전체 일괄 alias, 새 PlayerController/Pawn 부착, HUD/무기 활성 교체, 에셋 복사/저장/Migrate, 기존114개 덮어쓰기. 남은 이식 대상은 보류/미검증으로 남기며 기능 제외로 판정하지 않는다.
- 사용자 승인→위2파일 작업→사용자 Development Editor/Win64 빌드→신규 등록 검사와 기존15건/핵심 Equipment·Weapon 회귀→새 native 감사 후 후속 에셋 연결 범위를 정한다. 준비본과 실제 DreamCatcher 라이플 이식/PIE 완료를 구분한다.

### 2026-10-07 핵심 native 이름 연결 코드 작성 / 사용자 빌드 대기

- 사용자 승인으로 `Config/DefaultEngine.ini`와 신규 `Source/DreamCatcher/Tests/DCRifleNativeRegistrationTests.cpp`2파일만 작업했다. 명세 표의 **Class21/Struct9/Function별칭2=32개**와 설명 블록을 추가했다. 기존 동일키의 충돌/덮어쓰기・패키지 전체 Redirect는 없다.
- 검사 prefix는 `DreamCatcher.R5.NativeRegistration`이며 **Classes/Structs/ASCGetterAliases/TypeLinks4건**을 작성했다. 타입30개는 이미 메모리에 등록된 대상과 정확한 CoreRedirect 결과를 확인한 후, soft path를 먼저 fixup하고 그 대상 경로일 때만 native 로드를 검사한다. 원본 LyraGame 모듈을 raw load하거나 에셋/객체 인스턴스/CDO를 생성하지 않는다.
- ASCGetterAliases는2개 이전 경로의 동일 함수 해석, const/BlueprintCallable・반환타입DCAbilitySystemComponent・명시입력0/반환1을 검사한다. 로컬 FMemberReference만 사용해 기본 GameplayAbility와 FromEquipment/RangedWeapon 상속 범위에서 이전 이름의 해석을 확인하며 게임 함수 자체는 실행하지 않는다.
- TypeLinks는5개 inventory fragment와 equipment/weapon/ability/cost/ASC/reticle의 관련 상속 관계, EquipmentDefinition.InstanceType/AbilitySetsToGrant/ActorsToSpawn 및 EquippableItem.EquipmentDefinition의 반영 타입 연결을 확인한다. Blueprint Compile/실제 장비 부여/발사 검증과 구분한다.
- 정적으로 승인32개 각각1회 등록, 충돌0, CPP에 같은32개 source/target쌍 포함을 확인했다. 추가한 설명/Redirect 블록을 제외한 Config 정규화 SHA256은 작업 전과 같다. DefaultGame/uproject/인덱스/Build.cs/기존 GameplayAbility h/cpp/ASC cpp/감사Python/기존보고서의 보호9개 SHA256도 불변이다.
- `git diff --check`와 기존 감사 Python `--self-test` **9건 통과**다. 이 검사는 C++ 컴파일/실행이나 Unreal에서 새 Redirect가 적용됐다는 검증이 아니다. **신규C++4건・사용자 빌드・native 재감사는 미실행**이며 Codex는 빌드/패키징/에디터를 실행하지 않았다.
- 다음은 사용자 Development Editor/Win64 빌드 확인 후 **신규등록4＋기존15＋Equipment1/Weapon2=총22건** 회귀와 새 native 감사다. 기존 도구/장비/무기 검사의 command-line opt-in을 유지한다. 현재 source/target 보고서는 이전 결과로 보존하고 새 코드의 성공으로 바꾸지 않는다.
- 기존 Character/Controller 연결, PlayerState/GameInstance/GameState 등 부분 계층, 미확인4종, 함수본문・에셋・활성 Pawn/HUD는 그대로다. 전체 Migrate/기존114개 덮어쓰기/PIE/발사/복제 완료는 아니다.

### 2026-10-07 23:31 KST 핵심 등록32개 검증 통과 / 시각・상태 연결 제안

**최신 실행 근거**

- 사용자 UBT 빌드는 **03:41:27 KST 시작・Result: Succeeded・9.05초**, 게임 DLL은 **03:41:36 / 3,801,600 bytes**다. Codex는 빌드/패키징을 실행하지 않았다. 이번 자동 검사는 같은 날 밤에 실행했으며 오전 빌드 시각과 섞지 않는다.
- `Saved/Automation/R5NativeRegistrationChecks/index.json`의 UTC14:12:59, 즉 **23:12:59 KST** 결과는 **총22건 성공・실패/경고/오류/notRun0**이다. 신규등록4＋도구9＋Reticle6＋Equipment1＋Weapon2를 포함하며 로그 ensure/Fatal0이다. Class21/Struct9 및2개 ASC getter alias의 정확한 대상/로드/상속된 FMemberReference 해석/반환형・관련 속성 타입이 통과했다.
- `Saved/Logs/R5RegisteredNativeSource.log` **23:25:40 KST**, `R5RegisteredNativeTarget.log` **23:31:18 KST** complete/종료0/쓰기0/ensure0이다. 보고서는 `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261007_2314/`이며2314는 run 식별자이지 완료 시각이 아니다. source22개/native84종과 target80종을 조회했다.
- 원시비교는 **52일치/28차이/4후보 미확인**으로 남는다. Class flag/기존 추가 API/부분 플레이어 계층 등의 차이가 있어, 이 숫자가 그대로라는 사실은 이번32개 등록 검사 실패를 뜻하지 않는다. 반대로 이름 연결 통과를 전체 BP/게임 동작 호환으로 간주하지 않는다.
- 원본/동반192개는 프로세스 종료 후에도 불변이고 대상 보호9개(Config/프로젝트/인덱스/기존 코드/감사스크립트)도 SHA256 불변이다. 원본은 알려진 Niagara4개 load-dirty 및 이전 Death cue 중복 경고를 유지하며 저장하지 않았다. 이번 수정은 상태 문서뿐이다.

**다음 변경안 / 아직 미적용 — 총2파일・Redirect20개**

- 이미 이식된 Health/CharacterParts/Team 및 메시지・animation selection 타입을 원본 이름에 연결한다. 아래 Class4/Struct10/Enum2는 source/target 조회가 완료됐고 정확한 기존 Redirect는 없다. 원본 에셋 직접 참조와 이 타입들의 반영 멤버 의존성을 묶는 범위이며 새 게임 기능 구현/활성 전환이 아니다.
- 관련 delegate4개는 원본/대상 헤더 DECLARE_DYNAMIC_MULTICAST와 실제 반영 signature 문자열을 대조했다. 프로젝트 타입 이름 대응 후 인자명/순서/타입/flags가 일치한다. 객체 메서드 alias와 달리 **package-level delegate signature UFunction**이므로 이4개의 native 객체 경로 해석과 멤버 signature 연결을 별도로 검증해야 하며, 아직 이 Redirect를 실행 검증한 것은 아니다.
- 파일은 `Config/DefaultEngine.ini`와 신규 `Source/DreamCatcher/Tests/DCRiflePresentationRegistrationTests.cpp`다. 표의20개만 정확히 추가하고 기존 동일키 충돌은 덮어쓰지 않는다. 검사는 fixup/정확한 타입 및 enum 이름・값, 클래스의 delegate property가 기대 signature를 가리키는지, signature의 인자/flags를 확인하며 이벤트 자체를 broadcast하지 않는다.
- 대안은 BP/데이터의 참조를 개별 변경하는 것이지만 에셋 쓰기 범위가 커진다. 한정 Redirect를 통해 로드 시 이름 해석만 바꾸고 함수 본문・값・그리기・게임 규칙은 유지한다. delegate 경로 해석이 실패하면 제약을 기록하고 추가 변경 전 원본 보존 대안을 다시 설명한다. 패키지 전체 Redirect나 경고 무시는 사용하지 않는다.
- 원본 접두사는 `/Script/LyraGame.`, 대상은 `/Script/DreamCatcher.`다. 표의 Function4개는 gameplay 함수 실행이 아니라 delegate 서명 이름 연결이다.

| 종류 | 원본 타입/서명 | 대상 타입/서명 |
|---|---|---|
| Class | LyraHealthComponent | DCLyraHealthComponent |
| Class | LyraPawnComponent_CharacterParts | DCLyraPawnComponent_CharacterParts |
| Class | AsyncAction_ObserveTeamColors | DCLyraAsyncAction_ObserveTeamColors |
| Class | LyraTeamAgentInterface | DCTeamAgentInterface |
| Struct | LyraAbilityMontageFailureMessage | DCAbilityMontageFailureMessage |
| Struct | LyraVerbMessage | DCVerbMessage |
| Struct | LyraCharacterPart | DCLyraCharacterPart |
| Struct | LyraCharacterPartHandle | DCLyraCharacterPartHandle |
| Struct | LyraCharacterPartList | DCLyraCharacterPartList |
| Struct | LyraAppliedCharacterPartEntry | DCLyraAppliedCharacterPartEntry |
| Struct | LyraAnimLayerSelectionEntry | DCLyraAnimLayerSelectionEntry |
| Struct | LyraAnimLayerSelectionSet | DCLyraAnimLayerSelectionSet |
| Struct | LyraAnimBodyStyleSelectionEntry | DCLyraAnimBodyStyleSelectionEntry |
| Struct | LyraAnimBodyStyleSelectionSet | DCLyraAnimBodyStyleSelectionSet |
| Enum | ELyraDeathState | EDCLyraDeathState |
| Enum | ECharacterCustomizationCollisionMode | EDCLyraCharacterCustomizationCollisionMode |
| Function | LyraHealth_DeathEvent__DelegateSignature | DCLyraHealth_DeathEvent__DelegateSignature |
| Function | LyraHealth_AttributeChanged__DelegateSignature | DCLyraHealth_AttributeChanged__DelegateSignature |
| Function | LyraSpawnedCharacterPartsChanged__DelegateSignature | DCLyraSpawnedCharacterPartsChanged__DelegateSignature |
| Function | TeamColorObservedAsyncDelegate__DelegateSignature | DCLyraTeamColorObservedAsyncDelegate__DelegateSignature |

- **제외:** PlayerState/GameState/GameInstance/AnimInstance의 나머지 연결, 현재 Character/Controller Redirect 변경, OnLyraTeamIndexChangedDelegate signature, 미확인 LyraHUD/ELyraPlayerConnectionType/LyraReplicatedAcceleration/SharedRepMovement4종 추정 매핑, 전체 Migrate/기존114개 덮어쓰기, Pawn/HUD 활성 교체. 미검증 항목을 불필요로 확정하거나 원본 기능을 삭제하지 않는다.
- 다음은 사용자 승인→2파일 코드/Config→사용자 Development Editor/Win64 빌드→신규 검사와 기존22건/TeamColor 회귀→새 native 감사다. 현재 등록/독립 검증과 실제 라이플 에셋 이전・플레이어 연결・발사/재장전・PIE/복제를 구분한다.
- 일정상10/7 밤에도 R5-3 실사용 라이플 연결이 미완료이므로 **10/8 장비・발사・재장전 체크포인트 지연 위험**이 있다. 완료 약속이나 R9~R12/병합 범위 축소로 보상하지 않으며 이후 실제 연결 결과에 따라10/16 목표 영향을 다시 판단한다.

### 2026-10-07 시각・상태 이름 연결 코드 작성 / 사용자 빌드 대기

- 사용자 승인으로 `Config/DefaultEngine.ini` 및 신규 `Source/DreamCatcher/Tests/DCRiflePresentationRegistrationTests.cpp`2파일을 작업했다. 위 표 그대로 **Class4/Struct10/Enum2/Function4=20개**를 추가했으며 기존 동일키 충돌/덮어쓰기・패키지 전체 Redirect는 없다. Function4개는 package-level delegate signature이며 게임 함수 호출이 아니다.
- 엔진 `FSoftObjectPath::FixupCoreRedirects`가 `/Script` 경로에서는 Type_AllMask로 조회함을 확인했다. 검사에서는 이미 등록된 대상과 해당 CoreRedirect 종류를 먼저 확인하고, 정확한 대상 경로로 fixup됐을 때만 native TryLoad를 허용한다. 원본 경로를 먼저 로드하지 않는다.
- 신규 prefix는 `DreamCatcher.R5.PresentationRegistration`이다. **NativeTypes**는 타입16개의 등록/종류/fixup/로드, **EnumValues**는 원본/대상에서 확인한 enum2개의 이름・숫자값・생성 sentinel을 검사한다. 이는 BP의 모든 직렬화된 enum 기본값까지 검증했다는 의미는 아니다.
- **DelegateSignatures**는 package-level 함수4개의 등록・로드・multicast/Public flags・void 반환, 인자 순서/이름/bool・int32・float・object 타입 및 const/ref/out 한정자를 확인한다. **DelegateProperties**는 Health의 OnDeathStarted/Finished/HealthChanged/MaxHealthChanged와 Parts/Team 변경 delegate 등6개 property가 정확한4개 signature 객체를 가리키는지 검사한다. 함수/이벤트를 invoke/broadcast하거나 객체/CDO/월드/에셋을 생성하지 않는다.
- 원본/대상 헤더 및 native 보고서를 근거로 death state는 NotDead0/DeathStarted1/DeathFinished2, part collision은 NoCollision0/UseCollisionFromCharacterPart1을 검사하고 자동 생성 MAX 이름은 대상 enum 이름을 따른다. 이는 새 게임 수치/규칙을 만든 것이 아니다.
- 승인20개 각각1회/충돌0, CPP의동일20쌍 포함, 추가한 블록 외 Config 정규화 SHA256 동일을 정적으로 확인했다. DefaultGame/uproject/인덱스/Build・Health/Parts/Team 코드・기존 등록검사・감사스크립트・기존보고서의 보호13개 SHA256도 불변이다. 기존32개 등록과 다른 사용자 변경은 보존했다.
- `git diff --check` 및 기존 감사 Python `--self-test` **9건 통과**다. **사용자 C++ 빌드/신규4건 실행/global delegate Redirect의 실제 사용/native 재감사는 아직 미검증**이며 Codex는 빌드/패키징/에디터를 실행하지 않았다. 이전 성공 보고서를 새 코드의 성공으로 바꾸지 않는다.
- 다음은 사용자 Development Editor/Win64 빌드 확인→신규4＋기존22＋TeamColor1의 **총27건 회귀**→새 native 감사다. delegate 경로 사용이 실패하면 오류를 숨기지 않고 원인과 대안을 설명한다. 테스트만으로 Blueprint 이벤트 바인딩/게임플레이/시각/복제가 완료됐다고 보고하지 않는다.
- 함수본문/PlayerState・GameState・GameInstance 등 나머지 계층・팀 인덱스delegate・미확인4종・에셋・활성 Pawn/HUD는 그대로다. R5-3/R7 선행 준비와 실제라이플/10월8일 체크포인트/10월16일 전체마감은 구분한다.

### 2026-10-08 00:55 KST 시각・상태 연결 검증 통과 / 애니메이션 기반 이름 연결 제안

**최신 결과**

- 사용자 UBT 빌드는 **00:47:31 KST 시작・Result: Succeeded・27.31초**, 게임 DLL은 **00:47:57 / 3,826,688 bytes**다. Codex는 빌드/패키징을 실행하지 않았다.
- `Saved/Automation/R5PresentationRegistrationChecks/index.json` **00:49:47 KST** 총27건 성공, 실패/경고/오류/notRun0 및 동명 로그 ensure/Fatal0이다. 신규4＋기존22＋TeamColor1을 포함하며 타입16개/enum 값/전역 delegate 서명4개와6개 property 연결이 통과했다.
- `Saved/Logs/R5PresentationNativeSource.log` **00:52:13**, `R5PresentationNativeTarget.log` **00:55:12** complete/종료0/쓰기0/ensure0이다. `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261008_0050/`에서 source22개/native84와 target80을 조회했고 원시52일치/28차이/4후보미확인이다.
- 감사 Python은 property type 문자열 안의 global delegate signature 이름을 FunctionRedirect로 정규화하지 않는다. 따라서 이번에 실제 native 로드와 property 연결을 통과한 signature 이름 차이도 원시비교에는 남을 수 있다. 성공 숫자를 만들기 위해 보고서를 덮어쓰거나 차이를 숨기지 않았으며 전체 BP/게임 동작 동등성을 주장하지 않는다.
- 원본/동반192개와 대상보호10개(Config/프로젝트/인덱스/코드/스크립트)는 재대조에서도 불변이다. 기존 Niagara4개 load-dirty와 원본 Death cue 중복 경고는 유지되며 에셋을 저장하지 않았다. 이번 수정은 상태 문서뿐이다.

**남은 기반 계층의 차이**

- 원본/대상 AnimInstance.cpp 및 CharacterMovementComponent.cpp를 직접 대조했다. 프로젝트 타입・include 경로・namespace/CVar/stat 이름 치환 후 CPP 내용이 일치한다. 원본의 GameplayTagPropertyMap 초기화와 GroundDistance 갱신, 이동정지 태그/ground 정보 캐시・trace 알고리즘을 이미 재사용하고 있다.
- 대상 AnimInstance의 NativeUpdateAnimation은 ADreamCatcherCharacter 및 UDCCharacterMovementComponent를 요구한다. 현재 Character 생성자에서 해당 이동컴포넌트를 기본 subobject로 선택함을 확인했다. 이 정적 사실은 실제 특정 BP Pawn의 override/mesh/AnimBP/ground trace 성공을 뜻하지 않는다.
- 원본 GameInstance.cpp에는 CommonSession의 여행 delegate 등록/Shutdown 해제・CanJoinRequestedSession・암호화 callback/debug 경로가 있지만 DCGameInstance에는 InitState/UI manager 확인과 shared settings 초기화만 있다. 원본 PlayerController는 ACommonPlayerController, 대상은 APlayerController 기반이다. 따라서 GameInstance/PlayerState 등의 모든 동작을 일괄 Redirect로 같다고 취급하지 않는다. 재사용 불가가 아니라 추가 의존성/교체 범위 확인이 필요하다. 암호화 debug 예제는 실제 보안 구성으로 채택하거나 활성화한 것이 아니다.

**당시 승인안 — 아래 후속 승인으로 Config＋검사CPP2파일 반영**

- `Config/DefaultEngine.ini`에 다음3개만 추가한다. `/Script/DreamCatcher.LyraCharacterMovementComponent`에 대한 기존 alias는 유지하며 아래 원본 module 경로와 혼동하지 않는다. 기존 동일키 충돌은 덮어쓰지 않는다.

| 종류 | 원본 | 대상 |
|---|---|---|
| Class | /Script/LyraGame.LyraAnimInstance | /Script/DreamCatcher.DCAnimInstance |
| Class | /Script/LyraGame.LyraCharacterMovementComponent | /Script/DreamCatcher.DCCharacterMovementComponent |
| Struct | /Script/LyraGame.LyraCharacterGroundInfo | /Script/DreamCatcher.DCCharacterGroundInfo |

- 신규 `Source/DreamCatcher/Tests/DCRifleAnimationRegistrationTests.cpp`에서 exact Redirect/soft path fixup 후 native 로드, 클래스 상속/반영 GroundDistance와 GameplayTagPropertyMap 타입, ground-info 기본값 및 owner 없는 임시 이동컴포넌트의 안전한 cache 조회를 검사한다. 게임 월드/실제 Pawn/메시/AnimBP를 만들거나 trace・애니메이션 재생을 성공했다고 보고하지 않는다. 기존 Actor/AnimInstance 함수본문은 바꾸지 않는다.
- 이는 원본 animation 기반을 새로 재구현하는 것이 아니라 준비된 타입으로 native 이름을 해석하는 작업이다. 대안인 개별 BP 참조 수정은 에셋 쓰기가 필요하므로 이번 범위에서 하지 않는다. 기본 ground trace 거리100000과 animation 기본GroundDistance -1 등은 기존 코드 값이지 이번에 조정한 게임 수치가 아니다.
- 기존 이동 trace CVar는 이미 DCCharacter.GroundTraceDistance 이름을 사용하며 원본 LyraCharacter.GroundTraceDistance와 이름이 다르다. 이번에는 CVar를 추가/개명/변경하지 않는다.
- **제외:** 실제 AnimBP 재부모화/Compile/저장, 애니메이션/mesh 연결, Character/PlayerController/PlayerState/GameState/GameInstance 교체, UI/무기 활성화, 전체 Migrate/기존114개 덮어쓰기, 미확인4종 추정 매핑. 원본 GameState/Experience 및 부분 플레이어 계층은 후속 범위로 남는다.
- 사용자 승인→2파일 작업→사용자 Development Editor/Win64 빌드→신규 검사와 기존27건 회귀→다음 실제 에셋 연결 범위 확정 순서다. R5-3에서 R8의 직접 의존성을 선행하는 준비이며 R8 전체/실사용 라이플/PIE/복제 완료가 아니다.10/8 체크포인트 지연 위험은 아직 해소되지 않았다.

### 2026-10-08 애니메이션 기반 이름 연결 작성 / 사용자 빌드 대기

- 사용자 승인 후 `Config/DefaultEngine.ini`에 위 표의 Class2/Struct1 **3개만** 추가했다. 기존 `/Script/DreamCatcher.LyraCharacterMovementComponent` alias는 그대로다. 이번 변경은 이름 해석 연결이며 기존 AnimInstance/Movement 구현 본문・CVar 이름/값・실사용 캐릭터/메시/AnimBP를 바꾸지 않는다.
- 새 `Source/DreamCatcher/Tests/DCRifleAnimationRegistrationTests.cpp`에 `DreamCatcher.R5.AnimationRegistration` **3건**을 작성했다.
  - `NativeTypes`: 정확한 종류별 Redirect와 목적지, 이미 등록된 native 타입 확인 후 soft path를 fixup하여 로드한다. 미해석 원본 `/Script/LyraGame` 경로를 먼저 로드하지 않는다.
  - `ReflectedContract`: 엔진 AnimInstance/CharacterMovement 상속, GroundDistance float/읽기 전용 flags, GAS GameplayTagPropertyMap 타입/편집 flags, FHitResult 타입을 확인한다. native animation CDO의 GroundDistance -1만 읽으며 ASC 연결・animation 초기화/갱신은 호출하지 않는다.
  - `OwnerlessGroundCache`: ground-info 기본 frame0/거리0, owner/world 없는 미등록 transient 이동컴포넌트의 반복 조회가 같은 cache/default를 유지하는지 확인한다. 월드 생성・Pawn spawn・component 등록・trace・전역 frame/CVar 수정은 없다.
- 정적 검사는 INI/CPP3쌍 일치・동일 source key 각1회, 기존 alias 보존, 추가블록 제거 후 Config 정규화 SHA256 일치를 확인했다. DefaultGame.ini/uproject/Git 인덱스/Build.cs/animation・movement・Character h/cpp/native 감사 스크립트 **11개 보호 파일**의 해시는 작업 전후 같다. 기존 감사 Python self-test **9건 통과**는 비교기 회귀일 뿐 새 C++ 검사 실행이 아니다.
- **빌드/패키징/에디터 실행/에셋 저장 없음. 새 C++3건의 UHT/컴파일/런타임 검사는 미검증**이다. 앞선27건 성공 및 `NativeAudit_20261008_0050` 원시 보고서는 덮어쓰지 않고 보존한다.
- 다음은 사용자 **Development Editor / Win64 빌드** 확인 후 새3건＋기존27건을 새 보고서로 실행하고 읽기 전용 native 재감사를 진행하는 것이다. 실제 AnimBP 재부모화/Compile/저장・trace/애니메이션 재생・라이플/PIE/복제는 이 테스트의 검증 범위가 아니며 별도 승인/검증이 필요하다. 현재 **R5-3/R8 직접 의존성 선행 준비**이며10/8 체크포인트 지연 위험은 유지한다.

### 2026-10-08 01:29 KST 애니메이션 기반 검증 통과 / HUD・AddWidgets 준비 제안

**확인 완료**

- 사용자 빌드: UBT **01:24:40 KST 시작 / Result: Succeeded / 21.40초**, `DCRifleAnimationRegistrationTests.cpp` 컴파일 포함. 게임 DLL **01:25:01 / 3,843,072 bytes**. Codex는 빌드/패키징을 실행하지 않았다.
- `Saved/Automation/R5AnimationRegistrationChecks/index.json` **01:27:41 KST** 총30건 성공(신규3＋이전27), 실패/경고/오류/notRun0. 동명 로그의 Error/ensure/Fatal0, 프로세스 종료0이다. 신규3은 NativeTypes/ReflectedContract/OwnerlessGroundCache이며 실제 월드의 ground trace/AnimBP 재생 성공과 구분한다.
- `Saved/Logs/R5AnimationNativeSource.log` **01:28:38**, `R5AnimationNativeTarget.log` **01:29:53** complete/종료0/에셋쓰기0/ensure0. `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261008_0128/`는 source22개/native84, target80, 원시52일치/28차이/4후보미확인이다. 조회된22개 에셋/대상80개 타입의 개별 bounded 보고서는 모두 완료됐다. 나머지4종은 대응 미확인이지 전체 이식 불가가 아니다.
- 원본/동반192개 파일 상태를 종료 후에도 재대조했고 대상 Config/프로젝트/인덱스/코드/스크립트 보호12개 SHA256이 그대로다. 기존 Niagara4개 단순로드 dirty와 원본 Death cue 중복 경고는 여전히 남는다. 성공한 회귀 보고서의 경고0을 원본 프로젝트 전체 경고0으로 확대하지 않는다.
- 애니메이션 클래스의 원시class flags 차이와 property 문자열의 global delegate signature 이름 차이는 보존한다. 등록검사 성공이 모든 코드 동작 동등성이나 실제 에셋 연결 완료를 뜻하지 않는다. 이번 수정은 상태 문서뿐이다.

**다음 묶음의 원본 근거・제약・대안 — 아직 미적용**

1. 원본 `Source/LyraGame/UI/LyraHUD.h/.cpp`는 `AHUD` 기반이다. 생성자의 시작 Tick 비활성, PreInitializeComponents의 component receiver 등록, BeginPlay의 GameActorReady 송신, EndPlay의 receiver 해제, ASC actor debug 목록 수집을 갖는다. 대상의 `UDCPlayerHUDWidget`은 화면 위젯이지 이 Actor 수명의 대체물이 아니다. 원본 PlayerController의 `GetLyraHUD`는 이 구체 HUD 타입으로 CastChecked한다. 최신22개 에셋의 직접 참조가 아니라 PlayerController native 반환형을 따라 확장된 의존성임을 구분한다.
2. 원본 `GameFeatures/GameFeatureAction_AddWidget.h/.cpp`의 실제 클래스명은 `UGameFeatureAction_AddWidgets`다. HUD 준비/확장/제거 이벤트로 CommonUI Layout push/deactivate와 UIExtension context 등록/해제를 한다. HUD만 따로 준비한 뒤 다시 빌드하는 왕복을 줄이기 위해 R7/R13의 이 직접 소비자를 함께 준비한다.
3. 공통 `GameFeatureAction_WorldActionBase.h`는 대상과 원본 해시가 같고 CPP는 명시적 `Engine/Engine.h` include 외 동일하다. 이를 다시 만들거나 수정하지 않고 재사용한다. UIExtension 플러그인과 CommonGame/CommonUI/GameFeatures/ModularGameplay는 이미 있으나 DreamCatcher.Build.cs의 UIExtension 직접의존성은 없다. public 헤더의 `FUIExtensionHandle` 사용을 위해 해당 의존성1개만 추가할 계획이다.
4. **실제 화면의 제약:** AddWidgets는 HUD의 로컬 PlayerController/LocalPlayer, UI layer/extension 및 사전 로드된 soft widget class를 전제로 한다(`Entry.LayoutClass.Get()`, `Entry.WidgetClass.Get()`). 이 native 묶음만으로 HUDLayout/Widget/Experience 에셋이나 화면 표시가 완성되지 않는다. 원본 방식 보존 대안은 후속 Experience/Client bundle/레이어/원본 에셋 연결 검증이다. 즉석 LoadSynchronous, 임시 대체 UI, 기존 자체 HUD로의 강제 연결은 이번에 넣지 않는다. 원본 이식 불가가 아니라 해당 활성 연결이 미검증이다.

**당시 승인 요청 범위 — 아래 후속 승인으로 구현/설정7파일 반영**

- 신규 `Source/DreamCatcher/UI/Lyra/DCLyraHUD.h/.cpp`: 원본 HUD2파일, `ADCLyraHUD`로 이름/include/module만 대응.
- 신규 `Source/DreamCatcher/GameFeatures/DCGameFeatureAction_AddWidget.h/.cpp`: 원본 Action2파일, `UDCGameFeatureAction_AddWidgets`, `FDCLyraHUDLayoutRequest`, `FDCLyraHUDElementEntry`로 대응. 기존 WorldActionBase 사용, 원본 수명/검증/Client bundle 동작 유지.
- 신규 `Source/DreamCatcher/Tests/DCHUDFoundationAutomationTests.cpp`: 등록/상속/default・transient action의 데이터 검증/Client bundle・격리 월드 HUD receiver/ready/remove・빈 Action 활성/해제 검사4건을 준비한다. private 데이터 검사는 transient 객체의 반영 속성으로 제한하며 production 접근자를 추가하지 않는다. 수명 검사는 별도 unattended Editor와 `-DCR5HUDIsolatedAutomation`을 요구하고 소유한 fixture/handler/delegate만 정리한다. 실제 LocalPlayer/위젯/화면/PIE/복제는 검사 성공으로 주장하지 않는다.
- `Source/DreamCatcher/DreamCatcher.Build.cs`: PublicDependencyModuleNames에 `UIExtension`1개 추가. .uproject/플러그인 원본은 미수정.
- `Config/DefaultEngine.ini`: 다음5개만 추가하며 기존키 충돌 시 중단한다.

| 종류 | 원본 /Script/LyraGame. | 대상 /Script/DreamCatcher. |
|---|---|---|
| Class | LyraHUD | DCLyraHUD |
| Class | GameFeatureAction_WorldActionBase | GameFeatureAction_WorldActionBase |
| Class | GameFeatureAction_AddWidgets | DCGameFeatureAction_AddWidgets |
| Struct | LyraHUDLayoutRequest | DCLyraHUDLayoutRequest |
| Struct | LyraHUDElementEntry | DCLyraHUDElementEntry |

- 게임 동작 차이는 이 준비 단계에서 의도하지 않는다. 원본 알고리즘/수명은 유지하고 이름 해석과 module/include만 바꾼다. 신규 native 클래스가 선택 가능한 상태가 되지만 **GameMode.HUDClass/Experience Actions/위젯 에셋을 지정하거나 활성 HUD를 교체하지 않는다**.
- **제외:** HUDLayout/ActivatableWidget/ControllerDisconnectedScreen 이식, 원본 위젯/Experience 에셋 생성・복사・수정, PlayerController/PlayerState/Character/GameInstance 활성 교체, 미확인 나머지3종 임의 매핑, 전체 Migrate/114개 덮어쓰기, C++ 빌드/패키징.
- 사용자 승인→7파일 코드 작업과 정적 원본 비교→사용자 Development Editor/Win64 빌드→신규4＋기존30 검사→후속 실제 에셋/플레이어 연결안 순서다. 이번 확인 결과 추가 수정 없이는 재빌드가 필요하지 않으며, 다음7파일 작업은 승인 전 실행하지 않는다. 현재 R5-3의 R7/R13 선행 준비, 실제 장비/발사/재장전 핵심 흐름 및10/8 일정 위험은 미해결이다.

### 2026-10-08 HUD・AddWidgets 기반7파일 작성 / 사용자 빌드 대기

- 사용자 승인 범위의 구현/설정7파일을 반영했다. 신규4파일은 `UI/Lyra/DCLyraHUD.h/.cpp`, `GameFeatures/DCGameFeatureAction_AddWidget.h/.cpp`이며 원본 HUD/Action을 각각 `ADCLyraHUD`, `UDCGameFeatureAction_AddWidgets`로 이식했다. 원본 저작권 표기・수명 이벤트・debug actor 목록・Widget 등록/해제・검증・Client bundle 처리를 보존했다.
- 원본4파일 전체는 이름/경로/명시적 include6개/줄끝 공백을 정규화한 뒤 일치한다. 추가 include는 Action 헤더의 GameplayTagContainer/ObjectKey와 CPP의 AssetBundleData/LocalPlayer/World/PlayerController다. 기존 common WorldActionBase 및 활성 Character/Controller/PlayerState/GameInstance 본문은 수정하지 않았다.
- Build.cs에는 PublicDependencyModuleNames의 `UIExtension`1개만 추가했다. Config에는 위 표의 Class3/Struct2 **5개**만 추가했다. 기존 Redirect/Config/Build 내용을 보존했으며 HUDClass/Experience Actions를 지정하지 않았다. .uproject/플러그인/바이너리 에셋 변경은 없다.
- 신규 `Tests/DCHUDFoundationAutomationTests.cpp`의 `DreamCatcher.R5.HUDFoundation` **4건**:
  - `RegistrationAndDefaults`: Redirect5개 종류/정확한 대상・fixup 후 native 로드, HUD/WorldAction 상속, 원본 HUD Tick 시작값false, Layout/Element의 비어 있는 기본값.
  - `ActionDataAndBundles`: transient action의 반영 배열을 확인하고 기본값/누락4필드/유효한 class+tag 검증, 직접 Client bundle의 WidgetClass와 Layout/Widget의 Client metadata를 검사한다. 이미 등록된 태그를 데이터 유효성에만 사용하며 실제 UI layer 존재를 뜻하지 않는다. abstract/native widget class는 참조만 하고 인스턴스를 만들지 않는다.
  - `HUDReceiverLifecycle`: 엔진 FTestWorldWrapper의 격리 Game 월드에서 ownerless transient HUD의 ReceiverAdded→GameActorReady→ReceiverRemoved와 Tick 비활성을 확인하도록 작성했다. 실제 GameMode/HUDClass는 바꾸지 않는다.
  - `EmptyActionLifecycle`: 같은 격리 world handle로 빈 Action의 활성/해제를2회 수행하고 Action의 OnStartGameInstance delegate 등록/해제를 확인한다. 첫 회는 Action 해제 후 HUD 제거, 둘째는 HUD 제거 후 Action 해제다. 빈 배열/ownerless HUD 검사이므로 실제 위젯 등록/해제 성공을 대신하지 않는다.
- 수명 검사2건은 **별도 unattended Editor＋`-DCR5HUDIsolatedAutomation`**, 게임 스레드/비Commandlet/DreamCatcher 조건을 요구한다. runtime-subsystem 진단 flag는 거부하며 LocalPlayer수0/viewport없음을 사전 확인한다. 테스트가 만든 fixture/handler/Action만 정리하고 전역 delegate 목록을 일괄 비우지 않는다. 실제 plugin 활성화/PIE/복제/에셋 저장은 없다.
- 정적 검증: 원본4개 정규화 비교, INI/CPP5쌍 일치・동일 source key각1회, 추가블록 제거 후 Config SHA256 동일, UIExtension행 제거 후 Build.cs SHA256 동일, 기존 보호11개 SHA256 불변, whitespace/diff 점검을 통과했다. 기존 native 감사 Python self-test **9건 성공**은 비교기 회귀이며 새 C++4건의 실행 결과가 아니다.
- **Codex는 빌드/패키징/에디터 실행을 하지 않았다. 새 UHT/C++ 컴파일・4건 실행은 미검증**이다. 이전30건 성공과 `NativeAudit_20261008_0128`는 이전 코드 기준 증거로 보존한다. 다음은 사용자 에디터 종료 후 **Development Editor/Win64 빌드→신규4＋기존30 총34건**을 새 보고서로 검사하는 것이다.
- 현재 **R5-3의 R7/R13 직접 의존성 선행 준비**다. HUDLayout/원본 Widget/Experience 연결, 활성 플레이어 교체, 실제 UI/라이플/발사/재장전/PIE/복제와10/8 체크포인트 지연 위험은 별도로 남는다.

### 2026-10-08 18:24 KST HUD 기반 검증 통과 / 화면 기반3종 준비 제안

**확인 완료**

- 사용자 UBT: **02:28:32 KST 시작 / Result: Succeeded / 71.53초**, UHT 및 `DCGameFeatureAction_AddWidget.cpp`・`DCLyraHUD.cpp`・`DCHUDFoundationAutomationTests.cpp` 컴파일 포함. 게임 DLL **02:29:42 / 3,922,432 bytes**. Codex는 C++ 빌드/패키징을 실행하지 않았다.
- `Saved/Automation/R5HUDFoundationChecks/index.json` **18:24:38 KST** 총34건 성공(신규4＋기존30), 실패/경고/오류/notRun0. `Saved/Logs/R5HUDFoundationChecks.log` Error/ensure/Fatal0, 종료0이다.
- 신규4건 RegistrationAndDefaults/ActionDataAndBundles/HUDReceiverLifecycle/EmptyActionLifecycle이 모두 통과했다. HUD 수명 이벤트 순서와 빈 Action의2회 활성/해제・world-start delegate 제거를 확인했다. 실제 LocalPlayer/Widget 생성・UI 렌더링・GameFeature plugin 활성화・PIE/복제 검증으로 확대하지 않는다.
- Config2개/uproject/인덱스/Build/이번 원본 기반 코드/기존 플레이어/감사 스크립트 등 보호15개 SHA256이 동일하다. 이번 작업은 검사・원본 읽기 전용 조사・상태 문서뿐이다. 전체 native 감사는 재실행하지 않았으며 `NativeAudit_20261008_0128`는 이전 코드 기준으로 보존한다.

**다음 묶음의 원본 근거・제약・대안 — 미적용**

- 원본 `Source/LyraGame/UI/LyraActivatableWidget.h/.cpp`: ELyraWidgetInputMode의 Default/GameAndMenu/Game/Menu와 GetDesiredInputConfig를 제공한다. Default는 비어 있는 Optional, Menu는 NoCapture이며 나머지 입력모드/캡처 조합과 Blueprint focus-target 미구현 경고를 그대로 보존할 대상이다.
- 원본 `UI/LyraHUDLayout.h/.cpp`: ActivatableWidget을 상속하며 UI.Action.Escape binding, UI.Layer.Menu에 EscapeMenuClass push, 입력장치 연결/사용자 pairing delegate, 다음 tick의 장치 확인, NativeDestruct의 delegate/ticker 정리를 포함한다. 단순 AHUD Actor와 다른 화면 레이아웃 계층이다.
- 원본 `UI/Foundation/LyraControllerDisconnectedScreen.h/.cpp`: HUDLayout의 구체 의존성으로 HBox_SwitchUser/ Button_ChangeUser의 BindWidget, strict controller pairing 플랫폼 조건, 플랫폼 사용자 선택 UI를 포함한다. HandleChangeUserCompleted에는 원본부터 프로젝트별 사용자 변경 처리가 TODO로 남아 있으므로 이식을 계정 전환 전체 완료로 보고하지 않는다.
- 따라서 원본6파일을 함께 준비한다. 기존 CommonUI/CommonGame/ApplicationCore 등 module을 사용하며 프로젝트 타입/경로/module 이름 및 `LogLyra→LogDC` 연결만 대응할 계획이다. 알고리즘・enum 값・BindWidget 이름・원본 태그 문자열(PrimarlyController 철자 포함)・플랫폼 분기를 임의로 바꾸거나 제거하지 않는다.
- **현재 제약:** 해당3종은 abstract 위젯이다. 일반 NewObject/CreateWidget으로 직접 생성하는 검사를 하지 않고 CDO/반영 메타데이터/기본 정책만 검사한다. 실제 NativeOnInitialized/NativeOnActivated에는 LocalPlayer, CommonUI layer/action 설정, EscapeMenuClass, BindWidget2개를 갖춘 원본 Widget Blueprint가 필요하다.
- 원본 `Config/DefaultInput.ini:38~41`의 CommonUIInputSettings에는 UI.Action.Escape에 Escape/Gamepad_Special_Right 키를 지정한다. 대상 Config/소스 검색에서는 해당 action 설정을 찾지 못했다. native GameplayTag 정의를 가져오는 것과 실제 키매핑/메뉴층이 준비되는 것은 다르다. 이번에는 DefaultInput/전역 입력 설정을 변경하지 않는다.
- **보존 대안 및 재개 조건:** 필요한 원본 UI policy/root layer/Widget Blueprint/Action 설정을 후속 에셋 묶음으로 확인・연결한 뒤 플레이 검증한다. 임시 위젯을 넣거나 BindWidget 검사를 삭제하거나 메뉴 호출을 빼서 성공으로 만들지 않는다. 전체 이식 불가가 아니라 활성 연결 미검증이다. 사용자에게 게임패드가 없으므로 물리적 재연결/플랫폼 사용자 변경은 별도 보류다.
- 동작 차이는 타입명/include/module와 로그 카테고리 대응뿐이며 현재 활성 HUD는 그대로다. 원본 코드 내 계정 전환 TODO나 장치 이벤트 동작을 검증 없이 확장/교정하지 않는다.

**당시 승인 요청 범위 — 아래 후속 승인으로 총8파일 반영**

- 신규 `Source/DreamCatcher/UI/Lyra/DCLyraActivatableWidget.h/.cpp`2개.
- 신규 `Source/DreamCatcher/UI/Lyra/DCLyraHUDLayout.h/.cpp`2개.
- 신규 `Source/DreamCatcher/UI/Foundation/DCLyraControllerDisconnectedScreen.h/.cpp`2개.
- 신규 `Source/DreamCatcher/Tests/DCHUDLayoutFoundationAutomationTests.cpp`1개: 등록/enum/상속, 반영 타입・BindWidget 메타데이터/CDO 기본값, 기본 GetDesiredInputConfig의 unset 결과 등 검사3건. abstract 객체 생성・CDO 수정・장치 이벤트 송신・플랫폼 사용자 선택창・viewport/위젯 활성화를 하지 않는다. 다른3개 입력모드의 실제 활성 전환은 소스 비교와 구분하여 후속 검증으로 남긴다.
- `Config/DefaultEngine.ini`1개에 다음4개만 추가하고 기존키 충돌은 거부한다.

| 종류 | 원본 /Script/LyraGame. | 대상 /Script/DreamCatcher. |
|---|---|---|
| Class | LyraActivatableWidget | DCLyraActivatableWidget |
| Class | LyraHUDLayout | DCLyraHUDLayout |
| Class | LyraControllerDisconnectedScreen | DCLyraControllerDisconnectedScreen |
| Enum | ELyraWidgetInputMode | EDCLyraWidgetInputMode |

- **제외:** .uproject/Build.cs/DefaultInput 및 플랫폼 trait 설정, BP/WidgetTree/메뉴・Experience 에셋 변경, 활성 HUD/Pawn/Controller/GameInstance 교체, 실제 게임패드 입력・플랫폼 사용자 변경, 빌드/패키징. 추가 의존성이 발견돼 이 범위를 넘어가면 먼저 설명한다.
- 사용자 승인→8파일 작성/원본 정적 비교→사용자 Development Editor/Win64 빌드→신규3＋기존34 총37건 검사→후속 원본 UI 에셋/설정 연결안 순서다. 현재 검증된7파일에는 추가 수정이 없으므로 지금 다시 빌드할 필요는 없다. 현재는 R5-3에서 R7/R13 기반을 선행하는 상태이며10/8 실제 장비/발사/재장전 체크포인트 충족은 아직 확인되지 않았다.

### 2026-10-08 HUD 화면 기반8파일 작성 / 사용자 빌드 대기

- 사용자 승인 후 원본6파일을 `UI/Lyra/DCLyraActivatableWidget.h/.cpp`, `UI/Lyra/DCLyraHUDLayout.h/.cpp`, `UI/Foundation/DCLyraControllerDisconnectedScreen.h/.cpp`로 이식했다. enum은 `EDCLyraWidgetInputMode`다. 원본 저작권/enum 순서/입력 정책/Focus 경고/Escape/장치 delegate/ticker/플랫폼 조건/BindWidget 이름/사용자 변경 TODO를 보존했다.
- 타입/파일명・HUDLayout의 include 경로・LogLyra→LogDC 및 명시적 include6개를 제외한 원본6파일 전체의 정규화 비교가 일치했다. 명시적 include는 Activatable header의 EngineBaseTypes, CPP의 CommonInputModeTypes/UIActionBindingHandle, HUDLayout header의 GenericPlatformInputDeviceMapper, HUDLayout/Disconnected CPP의 LocalPlayer다. 원본 함수 동작을 재구현하거나 런타임 guard로 생략하지 않았다.
- `Config/DefaultEngine.ini`에는 위 표의 Class3/Enum1 Redirect **4개만** 추가했다. 기존키 중복이 없고 기존4쌍/CPP검사4쌍이 일치한다. DefaultInput의 Escape 매핑이나 플랫폼 trait/기본 UI 클래스 선택은 이번 범위에 없다.
- 신규 `Tests/DCHUDLayoutFoundationAutomationTests.cpp`에 `DreamCatcher.R5.HUDLayoutFoundation` **3건**을 작성했다.
  - `RegistrationAndEnum`: exact Redirect/대상 종류와 native path fixup 후 로드, abstract/native class와 원본 상속, 입력 enum4개＋생성 sentinel의 이름/값을 검사한다.
  - `ReflectedDefaults`: const CDO에서 InputConfig=Default, mouse capture=CapturePermanently, 비어 있는 Escape/disconnect 클래스/인스턴스, 원본 플랫폼 태그2개, 필수 BindWidget2개의 타입/메타데이터를 확인한다. native UI.Layer.Menu/UI.Action.Escape 태그 등록은 실제 UI layer나 키매핑 존재를 의미하지 않는다.
  - `DefaultInputPolicy`: Activatable/HUDLayout CDO의 원본 const getter가 unset Optional을 반환하고 설정 기본값이 유지되는지 확인한다. 다른 입력모드를 시험하려고 CDO를 변경하지 않는다.
- 검사 코드에는 일반 abstract 객체/Widget 생성, NativeOnInitialized/NativeOnActivated 호출, viewport/LocalPlayer/월드 생성, 장치 이벤트 송신, 플랫폼 사용자 선택창, 에셋 저장이 없다. 실제3개 비기본 입력모드 활성 전환/포커스/BindWidget 연결/장치 callback/ticker 정리는 후속 실행 검증 대상이다.
- 정적 검증: 원본6파일 정규화 동일, INI/CPP4쌍/각 source key1회, 새블록 제거 후 Config 정규화 SHA256 일치, protected14개 SHA256 불변, 신규파일 whitespace와 diff 점검을 통과했다. 보호대상은 DefaultInput/DefaultGame/uproject/Git 인덱스/Build.cs/기존 HUD・AddWidgets/Controller/GameInstance/UIManager/감사스크립트/이전34건 보고서다.
- 기존 native 감사 Python self-test **9건 통과**는 비교기의 회귀 결과이며 신규C++3건 결과가 아니다. **Codex는 UHT/C++ 빌드/패키징/에디터 실행을 하지 않았다. 새 코드 컴파일・신규3건 실행은 미검증**이다.
- 다음은 사용자 **에디터 종료→Development Editor/Win64 빌드→신규3＋기존34 총37건 검사**다. 기존 HUD 수명 검사를 함께 실행할 때 별도 unattended Editor＋`-DCR5HUDIsolatedAutomation`을 사용하고 runtime-subsystem 진단 flag는 넣지 않는다.
- 이전34건 성공과 `NativeAudit_20261008_0128`은 이전 코드 기준으로 보존한다. 현재 R5-3/R7/R13 선행 준비이며 실제 원본 UI/Experience 에셋・Escape 키매핑/레이어・활성 플레이어/라이플/PIE/복제 및10/8 장비/발사/재장전 체크포인트는 미완료다.

### 2026-10-08 19:03 KST HUD 화면 기반 검증 통과 / 실제 UI 에셋 읽기 전용 조사 제안

**확인 완료**

- 사용자 UBT **19:00:33 KST 시작 / Result: Succeeded / 38.16초**, UHT와 새3종/검사CPP 컴파일 포함. 게임 DLL **19:01:11 / 4,004,352 bytes**다. Codex는 빌드/패키징을 실행하지 않았다.
- `Saved/Automation/R5HUDLayoutFoundationChecks/index.json` **19:03:25 KST** 총37건 성공(신규3＋기존34), 실패/경고/오류/notRun0. `Saved/Logs/R5HUDLayoutFoundationChecks.log`의 Error/ensure/Fatal0, 프로세스 종료0이다.
- RegistrationAndEnum/ReflectedDefaults/DefaultInputPolicy가 통과했다. 원본 태그/enum/추상 native 상속, 필수 BindWidget 메타데이터와 기본값, Default policy의 unset 반환까지이며 실제 위젯/메뉴/Escape/장치 전환/계정 변경/PIE/복제는 검증하지 않았다.
- Config3개/프로젝트/인덱스/Build/이번6개 이식 코드/검사/기존 플레이어/감사 스크립트 등 보호16개 SHA256 불변. 이번에는 검사・원본 소스/설정/경로 조사・상태 문서만 진행했고 C++/Config/에셋/활성 경로를 변경하지 않았다. 전체 native 비교기도 다시 실행하지 않았으며 이전 보고서는 보존한다.

**다음 단계의 근거와 한계**

- 원본 `Config/DefaultGame.ini:76~77`의 UIManager는 `/Game/UI/B_LyraUIPolicy.B_LyraUIPolicy_C`를 선택한다. CommonGame의 `UGameUIPolicy`는 LayoutClass를, `UPrimaryGameLayout`은 RegisterLayer/GetLayerWidget을 제공한다. 원본 AddWidgets는 LocalPlayer의 레이어에 Layout을 push하고 UIExtension slot에 Widget을 등록한다. 따라서 native 타입 검사만 반복하는 대신 실제 원본 정책・레이어・HUD 위젯의 연결값/그래프를 확인한다.
- 아래10개는 원본 프로젝트의 실제 파일 존재를 확인했다. **파일명만으로 부모・Action 구성・계층・연결이 맞다고 판단한 것은 아니다.** 실제 UObject/그래프 조회는 아직 수행하지 않았다.
- 기존 `r5_rifle_source_audit.py`는 공개 BlueprintEditorLibrary의 generated_class/get_blueprint_parent_class/list_graphs/pin 조회와 graph outer별 노드 열거를 사용한다. 이 읽기 패턴을 재사용할 수 있으나 그 파일은 import 시 main을 실행하므로 그대로 import하지 않고 새 스크립트에 제한된 헬퍼를 작성한다. 기존 C++ 감사 도구의 source22개 제한을 해제하거나 범용 쓰기 API를 추가하지 않는다.
- WidgetTree/상속 위젯/그래프의 모든 정보가 Python에서 읽힌다는 보장은 없다. 공개 API로 접근할 수 없는 항목은 **미검증**과 필요한 추가 확인 절차를 남긴다. partial 보고서를 전체 이식 가능/성공으로 취급하거나 테스트용 대체 UI로 바꾸지 않는다.

**당시 승인 요청 — 아래 후속 승인으로 스크립트1파일 작성＋원본 읽기 전용 실행**

신규 파일: `Scripts/Editor/r5_ui_source_audit.py`. 원본 Lyra의 별도 unattended Python commandlet에서 다음10개만 명시적으로 로드・조회한다.

1. `/Game/UI/B_LyraUIPolicy`
2. `/Game/UI/W_OverallUILayout`
3. `/Game/UI/Hud/W_DefaultHUDLayout`
4. `/Game/UI/Foundation/Dialogs/W_ControllerDisconnected`
5. `/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD`
6. `/ShooterCore/UserInterface/W_ShooterHUDLayout`
7. `/ShooterCore/UserInterface/HUD/W_WeaponReticleHost`
8. `/ShooterCore/UserInterface/HUD/W_WeaponAmmoAndName`
9. `/ShooterCore/UserInterface/HUD/W_QuickBar`
10. `/ShooterCore/UserInterface/HUD/W_QuickBarSlot`

- 수집: 실제 class/parent/CDO의 선택 속성, ActionSet Actions・Layout/Widgets의 클래스/태그, UI policy의 LayoutClass, 원본 WidgetTree의 이름/타입 및 접근 가능한 그래프・핀 값・연결・패키지 참조. 원본 DefaultUIPolicy/Escape action 설정은 필요한 항목만 읽고 전체 INI를 출력하지 않는다.
- 부모/참조 경로는 기록하되 새 대상을 무제한 명시적 로드하지 않는다. 참조 대상의 자동 로드는 발생할 수 있으므로 AssetRegistry 기반 의존성 metadata/파일 상태를 사전 수집한다. 재귀 의존성은 metadata만 최대4096패키지, 그래프128/노드4096/핀32768/연결50000/보고서64MiB 등 상한을 두며 초과 시 중단/미검증으로 표시한다.
- 대상 프로젝트는 같은 상대경로의 파일 존재 등 읽기 전용 사전 충돌 확인만 한다. 동일경로 존재를 내용/호환성 동일로 판정하지 않으며 대상 에셋을 로드・수정・저장하지 않는다.
- 프로젝트/명시 opt-in/commandlet/unattended/SaveOnCompile=Never/초기 dirty0/소스컨트롤 비활성을 검사한다. 명시적 Compile/Save/Copy/Consolidate/재부모화/위젯 생성/메뉴 활성화는 호출하지 않는다. 로드 중 상태 변화・ensure/Error・읽기 실패를 숨기거나 package를 강제로 clean 처리하지 않는다.
- 원본 선택/확인된 Game・ShooterCore 의존 package와 동반파일 상태를 전후 비교하고, 보호 범위와 외부 package 제외 범위를 보고서에 명시한다. 예상 밖 dirty/파일변경은 성공 처리하지 않는다. 기존 진단 예외를 임의 확대하지 않는다.
- 출력은 `Saved/Diagnostics/R5UIFoundation/<fresh run>/` 아래 신규 JSON/요약뿐이며 같은 보고서 덮어쓰기는 거부한다. 순수 self-test로 경로/상한/직렬화/미검증 판정 등 보호 조건을 먼저 확인한다.
- **제외:** 게임 C++/Build/uproject/Config 변경, 바이너리 직접복사, 실제 Migrate/에셋 생성・저장・Compile, UI 활성 교체, 모든 기능 동등성 판정, 빌드/패키징. **추가 C++ 빌드 없이 진행 가능한 조사**이며 새 스크립트 작성/실행은 아직 승인 대기다.
- 승인→스크립트 작성/순수 검사→원본 읽기 전용 실행→실제 에셋 값/의존성/충돌/미검증 항목을 근거로 에셋 준비 묶음 제시 순서다. 이번 검사만으로 원본 HUD 화면이나 R5 라이플 이식이 완료된 것은 아니다. 현재 R5-3/R7/R13 선행 준비 및10/8 실제 장비/발사/재장전 체크포인트 위험을 유지한다.

### 2026-10-08 19:49 KST 원본 UI 읽기 전용 조사 결과 / 부분 완료

**실행 및 보호 결과**

- 승인된 신규 `Scripts/Editor/r5_ui_source_audit.py`1파일을 작성했다. 경로/선택10개/재귀 의존성/그래프・노드・핀・연결/문자열・배열/보고서 상한, partial 판정, INI 선택 출력, 파일변경 및 덮어쓰기 거부의 자체검사10건을 통과했다. 최초 sandbox 임시폴더 권한 오류3건은 같은 코드의 권한 승인 후10건 성공과 구분한다.
- 원본 Lyra 별도 commandlet A1은 **19:24:50 KST**, 보완 A2는 **19:49:55 KST** 보고서를 생성했다. 각각 `Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A1/`, `UIAudit_20261008_A2/`의 source.json/summary.json이며 이전 보고서를 덮어쓰지 않았다. 로그는 `Saved/Logs/R5UIAudit_UIAudit_20261008_A1.log`, `R5UIAudit_UIAudit_20261008_A2.log`다.
- A2: **선택10개・17그래프・256노드・774핀・568연결・Blueprint 소유 위젯 템플릿89개**, status=partial・failures0・미검증15. 양쪽 프로세스 종료0・에셋쓰기0・dirty0・ensure/Error/Fatal0이다. 기존 원본 프로젝트 경고를 전체 경고0으로 주장하지 않는다.
- metadata544 package 중 Game/ShooterCore542개를 보호했고 파일/동반 상태2710개는 실행 전후 및 프로세스 종료 후 외부 대조에서도 동일하다. Engine/Script 및 `/AudioModulation/Volume`, `/CommonUI/GenericInputActionDataTable`는 파일 해시 보호 밖임을 보고서에 명시했다. 대상의 같은 상대경로 파일52개는 존재만 확인했으며 내용/참조/사용자 변경의 동등성은 미검증이다.
- 대상 Config/uproject/Build/플레이어/기존 코드・도구/인덱스/이전37건 보고서 보호14개 SHA256도 동일하다. 새 스크립트/진단 결과/상태 문서 외 게임 C++/Config/에셋은 변경하지 않았고 빌드/패키징을 실행하지 않았다.

**실제로 확인한 연결**

- `B_LyraUIPolicy`의 부모는 CommonGame.GameUIPolicy, LayoutClass는 `W_OverallUILayout`이다. 전체 레이아웃 부모는 PrimaryGameLayout이며 그래프에 `UI.Layer.Game`, `GameMenu`, `Menu`, `Modal`의 RegisterLayer4개가 있다.
- `LAS_ShooterGame_StandardHUD`에는 AddWidgets Action1개, `W_ShooterHUDLayout→UI.Layer.Game` Layout1개, Widget/slot11개가 있다. QuickBar→HUD.Slot.Equipment, WeaponReticleHost→HUD.Slot.Reticle 외 EliminationFeed/Accolades/PerfStats/터치 위젯도 포함한다. 이를 PC에 당장 쓰지 않는다는 이유로 삭제하지 않았다.
- `W_DefaultHUDLayout`과 `W_ShooterHUDLayout`은 각각 native LyraHUDLayout 부모다(서로 부모/자식이라고 이름으로 추정하지 않는다). 두 BP의 InputConfig는 **GameAndMenu**, mouse capture는 CapturePermanently, EscapeMenuClass는 `/Game/UI/Hud/W_LyraGameMenu`, disconnect class는 `W_ControllerDisconnected`다. native CDO의 Default/빈 메뉴 값과 실제 BP 설정을 구분한다.
- ShooterHUD의 source-owned 템플릿에서 UIExtensionPoint14개와 태그/ExactMatch를 읽었다. Game/메뉴 스택과 slot 선언이 실제 플레이어에게 등록됐다는 의미는 아니다.
- ControllerDisconnected 템플릿에는 HBox_SwitchUser(UMG.HorizontalBox)와 Button_ChangeUser(W_LyraMenuButton)가 있고 공개 GetParent 결과로 Button→HBox 연결이 확인됐다. CDO의 protected 멤버가 런타임에 실제 바인딩되는지는 별개다.
- **직접 필요한 native 부모 발견:** WeaponReticleHost→`/Script/LyraGame.LyraWeaponUserInterface`; WeaponAmmoAndName/QuickBar/QuickBarSlot→`/Script/LyraGame.LyraTaggedWidget`. 대상 C++/Redirect에서 대응을 찾지 못했다. DefaultHUD 템플릿에는 `/Script/LyraGame.IndicatorLayer`도 있다. 기존 이식한 ReticleWidgetBase/HUDLayout로 임의 교체하지 않는다.
- StandardHUD의 `GameFeaturesToEnable=[ShooterCore]`도 확인했다. 대상 프로젝트 plugin 목록에는 ShooterCore가 없으며 ExperienceManagerComponent는 plugin URL을 찾지 못하면 ensure를 내는 경로를 갖는다. 실제 활성화 실패를 이번에 재현한 것은 아니지만 준비되지 않은 연결이며, 원본 flag를 지워 우회하지 않는다.
- 원본 DefaultUIPolicy 설정과 Escape/Gamepad_Special_Right UI.Action.Escape 매핑은 source에 있고 대상의 조사한 DefaultGame/DefaultInput 섹션에는 없다. 이번에는 설정을 적용하지 않았다.

**15개 미검증의 의미와 후속 확인**

- WidgetTree 직접 property 접근8개와 CDO의 Button_ChangeUser/HBox_SwitchUser2개는 Python이 protected라 거부했다. property flags/접근권한을 바꾸지 않았다. C++ UBaseWidgetBlueprint의 공개 GetAllSourceWidgets는 UFUNCTION이 아니므로 Python에 그대로 노출된다고 가정하지 않는다.
- 같은10개 Blueprint를 outer로 갖는 이미 로드된 WidgetTree/UWidget을 공개 ObjectIterator로 열거해89개 템플릿을 보완했다. 이는 source-owned 객체 목록이며 inaccessible WidgetTree 포인터의 정확한 연결/상속・generated tree/런타임 binding을 검증한 것으로 바꾸지 않았다.
- 나머지5개는 EdGraphNode_Comment의 비K2핀 미조회 기록이다. 별개5개 게임 기능 오류로 보지 않는다. 전체 보고서를 complete로 만들기 위해 기록을 삭제하지 않았다.
- 정확한 private property/상속tree/runtime binding은 후속의 제한된 C++ 공개 조회 어댑터 또는 원본 Editor 확인・실제 Widget 생성 검증으로 확인한다. 이번에는 새 C++ API/에셋 쓰기를 추가하지 않았다.

**당시 제안 — 아래 후속 승인으로 실제 누락 부모2종의6파일 반영**

1. `Source/DreamCatcher/UI/Lyra/DCLyraTaggedWidget.h/.cpp`2개: 원본 `Source/LyraGame/UI/LyraTaggedWidget.h/.cpp`.
2. `Source/DreamCatcher/UI/Weapons/Lyra/DCLyraWeaponUserInterface.h/.cpp`2개: 원본 `Source/LyraGame/UI/Weapons/LyraWeaponUserInterface.h/.cpp`. 기존 원본 기반 DCLyraEquipmentManagerComponent/WeaponInstance에 타입/include만 대응한다.
3. `Source/DreamCatcher/Tests/DCRifleWidgetFoundationAutomationTests.cpp`1개: 등록/상속, TaggedWidget의 반영 기본값, WeaponUserInterface의 transient CurrentInstance와 OnWeaponChanged 이벤트의 OldWeapon/NewWeapon 타입・순서 검사3건. abstract 일반 객체 생성/CDO 수정/위젯 활성화/실제 Tick・표시・장비 교체를 실행하지 않는다.
4. `Config/DefaultEngine.ini`1개: ClassRedirect **LyraGame.LyraTaggedWidget→DreamCatcher.DCLyraTaggedWidget**, **LyraGame.LyraWeaponUserInterface→DreamCatcher.DCLyraWeaponUserInterface**2개만 추가한다. 기존 동일키 충돌은 거부한다.

**원본 근거・제약・대안・동작 차이**

- 원본 TaggedWidget은 SetVisibility 요청을 저장해 ShownVisibility/HiddenVisibility를 적용하지만 **태그 감시/해제는 TODO이며 bHasHiddenTags=false**다. 원본부터 미구현인 자동 태그 숨김을 완성된 기능으로 보고하지 않는다. 이번 제안은 해당 원본 본문을 보존하며 실제 필요 시 별도 확장 승인/테스트로 구현한다.
- 원본 WeaponUserInterface는 Tick에서 owning Pawn의 EquipmentManager→첫 WeaponInstance를 조회하고, **변경＋Instigator 유효**일 때만 OnWeaponChanged를 호출한다. RebuildWidgetFromWeapon은 원본부터 빈 함수이며 무기가 없을 때 null 변경 알림도 없다. 이를 임의 개선하거나 기존 자체 무기 getter에 연결하지 않는다.
- 실제 동작에는 원본 기반 EquipmentManager가 연결된 Pawn과 inventory instigator가 필요하다. 독립 native 이식/검사는 현재 legacy 플레이어의 무기를 이 UI가 표시한다는 뜻이 아니다. 원본 장비 활성 경로 연결 후 실제 UI 검사로 이어가는 것이 보존 대안이다.
- 변경은 타입명/include/module 연결뿐이며 원본 동작 차이는 의도하지 않는다. 원본 TODO/한계까지 유지하고, source 파일 정규화 비교로 확인한다.
- **제외:** IndicatorLayer/Slate 표시계층, ShooterCore GameFeature 의존성 해결, 추가 위젯/원본 에셋 복사・Compile・저장, 같은경로52개 덮어쓰기, 메뉴/입력 설정, 활성 HUD/플레이어 교체, 빌드/패키징. 원본 전체 이식 불가 판정이 아니며 후속으로 남긴다.
- 사용자 승인→6파일 작성/정적 원본 비교→사용자 Development Editor/Win64 빌드→신규3＋기존37 총40건 검사→다음 의존성/에셋 준비안 순서다. 현재 R5-3/R7/R13 선행 준비이며 원본 실제 UI/라이플/PIE/복제와10/8 장비/발사/재장전 체크포인트는 미완료다.

### 2026-10-08 무기 UI 부모2종6파일 작성 / 사용자 빌드 대기

- 사용자 승인 범위의 원본 `UI/LyraTaggedWidget.h/.cpp`, `UI/Weapons/LyraWeaponUserInterface.h/.cpp`를 `Source/DreamCatcher/UI/Lyra/DCLyraTaggedWidget.h/.cpp`, `UI/Weapons/Lyra/DCLyraWeaponUserInterface.h/.cpp`로 이식했다. 실제 원본 위젯 조사에서 확인된 부모이며 기존 ReticleWidgetBase나 임시 UI로 대체하지 않았다.
- 원본4파일 전체는 타입/파일명・include경로・TaggedWidget header의 SlateWrapperTypes 명시include1개・줄끝 공백 정규화 후 일치한다. 원본 저작권/주석/TODO를 유지했다. WeaponUI는 기존 `UDCLyraEquipmentManagerComponent`/`UDCLyraWeaponInstance`를 참조하며 이들의 본문이나 활성 Pawn/Controller는 수정하지 않았다.
- `Config/DefaultEngine.ini`의 ClassRedirect2개만 추가했다. 기존키 중복0, 테스트의2쌍과 일치, 추가블록을 제외한 Config 정규화 SHA256이 작업 전과 같음을 확인했다.
- 신규 `Tests/DCRifleWidgetFoundationAutomationTests.cpp`에 `DreamCatcher.R5.RifleWidgetFoundation` **3건**을 작성했다.
  - `RegistrationAndInheritance`: 정확한 원본→대상 ClassRedirect, fixup 후 native 경로 로드, 공통 CommonUserWidget 부모와 TaggedWidget의 abstract flag.
  - `TaggedWidgetDefaults`: const CDO의 HiddenByTags 타입/편집・BlueprintReadOnly flags/빈 기본값, ShownVisibility=Visible 및 HiddenVisibility=Collapsed.
  - `WeaponEventAndDefaults`: const CDO의 transient CurrentInstance가 기존 원본 기반 WeaponInstance 타입이며 기본null인지, OnWeaponChanged가 void BlueprintImplementableEvent이고 OldWeapon/NewWeapon 입력2개의 순서/타입/한정자가 유지되는지 검사한다. 이벤트를 호출하지 않는다.
- 검사는 일반 abstract 객체 생성・CDO 수정・NativeConstruct/Tick/SetVisibility/OnWeaponChanged 실행・Widget/월드/플레이어/장비 생성・Blueprint Compile/저장을 하지 않는다. **실제 장비 전환・화면・태그 숨김・네트워크 성공을 검증하는 코드가 아니다.**
- 정적 원본 비교4개, INI/CPP2쌍/중복키0, 추가분 외Config 정규화 해시동일, 신규file whitespace/diff, 보호14개 SHA256 불변을 확인했다. 보호대상은 DefaultInput/DefaultGame/uproject/인덱스/Build/기존 EquipmentManager・WeaponInstance/Character・Controller/조사스크립트/이전37건 및UIAudit_A2 보고서다.
- 기존 native 감사 Python self-test **9건 성공**은 비교기 회귀이며 신규C++3건의 실행이 아니다. **Codex는 빌드/패키징/에디터를 실행하지 않았고 새 UHT/C++ 빌드・검사 실행은 미검증**이다.
- 다음은 사용자 **에디터 종료→Development Editor/Win64 빌드→신규3＋기존37 총40건**을 새 보고서로 검사하는 것이다. 기존 HUD 수명 검사를 포함할 때 `-DCR5HUDIsolatedAutomation`을 사용하고 runtime-subsystem 진단 flag는 넣지 않는다.
- 앞 절의 원본 TODO/태그감시 미구현・무기없음 알림부재를 수정하지 않았다. IndicatorLayer/ShooterCore plugin/나머지원본UI・공유파일52개/실제Pawn장비연결은 범위 밖이며 UIAudit_A2의 partial/미검증15도 그대로 보존한다. 현재 R5-3/R7/R13 선행 준비 및10/8 장비/발사/재장전 체크포인트는 미완료다.

### 2026-10-08 20:14 KST 무기 UI 부모 검증 통과 / Indicator・AsyncMixin 준비 제안

**확인 완료**

- 사용자 UBT **20:11:34 KST 시작 / Result: Succeeded / 28.20초**, UHT 및 TaggedWidget/WeaponUserInterface/검사CPP 컴파일 포함. 게임 DLL **20:12:01 / 4,035,072 bytes**다. Codex는 빌드/패키징을 실행하지 않았다.
- `Saved/Automation/R5RifleWidgetFoundationChecks/index.json` **20:14:25 KST** 신규3＋기존37 총40건 성공, 실패/보고서 경고/오류/notRun0. `Saved/Logs/R5RifleWidgetFoundationChecks.log` Error/ensure/Fatal0, 프로세스 종료0이다.
- RegistrationAndInheritance/TaggedWidgetDefaults/WeaponEventAndDefaults 통과. 실제 Tick/태그 감시/무기 전환/Widget 표시/PIE/복제 성공은 아니며 원본 TODO도 그대로다.
- Config3개/uproject/인덱스/Build/이번4개 코드/검사/EquipmentManager/Controller/조사스크립트/UIAudit_A2 보호15개 SHA256이 유지됐다. 이번에는 검사・원본 읽기・상태 문서만 수행했으며 C++/Config/에셋을 수정하지 않았다.

**원본 근거・구체적 제약・보존 대안**

- UIAudit_A2의 source-owned 템플릿에서 `/Script/LyraGame.IndicatorLayer`를 확인했다. 원본 `Source/LyraGame/UI/IndicatorSystem/`의11개 파일 전체를 읽었고 대상에 대응 구현이 없음을 확인했다.
- IndicatorLayer는 UWidget wrapper이고 실제 표시를 SActorCanvas에 위임한다. Canvas는 IndicatorManager의 추가/제거 이벤트, Descriptor의 위치/정렬/투영 모드, 위젯 Bind/Unbind 인터페이스, 비동기 로드/WidgetPool을 함께 사용한다. 따라서 Layer 하나만 이식하면 원본 표시 경로가 준비되지 않는다.
- SActorCanvas는 **FAsyncMixin을 상속**하고 AddIndicatorForEntry에서 AsyncLoad→StartAsyncLoading을 사용한다. 원본의 `Plugins/AsyncMixin`5파일은 있지만 대상에는 플러그인/Build/프로젝트 등록이 없다. 이는 기술적 이식 불가가 아니라 직접 의존성 미준비다.
- 보존 대안은 원본 Indicator11개와 AsyncMixin5텍스트파일을 함께 이식하는 것이다. 동기 LoadObject/LoadSynchronous로 치환하거나 빈 Canvas/IndicatorLayer를 넣으면 원본 로딩/수명 동작이 달라지므로 이번 안에서는 하지 않는다.
- AsyncMixin은 Core/CoreUObject/Engine만 의존하는 Runtime module이고 StartupModule/ShutdownModule 본문은 비어 있다. 기존 이름/모듈을 유지해 등록하되 게임/UI를 자동 활성화하지 않는다. 이식 자체가 실제 async asset loading 검증은 아니다.
- IndicatorLayer::RebuildWidget은 유효 LocalPlayer가 없으면 ensure를 내며, Canvas::UpdateCanvas는 LocalPlayer/Viewport/ProjectionData/Controller의 IndicatorManager가 필요하다. 단독 transient/CDO/NullRHI 검사로 화면 표시를 완료 판정하지 않는다. 후속 원본 UI/Controller/Indicator Blueprint 연결과 사용자 시각 검증이 필요하다.

**원본 동작을 보존하되 별도 확인할 사항**

- Manager AddIndicator는 manager 지정→추가 이벤트→배열 추가, RemoveIndicator는 제거 이벤트→배열 제거 순서다. 제거 후 Descriptor.ManagerPtr는 초기화하지 않고 SetIndicatorManagerComponent는 최초명시null만 허용한다. 같은 Descriptor를 재등록 가능하다고 가정하거나 임의 초기화하지 않는다.
- Projection은 결과Z에 거리를 기록하지만 원본 Canvas는 `CurChild.SetDepth(ScreenPositionWithDepth.X)`를 호출한다. 이 정적 사실을 확인했으며 거리순 정렬이 정상이라는 보증으로 보지 않는다. 이번 원본 이식에서는 수정하지 않고 후속 표시/중첩 검증 대상으로 남긴다.
- 원본의 manager 부재 처리 TODO/Clamp 상태 알림 주석, 화살표 초기10개 등도 임의 개선/축소하지 않는다. 타입/include/export/명시적 include 외 알고리즘 차이는 의도하지 않는다.

**당시 승인 요청 범위 — 아래 후속 승인으로 총20파일 반영**

1. 원본 Indicator11파일을 `Source/DreamCatcher/UI/IndicatorSystem/Lyra/`에 별도 이름으로 이식:
   - `DCLyraIndicatorLayer.h/.cpp`
   - `DCLyraIndicatorDescriptor.h/.cpp`
   - `DCLyraIndicatorManagerComponent.h/.cpp`
   - `DCLyraIndicatorLibrary.h/.cpp`
   - `IDCLyraActorIndicatorWidget.h`
   - `SDCLyraActorCanvas.h/.cpp`
   - 실제 원본 인터페이스 선언명은 파일명과 달리 **UIndicatorWidgetInterface/IIndicatorWidgetInterface**다. 대상은 UDCLyraIndicatorWidgetInterface/IDCLyraIndicatorWidgetInterface로 대응한다. FIndicatorProjection/Slate helper는 비반영 C++ 타입이므로 이름/include만 대응하고 Redirect를 만들지 않는다.
2. 원본 플러그인의 **텍스트5파일만** `Plugins/AsyncMixin/`에 보존 이식:
   - `AsyncMixin.uplugin`
   - `Source/AsyncMixin.Build.cs`
   - `Source/Public/AsyncMixin.h`
   - `Source/Private/AsyncMixin.cpp`
   - `Source/Private/AsyncMixinModule.cpp`
   - 원본 Binaries/Intermediate/바이너리 에셋은 복사하지 않는다. 플러그인 이름/알고리즘/저작권 표기를 유지한다.
3. `Source/DreamCatcher/Tests/DCIndicatorFoundationAutomationTests.cpp`1파일, **4건**:
   - native/module/Redirect/enum/인터페이스 Bind・Unbind 서명 검사.
   - transient Descriptor의 원본 기본값/공개 setter/getter와 component 없는 Projection 거부 경로. 화면 투영 수치・Viewport 성공으로 확대하지 않는다.
   - ownerless 미등록 Manager＋Descriptor의 추가/제거/Unregister 이벤트・배열 순서와 원본 소유참조 의미. Controller에 붙이거나 재등록 규칙을 바꾸지 않는다.
   - asset 없는 FAsyncScope::AsyncEvent의 호출순서/Start/Cancel/소멸 검사. 에셋 로드/WidgetPool/Slate 생성은 하지 않고 생성한 scope의 상태/timer만 정리한다. process-static state/ticker를 쓰므로 별도 unattended Editor＋`-DCR5IndicatorIsolatedAutomation`을 요구하고 전역 ticker를 강제로 tick하거나 전체 delegate/state를 비우지 않는다.
4. `Source/DreamCatcher/DreamCatcher.Build.cs`1파일: AsyncMixin 직접 의존성 추가.
5. `DreamCatcher.uproject`1파일: AsyncMixin enabled 등록. 플러그인의 EnabledByDefault=false는 원본 그대로 두고 프로젝트에서 명시적으로 켠다.
6. `Config/DefaultEngine.ini`1파일: 아래 **Class5/Enum1 Redirect6개만** 추가한다.

| 종류 | 원본 /Script/LyraGame. | 대상 /Script/DreamCatcher. |
|---|---|---|
| Class | IndicatorLayer | DCLyraIndicatorLayer |
| Class | IndicatorDescriptor | DCLyraIndicatorDescriptor |
| Class | LyraIndicatorManagerComponent | DCLyraIndicatorManagerComponent |
| Class | IndicatorLibrary | DCLyraIndicatorLibrary |
| Class | IndicatorWidgetInterface | DCLyraIndicatorWidgetInterface |
| Enum | EActorCanvasProjectionMode | EDCLyraActorCanvasProjectionMode |

- **제외:** 실제 Indicator BP/ArrowBrush/Widget 클래스 지정, UI 에셋 복사・Compile・저장, LocalPlayer/Viewport/Controller manager 설치, 활성 UI/Pawn/GameMode 변경, ShooterCore GameFeature 연결, UIAudit의protected15개 해결/공유52개 덮어쓰기, 빌드/패키징.
- 준비된 클래스/플러그인이 선택 가능해지는 것 외 현재 플레이 동작 차이는 의도하지 않는다. dependency-bound 묶음으로 한 번에 사용자 빌드/검사를 하되 원본 기능을 줄이지 않는다.
- 사용자 승인→20파일 작성/원본 정규화 비교→사용자 Development Editor/Win64 빌드→신규4＋기존40 총44건 검사→후속 실제 UI/Indicator/라이플 연결안 순서다. 이번 확인에서 코드 변경이 없으므로 지금 재빌드는 필요하지 않다. R5-3/R7/R13 선행 준비 및10/8 실제 장비・발사・재장전 체크포인트는 미완료다.

### 2026-10-08 Indicator・AsyncMixin20파일 작성 / 사용자 빌드 대기

- 승인한20파일을 반영했다. 신규17개는 `Source/DreamCatcher/UI/IndicatorSystem/Lyra/`의 원본 기반11개, `Plugins/AsyncMixin/`의 원본5텍스트파일, `Tests/DCIndicatorFoundationAutomationTests.cpp`1개다. 기존3개는 Build.cs/.uproject/DefaultEngine.ini이며 변경 범위는 아래와 같다.
- Indicator 타입/파일명・include 경로・LYRAGAME_API→DREAMCATCHER_API 및 명시적 include14개를 대응했다. 함수/이벤트 인자명과 Blueprint API 이름은 유지했다. 실제 인터페이스명은 UDCLyraIndicatorWidgetInterface/IDCLyraIndicatorWidgetInterface이고 헤더는 IDCLyraActorIndicatorWidget.h다.
- 원본 Indicator11파일과 AsyncMixin5파일 전체는 허용된 이름/경로/include/export/공백 정규화 후 모두 일치한다. AsyncMixin 이름/module/알고리즘/EnabledByDefault=false와 저작권을 보존했으며 원본 Binaries/Intermediate/에셋은 복사하지 않았다.
- Build.cs의 PublicDependencyModuleNames에 `AsyncMixin`1개, .uproject에 `AsyncMixin / Enabled=true`1개, Config에 위 표의 Class5/Enum1 **6개**만 추가했다. 각 추가분을 제거한 정규화 SHA256이 작업 전과 일치하고 JSON 문법 및 중복 없는 정확한 INI/CPP6쌍을 확인했다.
- 원본 동작 제약도 그대로다. Manager 추가/제거 이벤트와 배열 갱신 순서・제거 후 ManagerPtr 유지・최초 소유자만 허용, Canvas SetDepth(X), 화살표 초기10개, manager 부재/Clamp 알림 TODO, 비동기 load/WidgetPool/Bind・Unbind를 수정하지 않았다. LocalPlayer/Viewport 없는 상태에서 레이어를 억지로 생성하거나 빈 대체물을 넣지 않았다.
- 신규 `DreamCatcher.R5.IndicatorFoundation` **4건**:
  - `RegistrationAndInterface`: AsyncMixin module 로드, native/Redirect6개・enum/상속/Bind・Unbind 인자 타입/const 계약 및 원본 layer CDO visibility, null controller lookup.
  - `DescriptorDefaultsAndGuard`: transient descriptor의 기본값/공개 setter・getter, ownerless scene component에 대한 가시성/자동제거 조건, component 없는 Project 실패와 출력 보존. 실제 component/camera 투영은 실행하지 않는다.
  - `ManagerLifecycle`: owner/world 없는 미등록 manager＋새 descriptor1개의 Add→Unregister, 이벤트 시점의 배열 상태/소유자, 제거 후 manager pointer 유지, null removal. 같은 descriptor를 재등록하지 않으며 Controller에 컴포넌트를 붙이지 않는다. 생성한 listener handle만 RAII로 해제한다.
  - `AsyncSequenceAndCancel`: asset 없는 FAsyncScope::AsyncEvent2개의순서, Cancel 후 callback미실행, scope소멸 후 auto-start 취소. shared 관찰값과 짧은 latent callback으로 **자연스러운 후속 에디터 프레임**에서 중복/지연 callback 여부를 검사하며 전역 ticker를 수동 tick하거나 전체 loading map/delegate를 지우지 않는다.
- 전용조건은 game thread・DreamCatcher・unattended Editor・비commandlet・`-DCR5IndicatorIsolatedAutomation`이며 runtime-subsystem 진단 flag는 거부한다. 테스트는 Slate Canvas/WidgetPool/실제 widget/월드/viewport/에셋 load・save・Compile/PIE/복제를 실행하지 않는다.
- 정적 원본 비교16개・정확한 source manifest17개・설정3개의허용범위・JSON・INI/CPP6쌍・중복0・whitespace/diff・기존 native 감사 Python self-test **9건**을 확인했다. 보호13개(DefaultInput/DefaultGame/인덱스/기존 플레이어・UI 본문/조사스크립트/이전40건 및UIAudit_A2보고서)의 raw SHA256도 동일하다.
- 작업중 `Plugins/AsyncMixin/Intermediate/Build/Win64/x64/UnrealEditor/Development/AsyncMixin/Definitions.AsyncMixin.h`가 별도로 생긴 것을 확인했다(20:38:51 KST timestamp, Git ignored). Codex는 생성 명령/빌드를 실행하지 않았고 생성 주체는 확정하지 않았으며 삭제/수정하지 않았다. 승인된 원본 파일 수와 외부 생성 캐시를 구분한다.
- **Codex는 UHT/C++ 빌드/패키징/에디터를 실행하지 않았다. 새 Indicator/AsyncMixin의 컴파일・신규4건 실행은 미검증**이다. Python9건은 비교기 회귀이며 새 C++4건의 성공 증거가 아니다.
- 다음은 사용자 **에디터 종료→Development Editor/Win64 빌드(AsyncMixin 포함)→신규4＋기존40 총44건 검사**다. 기존 HUD수명 검사의 `-DCR5HUDIsolatedAutomation`과 신규Indicator flag를 함께 사용하며 runtime-subsystem 진단 flag는 넣지 않는다.
- 기존40건 보고서는 이전 코드 기준으로 보존한다. 활성 HUD/Controller/Pawn/에셋/입력 설정은 미변경이며 원본 화면・Slate projection/거리정렬/실제 async asset/WidgetPool/네트워크 및 ShooterCore/GameFeature・UIAudit미검증15/공유52개는 후속이다. R5-3/R7/R13 선행 준비와10/8 실제장비/발사/재장전 체크포인트는 미완료다.

### 2026-10-08 21:07 KST Indicator・AsyncMixin 검증 통과 / UI 읽기 전용 어댑터 제안

**확인 완료**

- 사용자 UBT **21:03:44 KST 시작 / Result: Succeeded / 67.21초**, AsyncMixin/Indicator/검사CPP 컴파일 및 두DLL 링크 포함. 게임 DLL **21:04:51 / 4,204,544 bytes**, AsyncMixin DLL **21:03:55 / 128,512 bytes**다. Codex는 빌드/패키징을 실행하지 않았다.
- `Saved/Automation/R5IndicatorFoundationChecks/index.json` **21:07:37 KST** 신규4＋기존40 총44건 성공, 실패/보고서 경고/오류/notRun0. `Saved/Logs/R5IndicatorFoundationChecks.log` Error/ensure/Fatal0, 종료0이다.
- RegistrationAndInterface/DescriptorDefaultsAndGuard/ManagerLifecycle/AsyncSequenceAndCancel이 통과했다. 에셋 없는 AsyncEvent 순서, Cancel/소멸 후 자연스러운 후속 프레임의 callback 차단도 확인했다. 실제 asset async loading/WidgetPool/Slate 투영・거리정렬/화면/Controller 연결/PIE/복제는 이번 검증이 아니다.
- 중간 대조에서는 Config/uproject/인덱스/Build/Indicator11/AsyncMixin5/검사/Controller/조사script/UIAudit_A2 등26개 SHA256이 동일했다. 최종 재대조에서는 .git/index만 변화가 관측됐고 나머지25개는 동일했다. 인덱스 변화 원인은 확정하지 않았으며 staging/commit/reset을 실행하거나 현재 스테이징을 복구하지 않았다. 이번에는 검사・엔진 원본 읽기・상태 문서만 작업했고 C++/Config/에셋/활성 경로는 수정하지 않았다. UIAudit_A2의partial/15개미검증을 새44건 성공으로 덮어쓰지 않는다.

**다음 단계의 원본 근거・제약・보존 대안**

- UIAudit_A2에서는 Python의 protected 접근 제한 때문에 WidgetTree 포인터8개와 ControllerDisconnected CDO의 HBox_SwitchUser/Button_ChangeUser2개를 직접 읽지 못했다. 공개 객체 열거로89개source-owned템플릿을 확보했지만 실제WidgetTree 포인터/루트 및 runtime BindWidget이 증명된 것은 아니다.
- 엔진 `Source/Editor/UnrealEd/Public/BaseWidgetBlueprint.h`에는 공개 `WidgetTree` 멤버와 const `GetAllSourceWidgets()`가 있다. `Private/BaseWidgetBlueprint.cpp`의 구현은 Widget 가상함수를 실행하지 않고 해당WidgetTree를정확한outer로갖는객체만열거한다. 따라서 protected flag를 바꾸거나 불완전한 목록을 전체검증으로 취급하지 않고, **Editor 전용 제한된 C++ 조회 API**로 명시적인 tree/root/source-template 정보를 읽을 수 있다.
- `UWidgetBlueprintGeneratedClass::GetWidgetTreeArchetype()`/`FindWidgetTreeOwningClass()` 등 공개 조회를 사용해 source tree와 generated/inherited owner를 구분하되, 이미 로드된 객체만 읽는다. 기존 CDO는 `GetDefaultObject(false)`로 확인하고 없으면 미검증으로 남기며 강제로 초기화하지 않는다.
- private BindWidget 멤버2개는 이름/타입을 제한한 const 반영값 조회만 한다. 값null이어도 "위젯누락"으로 판단하지 않는다. CDO값과 실제 Widget생성후binding은 별개이며, 대응 source template의이름/타입과도 별도로보고한다.
- 대안인 원본 Editor 수동 확인도 가능하지만 반복적인 복사 전후 비교에 활용할 수 있도록 어댑터를 제안한다. 이 조회가 아직 실에셋 복사・Migrate・시각/런타임 동등성을 보증하지 않는다. 원본 게임동작을 바꾸는 작업은 아니다.

**승인안 이력 — Editor 도구와 기존 script, 총5파일 / 아래 후속 승인으로 작성**

1. `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/Public/DCRifleMigrationLibrary.h`
   - UI용 새읽기전용 report/API 선언을추가한다. 기존 value-free native report를 CDO값용으로재해석하거나기존 API계약을바꾸지않는다.
2. 신규 `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/Private/DCUIBlueprintReadOnlyAudit.cpp`
   - source tree/root/template이름・타입・부모/slot・generated tree owner, 지정CDO필드2개와주석node의실제핀수 등을읽는다. 조회부에서 Load/Compile/Save/Copy/재부모화/위젯초기화/SetPropertyFlags를호출하지않는다.
3. 신규 `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/Private/Tests/DCUIBlueprintReadOnlyAuditTests.cpp`
   - 거부/범위보호, synthetic template/루트・연결 조회, 지정멤버/누락판정 및 읽기전후불변 등 **3건**. 새 transient WidgetBlueprint/WidgetTree template fixture만만들어필요한메모리Compile을허용한다. 기존실에셋Compile/저장이나player-bound runtime Widget/Viewport는없다. RF_Transient와고유fixture이름, 파일미생성, dirty/flags/포인터전후보존을검사한다. 잘못된입력fixture는비추상타입을사용하며부적절한 UObject/abstract Widget직접생성을하지않는다.
4. `Plugins/DCRifleMigrationTools/Source/DCRifleMigrationTools/DCRifleMigrationTools.Build.cs`
   - Editor 전용 PrivateDependency에 UMG/UMGEditor 추가. 게임module/Build/uproject/Config는변경하지않는다.
5. `Scripts/Editor/r5_ui_source_audit.py`
   - 새API의결과를source JSON에연결하고, 읽기실패/한도초과/누락은미검증으로유지한다. A1/A2보고서는보존하며새run만허용한다. source10개 명시적load경계/파일해시/dirty・ensure・Error보호와64MiB등기존상한은유지한다.

**고정된 실에셋 조회 경계**

기존 source10개 중 WidgetBlueprint8개만 새API에 허용한다.

- `/Game/UI/W_OverallUILayout`
- `/Game/UI/Hud/W_DefaultHUDLayout`
- `/Game/UI/Foundation/Dialogs/W_ControllerDisconnected`
- `/ShooterCore/UserInterface/W_ShooterHUDLayout`
- `/ShooterCore/UserInterface/HUD/W_WeaponReticleHost`
- `/ShooterCore/UserInterface/HUD/W_WeaponAmmoAndName`
- `/ShooterCore/UserInterface/HUD/W_QuickBar`
- `/ShooterCore/UserInterface/HUD/W_QuickBarSlot`

- 실에셋은 원본Lyra의별도unattended commandlet＋`-DCR5UISourceAudit`＋기존diagnostic조건에서만조회한다. 대상DreamCatcher의실에셋이나임의경로는거부한다. synthetic은DreamCatcher의별도unattended/nullrhi Editor＋`-DCR5UIContractTests`에서만허용하고사용자프로젝트내기존패키지를fixture로사용하지않는다.
- 기존 source22 native감사/일반16개・core18개copy정책과dirty예외목록은수정하지않는다. 새UI API는별도고정목록에대한읽기권한일뿐복사권한이아니다.
- **제외:** 게임 C++/AsyncMixin/Indicator/Config/uproject/활성UI・플레이어 변경, 실제UI복사・Compile・저장・Migrate, 전체클로저/공유52개동일성판정, ShooterCore GameFeature 연결, runtime BindWidget/시각/PIE/복제, 빌드/패키징.
- 사용자승인→5파일작성→사용자Development Editor/Win64 빌드→신규3＋기존44 총47건검사→새원본UI run조회→실제에셋준비/복사변경안순서다. 새조회API/실행은아직미적용・미검증이다. 현재확인에서는새코드가없으므로재빌드는필요하지않다. R5-3/R7/R13선행준비와10/8 실제장비/발사/재장전 체크포인트는미완료다.

### 2026-10-08 UI 읽기 전용 도구5파일 작성 / 사용자 빌드 대기

- 사용자 “진행하자” 승인에 따라 위5파일을 반영했다. 게임 C++/Config/uproject/AsyncMixin/Indicator/활성UI/실에셋은 수정하지 않았다. 빌드・패키징・Unreal 실행도 하지 않았다.
- `InspectUIBlueprintReadOnly(UObject*)`는 기존 value-free native report와 분리된 UI report를 반환한다. source 고정8개/원본 commandlet/기존 diagnostic 조건을 요구하며 copy22/16/18 정책은 건드리지 않았다. 테스트는 DreamCatcher 별도 unattended/nullrhi Editor와 `-DCR5UIContractTests`, 고유 GUID 이름・RF_Transient・기존 파일/Linker 없는 fixture로 제한한다. source와 synthetic 모드의 혼용도 거부한다.
- source WidgetTree/root/template 이름・타입・부모/slot, generated class별 tree/root와 최초 유효 owner, 기존 CDO의 지정2필드, 주석 노드의 실제 핀수를 읽는다. widgets4096/graphs128/nodes4096/comment pins32768/상속64/문자열32768/본문문자4Mi 등의 상한과 dirty・flags・tree/slot/CDO pointer・property flags・ensure 전후 검사를 둔다. 조회 실패/상한초과/누락은 완료로 표시하지 않는다.
- **추가 엔진 근거 및 구현 차이:** `Engine/Source/Runtime/UMG/Private/WidgetBlueprintGeneratedClass.cpp:476`의 `FindWidgetTreeOwningClass()`는 내부 `ConditionalPostLoad()`를 호출한다. 이 helper를 그대로 호출하는 대신 공개 `GetWidgetTreeArchetype()`/`GetSuperClass()`로 이미 로드된 계층만 관찰한다. 로딩 필요 flag는 거부하며 최초 비어 있지 않은 root의 class를 기록한다. 유효 root가 없으면 owner를 추정하지 않고 빈값을 기록한다. 원본 게임동작은 변경하지 않는다.
- CDO 필드는 hard `FObjectProperty` 단일값만 허용하고 getter・soft/lazy・잘못된 선언타입/미해결 참조는 읽지 않는다. 실제 source `HBox_SwitchUser`는 HorizontalBox 계열, `Button_ChangeUser`는 CommonButtonBase 계열로 제한한다. `null`/`missing_property`/`unsupported_type`/`unresolved_reference`를 구분하며 null을 runtime BindWidget 실패로 취급하지 않는다.
- C++ 자동검사 **3건 작성, 실행 미검증**: `DreamCatcher.MigrationTools.UIRead.RejectedInputs`, `TemplateTreeAndComments`, `NamedFieldsAndReadOnlyState`. 테스트만 새 WidgetBlueprint/template를 메모리 생성・Compile(SkipSave)한다. 실제 위젯/Viewport/player binding을 생성하지 않는다. 게임모듈 의존성을 추가하지 않기 위해 synthetic Button은 UButton의 typed BP변수로 검사하며, 실제 CommonButtonBase BindWidget 동작과 동일하다고 주장하지 않는다. 부모/자식 tree구분, comment핀0/1, null/명시적fixture값/누락/오류타입, dirty/flags/pointer/파일미생성을 검사한다.
- Python은 version2 보고서에 새API 결과를 연결하고 source-template 목록을 기존 public iterator와 교차 대조한다. comment핀수가0일 때만 빈핀임을 확인하며 1개 이상이면 topology/default는 계속 미검증으로 남긴다. 기존source10개 명시load와 파일해시/dirty/ensure/Error/64MiB/새run만허용 경계는 유지한다. NamedSlot 내용・generated tree전체템플릿/애니메이션/시각/runtime은 bounded조회범위밖이다. A1/A2 보고서는 변경하지 않았다.
- **실행한 검사:** Python 자체12건, AST, source8경계일치, 조회부의 Load/Compile/Save/Copy/PostLoad/flag변경 금지호출 부재, C++3테스트 등록, whitespace/diff 검사 통과. Python 최초 sandbox 임시폴더권한 실패 후 승인된 일반 임시폴더 재실행에서12건을 통과했다. 이는 Unreal/C++ 실행검증이 아니다.
- 작업 전후 보호42개(Config3/uproject/게임Build/Git인덱스/기존도구CPP・검사/Indicator・AsyncMixin/A2/44건보고서)는 SHA256동일했다. 사용자 스테이징/기존 변경은 보존했고 staging/commit/reset/merge는 실행하지 않았다.
- **다음:** 사용자 에디터 종료 후 Development Editor/Win64 빌드 → 신규3＋기존44 **총47건** 검사 → 원본 Lyra의 새 UIAudit run. 이전44건 성공은 보존하지만 새코드 빌드・3건 실행・A2미검증15 해소는 아직 확인 전이다. R5-3/R7/R13 선행 준비 중이며 실제 UI/라이플/PIE/복제・ShooterCore・공유52개 및10/8 실제장비/발사/재장전 체크포인트는 미완료다.

### 2026-10-08 UI 검사 C2666 교정 / 재빌드 대기

- 사용자 첨부 로그의 `DCUIBlueprintReadOnlyAuditTests.cpp` 146/286행 두 오류가 대상이다. `Core/Public/Misc/AssertionMacros.h:84`의 반환형은 SIZE_T이며 `AutomationTest.h:1987`에 SIZE_T 전용 TestEqual 오버로드가 있다. int 리터럴0과의 혼합으로 생긴 모호성을 기대값 `SIZE_T{0}`으로 교정했다.
- 소스 수정은 해당 CPP 두 줄뿐이다. ensure0 검사 의미와 원본/게임 동작은 같으며 테스트 삭제나 허용 기준 완화는 없다. 빌드/패키징/Unreal 실행은 하지 않았고 실제 컴파일 성공과 새검사 성공은 사용자 재빌드 이후 확인한다.
- 진행 방식은 보조 도구 확대보다 실제 라이플 장착·발사·재장전 연결을 우선한다. 작성된 검사를 정리한 뒤 필수 장애물과 R7/R13 후속 검증을 분리한 작업 묶음을 먼저 제시한다. 원본 기능은 삭제하지 않으며 새 코드·Config·에셋 범위는 별도 설명·승인 후 진행한다.

### 2026-10-08 22:41 KST UI 도구 검증 종료 / 실제 라이플 통합 우선

**이번 확인 완료**

- 사용자 UBT는 **22:35:17 KST 시작, Result: Succeeded, 6.80초**다. 수정한 UI 검사CPP 컴파일 및 도구DLL 링크를 확인했다. `UnrealEditor-DCRifleMigrationTools.dll`은 **22:35:23 / 618,496 bytes**다.
- `Saved/Automation/R5UIReadChecksFixed/index.json` **22:38:40 KST** 신규UI3＋기존44 **47건 성공**, 실패/보고서 경고/오류/notRun0. 동명 로그 Error/ensure/Fatal0, 종료0이다. UIRead 세 검사의 template/상속owner/CDO null·값·누락·오류타입/주석핀/상태보호 검증이 통과했다.
- 기존 스크립트를 수정하지 않고 원본에서 재실행했다. `Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A3/summary.json` **22:41:05 KST bounded_read_complete**, 고정10개/WidgetBlueprint8개/17그래프/256노드/774핀/568링크/89템플릿, 실패0・bounded읽기미검증0이다. 직접tree/root 조회와 iterator 목록8개 대조가 모두 일치하고 comment5개의 실제핀수를 확인했다. 지정CDO2필드는 모두 관찰된null이며 runtime BindWidget의 성공/실패 판정이 아니다.
- 에셋쓰기0/dirty0/ensure0/Error0/종료0, source 파일・동반2710개 및 보고서의 보호파일 상태 불변이다. 별도대조11개(Config/uproject/게임Build/Git인덱스/조회CPP/검사CPP/script/기존보고서)도SHA256동일하다. 이번 작업은 기존검사・읽기전용조회・문서갱신이며 새C++/Config/에셋수정・빌드/패키징은없다.
- A1/A2의 과거partial보고서는 보존한다. A3는 이전15개의 **읽기 제한**을 해소했을 뿐 전체UI시각/애니메이션/BindWidget/상속템플릿실행/PIE/복제 검증이 아니다. 이 도구 묶음은 정리하고 기능 연결로 넘어간다.

**다음 변경안 — R5-3＋R6 라이플 이식·테스트 플레이어 통합 묶음 / 아직 미적용**

- **실제 결과물:** 원본 라이플을 장착하고 발사·탄약소모·수동/자동재장전·해제까지 GAS 테스트 맵에서 실행한다. 원본Reticle/Ammo 에셋은 보존하되 전체HUD를 먼저 완성하는 작업과 구분한다.
- **에셋 범위:** 검증된 원본 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247` 핵심18개를 실제 DreamCatcher로 이식하고 남은 `GE_Damage_RifleAuto` 및 `Struct_UIMessaging` 참조를 해결한다. 준비폴더18파일의 존재를 재확인했고 대상 동일폴더는 아직 없다. 기존114개공유경로는 자동덮어쓰지 않는다. 항목별 기존 비교근거와 실로드/Compile 결과로 재사용을 판단하며, 미확정 충돌은 경로분리 또는 수동이식 대안을 먼저 제시한다.
- **원본 유지/제약:** 원본 GE의 Instant/Source.BaseDamage snapshot 보정12와 발사・재장전 Blueprint 책임을 유지한다. SetByCaller 테스트GE나 새C++발사로 대체하지 않는다. ADS의 기존 `/Game/LyraMigration/ADS/Struct_UIMessaging`와 원본은 멤버GUID가 같아도 struct GUID가 달라 단순 동명대체가 아니다. 단일타입 연결 후 Reticle Compile/메시지 검증이 필요하며, 기존ADS 에셋을 덮어쓰지 않는다.
- **플레이어 연결 범위:** 새 `/Game/DreamCatcher/GAS/Test/R5/Integration/` 아래 분리된 테스트 Controller/Pawn/PawnData/GameMode/맵으로 연결한다. 기존 원본계층인 Controller의 Inventory/QuickBar/WeaponState와 Pawn의 DCLyraEquipmentManager, ASC 준비 후 아이템부여/slot선택 경로를 사용한다. 현재 C++ Controller 생성자에 이 컴포넌트들이 없는 것은 확인했지만 기존BP 부착상태는 별도 확인한다. 원본 QuickBar::FindEquipmentManager/EquipItemInSlot은 Controller→Pawn의장비관리자를찾고 실제InventoryItem을Instigator로설정한다. 이미 검증한 동일소유관계를 실제 테스트 Pawn에 연결하는 작업이다.
- **기존 경로와의 차이/대안:** 기존 `DCEquipmentManagerComponent::HandleAbilitySystemInitialized`는 `PawnData.DefaultWeaponDefinition`을 Inventory 없이 Instigator=null로 자동장착한다. 새 테스트경로에서 이 자동장착과 원본QuickBar 장착이 동시에 동작하지 않게 분리한다. 기존 구현을 전체삭제하거나 기존맵을덮어쓰지 않고, 독립통합 검증 후 활성경로교체·회귀·대응구현제거 순서를 지킨다. 원본Experience/GameFeature 전체 전환 전의 테스트 연결은 과도기이며 R13완료로간주하지않는다. 추가코드가필요하면 실제막힌연결부만정확히설명하고포함한다.
- **현재 승인 경계:** 이 턴에서는 위변경안을 제시하고 문서에 기록했을 뿐 에셋복사/기존참조치환/새게임코드·Config를 실행하지 않았다. 상세변경범위 승인 후 기존도구/Unreal API로 이식·설정을 묶어서 진행한다. 새로운 범용도구 확장은 기본 다음단계가 아니다.
- **남은 완료 기준:** 실에셋 재로드/Compile → 테스트플레이어 장착1회/실제아이템Instigator/Ability 부여·회수 → 사용자 발사·재장전·카메라/사망 회귀 확인. PIE와복제는 별도 결과를 남긴다. 현재는 R5-3 진행 중이며10/8 실제장비/발사/재장전 체크포인트를 충족했다고 보고하지 않는다.

### 2026-10-08 실제 라이플 이식 수행 / 직접 의존성·초기 장착 코드 작성, 사용자 빌드 대기

사용자가 위 R5-3＋R6 묶음을 승인했다. 신규 범용 Editor 도구 없이 엔진 Migrate API와 제한된 `Scripts/Editor/r5_rifle_integration.py`를 사용했다. 이번에는 실제 에셋이 DreamCatcher에 들어왔지만 **실행 가능한 라이플 통합 완료는 아직 아니다.**

**실제 이식 결과**

- 검증된 원본 `RifleCore_Prepared_0247` 핵심18개와 Game/ShooterCore 프로젝트 의존성969개를 명시적으로 전달했다. 기존 대상114개는 `MigrationOptions.AssetConflict=Skip`으로 보존했고 source와 기존파일의 SHA256 불변을 확인했다. Engine/Niagara/ControlRig 등 엔진 플러그인 콘텐츠를 프로젝트로 복제하지 않았다.
- 첫 실행 `Saved/Logs/R5RifleIntegration-migrate.log` **22:53 KST**: 신규854개가 들어왔지만 `NS_ImpactDataChannel` 1개가 엔진 새 Migrate에서 “package didn't contain an asset” 오류로 빠졌다. 첫 실행은 실패이며 `Saved/Diagnostics/R5RifleIntegration/migration-after.json`은 이 부분 실패 이력으로 남긴다.
- `R5RifleIntegration-channel.log` **22:55:50 KST**: 원본/대상동일경로인 누락1개에만 프로세스전용 `AssetTools.UseNewPackageMigration=0`을 적용해 엔진의 기존 Migrate로 옮겼다. 사용자 Config나 엔진 파일을 변경하지 않았고 일반 파일 I/O로 바이너리를 복사하지 않았다. 해당uasset 원본동일해시 및 기존854개불변, 총신규855개존재/종료0을 확인했다.
- 실제 로드에서 원본 효과태그가 빠진 것이 확인돼 source Config가 참조하는 원본 `DT_AnimEffectTags`, `DT_SurfaceTypes` 2개를 같은 엔진 API로 추가 이식했다. `R5RifleIntegration-tags.log` **23:12:47 KST**, 덮어쓰기0/종료0이다. 이번 신규에셋은 **총857개(라이플18 포함)**이며, 대부분 원본 애니메이션/오디오/효과 의존성이다. 기존114개는 최종대조에서도 변경0이다.

**실에셋 로드 검사에서 확인한 실제 장애물**

- `R5RifleIntegration-verify.log` **22:58 KST / 종료1**: Reload/Reticle의 LyraPlayerState cast와 GetLyraPlayerController, WeaponAudioFunctions의 LyraGameState cast, 음악BP의 원본GameInstance 부모, FootstepEffectTagModifier의 ContextEffect notify/struct, AnimationWarping/AnimationLocomotionLibrary 타입, 피격Select의 Character/Concrete/Glass surface명, ADS 메시지struct 및 탄약tag 누락이 확인됐다.
- 이 검사에서 script의 `core_loaded`/`verify_complete` 출력은 로드 시도 기록일 뿐 성공판정이 아니다. 실제 Error가 있어 전체실패이며 에셋을 저장하지 않았다. 후속 verify모드에는 fresh `-DCRifleIntegrationRun=<name>`와 그에 맞는 `R5RifleIntegration-verify-<name>.log`의 Error/Fatal/ensure 확인을 추가해 즉시 거부한다. 이식한 원본 Blueprint에서 오류 노드/효과를 삭제하지 않았다.

**같은 승인 묶음에서 작성한 게임 코드·연결**

- `Source/DreamCatcher/Feedback/ContextEffects/` 원본10파일을 DCLyra 별도 타입으로 이식했다. AnimNotify・Interface・Component・Library・WorldSubsystem/Settings와 구조체를 함께 보존한다. 타입/파일/include/export/AllowedClasses module경로 및 필요한 명시include 외 본문은 정규화비교10개일치다. 원본의 동기Load/TODO/상태처리는 그대로이며 빈 효과나새발자국구현으로 대체하지 않는다.
- 무기오디오BP의 누락부모를 기존부분구현 GameInstance에 억지로 연결하지 않고, 원본 `LyraGameInstance.h/.cpp`를 별도 `System/DCLyraGameInstance.h/.cpp`로 보존했다. 동일 이름/include/module대응 외 본문2개일치이며 원본DTLS 조건부코드를위해 Build의CoreOnline/DTLSHandlerComponent 의존성을추가했다. 원본 디버그용 고정키·암호화CVar는 원본대로 남지만 기본테스트옵션은false이며 보안운영용으로간주하지않는다. **활성 GameInstanceClass는 기존 DCGameInstance 그대로**, 세션/암호화/전체GameInstance 전환검증은아니다.
- `Development/DCRifleIntegrationActors.h/.cpp` 새2파일은 테스트전용 Controller/GameMode다. Controller에 원본 Inventory/QuickBar/WeaponState를 생성하고 기존 PawnExtension/ASC GameplayReady를 기다려 실제Item을slot0에추가→QuickBar장착한다. 이미장착/legacyDefaultWeaponDefinition/기존slot중복을거부한다. UnPossess 전에 해제하며 ASC해제전/EndPlay도 정리한다. GameMode는 이 테스트에서만 별도PawnData를 기존spawn보다먼저PS에제공한다. 발사/재장전 로직을 C++로 재작성하지 않는다. **이 새코드 UHT/C++빌드·실행은미검증**이다.
- Config에는 정확한 PlayerState/GameState/새GameInstance/ContextEffects 타입Redirect와 PlayerState getter함수Redirect, 원본 및Migrate후ADS메시지경로→기존ADS struct의PackageRedirect2개를추가했다. 기존ADS struct파일은수정하지않으며GUID차이의실제Compile/메시지검증은빌드후실행한다. PhysicalSurfaces1=Character/2=Concrete/3=Glass 및원본Surface→Tag매핑, 원본태그테이블2개를등록했다. `Config/Tags/DCRifleMigrationTags.ini`에는실제로누락됐던태그의원본INI행18개만반영했다. AnimEffect.Footstep.Land는원본테이블, Cosmetic.AnimationStyle은등록된자식의부모태그로확인할예정이다.
- `.uproject`의AnimationWarping/AnimationLocomotionLibrary2개활성화는누락된원본AnimGraph/Modifier타입을복원하기위한것이다. 원본애니메이션을제거하거나대체하지않았다. 기존플레이어/GameMode본문과기존테스트맵은미변경이며신규Integration맵/Blueprint/PawnData는아직생성전이다.

**현재 검증과 다음 작업**

- ContextEffects10＋GameInstance2 정규화원본비교통과, script AST/uproject JSON/Redirect중복0/diff검사통과. 자동이식완료와원본동일성확인뿐이며새코드빌드·수정후실에셋Compile/실제발사·화면·PIE·복제는미검증이다. Codex는 C++빌드/패키징을실행하지않았다.
- **다음은 사용자 Development Editor/Win64 빌드가 필요하다.** 빌드후 같은승인범위에서 실제이식core/종속BP를재로드·Compile하고, 남은타입/태그문제를해당에셋범위에서만해결한다. 이어 `/Game/DreamCatcher/GAS/Test/R5/Integration/`의전용Pawn/PawnData/Controller/GameMode/맵을생성해장착·발사·재장전에연결한다. 새로운범용감사기제작이나전체HUD완료를선행조건으로추가하지않는다.

### R5/R6 실제 통합 코드 빌드 오류 교정 / 사용자 재빌드 대기

- 사용자 첨부 빌드 로그에서 ContextEffects CPP3개의 matching header가 첫 include가 아닌 오류, IntegrationController의 지역 `Pawn`이 AController::Pawn을 숨기는 C4458, `TSubclassOf<UDCEquipmentDefinition>` bool 확인 시 완전한 타입 정의가 없는 C2027/C2672, DTLS 모듈의 프로젝트 plugin 의존성 누락 경고를 확인했다.
- `AnimNotify_DCLyraContextEffects.cpp`, `DCLyraContextEffectComponent.cpp`, `DCLyraContextEffectsSubsystem.cpp`의 own header를 첫 include로 옮겼다. `DCRifleIntegrationActors.cpp`는 `Equipment/DCEquipmentDefinition.h` 추가와 지역변수 `ControlledPawn`으로의 이름 변경만 했다. `.uproject`에는 이미 Build.cs에서 사용하던 엔진 `DTLSHandlerComponent` plugin을 명시했다. 암호화 옵션이나 활성 GameInstance 선택을 바꾸는 작업이 아니다.
- 코드4개는 include와 지역변수명 이외의 본문 불변을 정규화해시로 확인했고, 프로젝트 JSON은 DTLS 한 항목 추가 외 구조가 같다. ContextEffects 전체 CPP의 첫 헤더, JSON, diff 정적 검사는 통과했다. 빌드/패키징/Editor 실행/에셋 변경은 하지 않았으며 실제 컴파일 성공은 사용자 재빌드 후 확인한다.
- 이 교정은 새 기능이나 도구 확장이 아니다. 재빌드 성공 후 기존 승인된 실제 라이플 재로드·Compile 및 Integration 플레이어/맵 연결을 이어간다. 857개 이식과 실사용 완료 상태를 구분한다.

### 2026-10-08 23:57 KST 실제 Integration 맵 생성·PIE 장착/해제 통과

- 사용자 재빌드는 **23:28:13 KST 시작 / 9.70초 / Succeeded**, 게임 DLL **23:28:22 / 4,370,944 bytes**다. 앞선 첫헤더순서/C4458/불완전타입/DTLS 의존성 교정의 빌드 성공을 확인했다. 이 턴에 C++/빌드/패키징은 하지 않았다.
- `R5RifleIntegration-verify-Build1.log` **23:29:35** 핵심18＋보조2 로드·Compile 종료0/Error/ensure0이었다. 다만 Invalid GameplayTag 경고가 남아 있었다. 원인은 `Config/Tags/DCRifleMigrationTags.ini`가 직접 LoadConfig되는 태그 목록 파일인데 Default*.ini 병합용 `+GameplayTagList` 표기를 사용했던 것이다. 원본 ShooterCoreTags.ini 형식처럼 `GameplayTagList=`로18행의표기만교정했다. 태그이름/의미는변경하지않았고 이후 새프로세스 `R5RiflePlayer-Reload2.log`의Invalid GameplayTag0을확인했다.
- `Scripts/Editor/r5_rifle_player_setup.py`는 이 맵에만 쓰는 제한된 설정 레시피다. 기존Pawn/Controller/PawnData와사용중인입력설정을읽었다. 원본테스트Pawn은native DreamCatcherCharacter를부모로하며Hero의legacyInput/camera는false, originalADS routing=true였다. 현재InputConfig에는Fire만있고Reload가없음을확인해새복사본의Fire태그1개를원본 `InputTag.Weapon.FireAuto`로연결하고 `InputTag.Weapon.Reload`+원본IA_Weapon_Reload/R키매핑을추가했다. 기존입력/ADS에셋은변경하지않았다.
- 첫 prepare는 Python의 FKey 생성자 인자오류로 저장전에중단됐다. persistedIntegration에셋0을확인하고엔진FKey::ImportTextItem에맞게R키를import한뒤새프로세스에서재시도했다. **23:43:35 `R5RiflePlayer-Prepare2.log` 생성완료/오류0/종료0**, **23:46:22 `R5RiflePlayer-Reload1.log` 별도재로드검증/저장0/종료0**이다.
- 새에셋7개는모두 `/Game/DreamCatcher/GAS/Test/R5/Integration/` 아래다: `BP_DC_RiflePawn`, `DA_DC_RiflePawn`, `DA_DC_RifleInput`, `BP_DC_RifleController`, `BP_DC_RifleGameMode`, `IMC_DC_RifleReload`, `L_DC_RifleIntegration`. 기존테스트플레이어4파일해시불변, 공유114개도최종대조변경0이다.
- 새Pawn에는원본 EquipmentManager/CharacterParts를추가하고BodyMeshes.DefaultMesh를채웠다. 기존템플릿Manny_Simple/자체ABP의스켈레톤호환을추정하지않고,새테스트Pawn에만원본 SKM_Manny/ABP_Mannequin_Base 묶음을연결했다. 기존무기메시는새Pawn에서만숨겼다. 새PawnData의legacyDefaultWeaponDefinition은비웠고기존PawnExtensionData는spawn전GameMode가공급하도록비웠다. 기존맵/기존PawnDefaults는유지했다. 테스트맵은단순바닥/PlayerStart/조명/기존GAS표적1개이며디자이너맵을수정하지않았다.
- **실제 PIE 자동검증:** 엔진에이미있는 `Project.Maps.PIE`를새맵하나/8초로제한해실행했다. `Saved/Automation/R5RifleIntegrationPIE1/index.json` **23:57:29 성공1/경고0/실패0**, 동명PIE로그Error/ensure/Fatal0/종료0이다. **23:57:21.686** `[R5-R6] Rifle equipped ... Item=DCInventoryItemInstance_0 Equipment=B_WeaponInstance_Rifle_C_0`와 **23:57:29.679** 해제로그를확인했다. 이장착로그는코드상장비1개/실제InventoryItem Instigator일치검사이후출력된다. PawnExtension/Hero GameplayReady도확인했다. 임시빈월드검사가아니라저장한실제Integration맵의PIE다.

**미완료 및 다음 확인**

- 이번PIE는시작/초기장착/종료해제범위다. 실제입력사격・탄약소모・수동/자동재장전・표적피해/사망・조준회귀・화면/복제는아직미검증이다. 사용자는새맵을열어PIE에서좌클릭/R/탄창소진을확인하고로그를전달한다. 이번턴의변경에는추가C++빌드가필요없다.
- **원본Cue연결미완료:** 기존 `/Game/DreamCatcher/GAS/GameplayCues/GCN_DC_Weapon_Rifle_Fire`와새원본 `GCN_Weapon_Rifle_Fire`의태그가동일한 `GameplayCue.Weapon.Rifle.Fire`임을CDO로확인했다. 현재Cue스캔경로는기존테스트Cue/Death이고원본Rifle폴더는미등록이다. 충돌을숨기거나무조건추가하지않았으며기존Cue를삭제하지않았다. 따라서사격연출의원본전환성공으로보고하지않는다. 다음은기존파일보존하에등록전환범위/기존맵영향을설명・승인후원본Fire/Impact Cue등록을연결한다.
- **별도 `-game` 시작실패:** `R5RiflePlayer-GameSmoke1.log` **23:46:31**은맵진입전DCAssetManager의GameData로드fatal/종료3이다. Editor/PIE에서GameData는DCGameData로로드되고registry조회도DCGameData/DefaultGameData였으나비Editor로드실패의근본원인은미확정이다. 이전부터있던문제라고단정하지않고R13/R14 standalone/패키징확인항목으로남긴다. 실패를우회하려고GameData원본/전역AssetManager정책을바꾸지않았다. PIE성공과Standalone성공은구분한다.
- R5-3의실에셋이식·실제장착까지진척했고현재R6사격·재장전실사용검증으로이동한다. R5/R6전체나UI/Standalone/복제전체완료는아니다. 추가범용도구확장은없다.

### 2026-10-09 R6 사용자 사격 확인 — 피해/사망 실패·마우스 상하 반전 진단

사용자가 앞쪽 표적의 피해·사망이 작동하지 않고 마우스 상하가 반대라고 보고했다. 이번 턴은 로그/코드/원본 및 대상 에셋의 읽기 전용 진단이며 **게임 코드·Config·에셋 수정과 빌드는 하지 않았다.** 일회성 조회 스크립트/로그는 Saved/Diagnostics/R6DamageMouse 및 Saved/Logs/R6DamageMouse-*에만 있다. API/인라인명령 조회가 실패한 초기 로그는 성공 근거로 사용하지 않는다.

**확인된 원인**

1. 실제 사용자 `DreamCatcher.log`의 2026-10-09 00:17~00:19 KST 사격마다 `ApplyGameplayEffectToTarget ... with no GameplayEffect` 오류가 있다. 이식본 `GA_Weapon_Fire_Rifle_Auto` CDO의 **GE_Damage=None**를 확인했다. 원본 ShooterCore 및 원본프로젝트의 검증준비본은 모두 `/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto.GE_Damage_RifleAuto_C`가 지정돼 있다. 대상GE는 `/Game/Weapons/Rifle/GE_Damage_RifleAuto`에 정상존재한다. 부모의Pistol GE를쓸것이아니라자식에원본Rifle GE의대상참조를복구해야한다. 어느이식내부단계에서참조가빠졌는지는미확정이며원본의빈설정으로간주하지않는다. 읽기근거: inspect2/original3 로그.
2. 배치된 기존 `BP_DC_GASTestTarget`의CollisionResponses는 **GameTraceChannel2=Ignore**다. 원본기반RangedWeapon은 ECC_GameTraceChannel2로추적하지만구형표적은Visibility만Block한다. 또한프로젝트에는원본무기Trace채널명의Config등록도없다. 원본Config의채널2/3/4는Weapon/Weapon_Capsule/Weapon_Multi이고기본Ignore다. 수정시전체Block으로규칙을바꾸지말고명시적등록과해당표적의Weapon응답만연결해야한다.
3. 이표적은헤더에명시된 **AI·사망Ability없는체력출력용Actor**다. Blueprint도 BP_OnHealthChanged→PrintString뿐이고HealthComponent/DeathAbility 경로가없다. 앞선4번안내에서이를피해·사망모두검증가능한표적으로취급한것은부정확했다. 현재PlayerState/표적의기본팀은NoTeam(-1)이며새Integration코드에는팀부여가없다. 원본TeamSubsystem은이조건에서피해를허용하지않으므로GE/충돌만고쳐서는충분하지않다. 실제실행중팀값은추가확인전이지만기본값/부여경로부재는확인했다.
4. 마우스에는원본과같은 SettingBasedScalar/AimInversion이사용되며Actor의상하처리C++도원본과동일한양수전달이다. 그러나프로젝트 `bEnableLegacyInputScales=True`, 통합Controller의실제CDO `GetDeprecatedInputPitchScale()=-2.5`가확인됐다. 엔진AddPitchInput은이값을곱한다. 원본 `LyraStarterGame/Config/DefaultGame.ini`의LyraPlayerController 섹션은Yaw/Pitch/Roll=1.0이며이부분이빠졌다. 현재활성R4마우스매핑에는구형IMC_Player의Y Negate가없어음수Legacy배율이그대로반영된다. 실제저장된사용자반전옵션까지PIE에서읽은것은아니므로수정후방향/감도검증은남긴다.

**수정 묶음 제안 이력 / 후속 사용자 승인으로 아래 범위 적용 중**

- 이식된자동라이플능력의GE_Damage를대상의원본Rifle GE로연결한다. 원본BaseDamage보정12/Execution/피해계산은유지하고테스트용SetByCaller GE나부모Pistol GE로대체하지않는다.
- 원본무기Trace채널등록과Integration표적의차단응답,서버측테스트팀지정을함께연결한다. 기존공용표적BP는보존한다. 피해규칙을무조건true로바꾸지않는다.
- 사망검증에는별도의Integration표적에이식된원본 HealthComponent/DeathAbility 흐름을연결한다. 기존체력출력Actor를사망지원으로오인하거나HP0를곧바로전체캐릭터사망연출완료로취급하지않는다. 단순큐브표적의흐름검증과래그돌/캐릭터시각검증은구분한다. 기존표적을그대로쓰는대안은피해/HP0까지만검증할수있다.
- `DCRifleIntegrationController`에만원본의Yaw/Pitch/Roll배율1.0을설정한다. 전역LegacyInput스위치나공용입력에셋을뒤집어기존맵을바꾸지않는다. 원본과같은입력처리에복귀하며,중복Legacy감도배율도제거되므로방향뿐아니라감도도다시확인한다.
- 사용자승인후이관련코드·에셋수정을한묶음으로진행하고사용자빌드후실제사격명중/HP감소/Death시작·종료/마우스방향을검증한다. 기존PIE장착1건성공은보존하지만사격/사망성공으로확대하지않는다. 원본Cue등록충돌과-game시작실패도여전히별도미완료다.

### 2026-10-09 R6 수정 적용 — GE 저장·재로드 통과 / 새 표적 코드 빌드 대기

사용자의 “진행하자” 승인에 따라 위 묶음을 진행했다. C++ 빌드와 패키징은 실행하지 않았다.

**적용 내용과 원본 대비 범위**

- `Development/DCRifleIntegrationTarget.h/.cpp`: 구형 공용 체력 표적 C++/BP를 변경하지 않고, 통합 전용 자식 Actor를 추가했다. 기존 ASC/HealthSet 초기화 후 이식된 원본 `DCLyraHealthComponent`와 `GA_Hero_Death`를 연결한다. 원본 HealthComponent의 `GameplayEvent.Death` → 원본 Death Ability의 StartDeath/FinishDeath를 사용하며 피해 계산이나 팀 규칙을 우회하지 않는다. 원본 Death BP의 **Duration=8.0, AutoStartDeath=true**를 유지한다. 표적은 DeathStarted에서 충돌을 끄고 DeathFinished 후 0.1초 뒤 제거한다. 이 연결 코드는 단순 표적용이며 원본 캐릭터 전체·래그돌·시각 연출 이식 완료가 아니다.
- `Development/DCRifleIntegrationActors.cpp`: Integration GameMode에서 서버 측 플레이어 팀을 0으로 부여한다. 새 표적은 팀 1이다. 기존 공용 GameMode/표적의 NoTeam 동작과 원본 TeamSubsystem의 피해 허용 규칙은 바꾸지 않았다.
- `Config/DefaultEngine.ini`: 원본 Weapon/Weapon_Capsule/Weapon_Multi 채널 2/3/4를 **기본 Ignore 그대로** 등록했다. 새 표적만 이 채널들을 Block한다. 전역 기본 Block이나 피해 강제 허용은 없다.
- `Config/DefaultGame.ini`: `DCRifleIntegrationController` 섹션에만 원본 Yaw/Pitch/Roll=1.0을 설정했다. 전역 LegacyInput 스위치·기존 공용 입력 에셋·`DefaultInput.ini`는 보존했다. 중복 Legacy 감도 배율도 제거되므로 실제 방향/감도는 사용자 재확인이 필요하다.
- `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/GA_Weapon_Fire_Rifle_Auto`: `GE_Damage=None`를 `/Game/Weapons/Rifle/GE_Damage_RifleAuto.GE_Damage_RifleAuto_C`로 복구했다. 원본 GE의 BaseDamage 보정12/Execution 자체는 수정하지 않았다.
- `Scripts/Editor/r6_rifle_damage_fix.py`: 이 묶음의 한정된 에셋 설정/검증용 스크립트다. 새 범용 감사기 확장은 아니다. 현재 GE 복구/재로드와 입력 설정 조회까지만 실행했다. **새 표적 BP 생성·기존 맵의 배치 표적 1개 교체는 사용자 빌드 후** 실행한다. 허용 목적지는 `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleDeathTarget`와 같은 폴더의 `L_DC_RifleIntegration`이며, 공용 구형 표적 에셋은 삭제하거나 덮어쓰지 않는다. 기존 위치/회전/스케일·메시/재질·초기 체력은 유지하고 구형 Ignore 충돌 설정은 복사하지 않는다.

**확인 근거와 남은 검사**

1. 최초 저장 2회는 실행 중인 사용자 에디터의 파일 잠금(Windows Error32)으로 실패했다. 실패를 완료로 처리하지 않았다. 사용자에게 저장 후 종료를 요청했고 **“에디터를 닫았습니다”** 확인 후 재시도했다. 에디터 강제 종료나 바이너리 파일 직접 이동/삭제는 없었다.
2. `Saved/Logs/R6RifleFix-Repair3.log` **02:56:00 KST**: GE 지정/Blueprint Compile/저장 1개 성공, 종료0. `Saved/Logs/R6RifleFix-Reload1.log` **02:56:54 KST**: 별도 프로세스 GE 참조 재로드, 통합 Controller 배율1.0, 원본 Death 부모/8초/AutoStart 확인, 저장0/종료0. 두 실행은 오류0이며 **GA_Weapon_AutoReload의 `/Script/LyraGame` import 경고 1종은 남아 있다.** 경고 없는 전체 이식 검증으로 보고하지 않는다.
3. 각 실행에서 공용 표적·Rifle GE·원본 Death·기존 플레이어 BP와 동반 파일20개 상태 유지. 별도 해시 검사에서 구형 표적 C++/BP·DamageExecution·Rifle GE·DefaultInput·Git 인덱스7개가 작업 전후 동일했다. Python AST와 `git diff --check` 통과. 신규 C++의 UHT/컴파일 성공 근거는 아직 없다.
4. 다음은 **사용자 Development Editor / Win64 빌드 → 같은 승인 범위에서 prepare-target → 새 프로세스 verify-target → 새 맵 PIE**다. 실제 사격 명중/HP 감소/Death 시작·8초 뒤 종료/표적 제거, 마우스 방향·감도를 확인한다. 새로운 표적 BP와 맵 교체는 아직 미실행이다. 단순 Actor 표적의 복제·캐릭터 래그돌/원본 사망 FX 전체는 별도 미검증이다.
5. 원본 발사 Cue와 기존 테스트 Cue의 동일 태그 충돌, `-game` 시작 시 GameData 로드 실패는 이번 범위에서 변경하지 않았다. R5-3 장착/해제 성공과 R6 전체 발사·재장전·연출·복제 완료를 구분한다.

### 2026-10-09 R6 새 표적 에셋 연결 — 사용자 빌드·저장/재로드 확인

- 사용자 UBT 로그: **03:03:30 KST 시작, 34.98초, Result: Succeeded**. `DCRifleIntegrationTarget.cpp`, `DCRifleIntegrationActors.cpp`, UHT 생성 모듈을 포함한 빌드다. 게임 DLL은 **03:04:04, 4,386,304 bytes**다. Codex는 빌드/패키징을 실행하지 않았다.
- 기존 승인 범위에서 `r6_rifle_damage_fix.py`의 `prepare-target`을 실행했다. `Saved/Logs/R6RifleFix-TargetPrepare1.log` **03:10:51 KST**에 새 `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleDeathTarget` 생성/Compile/저장 및 `L_DC_RifleIntegration`의 구형 배치 표적 **1개만 교체**가 완료됐다. 원래 공용 표적 BP/파일은 삭제하지 않았고 다른 맵도 변경하지 않았다.
- 별도 재로드 검사 초반3회는 Python enum 래퍼/멤버 이름 오류로 실패했다. 엔진 `EngineTypes.h`의 ScriptName 및 `PyGenUtil.cpp` 변환 규칙을 확인해 **`CollisionResponseType.ECR_BLOCK`**으로 교정했다. 게임 C++ 수정이나 재빌드는 필요하지 않았으며, 실패한 조회 실행은 에셋 저장0이었다.
- `Saved/Logs/R6RifleFix-TargetReload4.log` **03:14:24 KST**: 저장본의 native 부모·원본 `GA_Hero_Death` 참조·팀1·신규 표적1개/구형 표적0개·Weapon Block을 확인했다. 입력배율1.0 및 Death Duration8/AutoStart도 유지됐다. 종료0/저장0이다. 이 결과는 아직 피해·사망 런타임 성공이 아니다.
- 준비/최종 재로드의 엔진 요약은 각각 **오류0/경고15종**이다. AutoReload `/Script/LyraGame` import1종과 Manny PoseAsset 원본 애니메이션 GUID 불일치14종이며, 이번에 관련 에셋을 임의 재생성/재저장하지 않았다. 보호한 기존 표적·GE·Death·플레이어 BP 및 동반파일20개 상태를 유지했다. 별도 코드/Config/Git 인덱스 등9개 해시도 불변이다. 허용된 통합 맵 변경은 별도로 구분했다.
- **03:16:16 KST 실제 PIE 초기화 검사 통과:** `Saved/Automation/R6RifleTargetPIE1/index.json`의 엔진 기본 `Project.Maps.PIE` **성공1/실패0/보고서 경고·오류0**, 프로세스 종료0. `Saved/Logs/R6RifleTarget-PIE1.log`의 **03:16:08.069 플레이어 팀0**, **03:16:08.095 표적 Health100/팀1/GA_Hero_Death_C 준비**, **03:16:08.396 라이플 장착**, **03:16:16.115 해제**를 확인했다. 로그 Error/Fatal/ensure도0이다. PIE 보고서의 경고0은 앞선 로딩 경고15종이 사라졌다는 뜻이 아니다. PIE 전후 새 BP/맵 해시는 동일했다.
- 다음은 사용자 짧은 플레이 확인이다. 추가 C++/Config 수정이나 재빌드는 필요 없다. `L_DC_RifleIntegration`에서 마우스 상하 방향/감도, 앞쪽 표적의 사격 명중 후 Output Log `GASTestTarget`의 체력 감소, HP0에서 `[R6] Target death started`, 원본 대기8초 후 `Target death finished`/표적 제거를 확인한다. 새 BP는 구형 화면 PrintString 그래프를 복사하지 않았으므로 체력 숫자는 Output Log로 본다. 이 PIE 자동 검사는 사격 입력을 주입하지 않았으며 실제 피해·사망/래그돌/시각·복제 성공으로 확대하지 않는다. 원본 Cue 충돌·Standalone 실패도 별도 미완료다.

### 2026-10-09 R6 피해·사망/마우스 실사용 통과 — 원본 Cue 전환 제안

**완료 판정 근거**

- 사용자가 마우스 상하 방향/감도, 앞쪽 표적 명중 후 체력 감소, HP0 사망 시작 및 약8초 뒤 종료/표적 제거의 세 항목 모두 정상이라고 확인했다. 이 수정 묶음은 실사용 검증 통과로 기록한다.
- 최신 `Saved/Logs/DreamCatcher.log` **03:19:59.391~03:20:02.619 KST**에 `GASTestTarget` Health **100→88→76→64→52→40→28→16→4→0**, **03:20:02.619 Target death started → 03:20:10.619 Target death finished**를 확인했다. 이 조건의 피해12를 모든 거리/재질의 고정 피해로 일반화하지 않는다. 마우스 조작감/표적 제거는 사용자 확인이며 로그만으로 재검증했다고 보고하지 않는다.
- 같은 로그의 사격 시 `Default__GCN_DC_Weapon_Rifle_Fire_C: Rifle Fire Cue`가 남아 있다. 피해 계산은 연결됐지만 원본 발사 연출로 전환된 것은 아니다. 세 항목 성공만으로 R6 전체·수동/자동 재장전·원본 시각/반동·복제 완료로 확장하지 않는다.

**다음 묶음: R6-3 원본 발사·탄착 Cue 등록 + 해당 R6-4 연출 확인 / 사용자 승인 전, 미적용**

1. 원본 근거: 기존 `R5RiflePlayer-Reload2.log`의 CDO 조회에서 구형 Fire와 이식된 원본 Fire는 모두 `GameplayCue.Weapon.Rifle.Fire`, 원본 Impact는 `GameplayCue.Weapon.Rifle.Impact`다. 현재 `DefaultGame.ini`의 스캔 경로는 구형 Cue 폴더와 Death뿐이다. 구형 폴더에는 `GCN_DC_Weapon_Rifle_Fire` 1개가 있다. 원본 `LyraGameFeaturePolicy.cpp::OnGameFeatureRegistering/Unregistering`은 `GameFeatureAction_AddGameplayCuePath`의 경로를 CueManager에 추가/제거하고 라이브러리를 갱신한다.
2. 제약/대안: 대상 에셋은 ShooterCore GameFeature가 아니라 ProjectContent에 이식되어 있다. 이번 좁은 묶음에서는 기존 Config 등록 방식을 이용해 원본 Cue를 연결하는 안을 제시한다. 원본 GameFeature 등록 계층까지 앞당기는 대안도 가능하지만 별도 코드·플러그인 수명 연결 범위가 필요하며, 기술적으로 불가능하다고 판정한 것은 아니다. 양쪽을 동시 등록하거나 원본 태그를 새 태그로 바꿔 충돌을 숨기지 않는다.
3. 제안 변경: `Config/DefaultGame.ini`의 `GameplayCueNotifyPaths`에서 `/Game/DreamCatcher/GAS/GameplayCues`를 제외하고 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247`을 등록한다. `/Game/GameplayCueNotifies/DCMigration/Death`는 유지한다. 원본 Fire/Impact Cue와 관련 Niagara/MetaSound/CameraShake 그래프·수치는 그대로 사용하고, 구형 Cue 파일은 삭제/이동/덮어쓰기하지 않는다.
4. 동작 차이/영향: 원본 플러그인 등록·해제에 따른 Cue 수명 대신 프로젝트 Config의 상시 등록이다. 이 차이를 R13 전체 이식 완료로 취급하지 않는다. **프로젝트 전역 설정이므로 동일 태그를 쓰는 구형 테스트 맵의 발사 연출도 바뀔 수 있다.** 구형 무기 파라미터와 원본 Cue의 호환성은 아직 미검증이다. 파일 보존 및 이전 스캔 경로로 복귀 가능한 상태를 유지한다.
5. 순서/검증: 승인 후 먼저 별도 프로세스의 임시 등록 경로로 원본 Cue 로드/태그 중복/직접 의존성을 확인한다. 확인된 범위가 통과하면 Config를 전환하고 새 프로세스에서 실제 등록 클래스를 확인한다. 이후 통합 맵에서 총구·탄피/탄착 효과·소리·카메라 흔들림과 중복 발동/기존 사격·사망 회귀를 확인한다. 시각/조작감은 사용자 확인이며 수동 R/탄창 소진 자동 재장전도 같은 짧은 플레이에서 별도로 확인한다. 추가 C++/에셋 수정이 필요하면 정확한 근거/범위를 제시한 뒤 진행한다. 새 범용 도구 확장은 기본 계획이 아니다.

이번 턴은 완료 상태 기록·읽기 전용 조사·문서 갱신만 수행했다. 게임 C++/Config/에셋/Git 인덱스 변경, 빌드·패키징·새 에디터 실행은 하지 않았다. AutoReload import/Manny PoseAsset 경고, Standalone GameData 실패, 복제/래그돌 전체 검증은 별도 미완료다.

### 2026-10-09 03:34 KST R6-3 원본 Fire/Impact Cue 등록 전환 완료

위 전환 제안에 사용자가 “진행하자”로 승인했다. **Config의 Cue 스캔 경로1개와 한정 검증 스크립트/문서만 변경**했으며 새 게임 C++/범용 도구·에셋 저장·빌드·패키징은 없다.

- `Config/DefaultGame.ini`: 구형 `/Game/DreamCatcher/GAS/GameplayCues` 등록행을 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247`로 교체했다. Death 경로를 유지했다. 구형 Cue는 복구/비교용 파일로 남아 있으며 삭제/이동/태그 변경이 없다. 원본 Cue/연출 그래프·데이터도 변경하지 않았다.
- 원본 플러그인 등록/해제 수명 대신 ProjectContent를 Config로 상시 등록하는 승인된 차이는 그대로다. 프로젝트 전역 영향이므로 구형 맵에서 같은 Fire 태그를 보내면 원본 Cue로 연결된다. 구형 무기 파라미터와 원본 Cue 호환성은 미검증이며 구형 자체 구현의 최종 보존을 의미하지 않는다.
- `Scripts/Editor/r6_rifle_cue_verify.py`는 이 Fire/Impact 전환의 읽기 전용 검사다. 최초4개 실행은 Python에서 보호된 필드/직접 노출되지 않은 타입 접근으로 실패했다. 엔진 native class 조회와 실제 `GlobalGameplayCueSet`의 공개 데이터 조회로 교정했으며 검사를 통과하기 전에는 Config를 변경하지 않았다. 엔진 Console 출력만이 아니라 태그별 클래스/개수를 직접 비교한다.
- `Saved/Logs/R6RifleCue-Preflight5.log` **03:31:58 KST**: 엔진 `-ini:Game`의 프로세스 한정 배열 제거/추가로 사전 검사 성공. 작업 전7개 보호 해시(Config/Git 인덱스/기존Cue/원본Cue2개/통합맵 등)는 변경0이었다.
- `Saved/Logs/R6RifleCue-Configured1.log` **03:32:52 KST**: 실제 Config 전환 후 **임시 옵션 없이 새 프로세스**에서 성공했다. Runtime `DCGameplayCueManager`에 **GameplayCue.Character.Death→기존GCNL_Death, GameplayCue.Weapon.Rifle.Fire→원본GCN_Weapon_Rifle_Fire, GameplayCue.Weapon.Rifle.Impact→원본GCN_Weapon_Impact** 각1개만 등록됐다. 구형 `GCN_DC_Weapon_Rifle_Fire` 등록0. Cue2개 Compile·직접 package14개 로드·에셋저장0·보호파일77상태불변·종료0이다.
- 사전/실제등록 검사 엔진 요약은 각각 **오류0/경고10종**이다. 이 중9종은 엔진 `PrintCues` 명령이 Warning 레벨로 출력하는 태그→인덱스/미매핑 진단이며, 실제 import 경고는 `B_WeaponDecals`의 `/Script/LyraGame` 1종이다. 이 경고를 이번에 해결했다고 주장하지 않는다. AutoReload/PoseAsset의 기존 로딩 경고도 별도 남아 있다.
- `Saved/Automation/R6RifleCuePIE1/index.json` **03:34:02 KST**의 엔진 기본 `Project.Maps.PIE`는 **성공1/실패·보고서 경고·오류0**, 프로세스 종료0이다. `R6RifleCue-PIE1.log`에 03:33:54 플레이어 팀0·표적Health100/팀1/GA_Hero_Death_C·실제 라이플 장착,03:34:02 해제가 있다. Error/Fatal/ensure0. 이 검사는 무입력 시작·초기화·종료이며 실제 FX/음향/CameraShake 재생이나 사격/재장전을 실행하지 않았다.
- 후속 보호 해시에서 이전7개 중 **승인된 DefaultGame.ini만 변경**됐고, Config/스크립트 정적 검사도 통과했다. 원본 Cue·기존 Cue·통합 맵·DefaultEngine·Git 인덱스는 보존됐다.

**다음 사용자 확인 / 추가 빌드 불필요**

에디터를 새로 열고 `L_DC_RifleIntegration`에서 (1) 실제 총구/탄착 효과·사격음·카메라 흔들림과 중복 여부, (2) 일부 사격 후 R 수동재장전 및 탄창 소진 자동재장전, (3) 기존 피해/사망의 Cue 전환 후 회귀를 짧게 확인한다. 이미 통과한 피해·사망/마우스의 완료 이력은 유지한다. 원본과 픽셀/음향 수치 동등성·게임패드/복제·Standalone·전체 R6/R7/R8 완료는 여전히 별도이며 추가 필수 변경이 발견되면 원본 근거/범위를 먼저 설명한다.

### 2026-10-09 재장전 중 사격·유지입력 R·연사음 지연 진단

> **후속 정정으로 재장전 해석/변경안 철회:** 아래는 당시의 잘못된 재현 해석을 포함한 이력이다. 최신 조건은 **좌클릭을 한 탄창 소진→자동재장전 완료 후까지 유지했다가 놓은 이후에도 사격/자동재장전은 되지만 R 수동재장전만 작동하지 않는 상태**다. 단순 유지사격 중 R 차단과 다르며 원본 태그 규칙만으로 원인이 확정되지 않는다. 사용자는 원본과 달라질 필요가 없다고 명시했으므로 매핑 반전·재장전 Ability 정상 종료 연장안은 적용하지 않는다. 수정 승인은 아래 오디오 사본/참조 변경에만 해당한다. 원본 태그/재장전 그래프의 정적 관찰 사실은 남기되 지속 상태 버그의 원인으로 단정한 해석은 철회한다.

**사용자 요구/증상 정정**

- 재장전 모션은 사격 입력으로 끊기면 안 된다. 정상 재장전 모션이 끝나기 전에는 사격하지 않는다. 단, 사망/장비해제 등 별도 필수 취소를 모두 금지하는 요청은 아니다.
- 사용자의 후속 설명에 따라 2번은 **자동 재장전 자체 실패가 아니라, 자동 재장전 이후에도 좌클릭을 계속 유지하는 상태에서 R 수동재장전이 먹지 않는 증상**으로 확정한다. 자동 재장전 정상이라는 사용자 보고와 이 유지입력 경합을 구분한다.
- 연사 시 발사/애니메이션보다 음향이 밀리고 재장전 중에도 총성이 이어진다는 보고다. 자연스러운 잔향과 큐에 남은 추가 발사음은 후속 청취/시각 측정으로 구분한다.

**확인된 원본 근거와 제약**

1. 원본 `/ShooterCore/Game/TagRelationships_ShooterHero`와 대상 `/Game/LyraMigration/AbilitySystem/TagRelationships_ShooterHero`의 관계 배열은 동일하다. `WeaponFire`가 `Reload`를 Block 및 Cancel하고, `Reload`는 Emote만 Block하며 Fire를 막지 않는다. `DA_DC_RiflePawn`도 대상 매핑을 참조한다. 연사 Ability는 WhileInputActive이고 ASC가 유지입력으로 재활성화를 시도한다. 1·2번의 차단/취소는 이 규칙과 일치하며 원본 동작을 변경해야 새 요구에 부합한다.
2. `GA_Weapon_ReloadMagazine.EventGraph`의 `WaitGameplayEvent(GameplayEvent.ReloadDone)` → HasAuthority → ReloadAmmoIntoMagazine → EndAbility를 확인했다. `PlayMontageAndWait.bStopWhenAbilityEnds=false`이며 OnCompleted의 EndAbilityLocally와 별개다. 따라서 탄약이 채워지는 시점에 Ability가 끝나도 애니메이션은 남을 수 있다. 태그 차단만 추가하면 모션 도중 차단이 풀릴 수 있으므로 정상 종료 시점도 조정해야 한다. 원본 Notify 탄약 이전을 단순 Delay로 바꾸지 않는다.
3. 원본과 대상 라이플 CDO는 `FireDelayTimeSecs=0.1199999973`, `AutoRate=1`이다. 발사 부모는 FireDelayTimeSecs를 대기시간에, AutoRate를 몽타주 Rate에 연결한다. 원본과 대상 MetaSound의 `ShotInterval=0.15` 및 `TriggerCounter`, `TriggerQueue`, `AllowShot`, 내부 반복 검사/간격 게이트를 텍스트 export로 확인했다. 양쪽 오디오 export는 Root MetaSound ClassName GUID를 정규화하면 동일하다. 수치 차이가 이식 과정에서 새로 생겼다고 단정하지 않는다.
4. 원본 `/Game/Weapons/B_Weapon.TriggerFireAudio`와 이식본은 AudioComponent 생성/재사용 후 `Execute Trigger Parameter("Fire")`를 보낸다. ShotInterval을 발사간격으로 덮어쓰는 노드는 없다. 발사 약8.33회/초와 오디오 게이트 최대약6.67회/초의 차이는 요청 큐 적체를 설명하는 근거다. **실제 각 총성의 출력 시각/지연량과 이 수정만으로 완전히 해소되는지는 아직 미검증**이며 해결 완료로 보고하지 않는다.

**다음 변경 묶음 제안 / 아직 미적용**

- 공용 원본 매핑을 보존하고 `/Game/DreamCatcher/GAS/Test/R5/Integration/TagRelationships_DC_RifleIntegration` 사본을 생성해 `DA_DC_RiflePawn`에 연결한다. Fire 행의 Reload Block/Cancel을 제거하고 Reload 행에 Fire Block/Cancel을 추가한다. Emote·사망·그 밖의 관계는 보존한다. R 입력이 사격을 중단하고 재장전을 시작하며, 재장전 중 유지된 Fire는 완료 후 다시 활성화되는 규칙이다.
- 이식본 `RifleCore_Prepared_0247/GA_Weapon_ReloadMagazine`에서 탄약 보충은 기존 ReloadDone Notify에 남기고, 정상 Ability 종료는 몽타주 OnCompleted까지 유지하도록 연결을 조정한다. 중단/취소·사망/장비해제 정리는 유지한다. 원본 프로젝트에는 쓰지 않는다. 이 변경은 원본의 사격 우선/조기 Ability 종료와 의도적으로 다르다.
- 원본 `MSS_Weapons_Rifle2_Fire`는 보존한다. Integration/Audio 아래 별도 사본 `MSS_DC_Rifle_Fire`의 ShotInterval을 현재 실제 발사간격0.12로 맞추는 최소 변경을 우선 검증하고, 이식된 `GCN_Weapon_Rifle_Fire`의 해당 Sound 참조만 사본으로 바꾸는 안이다. 원본 음원·레이어/믹싱·잔향·발사속도/피해는 유지한다. 발사속도 자체를0.15로 늦추는 대안은 게임플레이를 바꾸므로 추천하지 않는다. 향후 발사간격을 변경하면 이 값도 동기화해야 하며, 동적 간격 전달은 별도 확장 대안이다. 0.12 대조 후에도 지연이 남으면 실제 출력 시각과 큐 처리를 재확인하고 이벤트 직접 구동/큐 제거 같은 확대 변경을 먼저 설명한다.
- 에셋 변경은 사용자 승인 후 Unreal 내부 API로 수행한다. 추가 C++가 기본 계획은 아니며 필요성이 새로 확인되면 별도 안내한다. 사용자 빌드·패키징 담당은 유지한다.

**읽기 전용 진단 근거**

`Saved/Logs/R6ReloadAudio-target2.log`, `R6ReloadAudio-original3.log`, `Saved/Diagnostics/R6ReloadAudio/DreamCatcher-audio.t3d` 및 `LyraStarterGame-audio.t3d`. 기존 graph reader를 재사용한 한정 스크립트로 에셋을 읽고 텍스트만 export했다. Compile/에셋 저장/게임 C++/Config 변경·빌드·패키징은 없다. 최종 대상52/원본48개 파일상태 불변. original2의 `/ShooterCore/Weapons/B_Weapon` 조회 실패는 정확한 `/Game/Weapons/B_Weapon`을 읽은 original3 결과로 보완했다. 새로운 요구·진단/제안만 문서에 기록했으며 R6 전체 완료 상태로 바꾸지 않았다.

### 2026-10-09 04:18 KST 사용자 정정 반영·오디오 간격만 적용

**최신 재현 조건/승인 범위**

사용자가 1·2번의 해석을 정정했다. 좌클릭을 꾹 눌러 **한 탄창 소진→자동재장전 완료 후까지 유지했다가 놓으면, 이후 사격/자동재장전은 되지만 R 수동재장전만 안 되는 상태**다. 유지사격 중에만 R이 차단되는 원본 규칙과 다르다. 따라서 앞선 재장전 우선 규칙·매핑 반전·모션 종료까지 Ability 연장안은 **철회하고 적용하지 않았다**. 원본 규칙 유지가 최신 요구다. R 버그는 원인 미확정으로 남기고 재현 전후 R 입력 전달·Ability 활성 여부·잔여 차단 태그/탄약 상태를 조사해야 한다. 오디오 수정으로 이 버그를 해결했다고 보고하지 않는다.

사용자가 직접 진행을 승인한 것은 **3번 연사음 간격 수정만**이다. 에디터 종료를 요청해 사용자가 종료했음을 확인한 뒤 다음 두 에셋만 저장했다.

1. 새 `/Game/DreamCatcher/GAS/Test/R5/Integration/Audio/MSS_DC_Rifle_Fire`: 원본 `/Game/Audio/Sounds/Weapons/Rifle2/MSS_Weapons_Rifle2_Fire`를 Unreal Editor 내부 API로 복제했다. 공식 MetaSound Builder의 `SetGraphInputDefault("ShotInterval")`로0.15→0.12만 변경했다. 원본 사본의0.15는 유지했다.
2. 이식 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/GCN_Weapon_Rifle_Fire`: `OnBurst.K2Node_CallFunction_6.Sound` 핀을 새 오디오 사본으로 교체했다. 변경 전후 Cue 핀 값/연결 비교에서 해당 핀1개만 변경됐고 Compile 성공 후 저장했다. 원본 프로젝트/음원·믹싱/레이어·잔향/큐 그래프·실제 발사0.12초·피해 규칙·재장전 Ability/태그/PawnData는 수정하지 않았다. 향후 발사속도를 바꾸면 오디오 사본의 간격도 재확인해야 한다.

**검증 결과**

- 스크립트: `Scripts/Editor/r6_rifle_audio_interval.py`의 한정 inspect/prepare/verify 절차. C++나 범용 Editor 도구 확장은 없었다.
- `Saved/Logs/R6RifleAudio-Inspect1.log` **04:14:57 KST**: 원본0.15/실제 FireDelay0.1199999973 및 기존 Cue Sound 참조 조회 성공·저장0.
- `Saved/Logs/R6RifleAudio-Prepare1.log` **04:16:25**: 새 사본0.12 및 Cue 핀1개 연결, 저장2개·종료0. 오류0/경고2(AN_PlayWeaponMontage의 LyraGame import, 무음 commandlet 환경에서 MetaSound PreSave 오디오 준비 생략)다. 이 실행을 오디오 재생 검증으로 취급하지 않는다.
- `Saved/Logs/R6RifleAudio-Reload1.log` **04:18:24**: 별도 프로세스에서 사본0.12·Cue 참조 재확인, 저장0·종료0·오류0/import 경고1. 원본은0.15 그대로다.
- `Saved/Diagnostics/R6ReloadAudio/Reload1-source.t3d`와 `Reload1-copy.t3d`: 에셋 이름/경로·Root MetaSound ClassName GUID 및 **해당 ShotInterval 기본값1개**만 정규화한 전체 텍스트가 동일하다. 나머지 음원/데이터/그래프가 같음을 텍스트 비교 범위에서 확인했으며 파형/실제 청취 동등성 검사는 아니다.
- 매 실행 보호한 원본/재장전/태그/PawnData/Config/인덱스31개 파일상태가 불변이다. 별도 기존8개 해시에서는 허용된 Fire Cue만 변경됐다. 원본 오디오 및 원본 규칙은 보존했다. 에디터 외부 바이너리 복사/수정·파일 삭제는 없다.

**후속 사용자 확인:** 사용자가 사운드 정상 작동을 확인했다. 따라서 해당 연사음 간격 수정은 사용자 청취 확인까지 통과로 기록한다. 이것을 모든 음향 장치/프레임레이트/복제의 샘플 단위 동등성 검증으로 확대하지 않는다. 다음은 별도 R키 지속 상태 버그의 재진단이며 변경안을 먼저 설명한 뒤 작업한다.

### 2026-10-09 사운드 사용자 통과·R 재장전 Action 수준 재현 결과

- **사운드 수정 실사용 통과:** 사용자가 정상 작동을 확인했다. 음향0.12 사본/참조 변경의 실제 청취 대기는 해소됐다. 모든 장치/프레임레이트/네트워크 음향의 동등성 완료와는 구분한다.
- **R 원인 미확정 / 이번에 게임 로직 수정 없음:** 정확한 조건은 좌클릭을 탄창소진→자동재장전 완료 후까지 유지했다가 놓은 뒤 R만 작동하지 않는 상태다. 원본 규칙 변경, 태그 반전, Ability 강제 종료/상태 리셋은 하지 않았다.
- 임시 `Saved/Diagnostics/R6ReloadState/probe.py`는 엔진의 기존 Enhanced Input Vector Action 주입으로 별도 테스트 PIE를 조작하고 실제 아이템 탄약/ASC 태그만 관찰한다. 새 C++/범용 도구/에셋은 없다. PIE1~4/API1은 Python API 접근/값 전달 문제로 입력 재현 전에 실패했다. 엔진 맵 시작 검사가 성공했더라도 이를 재현 성공으로 간주하지 않는다.
- **R6ReloadState-PIE5.log, 15:05 KST 관찰:** 수동재장전 기준검사25→30 성공. 발사유지로30→0, 자동재장전0→30 후에도1초간 발사를 유지해22발에서 해제했다.0.5초 대기 후 재장전 Action을 주입하자22→30으로 정상 보충됐다. Fire/Reload/NoFiring/InputBlocked 태그는 그 과정에서 정상 해제됐다. `finished reason=action_level_bug_not_reproduced`, `physical_R_key_tested=false`, `asset_saves=0`이다. 물리키/키 매핑/사용자 설정/포커스는 우회한 검사이며 사용자 현상의 해결 증거가 아니다.
- **전체 PIE 보고서 실패1:** `Saved/Automation/R6ReloadState5/index.json`은 NullRHI의 발사 시 Niagara/Decal null 인스턴스 접근 오류 때문에 실패했다. 엔진 Niagara 스폰 함수는 렌더 불가 환경에서 null을 반환한다. 따라서 위 탄약/태그 관찰만 좁게 사용하고 원본 시각 연출/전체 게임플레이 테스트 성공으로 보고하지 않는다. 실제 렌더링 환경에서 동일 FX 오류가 나는지는 이 검사로 확정하지 않는다.
- **정적 키 조회:** `R6ReloadState-Keys2.log`15:10 KST에서 실제 Hero의 IMC_Player priority0, IMC_DC_R4_Aim priority10, IMC_DC_RifleReload priority1을 확인했다. 세 기본 매핑의 R은 Reload 한 곳이며 Trigger/Modifier 없음, Controller의 별도 R 키 이벤트 충돌도 없었다. 최초 Keys1은 UE5.8의 deprecated Mappings만 읽었으므로 실매핑 근거는 DefaultKeyMappings와 Hero를 읽은 Keys2를 따른다. 런타임 리매핑까지 확인한 것은 아니다.
- **다음 확인 절차:** 사용자의 실제 렌더링 PIE에서 Output Log/콘솔로 `Log LogAbilitySystem Verbose`, `Log LogEnhancedInput Verbose`를 한 줄씩 실행한 뒤 정상 R1회→문제 조건→R 실패를 재현하고 PIE를 종료한다. 최신 DreamCatcher.log에서 입력 소비/무시·매핑 변경 및 CanActivate 거부 사유를 읽는다. 이 로그만으로도 불충분하면 필요한 추가 진단 범위를 설명하며, 임의로 규칙을 바꾸지 않는다. 지금 필요한 것은 빌드가 아니라 실제 키 경로의 실패 근거다.
- 게임 C++/Config/에셋/Git 인덱스는 변경하지 않았다. 보호7개 파일 해시 불변, 빌드·패키징 미실행. 문서에는 오디오 통과와 진단 한계를 기록했다.

### 2026-10-09 R 현재 미재현·다음 R7-1/2 무기 UI 연결 제안

사용자가 현재 R 버그가 재현되지 않아 다음 단계로 넘어가길 요청했다. 상태를 **사용자 현재 미재현 / 원인 미확정 / 후속 관찰**로 바꾼다. 확인된 수정 없이 ‘원인 해결 완료’로 쓰지 않는다. 추가 재현 요청은 일단 중단하고 재발 시 기존 조건·로그를 이어 확인한다. 원본 태그/재장전 수명 규칙은 그대로다. 사운드 정상 작동 사용자 확인은 별도로 유지한다.

**다음 제안 — 승인 전이며 이번 턴에는 미적용**

- 목표는 현재 장착된 원본 라이플의 **원본 조준점과 탄약 표시를 실제 화면에 연결**하는 것이다. R7-1의 기반 C++/Redirect는 이미 준비됐으므로 반복 작성하지 않고 R7-2의 Host 실에셋과 연결에 집중한다.
- 원본 근거: `Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A3/source.json`의 `/ShooterCore/UserInterface/HUD/W_WeaponReticleHost`는 native LyraWeaponUserInterface를 부모로 하며 VisWrapper(SizeBox)/WidgetStack(Overlay)를 가진다. OnWeaponChanged에서 아이템의 ReticleConfig.ReticleWidgets를 CreateWidget/InitializeFromWeapon으로 연결하고 기존 위젯을 정리한다. ID_Rifle에는 이식된 `W_Reticle_Rifle`·`W_AmmoCounter_Rifle`이 이미 지정돼 있다. 대상 `DCLyraWeaponUserInterface`는 실제 EquipmentManager의 무기와 Inventory Instigator를 읽고, `DCLyraReticleWidgetBase`는 같은 무기의 기본 퍼짐×배율·카메라 투영을 사용한다.
- 제약: 원본 최종 표시 경로는 **UIPolicy/OverallLayout → ShooterHUD/UIExtension 슬롯 → Host**다. StandardHUD ActionSet은 ShooterCore를 요구하고 현재 프로젝트 DefaultUIPolicy/Experience 전체 HUD 연결은 없다. 완전 경로를 지금 활성화하는 대안은 공용 UI 정책·메뉴/입력·추가 의존성을 함께 다뤄야 한다. 이는 이식 불가능이 아니라 아직 미연결인 범위다.
- 추천 범위는 통합 맵에서 **원본 Host를 테스트 Controller가 직접 생성·표시·제거하여 독립 검증**하는 것이다. 원본 Host·Reticle/탄약 위젯/아이템 데이터는 재사용하지만 표시/제거의 시작 주체가 GameFeature/UI 슬롯이 아닌 테스트 Controller라는 차이가 있다. 최종 프로덕션 경로나 R7-3/전체 HUD 완료로 취급하지 않으며 전체 슬롯 경로는 후속 연결 대상으로 남긴다. 원본 ActionSet의 ShooterCore flag를 지워 우회하거나 다른 위젯을 삭제하지 않는다.
- 제안 파일/에셋: `Development/DCRifleIntegrationActors.h/.cpp`의 로컬 플레이어용 Host 클래스 지정·장착 후 표시·해제/EndPlay 정리, 새 `/Game/LyraMigration/UI/R7/W_WeaponReticleHost` 사본, 기존 `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleController`에만 클래스 설정. 원본은 Unreal 내부 복제/Migrate로 준비하고 타입/그래프 연결 및 기존 Rifle 위젯 참조를 확인한다. 기존 목적지 덮어쓰기/공용 Config/다른 맵 변경은 없다.
- 기존 조준점과 중복되면 **통합 테스트 HUD 사본에서 해당 조준점만 표시하지 않도록** 연결한다. 체력/궁극기/Scope 등 기존 다른 UI를 함께 없애거나 공용 HUD 파일을 덮어쓰지 않는다. 이 경우 대상은 Integration 아래 새 HUD 사본 및 해당 Controller 참조로 제한한다. 정확한 기존 구조가 다르면 확인 근거와 필요한 변경을 먼저 설명한다.
- 검증: 실제 무기/아이템 연결과 탄약 변화, 연사 퍼짐/첫발 정확도/조준·FOV, 사망·장비해제·Pawn 교체의 표시/구독 정리 및 중복 표시를 확인한다. 명중/처치 표시도 원본의 실제 판정 연결 범위만 보고하며 가짜 성공 신호를 만들지 않는다. 빌드는 사용자 담당, 에셋 생성/설정/재로드 검사는 Codex 담당이다. 추가 범용 도구 확장 없이 기존 읽기/복사 API를 재사용한다.

이번 턴은 기존 원본 조사·대상 C++/Config/에셋 존재 읽기와 문서 상태 갱신만 수행했다. 게임 C++/Config/에셋 변경이나 빌드/패키징·새 Unreal 실행은 하지 않았다. R6 남은 퍼짐 회복·반동/표면 비교·장치/복제 및 R7 전체 레이아웃 검증을 임의 완료/제외하지 않는다.

### 2026-10-09 R0~R6 완료 기준 감사 — R7 착수 보류

> 아래는 감사 당시의 판정이다. 여기서 발견한 R1의 GameData 기동/실패태그2건은 후속 **16:15 KST R1 보완 완료** 절의 재저장·설정 복구 및 재검증으로 해소됐다. R6 및 나머지 통합 회귀는 그대로 남는다.

**결론: R0~R6 전체 완료로 판정할 수 없다.** 핵심 기능이 작동한다는 근거는 충분히 있으나, 파일 이식·독립 테스트·통합 테스트·사용자 확인을 단계 전체 완료와 혼합하면 안 된다. 이미 통과한 작업을 처음부터 다시 만들 필요는 없고, 아래 실제 누락과 미검증을 구분해서 닫는다. R7의 독립 Host 연결 제안도 이번 감사의 선행 보완보다 먼저 적용하지 않는다.

#### 점검 범위와 새 실행 근거

- 기준: 이 명세의 R0~R6 세부 작업/검증 항목, 현재 코드/Config, 실제 에셋의 읽기 전용 설정, 저장된 검사 보고서와 사용자 확인. 브랜치 GAS-System, 엔진5.8.1. 최신 사용자 UBT 빌드03:03:30/34.98초 성공, 게임DLL03:04:04/4,386,304bytes. dirty/staged 작업은 그대로 보존했고 C++ 빌드·패키징은 하지 않았다.
- `Saved/Automation/R0R6CompletionAudit_20261009/index.json` **15:35:46 KST**: 기존 Equipment.QuickBarLifecycle1, TeamColor1, NativeRegistration4, R6 Weapon.SpreadAndAttenuation/ItemTagCost2 — **성공8/실패0/경고0/notRun0**. `R0R6CompletionAudit-tests.log` Error/Fatal/ensure0, 종료0. 새 C++ 테스트/도구를 만들지 않았다. 이 검사는 임시 월드/합성 곡선/타입 검사이며 전체 실제 라이플 플레이를 대신하지 않는다.
- 기존 읽기 스크립트를 재사용한 `R0R6CompletionAudit-assets.log`, `-data.log`, `-registry.log`, `-camera.log`: Compile/에셋 저장0. 마지막 카메라 포함18개 package/동반72파일 상태가 읽기 전후 동일했다. 원본 무기/아이템 경로와 실제 Hero flags를 직접 확인했다.
- `R0R6CompletionAudit-game.log` **15:40:28 KST**: 현재 상태의 비에디터 `-game` 진입을 재시도했고 GameData Fatal로 **종료3**. 성공한 Editor/PIE 검사와 별개이며 패키징 검사는 아니다.

#### 단계별 판정

| 단계 | 확인된 완료 범위 | 전체 완료 판정을 위해 남은 범위 | 판정 |
|---|---|---|---|
| R0 | 원본 재사용/교체 결정, 기존 구현과 원본 이력 분리, 승인·역할·환경 기준 기록 | 다음 수정 때 승인 범위를 계속 갱신 | 기준 기록 완료 |
| R1 | 관계표·EffectContext/Globals·AbilitySource/TargetData·Cost·GlobalAbilitySystem·AssetManager/GameData 및 모듈 기반 존재/빌드·일부 실행 확인 | 비에디터 GameData 충돌 및 실패사유 태그 설정 누락 | 미완료: 실제 보완 필요 |
| R2 | ASC/AbilitySet/OnSpawn, PlayerState 소유 ASC, PawnExtension/Hero GameplayReady, 원본 입력/관계표 실행 이력. 현재 Hero legacy입력/카메라=false·원본ADS routing=true | 실제 라이플을 포함한 Pawn 교체/입력 취소/사망 재시작에서 중복 부여·Held/Delegate/태그 잔여를 묶은 최신 통합 회귀 근거 | 핵심 통과, 전체 회귀 닫힘 아님 |
| R3 | Damage/Heal→Health 구현·기존 검증 이력, 원본 Death/Health 연결, 실제 표적100→0 및8초 종료 사용자 확인. DamageImmunity 처리와 중복 OutOfHealth 억제 코드 존재 | 최신 통합 경로의 면역/회복·리셋·장비 정리 및 구형 적과의 경계 회귀. 코드 존재만으로 런타임 통과로 계산하지 않음 | 핵심 통과, 경계 회귀 미완료 |
| R4 | PC Hip/Shoulder/Scope, 정상/강제 종료·사망·UnPossess 및 마우스 정상 사용자 확인. 원본 ADS 카메라 tag/기본FOV70/Blend0.22/관통방지 설정 확인 | 실제 벽/예측 가림·FOV·카메라 blend→원본 무기 조준배율의 정밀 비교. 게임패드 Aim Assist/터치 UI는 기존 보류 | PC 입력/종료 범위 통과, 전체 범위 조건부 |
| R5 | 실제 Inventory→QuickBar→Equipment/Item Instigator 및 원본 Rifle 이식/장착/해제. fixture에서 Actor 부착·Ability 회수·슬롯 순환·동일 Item stat 재장착 보존 재검증 | 실제 Rifle 사격 후 교체/재장착·사망·Pawn 교체 결합 회귀. fixture를 실무기/전체 플레이 검증으로 확대하지 않음 | 기본 연결/독립 수명 통과, 통합 마감 남음 |
| R6 | 실제 원본 발사·탄약 소모·피해/사망, Cue 등록, 사용자 사운드 정상. 합성 퍼짐/거리/재질·비용 테스트 재통과 | 실제 발사 후 회복지연 불일치 재현, 움직임/조준/공중 배율·중복 Tick/반동·거리/표면/엄폐 및 시각 비교 | 명세상 미완료 |

위 ‘남은 근거’는 이미 사용자가 통과시킨 기존 범위를 취소한다는 뜻이 아니다. **현재 통합 상태에서 해당 완료 기준까지 확인한 자료가 있는지**를 엄격히 구분한 결과다. 게임패드/터치/네트워크나 R7 UI를 일괄 선행 조건으로 추가해 모든 단계를 무기한 미완료로 만들지 않는다. 각 단계 기준의 검증과 기존 보류/R14 전체 검증을 별도로 기록한다.

#### 실제 보완 항목과 증거

1. **GameData 비에디터 기동 실패 — 차단 문제.** 새 `-game` 로그1578행에 `Ignoring PrimaryAssetType DCGameData - Conflicts with LyraGameData - Asset: DefaultGameData`,1593~1594행에 로드 Fatal이 있다. `DCAssetManager.cpp`는 Editor에서 soft-path 동기 로드를 하지만 비에디터에서는 PrimaryAssetType 로드 핸들에 의존한다. 현재 Config는 DCGameData 타입/SpecificAssets를 지정하고 있다. 반면 `R0R6CompletionAudit-registry.log`에서 Editor의 일반/디스크전용 AssetRegistry 조회와 실제 클래스는 모두 DCGameData다. 따라서 ‘단순 파일 부재’나 ‘디스크 태그만 낡았다’로 단정하거나 무작정 재저장하지 말고, **비에디터 스캔/등록 시 LyraGameData와 충돌하는 경로**를 확인해 바로잡아야 한다. PIE 성공은 이 실패를 덮지 않는다.
2. **R1-3 실패사유 태그 설정 누락.** 원본 DefaultGame.ini에는 Cooldown/Cost/Networking/TagsBlocked/TagsMissing의 Ability.ActivateFail.*가 명시돼 있다. 대상 Config에 대응 설정이 없고 `R0R6CompletionAudit-registry.log`1911행의 공개 GameplayAbilitiesDeveloperSettings 조회에서5개 모두 `(TagName="")`다. Globals의 protected 필드 직접 조회는 거부됐으므로 해당 인스턴스 값을 읽었다고 주장하지 않는다. 코드가 유효한 실패 태그만 결과에 넣는 점과 최근 Blocking Tags 로그의 빈 사유 `()`가 부합한다. 이는 실패 메시지/진단 계약의 누락이며 **R키 지속 버그 원인이라는 뜻은 아니다**. 개별 ItemTagCost의 자체 FailureTag가 통과한 것과도 구분한다.
3. **R6-1 회복지연의 시간 경로 불일치.** 발사 활성화는 `DCLyraGameplayAbility_RangedWeapon.cpp:459`에서 `UpdateFiringTime()`을 호출하고, `DCLyraWeaponInstance.cpp:58`은 **TimeLastFired**를 갱신한다. `DCLyraRangedWeaponInstance.cpp:154`의 회복 조건은 별도의 **LastFireTime**을 읽으며 이 변수는 헤더196행의0 초기화 외 대입이 검색되지 않는다. 실제 Rifle CDO의 `SpreadRecoveryCooldownDelay≈0.15`를 확인했다. 원본 Lyra에도 같은 구조가 있다. 기존 `SpreadAndAttenuation` 검사는 합성 곡선/기본지연0만 다루며 테스트 자체가 실제 발사 후 지연을 미검증으로 명시한다. **코드상 연결 불일치는 확인됐지만 실제 회복 곡선 영향의 재현/수치 비교 및 변경 승인은 아직**이다. 원본과 다른 수정을 몰래 적용하지 않는다.

#### 현재 활성 범위/후속 단계 경계

- Integration의 `DA_DC_RiflePawn.DefaultWeaponDefinition=None`, 원본 WID/WeaponInstance/AbilitySet의 FireAuto·Reload·OnSpawn AutoReload 참조, Instigator 런타임 확인이 있다. 기존 Character 입력 바인딩은 Hero legacy입력=false일 때 우회되므로 중복 바인딩을 피하는 코드가 있다. Legacy Component가 남아 있다는 사실만으로 이중 발사가 발생한다고 단정하지 않는다.
- 전역 기본 `DA_DC_PlayerPawn`에는 여전히 구형 `WID_DC_Rifle`이 있고 기존 적 `ADCEnemyCharacter`는 비GAS DCHealthComponent를 사용한다. 플레이어 TakeDamage의 Legacy GE bridge는 남아 있다. 실제 원본 라이플 활성화가 검증된 대상은 **통합 테스트 맵**이지 모든 기존 맵/적이 아니다. 적 GAS/실맵 연결/R14 정리는 해당 단계에서 마무리하며 이번 감사에서 임의 삭제·전환하지 않는다.
- HUD 표시 자체는 R7이다. R7 미연결을 이유로 R5의 실제 Item/Instigator 검증을 실패 처리하지 않는다. 반대로 원본 Reticle 에셋 존재를 R7 완료로 계산하지 않는다.
- 사운드0.12 사본은 승인된 원본 대비 차이이며 사용자 정상 확인 완료. R 수동재장전 문제는 사용자 현재 미재현·원인 미확정으로 보류이며 요구대로 추가 재현을 강요하지 않는다.
- 원본과 다른 신규 수정, 디자이너 브랜치 병합, C++ 빌드/패키징은 이번 감사에 포함하지 않았다. 이 감사의 실행은 기존 테스트·읽기 전용 에셋/소스 확인·문서 갱신뿐이다.

**권장 마감 순서:** (1) R1 GameData 시작/실패태그 설정 보완안 → (2) R6 실제 회복지연 재현 및 필요 시 최소 수정안 → (3) R2/3/5의 라이플 수명 결합 회귀와 R4/6 카메라·반동/표면 비교 → (4) R7 착수. 어떤 범위도 자동 완료/삭제하지 않으며 수정은 별도 설명·승인 후 수행한다.

### 2026-10-09 16:15 KST R1 GameData 기동·실패태그 보완 완료

사용자가 감사 후 진행을 승인하여 R1의 두 보완점만 처리했다. R6 코드·재장전 정책·전체 UI·다른 맵 전환은 포함하지 않았다. C++ 빌드·패키징은 실행하지 않았다.

**원본/엔진 근거와 적용 범위**

- 엔진 `AssetManager.cpp::ScanPathsForPrimaryAssets`는 `ExtractPrimaryAssetIdFromData` 결과가 요청한 PrimaryAssetType과 다르면 등록을 거부한다. 이 추출 함수는 해당 검사에서 PrimaryAssetTypeRedirect를 적용하지 않는다. 단순 Redirect 추가/충돌 검사 삭제/비에디터도 강제 동기 로드하도록 변경하는 우회는 선택하지 않았다.
- `/Game/DefaultGameData`만 현재 native `DCGameData`로 로드 후 EditorAssetSubsystem으로 재저장했다. 클래스 이름 이식 이후의 저장 메타데이터를 정상 저장 경로로 갱신한 것이다. 원본 AssetManager 로드 본문, PrimaryAsset 스캔 정의, 게임플레이 수치는 그대로다. Engine 바깥 바이너리 I/O나 전체 에셋 재저장/캐시 삭제는 없다.
- GameData의 피해 GE는 `/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller`, 회복 GE는 `/Game/GameplayEffects/Heal/GE_Heal_SetByCaller`, 동적태그 GE는 `/Game/GameplayEffects/GE_DynamicTag`로 변경 전후 및 새 프로세스에서 모두 동일했다. 라이플의 원본 Rifle Damage GE 참조도 이번 변경 대상이 아니다.
- 원본 DefaultGame.ini의 실패태그5개(Cooldown/Cost/Networking/TagsBlocked/TagsMissing)를 **동일 이름/동작**으로 복구했다. UE5.8의 `GameplayAbilitiesDeveloperSettings.h::OverrideConfigSection`은 호환성을 위해 `/Script/GameplayAbilities.AbilitySystemGlobals`를 실제 INI 섹션으로 사용한다. 따라서 `Config/DefaultGame.ini`의 기존 섹션에 태그5행과 설명주석만 추가했다. DeveloperSettings라는 별도 INI 섹션으로 넣었던 첫 시도는 효과가 없어 제거했으며, 원본과 다른 태그나 새 차단 규칙은 만들지 않았다.
- 재실행 스크립트는 `Scripts/Editor/r1_startup_repair.py`로, resave 모드는 정확히 DefaultGameData1개만 저장하고 verify는 저장0이다. GE 참조/실패태그/보호 파일 검증에 실패하면 저장 전에 중단한다. 새 C++ 도구나 검사 프레임워크는 없다.

**검증 근거**

| 검사 | 결과와 범위 |
|---|---|
| `R1Startup-Resave2.log`,16:09:09 KST | 태그5개 정확한 값 확인, GameData1개 재저장, GE3참조 유지, 종료0 |
| `R1Startup-Reload1.log`,16:11:31 | 별도 프로세스에서 GE3참조/태그5개/PrimaryAssetType=DCGameData 재확인, 저장0/종료0 |
| `R1Startup-GameAfterResave2.log`,16:11:32~39 | **비에디터 -game에서 GameData 정상 로드, L_DC_RifleIntegration 맵 로드 완료, 종료0**. 기존 타입 충돌/Fatal/Error/ensure0 |
| `Saved/Automation/R1StartupRegression_20261009/index.json`,16:15:23 | 기존 장비1·팀1·타입4·무기2 총8건 성공, 실패/경고/notRun0. 로그 Error/Fatal/ensure0 |
| 보호 검사 | 기존9개 중 승인된 DefaultGame.ini/DefaultGameData.uasset만 변경. AssetManager h/cpp·GameData cpp·DefaultEngine·R6 RangedWeapon cpp·관계표·Git인덱스7개 불변. Config는 신규 태그5행+주석만 제거한 비교에서 작업 전 해시와 동일 |

`Resave1`과 `TagsInspect1`은 첫 INI 섹션 시도의 태그 검증 실패로 저장0이었다. 그 사이 `GameAfterResave1`은 이름과 달리 실제 재저장 전 상태에서 실행되어 기존 Fatal을 재확인한 실패 이력이다. 완료 근거로 사용하지 않는다. 성공한 commandlet의 경고1개는 읽기 조회에 사용한 AssetRegistry API deprecation이며 에셋/실행 실패가 아니다. 기존과 같은 클래스/GE 데이터를 재저장한 뒤 비에디터 타입 충돌이 사라졌다는 전후 결과를 근거로 기동 복구를 판정한다.

**완료 판정과 다음 범위**

이번 감사에서 발견한 **R1 기동 차단·실패태그 누락2건은 보완 검증 통과**다. 비에디터 시작/맵 로드 확인을 전체 Standalone 전투·렌더·복제·패키징 성공으로 확대하지 않는다. 현재 변경은 추가 사용자 C++ 빌드가 필요 없다. 다음은 R6 실제0.15초 회복지연의 시간 경로 불일치 재현/최소 변경안이며, 이번에 R6 코드를 수정하지 않았다. R2/3/4/5의 남은 결합 회귀 후 R7로 넘어가는 감사 순서는 유지한다. R 현재 미재현 및 사운드 사용자 통과 이력도 유지한다.

### 2026-10-09 16:27 KST R6 실제 회복지연 재현 — 수정 전 근거·제안 이력

> 아래는 변경 전 재현 및 제안 당시 기록이다. 후속 사용자 승인으로 코드2파일을 반영했으며, 현재 빌드/검증 상태는 바로 다음 ‘R6 회복지연 코드 반영’ 절을 따른다.

R1 보완 이후 실제 라이플 회복지연을 조사했다. **이번 작업은 임시 PIE 진단과 문서 갱신뿐**이며 게임 C++/Config/에셋 변경, C++ 빌드·패키징은 하지 않았다. 기존 입력 관찰 방식을 `Saved/Diagnostics/R6SpreadDelay/probe.py`에 한정 재사용했고 새 범용 도구/API는 추가하지 않았다.

**원본 근거 및 구체적인 제약**

- 원본 `LyraWeaponInstance.cpp:54~58`과 대상 `DCLyraWeaponInstance.cpp:54~58`은 `UpdateFiringTime()`에서 기반 `TimeLastFired`를 기록한다. 원본 RangedWeapon cpp150/대상154의 회복 조건은 별도의 `LastFireTime`을 사용한다. 원본/대상 Weapons 소스에서 이 변수는 헤더의0 초기화 외 갱신이 없다. 따라서 원본 코드 그대로 이식했더라도 마지막 발사 후의 지연을 계산하지 못한다.
- 대상 `DCLyraGameplayAbility_RangedWeapon.cpp:538~544`는 TargetData 유효/Commit 성공 뒤 `AddSpread()`를 호출한다. 이 지점이 발사 실패가 아닌 실제 발사에 연결된 최소 갱신 위치다. 기반 `TimeLastFired`/`GetTimeSinceLastInteractedWith()`는 장착/발사 활동과 AutoReload에 쓰이므로 회복 보완을 위해 동작을 바꾸지 않는다. 이 getter를 회복에 직접 쓰는 대안은 장착 후 대기까지 달라진다.
- 원본 실행 자체의 모든 상황을 비교한 것은 아니다. **원본 소스에도 같은 결함이 있고 이식된 실제 라이플에서 지연 무시가 재현됐다**는 범위다. 원본의 의도나 특정 버전의 수정 여부를 추정해 확정하지 않는다.

**실행과 재현 수치**

`Saved/Logs/R6SpreadDelay-Before1.log`에서16:27:44 KST, Integration PIE의 실제 장착 B_WeaponInstance_Rifle로2초 이상 대기 후6발(30→24)을 발사했다. 실제 설정 지연은0.150000006초였다. 기존 VisibleAnywhere 디버그 필드는 원본 Tick/AddSpread가 갱신하는 값을 읽기만 했다.

| 시점 | 탄약 | 열 | 퍼짐각 |
|---|---:|---:|---:|
| 마지막 발사 관찰, 게임시간2.7523635초 | 24 | 3.242385 | 5.282824° |
| 그 뒤0.019776초 | 24 | 3.163281 | 5.225622° |
| 그 뒤0.030082초 | 24 | 3.122058 | 5.195813° |

0.15초 이전에 탄약 추가 소모 없이 열/퍼짐이 계속 감소했다. 발사 중과 종료 후를 합쳐 지연 내 감소76샘플을 관찰했고 `finished.reason=early_cooling_reproduced`다. 일반적으로 장착시간도 포함하는 활동 getter지만, 이 검사는 장착2초 뒤 발사 시0으로 갱신됨을 확인했으므로 해당 관찰 구간에서는 마지막 발사를 기준으로 한다.

- 이 별도 프로세스에만 `AbilitySystem.DisableGameplayCues=1`을 적용했다. 엔진 `GameplayCueManager::ShouldSuppressGameplayCues()`의 공식 CVar 경로이며, NullRHI의 미생성 Niagara/Decal로 인한 오류를 퍼짐 진단과 분리했다. 에셋/Config에 저장하지 않았고 사용자 에디터에는 적용하지 않았다. **Cue/사운드/렌더/네트워크 검증은 제외**다.
- `Saved/Automation/R6SpreadDelayBefore_20261009/index.json`,16:27:53: stock `Project.Maps.PIE` 성공1/실패0/보고서경고0/notRun0, 종료0. 이것은 진단용 PIE가 실행됐다는 결과이지 퍼짐 지연 기능 통과나 수정 후 회귀 통과가 아니다.
- 전체 로그의 Error/Fatal/ensure0. 초기 로드 경고는 GA_Weapon_AutoReload의 `/Script/LyraGame` import1개와 Manny PoseAsset14개이며 보고서경고0과 구분한다. 이번에 이 경고를 고치거나 전체 에셋이 경고 없이 로드됐다고 보고하지 않는다.
- 보호11파일(C++3, Config2, Git인덱스1, Integration맵/PawnData2, Rifle instance/Fire/Reload3)의 SHA256 불변, 에셋 저장0이다.

**최소 변경안 — 적용 전 승인 필요**

1. `Source/DreamCatcher/Weapon/Lyra/DCLyraRangedWeaponInstance.cpp`의 `AddSpread()`에서 `LastFireTime = GetWorld()->GetTimeSeconds()`를 기록한다. 기존 발사 성공 경로/열·퍼짐 곡선/배율/활동 시계를 유지한다.
2. 기존 `Source/DreamCatcher/Tests/DCWeaponFoundationAutomationTests.cpp`에0.15초 같은 양의 지연 이전에는 회복하지 않고, 추가 발사 시 지연이 다시 시작되며, 이후 원본 냉각 곡선을 적용하는 좁은 회귀 검사를 추가한다. 새 범용 프레임워크/헤더/API는 만들지 않는다.
3. 사용자가 C++ 빌드한 뒤 기존 무기/장비 검사와 위 실제 라이플 진단을 반복한다. 현재는 **수정 전 재현까지만 완료**, 코드 적용/빌드/수정 후 검증은 미실행이다.

**대안과 동작 차이:** 원본 실제 동작을 완전히 유지하려면 수정하지 않고 ‘설정된0.15초가 발사 후 회복을 막지 못한다’를 제한으로 남길 수 있다. 제안안은 설정 지연을 작동시키는 원본 대비 보완이므로, 지연보다 짧은 간격의 연사에서는 기존보다 냉각이 줄고 열/퍼짐이 더 누적될 수 있다. 그 차이를 숨겨 원본과 완전히 동일하다고 보고하지 않는다. 발사간격0.12/탄약/수동·자동재장전/오디오/에셋 값은 변경 범위에서 제외한다. 현재 원본 이식이 불가능해서 대체하는 상황이 아니라 **확인된 원본 결함의 보완 여부를 선택하는 상황**이다.

R0~R6 전체 감사의 나머지 수명/배율/카메라·표면 비교는 유지하고 R7은 아직 착수하지 않는다. R 수동재장전은 사용자 현재 미재현/원인 미확정으로 관찰 보류, 사운드는 사용자 통과 상태를 유지한다.

### 2026-10-09 R6 회복지연 코드 반영 — 작성 당시 기록

> 아래 미검증/빌드 대기는 작성 당시 상태다. 후속 사용자 빌드와 실행은 다음 ‘회복지연·실무기 수명 검증 통과’ 절을 따른다.

사용자가 위 최소 변경안을 승인하여 다음 **C++2파일만** 변경했다. AGENTS.md와 이 명세도 상태를 갱신했다.

- `DCLyraRangedWeaponInstance.cpp::AddSpread()`: `LastFireTime = GetWorld()->GetTimeSeconds();`와 설명주석 추가. 성공한 Commit 뒤 기존 AddSpread 호출 경로에서 회복 대기를 다시 시작한다. 원본의 Heat/Spread 곡선, 회복조건의 `>` 및 Tick당 냉각 계산, 배율/FirstShotAccuracy, 기반 TimeLastFired 활동시계는 변경하지 않았다. 헤더/API/Config/에셋/발사속도/탄약/재장전/사운드 변경도 없다.
- `DCWeaponFoundationAutomationTests.cpp`: 기존 fixture/합성 곡선 helper를 재사용한 `DreamCatcher.R6.Weapon.SpreadRecoveryDelay` 검사1건(65줄)을 추가했다. native transient 무기에만 지연0.15초와 합성 곡선을 넣는다. 월드시간1초로 시작해 LastFireTime이0인 기존 코드가 우연히 통과하지 않게 한다. 발사 후0.10초는 퍼짐8° 유지, 추가 발사 후0.10초는9° 유지, 추가 발사0.16초 뒤 Tick에서는8.88°, 다음0.05초 Tick에서는8.78°를 기대한다. 뒤 두 값은 부동소수 오차를 허용해 비교한다. 기존 SpreadAndAttenuation/ItemTagCost는 그대로다.

**정적 확인만 완료:** 엔진 FTestWorldWrapper::TickTestWorld의 시간 진행과 기존 반영 속성 타입/API를 확인했다. 신규 게임 코드3줄과 신규 테스트 블록을 각각 제거한 메모리 비교에서 기존 내용과 동일했다(개행 정규화). 보호12파일(Config2/Git인덱스1/기반CPP·Ranged헤더·발사Ability CPP3/맵·PawnData2/실무기 instance·Fire·Reload3/DLL1) 해시 불변, diff 공백 오류0. 빌드·패키징·Unreal 실행·에셋 저장은 하지 않았다.

**현재 판정:** 코드 작성 완료이나 **UHT/C++ 빌드, 신규 검사 실행, 실제 라이플 수정 후 회복지연은 미검증**이다. 수정 전 PIE 보고서는 재현 근거로만 유지한다. 원본 대비 의도된 차이는0.15초 지연이 실제 작동하여 연사 중 냉각이 줄고 열/퍼짐이 더 누적될 수 있다는 점이다. 수치 재밸런싱이나 재장전 규칙 수정으로 범위를 넓히지 않았다.

**다음 확인:** 사용자가 Development Editor/Win64 빌드 → 별도 프로세스에서 `DreamCatcher.R6.Weapon`(신규 포함3건)과 기존 R5 장비/타입 회귀 → 이전 `R6SpreadDelay` 실제 라이플 관찰 재실행. 회복지연 내 열/퍼짐 유지와 지연 이후 회복을 확인한 뒤 완료 판정한다. Cue를 끈 진단은 계속 렌더/오디오/복제 검증과 구분한다. R0~R6 나머지 결합 회귀 후 R7로 가는 순서는 유지한다.

### 2026-10-09 16:44 KST 회복지연·실무기 수명 검증 통과

**빌드/회귀:** 사용자 UBT 로그16:36:54 시작,12.49초 `Succeeded`, 수정 CPP2개 컴파일과 DLL16:37:06/4,390,912bytes 갱신을 확인했다. `Saved/Automation/R6SpreadDelayRegression_20261009/index.json`16:38:25는 신규 `SpreadRecoveryDelay`와 기존 `SpreadAndAttenuation`/`ItemTagCost`, Equipment1/TeamColor1/NativeRegistration4의 **성공9/실패0/보고서경고0/notRun0**이다. 프로세스 종료0, Error/Fatal/ensure0. 초기 B_LyraGameInstance의 `/Script/LyraGame` import 경고1은 보고서경고0과 별도로 기록한다.

**실제 라이플 회복지연:** 수정 전과 동일한 임시 `R6SpreadDelay/probe.py`/Integration맵/실제 장착 Rifle/6발 조건으로 실행했다. `Saved/Logs/R6SpreadDelay-After1.log`16:39:28, 설정0.150000006초/탄약30→24이며 다음을 관찰했다.

| 마지막 발사 후 경과 | 열 | 퍼짐각 | 결과 |
|---|---:|---:|---|
| 0초 | 6.25 | 7.457666° | 마지막 발사 |
| 0.148285초 | 6.25 | 7.457666° | 지연 이전 값 유지 |
| 0.157990초 | 6.211184 | 7.429597° | 첫 냉각 관찰 |
| 0.509386초 | 4.805599 | 6.413202° | 이후 냉각 지속 |

지연 내 냉각은 수정 전76샘플에서 **수정 후0샘플**이다. 수정 전 마지막 발사의 열3.242385/퍼짐5.282824°보다 수정 후가 커진 것은 사전에 설명한 연사 중 냉각 방지의 동작 차이이며 곡선/발사간격을 재조정하지 않았다. `R6SpreadDelayAfter_20261009/index.json`16:39:37은 PIE 성공1/실패0/보고서경고0/종료0이다. **회복지연 항목 통과**이며 R6의 모든 배율/반동/표면/시각/복제를 완료한 것은 아니다.

**다음 감사 항목의 실제 수명 검사:** 새로운 C++ 도구 없이 기존 공개 QuickBar/Equipment/ASC/Controller/GameMode API를 임시 `Saved/Diagnostics/R6SpreadDelay/lifecycle_probe.py`에서 호출했다. 저장된 맵/에셋을 바꾸지 않고 별도 PIE의 임시 상태만 조작했다. 정상 순서의 `Saved/Logs/R2R5RifleLifecycle-PIE2.log`16:44:30에서 다음 assertion들이 통과했다.

1. 실제 Rifle4발 발사30→26, 빈 슬롯으로 전환: Equipment0, Ability9→6, 기존 무기Actor 무효화, 발사/재장전/NoFiring/InputBlocked 태그0.
2. 같은 Item으로 재장착: 탄약26 유지, 새 Equipment1개, Instigator가 동일 Item, 무기Actor 수 복구, Ability9개. 새 InventoryItem 생성으로 탄약이 초기화된 것과 구분했다.
3. 다시 사격26→24 도중 UnPossess: Equipment0/Inventory0/Ability6, 추적태그4종0. 입력은 새 Pawn 생성 전에 해제했다.
4. 이전 권한 Pawn을 파괴해 EndPlay/ASC 해제를 끝낸 뒤 GameMode.RestartPlayer: 기존과 **같은 ASC**, 다른 Pawn/새 Item30발, Equipment1/Inventory1/Ability9, Instigator 일치.
5. 새 Pawn에서30→27 사격 가능, 입력해제 후 추적태그0/장비1/Ability9 유지.

`Saved/Automation/R2R5RifleLifecycleOrdered_20261009/index.json`16:44:45는 **성공1/실패0/보고서경고0/notRun0**, 종료0이며 Error/Fatal/ensure0이다. 이 범위에서 중복 Ability 부여와 남은 무기/추적태그가 관찰되지 않았다. 내부 모든 delegate/held 배열을 직접 검사한 것이나 임의 입력 차단·사망 경로 검사를 완료한 것으로 확대하지 않는다.

**실패 이력과 범위 교정:** 첫 `R2R5RifleLifecycle-PIE1.log`/`R2R5RifleLifecycle_20261009/index.json`16:42:57은 **실패1**이다. 진단에서 이전 권한 Pawn을 남긴 채 새 Pawn을 생성하여 `DCPawnExtensionComponent.cpp:144`의 `ensure(!ExistingAvatar->HasAuthority())`를 유발했다. 원본 `LyraPawnExtensionComponent::InitializeAbilitySystem()`에도 동일 검사가 있고, 주석은 이전 Avatar가 늦게 제거되는 경우를 client 지연 상황으로 한정한다. `HandleControllerChanged()`는 owner가 존재하면 actor info를 refresh할 뿐 UnPossess만으로 Avatar를 해제하지 않는 것도 양쪽 소스에서 확인했다. 따라서 이 첫 실행은 supported authority 수명 순서 검사가 아니며 Python의 finished/pass 문자열이나 종료0으로 성공 처리하지 않는다. **게임 코드/ensure 변경 없이 진단 순서만 이전 Pawn 파괴→새 Pawn 생성으로 교정**했다. client 지연 중 늦은 정리의 검증은 별도로 남는다.

**보존과 한계:** 두 실제 PIE는 이 프로세스에만 Cue비활성/NullRHI/NoSound를 적용했다. 렌더/오디오/물리키/네트워크는 미검증이며, 초기 AutoReload LyraGame import1/PoseAsset14 경고는 남는다. 이번 턴 C++/Config/실에셋/Git인덱스/DLL14파일 해시 불변, 에셋 저장0, 빌드/패키징 실행0. 기존 사운드 사용자 통과와 R키 현재 미재현/관찰 보류는 유지한다.

**현재 완료 범위와 다음:** R1 기동/실패태그 보완, R6 회복지연, R2/R5 실제 재장착·조종해제·권한 Pawn 순차 교체의 이 범위는 통과다. R0~R6 전체 완료나 R7 착수로 승격하지 않는다. 다음은 **R3 실제 플레이어의 피해/치료/면역·사망→장비/Avatar 정리→레벨재시작 결합 검사**를 기존 API/한정 임시 진단으로 수행한다. R4/R6 카메라·이동/공중 배율·반동/표면 비교도 남는다. 새 게임 코드 변경이 필요해지면 원본 근거/차이/정확한 범위를 제시하고 승인받으며, 지금 추가 사용자 빌드는 필요 없다.

### 2026-10-09 16:58 KST R3 플레이어 피해·면역·사망·재시작 결합 검증 통과

앞선 R2/R5 수명 검사 다음으로 사용자 승인한 R3 결합 진단을 수행했다. 기존 GE/ASC/HealthComponent/Character/GameMode의 공개 기능만 호출했고 게임 C++/Config/에셋을 변경하지 않았다. 진단은 `Saved/Diagnostics/R3HealthLifecycle/probe.py`이며 새 C++ 도구나 범용 프레임워크를 만들지 않았다. 사용자의16:37:06 DLL로 실행했고 빌드·패키징은 하지 않았다.

**실제 Integration 플레이어의 정상 경로** — `Saved/Logs/R3HealthLifecycle-PIE2.log`,16:54:56~16:55:07 KST:

| 항목 | 확인 결과 |
|---|---|
| 피해/회복 | 기존 GE_Damage_Basic_SetByCaller로100→75, GE_Heal_SetByCaller로75→85, 과회복은100 상한 |
| 면역 | 임시 Gameplay.DamageImmunity1 중25피해를 받아도100, 태그 제거 뒤25피해는75. 이후100으로 회복 |
| 사격 중 사망 |30→27 사격 도중 치명피해로HP0. DeathStarted, MovementMode=None/캡슐NoCollision, 사격 태그0 |
| 사망 중 입력/중복 |0.5초 더 발사 Action을 유지해도 탄약27 유지. HP0에서 추가25피해 후에도 로그의 DeathStarted/Finished/RestartQueued는 각각1회 |
| 종료/정리 | 설정 Duration8초 뒤 Finish. 약8.009초에서 Inventory0, 무기Actor 제거, Ability9→6, 추적태그6종0. 약8.109초에 이전 Pawn 파괴 확인 |
| 레벨재시작 | 설정 RestartLevelDelay2초 후 기존 GameMode가 OpenLevel. 새 월드에서HP100/탄약30/장비1/Inventory1/Ability9/추적태그0 |
| 재시작 후 입력 |0.4초 무입력 대기 중 탄약30 유지, 새 발사 Action으로30→27. 해제 후 태그0/장비1/Ability9 유지 |

추적태그는 Status.Death.Dying/Dead, Event.Movement.WeaponFire/Reload, Ability.Weapon.NoFiring, Gameplay.AbilityInputBlocked다. 원본 HealthSet의 일반 DamageImmunity 분기와 원본 DeathAbility의 SurvivesDeath 제외 취소/Exclusive_Blocking 전환은 소스와 대조했다. SelfDestruct의 면역 예외, 모든 cheat/상태 조합과 네트워크 복제를 이번에 모두 검증한 것은 아니다.

`Saved/Automation/R3HealthLifecycleReloadSafe_20261009/index.json` **16:55:30 성공1/실패0/보고서경고0/notRun0**, 종료0. 전체 로그 Error/Fatal/ensure0. 이것은 테스트 플레이어의 원본 GE/사망 흐름과 **기존 프로젝트 GameMode의 종료 후2초 레벨재시작** 연결 검증이다. Lyra 원본의 전체 respawn/Experience 경로를 이식했다는 뜻은 아니다.

**구형 적과의 호환 입력 경계** — `ADCEnemyCharacter`는 여전히 비GAS `UDCHealthComponent`를 사용하고, 공격은 `UGameplayStatics::ApplyDamage()`를 호출한다. 플레이어 `TakeDamage()`는 ASC 없는 source에 TargetASC를 임시 source로 사용하고 기존 `LegacyDamageGameplayEffectClass`로 연결한다. 이 입력 경계를 실제 Integration Pawn에서 별도 확인했다.

- 공격자 대역은 ASC가 없는 기존 PIE GameMode Actor이며, `ApplyDamage(Player,5,Controller=None,Causer=대역,DamageType)`를 호출했다. 실제 적을 생성하거나 AI/공격 주기를 실행한 것이 아니다.
- 실제 Pawn의 호환 GE는 `/Game/DreamCatcher/GAS/Effects/Combat/GE_DC_Damage`다.5피해는HP100→95/반환값5, 임시 면역태그1 중 같은 호출은HP95 유지/반환값0이었다. 이 GE나 호환 source 경로를 원본 Execution으로 이번에 바꾸지 않았다.
- `Saved/Logs/R3LegacyBridge-PIE2.log`16:58:41, `Saved/Automation/R3LegacyBridgeExistingActor_20261009/index.json` **16:58:47 성공1/실패0/보고서경고0/notRun0**, 종료0/Error/Fatal/ensure0. **ASC 없는 source의 기존 ApplyDamage 수신·면역 경계만 통과**다. 적 AI/실제 적 사격/팀 판정/적 GAS 전환은 R10 및 실제 맵 검증에 남는다. 새 라이플이 기존 비GAS 적을 원본 GE로 정상 공격한다는 양방향 호환성도 이 결과만으로 보증하지 않는다.

**진단 실패 이력:** Health PIE1은 실제 사망/정리/맵 재로딩 뒤, GC된 이전 Pawn의 Python wrapper를 native IsValid에 넘겨 변환 오류가 발생했고 보고서 실패1이다. 진단만 travel 전에 경로로 다시 조회해 파괴를 관찰하고 새 월드로 넘어간 후에는 이전 wrapper를 native API에 넘기지 않도록 교정했다. Legacy PIE1도 Python에서 미노출된 BeginDeferredActorSpawnFromClass 호출로 적용 전에 실패1이었다. spawn 대신 ASC 없는 기존 Actor를 사용해 경계를 재검사했다. 두 실패를 성공으로 숨기거나 이를 고치려고 게임 코드를 수정하지 않았다.

**보존/한계:** 관련 C++5/Config2/Git인덱스1/실에셋7/DLL1 총16파일 해시 불변, 에셋 저장0. 두 성공 실행은 프로세스 한정 GameplayCue비활성/NullRHI/NoSound/Action 주입이며 사망 연출·카메라 화면·소리·물리키·복제 재검증은 아니다. 성공 로그에도 초기 AutoReload LyraGame import1/Manny PoseAsset14 로드 경고는 남는다. 기존 사용자 사망 연출/사운드 확인은 유지하되 이번 자동 검사로 확대하지 않는다.

**완료/다음:** R3의 이번 플레이어 피해/치료/면역/사망/정리/레벨재시작 결합과 기존 ApplyDamage 수신 경계는 통과했다. 기존 R1 보완/R6 회복지연/R2·R5 재장착·권한 Pawn 교체 결과도 유지한다. 다음은 **R4/R6 조준·이동·공중 퍼짐 배율과 카메라 연결**, 이어 반동 중복·거리/표면/엄폐 비교다. 자동 수치 검사와 사용자 화면/조작감 검사를 구분하며 신규 수정이 필요하면 원본 근거/동작 차이/범위를 제시한다. 추가 사용자 빌드는 필요 없고 R7은 아직 착수하지 않는다.

### 2026-10-09 17:17 KST R4/R6 상태 배율 점검 — 원인·제안 이력

> 아래는 수정 전 진단/제안이다. 사용자가 승인한 뒤 코드와 에셋을 반영했으며, 현재 상태는 다음17:34 절의 사용자 빌드 대기다.

새 게임 수정 없이 원본 소스/기존 원본 asset 조사와 현재 실제 Pawn을 비교했다. `Saved/Diagnostics/R4R6AimSpread/probe.py`를 이용한 `R4R6AimSpread-PIE1.log`17:08:58~17:09:13에서 다음을 확인했다. 실제 계수는 원본 B_WeaponInstance_Rifle 조사값과 같은 Aim0.65/Standing0.8/Crouch0.6/Air1.6이며, 전이속도는 모두5, 정지 기준20+20cm/s였다. 원본 에셋 파일은8/22 수정, 기존 조사로그는10/5 작성이다. 새 원본 키 조회는 아래 Source2에서 별도로 했다.

| 상태 | 실제 관찰 | 판정 |
|---|---|---|
| 정지/이동/정지 복귀 |0.8 → 이동600cm/s에서0.99686 → 0.8 | 통과 |
| 웅크림 요청 |Walking/속도0에서 요청했지만bIsCrouched=false,0.8 유지. 기대 raw0.8×0.6=0.48에 도달하지 않음 | **실패** |
| 점프/착지 |Falling84샘플, 배율최고1.578238, 착지 후0.8복구 | 공중 페널티/복귀 확인. 매 프레임 곡선의 완전 일치 비교는 아님 |
| Shoulder 유지 |State.Aim.Shoulder1, FOV70, raw0.52, MaxWalkSpeed300 | 안정 상태 연결 통과 |
| Shoulder 종료 |Hip FOV80/raw0.8/속도600, 조준태그0 | 통과 |
| Scope 탭/해제 |FOV40/raw0.52/속도300, 다음 입력을 유지해도Hip 복귀 유지 | 통과 |

위 raw 값은 `Debug_CurrentSpreadAngleMultiplier`다. 원본 `GetCalculatedSpreadAngleMultiplier()`의 첫발 정확도0 덮어쓰기를 포함한 최종 사격각/UI 표시를 검증한 것으로 보고하지 않는다. 웅크림/점프는 Character API를 직접 요청했고 이동/조준은 Enhanced Input Action을 주입했다. 물리 키/장치/실제 화면/복제와는 별개다. `R4R6AimSpread_20261009/index.json`17:09:26은 **실패1**이며 다른 상태 통과로 전체 성공 처리하지 않는다.

**확정한 원인/원본 근거**

- 엔진 Character.cpp::CanCrouch/Crouch는 Movement의 CanEverCrouch가 false이면 요청을 거절한다. 실제 로그17:09:03에 crouching disabled/NavAgentSettings 메시지가 있고, `R4R6Crouch-Inspect1.log`17:12:02에서 native CDO/BP CDO/실인스턴스 모두 `NavAgentProps.bCanCrouch=false`, `CanCrouch()=false`다. 현재 웅크림 배율 식이 잘못된 것이 아니라 해당 상태로 진입하지 못한다.
- 원본 `LyraCharacter.cpp:60~62`는 CanCrouch=true, bCanWalkOffLedgesWhenCrouching=true, CrouchedHalfHeight65를 설정하고 BaseEyeHeight80/CrouchedEyeHeight50을 사용한다. 현재는false/false/40/64/32다. 원본 OnStartCrouch/OnEndCrouch의 Status.Crouching1/0 hook도 대상 Character에는 없다.
- 현재 DA_DC_RifleInput.NativeInputActions는 Move/LookMouse/Aim/LookStick4개이며 InputTag.Crouch가 없다. 기존 `/Game/LyraMigration/BaseInput/Actions/IA_Crouch`는 이미 존재한다.
- `R4R6Crouch-InputSource2.log`/`InputTarget2.log`,17:17:22의 새 읽기 전용 조회: 원본/이식본 모두 Bool + InputTriggerPressed + Modifier없음이다. 원본 IMC_Default의 crouch 키는 **LeftControl**(추가 trigger없음), **Gamepad_FaceButton_Right**(추가 Pressed), 양쪽 모두 Modifier없음이다. 현재 IMC_Player에는 crouch 매핑이 없다. 첫 Source/Target 조회의 빈 목록은 UE5.8 deprecated Mappings를 읽은 오류이며, 실제 DefaultKeyMappings.Mappings를 읽은 Source2/Target2만 키 근거로 사용한다. 이는 진단 조회만 교정했으며 입력에셋은 변경하지 않았다.

**카메라 전환 특성 — 이번 변경 대상 아님**

Shoulder 진입 중FOV78.375→70.011에서도 raw0.8, 복귀 중72.180→79.985에서도raw0.52였다. 원본과 대상 모두 CameraModeStack.Insert(...,0)으로 새 모드를 앞에 넣지만 GetBlendInfo는 Last()의 태그/가중치를 반환한다. 이 소스와 실제 관찰이 부합하며, 전환 종료 전 배율이 연속적으로 보간된다고 보고할 수 없다. 로컬 원본과 같은 동작으로 남기며 원본과 다른 blend 계산을 몰래 추가하지 않는다. protected CameraModeStack의 runtime 직접조회는 거부되어 실제 weight값 자체는 미검증이다. 안정 상태 연결 통과와 전이 곡선/벽 가림 검증을 구분한다.

**다음 제안: 원본 웅크리기 연결 복구 묶음(아직 미적용)**

1. `Source/DreamCatcher/DreamCatcherCharacter.h/.cpp`2파일: 원본 OnStartCrouch/OnEndCrouch의 Status.Crouching1/0 + Super 호출을 이식한다. 새 클래스/범용 검사도구는 만들지 않는다.
2. 기존 `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn`의 이동/시야 기본값5개를 원본의true/true/65/80/50으로 설정한다. 이 Blueprint의 Hero DefaultInputMappings에 아래 새 crouch 전용IMC도 추가한다. 공유 Character 생성자의 모든 Pawn 기본값을 일괄 변경하지 않는다.
3. 기존 Integration `DA_DC_RifleInput`에 원본 이식 IA_Crouch/InputTag.Crouch1개를 추가한다. 신규 Integration `IMC_DC_RifleCrouch`에는 원본2키와 각 trigger만 넣는다. 기존 BaseInput.IA_Crouch/IMC_Default/공용 IMC_Player는 변경하지 않고 전체 IMC_Default를 함께 활성화하여 다른 발사/조준 키를 중복 바인딩하지 않는다.
4. 사용자 C++ 빌드 후 새 crouch Action/태그/물리상태/raw배율0.48/카메라 높이 및 기존 Shoulder/Scope/사격·재장전 회귀를 확인한다. 게임패드는 설정을 보존하지만 실장치 검증은 기존대로 보류한다.

**대안/범위와 동작 차이:** 원본 전체 Character 생성자를 가져오면 속도·가속·회전·충돌·기존 맵까지 영향이 넓다. 반대로 CanCrouch flag만 켜면 입력/Status태그/원본 자세값은 누락된 채 남는다. 제안은 원본 crouch 동작·값은 재사용하되 **설정 위치를 native 생성자 대신 통합 테스트 BP로 한정**하는 단계적 적용이다. 이식 불가여서 새 동작을 만드는 우회가 아니다. CrouchHalfHeight40→65, BaseEyeHeight64→80/CrouchedEyeHeight32→50이므로 standing/crouch 카메라 구도와 충돌 높이가 바뀔 수 있다. 기존 standing capsule42/96와 다른 이동/충돌/발사/재장전/사운드/카메라 blend 계산은 유지하므로 원본 Character 전체와 완전히 같다고 주장하지 않는다. R4/R6 상태 조건에 필요한 R8의 일부 선행이며 전체 AnimBP/LinkedLayer 시각 검증은 R8에 남긴다.

이번 턴은 임시 진단·읽기·문서만 수행했다. C++/Config/에셋 저장0, 빌드·패키징0, 보호13파일 해시 불변. 마지막 inspect의 성공1은 설정 조회 성공일 뿐 crouch 동작이 고쳐진 것이 아니다. 현재 **실제 누락 확인/복구안 승인 대기**이며 R7은 보류한다. 벽/가림·반동/거리/표면/엄폐·최종 첫발/UI/장치/복제 검증을 임의 완료하지 않는다.

### 2026-10-09 17:34 KST 원본 웅크리기 연결 반영 — 적용 당시 기록

> 아래 빌드 대기는 당시 기록이다. 후속 사용자 빌드/실행 및 새로 확인된 Pawn 교체 경계는 다음18:07 절을 따른다.

사용자가17:17 제안 묶음을 승인하여 코드2파일과 에셋3개를 한정 변경했다. C++ 빌드·패키징·수정 후 PIE는 실행하지 않았다.

**코드:** `DreamCatcherCharacter.h/.cpp`에 원본 OnStartCrouch/OnEndCrouch 선언과 구현을 추가했다. ASC가 있으면 Status.Crouching을1/0으로 맞추고 원본과 같은 순서로 Super를 호출한다. 이름/ASC getter/태그 namespace만 대응했다. 헤더4줄/CPP20줄이며 기존 공용 생성자 기본값, 이동/카메라 계산과 발사·재장전·오디오 코드는 변경하지 않았다. 아직 빌드하지 않았으므로 새 hook의 런타임 동작은 미검증이다.

**Unreal 내부 에셋 변경(Integration 폴더 한정):**

| 에셋 | 적용 내용 |
|---|---|
| 기존 BP_DC_RiflePawn | NavAgentProps.bCanCrouch=true, bCanWalkOffLedgesWhenCrouching=true, CrouchedHalfHeight65, BaseEyeHeight80/CrouchedEyeHeight50. Hero에 새 crouch IMC priority1/bRegisterWithSettings=true 추가 |
| 기존 DA_DC_RifleInput | NativeInputActions에 기존 `/Game/LyraMigration/BaseInput/Actions/IA_Crouch`/InputTag.Crouch1개 추가 |
| 신규 IMC_DC_RifleCrouch | 원본 LeftControl 및 Gamepad_FaceButton_Right만 연결. gamepad 추가Pressed/ActuationThreshold0.5/AlwaysTick=false 보존 |

IA_Crouch 자체의 Bool/Pressed 규칙은 그대로다. 새 매핑의 추가 trigger는 새IMC를 outer로 가진 객체로 만들고 원본 private subobject를 참조하지 않음을 저장 전/새 프로세스에서 확인했다. 전체 IMC_Default 활성화나 공용 IMC_Player 수정은 없다.

**보존 확인:** 기존 NativeInputActions4개/AbilityInputActions5개/기존 Hero IMC3개는 앞부분 전체 직렬화 비교에서 유지됐다. NavAgentProps는 bCanCrouch만 달라졌고, standing capsule42/96, MaxWalkSpeed600/MaxWalkSpeedCrouched300/MaxAcceleration2048/BrakingDecelerationWalking1800/AirControl0.2와 Hero 전환flag3개도 그대로다. 원본 생성자에 있는 crouch/시야값을 테스트 BP에 한정 배치하는 승인된 구성 차이이며 기존 플레이어 전체를 변경하지 않았다. 카메라 높이 변화와 전체 Character/애니메이션 동등성 미검증이라는 제안의 한계는 유지한다.

**자동화/실행 근거:**

- `Scripts/Editor/r4_crouch_setup.py`: 기존 SubobjectData 템플릿 조회와 기존 격리/SaveOnCompile/SCC/ensure 안전검사를 재사용한다. prepare는 신규IMC가 이미 있으면 덮어쓰지 않고 거부한다. verify는 성공 prepare 로그의 baseline과 대조하고 저장하지 않는다. 새 C++ 도구/API 확장은 없다.
- `Saved/Logs/R4Crouch-Prepare2.log`, **17:32:54 KST**: Pawn Blueprint Compile과 메모리 설정/기존값/원본 매핑 비교 후 위3개만 저장, 종료0. `Saved/Logs/R4Crouch-Reload1.log`, **17:34:58**: 별도 프로세스 재로드 후 기본값/키/trigger outer/기존 입력·설정 보존 검증 통과, 저장0/종료0. 두 성공 로그 Error/Fatal/ensure0. 기존 Manny PoseAsset 경고는 미해결로 남긴다.
- Inspect1은 필수 격리옵션을 빠뜨려 안전검사에서 중단됐고, Prepare1은 Blueprint Hero를 CDO에 생성된 실제 컴포넌트로 조회하다 실패했다. Blueprint 추가 컴포넌트는 기존 SubobjectData SCS 템플릿 경로로 조회하도록 교정했다. 두 실패는 저장 전이며 실제 에셋 준비 실패본/새 파일을 만들지 않았다. 성공 근거와 구분한다.
- 보호13개 실제파일(Config2/Git인덱스/DLL/공용·원본입력/기존플레이어/PawnData/통합맵/무기·카메라CPP)의 해시 불변. 스크립트 안에서도10개 보호 package의 동반 파일을 포함한53개 경로 상태를 비교했다. 기존 dirty/staged 변경과 Git인덱스는 보존했다. 공용입력/기존플레이어/맵은 저장하지 않았다.

**현재 판정/다음:** 코드 작성과 에셋 저장·새 프로세스 재로드 검증까지 완료, **사용자 C++ 빌드와 수정 후 PIE는 미완료**다. 이 에셋 자동화는 기존16:37:06 DLL로 실행했으므로 새 Status.Crouching hook을 검증한 것이 아니다. 사용자 Development Editor/Win64 빌드 후 실제 crouch Action의 토글/해제·capsule65↔96·태그1↔0·raw배율0.48↔0.8, 기존 Shoulder/Scope/점프/발사·재장전/수명 회귀를 실행한다. 사용자는 카메라 높이·웅크림 시각을 확인하며 수동 에셋 설정은 추가로 필요 없다. 게임패드/전체 R8 시각·복제와 R7/벽·반동/거리·표면 검증을 완료로 올리지 않는다.

### 2026-10-09 18:07 KST 웅크림·최종 수치 검증 / Pawn 교체 crouch 태그 잔류

> 아래는 수정 전 재현 및 제안 기록이다. 후속 사용자 승인으로 다음 절의 수명 동기화 코드1파일을 반영했으며 현재는 사용자 빌드 대기다.

사용자 빌드는17:38:54 시작/28.75초 Succeeded, Character CPP와 UHT 반영 및 DLL17:39:22/4,391,936bytes 갱신을 확인했다. 이번 턴은 기존 검사/임시 PIE 진단/문서만 진행했고 C++ 빌드·패키징·게임 코드/Config/에셋 저장0, 보호14파일 해시 불변이다.

**웅크림 및 기존 회귀 통과:** `R4Crouch-PostBuild1.log`17:43:43~45에서 직접 Crouch() 호출 대신 실제 NativeInputActions의 IA_Crouch를 Enhanced Input으로 주입했다. 첫 입력은 bIsCrouched=true/Status.Crouching1/캡슐반높이65/raw0.480673, 입력을 놓아도 토글 유지, 다음 입력은false/태그0/96/raw0.799360으로 돌아왔다. 이동/정지/점프/착지와 Shoulder70/Scope40/Hip80 및속도600복구도 통과했다. `R4CrouchPostBuild_20261009/index.json`17:44:06은 기존 R5/R6검사9+PIE1 **성공10/실패0/보고서경고0/notRun0**이다. 물리키/게임패드/실제 화면은 별도다.

**실제 라이플 최종 수치/엄폐 검사 통과:** 임시 `Saved/Diagnostics/R4R6AimSpread/combat_probe.py`는 기존 Integration 표적을 PIE 안에서만 이동하고, 임시 WeakSpot 물리재질과 Cube 벽을 사용했다. 맵/재질 에셋은 저장하지 않았다. 원본 발사 Ability/실제 무기 AbilitySource/HitResult/GE 경로로 발사했다.

| 조건 | 실제 체력 감소 |
|---|---:|
| 플레이어 기준650cm 위치의 일반 표적 |12 |
|3600cm 위치의 일반 표적 |6 |
|650cm 위치·Gameplay.Zone.WeakSpot 물리재질 |18 |
|3600cm 위치·같은 약점재질 |9 |
| 표적 앞에 막는 벽 |0 |
| 벽 충돌 제거 후 |12 |

매번 탄약1발을 소비했고 여섯 발 뒤24→30 수동재장전도 통과했다. 이는 거리곡선의1/0.5 구간 및 약점1.5가 실제 발사에 적용된 근거다. 명목 표적 위치가 정확한 Impact-Source 거리라는 뜻은 아니며2800/2801 경계 모든 점/모든 물리표면이나 실제 적의 뼈별 약점을 검증한 것은 아니다.

- 기존 W_Reticle_Rifle을 화면에 추가하지 않고 임시 생성해 InitializeFromWeapon/HasFirstShotAccuracy/ComputeSpreadAngle의 native 조회 경로만 사용했다. 냉각완료 ADS는Heat0/Base2.5/raw0.52지만 **FirstShot=true/최종각0**이다. 한 발 뒤Heat1/Base3.661328/raw0.52/FirstShot=false/최종각1.90389를 확인했고 다음 냉각 후0으로 복구했다. R7 HUD 표시/픽셀 투영/위젯 수명 통합 완료와는 다르다.
- 임시벽의 카메라 가림은 캐릭터와의 거리313.3295→161.8578cm로 당겨지고, 제거1.2초 후313.2881cm로 복구됐다. 여러 벽모양/예측 feeler 전체/화면 클리핑의 완전 검증은 아니다.
- 완료 근거는 `R6CombatNumbers-PIE5.log`18:05:16~27 및 `R6CombatNumbersWarm_20261009/index.json`18:05:42 **성공1/실패0/보고서경고0/notRun0**, 종료0/Error/Fatal/ensure0이다. 초기 기존 import/PoseAsset15개 경고는 남는다. Cue비활성/NullRHI/NoSound이므로 CameraShake/오디오/시각 반동 중복 검증으로 확대하지 않는다.
- PIE1~3은 진단의 Python enum/속성 경로 문제로 발사 전에 실패했다. 엔진 선언의 ECollisionResponse ScriptName=CollisionResponseType, 물리재질 override가 BodyInstance에 있음을 확인해 교정했다. PIE4는 최초 대기1초가 원본 장착 초기열6의 냉각시간보다 짧아 첫 cold-shot 검사만 실패했다. 첫 대기를2초로 바꾼 PIE5가 전체 통과이며 게임의 원본 초기열/곡선/회복값은 수정하지 않았다. 실패 보고서를 성공으로 재해석하지 않는다.

**실제 미해결 결함 — 새 Pawn에 남는 Status.Crouching**

`Saved/Diagnostics/R6SpreadDelay/lifecycle_probe.py`의 기존 정상 순차 교체에 `-DCCrouchHandoff`를 추가해 웅크림 상태에서 확인했다. `R4Crouch-Handoff1.log`18:06:51~52에서 이전 Pawn의 bIsCrouched=true/태그1 → UnPossess/이전 Pawn 파괴 → 같은 ASC의 새 Pawn 준비 후 **bIsCrouched=false/태그1**이었다. 장비/인벤토리 각1과 Ability9는 정상이다. 따라서 태그를 소비하는 애니메이션/규칙이 실제 자세와 다른 상태를 읽을 수 있다. `R4CrouchHandoff_20261009/index.json`18:07:08은 **실패1**이며 진단 API 오류가 아니라 관찰된 실제 상태 불일치다. 이전의 ‘서 있는 상태’ Pawn 교체 통과를 취소하지 않고 이 추가 조건을 분리한다.

원본 LyraCharacter의 OnStart/EndCrouch는 실제 crouch 전환에서만 태그를1/0으로 맞춘다. OnAbilitySystemUninitialized는 HealthComponent만 해제하고 InitializeGameplayTags도 movement-mode 태그만 초기화하므로 Status.Crouching은 포함하지 않는다. 대상 Character에도 ASC 재사용 시 crouch 상태를 새 Pawn에 맞추는 처리가 없다. 원본 구현의 이 수명 경계를 보완해야 하며 원본에 이미 있는 코드를 놓쳤다고 단정하지 않는다.

**최소 변경안(아직 미적용):** `Source/DreamCatcher/DreamCatcherCharacter.cpp`1파일에서 (1) OnAbilitySystemInitialized에 현재 IsCrouched() 기준 Status.Crouching1/0 동기화, (2) OnAbilitySystemUninitialized에 해당 Avatar 해제 범위의 태그0 정리를 추가한다. 오래된 Pawn이 이미 연결된 새 Avatar 태그를 지우지 않도록 소유관계를 확인한다. 기존 Start/EndCrouch, 물리자세/캡슐, 키/Ability/카메라/발사/재장전 규칙과 에셋은 그대로다. 원본 대비 추가 동작은 ASC 재사용 경계의 Pawn 상태 태그 동기화뿐이다. 대안인 교체 직전 강제 UnCrouch는 실제 자세/캡슐을 바꾸고, ASC 전체 재생성/태그 초기화는 지속 상태까지 영향을 주므로 선택하지 않는다. 사용자 승인/빌드 후 해당 handoff와 정상 토글 회귀를 재실행한다. 새 C++ 검사 프레임워크나 에셋 재작업은 필요 없다.

**남은 마감 조건 고정:** 사용자는 이번 최신 빌드의 웅크림/카메라 구도·조준/연사 반동 화면을 **아직 확인하지 않았다고 답했다**. 현재 R7 전 마감은 (1) 확인된 crouch 태그 잔류1건 수정/해당 회귀, (2) 사용자 화면·조작감/반동 확인이다. 첫발/기본 거리·약점·엄폐/카메라 가림 수치의 통과 범위를 새 도구를 만들며 반복 확장하지 않는다. 문제가 없으면 R7 연결 제안으로 넘어간다. 장치/복제/전체 실맵·전체 R8 시각 등 기존 별도 범위를 새 선행 조건으로 추가하지 않되 전체 완료라고 속이지 않는다. 이번이 무조건 마지막 검증이라고 약속하지 않는다.

### 2026-10-09 crouch 태그 수명 동기화 코드 반영 — 작성 당시 기록

> 작성 당시 빌드/검증 대기는 아래18:26 후속 검증으로 해소됐다. 사용자 화면 확인은 별도로 남는다.

사용자가 위 최소안을 승인하여 `Source/DreamCatcher/DreamCatcherCharacter.cpp`1파일에16줄을 추가했다. 기존 변경인 OnStartCrouch/OnEndCrouch는 보존했다.

- **초기화:** OnAbilitySystemInitialized에서 `ASC->GetAvatarActor() == this`일 때 Status.Crouching을 `IsCrouched() ? 1 : 0`으로 맞춘다. 서 있는 새 Pawn이 이전 Pawn의태그1을 물려받지 않으며, 이미 웅크린 Pawn에 새 ASC를 연결하는 경우도 실제 상태를 반영한다.
- **해제:** OnAbilitySystemUninitialized에서 ASC가 존재하고 Avatar가 null 또는 this일 때만 태그0을 설정한다. 현재 PawnExtension은 Avatar를 먼저 비우고 이 callback을 broadcast한 뒤 component의 ASC 참조를 지우므로 null도 정리 대상이다. Avatar가 다른 Pawn이면 건드리지 않아 오래된 callback이 새 Pawn의 태그를 지우지 않도록 한다.
- 원본과의 추가 차이는 위 두 수명 경계의 Pawn 상태 태그 동기화다. 실제 UnCrouch를 호출하지 않아 캡슐/자세를 강제 변경하지 않고, ASC 재생성이나 전체 태그 초기화도 하지 않는다. Health 초기화/해제, 기존 crouch hook, 입력/Ability 부여/카메라/발사/재장전 규칙은 그대로다.

정적 점검에서 이번 두 추가 블록을 제거한 CPP가 작업 전 내용과 동일했다(개행 정규화 SHA256). 보호12파일(헤더/PawnExtension/Config/Git인덱스/DLL/통합Pawn·입력·IMC·맵/무기·카메라CPP)은 해시 불변이고 diff 공백 오류0이다. 새 헤더/에셋/Config/검사 프레임워크는 추가하지 않았다. **빌드·패키징·Unreal 실행·에셋 저장은 하지 않았으며 수정 후 handoff 성공은 아직 미검증**이다.

다음은 사용자 Development Editor/Win64 빌드 확인 후 기존 `-DCCrouchHandoff`의 동일 실패 조건과 정상 crouch Action 토글을 재검사한다. 오래된 실패 로그를 수정 후 결과로 바꾸지 않는다. 사용자의 화면/카메라 구도·조준/연사 반동 확인도 여전히 남는다. 두 마감 조건에서 문제가 없으면 R7 연결안을 제시하며, 이미 통과한 첫발/거리/약점/엄폐/가림 수치 검사를 새로 확대하지 않는다.

### 2026-10-09 18:26 KST 태그 잔류 수정 검증 통과 — 이번 자동 감사 마감

사용자 실제 컴파일은 UBT backup 로그의18:20:53 시작/8.65초 Succeeded이며 Character.cpp가 컴파일되고 DLL18:21:01/4,391,936bytes가 갱신됐다. 최신 Log.txt의18:21:11/1.28초 성공은0 action/Target up to date 재확인이므로 실제 컴파일 기록과 혼동하지 않는다. Codex는 빌드하지 않았다.

| 마지막 확인 항목 | 새 실행 결과 |
|---|---|
| 동일 crouch Pawn 교체 실패조건 | `R4Crouch-Handoff2.log`18:24:14, 새 Pawn bIsCrouched=false/Status.Crouching0. 같은 ASC, Ability9/장비1/Inventory1,30→27 재사격 뒤에도태그0 |
| handoff 보고서 | `R4CrouchHandoffFixed_20261009/index.json`18:24:30 성공1/실패0/보고서경고0/notRun0 |
| 정상 crouch Action 토글 | `R4Crouch-ToggleFixed1.log`18:26:20~21, 태그1/캡슐65/raw0.480661, release 후 crouch 유지, 재입력 후태그0/96/raw0.799338 |
| 토글 보고서 | `R4CrouchToggleFixed_20261009/index.json`18:26:24 성공1/실패0/보고서경고0/notRun0 |

두 프로세스 종료0/Error/Fatal/ensure0. 기존 초기 import/PoseAsset15개 로드 경고는 그대로다. 실제 순차 authority Pawn 교체에서 잔류 결함이 해소됐다는 근거이며 client 지연/복제의 모든 callback 순서를 검증한 것은 아니다. 토글은 기존 진단에 `-DCCrouchToggleOnly`로 필요한 구간만 실행했고 이미 통과한 무기 수치/거리/표면 검사를 반복 확장하지 않았다.

이번 턴 게임 코드/Config/에셋 저장·빌드·패키징0, 관련 source/header/PawnExtension/Config/Git인덱스/DLL/통합에셋11파일 해시 불변이다. 기존 Cue비활성/NullRHI/NoSound/Action 주입 조건을 유지했으므로 시각·소리·물리 Ctrl키/게임패드 검증과는 다르다.

**완료 판정:** R7 전 마감으로 정한 **이번 R0~R6 자동 감사 범위는 완료**한다. 알려진 crouch 태그 잔류1건도 수정 후 회귀를 통과했다. 범용 도구/새 검증 항목을 계속 추가하지 않는다. 이전에 통과한 초기화·체력/면역/사망·장비/재장착·퍼짐/거리·약점/엄폐/가림 수치는 그대로 유지한다. 프로젝트의 모든 장치/복제/실맵·전체 R8 시각·패키징까지 완료했다는 의미는 아니다.

**현재 남은 것은 사용자 화면 확인2개:** (1) 통합 맵에서 Left Ctrl 웅크림/해제 자세와 카메라 높이, (2) 조준/연사 시 비정상적 이중 반동·떨림 여부. 마지막 사용자 답변은 ‘아직 확인하지 않았습니다’이며 이후 정상이라는 보고가 없어 완료로 추정하지 않는다. 이 확인이 정상이고 다른 문제가 없다면 **R7-1/2 원본 ReticleHost·라이플 조준점/탄약 표시 연결**의 원본 근거·차이·정확한 코드/에셋 범위를 제시하고 승인받는다. 기존0.10/10월9일 R7 제안은 참고 이력이지 자동 적용 권한이 아니다. 새 R7 코드/에셋은 이번에 변경하지 않았다.

### 2026-09-19까지의 이전 구현 이력 — 보존용

아래 완료 기록은 기존 사용자 보고 및 소스 점검에 따른 이력이다. 이번 문서 수정으로 빌드·PIE를 다시 검증한 것은 아니다.

| 항목 | 기존 작업 상태 | 이번 원본 이식에서의 취급 |
|---|---|---|
| 기존 0~6단계 | 기능 구현 완료 이력 | 원본 대체 검증은 별도. R1~R6에서 재검토·교체 |
| 기존 7-1 실제 퍼짐 기반 Reticle | 사용자 구현 완료 | 비교용 유지 후 R6~R7 원본으로 대체 |
| 기존 7-A Linked Anim Layer | Idle 연결까지만 완료 | 전체 완료 아님. R8에서 원본 계층으로 교체 |
| Lyra 크로스헤어 시각 에셋 | Migrate 진행됨 | 원본 Widget 및 UI C++ 기능 연동 완료와 구분 |
| 원본 UI C++·Reticle Widget 이식 | 중단 상태, 완료 아님 | R5~R6 의존성 준비 후 R7에서 재개 |
| R1 보조 타입·공통 의존성 | 태그 관계표, EffectContext/Globals, AbilityCost, GlobalAbilitySystem, AssetManager/GameData, 메시지·플러그인 등 준비 | R1-1부터 다시 시작하지 않음. 개별 기능 전체 검증과는 구분 |
| R2-1 AbilitySet·GameplayAbility·ASC | 원본 기반 본체 이식, OnSpawn·실행 그룹·추가 비용 및 기존 플레이 회귀 검증 통과 | 이번 단계 범위 완료. R2 전체 완료는 아님 |
| R2-2 초기화 연결 | 원본 PawnExtension 이식, PlayerState 부여 경로 연결, Character 알림 전환, Hero/PawnExtension의 GameplayReady 및 반복 PIE·사격 확인 | 초기화 기본 범위 검증 완료. 전체 플레이 회귀·실제 Pawn 교체 등은 아래 미검증 항목과 구분 |
| Hero·입력·카메라 | Hero를 통한 ASC 초기화가 실제 실행됨. 입력·카메라는 Legacy 옵션 기본값 true로 기존 Character 경로를 보존 | R2-3 입력 및 R4 카메라 전환은 아직 미완료 |

당시 재개 지점은 **R2-3 원본 입력 전환**이었다. 이 표와 아래 날짜별 인계는 과거 기록이며, 현재 시작점은 위 **2026-10-05 인계**를 따른다.
R1-1, R2-1 테스트, GameInstance 등록 및 PawnExtension 이식을 처음부터 다시 진행하지 않는다. 과거 미검증 항목은 이후 검증 근거와 대조하되, 모든 항목이 자동으로 해결됐다고 간주하지 않는다.

### 2026-09-19 이전 인계 — R2-2 확인 범위

이 2026-09-19 기록은 당시 소스·설정과 사용자 실행 로그를 읽어 작성했다. 당시 Codex가 새 빌드·Editor·PIE를 실행하거나 에셋을 수정한 것은 아니다.

| 확인 항목 | 실제 근거 | 판정 |
|---|---|---|
| C++ 빌드 | 2026-09-19 02:10 KST경 UBT 로그: `DreamCatcherEditor Win64 Development`, `Result: Succeeded` | 빌드 성공 기록 확인 |
| 원본 초기화 상태 전이 | 02:23:24와 02:23:29 KST의 두 PIE에서 PawnExtension과 Hero가 각각 Spawned → DataAvailable → DataInitialized → GameplayReady 출력 | Hero 실행 및 초기화 기본 연결 검증 완료 |
| ASC/Avatar 및 기존 전투 연결 | 같은 두 실행에서 R2-A PawnAvatarSet/Activated, 라이플 장착 Count: 1, HealthComponent의 ASC 연결(100/100), HUD Hip·CameraMode 활성화 기록 | 해당 연결의 실행 확인 |
| 기존 사격 | 두 실행 모두 Rifle Fire Cue와 연속 RangedFire 기록 | 사격 기본 회귀 확인. 기록된 명중은 StaticMeshActor이므로 적 피격 검증으로 해석하지 않음 |
| PIE 반복 | 첫 PIE 종료 후 두 번째 PIE에서 같은 초기화·사격이 다시 동작, 두 번째 종료 시 월드 Cleanup 기록 | PIE 종료/재실행 기본 확인. 같은 PlayerState를 유지한 실제 Pawn 교체 테스트와는 다름 |

- 실행 로그: `Saved/Logs/DreamCatcher.log`. 본문 UTC `2026.09.18-17.23.24` / `17.23.29`는 로컬 KST 2026-09-19 02:23:24 / 02:23:29이다. 로그 회전 후에는 백업 파일도 확인한다.
- 02:19경 Hero가 실행되지 않던 이전 PIE에는 `ASC is not initialized` 입력 경고가 있었다. 이후 위 두 PIE에서는 Hero 전이·ASC 연결·사격이 확인되며 같은 경고가 관측되지 않았다. 이전 실패 기록을 최신 상태와 혼동하지 않는다.
- **미검증:** 이번 변경 후 이동·점프·Shoulder/Scope의 전체 조작 회귀, 적의 실제 체력 감소·사망, 플레이어 사망 흐름, 같은 PlayerState를 유지한 Pawn 교체, 모든 Ability/GE/Attribute의 중복 여부 정밀 검사, 멀티플레이·패키징. 이전 9월 17일 회귀 통과를 이번 변경 후의 재검증으로 취급하지 않는다.
- LevelScript의 `EnableInput` 관련 경고는 최신 PIE에도 남아 있다. 초기화 성공과 구분하며, R2-3에서 입력 주체를 정리할 때 현재 테스트 맵/Level Blueprint 입력을 확인한다.
- 이 완료 판정은 **R2-2의 초기화 기본 연결 범위**다. R2 전체 완료, 원본 입력·카메라 전환 완료, Blueprint 그래프/설정 전체 검증을 의미하지 않는다.

### R2-2에서 실제 변경한 연결과 과도기 차이 — 2026-09-19 기록

- `DCPawnExtensionComponent.h/.cpp`: `LyraPawnExtensionComponent` 원본에서 UPawnComponent·InitState·PawnData 복제·Controller/Input 알림·ASC 연결/해제·TagRelationshipMapping 전달을 이식했다. 기존 PawnData AbilitySet 부여/회수와 `PawnDataGrantedHandles`는 제거했다.
- `DCPlayerState.h/.cpp`: 원본 `LyraPlayerState::SetPawnData()` 및 getter·복제·준비 이벤트를 이식했다. 기본 AbilitySet 부여는 이 경로를 사용하며 기존 `PlayerStateAbilitySets` 테스트 배열과 중복 설정하지 않는다.
- `DreamCatcherGameMode.h/.cpp`: 원본 LyraGameMode의 PawnData 기반 클래스 선택과 지연 생성/FinishSpawning 흐름을 이식했다. GameMode가 PlayerState의 SetPawnData를 한 번 연결하고 PawnExtension에도 같은 데이터를 공급한다.
- `DreamCatcherCharacter.h/.cpp`: 원본 LyraCharacter처럼 PossessedBy/OnRep 알림과 ASC 수명 이벤트로 연결했다. 직접 ASC 초기화 함수는 제거했고 기존 조준 구독은 초기화/해제 이벤트로 이동했다. HealthComponent는 자체 구독을 유지하여 중복 초기화하지 않는다.
- `DCHeroComponent.h/.cpp`: 기존 원본 이식본의 초기화 경로를 사용한다. `bUseLegacyPlayerInput=true`, `bUseLegacyCameraMode=true` 옵션으로 기존 Character 입력/카메라와의 중복 바인딩을 막는다. 각각 R2-3/R4에서 해당 원본 연결을 검증한 뒤 전환한다.
- **Experience 과도기:** 원본은 Experience를 통해 데이터를 선택하지만 현재 그 체계가 없으므로, 이미 이식된 DCAssetManager의 기본 PawnData를 직접 사용한다. `Config/DefaultGame.ini`의 `DefaultPawnData`는 `/Game/DreamCatcher/GAS/Pawn/DA_DC_PlayerPawn.DA_DC_PlayerPawn`이다. Experience 연결 시 임시 공급 시점을 교체하며, 원본 이식 불가 판정은 아니다.
- **장비/조준 호환:** 기존 EquipmentManager가 사용하는 해제 직전 이벤트·재진입 방지·구독 해제 API는 R5까지 유지한다. 현재 Scope는 Ability 종료 후에도 GE가 남을 수 있어 ASC 해제 시 조준 정리를 유지한다. 원본 SurvivesDeath 제외 취소·입력/Cue 정리 흐름은 보존한다.
- **BP 호환:** 기존 비템플릿 GetPawnData와 Class Defaults 접근을 유지한다. 저장된 PawnData가 GameMode 공급값과 다르면 오류로 처리하며 조용히 덮어쓰지 않는다. 테스트 BP의 Hero 실행은 로그로 확인했지만 내부 그래프·모든 프로퍼티 값까지 직접 검수한 것은 아니다.

### 2026-09-17 이전 인계 — 보존할 완료 이력

당시 작업 브랜치는 `GAS-System`이었다. 아래는 당시 사용자 실행 보고와 소스·설정·로그 확인에 따른 기록이다. 해당 시점에 Codex가 빌드·Editor·PIE를 직접 실행하거나 Blueprint 내부 그래프를 전부 확인한 것은 아니다.

| 검증 | 확인 내용 | 근거 수준 |
|---|---|---|
| R2-A OnSpawn, Avatar 연결 전 부여 | Added: Avatar NONE → PawnAvatarSet → Activated | 사용자 실행 및 로그 확인 |
| R2-B OnSpawn, Avatar 준비 후 부여 | Before Give → Added: Avatar READY → Activated → After Give. 별도 활성화 요청 없이 Give Ability로 검증 | 사용자 실행 및 로그 확인 |
| R2-C 실행 그룹 | Replaceable 취소, Blocking의 배타적 Ability 차단, Independent 실행, Blocking 종료 후 재활성화 | 사용자 실행 및 로그 확인 |
| R2-D AdditionalCosts | 0에서 거부, 3 충전 후 Commit마다 3→2→1→0 차감, 소진 후 거부 | 사용자 실행 및 정상 수량 로그 확인 |
| 기존 플레이·재시작 | 이동·조준·사격·타깃 피격·HUD/무기 표시, PIE 종료/재시작 및 테스트 수량 초기화 | 사용자가 전체 성공 보고 |
| CommonUI 선언 | `.uproject`에 CommonUI 추가. 이후 빌드 로그에서 기존 직접 의존성 미선언 경고 없음 | 파일·기존 빌드 로그 확인 |
| R2-2 등록 기반 | `[R2-2] InitStateOrder=OK, UIManager=OK`가 반복 PIE에서 출력. 기존 플레이 유지 | 소스·설정·로그 및 사용자 실행 보고 |

- 당시 확인한 UBT 로그는 2026-09-17 01:01 KST경의 `DreamCatcherEditor Win64 Development`, `Result: Succeeded`이다. 최신 기록은 위 2026-09-19 인계를 따른다.
- `Saved/Logs/DreamCatcher.log`에서 2026-09-17 01:12 KST경 등록 성공 로그가 여러 PIE에 걸쳐 확인되었다. 로그 본문의 시각은 UTC이므로 로컬 시각과 구분한다.
- 앞선 테스트 로그는 현재 로그 또는 `Saved/Logs/DreamCatcher-backup-*.log`에 남는다. 파일 회전으로 위치·줄 번호가 바뀔 수 있다.
- 수량 출력이 0으로 고정되던 문제는 사용자가 수정했다. 이후 실제 수량 로그는 정상이다. 그래프를 직접 보지 않았으므로 그 수정의 구체적 원인은 확정하지 않는다.

### 2026-09-17 채팅에서 추가·연결한 항목

- `Source/DreamCatcher/AbilitySystem/Abilities/DCAbilityCost_PlayerTagStack.h/.cpp`: Lyra의 PlayerTagStack 비용 원본 이식. 클래스·include 이름을 DC로 변경하고 검사·차감 로직은 유지.
- `Source/DreamCatcher/Player/DCPlayerState.h/.cpp`: 원본 `StatTags`, Add/Remove/GetCount/HasStatTag API 및 `DOREPLIFETIME` 등록 추가. 기존 `GameplayTagStack`을 재사용하며 별도 가짜 비용 로직은 만들지 않음.
- `Config/DefaultGameplayTags.ini`: 비용 테스트용 `Test.R2.CostCharge` 등록. 이 카운터는 PlayerState의 StatTags이며 ASC의 소유 태그 수량과 별개임.
- `DreamCatcher.uproject`: CommonUI 플러그인 직접 의존성 명시.
- `Source/DreamCatcher/System/DCGameInstance.h/.cpp`: `UCommonGameInstance`를 부모로 사용하고 원본 `ULyraGameInstance::Init()`의 네 InitState 등록을 부분 이식. 등록 순서·UIManager 확인 로그 추가.
- `Source/DreamCatcher/UI/Subsystem/DCUIManagerSubsystem.h/.cpp`: LyraUIManagerSubsystem 원본 이식. 명칭 변경 및 PlayerController 명시적 include 외 원본 흐름 유지.
- `Config/DefaultEngine.ini`: `GameInstanceClass=/Script/DreamCatcher.DCGameInstance`, `LocalPlayerClassName=/Script/CommonGame.CommonLocalPlayer` 연결.
- 기존 `BP_DC_PlayerState_GAS_Test`, `BP_DC_PlayerController_GAS_Test`에 테스트 AbilitySet 및 임시 키/조회/출력 그래프를 연결. 테스트용이므로 생산용 입력 구조로 간주하지 않음.

테스트 에셋은 `Content/DreamCatcher/GAS/Test/`에 보존되어 있다.

- `GA_DC_R2_OnSpawnTest`, `AS_DC_R2_OnSpawnTest`
- `GA_DC_R2_OnSpawnLateTest`
- `GA_DC_R2_GroupReplaceable`, `GA_DC_R2_GroupBlocking`, `GA_DC_R2_GroupIndependent`
- `GA_DC_R2_CostTest`

안내한 임시 키는 K(B), J·1·2·3(C), H·4·5(D)였다. 실제 키·대기 시간·그래프 값은 사용자가 바꿀 수 있으므로 다음 작업에서 에셋을 확인하며, 로그만으로 그래프 전체가 원본/안내와 동일하다고 단정하지 않는다.

### 당시 부분 이식·보류 범위와 이유 — 2026-09 기록

- **GameInstance:** 원본 전체 이식이 아니라 InitState 등록 부분 이식이다. `LyraGameInstance.cpp`의 `HandlerUserInitialized()`는 LyraLocalPlayer의 설정 로딩을 호출하고, 디버그 암호화/세션 이동 코드도 포함한다. 해당 설정·로그인 기능의 의존성과 이번 Pawn 초기화 범위를 구분해 보류했다. 이식 불가능 판정은 아니다.
- **LocalPlayer:** 원본 `UCommonLocalPlayer`를 현재 직접 사용한다. `ULyraLocalPlayer` 전용 팀 관찰·설정·오디오 연동까지 이식한 상태는 아니다. 관련 기능을 도입할 때 원본 의존성을 재검토한다.
- **UIManager:** `UCommonGameInstance::AddLocalPlayer()`가 구체적인 UIManager와 CommonLocalPlayer를 요구하므로 선행 이식했다. UI Policy·Root Layout·원본 HUD/Reticle 완료를 의미하지 않는다.
- **비용 검증:** PlayerState 태그 스택의 기본 검사·차감 경로를 검증했다. OnlyApplyCostOnHit 분기, Inventory 아이템 탄약, 멀티플레이 예측/복제까지 검증한 것은 아니다. 해당 소비처는 R5~R6 등에서 별도 확인한다.

### 당시 채팅에서 이어갈 작업과 주의점 — 2026-09-19 기록

1. **당시 다음 작업은 R2-3 입력 전환이었다.** 초기화 기본 연결을 다시 작성하지 않는다는 원칙은 유지하며, 현재 시작점은 2026-10-05 인계를 따른다.
2. 현재 Character의 BindLegacyAbilityActions는 Started/Completed/Canceled, 원본 Hero 바인딩은 Triggered/Completed를 사용한다. IA_Aim·IMC·InputConfig 태그를 확인하지 않고 Legacy 옵션만 끄지 않는다. 확정된 우클릭 규칙을 유지한다.
3. Hero의 기본 IMC와 PlayerController/Character/Level Blueprint 입력 경로를 함께 대조하고, 한 경로만 활성화한다. 입력 매핑 제거·추가 및 중복 바인딩을 검증한다.
4. 카메라의 Legacy 옵션은 R4 전환 검증 전까지 유지한다. 입력 전환과 무관하게 두 옵션을 한꺼번에 끄지 않는다.
5. TagRelationshipMapping의 ASC 전달은 코드에 연결되었다. 실제 데이터의 차단·취소·필수 태그 및 수명 검증은 R2-4에서 수행한다.
6. EquipmentManager의 과도기 API와 비템플릿 getter는 현재 소비처가 남아 있으므로 임의 제거하지 않는다. 새 원본 수명 경로로 옮긴 뒤 정리한다.

당시에는 R2-3 입력 전환 후 R2-4 태그 관계·수명 검증으로 이어가는 순서였다. 현재 시작점은 2026-10-05 인계를 따르며, R0~R14의 기능별 검증 범위는 유지한다.
당시 코드·설정에 사용자 미커밋/부분 스테이징 변경이 있었으며, 기존 사용자 변경을 보존한다는 원칙은 유지한다. 당시에는 사용자 직접 구현·Codex 안내 방식이었고, 현재는 위 2026-10-05의 작업 묶음 승인·자동화 운영을 따른다.

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
`R`은 과거 단계와 구분하기 위한 새 작업 식별자다. 각 `R번호-세부번호`로 완료 범위를 추적한다. 2026-10-05 승인에 따라 관련 코드·에셋과 선행 의존성은 범위를 먼저 설명한 하나의 작업 묶음으로 진행할 수 있다.

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
| R0 | 정책·현재 상태·작업 단위 기록 | 원본 교체 범위와 다음 작업이 문서로 고정 | 2026-10-05 자동화·작업 분담·마감 기준 갱신. 전체 원본 감사는 아님 |
| R1 | GAS 보조 타입 및 의존성 준비 | 원본 기반 클래스 이식에 필요한 타입·모듈 목록과 독립 파일 준비 | 기반 이식/빌드 확인. 10/9 감사 보완2건은16:15까지 GameData 단일 재저장·실패태그 복구 후 비에디터 맵 로드/재로드/기존8건 통과. 패키징/전체 플레이 검증과 구분 |
| R2 | GAS·Pawn 초기화·입력 공통 기반 교체 | 원본 ASC/Ability/AbilitySet, OnSpawn, 초기화·입력 동작 | 기존 초기화/서있는 Pawn 교체/사망·재시작 통과. crouch 태그 수명 보완은 사용자 빌드/18:24 동일 handoff 회귀 통과. 이번 자동 감사 마감. client 지연/복제는 별도 |
| R3 | 체력·데미지·사망 교체 | 원본 Attribute/Context/Execution/Health/Death 경로 동작 |10/9 실제 플레이어 피해/치료/면역/사망8초/정리/2초뒤 레벨재시작·재사격 통과. ASC 없는 대역의 기존 ApplyDamage 수신/면역 경계 통과. 실제 적 AI·양방향 적GAS/팀 통합은 별도 |
| R4 | 원본 카메라·조준 연결 | 원본 카메라와 확정 우클릭 규칙의 통합 | crouch Action/태그/캡슐·조준/FOV·점프·임시벽 가림 수치 및 태그 잔류 보완 후 토글 회귀 통과. 이번 자동 감사 마감, 사용자 자세/구도·반동 화면 확인 대기. 장치는 별도 |
| R5 | Inventory·QuickBar·Equipment·라이플 데이터 | 실제 Inventory 아이템에서 장비와 UI 데이터가 연결 | 실제 라이플/Item Instigator·사격후 재장착 탄약 보존·Ability 회수/복구·사망/권한 Pawn 교체 정리 통과. 전체 UI/복제·client 지연 수명은 별도 |
| R6 | 원본 퍼짐·발사·GameplayCue·반동 | 원본 무기 계산·탄약 비용·발사·연출 동작 | 발사/탄약/사운드/0.15초 지연/상태배율 및18:05 실제 최종첫발·12/6/18/9 피해·엄폐0/해제12/재장전 통과. CameraShake/시각 반동은 사용자 미확인. R은 현재 미재현·관찰 보류 |
| R7 | 원본 Reticle·HUD·명중 표시 | 원본 Widget/C++/Slate가 같은 무기·아이템을 표시 | UIManager·UIExtension 등 선행 의존성 준비. 원본 Reticle·HUD의 무기/아이템 통합은 미완료 |
| R8 | 전체 AnimBP·Linked Layer·재장전 | Idle 이외 이동·조준·발사·재장전·무기 계층 연결 | AnimInstance/CharacterMovement 지원 C++ 선행 준비. 기존 Idle 연결 외 전체 원본 계층 통합은 미완료 |
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
| OnSpawn·Ability 기반 | `Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp`, `Source/LyraGame/AbilitySystem/LyraAbilitySystemComponent.cpp` | 원본 기반 본체 이식 및 두 OnSpawn 경로 검증 통과. 상단 2026-09-17 인계 참조 |
| 추가 비용 | `Source/LyraGame/AbilitySystem/Abilities/LyraAbilityCost_PlayerTagStack.cpp`, `Source/LyraGame/Player/LyraPlayerState.cpp` | PlayerTagStack 및 PlayerState API 이식, 기본 검사·차감 검증 통과 |
| 초기화 순서 등록 | `Source/LyraGame/System/LyraGameInstance.cpp` | InitState 등록 부분 이식 및 등록 순서 확인. 원본 GameInstance 전체 이식은 아님 |
| UIManager 선행 의존성 | `Source/LyraGame/UI/Subsystem/LyraUIManagerSubsystem.h`, `Source/LyraGame/UI/Subsystem/LyraUIManagerSubsystem.cpp` | 원본 이식 및 서브시스템 생성 확인. UI Policy·Root Layout 연동은 미완료 |
| 태그 관계표 | `Source/LyraGame/AbilitySystem/LyraAbilityTagRelationshipMapping.h`, `Source/LyraGame/AbilitySystem/LyraAbilityTagRelationshipMapping.cpp` | 두 파일 본문 확인. R1-1 원본 |
| Pawn 초기화 | `Source/LyraGame/Character/LyraPawnExtensionComponent.h`, `Source/LyraGame/Character/LyraHeroComponent.cpp`, `Source/LyraGame/Player/LyraPlayerState.cpp` | 2026-09-19 원본 기반 초기화 연결 및 Hero/PawnExtension GameplayReady 확인. 과도기 차이는 상단 인계 참조 |
| EffectContext | `Source/LyraGame/AbilitySystem/LyraAbilitySystemGlobals.cpp`, `Source/LyraGame/AbilitySystem/LyraGameplayEffectContext.h` | 원본 Ability가 확장 Context를 요구. Config/호출부를 함께 준비 |
| 무기 아이템 연결 | `Source/LyraGame/Equipment/LyraQuickBarComponent.cpp`, `Source/LyraGame/UI/Weapons/LyraWeaponUserInterface.cpp` | QuickBar의 Instigator 할당과 UI의 non-null 조건 확인 |
| 퍼짐 계약 | `Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp`, `Source/LyraGame/UI/Weapons/LyraReticleWidgetBase.cpp` | 원본 기본각×배율, DC 최종각 반환 및 화면 좌표 차이 확인 |
| 무기 갱신 | `Source/LyraGame/Weapons/LyraWeaponStateComponent.cpp` | 원본 Tick 존재. DC Equipment Tick과 중복 금지 |
| 회복 지연 | `Source/LyraGame/Weapons/LyraRangedWeaponInstance.h`, `Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp`, `Source/LyraGame/Weapons/LyraWeaponInstance.cpp` | 원본의 LastFireTime/TimeLastFired 불일치를 실제 라이플에서 재현.10/9 승인 후 AddSpread의 LastFireTime 갱신만 보완,0.15초 지연 전 유지/후 회복·신규 회귀 통과 |
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

**2026-10-09 최신 재개 기준:** 아래 장문 인계는 이전 검사 이력을 포함한다. R6 피해·사망/마우스 수정의 사용자 C++ 빌드(03:03:30/34.98초 성공)와 새 `BP_DC_RifleDeathTarget` 생성·통합 맵 표적1개 교체·별도 재로드(03:14:24)까지 통과했다. 원본 Rifle GE와 입력배율1.0도 유지된다. `prepare-target`을 다시 실행하거나 이미 검증된 작업 때문에 재빌드를 요구하지 않는다. 실제 PIE 초기화와 사용자 명중·피해·원본 Death 시작/8초 뒤 종료/제거·마우스 방향/감도를 구분해서 확인한다. 공용 표적·원본 GE 계산·기존 입력 에셋은 그대로이며 복제/래그돌은 미검증이다. AutoReload import 및 Manny PoseAsset 경고·Cue 등록 충돌·Standalone 실패도 남아 있다. 자세한 근거는 위 10월 9일 새 표적 연결 섹션을 따른다.

### 시작 규칙

1. 루트 AGENTS.md와 이 명세를 읽는다. 별도 채팅의 완료 보고 없이 앞 단계가 끝났다고 추정하지 않는다.
2. 해당 R 세부 단계의 실제 파일·빌드/Editor 검증 기록을 확인하고 미완료 선행 작업이 있으면 먼저 설명한다.
3. 사용자에게는 존댓말로 원본 근거·변경안·검증 범위를 먼저 설명한다. 승인된 코드·에셋 자동화를 우선하고, 사용자에게는 C++ 빌드·패키징과 자동화하지 못한 Editor 작업·짧은 플레이 확인을 전달한다.
4. 코드·Config는 작업 묶음의 승인 범위만 수정한다. 허용된 Unreal 내부 에셋 자동화도 대상 경로와 작업을 제한한다. 문서 갱신만 요청하면 문서만 수정하며, C++ 빌드·패키징은 실행하지 않는다.
5. 확인 못 한 Blueprint 그래프·기본값은 추정하지 않는다. 가능한 경우 Unreal 내부 API로 읽기 전용 검사하고, 그 방법으로 검증할 수 없는 부분에만 사용자 확인·스크린샷을 요청한다.
6. 관련 R 세부 단계와 선행 의존성은 범위를 먼저 밝힌 승인 묶음으로 진행할 수 있다. 단계별 완료 수준은 따로 기록한다. 병렬 채팅·에디터·자동화 프로세스가 같은 파일·에셋을 동시에 수정하지 않는다.
7. 원본을 그대로 가져오지 못하는 부분은 해당 변경 안내 전에 원본 근거·구체적 제약·대안·동작 차이를 설명한다. 확인 부족이면 미검증으로 남기고 다음 확인 절차를 제시한다.

### 다음 채팅에 붙여 넣을 시작 문구

```text
DreamCatcher의 AGENTS.md와 docs/specs/gas-lyra-migration.md를 읽고
명세의 2026-10-06 최신 인계와 10월 5일 승인된 자동화·마감 기준부터 확인해 주세요.
R4-3 PC 조준 입력·정상 종료·사망·입력 차단·Possession 해제 검증은 통과했습니다.
R5-1 Inventory 기본 8개 파일은 사용자 빌드가 성공했고,
r5_inventory_automation.py로 새 테스트 에셋 생성·저장·별도 프로세스 재로드 후
초기 StatTags 7 → 증가 10 → 감소 6 → 조회 → 제거 검사가 통과했습니다.
임시 에디터 월드 검사이며 PIE·복제·장비·QuickBar 연결은 아직 미검증입니다.
R5-2 별도 DCLyra 장비 계층은 10월 5일 16:20 사용자 빌드가 성공했고,
테스트 에셋 4개 생성·재로드와 16:31 QuickBarLifecycle 독립 검사가 통과했습니다.
후속 R5-3/R6 선행 13개 파일은 16:51 사용자 빌드가 성공했고, 16:53 독립 검사 2건과 R5 회귀가 모두 통과했습니다.
17:00 원본 조사에서 발견한 CharacterParts 원본 3개 파일은 승인 후 DCLyra 이름으로 추가했습니다.
17:23 사용자 빌드와 17:26 클래스/Reflection/빈 CDO 확인, 17:28 기존 R5/R6 회귀 3건이 통과했습니다.
에셋·Config 묶음은 승인 후 원본 Seed 준비까지 진행했으나 Advanced Copy 저장 충돌/불완전으로 중단했습니다.
원본 Rifle/Content·Payload·Prepared는 부분 진단 결과이며, DreamCatcher 이식과 Config 적용은 아직 하지 않았습니다.
추가 TeamColor 코드는 테스트 API 교정 후 18:29 사용자 재빌드, 18:31 총 4건 검사가 통과했습니다.
실패 지점 주변 음악 컴포넌트/오디오 매크로는 개별 복사·저장·재로드에 성공했으나 대량 복사 충돌은 미해결입니다.
후속 승인된 Editor 전용 DCRifleMigrationTools 플러그인 7개 파일과 .uproject Editor 등록을 작성했습니다.
19:35 사용자 빌드, 19:38 NameGuards, 외부 Lyra 로드/preflight 및 오디오 2개 메모리/저장/재로드는 통과했습니다.
하지만 19:55~20:00 실제 B_Weapon+B_Rifle 비교에서 자식의 SK_Rifle/MI_Weapon_Rifle/ABP_Weap_Rifle 참조와 override 템플릿 소실을 확인했습니다.
후속 승인으로 원본 상속 컴포넌트 override 보존/복원/비교 C++와 transient Blueprint 테스트를 Editor 도구에 추가했습니다.
20:56 사용자 재빌드와 21:07 InheritedOverrides/NameGuards 2건 검사는 통과했고, 기존 손상본도 충돌 없이 불일치로 판정했습니다.
실제 Rifle 보존 시도는 21:27 Capture 후 Niagara 종속성 dirty 검사에서 복사 전에 중단됐습니다. 저장/재로드는 실행되지 않았습니다.
21:30 단순 로드 전후 조사에서 dirty 0→4(Niagara), 명시적 원본 B_Weapon/B_Rifle는 clean임을 확인했습니다.
그 정책은 21:53 사용자 빌드와 21:54 자동 검사 3건이 통과했습니다.
21:56~21:57 weapon_actor 보존 모드 메모리/저장/재로드 및 부모・참조・override 1개 비교까지 모두 성공했습니다.
성공 진단본은 원본 Lyra의 Rifle/Diagnostics/Explicit/WeaponActorPreserveSave_2155 아래 2개 에셋입니다. 전체 라이플 이식 완료는 아닙니다.
22:17 원본 핵심 16개 읽기 전용 로드는 모두 통과했고 원본 dirty 0, 종속 dirty는 기존 Niagara 4개뿐이었습니다.
후속 승인으로 정확한 16개 목록까지 정책 범위를 넓혔고 r5_rifle_core_stage.py와 순수 계약 테스트를 작성했습니다.
Python 순수 테스트 5건과 23:36 사용자 빌드, 23:38 MigrationTools 3건 회귀가 통과했습니다.
23:40 preflight는 통과했지만 core16 메모리 복사는 Cue의 B_Weapon 타입 불일치와 원본 B_Pistol dirty로 실패했습니다.
저장/Hero 핀 변경/prepare/verify는 실행하지 않았고 신규 core 에셋 파일은 없습니다. core16+Niagara4의 80개 파일 상태는 유지됐습니다.
23:58~23:59 읽기 전용 조사에서 WeaponAudioMacros의 B_Weapon cast와 WeaponAudioFunctions의 매크로 의존을 확인했습니다.
Advanced Copy 내부 ConsolidateObjects에는 제외 집합 밖 원본 자식 BP 재부모화/Compile 경로가 있습니다. Pistol을 dirty 허용 목록에 추가하지 마세요.
후속 사용자 승인으로 목적지 한정 복사/원본 BP 보호/공개 타입 조회/정확한 closure18 코드와 ScopedReferences 테스트를 작성했습니다.
Python 테스트 6건과 AST/diff 검사, 10월 6일 00:55 사용자 UHT/C++ 빌드는 통과했습니다.
00:57 도구 테스트는 InheritedOverrides/LoadDirtyPolicy 성공, NameGuards/ScopedReferences 실패로 2/4 통과입니다. 종료 코드 0은 성공을 뜻하지 않습니다.
NameGuards는 마운트 없는 DreamCatcher에서 /ShooterCore 유효성까지 요구했고, ScopedReferences는 EditorContext와 Commandlet 요구가 충돌해 본체 실행 전 실패했습니다.
00:59 별도 원본 Lyra inspect/preflight는 통과했고 원본 BP21개의 공개 타입 조회와 파일 상태 104개 유지, 에셋 쓰기0을 확인했습니다.
후속 사용자 승인으로 순수 이름/실제 마운트 검사 분리와 임시 테스트 실행 조건을 CPP 3개에서 교정했습니다.
실에셋 복사 API의 Commandlet/저장 보호는 유지하며, 임시 테스트만 별도 unattended/nullrhi Editor Automation에서 실행합니다.
01:43 사용자 재빌드는 성공했고 01:46 MigrationTools는 3건 성공/1건 실패입니다. NameGuards와 이전 실행 조건은 해결됐습니다.
ScopedReferences는 임시 BP 생성/Compile 후 첫 개별 복사에서 ReadOnly /Temp 목적지가 거부됐습니다. 실제 참조 치환/보호 검사는 아직 미검증입니다.
후속 승인으로 테스트 CPP 1개의 fixture를 새 /Game 진단 GUID 메모리 경로로 교정하고 사전 writable-root 검사를 추가했습니다.
원본 fixture RF_Transient, 자동 저장 금지, 기존 목적지 거부/파일 미생성 검사를 유지합니다. 이번에는 빌드/Unreal 실행을 하지 않았습니다.
02:11 사용자 재빌드와 02:12 MigrationTools 4건은 모두 통과했습니다(실패/경고0).
02:13 실제 closure18 memory에서 복사본 WeaponInstance CDO의 UberGraphFrame key mismatch ensure가 발생했습니다.
보조 도구 success=true/override 및 변경 전 18개 비교 통과만으로 전체 성공으로 취급하지 마세요. ensure 실패 게이트가 추가로 필요합니다.
승인된 Hero 입력 핀 변경 뒤 ReturnValue subtype 1개가 LyraCharacter로 바뀌어 스크립트가 저장 전 종료1로 중단했습니다.
GetTypedPawn의 DeterminesOutputType=PawnType 원본 근거를 확인했습니다. 기대값은 이 노드/핀 하나에만 좁게 적용해야 합니다.
원본 파일104개는 종료 후 대조에서도 불변이고 새 디스크 진단본은 없습니다. prepare/verify는 실행하지 않았습니다.
후속 사용자 승인으로 부모 우선 엔진 ReparentBlueprint/Compile, SuperStruct 직접 치환 거부, 실행 프레임 회귀 fixture와 진단 차단을 구현했습니다.
원본 GetTypedPawn node/class/입력값을 확인한 뒤 patched Base의 ReturnValue subtype 1개만 기대값에 반영합니다.
02:42 사용자 재빌드와 02:44 신규 EngineDiagnosticGate 포함 C++5건은 통과했습니다(실패/보고서 경고0).
02:45 실제 closure18 memory, 02:46 prepare 저장18개, 02:48 별도 verify/plan 모두 통과했습니다. ensure0, 직접참조 비교18개 missing/unexpected0, override1개 일치입니다.
원본 Lyra의 /Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247 아래18개가 검증된 준비본입니다. 과거 실패 폴더와 혼동하지 마세요.
원본/동반104개 및 저장본/동반72개 파일 상태를 유지했습니다. DreamCatcher 이식/활성 무기는 아직 미변경입니다.
다음 외부 의존성은1228개이며 원본 core/원본Hero 역참조0, 남은 ShooterCore2개는 Struct_UIMessaging과 GE_Damage_RifleAuto입니다.
대상 Game 경로114개가 겹치고 파일 해시는 전부 다릅니다. 이것만으로 실제 게임값 차이나 덮어쓰기 필요성을 단정하지 마세요.
후속 공용114개 조회/비교를 완료했습니다. 진단만 Spatialization 활성화, 원본464/대상460 파일 불변, asset writes0/ensure0입니다.
Unbound delegate 주소를 구분하면 조회 가능 속성113/114, text88/114, Texture pixel12/12, 직접참조100/114가 같습니다. 전체 동등성은 아닙니다.
SK_Rifle render LOD1/vertices11255/section1은 같지만 bulk 형상 전수 비교는 미검증입니다.
Struct_UIMessaging 멤버/멤버GUID/기본값은 같아도 struct GUID가 다릅니다. 기존 ADS 타입 하나로 연결 후 Compile/메시지 검증이 필요합니다.
원본 RifleAuto GE는 Instant/Source BaseDamage 스칼라 보정12입니다. SetByCaller 테스트 GE로 대체하지 마세요.
Circumference4파일/Redirect2개/Spatialization 정식 활성화는 사용자16:25 빌드 성공과16:28 신규 검사3건,16:30 실에셋 로드로 검증했습니다.16:32 기존 회귀4건도 통과했습니다(합계성공7/실패·보고서경고0).
Slate 옵션의 명시 초기화/UMG 전달만 승인 범위로 보강했고 나머지 원본 계산은 유지했습니다. 에셋 원본값OutsideRadius=true/Radius24와 클래스 기본false/Radius48을 구분하세요.
LyraGameState/ExperienceManagerComponent 및 실행 의존성은 원본 기반 코드 이식과 독립 검사를 통과했으나 활성 경로는 미전환입니다. B_LyraGameInstance/AnimInstance native 호환도 감사 대상입니다.
GameState/Experience를 빈 대체나 임의 cast 변경으로 연결하지 마세요. 기존114개/전체 Migrate/활성 무기는 아직 변경하지 않았습니다.
보고서 대형36개는 검증된 R5SharedAudit_20261006.zip에 복원 가능하게 보존 후 정리했습니다. 비교 스크립트는 ZIP도 읽습니다.
10/8 사용자재빌드23:28:13/9.70초성공을확인하고,이식core18＋보조2로드·Compile오류0후남은태그경고의INI행표기를교정했습니다. 새R5/Integration에플레이어/PawnData/Input/Controller/GameMode/R매핑/맵7개를생성해별도재로드와23:57:29 엔진기본Project.Maps.PIE검사1건을통과했습니다. 로그의실제라이플1개장착/InventoryItem Instigator검사후성공및종료해제를확인했습니다. 기존플레이어파일/공유114개는변경없고추가C++빌드/패키징은하지않았습니다. 다음은사용자새맵PIE의좌클릭/R/탄창소진/표적피해확인으로R6실사용을검증합니다. 원본발사Cue와기존테스트Cue가동일태그여서등록전환은아직미적용이며별도범위설명·승인후진행하세요. -game무화면실행은맵진입전GameData로드fatal로실패했으므로PIE성공을Standalone/패키징성공으로확대하지마세요. 원인미확정/원본GameData미변경입니다. 원본시각/조작/사격·재장전/복제와전체R5/R6/R7/R13완료는미검증이며추가도구확장보다실제연결을계속우선합니다.
범위는 일반 최대16 또는 정확한 closure18과 새 Rifle/Diagnostics/Explicit/<RunName>뿐입니다. 자동 확장/기존 덮어쓰기는 없습니다.
SourceControl 비활성, Save on Compile=Never, 초기 dirty 없음, 전용 unattended commandlet을 요구합니다. 사용자 설정을 자동 변경하지 마세요.
실제 Pawn 부착·외형/메시 변경·DreamCatcher 라이플 이식은 아직 하지 않았습니다. 이번 Config 변경은 Circumference Redirect2개뿐입니다. 원본 Seed의 Hero 타입 최소 수정 이력과 구분하세요.
원본 계층 독립 검증 → 활성 경로 교체·회귀 검증 → 대응하는 기존 구현 제거 순서를 지켜 주세요.
R2 입력 전환이나 이미 검증된 Inventory 기본 테스트를 수동 노드 작업으로 처음부터 반복하지 마세요.
Lyra 원본 재사용 → 최소 수정 이식 → 불가피한 부분만 직접 구현 원칙을 적용해 주세요.
변경안·대상 경로·검증 범위를 먼저 제시하고, 승인된 코드·에셋 묶음만 처리·자동 검사해 주세요.
Unreal 내부 에셋 자동화는 허용하지만 C++ 빌드·패키징은 제가 직접 합니다.
10월 16일까지 R9~R12 신규 제작과 디자이너 main 브랜치 병합까지 포함한 완료가 목표입니다.
원격 최신 상태와 디자이너 미커밋 변경은 별도 확인하고, 실제 병합은 승인 없이 실행하지 마세요.
게임패드 Aim Assist·터치 UI 등 보류 항목은 삭제하거나 완료로 간주하지 마세요.
원본을 그대로 이식하지 못하는 부분은 실제 근거와 이유, 원본을 보존할 대안, 달라지는 동작을 먼저 설명해 주세요.
일정 체크포인트가 밀리면 영향을 즉시 알리고 범위를 임의로 축소하지 마세요.
```

**최신 후속 확인:** 10월 9일 사용자가 사격 체력 감소·HP0 사망 시작/8초 뒤 종료/제거·마우스 방향/감도 세 항목 모두 정상이라고 확인했다. `DreamCatcher.log` 03:19:59~03:20:10의 Health100→0/DeathStarted→정확히8초뒤DeathFinished도 일치한다. 앞선 03:16:16 PIE 초기화 성공을 넘어 해당 수정 묶음의 실사용 검증 통과다. 이미 통과한 세 항목을 다시 미검증으로 돌리거나 재빌드/에셋 재생성을 요구하지 않는다. 다음은 **R6-3 원본 Fire/Impact Cue 등록 전환안의 사용자 승인**이며 아직 Config/에셋 미변경이다. 프로젝트 전역 스캔 경로 영향, 원본 GameFeature 수명 대신 Config 상시 등록이라는 차이를 먼저 설명한다. 수동/자동 재장전·원본 연출/반동·복제 전체 완료는 별개다. 위 과거 인계의 확인 대기보다 이 결과를 우선한다.

**2026-10-09 03:34 최신 재개:** 위 원본 Cue 전환 제안은 사용자 승인 후 적용됐다. `DefaultGame.ini`의 스캔 경로만 교체해 원본 Fire1/Impact1/Death1·구형Fire0을 임시/실제 Config 별도 프로세스에서 확인했고, 실제 PIE 시작/장착/표적 초기화/해제1건도 통과했다. 다음은 에디터 재시작 후 원본 FX·소리·CameraShake 및 R/자동재장전 사용자 확인이다. 새 C++ 빌드나 기존 Cue 삭제는 필요 없다. 등록 검증을 실제 연출/재장전 완료로 확대하지 않는다. `B_WeaponDecals` import 경고와 기존 AutoReload/PoseAsset 경고·Standalone/복제는 별도 남겨둔다.

**최신 사용자 재장전/음향 피드백:** 자동 재장전 불능이 아니다. 사용자는 자동 재장전은 되지만 좌클릭 유지 중 R 수동재장전이 안 된다고 정정했고, 재장전 모션을 사격으로 취소하지 못하도록 요구했다. 원본 태그 규칙이 Fire 우선이며 ReloadDone의 조기 Ability 종료가 모션 전체 보호와 다름을 확인했다. 또한 발사0.12초와 MetaSound ShotInterval0.15초/발사 요청 큐의 불일치를 확인했다. 위 재장전·음향 진단 섹션의 매핑 사본/재장전 정상 종료/오디오 사본 간격 변경 제안은 **아직 미적용, 사용자 승인 대기**다. 원본 음원·발사속도는 유지하는 안이며 실제 음향 지연 해소는 수정 후 검증한다.

**2026-10-09 04:18 최신 정정/재개:** 앞선 재장전 우선 변경안은 사용자 정정으로 철회했다. 정확한 버그는 **탄창소진→자동재장전 완료 후까지 좌클릭 유지→해제 이후 R 수동재장전만 실패**이며 사격/자동재장전은 정상이다. 원본 태그/재장전 정책을 유지한다. 승인된 오디오 사본0.12와 Cue Sound 핀1개 변경만 저장·재로드 검증했고 원본 오디오/재장전/태그/PawnData는 해시 불변이다. 추가 빌드 없이 실제 연사음 지연 해소를 확인한다. R 지속 상태 버그는 미해결·원인 미확정으로 남기고 정확한 조건의 런타임 상태를 조사한다.

**최신 인계:** 사운드는 사용자 정상 작동 확인까지 통과했다. R 버그는 별도 PIE의 Action 주입에서 자동재장전 후 수동재장전22→30이 되어 미재현이며, 물리 R키 경로는 검증하지 않았다. NullRHI 발사 FX 오류 때문에 해당 전체 Automation은 실패다. 코드/태그 규칙은 그대로 유지한다. 다음은 실제 사용자 PIE에서 기존 LogAbilitySystem/LogEnhancedInput Verbose를 켜고 정확한 키 재현 로그를 확인하는 절차이며 추가 빌드·에셋 수정은 필요 없다.

**최신 다음 단계:** 사용자가 R 문제의 현재 미재현을 확인하고 다음 진행을 요청했으므로 추가 R 재현을 먼저 요구하지 않는다. 원인/수정은 미확정 이력으로 보존한다. 다음 제안은 R7-1/2 원본 ReticleHost/라이플 조준점·탄약 위젯을 통합 테스트 맵에 독립 연결하는 코드·에셋 묶음이다. 원본 최종 GameFeature/슬롯 경로와 달리 테스트 Controller가 Host를 직접 표시/제거한다는 차이를 먼저 설명하며 아직 승인 전/미적용이다. 사운드 통과 및 원본 재장전 규칙 유지 결정은 그대로다.

**최신 재개 — R0~R6 완료 감사 우선:** R7 제안은 보류됐다. 2026-10-09 기존 테스트8건은 현재 빌드에서 통과했지만, 비에디터 GameData 타입 충돌/Fatal(종료3), 실패사유 태그5개 공란, 실제 Rifle의0.15초 회복지연이 갱신되지 않는 별도 시간 변수를 읽는 코드 경로가 남아 있다. R2/3/4/5 핵심 통과 이력은 보존하고 최신 통합 수명·면역/경계·카메라/반동·표면 회귀를 구분한다. 다음은 위 감사의 마감 순서에 따른 보완안 제시이며 코드/Config/에셋 수정은 아직 없다. R 현재 미재현 및 사운드 사용자 통과 결정은 유지한다.

**후속 이력 — R1 보완 통과:** 감사의 GameData 기동/실패태그2건은16:15까지 해결·검증했다. 기존 Globals 섹션의 원본 태그5개 복구와 DefaultGameData1개 내부 재저장만 했고 GE3참조/원본 AssetManager 코드는 유지했다. 새 프로세스 재로드, -game의 GameData/맵 진입·종료0, 기존8건 통과다. 당시 다음은 R6 회복지연 재현/최소 변경안이었다. 이후 결과는 아래 최신 재개를 따른다.

**2026-10-09 16:58까지의 후속 검증:** R6 회복지연 최소 보완은 사용자 빌드/신규 포함9건/실제 라이플0.15초 전 유지·후 회복을 통과했다. R2/R5 실제 재장착 탄약 보존·사격중 조종해제·권한 Pawn 순차 교체, R3 피해/치료/면역·사망8초/정리/2초뒤 레벨재시작·재사격과 ASC 없는 대역의 기존 ApplyDamage 수신/면역 경계도 통과했다. 상세 성공/진단 실패 로그와 범위는 상단16:44/16:58 절을 따른다. 실제 적 AI/양방향 적GAS·복제·화면/장치 확인까지 자동 완료 처리하지 않는다.

**2026-10-09 18:26 최신 재개 — 이번 자동 감사 마감:** crouch 태그 수명 보완은 사용자18:20:53 빌드 성공 후18:24 동일 handoff/18:26 정상 토글 회귀를 통과했다. 새 Pawn crouch태그0 및 장비/Ability/재사격 정상이므로 알려진 잔류 결함을 닫는다. 첫발/피해/거리/약점/엄폐/가림 등 이미 통과한 수치 검사를 더 확대하지 않는다. 현재 남은 마감 조건은 사용자 Left Ctrl 자세·카메라구도와 조준/연사 반동 화면 확인이다. 정상 확인 후 R7-1/2 원본 무기 UI 연결 범위를 제시/승인받고 진행한다. R7 새 구현/전체 장치·복제·실맵·패키징까지 완료한 것은 아니다. 상세는 상단18:26 절을 따른다.

### 단계 종료 인계 양식

```text
작업 묶음 ID / 포함 R 세부 단계 / 단계별 상태: 안내만·구현 중·검증 대기·완료
사전 설명·사용자 승인 범위 / 이번 직접 수정 범위:
브랜치 / 확인한 프로젝트·엔진 버전:
원본 파일·에셋 / 대상 파일·에셋:
원본 유지 범위:
최소 변경 내용과 이유 / 원본 대비 의도적인 동작 차이:
그대로 이식하지 못한 항목 / 확인 근거 / 제약 분류 / 검토한 대안 / 결정·재개 조건:
선행 의존성 / 앞당겨 수행한 다른 단계의 일부:
사용자·Codex가 각각 변경한 파일 / 기존 dirty 변경과의 구분:
사용자 C++ 빌드·패키징 결과 / Editor Compile / 자동 검사 / 사용자 PIE 결과: 실행 주체·시각·범위·근거를 각각 구분
자동화 스크립트·허용 경로 / 생성·저장·재로드 결과 / 사용자에게 남은 수동 확인:
남아 있는 참조·임시 연결·미검증 항목:
기존 경로 비활성화·제거 여부와 복구 기준:
다음 작업 ID 및 시작 조건:
마감 체크포인트 대비 상태 / 지연 영향 / 디자이너 중간 커밋·병합 준비 상태:
```

소스 존재만으로 사용자 빌드 성공을 추정하지 않는다. 완료 상태와 체크리스트를 파일에 반영하려면 문서 수정 승인 범위를 확인한다.
