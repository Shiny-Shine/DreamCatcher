# DreamCatcher 프로젝트 통합 컨텍스트 및 Codex 작업 지침

> **리포지토리:** `Shiny-Shine/DreamCatcher`  
> **기본 브랜치:** `main`  
> **문서 역할:** 프로젝트 전체 컨텍스트, 개발 정책, 아키텍처, 구현 현황, Codex 작업 규칙  
> **최종 통합일:** 2026-07-31 KST  
> **Lyra 이식 정책 갱신:** 2026-09-10 KST
> **작업 분담·자동화·마감 기준 갱신:** 2026-10-05 KST
> **권장 위치:** 리포지토리 루트 `/AGENTS.md`

---

## 0. 가장 먼저 읽을 내용

이 문서는 **DreamCatcher 프로젝트의 공통 기준 문서**입니다.

다음 자료를 종합해 작성했습니다.

- 현재 GitHub 리포지토리의 실제 코드와 설정
- 첨부된 `Dream Catcher 개발 최종 가이드`
- ChatGPT 프로젝트 내 세계관, 캐릭터, UI, 레벨, 개발 관련 대화
- 현재까지 확정된 게임플레이 및 시스템 결정
- Unreal Engine 협업 규칙
- UE 5.8 전환 작업
- Codex로 구현 작업을 넘기기 위한 운영 규칙

### 0.1 정보 충돌 시 우선순위

정보가 충돌하면 다음 순서를 따릅니다.

1. 사용자가 가장 최근에 명시한 요구사항
2. `docs/specs/` 아래의 기능별 명세 문서
3. 이 루트 `AGENTS.md`
4. 현재 소스 코드 및 프로젝트 설정
5. 오래된 README, 과거 문서, 이전 대화 내용

GAS 및 Lyra 전환 작업은 `docs/specs/gas-lyra-migration.md`를 기능 명세로 사용합니다.

2026-09-09 사용자 승인에 따라, 이식 가능한 Lyra C++·Blueprint·데이터·시각 에셋은
**원본 재사용 → 최소 수정 이식 → 불가피한 부분만 직접 구현** 순서로 적용합니다.
기존의 “Lyra C++는 복사하지 않고 직접 구현한다”는 일괄 원칙은 더 이상 적용하지 않습니다.

2026-09-10 추가 승인 사항:

- 기존 1~6단계의 자체 GAS·초기화·카메라·장비·발사 구현도 원본 재사용이 가능하면 모두 교체 대상입니다. 기존 구현은 대체 검증 전까지의 비교·복구용 보존입니다.
- OnSpawn은 원본 GameplayAbility·ASC·초기화 묶음으로 교체하고, 무기 UI를 위해 원본 Inventory·QuickBar·Equipment 연결을 선행합니다.
- 퍼짐은 원본 무기 계산·발사·Reticle·데이터를 함께 교체합니다. 기존 최종 퍼짐 getter에 원본 계산을 억지로 맞추는 것을 목표로 삼지 않습니다.
- 기존 무입력 전방/입력 방향 8방향 대시 규칙은 폐기합니다. Lyra GA_Hero_Dash의 실제 원본 동작을 확인해 이식하며, 방어 효과·성공 피드백은 별도 검증·확장으로 구분합니다.
- Hip / Shoulder / Scope 우클릭 규칙은 유지하되 기존 구현 자체를 유지해야 하는 것은 아닙니다.
- 실행 순서는 기능 명세의 **R0~R14**를 사용합니다. 기존 0~6단계 완료, 7-1 자체 Reticle 완료, 7-A Idle 연결까지만 완료라는 이력과 원본 이식 완료 상태를 구분합니다.
- 작업 단위와 직접 수정 권한은 아래 **0.4의 2026-10-05 승인 사항**을 따릅니다. 관련 R 세부 단계의 코드·에셋은 범위를 먼저 설명하고 승인받은 작업 묶음으로 진행합니다. 문서만 갱신하라는 요청은 추가 소스·Config·에셋 수정 권한이 아닙니다.
- 원본을 그대로 이식하지 못해 수정·대체·보류·제외할 때는 실제 소스/그래프/로그 등 확인 근거, 구체적 제약, 원본 보존 대안, 변경 범위·동작 차이를 해당 변경 안내 전에 설명합니다. 근거가 부족하면 이식 불가가 아니라 미검증으로 기록합니다.

아래 과거 소스 구조·수치·로드맵 기록은 최신 이식 목표보다 우선하지 않습니다.
원본 소스 확인과 Blueprint 그래프·런타임 검증을 구분하고, 원본에 없는 기능이나 수치를 추정해 확정하지 않습니다.

실제로 구현된 상태는 현재 소스 코드가 기준입니다.  
프로젝트가 앞으로 가야 할 방향은 이 문서와 기능별 명세가 기준입니다.

### 0.2 확정되지 않은 내용을 임의로 만들지 말 것

필요한 설계가 문서에 없을 경우:

- 큰 아키텍처를 임의로 선택하지 않습니다.
- 가장 작고 되돌리기 쉬운 구현을 선택합니다.
- 필요한 수치는 Blueprint 또는 DataAsset에서 조정할 수 있게 엽니다.
- 작업 완료 보고서에 어떤 가정을 했는지 명시합니다.

### 0.3 Unreal 바이너리 에셋 처리 원칙

다음 파일은 텍스트 코드처럼 직접 작성하거나 조작하지 않습니다.

- `.uasset`
- `.umap`
- `.ubulk`
- `.uexp`

Blueprint, UMG, Animation Blueprint, Input Action, 맵, 레벨 에셋 수정이 필요한 작업은 다음 순서로 처리합니다.

1. 원본 근거·변경 이유·대상 경로·검증 범위를 설명하고 작업 묶음의 승인을 확인합니다.
2. 필요한 C++ API와 이벤트 훅은 승인된 코드 범위에서 구현합니다. C++ 빌드는 사용자가 진행합니다.
3. 승인된 에셋 작업은 Unreal Editor 내부 API로 생성·설정·Compile·저장·검사하도록 자동화할 수 있습니다. Python/Editor Utility/Commandlet을 사용할 수 있지만, 에디터 밖의 일반 파일 I/O로 바이너리 에셋을 직접 복사·변조하지 않습니다.
4. 자동화하지 못한 부분만 정확한 수동 Editor 절차로 전달합니다. 자동 검사와 사용자 PIE·시각·조작감 검증을 구분합니다.

### 0.4 2026-10-05 승인 — 작업 분담·자동화·마감

이 항목은 과거의 “한 채팅에서 한 세부 단계를 사용자가 직접 구현”하는 운영 방식보다 우선합니다. 원본 재사용 정책과 변경 전 근거 설명 의무는 유지합니다.

**작업 분담과 승인 단위**

- 사용자: **C++ 빌드, 패키징, 조작감·화면·연출 확인**을 직접 진행합니다. Codex는 별도 지시 없이는 C++ 빌드·패키징을 실행하지 않습니다.
- Codex: 원본 조사, 승인된 코드 작업, **Unreal 내부 에셋 생성·설정·검사 자동화**를 담당합니다. 사용자는 에셋 자동화를 허용했습니다.
- 진행 순서는 **변경안·원본 대비 차이 제시 → 연관된 코드·에셋 작업 묶음 승인 → Codex 처리·자동 검사 → 사용자 빌드와 짧은 플레이 확인**입니다.
- “완료 상태 확인”, “다음 단계 진행”만으로 새로운 코드·Config 수정 범위를 임의 확대하지 않습니다. 문서 수정 요청이면 문서만 수정합니다.
- 의존성이 강한 여러 R 세부 단계는 묶을 수 있지만, 포함 범위·앞당기는 이유·각 단계의 미검증 항목을 먼저 밝힙니다. 원본 기능의 삭제·축소로 일정을 맞추지 않습니다.
- 자동화는 대상 경로와 작업을 제한하고 기존 변경을 보존합니다. 기존 에셋 덮어쓰기·삭제·대량 참조 교체·브랜치 병합은 해당 범위를 명시한 승인을 확인합니다. 에디터와 자동화 프로세스의 동시 저장도 피합니다.
- 명령줄 에디터 실행과 Blueprint Compile은 C++ 빌드·패키징과 구분합니다. 임시 에디터 월드의 검사는 PIE·복제·장비 연결 검증으로 보고하지 않습니다.

**2026-10-08 추가 승인 — 실제 작업 우선**

- 추가 도구 확장보다 **실제 라이플 에셋 이식과 테스트 플레이어의 장착·발사·재장전·해제 연결**을 우선합니다. R5-3에 R7/R13 선행 준비를 계속 누적하지 않고, 실제 진행을 막는 필수 의존성만 앞당깁니다.
- 기존 도구·검사는 재사용하되 검사 개수 증가를 구현 진척으로 보고하지 않습니다. 새 범용 복사기·감사기·검증 프레임워크는 실제 진행을 막는 재현된 문제와 필요성이 있을 때만 별도 설명·승인 후 확장합니다. 승인된 실제 에셋 이식·설정용 일회성 자동화는 사용할 수 있습니다.
- 직접 필요한 코드·에셋 변경과 관련 검증은 한 작업 묶음으로 제시해 사용자 빌드/승인 왕복을 줄입니다. 짧은 수동 Editor 절차가 자동화 개발보다 효율적이면 작업량·보존성·동작 차이를 설명하고 그 대안을 제안합니다.
- 기능 삭제·원본 동작 축소·무검증 완료 처리는 하지 않습니다. 전체 HUD/장치/복제처럼 현재 연결을 막지 않는 검증은 해당 단계에 미완료로 남깁니다. 기존 구현 제거는 활성 경로 교체와 회귀 검증 후에 합니다.
- “완료 상태 확인/다음 단계”는 새로운 코드·Config·에셋 변경의 포괄 승인이 아닙니다. 다음 변경안의 정확한 범위·원본 근거·차이를 먼저 제시하는 원칙과 사용자 빌드·패키징 담당은 유지합니다.

**마감 및 작업 시간**

- **2026-10-16까지:** 마이그레이션 작업에 더해 **R9~R12의 신규 제작과 디자이너 브랜치 병합까지 포함**하는 완료 목표입니다. 임의로 범위를 축소하거나 미완료 단계를 완료로 표시하지 않습니다.
- **2026-10-17~20:** 아웃게임 UI 등 게임 완성도와 프로젝트 제출 마감을 진행합니다.
- 사용자 작업 가능 시간은 매일 **3~5시간**, 많은 날은 **10시간**입니다. 매일 10시간을 사용할 수 있다고 가정하지 않습니다.
- 10월 5일 시점의 단순 가용 시간은 약 **36~60시간 + 집중 작업일의 추가 시간**입니다. 아래 날짜는 완료 보장이 아니라 지연을 조기에 판단할 기준입니다. 미달 시 일정 영향을 즉시 설명합니다.

| 날짜 (KST) | 판단 기준 |
|---|---|
| 2026-10-08 | 원본 장비·발사·재장전 핵심 흐름 확보 |
| 2026-10-12 | 신규 제작을 포함한 스테이지·보스·결과 흐름 실행 |
| 2026-10-14 | 기능 동결, 통합·오류 수정에 집중 |
| 2026-10-15 | 디자이너 변경 통합 및 병합 후보 검증 |
| 2026-10-16 | 최종 병합과 사용자 빌드·패키징 검증 완료 목표 |

**브랜치와 현재 재개 지점**

- 개발 작업은 `GAS-System`, 디자이너 작업은 `main`입니다. 디자이너에게 미커밋 변경이 많을 수 있으며, 이 작업 공간에서는 그 내용을 확인할 수 없습니다.
- 10월 5일 점검한 저장 커밋은 `main`의 10월 2일 기록과 `GAS-System`의 10월 5일 기록입니다. 공통 조상 이후 같은 파일 수정 경로는 0개였지만, 원격 최신 상태·미커밋 변경·참조 및 게임플레이 호환성을 보증하지 않습니다. 실제 병합은 아직 하지 않았습니다.
- GAS 테스트 맵은 디자이너 맵과 분리되어 있습니다. 다만 공용 Blueprint·머티리얼·Config와 향후 실제 맵 통합은 별도 확인합니다. 16일에 처음 병합하지 말고 디자이너의 중간 커밋으로 통합 검증할 시점을 잡습니다.
- **R4-3:** PC 조준 입력·정상 종료·사망·입력 차단·Possession 해제 경로를 검증했습니다. 게임패드 Aim Assist·터치 UI는 보류이며, 모든 장치·시각 수치의 전체 검증 완료를 의미하지 않습니다.
- **R5-1:** 원본 Inventory 기본 8개 C++ 파일과 사용자 빌드가 준비되었고, 아래 자동화의 생성·저장·별도 프로세스 재로드 및 기본 기능 검사가 통과했습니다. PIE·복제·Equipment·QuickBar 연결은 미검증입니다.
  - 스크립트: `Scripts/Editor/r5_inventory_automation.py`
  - 신규 테스트 에셋: `/Game/DreamCatcher/GAS/Test/R5/Automation/ID_DC_R5_InventorySmoke`
  - 근거: `Saved/Logs/R5InventoryAutomation-verify.log`의 `smoke_pass`, `mode=verify`, `PIE=false`
  - 확인 내용: 생성 → 초기 StatTags 7 → 증가 후 10 → 감소 후 6 → 조회 → 제거 후 빈 목록
- **R5-2 독립 검사 통과:** 10월 5일 원본 기반 별도 장비 계층 20개 파일과 테스트 C++에 대한 사용자 빌드가 **16:20 KST 성공**했습니다. 테스트 에셋 4개 생성·저장·별도 프로세스 재로드 후 **16:31 KST `DreamCatcher.R5.Equipment.QuickBarLifecycle` 성공(1건, 경고·실패 0건)**을 확인했습니다. 실제 플레이어 연결·PIE·복제 검증과 구분합니다.
- 교체 순서는 **별도 이름의 원본 계층 → 독립 검증 → 활성 경로 교체·회귀 검증 → 대응하는 기존 자체 구현 제거**입니다. 기존 구현을 최종 경로로 계속 유지하지 않되, 독립 검사만으로 실사용 Pawn·PlayerState·HUD·발사·복제가 검증됐다고 판단하여 먼저 삭제하지 않습니다.
- **R5-3/R6 선행 코드 검사 통과:** 원본 기반 13개 파일과 검사 코드에 대한 사용자 빌드가 **10월 5일 16:51 KST 성공**했습니다. **16:53 KST** 무기 `SpreadAndAttenuation`·`ItemTagCost`와 R5 `QuickBarLifecycle` 회귀 검사가 모두 통과했습니다(3건, 실패·경고 0건). 실제 라이플 BP 발사·재장전·화면·복제 검증은 아닙니다.
- **원본 라이플 읽기 전용 조사:** 17:00 KST 기준 원본 에셋 14개와 그래프 25개에서 데이터·노드/핀 연결을 수집했습니다. 추가 의존성 `LyraPawnComponent_CharacterParts`·`LyraCharacterPartTypes`와 장비 해제의 원본 Hero 타입 참조를 확인했습니다.
- **CharacterParts 준비 검증:** 원본 3개 파일 이식 후 **17:23 KST 사용자 빌드 성공**, **17:26 KST 클래스·구조체 4종·enum 로드/빈 CDO 조회 통과**, **17:28 KST 기존 R5/R6 검사 3건 회귀 통과(실패·경고 0건)**를 확인했습니다. 실제 Pawn의 파트·태그·메시 수명과 복제 검증은 아직 아닙니다. 원본은 `BodyMeshes` 선택값을 Pawn 메시에 적용하므로 기본 메시가 없는 상태에서 실사용 Pawn에 파트를 추가하지 않습니다.
- **승인된 에셋 준비 진행 중 / 미완료:** 원본 Lyra의 신규 `/Game/LyraMigration/Rifle/Seed`에 핵심 복사본을 만들고 Hero 타입 핀을 최소 연결했습니다. 하지만 Advanced Copy 후 저장이 두 방식에서 충돌했고, 헤더 치환 방식도 `Unknown section`으로 불완전했습니다. 원본의 Content/Payload/Prepared 하위 폴더는 **부분 진단 결과이며 이식 완료본이 아닙니다.** DreamCatcher에는 아직 이식하지 않았고 Config도 변경하지 않았습니다.
- **추가 C++ 준비 검증 통과:** 테스트의 multicast 조회 API를 교정한 뒤 **18:29 KST 사용자 재빌드 성공**, **18:31 KST 신규 TeamColor 검사와 기존 3건 모두 통과(성공 4, 실패·경고 0)**했습니다. 실제 색상 시각 출력·복제·에셋 연결 완료는 아닙니다.
- **복사 분리 진단:** 원본 음악 컴포넌트와 무기 오디오 매크로는 새 Diagnostics 폴더에서 개별 복사·저장·재로드에 성공했습니다. 대량 Advanced Copy의 참조 치환/저장 경로 문제와 구분해야 하며 원인 확정은 아닙니다.
- **Editor 보조 도구 검증:** `Plugins/DCRifleMigrationTools`는 **19:35 KST 사용자 빌드 성공**, **19:38 KST NameGuards 1건 통과(실패·경고 0)**, 원본 Lyra의 외부 플러그인 로드·사전 검사·오디오 2개 메모리 복사/저장/새 프로세스 재로드까지 성공했습니다. 한 번에 최대 16개·새 `Rifle/Diagnostics/Explicit/<RunName>`만 허용하며, 기존 목적지·dirty 패키지·맵·헤더 치환·활성 소스 컨트롤을 거부합니다. 별도 unattended commandlet과 `-DCRifleCopyDiagnostics`가 필요합니다. 기존 폴더 거부와 명시적 원본 해시 유지도 확인했습니다. C++ 빌드·패키징은 Codex가 실행하지 않았습니다.
- **R5-3 상속 보존 보강 / 사용자 빌드 대기:** 실제 `B_Weapon`+`B_Rifle` 복사본에서 **19:55~20:00 메시·재질·애니메이션 참조 및 자식 override 소실**을 확인했습니다. 후속 사용자 승인으로 Editor 도구에 SCS override 스냅샷 → 새 부모 GUID/변수명 대응 → 원본 엔진 속성 복사 → Compile/속성 비교 → 성공 시에만 저장하는 보존 모드와 별도 비교 API를 추가했습니다. `r5_rifle_explicit_copy.py`의 `-DCRiflePreserveOverrides`로 명시적으로 켭니다. 임시 Blueprint의 누락·복원·값 변조 검출 테스트도 작성했습니다. **보강 코드의 빌드·실행·실제 라이플 복구는 아직 미검증**입니다. C++ 빌드·패키징·새 에셋 복사는 이번 코드 작업에서 하지 않았습니다. 게임 C++·Config·기존 실패본·활성 경로는 그대로입니다. 다음은 사용자 빌드 확인 → NameGuards/InheritedOverrides 테스트 → 새 진단 폴더의 메모리/저장/재로드 비교입니다. SCS 외 구성·Blueprint 정의 컴포넌트 클래스 등 보존기가 아직 검증하지 못하는 구성은 거부하고 이유를 남기며, 원본 이식 불가로 단정하지 않습니다. 대량 저장 충돌의 정확한 원인도 여전히 미확정입니다.
- **후속 검사 이력:** **20:56 사용자 빌드, 21:07 InheritedOverrides/NameGuards 2건 통과** 및 **21:24 기존 손상본의 누락 판정**까지 성공했습니다. 21:27 실제 복사는 로드 후 Niagara 4개의 dirty 상태 때문에 실행 전 중단됐고, 21:30 단순 로딩만으로 이 상태가 생김을 확인했습니다. 실제 보존 복사·저장·재로드는 아직 미검증입니다.
- **상속 보존・정책 검증 통과:** **21:53 사용자 빌드 성공**, **21:54 MigrationTools 자동 검사 3건 성공(실패·경고 0)**, **21:56 원본 B_Weapon/B_Rifle의 보존 모드 메모리/저장 성공**, **21:57 별도 프로세스 재로드/Compile/부모・package 참조/override 1개 전체 비교 성공**을 확인했습니다. 새 원본 프로젝트 경로는 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/WeaponActorPreserveSave_2155/`의 2개 에셋입니다. 원본 Blueprint 2개와 Niagara 4개의 파일/동반 파일 24개 상태가 유지됐습니다. 전체 라이플 이식・PIE・발사・복제 성공은 아닙니다.
- **핵심 16개 준비 코드 이력:** 22:17 조사 후 승인된 정확한 16개 정책과 `r5_rifle_core_stage.py`를 작성했습니다. Python 순수 계약 테스트 5건이 통과했고, 이어 **10월 5일 23:36 사용자 빌드 성공・23:38 MigrationTools 3건 성공(실패・경고 0)**을 확인했습니다. 아래 통합 실패를 이 독립 검사 성공과 구분합니다.
- **core16 통합 검증 이력:** **10월 5일 23:40** 사전 검사는 통과했지만 메모리 복사에서 `GCN_Weapon_Rifle_Fire` 타입 혼재와 원본 `B_Pistol` dirty를 검출해 저장 전에 중단했습니다. 원본 core16+Niagara4의 파일/동반 파일 80개 상태는 유지됐습니다(Pistol은 이 사전 해시 집합 밖). **23:58~23:59** 원본 WeaponAudioMacros/WeaponAudioFunctions 의존성과 엔진 ConsolidateObjects의 원본 자식 BP 영향 경로를 확인했습니다. Niagara 단순 로드 dirty와 달라 Pistol을 예외 목록에 넣지 않습니다.
- **2026-10-06 승인된 도구 보강 코드 이력:** `DCRifleMigrationTools`의 전역 Advanced Copy 호출을 **엔진 개별 복사 + 새 패키지 내부 참조 치환**으로 교체하고, 복사 전 로드된 원본 BP의 부모/생성 클래스/상태/dirty/타입 참조/파일 보호 검사를 추가했습니다. 원본 매크로/함수 2개를 포함한 **정확한 18개만** 추가 허용하며, 기존 일반 상한 16개와 Niagara dirty 예외 4개는 유지합니다. 원본 Pistol 4개는 별도 사전 파일 감시/로드 대상이지 복사・dirty 예외 대상이 아닙니다. 매크로 그래프/소유 BP・함수 소유 클래스・cast/핀 subtype의 공개 C++ 조회 API와 `ScopedReferences` 테스트를 추가했습니다. 컴파일 자동 저장이 Never가 아니면 설정을 바꾸지 않고 거부합니다.
- **코드 작성 당시 검사:** closure18 준비 스크립트와 순수 Python 테스트 6건, AST/diff 검사를 통과했습니다. 아래 사용자 빌드/실행 결과가 최신이며, 당시의 빌드 대기 기록과 구분합니다.
- **2026-10-06 00:59 KST 검사 이력:** 사용자 UHT/C++ 빌드는 **00:54:34 시작・00:55:13 DLL 갱신・39.10초 성공**입니다. **00:57 MigrationTools 4건은 성공 2/실패 2**(`InheritedOverrides`, `LoadDirtyPolicy` 성공; `NameGuards`, `ScopedReferences` 실패). 프로세스 종료 0과 테스트 성공은 다릅니다. NameGuards의 원본 /ShooterCore 경로 검사는 마운트 없는 DreamCatcher에서 실행했고, ScopedReferences는 EditorContext 등록과 IsRunningCommandlet 요구가 서로 모순입니다. 실제 복사 코드의 성공/실패까지 도달한 것이 아닙니다.
- **원본 조회 완료:** 원본 Lyra `R5Closure18-inspect.log`의 **00:58:56 사전 검사・00:59:07 inspect 성공**, asset writes 0, 원본/동반 파일 104개 상태 유지. 18개와 Pistol4를 로드해 그중 BP21개의 매크로 graph/owner・함수 owner・cast/핀 타입을 공개 API로 조회했으며 원본 BP dirty 0입니다. 실제 18개 복사/저장/재로드・PIE・복제는 미검증입니다.
- **검사 조건 교정 코드 이력:** 사용자 승인 후 Editor 플러그인의 CPP 3개에서 이름/마운트 검사 분리와 임시 ScopedReferences의 headless Editor Automation 실행 조건을 교정했습니다. 실에셋 복사 API의 Commandlet/저장/범위 보호는 유지했습니다. 정적 검사와 Python 6건 통과 후 아래 재빌드/재실행으로 이어졌습니다.
- **2026-10-06 01:46 KST 검사 이력:** 사용자 빌드는 **01:43:41 시작・8.91초 성공・DLL 01:43:49 갱신**입니다. `R5ScopedPolicyChecks-fixed.log`와 `Saved/Automation/R5ScopedPolicyChecksFixed/index.json`의 **01:46:40 결과는 성공 3/실패 1**입니다. InheritedOverrides/LoadDirtyPolicy/NameGuards 통과, ScopedReferences는 실행 조건과 임시 BP 생성/Compile을 통과한 뒤 **첫 DuplicateSingleObject의 /Temp 목적지 검사에서 실패**했습니다. 엔진 PackageName.cpp는 `/Temp`를 ReadOnly로 등록하고 ObjectTools.cpp의 복사 API는 writable root를 요구합니다. 타입 치환/원본 형제 보호 성공까지 검증한 것은 아닙니다.
- **테스트 경로 교정 후 검증 통과:** 사용자 승인한 `/Game/.../ScopedFixture_<GUID>` 경로 교정은 **10월 6일 02:11:01 사용자 빌드(6.52초 성공), DLL 02:11:07 갱신**, **02:12:41 MigrationTools 4건 성공・실패/경고 0**으로 검증했습니다. `Saved/Automation/R5ScopedPolicyChecksGameRoot/index.json`을 근거로 하며 테스트 fixture의 파일 미생성도 통과했습니다. 실제 라이플 검증과 구분합니다.
- **2026-10-06 02:13 KST 실제 메모리 검사 이력:** 원본 `R5Closure18-memory.log`, 실행 `RifleCore_Memory_0213`에서 변경 전 18개 비교/원본 보호/override 1개는 통과했으나 **복사된 WeaponInstance 자식 CDO의 UberGraphFrame key mismatch ensure**가 발생했습니다. 보조 도구의 success=true와 구분해 전체 실패로 판정했습니다. 승인된 Hero 입력 변경 후 ReturnValue subtype 1개 변화도 엄격 비교에서 거부돼 종료1・저장/prepare/verify 미실행입니다. 원본/동반 파일104개는 종료 후 대조에서도 유지됐습니다.
- **수명 처리 보강 검증 통과:** 부모 우선 엔진 재부모화/Compile, SuperStruct 직접 치환 거부, ensure/Error 저장 차단, Hero 반환 subtype 1개 기대값과 프레임 회귀 fixture는 **10월 6일 02:42 사용자 빌드 성공(19.48초, DLL 02:42:50)**, **02:44:04 C++5건 성공・실패/경고0**으로 검증했습니다. 근거 `Saved/Automation/R5LifecycleChecks/index.json`. fixture의 알려진 계층 변경 Warning은 1건만 명시적으로 기대했고 ensure/Error는 허용하지 않았습니다.
- **2026-10-06 02:48 KST 핵심18개 준비/저장/재로드 검증 완료:** 원본 Lyra `R5Closure18-lifecycle-memory.log` **02:45:24 memory**, `R5Closure18-lifecycle-prepare.log` **02:46:46 prepare**, `R5Closure18-lifecycle-reload.log` **02:48:47 verify**가 모두 종료0/complete로 통과했습니다. 저장본은 원본 프로젝트 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/`의 **18개**입니다. ensure0, 변경 전후 주요 데이터/그래프/타입, override1개, 새 프로세스의 직접 package 참조 18개 모두 누락/추가0, 원본/동반 파일104개 및 재로드 대상72개 상태 유지까지 확인했습니다. 이 범위의 실에셋 준비 하위 검증은 완료지만 DreamCatcher 이식・활성 연결・PIE・발사/복제 성공은 아닙니다.
- **의존성 조사 이력:** 종속 package1228개(Game967, ShooterCore2, Niagara201, ControlRig56, AudioModulation1, AnimationLocomotionLibrary1; Engine/Script70 별도), 원본 core/Hero 역참조0입니다. 남은 ShooterCore는 W_Reticle_Rifle→Struct_UIMessaging, GA_Weapon_Fire_Rifle_Auto→GE_Damage_RifleAuto이며 대상 Game 경로114개가 겹칩니다. 핵심18개 준비 완료와 전체 이식 완료를 구분합니다.
- **2026-10-06 공용114개 읽기 전용 비교 완료:** `R5SharedAudit-full2.log` 양쪽 프로젝트 실행은 원본464개/대상460개 파일 상태 불변・에셋 저장0・ensure0으로 완료됐습니다. 이 조사 당시 진단 프로세스에만 Spatialization을 활성화했습니다. 미연결 delegate의 프로세스 주소8개를 구분한 뒤 **조회 가능 속성113/114, 텍스트 export88/114, 텍스처 export12/12, package 참조100/114**가 일치합니다. SK_Rifle의 SourceModels 캐시 차이는 남지만 별도 조회에서 양쪽 LOD1/정점11255/section1은 일치했습니다. 전체 형상/리깅/시각 동등성이나 114개 무조건 재사용 확정은 아닙니다.
- **확인된 다음 제약:** Struct_UIMessaging의 ON/Controller 멤버・멤버 GUID・기본값은 같지만 struct GUID는 달라 기존 ADS 구조체 하나로 연결 후 Compile/메시지 검증이 필요합니다. 원본 RifleAuto GE는 Instant, BaseDamage 스칼라 보정12이며 SetByCaller 테스트 GE로 대체하지 않습니다. 대상에서 Spatialization 클래스 미등록을 실제 로드 실패로 확인했습니다. 원본 Reticle은 아직 없는 CircumferenceMarkerWidget을, WeaponAudioFunctions.SendWeaponFire는 아직 없는 LyraGameState를 참조합니다. 원본 GameState는 ExperienceManager를 생성하므로 빈 대체 클래스나 임의 cast 변경으로 숨기지 않습니다. B_LyraGameInstance/AnimInstance 의존성도 native 연결 감사에 포함합니다.
- **후속 준비 묶음 제안 이력:** 원본 UMG・Slate CircumferenceMarker 4파일과 최소 검사, 해당 클래스/struct Redirect, Spatialization 플러그인 정식 활성화를 제안했습니다. 원본 Reticle 값은 OutsideRadius=true/Radius24지만 Slate의 해당 bitfield에 명시적 초기화/UMG 전달이 없음을 소스에서 확인하고, 옵션 초기화/전달만 보강하는 차이를 설명했습니다. 대형 임시 보고서36개는 SHA256 검증한 `Saved/Diagnostics/R5SharedAudit_20261006.zip`으로 복원 가능하게 보존한 뒤 정리했습니다.
- **Circumference 준비 코드 이력:** 사용자 승인으로 `UI/Weapons/Lyra/DCLyraCircumferenceMarkerWidget` 및 `SDCLyraCircumferenceMarkerWidget`의 h/cpp **4개**, transient 검사 CPP **1개/3건**, 타입 Redirect2개와 Spatialization 활성화를 추가했습니다. 원본 메서드7개 본문을 보존하고 OutsideRadius 초기화/UMG 전달만 보강했습니다. 아래 사용자 빌드/실행 결과가 최신이며, 원본과의 픽셀 동일성은 여전히 미검증입니다.
- **2026-10-06 16:32 KST UI 의존성 검증 통과:** 사용자 빌드 **16:25:32 시작・25.76초 성공・DLL16:25:57 갱신**을 UBT 로그에서 확인했습니다. **16:28:14 Circumference3건, 16:32:06 기존 Equipment/QuickBar・TeamColor・무기 계산/비용4건 모두 통과**(보고서 성공7/실패0/경고0). 근거는 `Saved/Automation/R5CircumferenceChecks/index.json`, `R5UIPrerequisiteRegression/index.json`입니다. **16:30:45 `R5CircumferenceDependencies.log`**에서 실제 Spatialization 에셋 로드, 에셋/동반4개 파일 상태 불변, 저장0/dirty0/ensure0을 확인했습니다. 진단용 Spatialization 활성화 옵션 없이 `.uproject` 등록만 사용했습니다. 재실행 가능한 읽기 전용 `Scripts/Editor/r5_circumference_dependency_verify.py`를 추가했으며 이번에는 게임 C++/Config/에셋/활성 경로를 수정하지 않았고 빌드/패키징도 실행하지 않았습니다. 현재 **R5-3 진행 중**, 실제 Reticle 화면・라이플 이식/발사・PIE・복제는 미검증입니다.
- **Experience 선행 제안 이력 / 아래 후속 승인으로 코드 반영:** 원본 `LyraExperienceDefinition`・`LyraExperienceActionSet`・`LyraExperienceManager`의 h/cpp **6파일**을 `GameModes/Lyra/DCLyraExperience*` 별도 계층으로 이식하고 데이터 기본값/검증・PIE 다중 요청 카운트의 독립 검사를 추가합니다. 기존 `UDCPawnData` 타입 연결 외 원본 동작을 유지하며 활성 GameMode/GameState/Config/Experience 에셋은 바꾸지 않습니다. 원본 GameState 생성자는 ExperienceManagerComponent를 생성하며, 그 컴포넌트는 CommonLoadingScreen의 인터페이스와 SettingsLocal.OnExperienceLoaded→ApplyNonResolutionSettings에 의존합니다. 대상에는 두 계층이 아직 없고, 원본 설정 재적용에는 오디오 버스/성능 설정도 포함됩니다. 빈 GameState나 설정 호출 삭제로 우회하지 않습니다. 전체 계층을 한 번에 이식하는 대안은 전역 설정 영향과 추가 의존성 검증이 먼저 필요하므로, 이 6파일은 후속 완전 이식을 위한 선행 준비이지 생략/대체가 아닙니다. CoreRedirect/PrimaryAsset 스캔・SettingsLocal/CommonLoadingScreen・GameState 활성 교체는 별도 승인 범위로 남깁니다. 후속 사용자 승인으로 아래 6파일/독립 검사 범위만 반영했으며, 나머지 제약은 유지합니다.
- **2026-10-06 Experience 선행 코드 작성 이력:** `GameModes/Lyra/DCLyraExperienceDefinition`・`DCLyraExperienceActionSet`・`DCLyraExperienceManager`의 h/cpp6개와 `Tests/DCExperienceFoundationAutomationTests.cpp`의 검사3건을 추가했습니다. 이름/include/module export 및 기존 `UDCPawnData` 타입 대응만 변경했으며 정규화한 원본6파일 비교가 일치합니다. 검사 범위는 transient 데이터 기본값/Action validation, 엔진 AddComponents Action의 Client/Server bundle 수집, EngineSubsystem의 중복 요청/마지막 해제 결정입니다. 실제 plugin 활성화/비활성화・동시PIE・Blueprint 상속 검사・에셋 import/scan은 미검증입니다. `PluginRequestArbitration`은 별도 unattended Editor와 `-DCR5ExperienceIsolatedAutomation`을 요구하고 합성 키만 균형 있게 등록/해제하며 기존 카운트를 일괄 초기화하지 않습니다. Config/uproject/Build.cs/기존 GameMode5파일의 작업 전후 해시 불변을 확인했습니다. 기존 에셋・활성 경로・SettingsLocal・CommonLoadingScreen・ExperienceManagerComponent는 변경하지 않았습니다. **작성 당시 UHT/C++ 빌드와 새 검사3건 실행은 미검증이었으며 아래 후속 결과가 최신**, Codex는 빌드/패키징/에디터 실행을 하지 않았습니다. 다음은 사용자 Development Editor/Win64 빌드 확인 → `DreamCatcher.R5.ExperienceFoundation`3건과 기존 회귀 검사입니다. 별도 native 클래스명으로 인해 PrimaryAssetType도 DCLyraExperienceDefinition/ActionSet 이름을 따르며 기존 원본 ID/scan 설정과의 연결은 후속 범위입니다. 현재 R5-3/R13 선행 준비일 뿐 GameState/Experience 전체 이식 완료가 아닙니다.
- **2026-10-06 16:48 KST Experience 기반 검증 통과:** 사용자 빌드 **16:47:10 시작・17.85초 성공・DLL16:47:28 갱신(3,387,392 bytes)**을 확인했습니다. **16:48:52 `Saved/Automation/R5ExperienceFoundationChecks/index.json`**에서 Experience3건＋Circumference3건＋기존 Equipment/QuickBar・TeamColor・무기 계산/비용4건 **성공10・실패/보고서 경고0**입니다. `Saved/Logs/R5ExperienceFoundationChecks.log`의 ensure/Error/Fatal도0이며 프로세스 종료0입니다. 임시 native 데이터/번들 수집/요청 카운트 범위이고 실제 Experience 로드・동시PIE・원본 BP import・활성 GameState・라이플/복제 완료는 아닙니다. 이번 확인은 테스트 실행・원본 읽기 전용 조사・문서 갱신만 했으며 게임 C++/Config/에셋/Git 인덱스는 수정하지 않았고 빌드/패키징도 실행하지 않았습니다. 이 확인 당시에는 추가 빌드가 필요하지 않았으며 아래 후속 작업으로 다시 빌드 대기 상태입니다.
- **Performance/Audio 데이터 준비 제안 이력 / 아래 후속 승인으로 코드 반영:** SettingsLocal의 직접 의존성인 원본 `Performance/LyraPerformanceStatTypes.h`, `LyraPerformanceSettings.h/.cpp`, `Audio/LyraAudioSettings.h/.cpp` **5파일**을 DCLyra 별도 이름으로 이식하고 설정 기본값/enum/성능 옵션 목록/오디오 soft-reference 구조 독립 검사를 추가합니다. 원본 구조・C++ 기본값은 유지하고 Config값/오디오 에셋 경로는 아직 복사하지 않습니다. 원본 SettingsLocal::Get은 전역 GameUserSettings를 CastChecked하며 대상에는 해당 클래스 선택이 없고, AudioMixEffectsSubsystem은 Game/PIE에서 생성되어 OnWorldBeginPlay에 믹스를 적용합니다. CommonLoadingScreen도 비전용서버 GameInstance에 생성되며 로딩 위젯 실패 시 Error＋대체 Throbber로 전환합니다(원본 Config의 W_LoadingScreen_Host 참조 필요). 따라서 다음 묶음은 이 데이터5개/검사만 준비하며 SettingsLocal/AudioMix/LoadingScreen 활성화・GameState/ExperienceManagerComponent・GameUserSettingsClassName/Config/에셋 이전은 별도 승인 대상으로 유지합니다. 전체 원본 기능을 삭제/축소하는 결정이 아닙니다. 현재 R5-3/R13 선행 준비 중입니다.
- **2026-10-06 Performance・Audio 데이터 코드 작성 이력:** 승인된 `Performance/DCLyraPerformanceStatTypes.h`, `DCLyraPerformanceSettings.h/.cpp`, `Audio/DCLyraAudioSettings.h/.cpp`5파일과 `Tests/DCSettingsFoundationAutomationTests.cpp`의 검사3건을 추가했습니다. 원본 타입명/include/module export 연결만 대응했고 정규화한 원본5파일 전체 비교가 일치합니다. 원본 enum 값/순서, Desktop FPS 후보10개・Mobile 후보6개, stat18개, Audio soft path8개와 HDR/LDR chain 구조/C++ 기본값을 유지했습니다. `DreamCatcher.R5.SettingsFoundation`의 PerformanceEnums/PerformanceDefaults/AudioSchemaAndDefaults를 작성했지만 **작성 당시 UHT/C++ 빌드・새 검사 실행은 미검증이었으며 아래 후속 결과가 최신**입니다. 원본 PerPlatformSettings 초기화는 플랫폼별 설정 데이터 객체를 준비할 수 있으나 DeviceProfile/FPS/오디오를 적용하지 않습니다. 테스트도 설정 저장/오디오 로드/벤치마크/믹스 적용을 호출하지 않습니다. Config2개・uproject・Build.cs・기존 GameMode/LocalPlayer h/cpp 총8개 해시가 그대로이며 기존 에셋/활성 경로/전역 사용자 설정은 미변경입니다. Codex는 빌드/패키징/에디터 실행을 하지 않았습니다. 다음은 사용자 Development Editor/Win64 빌드 후 새 검사3건과 이전10건 회귀입니다. 새 클래스의 Config 섹션은 DCLyra 이름이므로 원본 INI/오디오 에셋 참조 연결・SettingsLocal/AudioMix/CommonLoadingScreen/GameState 활성화는 여전히 후속 승인/검증 범위입니다. 현재 R5-3/R13 선행 준비 중이며 기능 전체 완료가 아닙니다.
- **2026-10-06 17:10 KST 설정 데이터 검증 통과:** 사용자 빌드 **17:09:33 시작・17.79초 성공・DLL17:09:50 갱신(3,470,336 bytes)**을 확인했습니다. **17:10:59 `Saved/Automation/R5SettingsFoundationChecks/index.json`**에서 SettingsFoundation3건＋이전10건 **성공13・실패/보고서 경고/notRun0**입니다. `Saved/Logs/R5SettingsFoundationChecks.log`의 ensure/Error/Fatal도0, 프로세스 종료0입니다. 데이터 기본값/스키마 독립 검증이며 실제 Config/오디오/FPS/DeviceProfile/Experience/라이플/복제 완료가 아닙니다. 이번에는 테스트・원본 읽기 전용 조사・문서 갱신만 했고 게임 C++/INI/uproject/에셋/인덱스는 수정하지 않았습니다. 보호 대상8개 해시도 이전 값과 일치합니다. 빌드/패키징은 Codex가 실행하지 않았고 당시에는 추가 빌드가 필요하지 않았으며 아래 후속 코드 작업으로 다시 빌드 대기입니다.
- **실행 계층 제안 이력 / 아래 사용자 승인으로 코드 반영:** 원본 SettingsLocal・AudioMixEffectsSubsystem・PlatformEmulationSettings・ExperienceManagerComponent・GameState h/cpp10개를 DCLyra 이름으로, CommonLoadingScreen 플러그인의 소스8개/Build.cs/descriptor10개를 원본 기반으로 준비합니다. 원본기반20파일＋독립 검사＋DreamCatcher.Build.cs의 ApplicationCore/CommonLoadingScreen/AudioMixer/AudioModulation 연결 및 .uproject 플러그인 등록이 제안 범위입니다. 기존 GameMode/LocalPlayer・INI(GameUserSettingsClassName 포함)・에셋・활성 Pawn/무기는 바꾸지 않습니다. 원본 로딩 위젯 W_LoadingScreen_Host가 대상 원본 경로에 없고 파일명 조사에서도 확인되지 않았으며, 원본 Subsystem은 자동 생성/믹스 적용, PlatformEmulation은 PostInit→ApplySettings로 전역 디버그 조건을 갱신합니다. 별도 타입명만으로 격리되지 않아 **원본에 없는 임시 진단 opt-in**을 LoadingScreen/AudioMix 생성 및 PlatformEmulation.ApplySettings에 추가하는 안을 제시합니다. 일반 실행에서는 비활성, 별도 unattended Editor의 -DCLyraRuntimeDiagnostics에서만 해당 원본 경로를 허용합니다. 원본 본체/설정 호출/기능은 삭제하지 않습니다. 이는 최종 동작이 아니며 실에셋·설정 준비 후 별도 승인된 활성 전환 때 조건 제거/회귀 검증이 필요합니다. 원본 그대로 즉시 활성화하는 대안과 내장 -NoLoadingScreen(화면만, 비Shipping)은 오디오/플랫폼/설정까지 격리하지 못하므로 이번 선행 준비에는 권장하지 않습니다. 전체 이식 불가 판정이 아니라 검증 전 활성 시점 분리이며, **후속 사용자 승인으로 이 임시 차이를 포함한 코드 묶음을 반영함**입니다. 예상 밖 추가 의존성/API 제약은 미검증으로 남기고 범위 확대 전 설명합니다.
- **2026-10-06 실행 계층 코드 작성 이력:** 원본 기반20파일(Game10＋CommonLoadingScreen10), plugin의 `DCLyraRuntimeDiagnostics.h` helper, `Tests/DCLyraRuntimeFoundationAutomationTests.cpp` 검사4건을 추가했습니다. Build.cs에 ApplicationCore/CommonLoadingScreen/AudioMixer/AudioModulation을 연결하고 .uproject에 CommonLoadingScreen을 등록했습니다. 진단 조건은 **WITH_EDITOR・GIsEditor・비Commandlet・unattended・-DCLyraRuntimeDiagnostics**를 모두 요구하며 LoadingScreen/AudioMix 생성과 PlatformEmulation.ApplySettings3곳에만 적용했습니다. 원본 본체/Experience 완료의 SettingsLocal 호출은 유지했습니다. 이름/include/module 대응과 허용된 guard3곳/읽기 전용 test friend1곳 외 정규화20파일 비교가 일치하며, INI/GameMode/LocalPlayer6개 해시 불변・JSON/diff 정적 검사를 통과했습니다. 기존 에셋/전역 GameUserSettings/활성 경로는 미변경입니다. 새 검사 prefix는 `DreamCatcher.R5.RuntimeFoundation`(DiagnosticGate/SettingsDefaults/SubsystemsAndLoadingTask/GameStateInitialState)이며 상태를 건드리는 검사는 별도 unattended Editor＋`-DCR5RuntimeIsolatedAutomation`을 요구합니다. 오디오 참조가 비어 있는지/뷰포트가 없는지 사전 확인하며 실제 Experience 선택/설정 Apply/Save/오디오 출력은 실행하지 않습니다. **작성 당시 UHT/C++ 빌드/새 검사 실행은 미검증이었으며 아래 결과가 최신**, Codex는 빌드/패키징/에디터 실행을 하지 않았습니다. 다음은 사용자 Development Editor/Win64 빌드 확인 후 진단 flag 없는 기본 모드와 flag 있는 진단 모드를 각각 새 프로세스로 검사하고 이전13건 회귀를 확인하는 것입니다. 원본 LoadingScreen의 PreLoadMapWithContext 등록/PreLoadMap 해제 차이는 그대로이며 실제 맵 전환 delegate 정리는 추가 검증 대상입니다. 정상 활성 전환 때 임시조건 제거/회귀가 필요하고 현재 R5-3/R13 선행 준비이지 전체 이식 완료가 아닙니다.
- **2026-10-06 23:34 KST 빌드 성공・진단 검사1건 실패:** 사용자 빌드 **23:28:48 시작・52.93초 성공**, 게임 DLL **23:29:40 / 3,724,288 bytes**, CommonLoadingScreen DLL **23:28:59 / 174,592 bytes**를 확인했습니다. **23:31:33 기본 모드17/17 성공**, **23:34:35 진단 모드16성공/1실패**이며 양쪽 보고서 경고/ensure/Fatal0입니다. 근거는 `Saved/Automation/R5RuntimeFoundationDefault/index.json`, `R5RuntimeFoundationEnabled/index.json`과 동명 Logs입니다. 종료코드는 둘 다0이지만 전체 검증 완료가 아닙니다. 실패는 SubsystemsAndLoadingTask의 `GetTickableTickType()==Never` assertion으로, Codex가 초기 등록 정책 getter를 현재 Tick 등록 상태로 잘못 읽은 검사 오류입니다. 엔진 Tickable.h/.cpp에서 GetTickableTickType은 초기 정책, SetTickableTickType은 내부 등록 목록 갱신임을 확인했습니다. 이 실패로 실제 Tick 지속 여부를 판단할 수 없으며 적합한 검사로 재검증해야 합니다. 이번 확인은 테스트/소스 조사/문서만 진행했고 게임 C++/INI/에셋/인덱스는 수정하지 않았으며 빌드/패키징도 실행하지 않았습니다.
- **교정 제안 이력 / 아래 후속 승인으로 코드 반영:** 범위는 `Tests/DCLyraRuntimeFoundationAutomationTests.cpp`와 plugin `Private/LoadingScreenManager.cpp` **2CPP**입니다. 잘못된 getter assertion을 그대로 통과시키려 원본 getter를 바꾸거나 실패 기대 처리하지 않습니다. 독립 tick probe에서 엔진의 실제 등록→tick→Never→미호출 계약을 검사하고, 실제 loading manager는 subsystem/task 수명 및 전역 delegate의 등록/해제를 확인하도록 교정합니다(완전한 화면/맵 이동 검증과 별개). 원본과 대상 모두 PreLoadMapWithContext에 등록하지만 PreLoadMap을 해제하므로, 종료 시 같은 PreLoadMapWithContext.RemoveAll(this)를 호출하는 최소1줄 수정과 IsBoundToObject 기반 전후 회귀를 함께 제안합니다. 원본 그대로 두는 대안은 shutdown 후 등록 해제를 보장하지 못하므로 권장하지 않으며, 동작 차이는 종료된 manager의 해당 callback을 즉시 제거하는 것입니다. 이 delegate 잔존은 이번 실행에서 직접 측정한 결과가 아니라 소스로 확인한 불일치입니다. 기존 로딩 표시/오디오/SettingsLocal/격리 정책은 유지하고 실제 맵 전환은 미검증으로 남깁니다. 다음은 이 교정 승인→코드만 수정→사용자 빌드→기본/진단 재검사이며 현 단계는 R5-3/R13 선행 검증 미완료입니다.
- **2026-10-06 Tick/delegate 교정 코드 작성 이력:** 사용자 승인한2CPP만 교정했습니다. LoadingScreenManager.cpp는 **PreLoadMapWithContext.RemoveAll(this)**로 해제 대상을 맞춘1줄 변경뿐이며 원본 getter/표시/오디오/진단 조건은 그대로입니다. 테스트 CPP에는 IsBoundToObject의 초기화/종료 전후 확인을 추가하고 잘못된 `GetTickableTickType()==Never` assertion을 제거했습니다. 독립 **TickRegistration** 검사는 GameInstance 없는 Inactive 임시 World에 dispatch를 한정하여 Never 초기 상태・대기 등록 취소・실제Tick 증가・해제 후 미호출・재등록/재해제를 관찰합니다. 초기 정책 getter는 계속 Conditional인 상태임도 확인하며, 실제 manager의 수명/delegate 검사와 엔진 Tick 등록 계약을 구분합니다. RuntimeFoundation은4→5건, 이전13건과 합쳐 각 모드18건 재검사 예정입니다. production1줄 diff/정적 검사 및 INI/프로젝트/Build/GameMode/LocalPlayer/진단helper9개 해시 불변을 확인했지만 **작성 당시 C++ 재빌드/새 검사 실행은 미검증이었으며 아래 재검사 결과가 최신**입니다. 빌드/패키징/에디터 실행은 하지 않았습니다. 다음은 사용자 Development Editor/Win64 빌드 후 기본/진단18건씩 재검사이며, 이전 진단1실패 결과를 현재 성공으로 덮어쓰지 않습니다. 실제 로딩 위젯/맵 이동/Experience 완료/라이플/복제는 여전히 미검증입니다.
- **2026-10-07 00:30 KST 실행 계층 독립 검증 통과 이력:** 사용자 재빌드 **00:21:04 시작・7.68초 성공**, 게임 DLL **00:21:11 / 3,730,432 bytes**, CommonLoadingScreen DLL **00:21:10 / 174,592 bytes**를 확인했습니다. **00:27:16 기본18/18, 00:30:40 진단18/18 모두 성공**(18종을2모드로 총36회 실행, 실패/보고서 경고/notRun/ensure/Fatal0). 근거 `Saved/Automation/R5RuntimeFoundationFixedDefault/index.json`, `R5RuntimeFoundationFixedEnabled/index.json`과 동명 Logs입니다. 독립 Tick 등록 계약 및 실제 manager delegate의 초기화/종료 전후 검사도 통과했으며 이전17건/진단1실패 보고서는 보존했습니다. 빌드/패키징은 Codex가 실행하지 않았고 이번에는 source/Config/에셋을 수정하지 않았습니다. 임시 진단 조건은 그대로이며 실제 위젯/맵 이동/오디오/Experience 완료/라이플/복제 전체 완료는 아닙니다. 현재 R5-3/R13 선행 준비의 독립 검사 통과 상태로 라이플 이식 준비에 복귀합니다.
- **승인 제안 이력 / 아래 후속 승인으로 코드 작성 — 라이플 native 연결 감사:** 원본 core18 inspect/reload와 native-plan 기록에서 관측한 LyraGame native28종 중 현재 정확한 Class/Struct/Enum Redirect는7종, 미등록은21종입니다(전체 필요 타입 전수 목록이 아님). 기존 도구는 FunctionOwner/핀 타입만 기록하고 함수명/인자/반환/변수 및 nested native 구조는 충분히 비교하지 않습니다. 원본 PlayerState.GetLyraPlayerController와 대상 GetDCPlayerController 이름 차이, 부분 포팅된 DCGameInstance 등이 있어 타입 존재만 보고21개를 일괄 Redirect하지 않습니다. 다음은 Editor 도구에 기존 복사 API와 분리된 읽기 전용 native contract 조회/API・synthetic 검사, `Scripts/Editor/r5_rifle_native_contract_audit.py`를 추가하는 묶음입니다. 대상은 검증된 Prepared core18＋남은 ShooterCore2＋B_LyraGameInstance/ABP_Mannequin_Base2의22개 및 필요한 native타입이며, 양쪽 함수를 실행하지 않고 class/struct/member/인자・반환/상속・참조를 수집합니다. 저장/Compile/복사/Config/활성 경로는 바꾸지 않으며 기존 copier18개 제한/보호 정책도 유지합니다. 결과 후 필요한 정확한 Redirect/호환 수정안을 별도로 제시합니다. 공용114개 덮어쓰기와 Struct_UIMessaging/GE_Damage_RifleAuto 연결도 아직 실행하지 않았습니다. 현재 추가 빌드는 불필요하며 이 새 도구 묶음 승인 후 진행합니다.
- **2026-10-07 native 연결 감사 도구 작성 이력:** 사용자 승인한 Editor 도구 header1・CPP2・Python1만 작성했습니다. 기존 복사 API/일반16・정확한18개 제한은 그대로이며 별도 읽기 전용 API로 함수명・소유자・인자 순서/반환・프로퍼티/컨테이너・delegate 서명・상속・핀 타입을 수집합니다. 원본 Prepared18＋추가4의 정확한22개만 에셋 조회 대상으로 허용하고 대상 프로젝트에서는 에셋을 로드하지 않고 이미 등록된 native 후보만 조회합니다. 미해석/미지원/한도 초과는 미검증으로 기록하며 CDO 생성・함수 실행・Compile・저장・Redirect 적용은 하지 않습니다. `Scripts/Editor/r5_rifle_native_contract_audit.py --self-test` **순수4건 성공**, AST/정적 검사・diff 검사 및 Config/uproject/Build/copier/진단helper 보호8개 해시 불변을 확인했습니다. **C++ 신규3건과 실제 원본/대상 감사는 아직 실행하지 않았으며 빌드/패키징/에디터 실행도 하지 않았습니다.** 다음은 사용자 Development Editor/Win64 빌드 후 `-DCRifleNativeContractAudit -DCRifleNativeContractTests`의 synthetic 검사와 기존 MigrationTools 회귀, 이후 별도 guarded commandlet의 source22/target native 보고서 비교입니다. 보고서는 새 `Saved/Diagnostics/R5NativeContracts/<Run>/` 아래에만 생성하고 덮어쓰기를 거부합니다. 후보 매핑은 Config 변경/전체 동등성 판정이 아니며 DCGameInstance 등 비반영 C++ 동작과 실제 라이플 이식/PIE/복제는 여전히 미검증입니다. AGENTS/명세 상태 기록 외 게임 C++・Config・에셋・활성 경로・Git 인덱스는 변경하지 않았습니다.
- **native 감사 C4456 교정 이력 / 아래 사용자 재빌드 성공:** 사용자 첨부 빌드 로그에서 `DCRifleNativeContractAudit.cpp`의 if/else-if 체인에 동일 지역변수 P를 반복 선언하여 C4456 오류가 발생한 것을 확인했습니다. 해당 CPP의15개 분기에서 변수명만 ArrayProperty/SetProperty 등 타입별 이름으로 변경했고, 이름을 역치환하면 작업 전 내용과 동일함을 정적 확인했습니다. 빌드 설정/경고 정책/리플렉션 동작/에셋/Config는 변경하지 않았습니다. 빌드·패키징·에디터 실행은 하지 않았으며 **재빌드 성공 및 C++3건/실제 감사는 미검증**입니다. 다음은 사용자 Development Editor/Win64 재빌드 후 기존 검증 절차 재개입니다.
- **2026-10-07 01:41 KST 검사 이력 / native 감사 검사7성공/1실패:** 사용자 재빌드는 **01:39:37 시작・6.53초 성공**, Editor 도구 DLL **01:39:43 / 508,928 bytes**로 확인했습니다. `Saved/Automation/R5NativeContractChecks/index.json` **01:41:41 KST** 결과는 기존5건＋신규 BlueprintReadOnly/NativeSchema 성공, **RejectedInputs 실패**입니다. 프로세스 종료0을 전체 성공으로 간주하지 않습니다. 실패는 테스트 CPP128행의 `NewObject<UObject>`가 Abstract 클래스를 생성하여 ensure1건을 일으킨 것입니다(해당 검사 경고2/오류26로그 기록, 별개26결함 아님). 엔진 Object.h의 UCLASS(Abstract), UObjectGlobals.cpp3295의 추상 생성 ensure와 callstack으로 확인했습니다. 실제 원본22개/대상 native 감사는 실행하지 않았습니다. 다음 미적용 제안은 **테스트 CPP1개**에서 비추상 `UEdGraph` fixture로 바꾸고 생성 성공을 확인하는 것입니다. 잘못된 입력 거부 의도와 production 감사 API는 유지하며 ensure를 허용/숨기지 않습니다. 사용자 승인 후 코드 교정→사용자 재빌드→전체8건 재검사→읽기 전용 실에셋 감사 순서입니다. 이번에는 검사/진단/상태 문서만 진행했고 C++/Config/에셋/활성 경로/인덱스는 변경하지 않았습니다(보호9개 SHA256 불변). 빌드/패키징은 Codex가 실행하지 않았으며 R5-3/R13 전체 완료도 아닙니다.
- **RejectedInputs fixture 교정 이력 / 아래 사용자 재빌드・8건 통과:** 승인한 `DCRifleNativeContractAuditTests.cpp`1개에서 추상 UObject 생성만 비추상 UEdGraph의 transient 생성으로 바꾸고 TestNotNull 실패 시 즉시 종료하도록 추가했습니다. 기존 거부 assertion4개와 실행 조건은 유지했고 ensure를 허용하지 않았습니다. 작업 전후 비교로 해당 fixture 변경 외 CPP 내용 동일, 감사 API/헤더/Build/스크립트/Config/uproject/인덱스/기존 실패 보고서 보호9개 SHA256 불변 및 diff 검사를 확인했습니다. AGENTS/명세 상태 기록 외 코드 수정은 테스트 CPP뿐입니다. 빌드/패키징/에디터 실행은 하지 않았으며 **교정 코드의 빌드・전체8건 재실행・실제 source22/target 감사는 미검증**입니다. 다음은 사용자 Development Editor/Win64 재빌드 후 새 보고서로 전체8건 재검사입니다. 기존7성공/1실패 결과는 보존합니다.
- **2026-10-07 01:52 KST 검사 이력 / 감사 도구8건 통과・원본/대상 보고서 확보:** 사용자 재빌드 **01:48:11 시작・5.95초 성공**, 도구 DLL **01:48:17 / 508,928 bytes**를 확인했습니다. `Saved/Automation/R5NativeContractChecksFixed/index.json` **01:50:05 KST 전체8건 성공・실패/경고/오류/notRun/ensure0**이며 이전7성공/1실패 보고서는 보존했습니다. `R5NativeContractSource.log` **01:51:47**, `R5NativeContractTarget.log` **01:52:37**는 종료0/complete, 에셋 쓰기0/ensure0입니다. `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261007_0150/`에 원본22개・native84종(에셋 직접참조34종＋반영 구조 참조 확장)과 대상59종 보고서를 확보했습니다. 비교는 **구조 일치33/차이26/후보 미등록25**이며 기능 동등성/전체 이식 완료가 아닙니다. 원본의 W_Reticle_Rifle/W_AmmoCounter_Rifle/WeaponAudioFunctions3개는 Make/Break/SetFieldsInStruct를 일반 VariableReference로 읽은 도구 분류 문제로 불완전합니다(엔진 StructOperation→Variable 상속 확인). 원본/동반192개 상태와 대상 보호8개 해시 불변, 알려진 Niagara load-dirty4개만 관측했습니다. 원본의 기존 Death cue 중복 경고는 이전 로그에서도 확인됐으며 무경고 실행으로 보고하지 않습니다. 25미등록 중20개 대응 선언은 소스에서 확인했으나 후보 목록에 빠졌고5개는 대응 미확인입니다. **다음 미적용 제안은 감사CPP/검사CPP/Python3파일:** 특정 struct operation 노드를 StructType/핀으로 읽고 회귀검사하며 확인20개 후보만 추가합니다(Config Redirect가 아님). 원시 flag/delegate이름 차이와 Character/Controller/PlayerState/부분GameInstance의 실제 차이를 구분하고, 미확인5종을 추정 매핑하거나 코드를 대신 만들지 않습니다. 이번에는 검사/읽기 전용 조회/상태 문서만 진행했고 빌드/패키징/C++/Config/에셋/활성 경로/인덱스 수정은 없었습니다. 현재 R5-3 라이플 이식 준비이며 PIE/복제/전체 Migrate는 미완료입니다.
- **struct 노드・후보20종 코드 작성 이력 / 아래 빌드・실행 검증 통과:** 승인한 감사CPP/검사CPP/Python3파일을 수정했습니다. Make/Break 및 Make를 상속한 SetFieldsInStruct만 StructType/반영 멤버/핀으로 읽고, 일반 변수 조회와 실제 누락 struct/변수의 미검증 판정은 유지합니다. transient Make/Break/SetFields fixture로 타입/멤버/핀 조회・캐시 핀이 남은 누락StructType3종・잘못된 변수・노드/핀/dirty/상태 무변경을 검사하는 C++ StructOperations1건을 추가했습니다(기존8＋신규1＝전체9건 예정). Python은 명시적 검토20개 후보만 추가하고 잘못된 과거 enum 철자를 실제 ECharacterCustomizationCollisionMode로 교정했습니다. 미확인5종은 매핑하지 않았으며 raw flag/delegate 이름 차이는 그대로입니다. **순수 Python8건/AST/diff 검사 통과**, 보호11개 SHA256 불변을 확인했지만 **C++ 빌드/신규 검사/실에셋 재감사는 미실행**입니다. 이전8건 통과 및 NativeAudit_20261007_0150 보고서는 보존했고 새 코드 성공으로 대체하지 않았습니다. 문서 외 게임 C++/헤더 API/Build.cs/Config/에셋/활성 경로/인덱스는 그대로이며 빌드/패키징/에디터 실행은 하지 않았습니다. 다음은 사용자 Development Editor/Win64 빌드 후9건 회귀→새 source22/target 보고서 감사입니다.
- **2026-10-07 02:36 KST 검사 이력 / 도구9건・source22개 조회 통과:** 사용자 빌드 **02:32:46 시작・9.40초 성공**, 도구 DLL **02:32:55 / 524,800 bytes**를 확인했습니다. `Saved/Automation/R5NativeStructChecks/index.json` **02:34:08 KST 전체9건 성공・실패/경고/오류/notRun/ensure0**입니다. `R5NativeStructSource.log` **02:35:16**, `R5NativeStructTarget.log` **02:36:40**는 종료0/complete/쓰기0/ensure0입니다. 새 보고서 `Saved/Diagnostics/R5NativeContracts/NativeAudit_20261007_0235/`에서 source22개 모두 불완전 메시지 없이 조회했고 native84/target79종 비교는 **52일치/27차이/5대응 미확인**입니다(값/동작/전체 이식 동등성 아님). 원본/동반192개와 대상 보호8개 해시 불변, 알려진 Niagara dirty4개, 원본 기존 Death cue 중복 경고를 유지했습니다. 미확인5종 중 W_Reticle_Rifle.WidgetTree.HitMarkerConfirmations가 UHitMarkerConfirmationWidget을 직접 소유하며 나머지4종은 PlayerController/PlayerState/Character 반영 참조에서 확장됐습니다. 다음 미적용 제안은 **R5-3/R7 선행 HitMarker UMG/Slate 원본4파일 이식＋검사CPP1＋ClassRedirect1개(Config파일1)＋감사후보갱신(Python1)**입니다. 기존 DCLyraWeaponState의 화면위치/경과시간 API를 재사용합니다. 원본 UMG는 HitNotifyDuration을 넘기지만 Slate Construct는 저장하지 않아 내부0.4초를 사용함을 소스로 확인했습니다. 원문 유지 대안은 그 고정 동작을 유지하며, 제안은 생성 시 전달 기간을 저장하는 최소 보정으로 기본0.4는 유지하고 사용자 지정 기간을 반영하는 것입니다. 나머지 그리기/색상/zone/감쇠 계산은 원본 유지하며 실제 화면/히트/복제는 후속 검증입니다. 이번에는 코드/Config/에셋/활성 경로/인덱스를 수정하지 않았고 빌드/패키징도 실행하지 않았습니다. 다음 이식 묶음은 승인 대기이며 다른4타입 추정 매핑・전체 Redirect/Migrate/플레이어 교체는 포함하지 않습니다.
- **HitMarker 코드 작성 이력 / 아래 사용자 빌드・검사 통과:** 승인된7파일을 작업했습니다. `UI/Weapons/Lyra/DCLyraHitMarkerConfirmationWidget.{h,cpp}`/`SDCLyraHitMarkerConfirmationWidget.{h,cpp}`4개는 원본명/경로/WeaponState 타입・명시 include와 UMG export만 맞췄고, Slate Construct에 기간 인자 저장1줄을 보정했습니다(기본0.4초 유지). OnPaint/ComputeDesiredSize/Tick은 타입명 치환 외 원본 동일함을 확인했습니다. `Tests/DCHitMarkerAutomationTests.cpp`의 SlateArguments/WidgetLifecycle/DependencyRegistration3건은 transient context 없는 기본값/기간・brush 전달/자원수명/Redirect 검사용이며 미실행입니다. Config는 HitMarker ClassRedirect1개와 설명만 추가했고 나머지 내용의 정규화 해시 동일을 확인했습니다. 감사 Python 후보1개와 미확인4종 검사 갱신 후 **순수9건/AST/diff 검사 통과**, 원본4파일 포함 보호12개 SHA256 불변입니다. 기존 실패/성공 보고서는 보존했고 C++ 빌드/패키징/에디터/에셋 실행은 하지 않았습니다. 다음은 사용자 Development Editor/Win64 빌드→HitMarker3＋도구9＋기존Circumference3 회귀→새 native 감사입니다. 실제 HUD 연결/화면/명중/zone/복제 및 전체 이식은 미검증이며 기존 경로/에셋은 유지합니다.
- **2026-10-07 03:07 KST 검사 이력 / HitMarker 독립 검증 통과:** 사용자 빌드 **02:59:56 시작・22.41초 성공**, 게임 DLL **03:00:17 / 3,774,464 bytes**를 확인했습니다. `Saved/Automation/R5HitMarkerChecks/index.json` **03:01:48 KST** HitMarker3＋도구9＋Circumference3 **총15건 성공・실패/경고/오류/notRun/ensure0**입니다. source **03:04:22**, target **03:07:22**의 `R5HitMarkerNativeSource/Target.log`와 `NativeAudit_20261007_0302/`는 종료0/complete/쓰기0/ensure0, source22/native84・target80 조회 및 **52일치/28차이/4후보미확인**을 기록합니다. HitMarker는 등록/반영 멤버 조회가 됐으며 원시 차이는 CLASS_RequiredAPI export flag1개입니다. 실제 화면/명중/복제 동등성은 아닙니다. 원본/동반192개・대상보호12개 해시 불변, 알려진Niagara dirty4개와 기존원본Death cue 경고는 유지했습니다. 다음 미적용 제안은 **Config/DefaultEngine.ini＋Tests/DCRifleNativeRegistrationTests.cpp의2파일**입니다. 명세 표의 기존 핵심 Inventory/Equipment/Weapon/Ability 클래스21・구조체9의 정확한 Redirect30개와 동일 ASC getter의 원본/클래스명 전환 후 이름을 처리하는 FunctionRedirect2개만 추가・검사합니다. 모두 현재source/target 조회 완료・미등록이며, 클래스flag/상속getter이름/대상ASC추가함수 차이를 숨기지 않습니다. 현재Character/Controller Redirect는 그대로 두고 PlayerState/GameInstance/GameState 등 부분 계층을 새로 연결하거나4미확인타입을 추정하지 않습니다. 이번에는 검사/읽기전용조회/상태문서만 진행했고 C++/Config/에셋/인덱스/활성경로 및 빌드/패키징은 변경・실행하지 않았습니다. R5-3/R7 선행 준비이며 다음2파일은 승인 대기입니다.
- **핵심 Redirect32개 코드 작성 이력 / 아래 사용자 빌드・22건 통과:** 승인된 Config/DefaultEngine.ini와 새 Tests/DCRifleNativeRegistrationTests.cpp2파일을 작업했습니다. 명세 그대로 Class21/Struct9/동일ASCgetter의Function별칭2를 추가했고, CPP의 Classes/Structs/ASCGetterAliases/TypeLinks4건은 고정 대상의 경로/종류・fixup 후 로드・함수반환형/인자・부모에서 상속한 별칭 조회・장비 관련 속성을 검사하도록 작성했습니다. 게임 함수 실행/객체생성/Blueprint 로드/저장은 없으며 잘못된 이름 해석은 로드 전에 실패로 기록하도록 했습니다. 승인32개가 각각1회 존재・중복/충돌0・CPP의32개쌍 일치・추가블록 외 Config 정규화 해시 동일・보호9개 불변・diff 검사와 기존Python9건 통과를 확인했습니다. **C++ 빌드/신규4건 실행/native 재감사는 아직 미검증**이며 빌드/패키징/에디터 실행은 하지 않았습니다. 기존 게임 함수본문/플레이어계층/에셋/활성경로/인덱스/기존보고서는 유지했습니다. 다음은 사용자 Development Editor/Win64 빌드→신규4＋기존15＋Equipment1/Weapon2의22건 회귀→새 native 감사입니다. R5-3/R7 선행 준비이며 전체이식/PIE/복제 완료가 아닙니다.
- **2026-10-07 23:31 KST 검사 이력 / 핵심 등록32개 검증 통과:** 사용자 빌드 **03:41:27 시작・9.05초 성공**, 게임 DLL **03:41:36 / 3,801,600 bytes**를 확인했고, `Saved/Automation/R5NativeRegistrationChecks/index.json`의 **23:12:59 KST 총22건 성공・실패/경고/오류/notRun/ensure0**을 확인했습니다. 오전 빌드와 밤 검사 시각을 구분합니다. 원본/source **23:25:40**, 대상/target **23:31:18**의 `R5RegisteredNativeSource/Target.log`는 종료0/complete/쓰기0/ensure0이며 `NativeAudit_20261007_2314/`에 source22/native84・target80, 원시 **52일치/28차이/4후보미확인** 보고서를 보존했습니다(폴더명2314는 run 식별자). 원본/동반192개와 보호9개 불변, 기존 Niagara dirty4개/Death cue 경고는 유지됩니다. 다음 미적용 제안은 Config/DefaultEngine.ini와 새 Tests/DCRiflePresentationRegistrationTests.cpp2파일로, 명세의 Health/CharacterParts/Team/메시지 타입16개(Class4/Struct10/Enum2)와 관련 package-level delegate 서명 FunctionRedirect4개의 **정확한20개**만 등록・검사하는 것입니다. 양쪽 타입 조회 및 delegate 인자/타입/flags의 이름 대응 일치를 확인했지만 전역 delegate Redirect 로드/Blueprint 사용은 후속 미검증입니다. PlayerState/GameState/GameInstance/AnimInstance 등 나머지 계층・팀 인덱스 delegate・미확인4종은 이번 범위 밖입니다. 함수본문/에셋/활성경로/인덱스 수정이나 빌드/패키징은 이번에 하지 않았습니다. R5-3/R7 준비 중이고 실사용 라이플/PIE/복제는 미완료이므로 10/8 장비・발사・재장전 체크포인트에 지연 위험이 있습니다. 다음2파일은 승인 대기입니다.
- **시각・상태20개 코드 작성 이력 / 아래 사용자 빌드・27건 통과:** 승인한 Config/DefaultEngine.ini와 새 Tests/DCRiflePresentationRegistrationTests.cpp2파일을 작업했습니다. Class4/Struct10/Enum2/package-level delegate signature Function4의20개만 추가했습니다. NativeTypes/EnumValues/DelegateSignatures/DelegateProperties4건은 fixup 후 대상/종류 로드, enum 이름/값/sentinel, multicast 서명flags・인자명/순서/타입/const・ref/out 한정자,6개 멤버delegate property와4개 signature 연결을 검사하도록 작성했습니다. 게임 객체/CDO 생성・delegate broadcast・에셋/Blueprint 저장은 없습니다. 승인20개 각각1회/충돌0/CPP20쌍 일치/추가블록 외 Config 정규화 해시 동일・보호13개 불변・diff 및 기존Python9건 통과를 확인했습니다. **C++ 빌드/신규4건 실행/실제 global signature Redirect 사용/native 재감사는 아직 미검증**입니다. Codex는 빌드/패키징/에디터를 실행하지 않았습니다. 기존 함수본문・기존 등록32개・플레이어 계층・에셋・활성경로・인덱스/보고서를 보존했습니다. 다음은 사용자 Development Editor/Win64 빌드→신규4＋기존22＋TeamColor1의27건 회귀→새 native 감사입니다. 전체라이플 이식/실사용/PIE/복제는 미완료이며 기존 일정 지연 위험을 유지합니다.
- **2026-10-08 00:55 KST 시각・상태20개 검증 통과:** 사용자 빌드 **00:47:31 시작・27.31초 성공**, 게임 DLL **00:47:57 / 3,826,688 bytes**를 확인했습니다. `Saved/Automation/R5PresentationRegistrationChecks/index.json`의 **00:49:47 KST 총27건 성공・실패/경고/오류/notRun/ensure0**으로 enum 값과4개 package-level delegate 서명/6개 property 연결까지 통과했습니다. `R5PresentationNativeSource/Target.log` source **00:52:13**, target **00:55:12**는 종료0/complete/쓰기0/ensure0이며 `NativeAudit_20261008_0050/`에 source22/native84・target80, 원시52일치/28차이/4후보미확인 보고서를 보존했습니다. 비교기는 property 안의 delegate signature 경로를 FunctionRedirect로 정규화하지 않아 이름 차이도 원시차이에 남으며, 별도 등록검사 통과와 구분합니다. 원본192개・대상보호10개 해시 불변, 기존 Niagara dirty4개/Death cue 경고는 유지됩니다. 다음 미적용 제안은 Config/DefaultEngine.ini와 새 Tests/DCRifleAnimationRegistrationTests.cpp2파일로 **LyraAnimInstance→DCAnimInstance, LyraCharacterMovementComponent→DCCharacterMovementComponent, LyraCharacterGroundInfo→DCCharacterGroundInfo**3개(Class2/Struct1)만 등록・검사합니다. AnimInstance/Movement CPP는 이름/경로/CVar・stat 이름 치환 외 원본과 같고 현재Character 생성자의 이동컴포넌트 선택도 확인했습니다. 실제 AnimBP/메시/애니메이션/ground trace/복제는 별도입니다. GameInstance는 원본의 세션/Shutdown/암호화 경로가 빠져 있고 PlayerController 부모도 원본CommonPC와 달라 일괄 연결하지 않습니다. 이번에는 상태 문서 외 코드/Config/에셋/활성 경로/인덱스 및 빌드/패키징 변경・실행이 없었습니다. R5-3/R7/R8 선행 준비이며 실사용 라이플과10/8 체크포인트 지연 위험은 해소되지 않았습니다. 이때 제안했던2파일은 아래 후속 승인 범위로 적용했습니다.
- **애니메이션 기반 이름 연결 코드 작성 이력 / 아래 사용자 빌드・30건 통과:** 승인한 `Config/DefaultEngine.ini`와 새 `Tests/DCRifleAnimationRegistrationTests.cpp`2파일을 반영했습니다. 원본 LyraGame의 AnimInstance/CharacterMovementComponent/CharacterGroundInfo를 기존 DC 타입에 연결하는 Class2/Struct1의3개 Redirect만 추가했고 기존 DreamCatcher 모듈 movement alias는 유지했습니다. `DreamCatcher.R5.AnimationRegistration`의 NativeTypes/ReflectedContract/OwnerlessGroundCache3건을 작성했습니다. exact Redirect・fixup 후 native 경로 로드, 상속/반영 속성 타입・flags, animation CDO의 GroundDistance -1과 ground-info 기본값0, owner/world 없는 transient 이동컴포넌트의 반복 cache 조회를 검사합니다. 원본 기반 구현 본문・CVar・Pawn/mesh/AnimBP・활성 경로는 바꾸지 않았습니다. 정적 검사에서3개 INI/CPP쌍・중복키0・추가블록 외 Config 정규화 해시 동일, 보호11개 파일 해시 불변, 기존 Python 감사 self-test9건 통과를 확인했습니다. **작성 당시 새 C++3건의 빌드・실행・실제 Redirect 적용은 미검증**이며 이번에는 빌드/패키징/에디터를 실행하지 않았습니다. 이전27건 성공/NativeAudit_20261008_0050 보고서는 이전 코드 기준 증거로 보존합니다. 다음은 사용자 Development Editor/Win64 빌드→신규3＋기존27의30건 회귀→읽기 전용 native 재감사입니다. 실제 AnimBP/trace/재생/라이플/PIE/복제와10/8 지연 위험은 미해결이며 R5-3/R8 선행 준비 상태입니다.
- **2026-10-08 01:29 KST 애니메이션 기반 검증 통과:** 사용자 UBT 빌드는 **01:24:40 시작・21.40초 성공**, 게임 DLL **01:25:01 / 3,843,072 bytes**입니다. `Saved/Automation/R5AnimationRegistrationChecks/index.json`의 **01:27:41 KST 신규3＋기존27 총30건 성공**, 실패/보고서 경고/오류/notRun0 및 로그 ensure/Fatal0, 프로세스 종료0을 확인했습니다. native 경로/상속/속성/default와 ownerless cache 검증이며 실제 AnimBP/mesh/trace/재생 검증은 아닙니다. `R5AnimationNativeSource.log` **01:28:38**, `R5AnimationNativeTarget.log` **01:29:53**은 종료0/complete/에셋쓰기0/ensure0이며 `NativeAudit_20261008_0128/`에 source22/native84・target80, 원시52일치/28차이/4후보미확인 보고서를 저장했습니다. 이름 등록 성공과 전체 동작/원시flags・delegate 이름 일치를 혼동하지 않습니다. 원본/동반192개와 대상보호12개 상태가 불변이며 기존 Niagara load-dirty4개/Death cue 중복 경고도 그대로입니다. 이번 작업은 검증・원본 조회・상태 문서 갱신이며 게임 C++/Config/에셋/인덱스와 빌드/패키징은 변경・실행하지 않았습니다. 이때의 **제안(아래 후속 승인으로 적용)**은 원본 HUD＋AddWidgets 연결 기반7파일: `UI/Lyra/DCLyraHUD.h/.cpp`, `GameFeatures/DCGameFeatureAction_AddWidget.h/.cpp`, `Tests/DCHUDFoundationAutomationTests.cpp`, Build.cs의 UIExtension 직접의존성, Config의Class3/Struct2 Redirect5개입니다. 기존 WorldActionBase는 header 동일/CPP Engine include 외 원본 동일로 재사용합니다. HUD 수명/위젯 등록・해제 본문을 원본대로 유지하고 이름/module/include만 대응합니다. HUDLayout/실제 Widget/Experience 에셋・GameMode HUDClass・활성 Pawn/Controller 교체는 제외합니다. 원본 AddWidgets는 이미 로드된 soft class와 LocalPlayer/UI layer/extension을 전제로 하므로 새 임시 로딩/위젯을 끼워 넣지 않으며 화면 검증은 후속입니다. HUD가22개 직접참조가 아니라 PlayerController.GetLyraHUD의 확장 의존성이라는 점도 구분합니다. R5-3/R7/R13 선행 준비 중이며 실사용라이플/PIE/복제・10/8 일정 위험은 미해결입니다.
- **HUD・AddWidgets7파일 작성 이력 / 아래 사용자 빌드・34건 통과:** 승인한 `UI/Lyra/DCLyraHUD.h/.cpp`, `GameFeatures/DCGameFeatureAction_AddWidget.h/.cpp`, `Tests/DCHUDFoundationAutomationTests.cpp`, Build.cs, Config를 반영했습니다. 원본4파일은 클래스/구조체/파일명・HUD include경로・명시적 include6개・줄끝 공백 정규화 후 전체 내용이 일치하며 원본 수명/Client bundle/검증/위젯 처리 본문은 유지했습니다. 기존 WorldActionBase와 플레이어 본문은 그대로이고 Build.cs의 Public UIExtension1개 및 Config Class3/Struct2의5개만 추가했습니다. `DreamCatcher.R5.HUDFoundation`의 RegistrationAndDefaults/ActionDataAndBundles/HUDReceiverLifecycle/EmptyActionLifecycle4건을 작성했습니다. native/default・transient 데이터 검사와 ownerless HUD/빈 Action의 격리 월드 수명 검사이며 LocalPlayer/위젯/viewport/PIE/복제를 생성하거나 검증했다고 주장하지 않습니다. 수명 검사는 별도 unattended Editor＋`-DCR5HUDIsolatedAutomation`을 요구하며 `-DCLyraRuntimeDiagnostics`와 함께 실행하면 거부합니다. fixture 소유 handler와 Action delegate만 정리하고 원본 private 배열은 transient 객체 반영 속성으로만 검사하며 production API를 넓히지 않았습니다. 정적 원본 비교4개・INI/CPP5쌍/중복0・Config/Build 추가분 외 정규화 해시 동일・기존보호11개 불변・diff 및 기존Python9건을 통과했습니다. **작성 당시 새 C++/UHT 빌드와 검사4건 실행은 미검증**이며 Codex는 빌드/패키징/에디터를 실행하지 않았습니다. 다음은 사용자 Development Editor/Win64 빌드→신규4＋기존30 총34건 검사→후속 실제 UI/플레이어 연결안입니다. 실제 HUDClass/Experience 에셋・LocalPlayer/레이어/원본 Widget 로드는 후속이고 R5-3/R7/R13 전체 완료나10/8 일정 위험 해소가 아닙니다.
- **2026-10-08 18:24 KST HUD 기반 검증 통과:** 사용자 UBT 빌드는 **02:28:32 시작・71.53초 성공**, 게임 DLL **02:29:42 / 3,922,432 bytes**이며 UHT와 HUD/AddWidget/검사CPP 컴파일을 확인했습니다. `Saved/Automation/R5HUDFoundationChecks/index.json` **18:24:38 KST 신규4＋기존30 총34건 성공・실패/보고서 경고/오류/notRun0**이고 동명 로그 Error/ensure/Fatal0, 프로세스 종료0입니다. HUD receiver/ready/remove와 빈 Action의2회 수명/delegate 해제까지 통과했지만 실제 Widget/LocalPlayer/화면/plugin 활성화/PIE/복제 성공은 아닙니다. 보호15개(Config/프로젝트/인덱스/원본 기반코드/스크립트)의 SHA256 불변을 확인했습니다. 이번에는 검증/원본 읽기 전용 조사/상태 문서만 작업했고 게임 C++/Config/에셋/활성 경로는 수정하지 않았으며 빌드/패키징도 실행하지 않았습니다. 전체 native 비교기는 재실행하지 않았고 기존 NativeAudit_20261008_0128은 이전 코드 기준 보고서로 유지합니다. 이때의 **제안(아래 후속 승인으로 적용)**은 `UI/Lyra/DCLyraActivatableWidget`・`UI/Lyra/DCLyraHUDLayout`・`UI/Foundation/DCLyraControllerDisconnectedScreen` h/cpp6개＋`Tests/DCHUDLayoutFoundationAutomationTests.cpp`＋Config의Class3/Enum1 Redirect4개, 총8파일입니다. 원본 입력정책/Focus 경고/Escape/장치 delegate・ticker/플랫폼 분기/BindWidget 이름을 보존하고 타입/include/module・LogLyra→LogDC만 대응합니다. 일반 객체로 abstract 위젯을 만들지 않고 native 등록/enum/반영 메타데이터/CDO 기본값과 Default 입력정책만 검사합니다. 원본 DefaultInput.ini의 UI.Action.Escape 매핑은 대상 Config 검색에서 없었고 실제 UI에는 LocalPlayer/Menu layer/EscapeMenuClass/2개 BindWidget도 필요합니다. 이들은 코드8파일만으로 완성되지 않으며 후속 원본 설정/에셋 연결로 검증합니다. 키매핑/플랫폼 태그 설정/Widget 에셋/활성 HUD 교체는 이번 제안에서 제외하며 원본 미이식 불가가 아니라 미검증으로 남깁니다. 실제 게임패드가 없어 장치 동작은 여전히 보류입니다. R5-3/R7/R13 선행 준비 중이며 실제 라이플/발사/재장전과10/8 체크포인트 위험은 아직 해소되지 않았습니다.
- **HUD 화면 기반8파일 작성 이력 / 아래 사용자 빌드・37건 통과:** 승인한 `UI/Lyra/DCLyraActivatableWidget.h/.cpp`, `UI/Lyra/DCLyraHUDLayout.h/.cpp`, `UI/Foundation/DCLyraControllerDisconnectedScreen.h/.cpp`, `Tests/DCHUDLayoutFoundationAutomationTests.cpp`, Config를 작성했습니다. 원본6파일은 타입/파일/include 경로・LogLyra→LogDC・명시적 include6개・공백 정규화 후 전체 내용이 일치하며 입력모드/Focus 경고/Escape/장치 delegate・ticker/BindWidget/플랫폼 분기 및 사용자 변경 TODO를 보존했습니다. Config에는 Class3/Enum1 Redirect4개만 추가했습니다. `DreamCatcher.R5.HUDLayoutFoundation`의 RegistrationAndEnum/ReflectedDefaults/DefaultInputPolicy3건은 native 등록/enum/상속, 반영 속성/BindWidget/CDO 기본값, Default 입력정책의 unset 반환만 검사하도록 작성했습니다. abstract 일반 인스턴스 생성・CDO 수정・실제 위젯/장치/메뉴 활성화는 하지 않습니다. 정적 원본 비교6개・INI/CPP4쌍 일치/중복0・추가분 외 Config 정규화 해시 동일・보호14개 불변・diff/whitespace・기존 감사 Python9건을 통과했습니다. **작성 당시 새 UHT/C++ 빌드와 검사3건 실행은 미검증**이며 빌드/패키징/에디터 실행은 하지 않았습니다. DefaultInput/DefaultGame/Build/uproject/플레이어・UIManager・HUD/AddWidgets/에셋/인덱스 및 이전34건 보고서를 보존했습니다. 다음은 사용자 에디터 종료 후 Development Editor/Win64 빌드→신규3＋기존34 총37건 검사입니다. 실제 Escape 키매핑/LocalPlayer/Menu layer/원본 Widget・BindWidget 연결, 다른 입력모드의 활성 전환/포커스/장치 재연결/플랫폼 사용자 변경/라이플/PIE/복제는 후속입니다. 현재 R5-3/R7/R13 선행 준비이며10/8 체크포인트 위험은 여전히 남습니다.
- **2026-10-08 19:03 KST HUD 화면 기반 검증 통과:** 사용자 UBT는 **19:00:33 시작・38.16초 성공**, 게임 DLL **19:01:11 / 4,004,352 bytes**입니다. `Saved/Automation/R5HUDLayoutFoundationChecks/index.json` **19:03:25 KST 신규3＋기존34 총37건 성공・실패/보고서 경고/오류/notRun0**, 동명 로그 Error/ensure/Fatal0 및 프로세스 종료0을 확인했습니다. 등록/enum/상속/BindWidget 메타데이터/CDO 기본값/Default 입력정책 범위이며 실제 Widget/메뉴/키입력/장치 동작/PIE/복제는 미검증입니다. 보호16개 SHA256 불변이며 이번에는 검사・원본 소스/설정/파일 존재 확인・상태 문서만 진행했습니다. C++/Config/에셋/인덱스는 수정하지 않았고 빌드/패키징은 실행하지 않았습니다. 이때의 **제안(아래 후속 승인으로 적용)**은 `Scripts/Editor/r5_ui_source_audit.py`1파일 작성과 별도 원본 commandlet의 읽기 전용 조사입니다. 원본 UI policy/root/default・Shooter HUD/StandardHUD ActionSet/ReticleHost/AmmoAndName/QuickBar/QuickBarSlot/Disconnected 총10개 경로의 파일 존재만 확인했고 실제 부모/값/그래프 연결은 아직 조사하지 않았습니다. 공개 Python API로 선택10개만 조회해 정책/레이어/Action 배열/CDO/WidgetTree/그래프핀・참조/원본 설정・대상 같은경로 파일 존재를 기록할 계획입니다. API 미지원/한도 초과/누락/새 dirty/ensure/Error는 미검증 또는 실패로 남기고 Compile/복사/저장/활성화를 하지 않습니다. 기존 rifle 감사 스크립트는 import 시 main이 실행되므로 그대로 import하지 않고 필요한 읽기 패턴만 새 스크립트에 제한적으로 사용합니다. 새 보고서는 Saved/Diagnostics/R5UIFoundation/<fresh run>/에만 생성하며 원본 파일/동반 상태와 대상 보호 파일을 전후 확인합니다. 조사에는 추가 C++ 빌드가 필요하지 않지만 이식/원본 UI 표시 성공은 아닙니다. 현재 R5-3/R7/R13 선행 준비이며 실제 라이플/발사/재장전・10/8 체크포인트 위험은 미해결입니다.
- **2026-10-08 19:49 KST 원본 UI 조사・부분 완료:** 승인한 `Scripts/Editor/r5_ui_source_audit.py`1파일을 작성했고 자체검사10건 통과 후 원본 별도 commandlet을2회 실행했습니다. A1은19:24:50 KST에10개/17그래프/256노드/774핀/568연결을 조회했고 WidgetTree protected 접근 제한을 확인했습니다. A2는 공개 ObjectIterator의 정확한 Blueprint outer 필터와 UWidget::GetParent const getter로89개 source-owned 템플릿을 추가 조회했습니다. `Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A2/summary.json` **19:49:55 KST status=partial**, 실패0/미검증15(WidgetTree 직접포인터8・CDO BindWidget멤버2・주석노드핀5), 에셋쓰기/dirty/ensure/Error0・종료0입니다. 참조 metadata544 package 중 Game/ShooterCore542개의파일/동반2710개 및 대상보호14개가불변이며 같은상대경로52개 존재는 내용동일 판정이 아닙니다. 실제 policy→W_OverallUILayout, Game/GameMenu/Menu/Modal4레이어, StandardHUD의ShooterGameLayout1＋Widget11, ShooterHUD ExtensionPoint14개를 확인했습니다. 원본HUD BP의 InputConfig=GameAndMenu/CapturePermanently와 실제 EscapeMenuClass=W_LyraGameMenu도 확인했습니다. 새 누락native 부모는 WeaponReticleHost의LyraWeaponUserInterface 및 Ammo/QuickBar/Slot의LyraTaggedWidget이고 DefaultHUD template의IndicatorLayer도미준비입니다. StandardHUD는 GameFeaturesToEnable=[ShooterCore]이며 대상프로젝트plugin목록에서없음을확인했으나 flag를삭제/대체하지 않았습니다. A1/A2 및 미검증을보존하며 직접protected필드/런타임BindWidget/상속tree/시각동등성/PIE/복제는미검증입니다. 이번에는 C++/Config/에셋/기존도구/인덱스 미수정, 빌드/패키징 미실행입니다. 이때의 제안(아래 후속 승인으로 적용)은 실제parent2종 원본h/cpp4＋등록/default/event검사1＋Config ClassRedirect2개, 총6파일입니다. 원본TaggedWidget의태그감시/TODO 및 bHasHiddenTags=false, WeaponUI의Tick검색/유효instigator조건/빈Rebuild/무기없을때null통지부재는 그대로보존하고 기능완성으로오인하지 않습니다. IndicatorLayer와ShooterCore/나머지위젯/공유52개/활성플레이어연결은 후속범위이며 R5-3/R7/R13/10월8일 실제장비・발사 체크포인트는 미완료입니다.
- **무기 UI 부모2종6파일 작성 이력 / 아래 사용자 빌드・40건 통과:** 승인한 `UI/Lyra/DCLyraTaggedWidget.h/.cpp`, `UI/Weapons/Lyra/DCLyraWeaponUserInterface.h/.cpp`, `Tests/DCRifleWidgetFoundationAutomationTests.cpp`, Config를 작성했습니다. 원본4파일은 타입/파일명・장비/무기 include경로・SlateWrapperTypes 명시include1개・공백 정규화 후 전체가 일치합니다. TaggedWidget의 태그감시TODO/bHasHiddenTags=false와 WeaponUI의 기존 EquipmentManager Tick조회/유효instigator조건/빈Rebuild/무기없을때null알림부재를 유지했습니다. Config ClassRedirect2개만 추가했고 기존 장비/무기/플레이어 본문과 Build/입력/에셋/활성HUD는 미변경입니다. `DreamCatcher.R5.RifleWidgetFoundation` RegistrationAndInheritance/TaggedWidgetDefaults/WeaponEventAndDefaults3건을 작성했으며 native/CDO/반영 이벤트 인자 검사뿐이고 일반abstract 객체 생성/CDO수정/Tick・event호출/화면표시/장비교체는 하지 않습니다. 원본4개 정규화 비교・INI/CPP2쌍/중복0・추가분 외 Config 정규화 SHA256동일・기존보호14개 불변・diff/whitespace 및 기존Python9건이 통과했습니다. **작성 당시 새 UHT/C++ 빌드와 검사3건 실행은 미검증**이며 빌드/패키징/에디터를 실행하지 않았습니다. 다음은 사용자 에디터 종료 후 Development Editor/Win64 빌드→신규3＋기존37 총40건 검사입니다. UIAudit_A2의미검증15와IndicatorLayer/ShooterCore/공유52개/원본에셋・실제플레이어연결은 후속으로 보존하며 R5-3/R7/R13 및10/8 실제장비・발사 체크포인트는 미완료입니다.
- **2026-10-08 20:14 KST 무기 UI 부모 검증 통과:** 사용자 UBT는 **20:11:34 시작・28.20초 성공**, 게임 DLL **20:12:01 / 4,035,072 bytes**이며 UHT/TaggedWidget/WeaponUserInterface/검사CPP 컴파일을 확인했습니다. `Saved/Automation/R5RifleWidgetFoundationChecks/index.json` **20:14:25 KST 신규3＋기존37 총40건 성공・실패/보고서 경고/오류/notRun0**, 동명로그 Error/ensure/Fatal0 및 종료0입니다. native/CDO/이벤트 계약만 검증했고 태그자동숨김/실제Tick・장비전환/화면/복제는 미검증입니다. 보호15개 SHA256불변, 이번에는 검사/원본 읽기/상태 문서만 수행했으며 게임 C++/Config/에셋/인덱스 미수정・빌드/패키징 미실행입니다. 이때의 제안(아래 후속 승인으로 적용)은 원본 IndicatorSystem11파일＋누락된 AsyncMixin plugin5텍스트파일＋검사1＋Build.cs1＋uproject1＋Config1, 총20파일입니다. 실제 UIAudit_A2의IndicatorLayer 템플릿을 근거로 하며 Renderer가FAsyncMixin을상속해비동기로드/취소/WidgetPool을사용하므로 플러그인을함께이식합니다. 이름/include/export만대응하고 동기로드나빈레이어로대체하지않습니다. Class5/Enum1 Redirect6개와AsyncMixin직접의존성/활성등록만추가하고 실제UI/Controller/Indicator 에셋은선택하지않습니다. Layer.Rebuild는유효LocalPlayer가필요하고Canvas에는Viewport/ProjectionData/Controller manager/Indicator BP가필요하므로 독립검사는registry/default・descriptor・ownerless manager・asset없는AsyncEvent순서/취소4건으로제한합니다. manager의이벤트/배열순서/제거후ManagerPtr유지와원본Canvas의SetDepth(X)도그대로보존하고 화면깊이정렬정상으로단정하지않습니다. 이후승인한20파일의검증과구분하며 IndicatorLayer 원본표시/Slate투영/실제비동기asset/WidgetPool/PIE/복제・ShooterCore・UIAudit미검증15/공유52개 및R5-3/R7/R13/10월8일체크포인트는미완료입니다.
- **Indicator・AsyncMixin20파일 작성 이력 / 아래 사용자 빌드・44건 통과:** 승인한 Indicator11파일을 `UI/IndicatorSystem/Lyra/`의 DCLyra 별도 타입으로 이식하고 `Plugins/AsyncMixin`의 원본5텍스트파일을 보존했습니다. `Tests/DCIndicatorFoundationAutomationTests.cpp`4건, Build.cs의Public AsyncMixin1개, .uproject의AsyncMixin enabled1개, Config의Class5/Enum1 Redirect6개를 반영했습니다. 원본16파일은 타입/파일명/include경로/export/명시include14개/공백 정규화 후 모두일치하며 async동기치환・빈레이어대체・ManagerPtr정리변경・SetDepth(X)수정은하지않았습니다. IndicatorFoundation의RegistrationAndInterface/DescriptorDefaultsAndGuard/ManagerLifecycle/AsyncSequenceAndCancel4건은별도unattended Editor＋-DCR5IndicatorIsolatedAutomation을요구합니다. transient descriptor/ownerless manager와asset없는AsyncEvent만사용하며취소/소멸뒤검사는자연스러운후속프레임의latent callback으로확인하고전역ticker를강제로실행하지않습니다. 정적원본비교16개/JSON/INI・CPP6쌍/추가분외Config・Build・uproject정규화해시동일/보호13개불변/diff・신규source whitespace 및기존Python9건을통과했습니다. **작성 당시 새UHT/C++/plugin 빌드와신규4건실행은미검증**이며Codex는빌드/패키징/에디터를실행하지않았습니다. 작업중별도생성된ignored Intermediate Definitions.AsyncMixin.h1개는생성주체를확정하지않고수정/삭제없이보존했습니다. 다음은사용자에디터종료후Development Editor/Win64 빌드(AsyncMixin포함)→신규4＋기존40 총44건검사입니다. 활성HUD/Controller/Pawn/에셋/입력설정은미변경이고실제Slate투영/거리정렬/asset비동기로드/WidgetPool/화면/복제・ShooterCore・UIAudit미검증15/공유52개 및R5-3/R7/R13/10월8일체크포인트는미완료입니다.
- **2026-10-08 21:07 KST Indicator・AsyncMixin 독립 검증 통과 이력:** 사용자 UBT **21:03:44 시작・67.21초 성공**, 게임 DLL **21:04:51 / 4,204,544 bytes**, AsyncMixin DLL **21:03:55 / 128,512 bytes**를 확인했습니다. `Saved/Automation/R5IndicatorFoundationChecks/index.json` **21:07:37 KST 신규4＋기존40 총44건 성공・실패/보고서 경고/오류/notRun0**, 동명로그 Error/ensure/Fatal0 및 종료0입니다. AsyncEvent순서/Cancel/소멸후자연프레임callback차단과Descriptor/Manager/native계약은통과했으나실제AsyncLoad/Slate투영/WidgetPool/표시는미검증입니다. 중간대조26개불변・최종대조인덱스외25개불변이며(.git/index해시변화관측・원인미확정・스테이징복구/변경안함),이번에는검사/엔진원본읽기/상태문서만수행했고C++/Config/에셋/인덱스미수정・빌드/패키징미실행입니다. 다음미적용승인안은Editor전용WidgetBlueprint조회보강5파일: DCRifleMigrationLibrary.h 새bounded report/API, 새DCUIBlueprintReadOnlyAudit.cpp, 새Tests/DCUIBlueprintReadOnlyAuditTests.cpp, tool Build.cs의UMG/UMGEditor, r5_ui_source_audit.py연결입니다. 원본UBaseWidgetBlueprint.WidgetTree/GetAllSourceWidgets는공개C++이고getter는가상Widget함수없이정확한outer의객체만열거함을확인했습니다. source10중WidgetBlueprint8개만실에셋조회허용하며root/소유class/템플릿/기존CDO의지정BindWidget2개/주석node핀수를읽고로드/Compile/저장/flags변경을하지않습니다. transient synthetic 검사3건만별도Editor opt-in에서메모리생성/Compile을허용하고파일미생성・dirty/flags/pointer보존을확인합니다. 기존라이플copy범위/asset22/native감사는완화하지않습니다. 실제runtime BindWidget/상속tree복제/시각동등성・ShooterCore/공유52개/활성UI・라이플/PIE/복제 및R5-3/R7/R13/10월8일체크포인트는미완료이며다음5파일은승인대기입니다.
- **작성 이력 / 2026-10-08 UI 읽기 전용 도구5파일 작성・사용자 빌드 대기:** 위 변경안에 대한 사용자 승인으로 `DCRifleMigrationLibrary.h`의 별도 UI report/API, 신규 `DCUIBlueprintReadOnlyAudit.cpp`, 신규 `Tests/DCUIBlueprintReadOnlyAuditTests.cpp`3건, Editor tool Build.cs의 private UMG/UMGEditor, `r5_ui_source_audit.py` 연결을 작성했습니다. 원본 고정 WidgetBlueprint8개와 synthetic 전용 GUID/RF_Transient/미저장 fixture만 허용하며 실제 에셋은 읽기만 합니다. 기존 native report/copy22·16·18 정책은 미변경입니다. 엔진 FindWidgetTreeOwningClass가 ConditionalPostLoad를 호출함을 추가 확인해 사용하지 않고, 로드된 class/tree/root 포인터를 순회합니다. 지정 CDO2필드는 hard object/타입/getter·미해결참조 제한을 두고 null・missing・unsupported를 구분하며 runtime BindWidget 성공으로 해석하지 않습니다. 테스트3건은 임시 template/상속 tree/주석 실제핀수/CDO값·누락·오류타입 및 dirty/flags/포인터/파일미생성 범위이며 실제실행은미검증입니다. Python 자체12건/AST/8개경계·금지호출·3테스트등록/whitespace・diff 정적검사를 통과했고 보호42개(Config/uproject/게임Build/기존도구CPP/Indicator・AsyncMixin/A2/44건보고서/Git인덱스)는SHA256불변입니다. Python 최초 sandbox 임시폴더권한 실패는 승인된 재실행으로 통과했으며 빌드/패키징/Unreal실행/에셋변경은하지않았습니다. 다음은 사용자 Development Editor/Win64 빌드→-DCR5UIContractTests 신규3＋기존44 총47건→새source UI run입니다. 기존44건성공과A2 partial/미검증15는보존하며 R5-3/R7/R13・실제UI/라이플/PIE/복제・10/8체크포인트는여전히미완료입니다.
- **후속 컴파일 오류 교정 이력 / 아래 재빌드 검증 통과:** 사용자 첨부 로그의 `DCUIBlueprintReadOnlyAuditTests.cpp:146,286` C2666을 확인했습니다. UE5.8의 `FDebug::GetNumEnsureFailures()` 반환형은 SIZE_T인데 TestEqual 기대값이 int 0이라 모호했습니다. 두 줄의 기대값만 `SIZE_T{0}`으로 교정했고 검사 의미/게임 코드/Config/에셋은 바꾸지 않았습니다. 엔진 선언과 SIZE_T 오버로드 대조 및 정확한2줄변경 정적검사만 수행하며 재빌드/새검사 성공은 아직 미검증입니다. 다음은 사용자 재빌드 확인 후 기존 작성 검사 정리와 실제 라이플 장착·발사·재장전 연결에 필요한 작업 묶음 제안입니다. UI 세부 준비를 무조건 추가하지 않고 필수 장애물과 후속 검증을 분리하며 새 범위는 설명·승인 후 작업합니다.
- **검증 이력 / 2026-10-08 22:41 KST — 47건·UI 제한조회 통과, 실제 라이플 통합 우선:** 사용자 재빌드22:35:17 시작/6.80초성공, 도구DLL22:35:23/618,496bytes입니다. `Saved/Automation/R5UIReadChecksFixed/index.json`22:38:40 신규3＋기존44 총47건 성공, 실패/보고서경고/오류/notRun0 및로그Error/ensure/Fatal0/종료0입니다. 기존script의원본재조회 `Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A3/summary.json`22:41:05 bounded_read_complete, 고정10개/WidgetBlueprint8개/89템플릿, 이전읽기미검증15해소・실패0・쓰기0・dirty/ensure/Error0/종료0입니다. source파일・동반2710개와별도보호11개(Git인덱스포함)가불변입니다. CDO2필드는관찰된null이지runtime binding판정이아닙니다. 이번에는검사/조회/문서만작업했고C++/Config/에셋/빌드/패키징은하지않았습니다. 새0.4원칙에따라추가도구확장을기본다음단계로삼지않습니다. 다음승인안은핵심18개실제이식＋남은원본GE/ADS구조체참조해결＋새 `/Game/DreamCatcher/GAS/Test/R5/Integration/`의플레이어/맵에Inventory→QuickBar→Equipment→발사·재장전연결묶음입니다. 기존공유114개/기존맵은자동덮어쓰지않고구Equipment자동장착과중복을방지하며,정확한범위·차이설명과승인후진행합니다. 현재R5-3/R6실사용과R7/R13전체・UI화면/PIE/복제・10/8실제장비체크포인트는미완료입니다.
- **작업 이력 / 2026-10-08 실제 라이플 이식・사용자 빌드 대기:** 승인한실제통합묶음으로엔진Migrate API를사용해라이플18개포함신규855개와원본태그테이블2개,총857개를이식했습니다. 기존114개SHA256불변/원본파일불변이며일반파일I/O로바이너리복사하지않았습니다. 첫새Migrate에서NS_ImpactDataChannel1개실패후그패키지만엔진기존Migrate(프로세스전용옵션)로원본동일하게보완했고,기존migration-after.json은첫부분실패이력입니다. 실제DreamCatcher로드22:58은누락타입/Surface/Plugin/Tag때문에실패・에셋저장0입니다. 이를해결하려고ContextEffects원본10파일,별도DCLyraGameInstance원본2파일,테스트초기장착DCRifleIntegrationActors2파일과정확한Redirect/Surface/원본Tag18행・테이블2개/AnimationWarping·AnimationLocomotionLibrary를연결했습니다. 원본12파일본문정규화일치/AST/JSON/중복Redirect0검사는통과했지만새UHT/C++빌드와수정후실로드는미검증입니다. 활성GameInstanceClass는기존DCGameInstance이며기존PlayerController/GameMode본문·기존맵미변경,새Integration맵/플레이어BP생성전입니다. 빌드/패키징은사용자가담당합니다. 다음은사용자빌드→실제core로드·Compile→동일승인범위의별도Integration플레이어/맵생성→장착·발사·재장전확인입니다. 상세파일/근거/제약은명세의실제라이플이식섹션을따르고추가범용도구확장을기본다음단계로삼지않습니다. R5-3/R6실사용・PIE/복제는여전히미완료입니다.
- **검증 이력 / 2026-10-08 23:57 KST — Integration 맵·실제 PIE 장착/해제 통과:** 사용자재빌드23:28:13/9.70초성공, 게임DLL23:28:22/4,370,944bytes를확인했습니다. core18＋보조2 Compile오류0뒤남은탄약태그경고를Config/Tags의직접LoadConfig형식(GameplayTagList=)으로교정했습니다. 새 `/Game/DreamCatcher/GAS/Test/R5/Integration/`에플레이어/PawnData/Input/Controller/GameMode/R매핑/맵7개생성・별도재로드성공,기존플레이어파일과공유114개불변입니다. `Saved/Automation/R5RifleIntegrationPIE1/index.json`23:57:29 엔진기본Project.Maps.PIE 성공1/경고·오류·실패0, 로그23:57:21.686 실제Rifle장착(1개/실제ItemInstigator검사후),23:57:29.679해제/종료0을확인했습니다. 이번에는게임C++/빌드/패키징을실행하지않았습니다. 다음은새맵PIE의좌클릭/R/탄창소진/표적피해실사용검증입니다. 원본FireCue와기존테스트Cue동일태그충돌로원본Cue등록전환은미적용(기존파일삭제없음)이며범위설명·승인후이어갑니다. 별도-game시도는맵진입전GameData로드fatal/종료3으로실패했고근본원인은미확정입니다. PIE성공을Standalone/패키징성공으로취급하지않습니다. R5-3실이식/장착에서R6실사용검증으로진척했지만사격·재장전·시각/조작/복제전체완료는아닙니다.
- **최신 재개 지점 / 2026-10-09 R6 피해·사망/마우스 진단:** 사용자보고와실제PIE로그에서자동라이플ApplyGameplayEffectToTarget의no GameplayEffect오류를확인했습니다. 대상GA_Weapon_Fire_Rifle_Auto의GE_Damage=None인반면원본/준비본은원본Rifle GE를가리키고대상GE파일도있습니다. 배치표적은Weapon채널2Ignore/Visibility만Block/기본NoTeam/사망Ability없는HP출력용이며,이표적에피해·사망전체확인을안내한것은부정확했습니다. 마우스는원본과같은Modifier/양수C++전달에프로젝트Legacy배율True와Controller Pitch=-2.5가겹친상태입니다. 원본은Controller Config의Yaw/Pitch/Roll=1.0이며해당대응이빠졌습니다. 이번은읽기전용진단과문서인계뿐으로게임코드/Config/에셋수정・빌드는하지않았습니다. 다음승인안은GE참조복구＋원본무기Trace채널/별도Integration표적충돌·팀/원본Health·Death흐름연결＋IntegrationController에만원본입력배율1.0복구입니다. 전체피해규칙우회/공용표적변경/전역입력뒤집기/기존에셋삭제는하지않습니다. 실사용수정후검증은미완료이며Cue충돌/Standalone실패도남아있습니다. 상세근거와제약은명세의10/9진단섹션을따릅니다.
- **이전 빌드 오류 교정 이력 / 위 10월 8일 23:28 사용자 재빌드로 검증:** 사용자 로그의 own-header 순서오류, Controller 지역 Pawn의 C4458, UDCEquipmentDefinition 불완전타입, DTLS 프로젝트 의존성 누락을 교정했습니다. 당시 재빌드 대기 기록은 위 성공 결과로 대체합니다.
- **최신 재개 지점 / 2026-10-09 02:56 KST R6 수정 적용·사용자 빌드 대기:** 사용자 승인으로 `DCRifleIntegrationTarget.h/.cpp`의 별도 표적에 원본 HealthComponent/DeathAbility 연결·Weapon 채널 Block·팀1·Death 종료 후 지연 제거를 작성하고, Integration GameMode의 플레이어 팀0을 추가했습니다. 원본 피해 계산/팀 규칙·공용 구형 표적은 보존합니다. Config에는 원본 Weapon 채널2/3/4의 기본 Ignore 등록과 통합 Controller만 Yaw/Pitch/Roll=1.0을 반영했습니다. `r6_rifle_damage_fix.py`로 자동라이플 GE_Damage를 원본 Rifle GE로 복구했고, `R6RifleFix-Repair3.log` 02:56:00 저장1개/종료0 및 `R6RifleFix-Reload1.log` 02:56:54 별도 재로드/입력배율/원본 Death 부모·8초·AutoStart 확인/저장0/종료0을 통과했습니다. 오류0이지만 AutoReload의 `/Script/LyraGame` import 경고1종은 남았습니다. 초기 저장2회는 사용자 에디터 파일 잠금으로 실패했고 사용자가 종료한 뒤 성공했으며, 강제 종료나 바이너리 직접 이동/삭제는 없었습니다. 보호 에셋/동반파일20개와 별도 구형 표적 C++/BP·DamageExecution·Rifle GE·DefaultInput·Git 인덱스7개 상태 유지, Python AST/diff 검사 통과입니다. **C++ 빌드/패키징·새 표적 BP 생성/맵 교체·실제 피해/사망 재검증은 미실행**입니다. 다음은 사용자 Development Editor/Win64 빌드 후 동일 승인 범위에서 새 `R5/Integration/BP_DC_RifleDeathTarget` 생성 및 `L_DC_RifleIntegration`의 구형 배치 표적1개만 교체→별도 verify-target→PIE입니다. 원본 Death 지연8초를 유지하며 단순 큐브 표적 검사는 캐릭터 래그돌/시각·복제 완료가 아닙니다. 기존 Cue 충돌·Standalone GameData 실패도 별도 미완료입니다. 위 진단 당시의 미적용 기록보다 이 항목을 우선합니다.

---

## 0.5 검증 이력 — 2026-10-09 R6 피해·사망/마우스 실사용 통과

위 02:56의 빌드 대기는 아래 사용자 빌드 및 에셋 연결 결과로 대체합니다.

- 사용자 UBT 로그 **03:03:30 시작 / 34.98초 / Succeeded**, 게임 DLL **03:04:04 / 4,386,304 bytes**로 신규 표적 C++/UHT 빌드를 확인했습니다. Codex는 빌드·패키징을 실행하지 않았습니다.
- 같은 승인 범위에서 새 `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleDeathTarget`을 생성/Compile/저장하고 `L_DC_RifleIntegration`의 구형 배치 표적 **1개만 교체**했습니다. 공용 구형 표적 BP는 그대로이며 파일 삭제는 없습니다. `R6RifleFix-TargetPrepare1.log` **03:10:51** 성공/종료0입니다.
- 초기 재로드3회는 검증 스크립트의 Python enum 이름 오류로 실패했고 에셋은 저장하지 않았습니다. 엔진 선언/변환 규칙을 확인해 `CollisionResponseType.ECR_BLOCK`으로 교정한 `R6RifleFix-TargetReload4.log` **03:14:24**는 부모·원본 Death Ability·팀1·신규 표적1/구형0·Weapon Block·입력배율1.0 확인/저장0/종료0입니다. 게임 C++/Config는 이번 턴에 수정하지 않았습니다.
- 준비/최종 재로드 각각 오류0/경고15종(AutoReload LyraGame import1, Manny PoseAsset 애니메이션 GUID14)이므로 경고 없는 전체 이식 검증은 아닙니다. 보호 에셋/동반파일20개 및 별도 코드·Config·Git 인덱스 등9개 해시는 그대로입니다.
- **실제 PIE** `Saved/Automation/R6RifleTargetPIE1/index.json` **03:16:16 성공1/실패·보고서 경고·오류0**, 프로세스 종료0입니다. `R6RifleTarget-PIE1.log`에서 **03:16:08 플레이어 팀0 / 표적 Health100·팀1·GA_Hero_Death_C 준비 / 라이플 장착**, **03:16:16 해제**를 확인했습니다. Error/Fatal/ensure0, PIE 전후 새 BP/맵 해시 불변입니다. 입력 사격·피해·사망을 실행한 검사는 아닙니다.
- **다음은 추가 빌드 없이 사용자 플레이 확인**입니다. 같은 통합 맵에서 마우스 상하/감도, 앞쪽 표적 사격 후 Output Log `GASTestTarget`의 체력 감소, HP0의 `[R6] Target death started`와 8초 뒤 `Target death finished`/표적 제거를 확인합니다. 새 BP는 화면 PrintString을 복사하지 않았습니다. `prepare-target` 재실행/기존 작업 재빌드는 필요 없습니다. 실제 피해·사망/시각·복제, 원본 Cue 충돌 및 Standalone GameData 실패는 여전히 미완료로 구분합니다.

---

**후속 사용자 실사용 확인 / 위 플레이 확인 대기보다 우선:** 사용자는 마우스 방향·감도, 표적 사격 시 체력 감소, HP0 사망 시작 및 약8초 뒤 종료·제거가 모두 정상이라고 확인했습니다. 최신 `Saved/Logs/DreamCatcher.log`에서도 **03:19:59.391~03:20:02.619 KST Health100→88→…→4→0**, **03:20:02.619 DeathStarted → 03:20:10.619 DeathFinished**를 확인했습니다. 마우스 조작감과 표적 제거는 사용자 확인 근거이며 로그만으로 판정하지 않습니다. 이 수정 묶음은 실사용 검증 통과입니다. 수동/자동 재장전·원본 발사/탄착 연출·복제 및 R6 전체 완료로 확대하지 않습니다.

**다음 제안 / 아직 미적용:** R6-3의 원본 `GCN_Weapon_Rifle_Fire`·`GCN_Weapon_Impact` 등록과 R6-4의 시각·음향·CameraShake/중복 여부 확인을 묶습니다. 최신 사격 로그는 여전히 `GCN_DC_Weapon_Rifle_Fire`를 호출합니다. 두 Fire Cue가 같은 태그이므로 `DefaultGame.ini`에서 기존 테스트 Cue 스캔 경로를 제외하고 이식된 `RifleCore_Prepared_0247` 폴더를 등록하는 범위를 먼저 승인받습니다. Death 경로/태그/원본 Cue 그래프는 유지하고 구형 파일은 삭제하지 않습니다. 원본 `LyraGameFeaturePolicy`는 플러그인 등록/해제 시 Cue 경로를 추가/제거하지만 현재는 ProjectContent이므로 이번 제안은 Config 상시 등록이라는 수명 차이가 있습니다. 전체 GameFeature 연결을 앞당기는 대안은 별도 범위이며 불가능 판정이 아닙니다. Config는 프로젝트 전역이므로 동일 태그를 쓰는 구형 테스트 맵의 연출도 바뀔 수 있습니다. 먼저 별도 프로세스의 임시 경로 설정으로 검사하고, 통과 후 승인된 Config를 전환하는 순서입니다. C++ 추가/새 범용 도구 확장은 기본 계획이 아니며 새로운 필수 변경은 다시 설명합니다. 이번 확인 턴은 읽기 전용 조사·문서 갱신만 했고 소스/Config/에셋/빌드/패키징은 변경·실행하지 않았습니다.

---

## 0.6 검증 이력 — 2026-10-09 03:34 KST 원본 Fire/Impact Cue 등록 전환 완료

- 사용자 승인 후 `Config/DefaultGame.ini`의 Cue 경로를 기존 `/Game/DreamCatcher/GAS/GameplayCues`에서 이식된 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247`로 전환했습니다. Death 경로는 유지했습니다. 구형 Cue 파일/태그와 원본 Fire/Impact/연출 에셋을 수정·삭제하지 않았습니다. 원본 GameFeature 등록 수명 대신 Config 상시 등록이며 같은 태그를 쓰는 구형 맵도 영향을 받는다는 승인된 차이는 유지합니다.
- 읽기 전용 한정 스크립트 `Scripts/Editor/r6_rifle_cue_verify.py`로 검사했습니다. 초기 preflight1~4는 Python 프로퍼티/타입 접근 오류로 실패했고 프로젝트 Config/에셋은 쓰지 않았습니다. 엔진 native class와 공개 Runtime CueSet 데이터 조회로 교정했으며 새 C++/범용 도구를 추가하지 않았습니다.
- `R6RifleCue-Preflight5.log` **03:31:58 KST** 임시 프로세스 Config 검사 성공 후 실제 Config를 반영했습니다. 임시 옵션 없는 `R6RifleCue-Configured1.log` **03:32:52**에서도 **Death1/원본Fire1/원본Impact1, 구형Fire0**의 실제 등록 클래스가 일치했습니다. Cue2개 Compile, 직접 package14개 로드, 저장0/보호파일77상태 불변/종료0입니다. 요약 오류0/경고10종 중9종은 엔진 PrintCues 진단 출력이며, `B_WeaponDecals`의 `/Script/LyraGame` import 경고1종은 미해결입니다. 기존 AutoReload/PoseAsset 경고도 전체 해결로 취급하지 않습니다.
- `Saved/Automation/R6RifleCuePIE1/index.json` **03:34:02 실제 Project.Maps.PIE 성공1/실패·보고서 경고·오류0**, 프로세스 종료0입니다. `R6RifleCue-PIE1.log`에서 03:33:54 플레이어 팀0/표적Health100·팀1·Death Ability/라이플 장착,03:34:02 해제를 확인했습니다. Error/Fatal/ensure0입니다. 사격을 입력하거나 FX/음향을 재생한 검사는 아닙니다.
- 이번 변경은 위 Config 경로/검증 스크립트/문서뿐입니다. 게임 C++·에셋·맵·Git 인덱스는 그대로이며 빌드·패키징은 실행하지 않았습니다. **재빌드 없이 에디터를 새로 열고** 통합 맵에서 총구/탄착 효과·사격음·CameraShake/중복발동, R 수동 및 탄창소진 자동재장전을 확인합니다. 앞서 통과한 피해·사망/마우스는 완료 이력을 유지하되 Cue 전환 후 회귀는 구분합니다. 실제 원본 연출·재장전·복제·Standalone 및 R6 전체 완료는 아직 아닙니다.

---

## 0.7 진단 이력 — 아래 재장전 해석/변경안은 후속 사용자 정정으로 철회

**최신 사용자 정정 우선:** 1·2번은 단순히 좌클릭 유지 중 R이 차단되는 문제가 아닙니다. **좌클릭을 한 탄창 소진→자동 재장전 완료 후까지 유지했다가 놓으면, 이후 사격/자동 재장전은 되지만 R 수동재장전만 안 되는 지속 상태 버그**입니다. 원본과 다르게 만들 필요가 없다고 명시했으므로 아래 ‘재장전 우선’ 해석/태그 반전/모션 끝까지 Ability 연장 제안은 폐기합니다. 원본 태그 규칙이 이 지속 상태 버그의 원인이라고 확정하지 않습니다. TagRelationships/PawnData/Reload Ability는 변경하지 않습니다. 현재 승인 범위는 **3번 오디오 사본 ShotInterval0.12 및 Cue Sound 연결만**입니다. 1·2번의 원인은 미확정이며 재현 전후 R 입력 전달·Ability 활성 상태·잔여 차단 태그/탄약 상태를 확인해야 합니다.

- 사용자는 (1) 재장전 모션을 사격으로 취소하면 안 됨, (2) **자동 재장전은 되지만 좌클릭을 계속 유지한 상태에서 R 수동재장전이 먹지 않음**, (3) 연사음이 밀려 재장전 중에도 총성이 남음을 보고했습니다. 2번을 자동 재장전 불능으로 기록하지 않습니다. 신규 요구사항은 **사격보다 재장전 우선, 모션 완료 전 사격 금지**이며 죽음/장비해제 등 필요한 취소 전체를 금지하는 뜻은 아닙니다.
- 원본/이식본 `TagRelationships_ShooterHero` 배열이 일치합니다. WeaponFire는 Reload를 Block/Cancel, Reload는 Emote만 Block합니다. 따라서 1·2번은 이식 중 규칙 유실보다 원본의 발사 우선 규칙과 새 요구의 차이입니다. 현재 Integration PawnData도 이 매핑을 참조합니다. 또한 원본/이식 `GA_Weapon_ReloadMagazine`은 `ReloadDone` 이벤트에서 권한 측 탄약 보충→EndAbility, `bStopWhenAbilityEnds=false`라 모션 종료와 Ability 종료가 분리될 수 있습니다. 태그 반전만으로 모션 전체 사격 금지를 보장하지 않습니다.
- 오디오 읽기 근거: 원본/이식 자동라이플 `FireDelayTimeSecs≈0.12`, `AutoRate=1`; 원본/이식 `MSS_Weapons_Rifle2_Fire`의 `ShotInterval=0.15`, 내부 `TriggerCounter/TriggerQueue/AllowShot`·간격 게이트를 확인했습니다. 원본/이식 `B_Weapon.TriggerFireAudio`는 AudioComponent 생성/재사용 후 `Fire` Trigger를 보내며 간격을 덮어쓰지 않습니다. MetaSound 텍스트 export는 Root ClassName GUID를 정규화하면 일치합니다. 밀리는 음향 요청 큐를 설명하는 구체적 정적 근거지만 실제 음향 출력 시각/지연량은 아직 미측정입니다. 증상을 해결했다고 보고하지 않습니다.
- 근거는 `Saved/Logs/R6ReloadAudio-target2.log`, `R6ReloadAudio-original3.log`와 `Saved/Diagnostics/R6ReloadAudio/*-audio.t3d`입니다. 읽기 전용 그래프/설정/텍스트 export이며 Compile·에셋 저장·코드/Config 수정·빌드/패키징은 없습니다. 원본 B_Weapon의 잘못된 /ShooterCore 조회는 원본2 로그의 미검증 이력이며 정확한 /Game/Weapons/B_Weapon을 조회한 원본3 결과를 우선합니다. 최종 보호 파일상태 target52/source48 불변입니다.
- 다음 제안: Integration 전용 매핑 사본에서 Fire→Reload Block/Cancel 제거, Reload→Fire Block/Cancel 추가 및 PawnData 연결. 이식 ReloadMagazine의 탄약 보충은 원본 Notify 시점을 유지하되 정상 Ability 종료는 모션 완료까지 유지합니다. 원본 Audio는 보존하고 전용 사본의 ShotInterval을 실제 발사간격0.12와 맞추는 최소 변경을 우선 비교하며, 연결 Cue의 Sound 참조만 이 사본으로 전환하는 범위를 승인받습니다. 발사를0.15로 늦추는 대안은 전투속도를 바꾸므로 추천하지 않습니다. 음향이 남으면 잔향과 추가 발사음/큐 지연을 구분해 재생시각을 확인하고, 큐 제거·스케줄러 변경 같은 확대 수정은 먼저 설명합니다. 아직 수정 미적용이며 다음은 이 작업 묶음의 사용자 승인입니다.

---

## 0.8 오디오 수정 이력 — 별도 재로드 및 후속 사용자 청취 확인 통과

**후속 사용자 확인 우선:** 사운드가 정상 작동한다고 사용자가 확인했습니다. 아래 작성 당시의 실제 청취 대기는 해소됐으며, 다음 대상은 원본 규칙을 유지한 R 수동재장전 지속 상태 버그의 재현/진단입니다. 모든 장치/복제의 음향 검증 완료를 뜻하지 않습니다.

- 최신 사용자 정정에 따라 1·2번은 **한 탄창 소진과 자동재장전 완료 후까지 좌클릭 유지→해제한 뒤, 이후 사격/자동재장전은 되지만 R 수동재장전만 실패하는 지속 상태 버그**로 다룹니다. 기존 ‘유지사격 중 R 차단’ 해석과 재장전 우선 정책/태그 반전/Ability 종료 연장 제안은 철회했습니다. 원본 규칙을 유지하며 이 버그의 원인은 아직 미확정입니다.
- 직접 승인된 **3번 오디오만** 적용했습니다. 새 `/Game/DreamCatcher/GAS/Test/R5/Integration/Audio/MSS_DC_Rifle_Fire`를 Unreal 내부 복제로 만들고 공식 MetaSound Builder로 ShotInterval0.15→0.12를 설정했습니다. 이식 `GCN_Weapon_Rifle_Fire`의 `OnBurst.K2Node_CallFunction_6.Sound` 핀1개만 새 사본으로 연결했습니다. 원본 오디오/음원/큐 처리 그래프·발사속도·재장전·TagRelationships·PawnData는 그대로입니다.
- 한정 스크립트 `Scripts/Editor/r6_rifle_audio_interval.py`. 사용자 에디터 종료 확인 후 `R6RifleAudio-Prepare1.log` **04:16:25 KST** 저장2개/종료0, `R6RifleAudio-Reload1.log` **04:18:24** 별도 재로드/0.12 및 Sound 참조 확인/저장0/종료0입니다. 원본/사본 텍스트를 에셋 이름·경로·Root ClassName GUID 및 해당 ShotInterval 기본값1개만 정규화해 **전체 동일**을 확인했습니다.
- 보호파일31상태 불변, 별도 기존8개 해시에서 허용된 Fire Cue만 변경됐습니다. 원본 오디오·재장전 Ability/태그 매핑/PawnData/Fire Ability·Config·Git 인덱스는 보존했습니다. Prepare 오류0/경고2(AN_PlayWeaponMontage LyraGame import, 무음 commandlet의 MetaSound PreSave 준비 생략), Reload 오류0/해당 import 경고1입니다. 음향 렌더/청취 및 지연 해소는 아직 미검증입니다.
- 이번에는 C++/Config 수정·빌드·패키징을 하지 않았습니다. **추가 빌드 없이** 에디터를 새로 열어 연사→탄창소진→자동재장전 때 뒤늦은 총성이 이어지는지 확인합니다. R키 지속 상태 버그는 이번 수정 대상이 아니며 원본 규칙을 바꿔 해결했다고 보고하지 않습니다. 후속은 정확한 재현 전후 R 입력 전달/Ability 활성·차단 상태/탄약을 조사하는 것입니다.

---

## 0.9 진단 이력 — 사운드 사용자 검증 통과 / R 물리키 경로 추가 확인

- 사용자는 사운드 정상 작동을 확인했습니다. 오디오0.12 간격 수정은 실사용 청취 확인까지 통과입니다. R 재장전 버그와 별개이며 원본 규칙 변경은 계속 금지합니다.
- 새 코드/에셋 변경 없이 기존 Enhanced Input의 Vector Action 주입과 엔진 `Project.Maps.PIE`를 이용해 별도 PIE에서 재현을 시도했습니다. 한정 임시 진단은 `Saved/Diagnostics/R6ReloadState/probe.py`입니다. 초기 PIE1~4/API1은 Python 별칭/함수 노출/비반영 InputActionValue 전달 문제로 입력 재현 전에 실패했으며 재현 성공으로 보고하지 않습니다.
- `Saved/Logs/R6ReloadState-PIE5.log` **15:05 KST**에서는 실제 입력 주입·탄약 관찰까지 실행됐습니다. 시작 수동재장전25→30, 유지사격30→0, 자동재장전0→30, 이후1초 더 사격해22발에서 입력해제,0.5초 후 재장전 Action 입력 시 **22→30으로 정상 보충**했습니다. 이 시점 Fire/Reload/NoFiring/InputBlocked 태그가 해제되는 것도 확인했습니다. 따라서 **이 Action 수준 조건에서는 미재현**이지 사용자 버그가 없거나 해결됐다는 뜻이 아닙니다. 물리 R키/키 매핑 경로는 우회했습니다.
- 동일 실행의 전체 Automation 보고서는 **실패1**입니다. NullRHI에서 Niagara/Decal 인스턴스가 생성되지 않아 발사 FX 그래프의 Accessed None 오류가 발생했습니다. 로직 관찰 사실과 전체 테스트 성공을 분리하며, 실제 화면의 FX 오류로 확정하거나 이를 없애려고 원본 에셋을 수정하지 않습니다.
- `R6ReloadState-Keys2.log` **15:10 KST**의 정적 조회: Hero 기본IMC_Player priority0, R4 Aim priority10, RifleReload priority1. 확인한 세 기본 매핑에서 R은 IA_Weapon_Reload 한 곳이며 추가 Trigger/Modifier 없음, Controller 키 이벤트 충돌도 발견하지 못했습니다. 런타임 사용자 리매핑/컨텍스트/포커스까지 입증한 결과는 아닙니다.
- 원인 미확정입니다. 다음은 사용자 실제 PIE에서 기존 엔진 로그만 활성화(`Log LogAbilitySystem Verbose`, `Log LogEnhancedInput Verbose`)한 뒤 정확한 조건을 재현하고 R 실패 직후 로그를 읽습니다. 이 방식은 C++ 빌드·에셋 수정 없이 입력 소비/재매핑 및 CanActivate 실패 사유를 확인하기 위한 것입니다. 태그 반전/Ability 강제 종료/입력 강제 리셋은 적용하지 않습니다.
- 이번 턴은 임시 진단·문서 갱신만 했습니다. 게임 C++/Config/에셋/빌드/패키징 수정·실행 없음. 보호7개 해시(Git 인덱스·Config·태그·Reload/Fire·PawnData·맵)는 모두 동일했습니다.

---

## 0.10 제안 이력 — R 현재 미재현·진단 보류 / R7 제안은 아래 완료 점검으로 보류

- 사용자는 현재 R 재장전 버그가 재현되지 않으며 다음 단계로 진행하라고 요청했습니다. **현재 사용자 미재현·원인 미확정·후속 관찰**로 기록하고 해결 원인/수정이 확인됐다고 단정하지 않습니다. 추가 R 재현 요청은 일단 멈추며, 재발하면 기존 조건/로그를 이어 봅니다. 원본 재장전 규칙은 변경하지 않았습니다. 사운드는 별도 사용자 통과 이력을 유지합니다.
- 다음 제안은 **R7-1/2 원본 ReticleHost 기반 무기 UI의 통합 맵 독립 연결**입니다. 원본 조회 `UIAudit_20261008_A3/source.json`에서 W_WeaponReticleHost는 LyraWeaponUserInterface 부모, VisWrapper(SizeBox)/WidgetStack(Overlay), OnWeaponChanged→ReticleConfig.ReticleWidgets 생성/InitializeFromWeapon/ClearExistingWidgets 구조입니다. 이식한 ID_Rifle은 W_Reticle_Rifle과 W_AmmoCounter_Rifle을 이미 가리킵니다. 대상 native 부모/Redirect도 준비됐습니다.
- 원본 최종 경로는 UIPolicy/OverallLayout→ShooterHUD/UIExtension 슬롯→Host이고 StandardHUD ActionSet은 ShooterCore 활성화를 요구합니다. 현재 DefaultUIPolicy 설정/전체 Experience HUD 경로는 미연결입니다. 따라서 이번 제안은 **테스트 Controller가 원본 Host를 직접 표시/제거하는 독립 검증 연결**이며 원본의 GameFeature/슬롯 수명까지 완료한 것으로 보고하지 않습니다. 전체 정책/메뉴/ActionSet을 지금 앞당기는 대안은 공용 입력·레이아웃 영향과 추가 의존성을 포함하므로 별도 범위입니다. 불가능 판정이나 원본 기능 삭제가 아닙니다.
- 승인받을 범위: 원본 Host를 새 `/Game/LyraMigration/UI/R7/` 사본으로 이식/타입 연결, 기존 Rifle 위젯/Item 참조 재사용, `DCRifleIntegrationActors.h/.cpp`에 로컬 플레이어 생성/해제 연결과 클래스 지정만 추가, 통합 Controller BP에만 설정합니다. 공용 HUD/Config/다른 맵은 변경하지 않습니다. 기존 조준점과 중복될 경우 통합 테스트 HUD 사본에서 해당 조준점 표시만 억제하고 체력/Scope 등 다른 요소는 유지하며 원본 HUD를 삭제하지 않습니다. 새 일반 UI 감사기/범용 도구를 확대하지 않습니다.
- 검증은 실제 Inventory/Equipment Instigator의 탄약/Reticle 데이터, 발사·재장전·조준/FOV·사망/장비해제/Pawn 교체 시 중복 및 잔여 위젯입니다. C++ 빌드는 사용자 담당입니다. 현재는 읽기 전용 코드/기존 조사 확인과 문서 갱신만 했고 이 **새 코드·에셋 묶음은 승인 전/미적용**입니다. R6 퍼짐/반동/표면·복제 전체와 R7 전체 HUD는 여전히 별도 미완료입니다.

---

## 0.11 감사 이력 — 2026-10-09 R0~R6 완료 기준, R7 착수 보류

- 사용자는 R7 전에 R0~R6의 실제 완료 여부를 점검하라고 요청했습니다. **전체 완료 판정은 불가**입니다. 기준은 명세의 각 R 검증 항목이며 과거 자체0~6단계/원본 파일 이식/독립 테스트/통합 PIE/사용자 확인/전체 전환을 구분합니다. R7 제안은 착수하지 않습니다.
- **실행 근거:** 현재 GAS-System/UE5.8.1, 사용자 빌드03:03:30 성공/DLL03:04:04(4,386,304bytes). 기존 장비1·팀1·타입등록4·무기계산/비용2를 재실행해 `Saved/Automation/R0R6CompletionAudit_20261009/index.json` **15:35:46 KST 성공8/실패·경고0**, 로그 Error/Fatal/ensure0을 확인했습니다. 합성 곡선/임시 월드 검사이지 모든 단계 런타임 검증은 아닙니다. 실제 에셋 읽기에서는 Hero legacy입력/카메라=false, 원본ADS routing=true, Integration PawnData의 legacy무기=None, 실제 Rifle AbilitySet의 FireAuto/Reload/OnSpawn AutoReload 연결을 확인했습니다.
- **확인된 차단 문제(R1/R2 기동):** `R0R6CompletionAudit-game.log` **15:40:28 KST** `Ignoring PrimaryAssetType DCGameData - Conflicts with LyraGameData` 후 `/Game/DefaultGameData` 로드 Fatal/종료3을 재현했습니다. Editor/PIE와 `-game`의 GameData 로드 분기가 다릅니다. 에디터 조회에서는 디스크전용/일반 AssetRegistry 모두 DCGameData여서 ‘에셋 메타데이터만 잘못됐다’고 원인을 확정하지 않습니다. 실제 비에디터 스캔/등록 충돌을 해소해야 하며 패키징 성공 여부는 이번에 검사하지 않았습니다.
- **R1-3 실제 설정 누락:** `R0R6CompletionAudit-registry.log`15:47 KST의 공개 GameplayAbilitiesDeveloperSettings에서 Cooldown/Cost/Networking/TagsBlocked/TagsMissing 실패 태그5개가 모두 빈 값입니다. 원본 DefaultGame.ini에는 대응 Ability.ActivateFail.* 설정이 있습니다. Globals의 protected 필드 직접 조회는 불가였으므로 설정값/코드/최근 빈 실패사유 로그를 근거로 삼습니다. 실패 메시지·진단 계약 보완 대상이지 이전 R키 버그의 원인 확정은 아닙니다.
- **R6-1 미완료:** 발사는 Base Weapon의 TimeLastFired를 갱신하지만 RangedWeapon의 회복은 별도의 LastFireTime(초기0/대입 없음)을 읽습니다. 실제 라이플 SpreadRecoveryCooldownDelay는0.15초입니다. 원본에도 동일한 불일치가 있고 기존 합성 테스트는 기본지연0만 검사합니다. 명세가 요구한 실제 발사 후 회복지연 재현을 아직 완료하지 않았으며 수정은 별도 근거/승인이 필요합니다.
- **핵심 통과/남은 범위:** R0 기준 문서 완료. R1 기반 클래스는 있으나 위 기동/설정 보완 필요. R2 핵심 GAS/OnSpawn/입력·태그 연결 이력은 유지하되 라이플 포함 Pawn 교체·입력 취소·정리의 최신 통합 회귀가 충분히 닫히지 않았습니다. R3 피해/회복/사망 이력과 실제 표적100→0/8초 종료는 유지하되 면역/리셋 및 기존 적 경계의 최신 회귀를 구분합니다. R4 PC 조준/종료 범위는 통과, 벽 가림/FOV 정밀 비교와 보류 장치는 별개입니다. R5 기본 Inventory→QuickBar→Equipment/Instigator 및 fixture 재장착 보존 통과, 실제 라이플 재장착/사망·Pawn 교체 결합 회귀는 남습니다. R6 기본 발사·피해·Cue 등록·사운드는 통과했지만 위 회복지연 및 반동 중복·거리/표면/엄폐 비교는 미완료입니다.
- 현재 원본 라이플 활성 경로는 Integration 맵입니다. 전역 기본 DA_DC_PlayerPawn에는 구형 WID_DC_Rifle이 남아 있고 구형 적 ADCEnemyCharacter는 비GAS HealthComponent입니다. 이들을 이번에 삭제/전환하지 않았습니다. UI 실제 표시는 R7, 기존 적 GAS는 R10, 실게임 맵/전체 경로 전환은 후속 단계이며 선행 단계를 끝내려고 임의로 앞당기거나 누락시키지 않습니다.
- **다음 권장 순서:** R1 GameData 비에디터 기동/실패태그 설정 정리 → R6 실제 회복지연 검증/필요 변경안 → R2/3/5 라이플 수명 결합 회귀 및 R4/6 카메라·반동/표면 비교 → R7. 원인미확정·현재미재현인 R키 문제는 사용자 요청대로 관찰 이력만 유지합니다. 이 감사 턴에는 코드·Config·에셋 수정·빌드/패키징 없이 기존 테스트/읽기/문서 갱신만 했습니다. 상세 근거와 단계별 판정은 명세의 R0~R6 감사 섹션을 따릅니다.

---

## 0.12 검증 이력 — 2026-10-09 16:15 KST R1 기동/실패태그 보완 통과

- 감사 후 사용자 승인으로 **R1의 GameData 기동과 실패사유 태그5개만** 보완했습니다. R6 회복지연/재장전 규칙/다른 게임 로직은 변경하지 않았고 C++ 빌드·패키징은 실행하지 않았습니다.
- `Config/DefaultGame.ini`의 기존 AbilitySystemGlobals 섹션에 원본과 같은 Cooldown/Cost/Networking/TagsBlocked/TagsMissing 매핑5개를 추가했습니다. UE5.8 GameplayAbilitiesDeveloperSettings는 `OverrideConfigSection`에서 실제로 이 기존 섹션을 사용합니다. 최초 다른 섹션 시도는 검사 실패/에셋저장0으로 중단했고 현재 파일에는 잘못된 별도 섹션이 없습니다.
- Unreal 내부에서 `/Game/DefaultGameData` **1개만 재저장**했습니다. 피해/회복/동적태그 GE3개 참조를 그대로 유지하며 클래스명 이식 후의 등록/직렬화 메타데이터를 현재 DCGameData에 맞췄습니다. 원본 AssetManager의 Editor/비에디터 로드 분기와 엔진의 타입 충돌 검사를 우회하지 않았습니다. 무조건 동기 로드 fallback/전체 캐시 삭제/전체 재저장도 하지 않았습니다.
- `R1Startup-Resave2.log` **16:09:09 KST** 저장1개·종료0, `R1Startup-Reload1.log` **16:11:31** 새 프로세스에서 GE3참조/태그5개/DCGameData 타입 확인·저장0·종료0입니다. 재로드 경고1개는 사용한 AssetRegistry 조회 API의 deprecation 경고입니다. `R1Startup-GameAfterResave2.log` **16:11:32 GameData 로드 성공 → 16:11:39 Integration 맵 로드 완료 → 종료0**, 기존 PrimaryAssetType 충돌/Fatal/Error/ensure0입니다. 비에디터 시작/맵 로드 검증이지 전체 Standalone 플레이/패키징 성공은 아닙니다.
- `Saved/Automation/R1StartupRegression_20261009/index.json` **16:15:23** 기존 장비1/팀1/타입4/무기2 총8건 성공·실패/경고/notRun0. 로그 Error/Fatal/ensure0입니다. 보호9파일 중 승인된 Config와 GameData만 바뀌었고, 나머지7개(AssetManager C++/GameData C++/DefaultEngine/R6 코드/관계표/Git인덱스)는 동일했습니다. Config는 태그5행+설명주석을 제거한 가상 비교에서 작업 전 해시와 동일했습니다.
- 한정 스크립트는 `Scripts/Editor/r1_startup_repair.py`입니다. 첫 Resave1/TagsInspect1은 태그 설정 검증 실패로 저장하지 않았고, 그 사이 GameAfterResave1은 실제로 저장 전 상태의 재실패 이력입니다. 성공 근거는 Resave2/Reload1/GameAfterResave2/Regression1입니다.
- **다음은 R6 실제 발사 후0.15초 회복지연의 재현/최소 변경안**입니다. 원본에도 있는 LastFireTime/TimeLastFired 불일치를 보완하려면 근거·동작 차이를 설명한 뒤 진행합니다. 지금 R1의 두 보완점은 닫혔지만 R0~R6 전체 완료나 R7 착수 조건 전체 충족으로 확대하지 않습니다. R키 현재 미재현/관찰 보류와 사운드 사용자 통과는 유지합니다. 이번 결과에는 사용자 추가 빌드가 필요 없습니다.

---

## 0.13 재현·제안 이력 — 2026-10-09 16:27 KST R6 회복지연

- R1 보완 다음으로 R6의 시간 경로와 실제 라이플만 조사했습니다. **게임 C++/Config/에셋 변경0, C++ 빌드·패키징0**입니다. 새 범용 도구 없이 기존 Enhanced Input/PIE 진단을 재사용한 임시 `Saved/Diagnostics/R6SpreadDelay/probe.py`로 실제 장착 인스턴스의 탄약/열/퍼짐을 관찰했습니다.
- 원본/대상 모두 발사 Ability 활성화의 `UpdateFiringTime()`은 기반 `TimeLastFired`를 갱신하지만, `UpdateSpread()`는 대입이 없는 별도 `LastFireTime=0`을 읽습니다. `AddSpread()`는 유효 TargetData와 Commit 성공 뒤 호출됩니다. 원본에도 같은 연결 누락이 있으며 이식 과정에서 새로 발생한 문제로 보고하지 않습니다.
- `Saved/Logs/R6SpreadDelay-Before1.log` **16:27:44 KST**: Integration의 실제 B_WeaponInstance_Rifle, 설정 지연0.150000006,6발 발사/탄약30→24. 마지막 발사 게임시간2.7523635에서 열3.242385/퍼짐5.282824°, **0.019776초 뒤** 탄약24 그대로 열3.163281/퍼짐5.225622°로 감소했습니다. 전체 발사/회복 관찰에서 지연 내 감소76샘플, `early_cooling_reproduced`입니다. 장착 후2초 이상 대기하고 발사를 관찰했으므로 이 조건에서 `GetTimeSinceLastInteractedWith()`는 최근 발사시간을 나타냅니다. 일반적으로 장착시간도 포함하는 getter임은 유지합니다.
- 이 진단 프로세스에서만 엔진 `AbilitySystem.DisableGameplayCues=1`/NullRHI/NoSound를 사용했습니다. Cue/시각/오디오/복제 검증은 아닙니다. stock `Project.Maps.PIE`는 **16:27:53 성공1/실패0/보고서경고0**, 종료0입니다. 이는 PIE 실행 성공이며 퍼짐 버그가 해결됐다는 판정이 아닙니다. 전체 로그에는 기존 LyraGame import1/PoseAsset14 로드 경고가 있고 Error/Fatal/ensure는 없습니다. 검사한 C++/Config/실에셋/Git인덱스11파일 해시 불변, 저장0입니다.
- **제안만 했으며 아직 적용하지 않은 변경:** `DCLyraRangedWeaponInstance.cpp::AddSpread()`에서 성공한 발사의 `LastFireTime`을 갱신하고, 기존 `DCWeaponFoundationAutomationTests.cpp`에 양의 회복지연/연속발사 시 지연 재시작/지연 뒤 원본 곡선 회복 검사만 추가합니다. production CPP1개+기존 test CPP1개이며 헤더/에셋/Config/발사간격/탄약/재장전/사운드는 변경하지 않습니다. 사용자 승인 후 코드만 적용하고 사용자 빌드 뒤 기존 검사와 동일 PIE 관찰을 반복합니다.
- 원본 보존 대안은 현재 코드/실제 즉시 회복 동작을 그대로 두고0.15초 설정이 작동하지 않는 제한을 명시하는 것입니다. 최소 수정안은 그 대신 설정된 지연을 실제로 적용하므로 **연사 중 냉각 감소, 열/퍼짐 누적 증가 가능**이라는 원본 대비 동작 차이가 있습니다. 기반 activity getter를 그대로 회복에 쓰면 장착 시점도 회복 지연에 섞이고 AutoReload 활동 시계와 결합하므로 선택하지 않습니다. 원본 곡선/배율과 기존 활동 시계는 보존합니다.
- 현재는 **R0~R6 완료 감사 중 R6 보완안 승인 대기**, R7 착수 보류입니다. R2/3/5 수명 결합 회귀와 R4/6 남은 비교, R키 현재 미재현/관찰 보류, 사운드 사용자 통과는 그대로 유지합니다. 이번 진단만으로 단계 전체 완료로 올리지 않습니다.

---

## 0.14 코드 작성 이력 — 2026-10-09 R6 회복지연

- 위 두 파일 최소 수정안에 사용자가 진행을 승인했습니다. `DCLyraRangedWeaponInstance.cpp::AddSpread()`에 **LastFireTime 갱신1문장+설명주석만** 추가했습니다. 기반 활동시계, 원본 곡선/회복 계산/배율, 발사간격/탄약/재장전/사운드는 유지합니다. 지연이 이제 실제 적용되어 연사 중 열/퍼짐이 기존보다 누적될 수 있다는 승인된 차이는 유지합니다.
- 기존 `DCWeaponFoundationAutomationTests.cpp`에 `DreamCatcher.R6.Weapon.SpreadRecoveryDelay` 검사1건을 추가했습니다. transient 무기의 합성 곡선과 지연0.15를 사용해 발사0.10초 뒤 회복 금지, 두 번째 발사0.10초 뒤에도 회복 금지(첫 발사 기준0.20초), 두 번째 발사0.16초 이후 원본 곡선 냉각 및 연속 냉각을 검사하도록 작성했습니다. 기존 검사2건의 코드는 변경하지 않았습니다.
- 정적 확인에서 production CPP는 신규 주석/대입만 제거하면 작업 전과 동일하고, test CPP는 신규 검사 블록만 제거하면 기존과 동일합니다(개행 정규화 비교). Config/헤더/기반·발사Ability CPP/실에셋/Git인덱스/기존DLL 보호12파일 해시 불변, diff 공백 오류0입니다. **새 테스트를 실행해 통과한 상태는 아닙니다.**
- C++ 빌드·패키징·Unreal 실행·에셋 저장은 하지 않았습니다. 현재 **코드 작성 완료, 사용자 Development Editor/Win64 빌드 및 수정 후 런타임 검증 대기**입니다. 이전 수정 전 PIE 성공/지연 재현을 수정 후 통과로 재사용하지 않습니다.
- 다음은 사용자 빌드 확인 → 기존 무기/장비 회귀와 신규 SpreadRecoveryDelay → 동일 실제 라이플 PIE 수치 관찰입니다. 그 뒤 R0~R6 감사의 나머지 결합 회귀를 이어갑니다. R7은 아직 보류이며 R키/오디오의 기존 판정은 그대로입니다.

---

## 0.15 검증 이력 — 2026-10-09 16:44 KST 회복지연·실무기 수명 검증 통과

- 사용자 UBT 빌드 **16:36:54 시작/12.49초 성공**, RangedWeaponInstance/Test CPP2개 컴파일, 게임DLL **16:37:06/4,390,912bytes** 갱신을 확인했습니다. Codex는 빌드/패키징을 실행하지 않았습니다.
- `Saved/Automation/R6SpreadDelayRegression_20261009/index.json` **16:38:25**: 신규 SpreadRecoveryDelay 포함 기존 장비/팀/타입/무기 **9건 성공, 실패/보고서경고/notRun0**. 전체 로그 Error/Fatal/ensure0, 초기 B_LyraGameInstance의 LyraGame import 경고1은 별도로 남습니다.
- `R6SpreadDelay-After1.log` **16:39:28**, 실제 Rifle6발/탄약30→24: 마지막 발사 후0.148285초까지 열6.25/퍼짐7.457666° 유지,0.157990초에서 열6.211184/퍼짐7.429597°로 첫 냉각,0.509386초까지 냉각 지속. 지연 내 냉각0샘플입니다. **설정0.15초 회복지연 수정 후 검증 통과**이며 R6 전체 완료는 아닙니다. After 보고서는16:39:37 PIE 성공1/실패0/보고서경고0, 종료0입니다.
- 다음 감사 항목도 기존 공개 API/임시 PIE로 확인했습니다. `Saved/Diagnostics/R6SpreadDelay/lifecycle_probe.py`, `R2R5RifleLifecycle-PIE2.log` **16:44:30**: 사격30→26 → 빈 슬롯/장비0/Ability9→6/무기Actor 제거 → 같은 Item 재장착26발/장비1/Ability9복구 → 사격 중 UnPossess/장비0/인벤토리0/추적태그4종0 → 이전 Pawn 파괴 후 RestartPlayer → 같은 ASC의 새 Pawn/신규 Item30발/장비1/Ability9 → 사격30→27 및 입력해제 후 추적태그0. Ordered 보고서는 **16:44:45 PIE 성공1/실패0/보고서경고0**, 종료0/Error/Fatal/ensure0입니다.
- **첫 수명진단 PIE1은 실패1로 보존**합니다. 이전 권한 Pawn의 ASC를 해제하지 않은 채 새 Pawn을 만들도록 진단을 작성해, 원본에도 있는 `ensure(!ExistingAvatar->HasAuthority())`를 유발했습니다. Python finished 표시는 전체 성공 근거가 아닙니다. 원본 주석은 이 중첩 경로를 지연된 client에 한정합니다. 게임 코드를 수정하거나 ensure를 제거하지 않고 진단만 권한 Pawn 정리→새 Pawn 순서로 교정했습니다. client 지연/늦은 이전 Pawn 정리까지 통과한 것으로 확대하지 않습니다.
- 두 실제 PIE는 진단 프로세스의 Cue비활성/NullRHI/NoSound 조건이며 렌더/오디오/물리키/네트워크 검증이 아닙니다. 실제 PIE의 초기 import1/PoseAsset14 로드 경고도 남습니다. 이번 턴은 기존 검사·임시 진단·문서만 작업했고 C++/Config/실에셋/Git인덱스/DLL 보호14파일 해시 불변, 에셋 저장0입니다.
- **현재: R0~R6 감사 중.** R1의 두 보완점과 R6 회복지연은 닫혔고, R2/R5의 실제 재장착·사격중 조종해제·권한 Pawn 순차 교체 범위를 추가 통과했습니다. 사망/치료/면역/레벨재시작 결합(R3), 카메라/움직임·공중 배율/반동·표면(R4/R6), 기존 장치/복제 보류는 남습니다. 다음은 **R3 실제 플레이어의 피해·치료·면역·사망→정리→레벨재시작 진단**입니다. 새 게임 코드 변경이 필요하면 근거/범위를 먼저 제시합니다. 지금 추가 사용자 빌드는 필요 없고 R7은 아직 보류입니다.

---

## 0.16 검증 이력 — 2026-10-09 16:58 KST R3 피해·면역·사망·재시작

- 사용자 요청으로 기존 API/GE와 별도 PIE만 사용했습니다. 임시 진단 `Saved/Diagnostics/R3HealthLifecycle/probe.py`이며 **게임 C++/Config/에셋 변경0, 에셋 저장0, 빌드·패키징0**입니다. 최신 게임DLL은 앞선 사용자 빌드16:37:06 그대로입니다. 보호16파일(관련 C++/Config/실에셋/Git인덱스/DLL) 해시도 동일했습니다.
- `R3HealthLifecycle-PIE2.log` **16:54:56~16:55:07**: 기존 Damage/Heal SetByCaller GE로100→75→85→상한100, DamageImmunity 중25피해 차단/해제 뒤100→75를 확인했습니다. 사격30→27 도중 치명피해로HP0, MovementMode=None/캡슐충돌없음,0.5초 입력유지 중 탄약27 유지. HP0에 재피해를 줘도 DeathStarted/Finished/RestartQueued 로그는 각각1회입니다.
- 사망8초 뒤 정리에서 Inventory0/무기Actor 제거/Ability9→6/사망·발사·재장전·NoFiring·InputBlocked 태그0, 약8.109초에 이전 Pawn 파괴를 확인했습니다. 종료2초 뒤 자동 레벨재시작으로 체력100/탄약30/장비1/Inventory1/Ability9/태그0 복구, 새 입력으로30→27 사격 후 정상 해제까지 확인했습니다. `R3HealthLifecycleReloadSafe_20261009/index.json` **16:55:30 성공1/실패0/보고서경고0**, 종료0/Error/Fatal/ensure0입니다.
- 구형 적의 `ApplyDamage` 호환 경계도 별도로 검증했습니다. 실제 적 AI 대신 **ASC 없는 기존 월드Actor**를 DamageCauser로 사용했습니다. Integration Pawn의 기존 GE는 `/Game/DreamCatcher/GAS/Effects/Combat/GE_DC_Damage`이며5피해 시100→95/반환5, 면역 중에는95유지/반환0입니다. `R3LegacyBridge-PIE2.log` **16:58:41**, `R3LegacyBridgeExistingActor_20261009/index.json` **16:58:47 성공1/실패0/보고서경고0**, 종료0/Error/Fatal/ensure0. 적의 AI·조준·사거리·팀 관계·적 자체 GAS 전환 성공은 아닙니다.
- 실패 이력은 삭제/성공 처리하지 않습니다. Health PIE1은 맵 재시작 후 GC된 이전 Pawn Python wrapper를 IsValid에 전달해 마지막 확인이 중단됐습니다. 재실행은 travel 전에 경로로 다시 조회해 파괴를 확인한 뒤 새 월드만 읽도록 진단을 교정했습니다. Legacy PIE1은 Python에 미노출된 actor spawn API 때문에 적용 전 실패했고 기존 월드Actor로 교정했습니다. **두 실패 모두 보고서 실패1, 게임 코드 수정 없음**입니다.
- 두 성공 진단은 Cue비활성/NullRHI/NoSound, Action 주입 조건입니다. 시각·사운드·물리키·복제는 검증하지 않았고 초기 LyraGame import1/PoseAsset14 로드 경고는 그대로 남습니다. 원본 사망 흐름과 기존 프로젝트의 사망 종료 후2초 OpenLevel 연결을 확인한 것이며, 원본 Lyra의 전체 respawn/Experience 흐름을 이식한 것으로 확대하지 않습니다.
- 현재 **R0~R6 감사 중 R3의 이번 플레이어 결합 회귀와 기존 ApplyDamage 입력 경계는 통과**입니다. 구형 적은 여전히 비GAS HealthComponent이며 적 GAS/실제 전투 통합은 R10에서 확인합니다. 다음은 **R4/R6 조준·이동·공중 퍼짐 배율/카메라 연결, 반동 중복·거리/표면/엄폐 비교**입니다. 자동 수치 검증과 사용자의 화면/조작감 확인을 나누고 새 수정 필요 시 원본 근거·범위를 먼저 제시합니다. R7은 아직 보류이며 추가 사용자 빌드는 필요 없습니다.

---

## 0.17 원인·제안 이력 — 2026-10-09 17:17 KST R4/R6 웅크리기 누락

- 원본/현재 RangedWeapon.UpdateMultipliers와 CameraModeStack.GetBlendInfo를 읽고, 기존 DLL의 실제 Integration Pawn을 별도 PIE에서 움직여 확인했습니다. 임시 진단은 `Saved/Diagnostics/R4R6AimSpread/probe.py`, 결과 `R4R6AimSpread-PIE1.log` **17:08:58~17:09:13**입니다. 게임 C++/Config/에셋 변경·저장/빌드/패키징0, 보호13파일 해시 불변입니다.
- 실제 원본 계수는 조준0.65/정지0.8/웅크림0.6/공중1.6이며 기존 원본 에셋 조사와 일치합니다. 정지0.8, 이동600cm/s에서 약0.997→1, 착지 후0.8, 점프 중 최고1.578을 관찰했습니다. Shoulder는FOV70/배율0.52/MaxWalkSpeed300, Scope는FOV40/배율0.52/300, 종료 후Hip FOV80/배율0.8/600 복구와 Scope 해제 입력 유지 중Hip 유지가 통과했습니다. **배율은 첫발 정확도0 덮어쓰기 전 raw debug 값**이며 실제 최종 첫발 퍼짐이나 UI 검증으로 확대하지 않습니다. 이동/조준은 Action 주입, 웅크림/점프는 Character API 요청입니다.
- **웅크리기 실패는 실제 누락:** 정지/Walking 상태에서 Crouch()를 요청해도 bIsCrouched=false,배율0.8 유지. 엔진이 NavAgentSettings에서 crouching disabled라고 기록했습니다. 전체 Automation 보고서는 **17:09:26 실패1**이며 통과한 다른 상태로 덮지 않습니다. `R4R6Crouch-Inspect1.log`17:12:02에서 native CDO/BP CDO/실제 instance 모두 bCanCrouch=false, ledge=false, CrouchedHalfHeight40, BaseEyeHeight64/CrouchedEyeHeight32를 확인했습니다. InputTag.Crouch도 NativeInputActions에서 빠져 있습니다.
- 원본 LyraCharacter는 bCanCrouch=true/웅크림ledge=true/halfheight65/BaseEyeHeight80/CrouchedEyeHeight50 및 OnStart/EndCrouch의 Status.Crouching 태그1/0을 사용합니다. 대상 Character에는 태그 hook도 없습니다. `R4R6Crouch-InputSource2.log`/`InputTarget2.log` **17:17:22**에서 원본과 기존 이식 IA_Crouch의 Bool/Pressed/Modifier없음, LeftControl 및 Gamepad_FaceButton_Right(추가Pressed) 매핑 일치를 확인했습니다. IA_Crouch는 이미 `/Game/LyraMigration/BaseInput/Actions/IA_Crouch`에 있으므로 재이식할 필요가 없습니다. 첫 InputSource/Target은 UE5.8의 deprecated Mappings를 읽은 것으로 빈 키 목록을 근거로 쓰지 않습니다. 실제 근거는 DefaultKeyMappings.Mappings를 읽은 Source2/Target2입니다.
- 카메라 전환 중FOV가80→70으로 바뀌는 동안 raw배율0.8, 복귀 중에는0.52를 유지하다 전환 완료 때 바뀌는 것도 관찰했습니다. 원본과 대상 모두 새 모드를 index0에 넣지만 GetBlendInfo는 Last()를 읽습니다. **로컬 원본과 같은 전환 특성**으로 기록하고 이번에 카메라 계산을 고치지 않습니다. protected CameraModeStack 직접 조회는 거부됐으므로 런타임 blend weight를 직접 읽었다고 하지 않습니다.
- **다음 제안(미적용): 원본 웅크리기 연결 복구 묶음.** Character.h/cpp에 원본 OnStart/EndCrouch 태그 hook만 추가하고, Integration의 BP_DC_RiflePawn에 위 원본 웅크리기/시야5개 기본값을 설정합니다. DA_DC_RifleInput에 기존 IA_Crouch/InputTag.Crouch를 연결하고 새 `IMC_DC_RifleCrouch`에 원본2키/트리거만 넣어 해당 Pawn Hero에 추가합니다. 공용 IMC_Player와 전체 IMC_Default는 바꾸거나 통째로 활성화하지 않습니다. 원본 C++ 생성자 기본값을 테스트 BP에 한정 적용하는 구성 위치 차이이며, 기존 플레이어 전체의 기본값을 한 번에 바꾸는 대안보다 활성 맵 범위가 작습니다. 원본 이식 불가를 주장하는 것이 아닙니다.
- 기존 standing capsule42/96, 다른 이동/충돌/조준/재장전/사운드/카메라 blend 계산은 유지합니다. 원본 전체 Character 기본값과 동등하다는 뜻이 아닙니다. 이 묶음은 R4/R6 웅크림 조건에 필요한 R8의 일부 선행이며 전체 애니메이션 이식 완료는 아닙니다. 시야높이64→80/32→50과 crouch capsule40→65로 화면 구도가 바뀔 수 있음을 먼저 안내합니다. 단순히 허용flag만 켜는 대안은 원본 입력/태그/자세값의 누락이 남습니다.
- 사용자 승인 뒤 코드2파일+기존에셋2개/신규IMC1개를 한정 처리하고, 사용자 빌드 후 상태/태그/입력/카메라 및 기존 발사·재장전 회귀를 확인합니다. 현재는 **원인 확인/복구안 승인 대기**, 수정 후 검증 미실행입니다. 게임패드/시각/복제 및 벽·반동/거리·표면/엄폐 비교는 남고 R7은 아직 보류입니다.

---

## 0.18 적용 이력 — 2026-10-09 17:34 KST 웅크리기 코드·에셋 반영

- 사용자 승인한 묶음을 적용했습니다. `DreamCatcherCharacter.h/.cpp`에 원본 OnStartCrouch/OnEndCrouch의 Status.Crouching1/0과 Super 호출만 추가했습니다(헤더4줄/CPP20줄). 공용 생성자/카메라 계산/발사·재장전 코드는 변경하지 않았습니다. **새 C++ 빌드·실행 검증은 미완료**이며 Codex는 빌드·패키징을 하지 않았습니다.
- Unreal 내부에서 기존 Integration `BP_DC_RiflePawn`/`DA_DC_RifleInput`2개와 신규 `IMC_DC_RifleCrouch`1개만 저장했습니다. Pawn의 crouch 허용/ledge=true, halfheight65, 눈높이80/50 및 Hero 전용IMC(priority1)를 적용했고, NativeInputActions에 기존 원본 IA_Crouch/InputTag.Crouch1개를 추가했습니다. 기존 입력4개/Ability 입력5개/기존IMC3개는 그대로입니다.
- 새IMC는 원본 LeftControl 및 Gamepad_FaceButton_Right2키만 가지며, gamepad의 추가Pressed(임계0.5/AlwaysTick=false)도 보존했습니다. 트리거 객체는 새IMC 내부에 생성되어 원본IMC private 객체를 참조하지 않습니다. 기존 IA_Crouch/IMC_Default/IMC_Player는 수정하지 않았습니다. MaxWalkSpeed600/웅크림속도300/가속2048/제동1800/AirControl0.2, standing capsule42/96, Hero 전환flag3개도 보존했습니다.
- 한정 자동화는 `Scripts/Editor/r4_crouch_setup.py`입니다. `R4Crouch-Prepare2.log` **17:32:54**: Pawn Blueprint Compile/메모리 값 비교 후 정확히3개 저장·종료0. `R4Crouch-Reload1.log` **17:34:58**: 새 프로세스에서 값/기존입력·IMC보존/원본키·트리거/소유자 재검증 통과·저장0·종료0. Error/Fatal/ensure0이며 기존 Manny PoseAsset 로드 경고는 남습니다.
- Inspect1은 기존 안전검사의 필수 `-DCRifleCopyDiagnostics` 인자를 빠뜨려 저장 전에 중단됐습니다. Prepare1도 Blueprint 추가 Hero를 CDO의 실제 컴포넌트로 찾으려다 사전 조회에서 중단됐습니다. 기존 SubobjectData 템플릿 조회를 재사용하도록 교정했고 **두 실패는 저장0**, 성공 근거는 Prepare2/Reload1뿐입니다. 보조 C++ 도구를 수정하거나 보호 검사를 우회하지 않았습니다.
- 외부 보호13파일(Config/원본입력/기존플레이어/PawnData/맵/무기·카메라CPP/기존DLL/Git인덱스) 해시 불변입니다. 스크립트 안에서도 보호 package/동반 파일 경로53개 상태를 비교했습니다. 에셋 준비는 기존16:37:06 DLL로 했으므로 새 Status.Crouching hook이 실행됐다고 보고하지 않습니다. 새 클래스/UPROPERTY 없이 기존 에셋 설정만 저장한 것입니다.
- **다음은 사용자 Development Editor/Win64 빌드**입니다. 그 뒤 실제 crouch Action 토글→캡슐65/96·Status.Crouching1/0·raw배율0.48/0.8, 조준/점프/발사·재장전/수명 회귀와 카메라 높이를 확인합니다. 입력/기본값 자동 저장을 전체 플레이나 R8 시각·게임패드·복제 성공으로 확대하지 않습니다. R7/벽·반동/거리·표면 검증은 여전히 남으며, 사용자 추가 수동 에셋 설정은 없습니다.

---

## 0.19 검증·제안 이력 — 2026-10-09 18:07 KST 웅크림·수치 통과 / Pawn 교체 태그 잔류

- 최신 사용자 빌드 **17:38:54 시작/28.75초 Succeeded**, Character CPP/UHT 반영과 DLL **17:39:22/4,391,936bytes** 갱신을 확인했습니다. Codex는 빌드·패키징·게임 코드/Config/에셋 저장을 하지 않았고, 이번 보호14파일 해시 불변입니다.
- `R4Crouch-PostBuild1.log`17:43:43~45에서 **실제 IA_Crouch Action** 입력으로 Status.Crouching1/캡슐65/raw배율0.480673, release 뒤 crouch 유지, 다시 누르면 태그0/캡슐96/raw0.799360 복구를 확인했습니다. 정지/이동/점프/착지/Shoulder/Scope/해제 회귀도 통과. `R4CrouchPostBuild_20261009/index.json` **17:44:06 기존9건+PIE1=성공10/실패0/보고서경고0**입니다. 물리 Ctrl키/렌더 검증은 별도입니다.
- `R6CombatNumbers-PIE5.log` **18:05:16~27**에서 기존 라이플 실발사로650cm 위치표적12/3600cm6, 임시 WeakSpot 물리재질에서18/9, 엄폐0/제거후12, 탄약1발씩 소모·수동재장전30복구를 확인했습니다. 기존 Reticle Widget을 화면 없이 임시 생성해 native 조회함수로 냉각완료 ADS의 FirstShot=true/최종각0, 발사 후false/최종각1.90389=3.661328×0.52, 냉각 뒤복구까지 확인했습니다. 전체 HUD/Reticle 화면 연결 완료는 아닙니다.
- 같은 임시벽으로 카메라-캐릭터 거리가313.3295→161.8578cm, 벽 제거1.2초 뒤313.2881cm 복구했습니다. `R6CombatNumbersWarm_20261009/index.json` **18:05:42 성공1/실패0/보고서경고0/종료0**, Error/Fatal/ensure0입니다. Cue를 꺼서 연출/CameraShake 중복은 이 검사로 검증하지 않았습니다.
- CombatNumbers PIE1~3은 진단의 Python enum명/물리재질 속성 위치 때문에 발사 전에 실패했습니다. 엔진 ScriptName=CollisionResponseType와 BodyInstance.PhysMaterialOverride로 교정했습니다. PIE4는 다른 수치검사는 통과했지만 장착 초기열(midpoint6)을1초만 기다려 첫 cold-shot 조건에 실패했습니다. 첫 대기를2초로 늘린 PIE5만 전체 통과 근거입니다. 게임의 열/곡선/충돌 규칙을 바꿔 테스트를 맞추지 않았습니다.
- **실제 미해결1건:** 기존 순차 Pawn 교체 진단에 crouch를 추가한 `R4Crouch-Handoff1.log` **18:06:52**에서 이전 Pawn crouch태그1 → 제거/ASC재사용 → 새 Pawn bIsCrouched=false인데 **Status.Crouching=1**이 남았습니다. 장비1/Inventory1/Ability9는 정상. `R4CrouchHandoff_20261009/index.json`18:07:08 **실패1**이며 이는 진단 API 오류와 다른 실제 상태 불일치입니다.
- 원본 OnStart/EndCrouch는 동작 전환에만 태그를1/0으로 맞춥니다. 원본 OnAbilitySystemUninitialized는 Health만 해제하고 InitializeGameplayTags는 movement-mode 태그만 초기화하여 crouch태그는 포함하지 않습니다. 대상 Character에도 ASC 재사용 경계의 crouch 초기화/해제가 없습니다. **제안(미적용): Character.cpp1파일의 OnAbilitySystemInitialized에서 실제 IsCrouched()에 태그1/0을 맞추고, OnAbilitySystemUninitialized에서 해당 Avatar 해제 범위의 태그를0으로 정리합니다. 새 Avatar의 태그를 오래된 Pawn이 지우지 않도록 소유관계를 확인합니다.** 기존 원본 Start/End hook/입력/물리자세/카메라/Ability 부여와 에셋은 그대로입니다.
- 원본 대비 추가되는 것은 ASC재사용 때 Pawn 상태 태그 동기화뿐입니다. 대안인 교체 전 강제 UnCrouch는 실제 캡슐/자세를 바꾸며, ASC 전체 초기화는 지속 상태를 지울 수 있으므로 제안하지 않습니다. 사용자 승인/빌드 후 정상 웅크림 토글과 동일 handoff 회귀를 재실행합니다. 새 범용 도구/에셋 재작업은 필요 없습니다.
- 사용자는 최신 빌드의 **화면·조작감은 아직 확인하지 않았다고 답했습니다.** 이번이 전체의 마지막 검증이라고 약속하지 않습니다. 현재 마감 조건은 (1) 확인된 crouch 태그 잔류1건 보완·해당 회귀, (2) 사용자 웅크림/카메라 구도·조준/연사 반동 확인입니다. 원거리/약점/엄폐/첫발/가림 수치의 이미 통과한 범위는 반복 확장하지 않습니다. 다른 문제가 없으면 R7 연결 제안으로 넘어가며 장치/복제/전체 실맵 회귀는 기존 단계 범위를 유지합니다.

---

## 0.20 코드 작성 이력 — 2026-10-09 crouch 태그 수명 보완

- 사용자가 위 최소안을 승인하여 **DreamCatcherCharacter.cpp1파일만** 수정했습니다. 기존 Start/EndCrouch hook은 그대로이며 새 헤더/에셋/Config/도구는 추가하지 않았습니다.
- OnAbilitySystemInitialized는 ASC의 Avatar가 this일 때만 Status.Crouching을 실제 IsCrouched()?1:0으로 동기화합니다. OnAbilitySystemUninitialized는 ASC가 있고 Avatar가 null 또는 this일 때만 해당 태그를0으로 정리합니다. PawnExtension이 Avatar를 먼저 비우고 uninitialized를 알리는 현재 순서에 맞추며, 이미 다른 Pawn을 가리키면 건드리지 않습니다.
- 원본 대비 차이는 이 초기화/해제 경계의 crouch 태그 동기화뿐입니다. 강제 UnCrouch/캡슐 변경/ASC 재생성/전체 태그 초기화 없이 다른 지속 상태를 보존합니다. 입력·조준·발사·재장전·카메라 규칙과 이전 웅크림 에셋은 그대로입니다.
- 정적 확인: 이번 추가16줄을 제외한 CPP는 작업 직전과 동일(개행 정규화 SHA256). 헤더/PawnExtension/Config/에셋/DLL/Git인덱스 등 보호12파일 해시 불변, diff 공백 오류0입니다. **C++ 빌드·Unreal 실행·패키징·에셋 저장은 하지 않았고 수정 후 실행 검증은 미완료**입니다.
- 다음은 사용자 Development Editor/Win64 빌드 → 기존 `R4Crouch-Handoff1`의 동일 조건 재검사와 정상 crouch Action 토글 회귀입니다. 화면/카메라 구도·조준/연사 반동은 사용자 미확인 상태를 유지합니다. 이 두 마감 조건이 통과하고 새 문제가 없으면 R7 연결 제안으로 넘어가며, 이미 통과한 수치 검증 범위를 다시 확대하지 않습니다.

---

## 0.21 최신 재개 — 2026-10-09 18:26 KST 태그 잔류 회귀 통과 / 이번 자동 감사 마감

- 사용자 UBT 실제 컴파일 기록은 **18:20:53 시작/8.65초 Succeeded**, Character.cpp 컴파일 및 DLL **18:21:01/4,391,936bytes** 갱신입니다. 직후18:21:11의 최신 Log.txt는 Target up to date/1.28초 성공이고 실제 컴파일 기록과 구분했습니다. Codex는 빌드를 실행하지 않았습니다.
- `R4Crouch-Handoff2.log` **18:24:14**에서 동일한 crouch 중 Pawn 교체 후 새 Pawn은 **bIsCrouched=false/Status.Crouching0**입니다. 같은 ASC 재사용, Ability9/장비1/Inventory1, 탄약30→27 재사격과 최종태그0까지 정상입니다. `R4CrouchHandoffFixed_20261009/index.json` **18:24:30 성공1/실패0/보고서경고0**으로 기존 잔류 결함을 닫았습니다.
- `R4Crouch-ToggleFixed1.log` **18:26:20~21**의 정상 Action 토글도 태그1/캡슐65/raw0.480661 → release 후 crouch 유지 → 다시 입력하면0/96/raw0.799338로 통과했습니다. `R4CrouchToggleFixed_20261009/index.json` **18:26:24 성공1/실패0/보고서경고0**입니다. 기존 임시 진단에 토글만 확인하는 모드를 사용했고 이미 통과한 거리/표면 수치를 재검사하지 않았습니다.
- 두 프로세스 종료0, Error/Fatal/ensure0이며 기존 import/PoseAsset 로드 경고15개는 남습니다. 게임 C++/Config/에셋 저장/빌드/패키징0, 보호11파일 해시 불변입니다. 장치/물리키/시각/복제 전체 검증이 아닙니다.
- **R7 전으로 합의한 이번 R0~R6 자동 감사는 마감합니다.** 알려진 태그 잔류까지 해결·재검증했고 검사를 새로 늘리지 않습니다. 남은 마감 조건은 사용자 화면 확인: (1) Left Ctrl 웅크림/해제 자세·카메라 높이, (2) 조준/연사 시 비정상적인 이중 반동·떨림 여부입니다. 마지막 명시 답변이 미확인이므로 확인됐다고 추정하지 않습니다.
- 다음 작업은 **R7-1/2 원본 ReticleHost·라이플 조준점/탄약 표시의 통합 테스트 맵 연결**입니다. 화면 확인이 정상이라면 원본 대비 연결 차이와 정확한 코드·에셋 범위를 다시 제시/승인 후 진행합니다. R7 새 구현은 아직 미적용입니다. 기존 장치/복제/전체 실맵·R8 시각·패키징 범위가 완료됐다는 뜻은 아닙니다.

---

# 1. 프로젝트 정체성

## 1.1 프로젝트 한 줄 정의

**DreamCatcher는 짧고 밀도 높은 전투 세션과 캐릭터 연출을 중심으로 한 Unreal Engine 기반 3인칭 오버숄더 서브컬처 슈터입니다.**

기획 문구:

> 꿈과 현실의 경계에서 펼쳐지는 청량한 근미래 서브컬처 TPS 슈팅 게임.

## 1.2 팀 구성

프로젝트는 개발자와 디자이너 2명이 제작합니다.

### 개발자 담당

- C++ 게임플레이 아키텍처
- 입력 시스템
- 플레이어 상태 및 전투 규칙
- 체력, 데미지, 사망
- 적 행동 규칙
- 보스 로직
- 스포너, 인카운터, 스테이지 진행
- HUD 데이터 바인딩
- 결과, 보상, 강화, 저장 로직
- DataAsset 적용
- 빌드 및 기술 협업

### 디자이너 담당

- 캐릭터 및 배경 디자인
- 애니메이션 연결
- VFX, SFX 연결
- UI 레이아웃과 시각 디자인
- 레벨 블록아웃과 환경 배치
- 전투 가독성
- 컷신 자산
- Blueprint 연출
- 캐릭터 매력과 시각적 완성도

### 공동 결정 항목

- 무기 손맛
- 카메라 동작
- 회피 감각
- 적 공격 텔레그래프
- 전투 템포
- 보스 난이도
- 레벨 가독성
- 결과 및 보상 루프
- 최종 UI 사용성

## 1.3 핵심 감정

플레이어가 느껴야 할 핵심 감정:

- 신난다
- 시원하다
- 귀엽다
- 청량한 근미래 세계에 몰입된다
- 적 공격을 읽고 명확하게 대응할 수 있다

게임은 지나치게 어둡거나 군사적이거나 복잡한 방향으로 가지 않습니다.

## 1.4 참고 방향

프로젝트에서 참고한 주요 방향:

- **Snowbreak**: 3인칭 오버숄더 슈팅
- **Zenless Zone Zero**: 플랫하고 강한 그래픽 UI
- **No Straight Roads**: 에너지 있는 시청각 연출
- **Blue Archive**: 서브컬처 캐릭터 매력, 비율, 셀 채색

직접적인 모방이 아니라 방향 참고입니다.

---

# 2. 게임 범위

## 2.1 목표 플레이 세션

프로토타입 기준:

- 약 **5~8분**
- 짧지만 완결된 한 판
- 시작, 전개, 보스 클라이맥스, 결과가 있는 구조

## 2.2 핵심 플레이어 행동

현재 코어 액션은 의도적으로 작게 유지합니다.

- 이동
- 카메라 조작
- 기본 공격
- 조준
- 회피
- 궁극기

이 핵심 행동은 Lyra Starter Game의 GAS 기반 Ability, GameplayEffect,
AttributeSet, Gameplay Tag, AbilitySet 구조로 단계적으로 전환합니다.
이식 가능한 원본 코드와 에셋을 우선 재사용하고, DreamCatcher 연결에 필요한 부분만 최소 수정합니다.

기존 전투 시스템은 원본 이식 구현이 승인된 목표 동작을 정상적으로 수행하는 것이
검증된 뒤 단계적으로 제거합니다. 사용자가 폐기한 회피 방향 규칙까지 유지하는 것은 아닙니다.

## 2.3 목표 스테이지 흐름

첫 수직 슬라이스 목표:

1. 시작
2. 인트로 컷신
3. 전투 1
4. 전투 2
5. 전환 컷신
6. 보스전
7. 결과

처음부터 큰 게임을 만드는 것이 목표가 아닙니다.  
짧지만 처음부터 끝까지 플레이 가능한 수직 슬라이스 하나를 완성하는 것이 우선입니다.

## 2.4 향후 메타 루프

전투 슬라이스가 안정된 이후:

1. 스테이지 선택
2. 로드아웃
3. 스테이지 플레이
4. 결과
5. 보상
6. 강화
7. 저장
8. 다음 플레이

메타 상태는 `GameMode`가 소유하지 않습니다.

---

# 3. 세계관

## 3.1 루시드 시티

주요 배경은 **루시드 시티(Lucid City)** 입니다.

꿈 기반 기술과 평범한 청춘의 일상이 공존하는 청량한 근미래 도시입니다.

시각적 특징:

- 깨끗한 도시 인프라
- 거대한 기업 또는 공공기관 건축
- 투명한 근미래 기술
- 꿈을 연상시키는 빛과 색
- 스트리트 문화
- 전투 동선이 명확한 공간
- 지나치게 어두운 사이버펑크가 아닌 밝고 청량한 분위기

## 3.2 꿈 에너지와 드림 코어

세계의 핵심 개념은 꿈을 에너지로 변환하는 기술입니다.

### 드림 코어

**드림 코어(Dream Core)** 는 꿈을 변환해 만들어지는 실용 에너지입니다.

도시의 기술과 인프라를 움직이는 중요한 에너지원입니다.

### 악몽 부산물

꿈을 에너지로 변환하는 과정에서는 위험한 부산물이 발생합니다.

이 부산물은:

- 악몽과 관련된 오염으로 나타납니다.
- 침식 또는 변질을 일으킵니다.
- 공간, 물체, 신호, 생명체에 영향을 줄 수 있습니다.
- 전문 처리 조직이 필요한 사건을 만듭니다.

정확한 과학 원리, 공식 명칭, 정치적 의미는 아직 완전히 확정되지 않았습니다.  
별도 스토리 명세 없이 세부 설정을 임의로 확정하지 않습니다.

## 3.3 드림캐쳐 조직

DreamCatcher는 악몽 사건을 처리하는 중심 조직 또는 대응 부대입니다.

조직의 분위기:

- 공식적인 대형 기관
- 기업과 공공기관이 결합된 인상
- 악몽 처리 전문 조직
- 높은 위치의 간부와 젊은 현장 요원이 공존하는 구조

악몽처리반의 최종 공식 영문 부제는 아직 확정되지 않았습니다.

## 3.4 환경 스토리텔링

배경은 다음 내용을 전달해야 합니다.

- 꿈 기술로 움직이는 도시
- 겉으로는 정상적인 일상
- 그 아래 존재하는 악몽 위험
- 드림캐쳐 조직의 강한 존재감
- 전투 동선의 명확성
- 대규모 침식 사건의 흔적

모든 면에 세부 묘사를 가득 채우지 않습니다.

- 중심부는 대비, 디테일, 빛을 강하게 합니다.
- 주변부는 대비와 묘사를 줄입니다.
- 플레이어의 시선이 필요한 곳에 집중되게 합니다.

## 3.5 BIG BROTHER 보스 콘셉트

논의된 주요 보스 콘셉트:

**BIG BROTHER**

작업 문구:

> 수많은 신호 속에서 깨어난 관측자.

핵심 요소:

- 감시
- 관측
- 정보 노이즈
- 신호 왜곡
- 기계 눈
- 거대한 몸체 또는 머리
- 마천루 옥상 보스전
- 원형 판옵티콘형 스테이지
- 건물 위에서는 머리나 상체 일부만 보일 정도의 압도적 크기

단순한 CCTV 머리 로봇처럼 보이지 않도록 합니다.

- 분산된 눈
- 신호 구조
- 정보 왜곡
- 건축물과 연결된 규모
- 도시 전체를 감시하는 인상

별도 보스 명세가 나오기 전까지는 콘셉트 방향입니다.

---

# 4. 아트 방향

## 4.1 전체 톤

핵심 키워드:

- 청량함
- 청춘
- 근미래
- 서브컬처
- 스트리트 테크웨어
- 셀 채색
- 투명감
- 그래픽 대비
- 꿈
- 별
- 원형
- 신호
- 노이즈

게임은 지나치게 무겁고 진지한 분위기보다 활기 있고 접근하기 쉬운 방향을 유지합니다.

## 4.2 캐릭터 렌더링

선호 방향:

- 애니메이션 스타일
- 읽기 쉬운 실루엣
- 과도하지 않은 디테일
- 정돈된 셀 채색
- 실사보다 단순한 재질 표현
- 얼굴과 표정이 잘 보이는 구성
- 깨끗한 선과 형태
- 캐릭터 색이 배경 조명과 자연스럽게 섞이는 표현

콘셉트 아트 제작 원칙:

- 메인 캐릭터 비중을 키웁니다.
- 주변 대비를 줄입니다.
- 포즈를 배경과 자연스럽게 연결합니다.
- 과도한 회화적 노이즈를 피합니다.
- 서브컬처 게임 이미지처럼 읽기 쉬워야 합니다.

## 4.3 의상 방향

기본 방향:

- 스트리트 테크웨어
- 제복 요소
- 현실 군복처럼 무겁지 않은 전술 포인트
- 비대칭
- 스트랩, 홀스터, 장비를 디자인 포인트로 사용
- 흰색, 파란색, 보라색, 분홍색
- 제한적으로 어두운 색 사용

## 4.4 그래픽 모티프

반복적으로 사용할 요소:

- 드림캐쳐를 연상시키는 원과 방사형 구조
- 별
- 링
- 신호선
- 투명 패널
- 절제된 글리치 또는 노이즈
- 기관형 로고 시스템

실제 드림캐쳐 장식을 모든 곳에 붙이지 않습니다.  
상징은 간접적으로 사용합니다.

## 4.5 브랜드 방향

DreamCatcher 로고와 브랜딩은 다음을 지원해야 합니다.

- 공식 기관 또는 종합 기업 같은 신뢰감
- 깨끗한 타이포그래피
- 조직 및 부대 아이덴티티
- 청량한 청춘 분위기
- 공식성과 서브컬처 감성의 균형

굿즈와 기획서 페이지도 지나치게 어둡고 무겁게 만들지 않습니다.

---

# 5. 캐릭터 방향

캐릭터 이름, 최종 배경, MBTI는 별도 문서가 없으면 확정 상태가 아닙니다.

## 5.1 메인 캐릭터 A

성격 방향:

- 밝다
- 쾌활하다
- 에너지가 넘친다
- 행동이 빠르다
- 쉽게 주눅 들지 않는다
- 팀 분위기를 자연스럽게 끌어올린다
- 낙천적이다
- 팀의 추진력과 분위기 메이커 역할

밝은 성격이라는 이유로 무능하거나 단순한 캐릭터로 만들지 않습니다.

## 5.2 캐릭터 B

성격 방향:

- 겉으로는 시크하고 절제되어 있다
- 실제로는 상냥하고 배려심이 있다
- 감정 표현이 크지 않다
- 믿을 수 있다
- 날카로운 인상과 부드러운 행동의 대비가 매력

공식 설명에서는 “갭모에”라는 단어를 직접 쓰지 않습니다.

## 5.3 캐릭터 C

성격 방향:

- 나른하다
- 느긋하다
- 귀찮아하는 말투를 사용한다
- 의욕이 없어 보인다
- 실제 능력은 월등하다
- 필요할 때 매우 신뢰할 수 있다

단순히 게으른 개그 캐릭터로 소비하지 않습니다.

## 5.4 높으신 분 / 리더 캐릭터

일반 현장 팀원이 아니라 조직 내 높은 위치의 인물입니다.

성격 및 디자인 방향:

- 높은 직위
- 조용하고 신비로운 분위기
- 표정 변화가 적다
- 존댓말 사용
- 팀 전체를 깊이 이해한다
- 조직의 방향을 잡는다
- 성숙하고 안정된 존재감
- 매우 긴 생머리
- 정복 또는 의전복
- 흰색 베이스
- 보라·분홍 포인트
- 드림캐쳐를 은유한 머리 장식 또는 액세서리

기본적으로 악역처럼 보이게 만들지 않습니다.

---

# 6. 게임 모드와 화면 구조

## 6.1 스토리 / 기본 전투 모드

포함 요소:

- 전투 HUD
- 대화 및 스토리 UI
- 적 인카운터
- 스테이지 진행
- 보스전
- 결과 전환

## 6.2 프리 모드

기존 게임 화면을 재사용하되 전투 압박 요소를 줄입니다.

예상 차이:

- 적 표시 제거
- 보스 UI 제거
- 인카운터 진행 압박 제거
- 전투 전용 안내 최소화
- 이동과 배경 감상에 필요한 플레이어 상태 UI 유지

## 6.3 무한 모드

기본 전투 HUD에 다음 요소를 추가합니다.

- 현재 페이즈 또는 웨이브
- 점수
- 진행 난이도 상승
- 명확한 실패 및 결과 상태

별도의 완전히 다른 UI 언어를 만들지 않고 기존 전투 HUD를 확장합니다.

## 6.4 설정 UI

프로토타입 범위는 사운드 설정 중심입니다.

- 전체 음량
- 배경음
- 효과음
- 음성이 있을 경우 음성 볼륨

명확한 필요가 없으면 설정 메뉴 범위를 크게 늘리지 않습니다.

## 6.5 공통 UI

공통 기능:

- 확인
- 취소 / 뒤로
- 일시정지
- 알림
- 로딩
- 화면 전환
- 버튼 안내
- 모달 패널

모든 공통 UI는 하나의 그래픽 규칙을 공유해야 합니다.

---

# 7. UI / UX 방향

## 7.1 기준 해상도

UI 디자인 기준:

- **1920 × 1080**
- 16:9

절대 좌표만 사용하지 않고 Safe Margin과 해상도 스케일링을 고려합니다.

## 7.2 전투 HUD 기능 그룹

### 플레이어 상태

- 체력
- 궁극기 게이지
- 필요 시 상태 이상
- 조준 상태
- 크로스헤어

### 무기 상태

- 주무기를 크게 강조
- 보조무기는 낮은 우선순위
- 탄약 또는 발사 상태
- 현재 무기 정보

### 스킬

- 스킬 버튼
- 쿨다운
- 궁극기 상태
- 활성/비활성 상태 구분

### 적과 인카운터

- 적 표시
- 보스 체력
- 인카운터 또는 페이즈 정보
- 무한 모드 웨이브와 점수

### 진행 및 피드백

- 크로스헤어
- 히트마커
- 데미지 숫자
- 목표 또는 진행 알림
- 클리어 / 실패 화면 전환

## 7.3 대화 UI

대화 UI는 작은 전투 툴팁이 아니라 독립적인 전체 화면 연출로 취급합니다.

가능한 요소:

- 캐릭터 초상 또는 전신
- 화자 이름
- 대사
- 다음 / 스킵 / 자동 진행
- 배경 가독성 처리
- 게임 화면 복귀 전환

## 7.4 시각 스타일

UI 디자인 방향:

- 플랫한 그래픽 아이콘
- 반투명 패널
- 기획서 및 UI 쇼케이스에서는 밝은 배경
- 명확하고 절제된 포인트 색
- 강한 정보 위계
- 둥근 형태와 각진 형태를 의도적으로 혼합
- 꿈, 원, 별 모티프
- 불필요한 장식 픽토그램 최소화
- 타이틀과 본문 가독성 우선

## 7.5 UI 구현 원칙

UI는 상태를 표시하고 입력을 전달합니다.  
핵심 게임 규칙을 계산하지 않습니다.

UMG Blueprint 안에 넣지 말아야 할 것:

- 보상 계산
- 강화 계산
- 데미지 규칙
- 스테이지 진행 판정
- 저장 규칙

---

# 8. 현재 리포지토리 상태

## 8.1 리포지토리

- 리포지토리: `Shiny-Shine/DreamCatcher`
- 공개 리포지토리
- 기본 브랜치: `main`
- 마지막 확인 커밋: `11c8aeab7f84b306dce24729547edfc4fc448f92`
- 마지막 확인 날짜: 2026-07-22
- 마지막 확인 변경 파일: `Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap`
- 최근 작업은 Level 1 환경과 블록아웃에 집중되어 있습니다.

## 8.2 엔진 상태

현재 GitHub `main`의 `.uproject`는 다음 값을 사용합니다.

```json
"EngineAssociation": "5.7"
```

개발자 로컬에서는 **Unreal Engine 5.8 전환 작업**이 진행 중입니다.

다음을 구분해야 합니다.

- GitHub `main`은 아직 UE 5.7 상태일 수 있습니다.
- 로컬 작업본은 이미 UE 5.8일 수 있습니다.
- 엔진 전환은 별도 브랜치와 별도 커밋으로 처리해야 합니다.
- 디자이너용 바이너리는 정확히 같은 엔진 버전과 Build ID여야 합니다.

## 8.3 UE 5.8 Target 설정

현재 확인된 Target 파일은 다음 설정입니다.

```csharp
DefaultBuildSettings = BuildSettingsVersion.V6;
IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
```

UE 5.8로 전환할 때 두 파일을 수정합니다.

- `Source/DreamCatcher.Target.cs`
- `Source/DreamCatcherEditor.Target.cs`

권장 값:

```csharp
DefaultBuildSettings = BuildSettingsVersion.V7;
IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
```

정상적인 마이그레이션 문제 해결을 위해 다음 설정을 먼저 사용하지 않습니다.

```csharp
BuildEnvironment = TargetBuildEnvironment.Unique;
bOverrideBuildEnvironment = true;
```

이 설정은 일반적인 버전 불일치를 해결하기보다 공유 빌드 환경을 분리하거나 검사를 우회합니다.

## 8.4 Windows 빌드 필수 구성

UE 5.8 C++ 개발 환경:

- Visual Studio 2022 또는 Visual Studio Build Tools
- C++ 게임 개발 구성 요소
- MSVC x64/x86 빌드 도구
- Windows SDK
- `.NET Framework 4.8 SDK`
- `.NET Framework 4.8 Targeting Pack`

UnrealBuildTool 실행에 사용되는 최신 .NET과 `SwarmInterface`가 요구하는 `.NET Framework SDK`는 다른 구성 요소입니다.

---

# 9. 현재 소스 아키텍처

## 9.1 핵심 원칙

- **규칙과 상태:** C++
- **연출과 에셋 연결:** Blueprint
- **밸런스와 콘텐츠 값:** 점진적으로 DataAsset

## 9.2 플레이어 계층

### `ADreamCatcherCharacter`

현재 역할:

- 플레이어 본체
- 입력 수신
- 이동
- 카메라
- 오버숄더 시점
- 조준점 계산
- Health / Combat 이벤트 연결
- Blueprint 연출 훅 제공

현재 동작:

- 카메라 기준 이동
- 컨트롤러 Yaw를 따르는 슈터형 회전
- Spring Arm과 Follow Camera
- 기본 공격 입력
- 회피 입력
- 궁극기 입력
- 카메라 Line Trace 조준점 계산
- 총구 소켓 위치 계산
- 사망 시 입력과 이동 비활성화

관련 없는 시스템을 계속 Character에 추가하지 않습니다.

### `UDCHealthComponent`

현재 역할:

- 최대 체력
- 현재 체력
- 데미지
- 회복
- 리셋
- 체력 변경 이벤트
- 사망 이벤트

연출과 분리된 상태를 유지합니다.

### `UDCCombatComponent`

현재 역할:

- 자동 발사 타이머
- 회피 쿨다운
- 궁극기 게이지
- 궁극기 사용 가능 여부
- 전투 액션 요청 이벤트

향후 적절한 역할:

- 조준 상태
- 전투 액션 제한
- 발사 상태 규칙
- `WeaponComponent` 도입 전까지 무기 요청 전달

이 컴포넌트에서 직접 VFX를 생성하거나 캐릭터 애니메이션을 재생하지 않습니다.

## 9.3 컨트롤러와 HUD

### `ADreamCatcherPlayerController`

현재 역할:

- 로컬 입력 매핑 컨텍스트
- 입력 모드
- HUD 생성
- 현재 Pawn과 HUD 연결

### `UDCPlayerHUDWidget`

현재 역할:

- Character 관찰
- HealthComponent 구독
- CombatComponent 구독
- 컴포넌트 이벤트를 Blueprint UI 이벤트로 전달
- Pawn 교체 또는 파괴 시 바인딩 해제

현재 이벤트 기반 구조를 유지합니다.

## 9.4 게임 규칙

### `ADreamCatcherGameMode`

현재 역할:

- 플레이어 사망 이벤트 연결
- 일정 시간 후 현재 레벨 재시작

결과, 보상, 메타 상태를 모두 GameMode에 넣지 않습니다.

## 9.5 적 계층

### `ADCEnemyCharacter`

현재 동작:

- 기본 AIController
- 플레이어 타깃 탐색
- 플레이어 쪽 이동
- 사거리 근처에서 정지
- 타깃을 바라봄
- Line Trace 공격
- 데미지 적용
- Blueprint 공격 / 피격 / 사망 이벤트
- 일정 시간 후 파괴

현재 AI는 의도적으로 프로토타입 수준입니다.

### 현재 적 공격 문제

현재 공격 함수 안에서 데미지 처리와 연출 이벤트가 거의 동시에 실행됩니다.

공격 텔레그래프를 만들기 위해 다음 단계로 분리해야 합니다.

1. 공격 의도
2. 선딜
3. 공격 확정
4. 명중 판정
5. 후딜

## 9.6 인카운터 계층

### `ADCEnemySpawner`

현재 역할:

- 적 클래스
- 총 스폰 수
- 최대 생존 수
- 스폰 간격
- 사망 추적
- 완료 이벤트

### `ADCEncounterController`

현재 역할:

- 여러 스포너 활성화
- 남은 스포너 추적
- 인카운터 완료 이벤트

### `ADCStageDirector`

현재 페이즈:

- `Delay`
- `Encounter`
- `BossEncounter`
- `Complete`

현재 역할:

- 페이즈 순차 실행
- 인카운터 이벤트 바인딩
- 완료까지 진행
- Blueprint 스테이지 및 페이즈 이벤트

향후 필요에 따라 추가 가능한 페이즈:

- `Sequence`
- `Reward`
- `Result`

수직 슬라이스에서 실제 필요가 생기기 전에는 불필요하게 확장하지 않습니다.

---

# 10. 확인된 현재 파일 구조

## 10.1 코어 소스

```text
Source/DreamCatcher/
  DreamCatcherCharacter.h
  DreamCatcherCharacter.cpp
  DreamCatcherPlayerController.h
  DreamCatcherPlayerController.cpp
  DreamCatcherGameMode.h
  DreamCatcherGameMode.cpp
  DreamCatcher.Build.cs
```

## 10.2 컴포넌트

```text
Source/DreamCatcher/Components/
  DCHealthComponent.h
  DCHealthComponent.cpp
  DCCombatComponent.h
  DCCombatComponent.cpp
```

## 10.3 AI

```text
Source/DreamCatcher/AI/
  DCEnemyCharacter.h
  DCEnemyCharacter.cpp
  DCEnemySpawner.h
  DCEnemySpawner.cpp
```

## 10.4 스테이지

```text
Source/DreamCatcher/Stage/
  DCEncounterController.h
  DCEncounterController.cpp
  DCStageDirector.h
  DCStageDirector.cpp
```

## 10.5 UI

```text
Source/DreamCatcher/UI/
  DCPlayerHUDWidget.h
  DCPlayerHUDWidget.cpp
```

## 10.6 주요 Blueprint 에셋

```text
Content/Blueprint/
  BP_DreamCatcherGameMode.uasset
  BP_DreamCatcherPlayerController.uasset
  BP_DreamCatcherChracter.uasset

Content/Blueprint/UI/
  WBP_PlayerHUD.uasset

Content/Blueprint/AI/
  BP_EnemyCharacter.uasset
  BP_EnemySpawner.uasset

Content/Blueprint/Stage/
  BP_EncounterController.uasset
  BP_StageDirector.uasset
```

확인된 오탈자:

```text
BP_DreamCatcherChracter
```

Unreal Editor에서 참조와 Redirector를 확인하지 않고 임의로 이름을 변경하지 않습니다.

## 10.7 입력 에셋

```text
Content/Input/
  IMC_Player.uasset

Content/Input/Actions/
  IA_Move.uasset
  IA_Look.uasset
  IA_Fire.uasset
  IA_Dodge.uasset
  IA_Ultimate.uasset
  IA_Aim.uasset
```

## 10.8 맵

```text
Content/DreamCatcher/DreamCatcher_ENV/Map/Main/
  Level_1.umap

Content/DreamCatcher/DreamCatcher_ENV/Map/Test_Map/
  Test_Map.umap
```

## 10.9 프로젝트 설정

```text
Config/DefaultGame.ini
Config/DefaultEngine.ini
DreamCatcher.uproject
Source/DreamCatcher.Target.cs
Source/DreamCatcherEditor.Target.cs
```

---

# 11. 현재 구현 상태

## 11.1 구현됨

- 플레이어 Character 기반
- 오버숄더 카메라
- 카메라 기준 이동
- HealthComponent
- CombatComponent
- 자동 발사 요청 타이밍
- 회피 쿨다운
- 궁극기 게이지 상태
- Enhanced Input
- PlayerController 입력 매핑
- HUD 생성
- 이벤트 기반 HUD 연결
- Hip / Shoulder / Scope 조준 상태 머신
- Hip 상태의 짧은 우클릭 / Hold 판정
- 조준 모드별 카메라 프로필, FOV, Spring Arm, 감도 보간
- `OnAimModeChanged` 기반 Character / HUD 이벤트 연결
- `IA_Aim` 및 `IMC_Player` 우클릭 입력 연결
- 플레이어 사망 및 레벨 재시작
- 기본 적 Character
- 적 이동 및 공격
- EnemySpawner
- EncounterController
- StageDirector
- Level 1 환경 및 블록아웃 작업
- Unreal 에셋 Git LFS
- 디자이너용 Win64 Editor 바이너리 동기화 구조

## 11.2 부분 구현

- 플레이어 공격 연출
- 실제 발사체 또는 명중 방식
- 플레이어 피격 피드백
- 적 공격 연출
- 적 공격 텔레그래프
- 궁극기 실제 효과
- 궁극기 게이지 획득 방식
- HUD 시각 완성도
- 조준 AnimBP, Aim Offset, 최종 Scope 오버레이 연출
- 실제 레벨 인카운터 배치
- 전체 스테이지 진행
- 컷신 연결

## 11.3 미구현 또는 확인되지 않음

- 전용 BossCharacter
- 보스 페이즈 컴포넌트
- 보스 패턴 시스템
- 보스 HUD
- 결과 UI 및 결과 상태
- 보상 시스템
- 강화 시스템
- 인벤토리
- SaveGame
- GameInstance 또는 MetaSubsystem
- DataAsset 기반 밸런스
- 스테이지 선택
- 미니맵
- 정식 WeaponComponent
- WeaponData
- HitReactionComponent
- 회피 무적 프레임
- 최종 데미지 숫자 디자인
- 완성된 대화 흐름
- 프리 모드
- 무한 모드

---

# 12. 확정된 조준 시스템 명세

이 항목은 현재 가장 최근에 확정된 우클릭 조작 규칙입니다.

## 12.1 입력 방식

우클릭 하나를 사용합니다.

### 짧게 클릭

- 기준 시간 전에 눌렀다 뗍니다.
- Scope 조준을 켜거나 끕니다.

### 길게 누르기

- 기준 시간을 넘길 때까지 유지합니다.
- Shoulder Aim으로 진입합니다.
- 버튼을 누르고 있는 동안 유지합니다.
- 버튼을 떼면 Shoulder Aim을 종료합니다.

### 초기 기준 시간

```text
0.18초
```

테스트 범위:

```text
0.16~0.22초
```

## 12.2 상태 저장 원칙

두 개의 의도 상태를 별도로 저장합니다.

```cpp
bool bScopeToggled;
bool bShoulderHeld;
```

`bScopeToggled`와 `bShoulderHeld`는 동시에 참이 되지 않습니다.

- Scope는 Hip 상태의 짧은 클릭으로만 진입합니다.
- Shoulder는 Hip 상태의 Hold로만 진입합니다.
- Scope 상태에서는 Hold 판정을 시작하지 않습니다.
- Shoulder 해제 후 이전 Scope 상태를 복구하지 않습니다.

권장 enum:

```cpp
UENUM(BlueprintType)
enum class EDCAimMode : uint8
{
    Hip,
    Shoulder,
    Scope
};
```

## 12.3 상태 전이

```text
Hip
  짧은 클릭 -> Scope
  길게 누름 -> Shoulder
  길게 누른 뒤 해제 -> Hip

Scope
  우클릭 누름 -> Hip

Shoulder
  우클릭 유지 -> Shoulder
  우클릭 해제 -> Hip
```

짧은 클릭과 Hold 구분은 Hip 상태에서만 실행합니다.  
Scope에서 Shoulder로 직접 전환하는 상태 전이는 허용하지 않습니다.

## 12.4 강제 해제 조건

다음 행동은 모든 조준 상태를 해제합니다.

- 회피
- 궁극기
- 사망
- Possession 해제
- 입력 취소
- 향후 전투 입력을 막는 컷신 상태

별도 기능 명세에서 정책을 바꿀 수 있지만 의도적으로 결정해야 합니다.

## 12.5 책임 분리

### Character

- `IA_Aim` 입력 수신
- Hold Timer 시작 및 정리
- 짧은 클릭과 길게 누르기 판정
- 목표 카메라 프로필 관리
- 조준 감도 배율 적용

### CombatComponent

- 조준 의도와 최종 조준 상태 저장
- 조준 가능 여부 판정
- `OnAimModeChanged` 이벤트
- 현재 조준 모드 제공
- 회피, 궁극기 등 충돌 액션에서 조준 해제
- 향후 조준 모드별 탄 퍼짐 제공

### Blueprint / HUD / AnimBP

- 카메라 연출
- 견착 애니메이션
- Aim Offset
- 스코프 오버레이
- 크로스헤어 변화
- 사운드
- 화면 전환 효과

## 12.6 초기 카메라 값

초기 테스트값이며 최종 밸런스가 아닙니다.

| 상태 | FOV | Spring Arm | Socket Y | 감도 |
|---|---:|---:|---:|---:|
| Hip | 90 | 325 | 55 | 1.0 |
| Shoulder | 72 | 235 | 80 | 0.7 |
| Scope | 40 | 210 | 70 | 0.35 |

## 12.7 Scope 구현 방식

첫 수직 슬라이스에서는 다음 방식으로 구현합니다.

- 기존 플레이어 카메라 사용
- FOV 축소
- 감도 감소
- 스코프 마스크 / 오버레이
- 크로스헤어 전환

첫 구현부터 SceneCapture2D 방식 스코프를 만들지 않습니다.

이유:

- 정식 무기 시스템이 아직 없습니다.
- 렌더링 설정이 이미 무겁습니다.
- 첫 프로토타입에서 불필요한 성능 비용과 복잡도가 발생합니다.

## 12.8 현재 구현 범위

2026-07-31 기준 조준 시스템 1차 기능 프로토타입이 구현되었습니다.

구현된 C++ 파일:

```text
Source/DreamCatcher/DreamCatcherCharacter.h
Source/DreamCatcher/DreamCatcherCharacter.cpp
Source/DreamCatcher/Components/DCCombatComponent.h
Source/DreamCatcher/Components/DCCombatComponent.cpp
Source/DreamCatcher/UI/DCPlayerHUDWidget.h
Source/DreamCatcher/UI/DCPlayerHUDWidget.cpp
```

반영된 Unreal Editor 에셋:

```text
Content/Input/Actions/IA_Aim.uasset
Content/Input/IMC_Player.uasset
Content/Blueprint/BP_DreamCatcherChracter.uasset
Content/Blueprint/UI/WBP_PlayerHUD.uasset
```

후속 연출 작업:

- Animation Blueprint 견착 자세
- Aim Offset
- 최종 Scope 오버레이 아트
- 조준 전환 VFX / SFX
- 조준 모드별 탄 퍼짐과 이동 속도

---

# 13. 향후 권장 아키텍처

## 13.1 전투 계층

현재:

- `ADreamCatcherCharacter`
- `UDCHealthComponent`
- `UDCCombatComponent`

향후:

- `UDCWeaponComponent`
- `UDCHitReactionComponent`
- `IDCDamageable`

원칙:

- 상태, 쿨다운, 판정은 C++
- 애니메이션, 카메라 쉐이크, VFX, SFX는 Blueprint
- 반복 조정이 시작된 수치는 DataAsset

## 13.2 무기 계층

향후 `UDCWeaponComponent` 역할:

- 현재 장착 무기 데이터
- 발사 방식
- 발사 간격
- 탄 퍼짐
- 반동
- Projectile / Hitscan 선택
- 탄약과 재장전
- 무기 교체 상태

Character에 남길 역할:

- 입력
- 카메라 기준
- 조준 원점과 방향
- 무기 컴포넌트로 요청 전달

향후 `UDCWeaponData` 후보 값:

- 발사 모드
- 데미지
- 발사 간격
- Hip 탄 퍼짐
- Shoulder 탄 퍼짐
- Scope 탄 퍼짐
- Shoulder FOV
- Scope FOV
- 조준 중 이동 속도
- 조준 감도
- 전환 속도
- 스코프 UI 클래스

## 13.3 보스 계층

권장 구조:

- `ADCBossCharacter`
- `UDCBossPhaseComponent`
- `UDCBossData`
- `WBP_BossHUD`

첫 보스 최소 요구:

- 전용 체력 UI
- 구분되는 공격 패턴 2개
- 명확한 텔레그래프
- 체력 기준 페이즈 전환 1회
- 사망 시 스테이지 완료 신호

일반 적의 체력과 데미지만 높인 형태로 만들지 않습니다.

## 13.4 스테이지 계층

- GameMode: 레벨 규칙
- StageDirector: 순서
- EncounterController: 한 전투 구간
- EnemySpawner: 적 생성
- Sequence: 컷신
- Result: 플레이 종료

## 13.5 메타 계층

향후:

- `UDreamCatcherGameInstance`
- `UDCSaveGame`
- `UDCMetaSubsystem`
- Reward Service
- Upgrade Service
- StageRunResult 구조체

영구 상태는 GameMode가 아니라 이 계층에서 관리합니다.

## 13.6 데이터 계층

DataAsset 적용 순서:

1. 적 데이터
2. 무기 데이터
3. 플레이어 데이터
4. 보스 데이터
5. 스테이지 데이터
6. 보상 및 강화 데이터

실제 반복 수정이 시작된 데이터부터 옮깁니다.  
수직 슬라이스가 플레이 가능하기 전에 대규모 DataAsset 전환을 하지 않습니다.

---

# 14. 개발 로드맵

## 마일스톤 1 — 전투 프로토타입

목표:

- 읽기 쉽고 반응성 좋은 사격
- 의미 있는 회피
- 작동하는 궁극기 루프
- 기본 적 전투
- 기능적인 HUD

남은 핵심 작업:

- 총구 화염
- 트레이서
- 피격 이펙트
- 피격 사운드
- 히트마커
- 카메라 반응
- 적 선딜
- 회피 무적
- 궁극기 게이지 획득
- 조준 AnimBP 및 최종 연출

## 마일스톤 2 — 수직 슬라이스

목표:

- 인트로
- 전투 1
- 전투 2
- 전환
- 보스
- 결과

완료 기준:

> 개발자 개입 없이 게임 시작부터 결과 화면까지 도달할 수 있다.

## 마일스톤 3 — 보스전

목표:

- 전용 보스
- 페이즈 전환
- 패턴 시스템
- 보스 HUD
- 읽기 쉬운 텔레그래프
- 보스룸 연출

## 마일스톤 4 — 데이터 기반

목표:

- 자주 수정하는 수치를 DataAsset로 이동
- 디자이너가 C++ 수정 없이 밸런스 조정
- 기존 기본값 안전하게 이전

## 마일스톤 5 — 메타 루프

목표:

- 결과
- 보상
- 강화
- 저장
- 다음 판 반영

## 마일스톤 6 — 제작 기반 및 폴리싱

목표:

- 샘플 의존 축소
- 콘텐츠 구조 정리
- 아트 교체 가능한 구조
- 최종 UI 언어
- VFX / SFX 리듬
- 캐릭터 매력
- 성능 검증

---

# 15. 현재 우선순위

실행 계획·세부 작업·완료 기준의 단일 기준은 `docs/specs/gas-lyra-migration.md`의 R0~R14입니다.

1. R0: 정책·기존 구현 이력·다음 작업 기록
2. R1: 원본 GAS 보조 타입 및 의존성 준비
3. R2: 원본 GameplayAbility·ASC·AbilitySet·Pawn 초기화·입력 공통 기반 교체
4. R3: 원본 체력·데미지·사망 경로
5. R4: 원본 카메라와 확정 우클릭 규칙 연결
6. R5: 원본 Inventory·QuickBar·Equipment·라이플 데이터
7. R6: 원본 무기 퍼짐·발사·GameplayCue·카메라 반동
8. R7: 원본 Reticle·HUD·명중 표시 재개
9. R8: 전체 원본 AnimBP·Linked Layer·재장전
10. R9: Lyra Dash 원본, 회피 방어 확장, 궁극기
11. R10: 적 GAS·공격 텔레그래프
12. R11: 전투 1 → 전투 2 → 보스 Placeholder → 결과 Placeholder 완주
13. R12: 최소 보스
14. R13: 필요한 Experience·추가 무기·공통 기능 후속 이식
15. R14: Legacy·참조·회귀 최종 정리

다음 구현 단위는 R1-1의 Ability Tag Relationship Mapping 원본 두 파일 이식입니다.
직전 안내만으로 구현 완료를 추정하지 말고 실제 파일·빌드 상태부터 확인합니다.
원본 C++ 상호 의존성 때문에 Hero/Camera/Health 등의 일부를 앞당길 수 있습니다. 이를 이유로 원본 기능을 임의 삭제하지 않습니다.
기능별 대체 검증 후 옛 경로를 순차 정리하며, R14는 잔여 참조를 종합 점검하는 단계입니다.

수직 슬라이스 전에는 다음을 우선하지 않습니다.

- 미니맵
- 대형 인벤토리 및 복잡한 아이템 경제
- 멀티플레이 완성
- Lyra 또는 ShooterCore 전체 복제
- 복잡한 AI 프레임워크

---

# 16. 기술 부채와 위험

## 16.1 Variant 샘플 코드

`DreamCatcher.Build.cs`에 다음 관련 경로와 의존성이 남아 있습니다.

- `Variant_Combat`
- `Variant_Platforming`
- `Variant_SideScrolling`
- `StateTreeModule`
- `GameplayStateTreeModule`

영향:

- 빌드 범위 증가
- 실제 프로젝트 구조와 샘플 구조 혼동
- 유지보수 어려움

정책:

- 실제 참조 여부를 확인하기 전에는 무작정 삭제하지 않습니다.
- 새 DreamCatcher 시스템을 Variant 구조 위에 만들지 않습니다.
- 프로젝트 구조가 안정된 뒤 별도 작업으로 제거합니다.

## 16.2 Content 루트 불일치

현재 프로젝트 콘텐츠가 여러 루트에 나뉘어 있습니다.

```text
Content/Blueprint/
Content/DreamCatcher/
Content/Input/
```

장기 목표:

```text
Content/DreamCatcher/
  Blueprints/
  Data/
  Input/
  Maps/
  Animations/
  FX/
  Audio/
  Sequences/
  Placeholder/
```

개발 중에는 대규모 에셋 이동을 하지 않습니다.

## 16.3 Blueprint 이름 오탈자

```text
BP_DreamCatcherChracter
```

Unreal Editor에서만 이름을 수정하고 Redirector와 참조를 검증합니다.

## 16.4 렌더링 비용

현재 프로젝트 설정에 포함된 고비용 기능:

- Lumen Hardware Ray Tracing
- Ray Tracing
- Path Tracing
- Nanite
- Substrate

가능한 영향:

- 셰이더 컴파일 증가
- 반복 작업 속도 저하
- 팀원 PC 성능 차이
- SceneCapture 기반 기능 추가 비용

작업 속도가 지나치게 느려지면 개발용 렌더링 프로파일을 고려합니다.

## 16.5 회피 제한

현재 회피는 `LaunchCharacter`와 쿨다운 중심입니다.

확인되지 않은 것:

- 무적 프레임
- 데미지 무시
- 성공 피드백
- 애니메이션 기반 이동

의미 있는 회피를 위해 실제 방어 효과가 필요합니다.

## 16.6 적 텔레그래프 제한

현재 적 공격은 공격 함수에서 바로 Line Trace와 데미지를 처리합니다.

목표 구조:

```text
공격 의도 -> 선딜 -> 확정 -> 명중 -> 후딜
```

## 16.7 `.gitignore` 위험

확인된 `.gitignore`에는 다음 규칙이 있습니다.

```text
/Content/Characters/
```

이미 추적된 파일은 유지되지만 새 캐릭터 에셋이 Git에 나타나지 않을 수 있습니다.

생산용 캐릭터 에셋을 해당 경로에 추가하기 전 규칙을 검토합니다.

## 16.8 IDE 메타데이터

`.idea` 개인 작업 파일이 리포지토리에 포함된 이력이 있습니다.

별도 정리 작업에서:

- 적절한 ignore 규칙 추가
- 이미 추적된 개인 workspace 파일 제거

## 16.9 main 직접 커밋

현재 PR과 Issue 기반 작업 흐름이 약합니다.

위험:

- 바이너리 충돌
- 리뷰 부족
- 엔진 전환과 기능 변경 혼합
- 회귀 확인 어려움

가벼운 브랜치와 PR 흐름을 도입합니다.

---

# 17. Git 및 협업 규칙

## 17.1 브랜치 규칙

Codex 작업을 `main`에 직접 커밋하지 않습니다.

예시:

```text
feature/aim-system
feature/enemy-telegraph
feature/boss-foundation
fix/ue58-target-settings
docs/project-context
```

한 브랜치에는 하나의 기능 또는 하나의 마이그레이션만 포함합니다.

## 17.2 커밋 메시지

의도를 설명하는 메시지를 사용합니다.

권장:

```text
feat: add aim input state machine
fix: update target settings for UE 5.8
refactor: separate enemy attack telegraph from damage
docs: add DreamCatcher project context
```

지양:

```text
작업 1
Level 1 1-2
수정
```

맵 작업은 변경 구역을 명시합니다.

```text
level: revise Level 1 encounter-2 rooftop route
```

## 17.3 바이너리 에셋 담당자

`.umap`, `.uasset`은 일반적인 텍스트 병합이 어렵습니다.

작업 전 한 명이 다음 에셋의 임시 소유권을 가집니다.

- 맵
- 주요 Widget Blueprint
- Character Blueprint
- Animation Blueprint

두 사람이 같은 바이너리 에셋을 동시에 수정하지 않습니다.

## 17.4 Git LFS

현재 LFS 대상:

- `.uasset`
- `.umap`
- `.ubulk`
- `.uexp`
- `.utoc`
- `.ucas`
- `.fbx`
- `.blend`
- `.psd`
- `.exr`
- `.wav`
- `.mp3`
- `.mp4`

LFS 규칙을 임의로 제거하지 않습니다.

## 17.5 디자이너용 바이너리 동기화

디자이너가 Visual Studio 전체를 설치하지 않고 C++ 클래스를 사용할 수 있도록 Win64 Editor 바이너리를 동기화하는 방향을 사용합니다.

대상:

```text
Binaries/Win64/
Plugins/**/Binaries/Win64/
```

주의:

- 개발자와 디자이너가 같은 엔진 버전을 사용해야 합니다.
- `UnrealEditor-DreamCatcher.dll`과 `UnrealEditor.modules`가 같은 빌드여야 합니다.
- C++ 변경 후 개발자는 다시 빌드하고 일치하는 바이너리를 커밋해야 합니다.
- UE 5.7 바이너리와 UE 5.8 프로젝트를 섞지 않습니다.

---

# 18. Codex 작업 규칙

## 18.1 작업 전 확인

Codex는 작업 전에 다음을 수행합니다.

1. 작업을 시작하기 전에 이 파일과 `docs/specs/gas-lyra-migration.md`를 먼저 읽습니다.
2. 현재 브랜치를 확인합니다.
3. 관련 파일을 확인합니다.
4. 현재 프로젝트가 UE 5.7인지 UE 5.8인지 확인합니다.
5. Unreal 내부 API로 자동화할 에셋 작업과 사용자 수동 확인이 필요한 작업을 구분합니다.
6. 코드·Config·에셋·자동화 스크립트의 변경 범위와 승인 여부를 명시합니다. C++ 빌드·패키징은 사용자 담당입니다.

## 18.2 아키텍처 원칙

선호:

- 책임이 명확한 작은 컴포넌트
- 이벤트 기반 UI
- Blueprint 연출 훅
- 조정 가능한 수치
- 되돌리기 쉬운 변경
- 명확한 상태 전이
- 기존 이름 규칙 유지

현재 승인된 아키텍처 방향:

- Gameplay Ability System 도입
- Lyra Starter Game의 GAS, AbilitySet, PawnData 등 원본 코드·에셋 재사용 우선
- PlayerState가 플레이어 ASC 소유
- Gameplay Tag 기반 입력과 상태 관리
- AttributeSet 기반 체력, 데미지, 자원 관리
- Equipment와 WeaponInstance 기반 무기 구조
- GameplayCue 기반 VFX, SFX, Camera Shake 연출
- 기존 시스템과 병행 구현 후 검증된 기능부터 단계적으로 교체
- 사용 가능한 Lyra Blueprint·데이터·시각 에셋은 의존성과 함께 최대한 Migrate
- Lyra C++도 필요한 범위의 원본 이식을 우선하고, 검증된 계산·표시·상태 처리 로직을 최대한 보존
- 부모 클래스·모듈·플러그인·Reflection 타입·메시지·패키지 참조를 확인하고, 프로젝트 연결부만 최소 수정
- 재구현이 필요하면 원본 이식이 부적합한 이유와 동작 차이를 먼저 설명하고 해당 부분만 직접 구현
- 크로스헤어는 원본 머티리얼·Widget Blueprint·UMG 애니메이션·UI 전용 C++를 함께 이식하는 대상
- 기존 1~6단계의 자체 구현도 원본 재사용이 가능하면 교체를 기본 방향으로 하되, 이식한 대체 기능이 검증되기 전에는 삭제하지 않음
- 기존 자체 API에 맞추려고 원본 OnSpawn·Inventory 연결·퍼짐 데이터 계약을 축소하지 않음
- 실제 필요한 최소 Inventory·QuickBar·공통 모듈은 원본 무기/UI의 선행 의존성으로 이식
- 변경안과 원본 근거 설명은 유지하되, 승인된 코드·에셋 작업 묶음을 Codex가 처리·자동 검사하여 사용자 수동 작업을 줄임. C++ 빌드·패키징·최종 플레이 확인은 사용자가 담당
- 원본 Ability Blueprint에 있는 발사·재장전·Dash 흐름을 역할 분리 명목으로 C++ 재구현하지 않고 원본 책임 분할을 보존

원본 확인과 보고 원칙:

- 실제 Lyra 소스·에셋·그래프를 확인하고, 기억이나 추정으로 원본 동작을 단정하지 않습니다.
- C++ 기본값, Blueprint 설정값, 런타임 값을 구분합니다. 곡선의 키·보간·단위를 임의로 추정하지 않습니다.
- 에셋의 이름·참조 문자열 확인은 그래프 연결이나 런타임 검증과 구분합니다.
- 원본 유지 부분, 최소 수정 부분, 의도적인 게임 규칙 차이, 미검증 항목을 명시합니다.
- 원본 코드·에셋의 저작권 및 라이선스 표기를 유지하고 적용되는 배포 조건을 확인합니다.
- 이식에 필요한 엔진·플러그인 모듈의 사용과 무관한 프레임워크 전체 복사를 구분합니다.
- 정책 변경이나 에셋 복사만으로 기능을 완료 처리하지 않으며, 임의의 단계 번호를 기존 확정 계획처럼 제시하지 않습니다.
- 이번 승인된 R0~R14와 과거 단계 번호를 구분하고, 한 채팅의 종료 시 원본/대상·변경 이유·빌드/Editor/PIE 결과·미검증 항목·다음 작업 ID를 인계합니다.
- 원본 그대로 이식하지 못하는 모든 항목에 기능 명세의 `원본 그대로 이식하지 못하는 경우의 근거 설명 의무`를 적용합니다. 대상 경로·함수/그래프, 확인 근거, 그대로 이식할 때의 영향, 제약 종류, 원본 보존 대안, 변경 범위·동작 차이·검증 기준을 먼저 설명하고 인계합니다.
- “Lyra 전용”, “복잡함”, “의존성이 많음”, “현재 구현과 다름”만으로 재구현·제외하지 않습니다. 선행 의존성 미준비와 기술적 불가능, 범위상 보류, 미검증을 구분합니다.
- 소스 분석상의 예상과 실제 빌드/Editor/PIE 재현을 구분합니다. 관련 공식 문서가 있으면 출처를 제시하고, 확인하지 못한 원본은 필요한 확인 항목을 밝힙니다.
- 요구 동작이나 승인 범위를 바꾸는 대안은 사용자 확인 전 확정하지 않습니다. 보류·제외 항목에는 결정 근거와 재개 조건을 남깁니다.

명시적 요청 없이는 피할 것:

- 멀티플레이 복제 아키텍처 확장
- 현재 전투 범위를 벗어난 대형 Ability / Attribute 확장
- Behavior Tree 중심 구조 전환
- 과도한 Subsystem 추가
- 전체 폴더 일괄 재구성
- 대량 에셋 이름 변경
- Variant 샘플 구조로 교체
- Lyra 또는 ShooterCore 전체 소스와 플러그인 복제

## 18.3 C++ 규칙

- Unreal 이름 규칙을 따릅니다.
- UnrealHeaderTool 호환 선언을 유지합니다.
- 헤더에서는 가능한 경우 Forward Declaration을 사용합니다.
- 필요한 헤더는 명시적으로 Include합니다.
- 우연한 Include 순서에 의존하지 않습니다.
- 반환형이 있는 함수의 모든 경로를 확인합니다.
- Timer와 Dynamic Delegate를 정리합니다.
- UObject 수명에 적절한 `UPROPERTY`, `TObjectPtr`, Weak Pointer를 사용합니다.
- 디자이너가 필요한 항목만 Blueprint에 노출합니다.
- 연출 전용 로직을 저수준 컴포넌트에 넣지 않습니다.
- Timer 또는 Event로 해결 가능한 경우 Tick을 추가하지 않습니다.
- Tick이 필요하면 최소한의 작업만 수행합니다.

## 18.4 Blueprint 전달 규칙

승인된 Unreal 내부 자동화를 우선 검토하고, 수동 Editor 작업이 필요한 부분만 사용자에게 전달합니다. 자동화와 수동 작업 모두 다음을 보고합니다.

- 정확한 에셋 경로
- 부모 클래스
- 할당할 프로퍼티
- 구현할 이벤트
- 노드 연결 순서
- 예상 실행 결과
- 검증 방법
- Redirector 또는 참조 위험
- 자동화 실행·저장 범위, 재로드 결과와 사용자에게 남은 확인 항목

## 18.5 작업 범위 통제

관련 없는 파일을 수정하지 않습니다.

특히 다음을 포함하지 않습니다.

- `.idea`
- `Saved`
- `Intermediate`
- 관련 없는 `.uasset`
- 관련 없는 `.umap`
- 불필요하게 재생성된 대량 에셋
- 기능 작업과 무관한 엔진 전환 변경

## 18.6 작업 완료 보고서

모든 Codex 작업 완료 보고에는 다음을 포함합니다.

1. 요약
2. 변경 파일
3. 구현된 동작
4. 사용한 가정
5. 빌드 결과
6. 테스트 내용
7. Unreal Editor 수동 작업
8. 알려진 제한
9. 다음 권장 작업

---

# 19. 빌드 및 검증

아래 C++ 빌드·패키징 및 관련 실행 절차는 사용자 담당입니다. Codex는 승인된 코드 수정과 Unreal 내부 에셋 자동 검사를 수행하고, 사용자 빌드·패키징 결과를 읽어 확인합니다. 이 문서의 예시 명령은 Codex의 직접 빌드·패키징 실행 승인이 아닙니다.

## 19.1 엔진 전환 후 클린 빌드

Unreal Editor와 IDE를 종료한 뒤 생성 폴더를 삭제합니다.

삭제 가능:

```text
Binaries
Intermediate
.vs
```

`Saved`는 로컬 상태를 초기화할 때만 선택적으로 삭제합니다.

삭제 금지:

```text
Content
Config
Source
Plugins
DreamCatcher.uproject
```

## 19.2 UE 5.8 Editor 빌드 예시

로컬 경로에 맞게 수정합니다.

```bat
"<UE_5.8>\Engine\Build\BatchFiles\Build.bat" ^
DreamCatcherEditor Win64 Development ^
-Project="<Project>\DreamCatcher.uproject" ^
-WaitMutex -NoHotReloadFromIDE
```

## 19.3 검증 단계

### 1단계 — 정적 확인

- 선언 구조 확인
- Include 확인
- Reflection Macro 확인
- Delegate Signature 확인
- Timer와 소유권 확인

### 2단계 — C++ 빌드

- `DreamCatcherEditor Win64 Development` 성공
- 새 Warning-as-Error 없음
- Generated Code 성공

### 3단계 — Editor 확인

- 프로젝트 실행
- C++ 부모 Blueprint 로드
- Missing Class 경고 없음
- Blueprint Compile Error 없음
- 필요한 프로퍼티 할당 가능

### 4단계 — 게임플레이 확인

- 대상 맵 실행
- 기능 동작이 요구사항과 일치
- 상태가 올바르게 초기화
- 입력 잠김 없음
- Delegate 또는 Timer 잔여 없음
- 재시작과 레벨 전환 정상

## 19.4 바이너리 에셋 한계

C++ 빌드 성공만으로 다음을 보장하지 않습니다.

- Blueprint 그래프 연결
- Input Action 할당
- Widget 레이아웃
- 애니메이션 연결
- 레벨 Actor 배치

항상 Editor 검증을 별도로 작성합니다.

---

# 20. 향후 폴더 구조

당장 재구성하지 않고 장기 목표로 사용합니다.

## 20.1 Source

```text
Source/DreamCatcher/
  Core/
  Player/
  Components/
  AI/
  Stage/
  UI/
  Meta/
  Data/
```

## 20.2 Content

```text
Content/DreamCatcher/
  Blueprints/
    Core/
    Player/
    AI/
    Stage/
    UI/
  Data/
    Characters/
    Weapons/
    Enemies/
    Bosses/
    Stages/
    Rewards/
    Upgrades/
  Input/
  Maps/
    Prototype/
    Combat/
    Boss/
  Animations/
  FX/
  Audio/
  Sequences/
  Placeholder/
```

## 20.3 에셋 이동 조건

다음 조건에서만 에셋을 이동합니다.

- 수직 슬라이스가 안정됨
- 참조 구조를 이해함
- 두 팀원이 동의함
- Unreal Editor에서 이동함
- Redirector 정리
- 게임플레이 변경과 분리된 커밋

---

# 21. 작업 루틴

## 21.1 개발자 일일 루틴

1. 오늘 만들 기능 하나를 정합니다.
2. 기능의 규칙과 상태를 정의합니다.
3. C++를 구현합니다.
4. 빌드합니다.
5. Blueprint 훅을 엽니다.
6. Test Map에서 검증합니다.
7. 프로젝트를 항상 플레이 가능한 상태로 유지합니다.
8. 다음 시스템 전에 막는 버그를 해결합니다.
9. 하나의 의도로 커밋합니다.

## 21.2 디자이너 일일 루틴

1. 오늘 작업할 Blueprint 또는 레벨 구역을 정합니다.
2. 메시, 애니메이션, VFX, SFX, UI를 연결합니다.
3. 아름다움보다 가독성을 먼저 봅니다.
4. 적 공격 의도가 보이는지 확인합니다.
5. 피격 결과가 이해되는지 확인합니다.
6. 피드백을 다음으로 구분해 기록합니다.
   - 버그
   - 가독성 문제
   - 손맛 문제
   - 시각 폴리싱

## 21.3 주간 루틴

- 최소 한 번 전체 플레이 빌드 확인
- 현재 가능한 흐름 전체 플레이
- 구현 상태 문서 갱신
- 버그와 재미 문제 분리
- 바이너리 에셋 소유권 확인
- 다음 주 핵심 위험 하나 선택
- 큰 시스템 여러 개 동시 시작 금지

---

# 22. 기능 완료 기준

함수가 존재한다고 기능이 완성된 것이 아닙니다.

## 22.1 사격

완료 기준:

- 입력 반응이 빠름
- 발사가 보임
- 명중 지점이 읽힘
- 명중과 빗나감 구분
- 사운드가 타이밍을 보조
- 카메라 반응이 과하지 않음
- 수치 조정 가능
- 사망 또는 상태 중단 시 발사 종료

## 22.2 회피

원본 Dash 이식 완료와 DreamCatcher 방어 확장 완료를 구분합니다. 기존 무입력 전방/8방향 자체 규칙은 재도입하지 않습니다.

최종 회피 기능의 완료 기준:

- 시작과 끝이 보임
- 실제 방어 효과가 있음
- 쿨다운 이해 가능
- 이동 제어 가능
- 충돌 문제 확인
- 피격 중 상호작용 확인

위 방어 효과·성공 피드백이 Lyra Dash 원본에 포함되어 있다고 가정하지 않습니다.
명세의 R9-1에서 원본 이동을 검증한 뒤 R9-2에서 별도로 확인·연결합니다.

## 22.3 적 공격

완료 기준:

- 데미지 전에 공격 의도가 보임
- 대응 시간이 공정함
- 피격 방향 이해 가능
- 장애물이 공격을 막음
- 후딜이 있어 난사처럼 보이지 않음

## 22.4 조준 시스템

완료 기준:

- 짧은 클릭이 Scope를 안정적으로 토글
- Hip에서 길게 누르면 Shoulder Aim 시작
- Scope에서 우클릭을 누르면 즉시 Hip으로 복귀
- Shoulder에서 우클릭을 떼면 항상 Hip으로 복귀
- Scope에서 Shoulder로 직접 전환되지 않음
- 클릭과 Hold가 중복 실행되지 않음
- 강제 취소 시 상태 초기화
- 카메라 보간 안정
- 감도 정상 전환
- HUD와 애니메이션이 실제 상태 반영

## 22.5 인카운터

완료 기준:

- 스포너가 한 번만 시작
- 모든 적 사망 집계
- 완료 이벤트 한 번만 발생
- 잘못된 스포너 또는 클래스 때문에 진행 막힘 없음
- StageDirector 진행
- 재시작 후 이전 상태가 남지 않음

## 22.6 수직 슬라이스

시작부터 결과까지 Unreal Editor에서 수동 개입 없이 플레이할 수 있어야 합니다.

---

# 23. Codex 작업 명세 템플릿

```md
# 작업명

## 목표

플레이어 또는 개발자가 확인할 수 있는 결과를 작성합니다.

## 배경

`AGENTS.md`와 기능별 명세를 참조합니다.

## 수정 범위

변경 가능한 파일과 시스템을 명시합니다.

## 필수 동작

결정적인 요구사항을 작성합니다.

## 제외 범위

추가하면 안 되는 기능을 작성합니다.

## Blueprint 전달

필요한 수동 에셋 작업을 작성합니다.

## 완료 기준

관찰 가능한 성공 / 실패 기준을 작성합니다.

## 검증

- 정적 확인
- 빌드
- Editor 확인
- 게임플레이 테스트

## 완료 보고

- 변경 파일
- 구현 내용
- 가정
- 테스트 결과
- 수동 Editor 작업
- 제한 사항
```

### 조준 시스템 예시

```md
# 작업: 조준 입력 상태 머신

## 목표

짧은 우클릭 Scope 토글과 길게 누르는 Shoulder Aim을 구현합니다.

## 수정 범위

- DreamCatcherCharacter
- DCCombatComponent
- 필요한 경우 DCPlayerHUDWidget

## 필수 동작

- 짧은 우클릭은 Scope 토글
- Hold 기준 기본값 0.18초
- Hip에서 기준 시간 이후 Shoulder 활성화
- Scope에서 우클릭을 누르면 Hip으로 전환하고 Hold 판정을 시작하지 않음
- Shoulder에서 우클릭 해제 시 Hip으로 복귀
- Scope에서 Shoulder로 직접 전환하지 않음
- 회피, 궁극기, 사망, 입력 취소 시 상태 정리

## 제외 범위

- WeaponComponent
- SceneCapture 스코프
- 바이너리 Input Action 직접 생성
- AnimBP 수정
- 최종 UI 아트

## Blueprint 전달

IA_Aim, IMC_Player, Character Blueprint, HUD, AnimBP 연결 절차를 작성합니다.
```

---

# 24. ChatGPT, GitHub, Codex 작업 흐름

ChatGPT 프로젝트의 모든 대화가 Codex에 자동으로 전달되지는 않습니다.

GitHub 문서를 공통 기준으로 사용합니다.

```text
ChatGPT 프로젝트
  -> 기획 및 아키텍처 결정
  -> AGENTS.md 또는 docs/specs/<기능>.md 갱신
  -> Codex가 리포지토리 읽기
  -> 기능 브랜치에서 코드 수정
  -> 개발자 Diff 검토
  -> Unreal Editor 수동 연결
  -> 게임플레이 검증
  -> Merge
  -> 프로젝트 상태 문서 갱신
```

## 24.1 ChatGPT에 둘 내용

- 논의
- 대안
- 시각 아이디어
- 초기 세계관
- 디자인 피드백
- 기능 정의

## 24.2 GitHub Markdown에 둘 내용

- 확정 결정
- 아키텍처
- 구현 요구사항
- 현재 상태
- 완료 기준
- 빌드 규칙
- Editor 절차
- 알려진 위험

## 24.3 Codex 작업 범위

- 소스 분석
- C++ 구현
- Config 변경
- 사용자 빌드 결과 분석 및 승인된 코드 오류 수정 (C++ 빌드·패키징 실행은 사용자 담당)
- 테스트
- 범위가 명확한 리팩터링
- Diff 검토
- Blueprint 전달 문서

---

# 25. 명시적 승인 없이 하지 말 것

- Lyra C++ 소스 또는 ShooterCore 전체를 의존성 검토 없이 복사
- GAS 전환이 검증되기 전에 기존 전투 시스템 삭제
- Lyra Blueprint의 부모 클래스 의존성을 확인하지 않고 Migrate
- 멀티플레이 복제 설계
- 대형 Behavior Tree 프레임워크
- 수직 슬라이스 전에 큰 인벤토리 개발
- 레벨 구조 확정 전 미니맵 개발
- 전체 Content 폴더 일괄 이동
- 대량 에셋 이름 변경
- Unreal Editor 밖에서 바이너리 에셋 수정
- UMG 안에 게임 규칙 계산
- 모든 기능을 Character에 추가
- 보스를 체력 높은 일반 적으로 구현
- 첫 스코프를 SceneCapture2D로 구현
- 정상적인 빌드 전환 문제를 강제 Override로 숨김
- 엔진 전환과 게임플레이 기능을 한 커밋에 혼합
- `main` 직접 커밋
- 두 사람이 같은 `.umap` 동시 수정

---

# 26. 프로젝트 용어

## DreamCatcher

프로젝트 제목이자 악몽 사건을 처리하는 중심 조직 또는 부대.

## Lucid City

근미래 도시 배경.

## Dream Core

꿈을 변환해 만든 에너지.

## 악몽 오염

Dream Core 생산 부산물로 발생하는 악몽 현상과 침식.

## Shoulder Aim

우클릭을 기준 시간 이상 유지했을 때 활성화되는 3인칭 견착 조준.

## Scope

우클릭 짧은 클릭으로 토글되는 확대 조준.

## Hip

기본 비조준 상태.

## Encounter

하나 이상의 EnemySpawner를 실행하고 적 처치 후 완료되는 전투 구간.

## StageDirector

스테이지 페이즈 순서를 실행하는 Actor.

## Vertical Slice

전투, 보스, 결과까지 포함하는 짧고 완결된 대표 플레이 세션.

---

# 27. 현재 확정되지 않은 항목

다음은 현재 자료만으로 완전히 확정할 수 없습니다.

- 최종 공식 스토리 시놉시스
- 최종 캐릭터 이름
- 최종 MBTI
- 최종 플레이어블 캐릭터 수
- 최종 무기 목록
- 최종 보스 목록
- Dream Core의 세부 과학 원리
- 루시드 시티의 정치 구조
- DreamCatcher 조직의 최종 영문 부제
- 최종 스테이지 수
- 최종 성장 경제
- 최종 재화 종류
- 최종 인벤토리 범위
- 현재 논의된 액션 외 전체 조작법
- 모든 HUD 요소의 최종 위치
- 최종 오디오 자산 목록
- 최종 타깃 하드웨어 및 성능 예산
- UE 5.8 전환을 최종 확정할지 여부

별도 결정 없이 확정 설정으로 만들지 않습니다.

---

# 28. 다음 권장 작업 패키지

## 패키지 A — UE 5.8 전환 안정화

- 두 Target 파일 V7 / Unreal5_8 적용
- 클린 빌드
- 새로운 기본 설정으로 드러난 실제 컴파일 오류 수정
- 프로젝트 실행
- Blueprint 부모 클래스 확인
- 디자이너용 바이너리 재빌드
- 엔진 전환만 별도 커밋

## 패키지 B — 조준 시스템

상태: 2026-07-31 1차 기능 프로토타입 완료

- C++ 입력 상태 머신
- CombatComponent 조준 상태
- Blueprint 이벤트
- IA_Aim 및 IMC_Player 연결
- 카메라 프로필, 감도 보간, HUD 상태 전달
- 클릭 / Hold 기준 테스트
- 후속: AnimBP, Aim Offset, Scope 최종 아트 및 전투 연동

## 패키지 C — 적 공격 텔레그래프

- 공격 의도, 선딜, 명중, 후딜 분리
- 연출 이벤트
- 즉시 데미지 제거
- 시야 및 중단 테스트

## 패키지 D — 회피 방어

- 명확한 무적 또는 데미지 차단 상태
- 회피 상태 이벤트
- 충돌 및 사망 상호작용
- 지속시간과 쿨다운 조정

## 패키지 E — Level 1 수직 슬라이스 연결

- 인카운터 구역 확정
- EncounterController와 Spawner 배치
- StageDirector 연결
- 보스 진입 Placeholder
- 결과 Placeholder
- 재시작 검증

## 패키지 F — 최소 보스 기반

- 전용 보스 클래스
- 페이즈 컴포넌트
- 공격 패턴 2개
- 보스 HUD 이벤트
- 사망 완료 신호

---

# 29. 최종 프로젝트 원칙

DreamCatcher는 잘못된 구조 위에 있는 프로젝트가 아닙니다.

현재는 작은 팀에 적절한 구조 위에 아직 완성되지 않은 기능이 쌓여 있는 상태입니다.

프로젝트는 다음 순서로 완성합니다.

- 현재 전투를 읽기 쉽고 시원하게 만듭니다.
- 처음부터 끝까지 플레이 가능한 한 판을 완성합니다.
- 보스를 일반 적과 분리합니다.
- 실제 반복 조정이 필요한 데이터를 DataAsset로 옮깁니다.
- 전투가 재미있다는 것이 확인된 뒤 메타 루프를 추가합니다.

성공 기준은 아키텍처의 복잡성이 아닙니다.

> 현재 구조를 안정적이고, 신나고, 읽기 쉽고, 완결된 DreamCatcher 경험으로 완성하는 것이 성공 기준입니다.
