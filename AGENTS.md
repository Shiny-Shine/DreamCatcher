# DreamCatcher 프로젝트 통합 컨텍스트 및 Codex 작업 지침

> **리포지토리:** `Shiny-Shine/DreamCatcher`  
> **기본 브랜치:** `main`  
> **문서 역할:** 프로젝트 전체 컨텍스트, 개발 정책, 아키텍처, 구현 현황, Codex 작업 규칙  
> **최종 통합일:** 2026-07-31 KST  
> **Lyra 이식 정책 갱신:** 2026-09-10 KST
> **작업 분담·자동화·마감 기준 갱신:** 2026-10-05 KST / **TODO 승인 갱신:** 2026-10-18 KST / **서브에이전트 운영 갱신:** 2026-10-23 KST
> **권장 위치:** 리포지토리 루트 `/AGENTS.md`
> **최신 재개:** 2026-10-24, 아래 **0.140** — 사용자 빌드 후 승인한 PV 에셋3개 적용·별도 재로드, 새 native 입력/정리 검사2개, 실제 PIE의 1920×1080 재생·끝 부근 seek 후 반복·모드 선택 맵 재진입 재생·미디어 실패 시 메뉴 진행을 확인했다. **추가 C++ 변경/재빌드 불필요, 사용자 실제 키보드·마우스·게임패드 입력·화면·소리·패키징 수용 대기**다. 최신 Level_1/2·기존 미커밋 작업·두 Stash를 보존했다. 운영 기준은 0.4·0.138, BaseDamage 완료/수용 한계는 0.136을 유지한다.

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
- 작업 단위와 직접 수정 권한은 아래 **0.4의 작업 분담·2026-10-18 TODO 승인 절차·2026-10-23 직접 작업·서브에이전트 운영 규칙**을 따릅니다. 코드·Config·에셋·문서 작업 모두 TODO와 이해한 동작을 보고해 1차 승인을 받고, 항목별 상세 검토 후 최종 확인된 범위만 작성합니다. 문서만 갱신하라는 요청은 추가 소스·Config·에셋 수정 권한이 아닙니다.
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

1. 아래 0.4의 TODO 1차 승인과 항목별 최종 확인을 따릅니다. 상세 검토에서 원본 근거·변경 이유·대상 경로·검증 범위를 설명하고, 최종 승인된 에셋만 처리합니다.
2. 필요한 C++ API와 이벤트 훅은 승인된 코드 범위에서 구현합니다. C++ 빌드는 사용자가 진행합니다.
3. 승인된 에셋 작업은 Unreal Editor 내부 API로 생성·설정·Compile·저장·검사하도록 자동화할 수 있습니다. Python/Editor Utility/Commandlet을 사용할 수 있지만, 에디터 밖의 일반 파일 I/O로 바이너리 에셋을 직접 복사·변조하지 않습니다.
4. 자동화하지 못한 부분만 정확한 수동 Editor 절차로 전달합니다. 자동 검사와 사용자 PIE·시각·조작감 검증을 구분합니다.

### 0.4 작업 분담·자동화·마감 — 2026-10-05 승인 / 2026-10-18 TODO 승인 / 2026-10-23 직접 작업·서브에이전트 운영 갱신

이 항목은 과거의 “한 채팅에서 한 세부 단계를 사용자가 직접 구현”하는 운영 방식보다 우선합니다. 2026-10-18 이후의 승인·작성 순서는 아래 TODO 절차를 적용하고, 2026-10-23 직접 작업 우선·서브에이전트 병렬 분담 기준과 2026-10-19의 작업 후 정리 규칙을 함께 따릅니다. 원본 재사용 정책과 변경 전 근거 설명 의무는 유지합니다.

**작업 분담과 승인 단위**

- 사용자: **C++ 빌드, 패키징, 조작감·화면·소리·연출 확인과 GitHub Desktop 커밋·푸시·병합**을 직접 진행합니다. Codex는 별도 지시 없이는 이를 대신 실행하지 않습니다.
- Codex: 원본 조사, 승인된 코드 작업, **Unreal 내부 에셋 생성·설정·검사 자동화**를 담당합니다. 사용자는 에셋 자동화를 허용했습니다.
- 진행 순서는 **요구사항 → TODO·이해한 동작 보고 → 1차 승인 → 항목별 상세 검토 → 최종 확인 → 직접 작성 또는 독립 작업 병렬 분담 → 통합 검증·결과 보고**입니다. 필요한 사용자 C++ 빌드와 플레이 확인은 기존 분담에 따라 진행합니다.
- “완료 상태 확인”, “다음 단계 진행”만으로 새로운 코드·Config 수정 범위를 임의 확대하지 않습니다. 문서 수정 요청이면 문서만 수정합니다.
- 의존성이 강한 여러 R 세부 단계는 묶을 수 있지만, 포함 범위·앞당기는 이유·각 단계의 미검증 항목을 먼저 밝힙니다. 원본 기능의 삭제·축소로 일정을 맞추지 않습니다.
- 자동화는 대상 경로와 작업을 제한하고 기존 변경을 보존합니다. 기존 에셋 덮어쓰기·삭제·대량 참조 교체·브랜치 병합은 해당 범위를 명시한 승인을 확인합니다. 에디터와 자동화 프로세스의 동시 저장도 피합니다.
- 명령줄 에디터 실행과 Blueprint Compile은 C++ 빌드·패키징과 구분합니다. 임시 에디터 월드의 검사는 PIE·복제·장비 연결 검증으로 보고하지 않습니다.

**2026-10-18 승인 — TODO·이해한 동작 보고와 두 단계 승인**

이후 사용자가 제시하거나 요청하는 작업은 다음 절차로 진행합니다. 코드·Config·에셋뿐 아니라 문서 작성·수정에도 적용합니다.

1. 사용자의 요구사항을 **TODO 목록**으로 정리하고, 각 항목의 **목적·이해한 예상 동작·변경 범위·불확실한 점**을 보고합니다. 사용자가 목록을 수정·추가·삭제할 수 있도록 한 뒤 **1차 승인**을 받습니다.
2. 1차 승인된 항목을 하나씩 상세 검토합니다. **현재 구현·원본 근거·수정할 파일과 에셋·원본 및 현재 동작과의 차이·의존성·검증 방법·직접 작성 또는 병렬 분담안**을 목록으로 보고하고 사용자의 **항목별 최종 확인**을 받습니다. 확인에 필요한 읽기 전용 조사는 작성과 구분합니다.
3. **최종 확인 전에는 작성·수정을 시작하지 않습니다.** 부분 승인이라면 최종 승인한 항목만 진행합니다. 범위 변경이나 미승인 항목 추가가 필요하면 해당 변경을 다시 목록으로 제시해 확인받습니다. 이미 최종 승인된 동일 범위에는 불필요한 재승인을 요구하지 않습니다.
4. 작성은 **Codex가 직접 진행하는 것을 기본**으로 합니다. 아래 2026-10-23 기준에 따라 **서브에이전트 2개 이상에게 맡길 수 있는 독립 작업으로 나눠 실제 병렬 처리할 수 있을 때만** 서브에이전트를 사용합니다. 그 외의 작은 작업, 같은 파일·에셋 수정, 공통 API·설계가 미확정인 작업, 선행 결과가 필요한 작업은 직접 순차 처리합니다. 병렬 분담할 때는 각 에이전트의 담당 범위를 나누고 기존 사용자 변경을 보존합니다.
5. 직접 작업할 때는 Codex가 결과를 검토하며, 병렬 분담할 때는 부모 에이전트가 범위·진행·의존성을 조율하고 결과를 통합 검토합니다. 관련 검증을 마친 뒤 **TODO별 완료·미완료·사용자 확인 사항**과 검증 근거·한계를 보고합니다. 사용자 빌드·화면·소리 확인을 자동 검사 완료로 대신 표시하지 않습니다.

**2026-10-23 변경 — 직접 작업 우선·서브에이전트 병렬 분담 / 2026-10-19 결과 회수 후 정리 유지**

- **서브에이전트 2개 이상으로 독립 작업을 나눠 실제 병렬 처리할 수 없는 상황에서는 Codex가 직접 작업**합니다. 서브에이전트1개에만 단독 위임하는 방식은 사용하지 않으며, 병렬 분담을 위해 작업을 억지로 쪼개지 않습니다.
- 병렬 분담할 때 **동시 활동 서브에이전트는 하위 재위임까지 합산해 최대3개**이며, 부모 조율자는 이 수에 포함하지 않습니다.
- 병렬 분담은 **상호 소통·중간 결과 전달이 거의 필요 없는 독립 작업에만** 사용합니다. 같은 파일·에셋 수정, 공통 API·설계 미확정, 선행 결과 의존 작업은 직접 순차 처리합니다. 미래 작업을 기다리기만 하는 에이전트를 미리 만들지 않습니다.
- 담당 결과를 회수하면 즉시 **사용 가능한 종료 수단으로 정리하고 실행 상태를 확인**합니다. 완전 종료 도구가 없으면 완료 상태를 확인한 뒤 불필요한 호출·대기를 중단하고 그 한계를 보고합니다. 현재 도구에는 세션 완전 종료 기능이 없으며 `interrupt_agent`는 현재 실행을 중단하는 기능입니다. 완료·중단을 세션 완전 종료로 표현하지 않고, 완료 후 대기 중인 세션이 존재한다는 이유만으로 계속 토큰을 사용한다고 단정하지 않습니다.

원본 재사용 순서, Unreal 내부 API를 통한 바이너리 에셋 처리, 사용자 빌드·패키징·GitHub Desktop 작업 분담은 그대로 유지합니다. 이후의 단순한 “진행” 요청을 새 범위의 최종 승인으로 확대하지 않습니다.

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

## 0.21 검증 이력 — 2026-10-09 18:26 KST 태그 잔류 회귀 통과 / 이번 자동 감사 마감

- 사용자 UBT 실제 컴파일 기록은 **18:20:53 시작/8.65초 Succeeded**, Character.cpp 컴파일 및 DLL **18:21:01/4,391,936bytes** 갱신입니다. 직후18:21:11의 최신 Log.txt는 Target up to date/1.28초 성공이고 실제 컴파일 기록과 구분했습니다. Codex는 빌드를 실행하지 않았습니다.
- `R4Crouch-Handoff2.log` **18:24:14**에서 동일한 crouch 중 Pawn 교체 후 새 Pawn은 **bIsCrouched=false/Status.Crouching0**입니다. 같은 ASC 재사용, Ability9/장비1/Inventory1, 탄약30→27 재사격과 최종태그0까지 정상입니다. `R4CrouchHandoffFixed_20261009/index.json` **18:24:30 성공1/실패0/보고서경고0**으로 기존 잔류 결함을 닫았습니다.
- `R4Crouch-ToggleFixed1.log` **18:26:20~21**의 정상 Action 토글도 태그1/캡슐65/raw0.480661 → release 후 crouch 유지 → 다시 입력하면0/96/raw0.799338로 통과했습니다. `R4CrouchToggleFixed_20261009/index.json` **18:26:24 성공1/실패0/보고서경고0**입니다. 기존 임시 진단에 토글만 확인하는 모드를 사용했고 이미 통과한 거리/표면 수치를 재검사하지 않았습니다.
- 두 프로세스 종료0, Error/Fatal/ensure0이며 기존 import/PoseAsset 로드 경고15개는 남습니다. 게임 C++/Config/에셋 저장/빌드/패키징0, 보호11파일 해시 불변입니다. 장치/물리키/시각/복제 전체 검증이 아닙니다.
- **R7 전으로 합의한 이번 R0~R6 자동 감사는 마감합니다.** 알려진 태그 잔류까지 해결·재검증했고 검사를 새로 늘리지 않습니다. 남은 마감 조건은 사용자 화면 확인: (1) Left Ctrl 웅크림/해제 자세·카메라 높이, (2) 조준/연사 시 비정상적인 이중 반동·떨림 여부입니다. 마지막 명시 답변이 미확인이므로 확인됐다고 추정하지 않습니다.
- 다음 작업은 **R7-1/2 원본 ReticleHost·라이플 조준점/탄약 표시의 통합 테스트 맵 연결**입니다. 화면 확인이 정상이라면 원본 대비 연결 차이와 정확한 코드·에셋 범위를 다시 제시/승인 후 진행합니다. R7 새 구현은 아직 미적용입니다. 기존 장치/복제/전체 실맵·R8 시각·패키징 범위가 완료됐다는 뜻은 아닙니다.

---

## 0.22 승인 전 제안 이력 — 2026-10-10 사용자 확인 완료 / R7 연결 묶음

- 사용자가 남았던 화면·조작감의 정상 작동을 확인했습니다. **R7 전으로 합의한 R0~R6 마감 범위는 자동 검사와 사용자 확인까지 완료**로 기록합니다. 웅크림/해제 자세·카메라 높이, 조준/연사 시 비정상적 이중 반동·떨림 확인 대기는 해소됐습니다. 전체 장치/복제/실맵/R8 시각/패키징의 모든 조건이 완료됐다는 뜻은 아니며 기존 별도 범위를 유지합니다. 이미 통과한 감사를 다시 시작하지 않습니다.
- 다음은 **R7-1/2 원본 ReticleHost 기반 라이플 조준점·탄약 위젯의 화면 연결**입니다. 현재 GAS-System의 관련 코드/Config를 읽었고 원본 W_WeaponReticleHost SHA256이10/8 UIAudit 기록과 동일함을 확인했습니다. 원본 Host는 LyraWeaponUserInterface 부모, VisWrapper/WidgetStack, 실제 무기 Instigator→ReticleConfig.ReticleWidgets→CreateWidget/InitializeFromWeapon 흐름이며 사망 표시/구독 관련 그래프도 있습니다. 기존 이식 Rifle 위젯2개와 native 부모/Redirect를 재사용합니다.
- 현재 제약: 프로젝트 Config에 DefaultUIPolicy 지정이 없고 전체 Experience/슬롯 표시 경로가 아직 미연결입니다. 원본 StandardHUD ActionSet은 ShooterCore를 요구합니다. 또한 기존 Controller의 HUDWidgetClass는 UDCPlayerHUDWidget 전용이라 원본 Host 부모와 타입이 다릅니다. **전체 UI 정책/RootLayout/ActionSet을 지금 함께 연결하는 대안**은 공용 UI·입력·추가 의존성까지 범위가 넓습니다. 이식 불가가 아니라 후속 최종 연결 범위입니다.
- **승인받을 묶음(아직 미적용):** `Development/DCRifleIntegrationActors.h/.cpp`2파일에 로컬 플레이어 전용 Host 클래스/인스턴스와 생성·제거 연결 추가; 원본 `/ShooterCore/UserInterface/HUD/W_WeaponReticleHost`를 새 `/Game/LyraMigration/UI/R7/W_WeaponReticleHost`로 Unreal 내부 복제/Migrate; 기존 `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleController`에만 새 클래스 설정. 기존 ID_Rifle/W_Reticle_Rifle/W_AmmoCounter_Rifle은 재사용하며 수정·덮어쓰지 않습니다. 새로운 범용 복사기/감사기는 만들지 않습니다.
- 원본 대비 차이는 **Host 표시/제거의 시작 주체가 UIExtension/GameFeature 슬롯 대신 통합 테스트 Controller**라는 점입니다. 로컬 플레이어의 유효한 무기/아이템이 있을 때 표시하고 장비 없음·사망/조종해제/EndPlay에는 제거해 잔여 표시를 막습니다. 원본 NativeTick은 유효한 새 무기만 처리하므로 empty-slot 정리는 Controller 연결에서 명시적으로 확인합니다. 원본 Host/자식 위젯의 계산·그래프는 유지하며 전체 HUD/슬롯 수명 완료로 보고하지 않습니다.
- 공용 HUD/Config/다른 맵과 게임 입력모드는 변경하지 않습니다. 새 Host와 기존 HUD가 실제로 중복되면 임의 삭제하지 않고 정확한 대상/추가 변경을 먼저 설명합니다. 이번 묶음은 기존 HUD 숨김·수정의 포괄 승인이 아닙니다.
- 다음 순서: 범위 승인 → 코드 작성 → 사용자 빌드 → 에셋 자동 이식/설정·Compile·재로드·실제 UI 연결 검사 → 사용자 짧은 화면 확인. 새 UPROPERTY를 기존 DLL에 억지로 설정하지 않습니다. 현재 턴은 문서 상태 갱신/읽기만 했고 게임 코드·에셋·Config 변경 및 빌드/패키징/새 Unreal 검사는 하지 않았습니다.

---

## 0.23 최신 재개 — 2026-10-10 R7 Host 연결 코드 작성 / 사용자 빌드 대기

- 사용자가 R7 묶음을 승인했습니다. **DCRifleIntegrationActors.h/.cpp2파일의 코드 부분을 작성**했고, 새 UPROPERTY 반영을 위한 사용자 빌드를 기다립니다. 원본 Host 에셋 이식/Controller BP 설정은 아직 하지 않았으며 C++ 빌드·패키징·Unreal 실행도 하지 않았습니다.
- IntegrationController에 `WeaponReticleHostClass`(DCLyraWeaponUserInterface subclass, 기본None)와 Transient `WeaponReticleHost`, 소유 Pawn 추적을 추가했습니다. PlayerTick에서 로컬 플레이어/viewport/Pawn GameplayReady/해당ASC/생존/유효한 원본 무기+Inventory Item을 확인하여 AddToPlayerScreen으로1개만 표시하도록 작성했습니다. HUD.bShowHUD도 따르며 입력 모드와 기존 HUDWidgetClass는 바꾸지 않습니다.
- 무기 없음/빈 슬롯·사망·Pawn 변경·ASC 해제·조종해제·EndPlay에서는 제거합니다. ClearIntegrationRifle의 authority guard 앞에서 UI를 정리하고 EndPlay 차단flag로 재생성을 막습니다. RemoveFromParent 전에 자체 참조를 비워 Blueprint Destruct의 정리 재진입에 대비했습니다. 추상클래스/생성·표시 실패는 로그를 남기고 같은 유효 상태에서 매프레임 재시도하지 않습니다. 다른 무기로의 정상 변경 시 내부 위젯 갱신은 원본 Host에 맡깁니다.
- 원본 DCLyraWeaponUserInterface/Reticle 계산은 변경하지 않았습니다. 신규 UI 블록/include와 정리 연결을 제외한 CPP는 작업 전과 동일함을 비교했고, 기존 장비/GameplayReady/GameMode 로직을 보존했습니다. 공용 Controller/UI native/Build.cs/Config/에셋/DLL/Git인덱스 보호13파일 해시 불변, diff 공백 오류0입니다. **이는 정적 확인이며 빌드·런타임 통과는 아닙니다.**
- 다음: 사용자 Development Editor/Win64 빌드 → 새 Host 원본 에셋을 승인된 `/Game/LyraMigration/UI/R7/W_WeaponReticleHost`로 이식 → Integration Controller BP의 WeaponReticleHostClass 설정 → Compile/별도 재로드 및 실제 화면/무기·아이템/수명 검사. 새 속성이 없는 구DLL에 설정하지 않습니다. 현재 클래스None이므로 아직 R7 UI가 나타나지 않는 것이 정상입니다.
- R0~R6 마감은 유지합니다. 이 코드는 원본 GameFeature/UI 슬롯 경로 대신 테스트 Controller가 Host 표시/제거를 맡는 승인된 독립 연결이며 전체 HUD/슬롯·R7 완료를 뜻하지 않습니다. 사용자에게 수동 에셋 설정을 요구하지 않고 승인된 나머지 자동화를 빌드 후 이어갑니다.

---

## 0.24 최신 재개 — 2026-10-10 R7 Host 이식·화면 연결 자동 검사 통과 / 사용자 화면 확인 대기

- **사용자 빌드 확인:** UBT `Log.txt`의15:00:17 시작/44.65초 Succeeded, DreamCatcher DLL15:01:00/4,380,160bytes를 확인했다. Codex는 C++ 빌드·패키징을 실행하지 않았으며 이번 턴에 게임 C++/Config를 추가 수정하지 않았다.
- 승인된 원본 `/ShooterCore/UserInterface/HUD/W_WeaponReticleHost`를 Unreal 내부 복제·Migrate로 새 `/Game/LyraMigration/UI/R7/W_WeaponReticleHost`에 이식했다. 원본 프로젝트에도 같은 Game 경로의 준비 사본1개가 생겼다. 원본 ShooterCore Host는 SHA256 불변이다. 대상에서는 새 Host와 `BP_DC_RifleController.WeaponReticleHostClass`만 저장했다. 기존 ID_Rifle/Reticle/AmmoCounter·공용 HUD·다른 맵은 변경하지 않았다.
- 한정 레시피 `Scripts/Editor/r7_reticle_host_setup.py`, 실제 PIE 관찰용 `r7_reticle_host_probe.py`를 추가했다. 범용 Editor 플러그인/검증 프레임워크는 확장하지 않았다. 후속 범위 밖의 에셋을 덮어쓰거나 삭제하지 않았다.
- 최초 Migrate는 파일을 생성했지만 Blueprint 검색 DB의 동일 경로 인덱스 ensure가 발생하여 그 실행은 실패로 기록했다(`R7Host-prepare_1515.log`). 이어 별도 원본 프로세스에서 저장 사본/원본 그래프·기본값·WidgetTree를 비교했다. 이식 전후 바이너리 해시는 다르므로 byte-identical이라고 보고하지 않는다. 대상의 의미상 비교/Compile/재로드를 별도로 통과했다.
- **15:13:46 저장·15:14:26 새 프로세스 재로드 통과:** 그래프2개/노드65개의 핀·값·연결과 WidgetTree2개/기본값을 비교했다. 차이는 기존 native Redirect로 Cast 출력 핀 `AsLyra Inventory Item Instance`가 `AsDCInventory Item Instance`로 재구성된1개 이름 대응뿐이다. 원본 그래프의 연결/동작을 별도 수정하지 않았다. 재로드 package 참조는 Engine StandardMacros 및 DreamCatcher/SlateCore/UMG/UMGEditor이며 ShooterCore/LyraGame 잔여 참조가 없다. 상세 `Saved/Diagnostics/R7ReticleHost/reload1514.json`.
- NullRHI PIE에서는 Host 등록1개만 확인되고 자식 위젯이 비어 있어 검사 실패로 남겼다. 수동으로 NativeTick/OnWeaponChanged를 호출하거나 게임 코드를 바꾸지 않고 **렌더링을 켠 별도 PIE**로 검증했다. **15:17:50 관찰 통과/15:18:14 Project.Maps.PIE 성공1·실패/보고서경고0**, 해당 로그 Error/ensure/Fatal0이다(`Saved/Logs/R7Host-PIERender1.log`, `Saved/Automation/R7HostPIE_20261010_Render/index.json`).
- 실제 Host NativeTick이 W_Reticle_Rifle+W_AmmoCounter_Rifle2개를 생성하고 동일한 Weapon/Inventory Item을 참조했다. 발사/수동재장전의 실제 아이템 탄약30→27→30, 위젯 퍼짐2→약3.70→2, 조준 시0/첫발정확도true를 관측했다. 빈 슬롯·조종해제·사망 때 viewport Host0, 재장착/새 Pawn 때1개/새 데이터로 복귀, PIE 종료의 Host 제거도 확인했다. 해당 로컬 플레이어의 viewport 최상위 UMG는 새 Host1개였다. 전체 화면의 픽셀 중복·탄약 머티리얼/텍스트 가독성·모든 구독 해제까지 자동 증명한 것은 아니다.
- 마지막 검사 후 원본/재사용 위젯/새 Host/Controller/Config/코드/Git인덱스 등 보호37경로의 상태가 재로드 직전과 동일했다. 검사는 process-only GameplayCue 비활성/무음이므로 음향·FX·복제 증거가 아니다. R0~R6 사용자 통과 상태는 유지한다.
- **현재 R7-1/2 승인된 독립 화면 연결 묶음의 구현·자동 검사 완료, 사용자 화면 확인 대기.** 추가 C++ 빌드나 수동 에셋 설정은 필요 없다. `L_DC_RifleIntegration`에서 조준점/탄약 표시가 중복 없이 보이며 발사·R재장전·조준 변화가 자연스러운지만 짧게 확인한다. 이후 R7-3 Aim/Scope·명중/처치 표시와 최종 HUD/슬롯 경로의 실제 근거·차이·변경 범위를 제시한다. 전체 R7·장치/DPI/복제/전체 HUD 완료로 확대하지 않는다.

---

## 0.25 최신 재개 — 2026-10-10 R7 사용자 피드백: 원본 대비 크기·회복 차이 조사 / 수정 승인 대기

- 사용자는 Host 동작은 정상이지만 원본보다 조준점이 크고 사격 후 회복 시작이 늦다고 보고했다. 추가 답변은 **정지 Hip와 조준/사격 후 모두 크다**이다. R7 화면 일치 확인을 완료로 표시하지 않고 다음 R7-3 진입 전에 이 차이를 다룬다. 기존 장착·탄약·UI 수명 검증 성공은 보존한다.
- 양쪽 프로젝트를 별도 읽기 전용 commandlet로 조회했다(`Saved/Diagnostics/R7ReticleComparison/{source,target}.json`, `*-detail.json`, `Saved/Logs/R7Reticle-*.log`). 에셋 저장/Compile0, 게임 C++/Config 변경0, 빌드/패키징0이다. 임시 Saved 진단 스크립트와 본 상태 문서만 작성했다. 기존 범용 도구는 확장하지 않았다.
- **확정된 회복 차이:**10/9 승인 보완으로 대상 `DCLyraRangedWeaponInstance.cpp::AddSpread`에만 LastFireTime 갱신이 추가됐다. 로컬 원본은 이 별도 시계를0 초기화 후 갱신하지 않는다. 따라서 원본의 실제 발사 후 냉각과 달리 대상은 매 발사 후0.15초 기다리며 연사 중 냉각도 줄어든다. 이전 실제 대상 수정 전/후6발 결과5.282824°/7.457666°와0.15초 지연 관찰이 근거다. 원본과 같은 동작이 아니라 설정 지연을 작동시킨 보완이었다. 원본 동작 우선이라는 최신 요청에 맞춰 원복안을 제시하되 아직 수정하지 않았다.
- **크기 관련 코드 차이:**10/6 보완은 원본 UMG에서 Slate로 전달되지 않던 OutsideRadius 옵션을 연결했다. 실제 양쪽 CrossHairs는 옵션true, 이미지6×20, 초기반경24로 같고 대상에서는 반경에 ImageSize.X/2=3 UI단위가 추가된다. 원본 Slate bool은 명시적 초기화도 없으므로 ‘원본은 반드시false/정확히3px작다’고 단정하지 않는다. 소스 확인상 유력한 차이이며 동일 viewport/FOV의 렌더 A/B는 미검증이다.
- **배율 차이:** 원본 UI ScaleToFit(설계1920×1080), 대상은 ShortestSide이다. 공통 ApplicationScale1/동일 DPI곡선이더라도 비16:9 창에서는 배율이 다르다. 예를 들어1280×960에서 원본약0.667, 대상약0.889다. HighDPI GameMode도 원본true/대상false이나 현재 PIE 증상의 기여도는 미검증이다. 전역 UI 설정 변경은 다른 HUD/디자이너 UI까지 영향을 주므로 임의로 적용하지 않는다.
- 읽은 범위에서 원본/대상 Reticle 그래프66노드의 핀·연결·값은 기존 타입/구조체 대응 후 일치한다. 위젯9개, CrossHairs 이미지/배치, SizeBox128/58, 무기 열/퍼짐/냉각 곡선 및 배율도 같다. Hip 기본FOV80/ADS70도 같다. 라이플 조준점 MIC의 부모/조회 가능한 scalar/vector/texture/기본 override는 같고 바이너리 해시는 다르다. protected StaticParameters와 상속 머티리얼 전체 시각 동등성은 이 검사로 확정하지 않는다.
- **제안/미적용:** 게임 CPP2개(AddSpread의 추가 시계 갱신 제거, Circumference UMG→Slate 추가 옵션 전달 제거)와 대응 검사CPP2개를 원본 동작 기준으로 수정한다. Slate의 안전한false 초기화는 유지하고 원본의 미초기화 메모리까지 복제하지 않는다는 차이를 먼저 설명한다. 원본 무기 곡선·0.15 데이터값·위젯 크기/머티리얼·사운드·Config는 유지한다. 사용자가 코드 작업을 승인하면 적용하고 사용자가 빌드한다. 그 뒤 같은 viewport/FOV/상태에서 회복과 조각 간격을 비교한다. UI 배율의 잔여 차이가 있으면 전역 정책 원본 적용과 Host 한정 보정의 영향/차이를 제시해 별도 선택받는다. 단순 Radius/RenderScale/Delay값 조절로 겉모양만 맞추는 안을 우선하지 않는다.

---

## 0.26 최신 재개 — 2026-10-10 원본 회복·조각 배치 복원 코드 작성 / 사용자 빌드 대기

- 사용자가0.25의 정확한 범위를 승인했다. **게임 CPP2개와 기존 검사 CPP2개만 수정**했다. 빌드·패키징·Unreal 실행·에셋 저장은 하지 않았다. 아래는 코드/정적 비교 결과이며 컴파일·런타임 성공으로 보고하지 않는다.
- `DCLyraRangedWeaponInstance.cpp::AddSpread()`에서10/9 추가한 LastFireTime 갱신/주석3줄만 제거했다. 원본 AddSpread/UpdateSpread와 함수 본문이 일치한다. 원본의 LastFireTime0 초기값·회복조건 및 에셋의0.15 데이터·열/퍼짐/냉각 곡선은 유지하고, 매 발사 후 대기를 새로 시작하던 대상 보완만 제거했다. 기반 TimeLastFired/AutoReload 시계는 그대로다.
- `DCLyraCircumferenceMarkerWidget.cpp`의 RebuildWidget/SynchronizeProperties에서 추가 OutsideRadius 전달2곳을 제거했다. 원본과 같은 UMG 연결이며 UMG 에셋의 옵션값true를 변경하지 않는다. Slate의 결정적인false 초기화는 유지하여 반경+반폭을 적용하지 않도록 작성했다. 원본의 미초기화 bit를 그대로 복사하지 않는 승인된 안전 차이는 남는다. Slate 헤더/CPP/독립 옵션 API는 이번에 변경하지 않았다.
- `DCWeaponFoundationAutomationTests.cpp`의 지연 보완 테스트를 `DreamCatcher.R6.Weapon.OriginalSpreadRecovery`로 변경했다. 합성 곡선/월드시간1초 이후 발사에서8→7.8, 다음 발사8.8→8.6→8.48→8.38을 기대하고, 설정0.15 보존을 확인하도록 작성했다. 기존 SpreadAndAttenuation/ItemTagCost는 유지했다. 이전 SpreadRecoveryDelay 성공 이력은 원복 전 보완의 증거이지 이번 코드의 통과 증거가 아니다.
- `DCCircumferenceAutomationTests.cpp`의 기존 WidgetLifecycle에서6×20 브러시·반경24·UMG옵션true에도 Slate false/조각위치(0,-24), 옵션 토글·재생성 뒤false 유지, Radius/MarkerList 전달/해제를 검사하도록 바꿨다. SlateGeometry/DependencyRegistration은 유지했다. 새 범용 테스트 도구/API는 추가하지 않았다.
- **정적 확인:** AddSpread/UpdateSpread/RebuildWidget/SynchronizeProperties4개 함수는 이름/주석/공백 정규화 후 원본과 일치한다. Ranged CPP는 승인한3줄 제거 외 작업 전과 동일하다. Fixture 기대값의 산술을 점검하고 diff 공백 오류0을 확인했다. 보호22파일(Config/uproject/Git인덱스/DLL/기존헤더·Slate·Reticle·Integration코드/Host·Controller·맵·Rifle 위젯·무기·발사/재장전·머티리얼)의 SHA256은 불변이다.
- **다음:** 사용자 Development Editor/Win64 빌드 → 기존 무기3건(OriginalSpreadRecovery 포함)/Circumference3건 재실행 → 동일 viewport/FOV에서 실제 회복 시작·조각 간격 비교. 아직 새 코드의 빌드/검사/PIE/시각 일치는 미검증이며 R7 다음 기능으로 넘어가지 않는다. 에셋 수정/재마이그레이션은 필요 없다. DPI 정책·HighDPI·UI 배율은 미변경이며 크기 차이가 남으면 전역 영향/Host 한정 대안을 설명 후 별도 승인받는다.

---

## 0.27 최신 재개 — 2026-10-10 원본 호환 복원 검증 통과 / DPI 정책 적용안 승인 대기

- **사용자 빌드 확인:** UBT16:08:52 시작/30.72초 Succeeded, 수정CPP4개 컴파일과 DLL16:09:22/4,382,720bytes를 확인했다. Codex는 빌드·패키징을 하지 않았다.
- **16:15:13 자동 검사 통과:** `Saved/Automation/R7OriginalRestore_20261010_B/index.json`의 무기3건(OriginalSpreadRecovery 포함)+Circumference3건+실제 PIE1건은 성공7/실패·보고서경고0이다. `R7OriginalRestore-ChecksPIE2.log` Error/Fatal/ensure0. 첫 실행은 C++6건 통과 뒤 관찰 Python의 SlateBlueprintLibrary 별칭 오류로 PIE가 실패했다. 엔진 ScriptName=SlateLibrary임을 확인하고 기존 native 반사 API 호출로 교정했으며 게임 코드는 추가 변경하지 않았다.
- 실제 라이플6발(탄약30→24) 후 **0.028808초부터 열/퍼짐 감소**를 확인했다. 그 뒤0.045475/0.062142초에도 계속 줄어0.15초 발사 후 대기가 제거됐다는 기준을 통과했다. 실제 Host 자식2개/Weapon·Item 연결, UMG Radius와 원본 화면투영 getter의 일치도 확인했다. 테스트 fixture의 UMG옵션true/Slate false/반경24 배치 검증도 통과했다. 코드 원복 묶음은 이 범위에서 검증 완료다.
- **DPI 설정 차이 확인:** 같은1813×1326 viewport/FOV80에서 현재 ShortestSide 배율1.227778, 원본 ScaleToFit 규칙 계산값0.944271이다.16:18:17에는 `-ini:Engine:[/Script/Engine.UserInterfaceSettings]:UIScaleRule=ScaleToFit`을 **별도 검사 프로세스에만** 적용해 실제 API의 배율0.944271을 확인했다. `R7OriginalRestore-DPIProcess.log`/`Saved/Automation/R7OriginalDPIProcess_20261010/index.json`16:18:31 성공1/실패·경고0, Error/Fatal/ensure0이다. 이 실행도6발 후0.033708초부터 냉각됐다. Config 저장은 하지 않았다.
- 약30%는 **이 viewport의 설정 배율 비율**이지 원본과 현재 조준점 픽셀이 실제30% 차이 난다는 확정이 아니다. 원본 프로젝트의 동일조건 렌더 A/B/사용자 수정후 화면은 아직 미확인이다. Python cached geometry 조회는 local size0/scale1을 반환해 실측 픽셀 증거로 사용하지 않는다. 실제 전체 UI 레이아웃/플랫폼 DPI의 픽셀 기여도는 별도 확인한다. 이 한계를 숨겨 R7 화면 동등성 완료로 보고하지 않는다.
- 이번 턴은 기존 검사와 한정 Saved 진단 스크립트/문서만 사용했다. 에셋 저장0, 게임C++/Config 변경0, 보호16경로(수정CPP4/Config3/uproject/Git인덱스/DLL/Host·Controller·맵·라이플 위젯·무기·머티리얼) 해시 불변. GameplayCue는 검사 프로세스에서만 비활성/무음이며 FX·오디오·복제 검증이 아니다.
- **다음 변경안/미적용:** 원본UI정책을 재사용하려면 `Config/DefaultEngine.ini`의 `/Script/Engine.UserInterfaceSettings`에 `UIScaleRule=ScaleToFit`만 명시한다. ApplicationScale1/DesignScreenSize1920×1080은 현재도 같아 변경하지 않고, HighDPI/에셋 크기/무기값/공용HUD 그래프도 건드리지 않는다. 영향은 조준점만이 아니라 모든 UMG의 창 비율별 배율이므로 디자이너UI 가독성/클리핑 회귀가 필요하다. Host한정 배율 보정은 다른UI영향을 줄이는 대안이나 원본 전체UI정책과 다른 추가 구현이다. 전역 설정 적용은 사용자 범위 승인을 받고 진행하며 C++재빌드는 필요 없다. R7-3의 다음 기능으로 넘어가는 것은 아직 보류한다.

---

## 0.28 최신 재개 — 2026-10-10 원본 ScaleToFit 설정 적용·재로드 통과 / 사용자 화면 확인 대기

- 사용자가0.27의 전역UI 정책 변경1항목을 승인했다. `Config/DefaultEngine.ini:7~8`에 `/Script/Engine.UserInterfaceSettings` 섹션과 `UIScaleRule=ScaleToFit`만 추가했다. 기존설정에서 승인된3줄(섹션/값/빈줄)을 제거한 정규화해시가 작업 전과 같으며 다른값은 변경하지 않았다. ApplicationScale1/설계1920×1080/HighDPI false, C++·에셋 크기·머티리얼·무기 데이터는 유지했다.
- **새 프로세스 영구설정 확인 통과:** 이전 process-only UI override 없이 실행했다.16:26:04 실제1813×1326 viewport/FOV80에서 `GetViewportScale=0.94427085`, 원본ScaleToFit 계산값0.94427083과 일치했다. 정지 Hip 최종퍼짐2°/투영반경24.518799, 실제Reticle/AmmoCounter·무기/아이템 연결과6발 후0.037769초부터 냉각도 확인했다.
- 근거 `Saved/Logs/R7PersistedDPI-PIE.log`, `Saved/Diagnostics/R7ReticleComparison/restore-pie-persisted-dpi.json`, `Saved/Automation/R7PersistedDPI_20261010/index.json`.16:26:18 PIE 성공1/실패·보고서경고0, Error/Fatal/ensure0이다. 이전 원복C++6건 성공은 유지하며 이번 Config변경에 C++를 다시 빌드하거나 범용 도구를 추가하지 않았다.
- 대표해상도1920×1080/1280×720/1280×960/2560×1080의 원본정책 기대배율1/0.666667/0.666667/1을 확인했다. **이는 정책 계산값이며 각 해상도의 실제 렌더/잘림 검증은 아니다.** 원본 프로젝트와의 픽셀 A/B, 사용자 최신화면, 디자이너UI/다른HUD의 크기·클리핑은 아직 미검증이다. cached geometry의0크기는 여전히 픽셀 증거로 쓰지 않는다.
- 검사 후 게임CPP4·다른Config2·uproject/Git인덱스/DLL/Host·Controller·맵·Reticle·무기·머티리얼 등 보호15경로 SHA256 불변. DefaultEngine도 승인한추가 외 불변이다. 에셋 저장0/빌드0/패키징0이며 Saved 임시 진단과 문서만 함께 갱신했다.
- **다음:** 추가 빌드·재마이그레이션 없이 에디터를 재시작하고 `L_DC_RifleIntegration`에서 가능하면 원본과 같은 viewport 크기/Hip상태로 조준점 크기·발사 후 회복을 확인한다. 전역 정책이므로 다른HUD/디자이너UI의 잘림도 짧게 확인한다. 사용자확인 전 R7화면동등성/전체R7 완료로 처리하거나 R7-3 다음 기능을 임의 구현하지 않는다.

---

## 0.29 최신 재개 — 2026-10-10 크기·회복 사용자 확인 완료 / R7-3 기존 신호 조사

- 사용자가 ScaleToFit 적용 후 정상 작동을 확인했다. **직전 조준점 크기·회복 보완 묶음은 자동 검사+사용자 확인 완료**로 기록하고 되풀이하지 않는다. 모든 해상도/디자이너UI/복제/전체HUD 동등성 완료라는 의미는 아니다. 다음은R7-3이며 이 턴은 조사/한정 임시PIE/문서 갱신뿐이다. 게임C++/Config/에셋 변경·빌드·패키징은 하지 않았다.
- 실제 통합맵에서 입력Action으로Shoulder/Hip/Scope/Hip 전환, 라이플 단발100→88/탄약30→29, 추가연사88→0/탄약21을 관측했다. 실제 사망 경로가 `Lyra.Elimination.Message`를 보내 원본 `Elimination` 애니메이션이 재생됐다(16:51:34, 시간0.03355/playing=true). `Saved/Logs/R7Signals-PIE3.log`, `Saved/Diagnostics/R7Signals/signals.json`, `elimination.png`가 근거다. 인위적인 메시지 발송/애니메이션 직접 실행은0이다.
- **중요한 원본 사실:** ADS Ability는 `Gameplay.Message.ADS`에ON=true/false 및 실제 소유Controller를 정상 발신한다(`R7Signals-ADSMessageTrace.log`16:57). Reticle의Owner도 같은Controller이고 리스너2개가 존재하며, 발신/수신핀 구조체 경로도 `/Game/LyraMigration/ADS/Struct_UIMessaging`로 같다. 다만 **로컬 Lyra 원본 W_Reticle_Rifle부터 `ADS` CustomEvent(K2Node_CustomEvent_0)의then과 `IsAnimationPlayingForward`(K2Node_CallFunction_11)의execute가 미연결**이다. 이식본도 동일하다. 원본스냅샷`Saved/Diagnostics/R7ReticleComparison/source.json`의 원본uasset해시는 현재도 일치한다. 이식누락/메시지발신부문제로 추정해 고치지 않는다.
- 원본 그대로를 원하는 최신방향에 맞춰 비활성 ADS애니메이션의 실행선을 임의 연결하지 않는다. 애니메이션 활성화를 선택하려면 기존애니메이션/ON분기/정·역재생을 유지하고 해당실행선1개를 잇는 별도 에셋변경안으로 설명·승인받는다. 그 경우 ‘원본에서 실행되지 않던 연출이 켜짐’이 명확한 동작차이다. 원본의 의도적 비활성인지 실수인지는 확인되지 않았다.
- `AimDownSights` 길이는0.160017초, `Elimination`은0.960017초이며 에셋/애니메이션을 새로 만들 필요는 없다. 실제 HitMarker 설정은8×8 브러시/0.4초다. 기존 Slate기간전달 보완은 현재기본값0.4에서 원본과 값차이가 없다. 명중판정 원본TODO(무적 등 실제0피해를 성공으로 보일 수 있음)는 원본CanCauseDamage/서버확인 경로 그대로 남긴다. 이를 실제피해양>0 검사로 임의 변경하지 않는다.
- 실패/제한: 첫PIE는 비변수 EliminationMarker를속성으로 읽으려다 진단실패했다. 두 번째/세 번째는 ‘ADS애니메이션이 실행돼야 함’이라는 가정으로실패했으며, 이후원본도실행선이없음을확인했다. 실패보고서를전체성공으로바꾸지 않는다. 마지막 ADS메시지추적의Automation성공은 추적실행성공이지 ADS애니메이션활성화성공이 아니다. 일반명중스크린샷은편집viewport를잡아명중마커의픽셀/페이드증거로채택하지 않았다. 처치스크린샷은실제PIE의처치연출을확인했다. 네이티브명중배열/약점/빗나감/정확한페이드·복제는미검증이다.
- 보호13경로(Config3/uproject/Git인덱스/DLL/관련C++3/맵/Host/Reticle/무기)의해시불변이다. 기존도구를읽기용으로재사용했으며 새범용도구/C++노출API는추가하지 않았다. 노드내부StructType의Python보호속성조회불가는기록했지만 실행선미연결이라는확인근거를두고구조체치환으로우회하지않는다.
- **다음:** 원본동작유지를기본방향으로두고 일반명중·약점/빗나감 표시/페이드의남은확인을마무리한다. 짧은사용자화면확인이새범용진단개발보다효율적이면그절차를제시한다. ADS연출활성화는별도선택이며 전체HUD/슬롯경로·R7전체는여전히미완료다. 추가수정이필요하면정확한코드/에셋범위를다시설명하고승인받는다.

---

## 0.30 제안 이력 — 2026-10-10 명중/약점/빗나감 확인 완료, 원본 Root UI 연결 제안

- 사용자가 일반 명중 표시가 잠깐 나타난 뒤 사라지는 동작까지 정상이라고 확인했다. 이 항목을 다시 사용자 확인 대기로 돌리지 않는다. 이번 턴은 기존 코드의 한정 PIE 검사와 문서 갱신이며 게임 C++/Config/에셋 수정, 빌드·패키징은 하지 않았다.
- **실제 사격 4조건 통과:** 통합맵에서 일반 명중12, 약점18, 빗나감0, 동일 팀0의 피해와 탄약30→26을 확인했다. 약점은 임시 PIE 물리재질의 `Gameplay.Zone.WeakSpot`, 팀은 기존 `ConfigureTestTeams`를 사용했으며 실제 라이플 Ability/GE를 거쳤다. 성공 메시지/명중 확인을 직접 만들어 넣지 않았다. 물리재질/팀/위치는 검사 후 복구했고 저장하지 않았다.
- 실제 `HitMarkerConfirmations.PerHitMarkerZoneOverrideImages`의 WeakSpot→Critical 머티리얼 연결을 확인했다. 엔진의 `TakeAutomationScreenshotOfUI`로 **게임 viewport**를 직접 캡처하여 일반/약점 중앙 표시의 색상 차이(약점은 노란색), 약점 표시의0.65초 후 소멸, 빗나감/동일 팀에 명중 표시가 생기지 않음을 이미지에서 확인했다. 기존 전역Shot 명령의 편집 viewport 오캡처를 반복하지 않았다. 네이티브 배열 직접 조회/정밀한0.4초 측정/네트워크는 이 검사 범위 밖이다.
- 근거: `Saved/Logs/R7HitZones-PIE2.log`17:19:17의 `damage_and_zone_binding_pass`, `Saved/Diagnostics/R7HitZones/observations.json`. 이미지는 `Saved/Automation/Tmp/L_DC_RifleIntegration/R7Zones_20261010_{body,weak,miss,friendly}_{hit,fade}/Windows/D3D12_SM6/`이다.17:19:50 보고서는 **성공(경고 포함)1/실패0**, 신규 캡처의 비교기준 이미지가 없다는 경고8건이다. golden image/원본픽셀 동등성 검사를 통과했다고 보고하지 않는다. Error/Fatal/ensure0이며 기존 PoseAsset/광원 경고는 별도다. 첫 실행은 진단의 보호된LatentActionInfo.UUID 설정 시도로 실패했고, 기본값 사용으로 교정했다. 게임 코드 문제로 오인하지 않는다.
- 보호11경로(Config3/uproject/Git인덱스/DLL/관련C++2/맵/Host/Reticle)의 해시 불변이다. 임시 `Saved/Diagnostics/R7HitZones/probe.py`만 사용했으며 새 범용 도구/테스트 프레임워크를 추가하지 않았다.
- **현재:** 단일 Standalone 통합맵의 독립 무기 UI/주요 피드백 검증은 마감했다. 원본에도 비활성인 별도 ADS 애니메이션은 그대로 둔다. 전체 R7은 아직 아니며 Controller 직접표시를 원본 UI 정책/레이어/슬롯 경로로 옮기는 작업이 남았다. 기존 정상경로는 새 경로 검증 후 교체한다.
- **다음 승인 제안(미적용):** 원본 `B_LyraUIPolicy`, `W_OverallUILayout`2개를 새 `/Game/LyraMigration/UI/R7/Foundation/`에 Unreal 내부 복제/Migrate한다. 정책의LayoutClass만 새 레이아웃 경로로 대응시킨다. `DefaultGame.ini`의DCUIManagerSubsystem에DefaultUIPolicyClass1항목을 지정하고, `DefaultGameplayTags.ini`에 원본 UI.Layer.Game/GameMenu/Modal3개 선언을 추가한다(Menu는기존native선언). 원본2개 해시는10/8 조사와 현재가 같으며 의존성은 서로와CommonGame/CommonUI/GameplayTags/UMG native뿐이다.
- DCUIManagerSubsystem/CommonGame, DCLocalPlayer(UCommonLocalPlayer), DCGameInstance(UCommonGameInstance), 원본 기반AddWidgets/WorldActionBase/HUD는 이미 있다. 새 C++/Build.cs/범용도구를 만들지 않는다. 원본은 UIManager가Policy→RootLayout을 생성하고Game/GameMenu/Menu/Modal4레이어를등록한다. 대상은현재정책설정/2에셋이없다. 현재Controller방식유지는대안이지만원본Root/슬롯 연결은미완료로남는다.
- 이번 제안은 기본Root를 실제 연결하는 첫 묶음이다. 전역 UI 정책이므로 모든 로컬 플레이어의 Root 생성/기본 포커스에 영향이 있다. 기존ReticleHost의직접표시·HUDClass·기존HUD/입력방식·다른맵 에셋은 이 묶음에서 교체/삭제하지 않는다. W_ShooterHUDLayout/StandardHUD ActionSet의전체위젯11개와ShooterCore요구flag/Experience연결은별도필수후속이며삭제해우회하지않는다. 승인 후 신규2에셋/설정2파일→재로드/Root1개/4레이어/기존무기UI회귀를검사한다. C++빌드·패키징은필요없고최종전체HUD완료로보고하지않는다.

---

## 0.31 적용·진단 이력 — 2026-10-10 Root 에셋·설정 이식 / Controller 원본 부모 연결 제안

- 사용자가 0.30의 원본 Root UI 묶음을 승인했다. 실행 중인 에디터 2개는 사용자 저장·종료 확인 후 작업했다. 원본 Lyra와 DreamCatcher의 새 `/Game/LyraMigration/UI/R7/Foundation/`에 `B_LyraUIPolicy`, `W_OverallUILayout` 2개를 Unreal 내부에서 복제·저장·마이그레이션했다. 정책의 LayoutClass만 새 경로로 대응하고 원본은 변경하지 않았다.
- `Scripts/Editor/r7_root_ui_setup.py`는 정확히 이 2개/새 목적지만 허용하는 일회성 레시피다. 기존 그래프 조회를 재사용했으며 범용 복사기나 C++ 도구를 확장하지 않았다. 이전 새 마이그레이션의 검색 DB ensure를 피하려고 준비/저장과 Migrate를 분리하고 엔진의 기존 migration API를 프로세스 한정 `AssetTools.UseNewPackageMigration=0`으로 실행했다. 게임 동작/프로젝트 설정의 우회 변경은 아니다.
- **에셋 검증 통과:** 17:40:19 `prepare_B`, 17:41:25 `migrate_A`, 17:42:38 `verify_A` 모두 종료0/complete다. 대상의 별도 프로세스 로드·Compile, 그래프 노드/핀/연결·WidgetTree 구성·조회한 designer 값·부모·LayoutClass 비교, 원본 참조 잔류 없음, 보호 파일 불변을 확인했다. 근거는 `Saved/Logs/R7Root-*.log`, `Saved/Diagnostics/R7RootUI/{prepare_B,migrate_A,verify_A}.json`. 첫 prepare_A의 Python 속성명 IsEnabled 오류는 저장 전에 멈췄고 bIsEnabled로 교정했다.
- 승인된 Config만 적용했다: `DefaultGame.ini`의 DCUIManagerSubsystem.DefaultUIPolicyClass, `DefaultGameplayTags.ini`의 UI.Layer.Game/GameMenu/Modal 3개. 기존 native Menu 태그는 그대로다. 현재 Controller의 ReticleHost 직접 표시, HUDClass, 맵/다른 HUD, 기존 C++/DLL/DefaultEngine.ini는 바꾸지 않았다. 빌드·패키징은 실행하지 않았다.
- **Root 실제 연결은 미완료:** 새 정책 설정 후 17:43:59 PIE에서 viewport Root가 없음을 확인했다. `Saved/Automation/R7RootUI_20261010_A/index.json`은 실패1이며 프로세스 종료0으로 성공 판정하지 않는다. `Scripts/Editor/r7_root_ui_probe.py`의 Root1개/4레이어 Push·Pop 검사는 이 첫 조건에서 멈췄다. 4레이어는 에셋 그래프에서만 확인됐고 실제 등록 성공으로 보고하지 않는다.
- **17:50~17:51 읽기 전용 진단/기존 경로 회귀:** `R7Root-PIE_DiagnoseD.log`의 엔진 getall/obj list에서 설정 경로 정상·정책 인스턴스1개·Root0개를 확인했다. 따라서 정책 미로드와 Controller 알림 누락을 구분한다. 기존 Host1개/자식2개, 실제 사격30→27·재장전30·조준·빈슬롯/재장착·UnPossess/Pawn재시작·사망 UI 정리는 통과했다. `Saved/Automation/R7RootUI_20261010_DiagnoseD/index.json`17:51:00은 기존경로 회귀 성공1/실패·보고서경고0, 로그 Error/Fatal/ensure0이다. Root 성공으로 해석하지 않는다. 진단 초반3회는 Python 비노출타입/전역객체의 메서드명 충돌로 실패했고 공개 엔진 진단 명령으로 교정했다. 게임코드는 수정하지 않았다. Cue를 프로세스 한정 비활성/오디오 미실행했으므로 FX·소리 재검증은 아니다. 원본/동반 파일, 기존 맵/Controller/Host/코드/DLL/uproject/Git인덱스 등 보호값은 불변이다.
- **원본 근거/제약:** LyraPlayerController.h는 `ACommonPlayerController`를 상속하고 원본 CommonPlayerController.ReceivedPlayer가 `OnPlayerControllerSet`을 Broadcast한다. GameUIPolicy.NotifyPlayerAdded는 Controller 생성 전 알림을 구독하고 이후 CreateLayoutWidget을 실행한다. 현재 `ADreamCatcherPlayerController`는 APlayerController 직계이며 이 알림이 없다. DCLocalPlayer의 OnPlayerControllerChanged는 팀 관찰용이지 해당 알림을 대신하지 않는다. CommonPlayerController/GameUIPolicy/GameUIManagerSubsystem/ModularPlayerController CPP4개는 로컬 원본과 해시가 일치한다. 앞서 C++ 없이 완료 가능하다는 예상은 이 원본 부모 의존성 누락을 놓친 것이므로 철회한다.
- **다음 수정안(아직 미적용):** `Source/DreamCatcher/DreamCatcherPlayerController.h`의 include와 부모를 `ACommonPlayerController`로 대응한다. CommonGame/ModularGameplayActors의 Public 모듈 의존성은 이미 있어 Build.cs 추가는 불필요하다. 기존 Controller 이름/자식 BP/입력/카메라/HUD 구현은 보존한다. 원본 Controller·PlayerState·Pawn 알림 및 Modular receiver/Ready/ControllerComponent tick 경로가 모든 해당 자식 Controller에 추가되는 영향이 있으므로 사용자 승인·빌드 후 Root/4레이어·입력/무기UI·Possession 수명을 다시 검사한다. 임의 지연 생성이나 Broadcast 직접 복사는 대안이지만 원본 부모의 전체 알림/Modular 수명을 대체하므로 우선하지 않는다. 전체 LyraPlayerController의 나머지 기능까지 이식 완료됐다고 해석하지 않는다.
- 전체 ShooterHUD/11개 slot 위젯/StandardHUD ActionSet·Experience 연결, 활성 경로 교체/기존 구현 제거, 다른 맵/네트워크/장치/사용자 포커스 검증은 미완료다. 새 정책·에셋은 보존하되 전체 R7 또는 Root 런타임 완료로 표시하지 않는다.

---

## 0.32 코드 적용 이력 — 2026-10-10 원본 CommonPlayerController 부모 연결 / 사용자 빌드 대기

- 사용자가 0.31의 부모 연결 수정안을 승인했다. `Source/DreamCatcher/DreamCatcherPlayerController.h`에서 include를 `CommonPlayerController.h`로, 부모를 `ACommonPlayerController`로 변경했다. 게임 소스 변경은 이 2줄뿐이며 기존 클래스명·멤버·함수·입력/카메라/HUD CPP와 자식 BP는 그대로다.
- 이미 이식된 원본 CommonPlayerController/ModularPlayerController를 상속하여 Controller/PlayerState/Pawn 알림 및 Modular receiver/Ready/ControllerComponent tick 경로를 재사용한다. 수동 Broadcast·지연 Root 생성·새 프레임워크는 추가하지 않았다. 전체 LyraPlayerController 이식 완료는 아니다.
- 정적 확인: 기존 생성자의 `Super(ObjectInitializer)`와 Controller/통합 Controller의 OnPossess·OnUnPossess·EndPlay·PlayerTick의 Super 호출이 이어지고, CommonGame/ModularGameplayActors Public 의존성이 이미 있어 Build.cs 변경은 없다. 이 소스 점검은 UHT/컴파일·런타임 성공의 대체가 아니다.
- Config·에셋·맵·DLL·기존 CPP·Build.cs·uproject·Git 인덱스 등 보호15파일은 변경하지 않았다. 문서2개 외 추가 작업은 없고 C++ 빌드·패키징·Editor/PIE는 실행하지 않았다. 직전의 정책1개/Root0개 결과를 새 코드 성공으로 재사용하지 않는다.
- **다음:** 사용자가 에디터를 종료하고 Development Editor / Win64 빌드를 진행한다. 부모 계층 변경이므로 새 DLL로 에디터를 다시 시작한다. 이후 기존 `r7_root_ui_probe.py`로 Root1개/원본4레이어 Push·Pop 및 기존 무기UI·입력·장착/해제·Pawn교체/사망 수명을 검사한다. 다른 맵의 공통 부모 영향과 사용자 포커스 확인은 별도로 남는다. 현재 R7 Root 연결은 **코드 적용/빌드·PIE 미검증**이며 전체 HUD/슬롯 전환과 R7 전체는 미완료다.

---

## 0.33 검증·제안 이력 — 2026-10-10 Root·4레이어 자동 검증 통과 / QuickBar UI 묶음

- 사용자 Development Editor/Win64 빌드를 UBT 로그에서 확인했다: 18:00:01 시작, 36.64초 Succeeded, DLL 18:00:37 갱신(4,383,744 bytes). Codex는 빌드·패키징을 실행하지 않았다.
- **18:04 자동 검증 통과:** 새 CommonPlayerController 계층에서 정책이 Root1개를 자동 생성했다. Game/GameMenu/Menu/Modal 각각 원본 stack으로 임시 빈 위젯을 Push·Pop하는 검사를 통과했다. 수동 Root 생성/연결 알림 호출로 우회하지 않았다. 기존 무기 Host1개·Reticle/Ammo2개와 사격30→27/재장전30·조준·장비해제/재장착·UnPossess/Pawn재시작·사망 정리도 통과했고 이 과정에서 같은 Root가 유지됐다.
- 근거: `Saved/Logs/R7Root-AfterBuildB.log`18:03:57~18:04:03, `Saved/Diagnostics/R7RootUI/PIE-observations.json`, `Saved/Automation/R7RootUI_20261010_AfterBuildB/index.json`18:04:26 성공1/실패·보고서경고0, 로그 Error/Fatal/ensure/InvalidGameplayTag0. 첫 AfterBuildA는 Root 생성 뒤 검사 스크립트의 subobject 경로 가정에서 실패했다. `r7_root_ui_probe.py`를 원본 그래프가 사용하는 Blueprint 변수4개 조회로 교정했으며 게임 오류를 숨기거나 검증 조건을 제거한 것이 아니다.
- 이 턴은 기존 검사 스크립트 조회 교정·문서 외 변경이 없다. Config/게임 C++/에셋/맵/DLL/uproject/Git인덱스 등 보호15파일 해시 불변. 렌더링을 켠 단일 PIE이며 Cue를 프로세스 한정 비활성/무음으로 실행했으므로 FX·소리·복제 검증은 아니다. 사용자는 새 빌드의 실제 마우스 포커스/표시 확인을 **아직 하지 않았다**고 답했다. 이 수동 확인과 다른 맵·장치 검증은 미완료로 남긴다.
- **다음 제안(미적용):** R7의 원본 `W_QuickBar`, `W_QuickBarSlot`, `W_WeaponAmmoAndName`3개 및 필수 입력/폰트/머티리얼 참조를 새 `/Game/LyraMigration/UI/R7/Equipment/` 범위에 준비·이식하고 실제 Inventory/QuickBar 데이터로 독립 검사한다. 기존 DCLyraTaggedWidget·Inventory·QuickBar 및 도구를 재사용하며 새 범용 도구를 만들지 않는다. 10/8 원본 조사에서 이 UI 묶음의 재귀 content 참조는66개이며 이는 예상 manifest 상한이지 이미 가져온 개수가 아니다. 기존 공용 에셋을 덮어쓰지 않고 기존 Reload InputAction의 실제 동일 인스턴스 연결을 우선 확인한다. QuickSlot 입력 바인딩·버튼 동작·기존 참조 재사용의 동등성은 아직 미검증이다. 추가 C++/Config/기존 에셋 변경 필요 시 근거와 범위를 별도 설명한다.
- 원본 ShooterHUDLayout의 재귀 content 참조484개에는 W_LyraGameMenu→설정 UI/프론트엔드 맵과 `/Script/GameSettings`가 포함되고 대상에는 해당 원본 GameSettings 플러그인이 없다. 원본 UI4개 해시는10/8 조사와 현재가 같다. 숫자는 당시 참조 메타데이터이며 최신 전체 closure/전체 native 호환 검증으로 확대하지 않는다. 메뉴/터치/자막/11개 위젯 또는 ShooterCore 요구를 삭제하는 대안은 승인하지 않았다. QuickBar를 먼저 독립 준비하는 것은 순서 분리이며 전체 HUD/슬롯·Experience 연결 의무를 없애지 않는다. 현 Controller Reticle 표시/기존 HUDClass/다른 맵은 다음 묶음에서도 유지하고, 최종 슬롯 전환은 검증 후 별도 승인한다. **전체 R7은 미완료**다.

---

## 0.34 이식·진단 이력 — 2026-10-10 QuickBar 에셋66개 / LyraActionWidget 부모 준비 제안

- 사용자가 0.33의 QuickBar3위젯과 필수 의존성 묶음을 승인했다. 원본의 최신 참조 metadata와10/8 조사 closure66개가 동일함을 확인하고, 기존 에디터가 없는 상태에서 작업했다. `Scripts/Editor/r7_quickbar_setup.py`와 `r7_quickbar_target.py`의 한정 레시피를 추가했으며 범용 C++ 도구/게임코드/Config는 변경하지 않았다. 빌드·패키징은 실행하지 않았다.
- **원본 준비/마이그레이션 완료:**18:26:06 `prepare_A`는 엔진 메모리 Advanced Copy→원본5BP의 상태/실제Graph.Nodes 타입/그래프/WidgetTree 비교→새66개 저장까지 통과했다.18:27:38 `migrate_A`는 별도 엔진 프로세스로 정확히66개만 DreamCatcher에 Migrate했다. 주요 목적지는 `/Game/LyraMigration/UI/R7/Equipment/{W_QuickBar,W_QuickBarSlot,W_WeaponAmmoAndName}`이며 나머지는 해당 루트의 Input/Actions 및 LyraStarterGame 하위다. 원본/동반 파일과 기존 활성 경로 등344경로가 불변이고, 대상66개는 준비된 원본 사본66개와 파일 해시가 일치한다. 기존 목적지를 덮어쓰지 않았다.
- 준비 과정 차이: 엔진 Advanced Copy가 `W_WeaponAmmoAndName.EventGraph.K2Node_CallFunction_7.Action`의 Reload literal1개를 그대로 남겨, 새 사본의 입력 에셋 경로로만 대응했다. 노드/연결/값의 다른 차이는 허용하지 않았다. 원본/사본 조회에 섞인 연결 없는 컴파일 임시 BreakStruct2개는 엔진 GC 후 사라졌고 실제 원본 그래프·타입·상태는 보존됐다. memory_A는 Python 비노출 설정타입 조회, B/C는 이 임시노드, D/E는 Reload literal 차이에서 저장 전 중단했다. 이 초기 실패들을 실제 이식 완료로 계산하지 않는다.
- **대상 로드 실패/런타임 미완료:**18:29:58~18:30:00 `R7Quick-target_A.log`에서 공용 `InputActionWidget`의 부모 `/Script/LyraGame.LyraActionWidget`을 찾지 못했다. 이어 W_QuickBar의 입력아이콘 변수/GUID ensure가 발생했고 저장 전에 중단했다. 현재 새66개는 이식 직후 그대로이며 대상 타입 변환 저장·fresh재로드·PIE는 아직 성공하지 않았다. 기존 UI 활성 경로는 그대로라 이 새 묶음은 연결되지 않았다. 부모 코드 준비 전 새 QuickBar BP를 열어 저장하지 않는다.
- 원본 `UI/Foundation/LyraActionWidget.h/.cpp`를 읽어 `UCommonActionWidget` 자식이며 `AssociatedInputAction`, EnhancedInput의 실제 할당 키 조회→CommonInput의 아이콘 조회→원본 Super fallback인2메서드를 확인했다. 대상에는 이 클래스가 없다. UCommonActionWidget로 단순 재부모화하면 AssociatedInputAction 기반 재바인딩 키 표시가 빠지므로 대안으로 선택하지 않는다. 기존 CommonUI/CommonInput/EnhancedInput 모듈은 있어 Build.cs 추가는 필요하지 않다.
- **다음 수정안(미적용):** 원본2파일을 `Source/DreamCatcher/UI/Foundation/DCLyraActionWidget.h/.cpp`로 이름/include/module대응만 하여 이식하고, `DefaultEngine.ini`에 해당 ClassRedirect1개와 이미 구현된 QuickBar SlotsChanged/ActiveIndexChanged의 StructRedirect2개를 추가한다. 원본과 DC 메시지 필드 Owner/Slots 및 Owner/ActiveIndex는 같은 계약이다. 이번 대상 준비 시도는2Struct를 process-only로 대응했지만 부모 누락에서 멈췄으므로 변환 성공으로 보고하지 않는다. 영구 Config3행 추가는 별도 승인 후 진행한다. 새 입력 규칙/아이콘 연출/게임 C++ 로직을 만들지 않는다.
- 승인→코드/Redirect만 적용→사용자 빌드→부모 사전 검사→대상 새5BP만 타입/Reload 참조 대응·저장→override없는재로드→실제 QuickBar 데이터/PIE 순서로 재개한다. Reload UI는 실제 사용 중인 `/Game/LyraMigration/BaseInput/Actions/IA_Weapon_Reload`에 연결할 계획이나 **아직 미적용**이다. QuickSlot 키/클릭·시각/장치·최종HUD슬롯은 미검증이며 전체 R7은 미완료다. Root의 사용자 실제 포커스 확인도 이전 답변대로 아직이다. 불필요한 추가 프레임워크/검사 도구 확장으로 우회하지 않는다.

---

## 0.35 코드 적용 이력 — 2026-10-10 원본 ActionWidget·Redirect / 사용자 빌드 대기

- 사용자가 0.34의 원본 부모 준비안을 승인했다. `Source/DreamCatcher/UI/Foundation/DCLyraActionWidget.h/.cpp`2개를 추가하고 `DefaultEngine.ini`에 LyraActionWidget ClassRedirect1개와 QuickBar SlotsChanged/ActiveIndexChanged StructRedirect2개만 추가했다. 기존 Config 변경은 보존했다.
- 원본 `LyraActionWidget.h/.cpp`의 AssociatedInputAction·GetIcon·GetEnhancedInputSubsystem을 재사용했다. 실제 매핑 키→CommonInput 아이콘 조회, 바인딩 소유 LocalPlayer 우선 선택, Super fallback을 유지했다. 변경은 DC 클래스명/include/generated 파일명 및 모듈 export뿐이다. 부모를 단순 CommonActionWidget로 바꾸거나 새 아이콘/입력 규칙을 만들지 않았다.
- 정적 검사: 두 파일 모두 이름/모듈 표기·공백 정규화 후 원본 전체와 일치한다. CommonUI/EnhancedInput/CommonInput 모듈과 대상 메시지2개는 이미 있어 Build.cs/메시지 본문은 변경하지 않았다. Config의 이번3행 외 본문은 불변이며 QuickBar66개/Root/Host·테스트맵/Controller/DLL/기존코드/레시피 등83보호파일도 불변이다. UHT/C++ 빌드·패키징·Editor·PIE는 실행하지 않았다.
- **다음:** 사용자 에디터 종료 → Development Editor/Win64 빌드 → 새 DLL로 재시작 후 기존 `r7_quickbar_target.py`의 부모 사전 검사부터 재개한다. 새5BP의 타입·현재 Reload Action 참조 대응/저장, override없는 새 프로세스 재로드, 실제 Inventory·QuickBar UI 검사는 아직 미검증이다. 빌드 전 신규 QuickBar BP를 열어 저장하지 않는다. 현재 상태는 **R7 코드·설정 적용/사용자 빌드 대기**이며 QuickBar 런타임/키·클릭·장치/최종 HUD슬롯과 전체R7은 미완료다.

---

## 0.36 최신 재개 — 2026-10-10 QuickBar 원본 데이터 복원·독립 화면 검증 통과

- 사용자 빌드 확인:18:38:57 시작·20.34초 Succeeded, DLL18:39:17(4,393,984 bytes). ActionWidget 부모와 QuickBar 메시지 타입을 정상 로드했다. Codex는 이번에도 C++ 빌드·패키징을 실행하지 않았다.
- 기존 승인66개 묶음 안에서 원본 데이터와 대조하여 타입/Reload 참조 및 누락 참조를 복원했다. 대상BP5개의 그래프 비교 후 저장했고, 원본 패키지 참조가 남거나 대상 로드 중 None이 된 폰트·머티리얼·위젯 참조를 원본 값으로 연결했다. 블루프린트 그래프/색/크기/여백을 임의 재설계하지 않았다. 기존 공용 에셋·게임C++·Config·맵·활성HUD는 변경하지 않았다.
- 브러시/폰트/슬롯Action25항목 중 비어 있던17참조를 원본대로 복원했다. CompositeFont2개는 원본 FontFace·문화권/문자범위/스타일 구성을 그대로 대응했다. 머티리얼24개에 대해 원본 부모/파라미터·함수/커브/텍스처 참조와 함수 입력 연결을 비교·복원했고, 함수 복사본의 핀GUID 차이로 끊어지는 선은 원래 입력/출력 이름으로 연결한 뒤 검사했다. CycleQuickbar 입력 안내의 DataTable 행도 원본과 동일하게 복원했다. 원본과 다른 단순 기본 머티리얼/폰트로 대체하지 않았다.
- **19:50:42 재로드 통과:** `Saved/Diagnostics/R7QuickBar/verify_F.json`은5BP 그래프·타입/66패키지 참조/25스타일·폰트2/머티리얼24/입력안내1의 검사 및 저장0이다. `R7Quick-verify_F.log` complete/종료0. 이 보고서의 원본·대상·기존활성 경로674보호경로는 최종 PIE 후에도 불변이다.
- **19:52~19:53 독립 PIE 통과:** 임시 QuickBar의3슬롯이 실제 Inventory/QuickBar와 연결돼30/60→27/60→30/57, 총탄약90→87을 표시했다. 컴포넌트 API로 빈슬롯/재장착을 바꾸어 UI 선택/빈상태/메시지 갱신을 확인했고 UnPossess/Pawn교체 때 새데이터를 표시했다. 기존Root1개/4레이어·ReticleHost 수명도 회귀 통과했다. QuickBar 자체는 검사 스크립트가 생성·종료 때 제거한 것이며 정식HUD 생성/제거 완료가 아니다. 사망 직후 장비가 남는 구간의 QuickBar 항목을 강제로 지우지 않았다.
- 근거: `Saved/Logs/R7Quick-PIE_E.log`, `Saved/Diagnostics/R7QuickBar/PIE-observations.json`, `Saved/Automation/R7QuickBar_20261010_E/index.json`19:53:18 **성공(경고포함)1/실패0/경고1**. Error/Fatal/ensure0이며 유일한 보고서 경고는 신규 캡처의 기준이미지 없음이다. `.../imageCompare/56057345/L_DC_RifleIntegration/R7QuickBar_20261010_initial/Windows/D3D12_SM6/Incoming.png`에서 숫자/총기아이콘/선택강조/EMPTY2칸을 확인했다. 왼쪽 배치는 검사용이고 원본과의 픽셀 동등성·최종배치 검증은 아니다. 기존 맵의 중복 방향광 경고는 남아 있다. Cue비활성/무음이라 FX·소리 재검증도 아니다.
- 원본 확인에 따른 예외: `W_WeaponAmmoAndName.WeaponNameWidget`은 원본부터Collapsed이며 `K2Node_CallFunction_6` SetText 실행선이 없다. 이름을 표시하는 테스트 기대가 잘못됐던 것이므로 원본대로 숨김을 유지했다. 빈 슬롯의 IsSelected는 원본 애니메이션 캐시라 남을 수 있고 실제선택은 원본 IsThisTheActiveQuickbarSlot로 확인했다. 임의 이름/선택 동작을 추가하지 않았다.
- 실패 이력: target_B 타입표시명 비교, target_C/verify_A~C 잔류참조, hard_references_A/B 함수선 손실, PIE_A 빈브러시 오류, PIE_B/C 잘못된 이름 표시 기대를 각각 구분했다. PIE_D는 데이터만 통과했으나 글리프/안내 등 경고85건과 깨진 화면이 있어 완료로 취급하지 않았다. 원본 데이터 복원 뒤 위verify_F/PIE_E가 최신이다. 레지스트리의 잔류참조가 없다는 것만으로 None으로 사라진 필드까지 보존됐다고 판정하지 않는다.
- **현재/다음:** QuickBar 이식본의 독립 데이터·화면 검사 묶음은 통과했다. 아직 테스트 스크립트의 임시 표시이며 기존 Controller Reticle 경로는 그대로다. 실제1~3/휠/클릭 입력, 최종HUD슬롯·Experience 연결·복제/장치·사용자포커스 확인은 미완료다. 읽기전용 조사에서 원본 `/ShooterCore/Game/GA_QuickbarSlots`가 OnSpawn/Independent/InstancedPerActor/LocalPredicted로3개의 `InputTag.Ability.Quickslot.{CycleForward,SelectSlot,CycleBackward}` GameplayEvent를 기다리고 SetActiveSlotIndex/순환API를 호출함을 확인했다. B_QuickBarComponent의 조회한 이벤트 그래프에는 실행 연결이 없다. **다음은 이 원본 입력 발신부까지 확인하여 키·클릭 연결의 정확한 변경 묶음을 제시하는 단계**이며 아직 GA/입력/AbilitySet/Config를 추가 변경하지 않았다. 전체 R7은 미완료다.

---

## 0.37 최신 재개 — 2026-10-10 QuickSlot 원본 발신부 확인 / 적용 승인 전

- 읽기 전용 근거: `Saved/Diagnostics/R7QuickBar/sender_read_B.json`(20:09:22 KST), `input_target_read_B.json`(20:19:00 KST). 원본40/대상48 파일·동반 경로가 유지됐고 두 실행 모두 종료0, 명시적 에셋 저장/Blueprint Compile0, Error/Fatal/ensure0이다. 기존 원본 중복Cue·대상 애니메이션/이식 참조 경고는 별개이며 무경고 실행이나 PIE 성공으로 보고하지 않는다. 초기 조회A의 경로 범위/객체 경로 API 오류는 후속B와 구분한다.
- 원본 발신자는 `/ShooterCore/Game/B_Hero_ShooterMannequin`이다. IA_QuickSlot1/2/3의 Pressed trigger → Triggered 핀 → `ChangeQuickbarSlot(0/1/2, SelectSlot)`이다. 함수는 Controller의 QuickBar 슬롯 Item 유효성을 검사하고 Pawn 자신에게 GameplayEvent를 보낸다. EventMagnitude는 0-based 인덱스다. 휠도 같은 함수에 CycleForward/Backward와 기본 인덱스0을 전달하므로 **원본 발신부의 슬롯0 유효성 조건**을 임의 제거하지 않는다. 수신 GA 이후 native 순환은 빈 슬롯을 건너뛴다.
- 원본 IMC_ShooterGame의 관련7행은 1/2/3, 휠위/아래, D-pad오른쪽/왼쪽이다. W_QuickBarSlot의 MouseButtonDown은 SlotInputAction에 값1을 InjectInputForAction하여 같은 입력 경로로 들어간다. 독립 검사에서 API로 빈 슬롯을 강제선택한 것은 실제 원본 입력 동작이 아니다.
- 실제 통합 Pawn에는 구형 디버그 바인딩 **1=EquipItem, 2=UnequipItem, 3=AddSpread**가 남아 있다. 새 QuickSlot과 중복될 수 있으므로 다음 승인 범위에 이 사본의 세 입력 바인딩만 해제를 포함한다. 4번 출력·원본 테스트 BP·공용 캐릭터는 그대로 둔다. 현재 매핑/AbilitySets에는 QuickSlot 연결이 없다.
- 다음 제안은 원본 GA_QuickbarSlots/순환Action2개 이식, 기존 슬롯Action3개 재사용, 원본7행을 분리한 신규 IMC, 전용 AbilitySet을 통합 PawnData에 추가, 원본 발신 노드5개와 ChangeQuickbarSlot 함수의 통합 Pawn 이식, 원본 Quickslot 태그3개 등록이다. 원본 Hero 전체 교체는 초기 인벤토리·외형·권총 등 다른 의존성을 동반하므로 지금은 입력 부분만 옮기는 안을 제시한다. 전체 Hero 재사용 불가 판정이 아니다. 기존 도구를 통한 부분 그래프의 무손실 자동 이식은 아직 미검증이며 새 사본에서 먼저 확인하고, 실패해도 임의 C++ 대체/범용 도구 확장은 하지 않는다.
- **아직 위 변경을 적용하지 않았다.** C++/Config/에셋/활성 HUD 변경·빌드·패키징 없이 조회 기록과 문서만 남겼다. 승인 후 재로드/노드·핀·기본값 비교, 실제 입력 이벤트→GA→장비 전환 및 반복/Pawn 교체 회귀를 확인한다. 최종 HUD 배치/수명·물리 마우스 포커스·게임패드/터치·복제는 미검증으로 유지하며 전체 R7은 진행 중이다.

---

## 0.38 최신 재개 — 2026-10-10 QuickSlot 승인 묶음의 메모리 이식 중단 / 방식 선택 필요

- 사용자가 0.37의 GA/Action/IMC/AbilitySet/통합 Pawn/태그 묶음을 승인했다. 원본 노드 그대로의 부분 그래프 이식을 먼저 시험했으나 **아직 적용 완료가 아니다**.
- `Saved/Logs/R7Slot-memory_A.log`20:28:14에서 원본 Hero의 저장하지 않은 엔진 복사본을 만들고 입력 이벤트5개+호출5개만 남긴 뒤, `Blueprint.FunctionGraphs` 조회가 **protected and cannot be read**로 거부됐다. 목적 Pawn으로 그래프 이전/Compile 비교/저장에는 도달하지 않았다. 종료-1이며 테스트 성공으로 기록하지 않는다.
- 원본 엔진 `Source/Editor/UnrealEd/Public/EdGraphUtilities.h`의 CloneGraph(77), ExportNodesToText(110), ImportNodesFromText(119)는 일반 C++ static 함수이지 Python UFUNCTION이 아니다. 현재 노출 API에서 이 직접 이동 방식이 막힌 근거이며, 원본 이식 자체가 불가능하다는 판정은 아니다. 보호 속성 우회·바이너리 편집·임의 native 게임 로직 재작성은 하지 않았다.
- 프로세스 종료 후 기존 원본40/대상48 파일·동반 경로를 대조해 **88개 불변**을 확인했고 원본/대상 모두 QuickSlot_memory_A 에셋 폴더가 생기지 않았다. 게임 C++/Config/에셋/활성 경로·빌드·패키징 변경은 없다. 실패한 스크립트는 실행을 차단한 진단 기록 `Saved/Diagnostics/R7QuickBar/quickslot_transfer_memory_attempt.py`로 보존했으며 Scripts/Editor의 실행 단계로 남기지 않았다.
- 다음 선택지: (A) 원본 함수1개와 입력10노드를 Editor에서 수동 복사하고 나머지 승인 설정은 기존 자동화로 처리 — 신규 C++ 빌드가 필요 없고 원본 그래프를 유지한다. (B) 엔진 CloneGraph/노드 복사 API를 호출하는 **이번 QuickSlot 원본/목적지 한정 최소 Editor C++ 연결함수**를 별도 승인 후 추가 — 사용자 빌드가 필요하지만 게임 로직 native 대체 없이 자동화 가능성을 검증할 수 있다. 둘 다 타입/Action 경로 대응과 원본 조건 보존·재로드/PIE 검증이 필요하다. 새 범용 도구나 게임 로직 대체는 승인되지 않았다. 방식 선택 전 현재 플레이어 변경을 진행하지 않는다.

---

## 0.39 최신 재개 — 2026-10-10 QuickSlot 한정 C++ 연결함수 작성 / 사용자 빌드 대기

- 사용자가 0.38의 **B(최소 Editor C++ 연결함수)**를 승인했다. 기존 Editor 플러그인의 `DCRifleMigrationLibrary.h`에 `CopyQuickSlotInputGraphs`를 선언하고 `Private/DCQuickSlotGraphTransfer.cpp`에 구현했다. 게임 런타임 로직을 C++로 다시 작성하지 않았고 모듈 의존성도 추가하지 않았다.
- 기존 진단 Commandlet/SCC/자동저장 보호와 별도 `-DCR7QuickSlotTransfer`를 요구한다. 원본 Hero→고정 준비용 Pawn, 이식된 준비용 Pawn→통합 Pawn 또는 저장하지 않은 Preview의 정확한 경로만 허용한다. 원본 ChangeQuickbarSlot12노드와 입력 이벤트/호출10노드를 엔진 CloneGraph로 복사한다. 다른 입력 노드는 원본이 아니라 임시 복사본에서만 제거한다. 자기 BP/생성클래스/스켈레톤클래스 참조만 새 그래프 안에서 대응하고, 원본 슬롯 조건·태그·트리거·계산을 대체하지 않는다.
- 기존 함수/입력 그래프·QuickSlot 바인딩·dirty 기존 대상·저장된 준비용 목적지·선택 범위 밖 연결은 거부한다. 함수 안에서 에셋 Load/Save/명시적 Compile은 하지 않는다. 실패 시 해당 프로세스를 저장 없이 폐기해야 하며, true도 메모리 복사이지 이식 완료가 아니다.
- `Scripts/Editor/r7_quickslot_transfer_probe.py`는 사용자 빌드 후 원본 Lyra의 새 사본에서 복사→Blueprint Compile→원본 노드/핀/타입/기본값 비교→중복 호출 거부→원본/기존 파일 보호를 확인한다. 이 스크립트는 에셋을 저장하지 않는다. 이전 Python 직접 이전 실패 기록은 Saved에 보존했다.
- **실행한 검증:** Python AST, 저장/마이그레이션/삭제 API 미호출 정적 확인, 기존 원본 조회 JSON의12+10노드 및 연결 범위 대조, git diff --check 통과. **UHT/C++ 빌드·새 native API 실행·실제 그래프 Compile/재로드·PIE는 아직 미검증**이다. Codex는 빌드/패키징/Editor 실행을 하지 않았고 Config/바이너리 에셋/활성 입력도 바꾸지 않았다.
- 다음은 사용자 Development Editor/Win64 빌드 성공 확인→새 플러그인을 원본 Lyra의 별도 프로세스에 로드하여 위 메모리 검증→기존 승인된 GA/Action/매핑/AbilitySet/태그·통합 Pawn 연결 묶음을 계속한다. 새 입력과1~3 디버그 바인딩 해제는 아직 적용하지 않았다. 전체 R7은 진행 중이다.

---

## 0.40 최신 재개 — 2026-10-10 QuickSlot 사용자 빌드 성공 / 실행 오류 교정 후 재빌드 대기

- 사용자 빌드는 UBT **20:38:34 시작,16.22초 Succeeded**, 새 연결함수CPP 컴파일과 플러그인DLL **20:38:50(636,928 bytes)** 갱신으로 확인했다. Codex는 빌드하지 않았다.
- 별도 원본 Lyra 프로세스의 `Saved/Logs/R7Slot-bridge_A.log`에서 **20:39:50 abstract UObject 생성 ensure →20:39:57 access violation/종료3**가 발생했다. 새 API는 로드됐으나 복사/Compile 검증은 실패다. 원인은 Codex가 만든 연결함수의 `NewObject<UObject>` 임시 소유자 생성이다. 이어 K2 CallFunction.PostDuplicate의 자기 참조 처리에는 정상 Blueprint 문맥이 필요함을 엔진 소스에서 확인했다. 원본 에셋 결함이나 원본 이식 불가로 취급하지 않는다.
- 실패 뒤 원본40/대상48 파일·동반 경로88개가 기존 해시와 일치했고 준비용BP 파일은 없다. 기존 플레이어·키·Config·에셋·빌드/패키징 변경은 없다. 네이티브 오류로 Python finally까지 실행되지 않아 이번 실패의 근거는 로그와 종료 후 해시 대조이며 성공 JSON은 없다.
- 승인된 연결함수 내부만 교정했다: 엔진 CreateBlueprint로 원본 타입의 임시 자식BP/스켈레톤을 만들고, 임시 소유 그래프만 정리한 뒤 복사한다. 각 생성/정리 경계에서 diagnostics를 확인한다. K2 PostDuplicate가 새 함수의 스켈레톤 준비 전 FunctionReference를 바꿀 수 있어 복사된 호출 노드의 해당 필드를 원본에서 복원한 후 기존 자기 타입 대응을 적용한다. 원본 게임 로직은 그대로다. Header 주석을 실제 임시BP 초기화에 맞추고 검사 스크립트에 native 호출 전 기록을 추가했다.
- 교정본의 Python AST/정적 API 대조와 diff 검사만 수행했다. **교정 C++ 재빌드·실제 복사·Blueprint Compile 비교·저장/재로드·PIE는 아직 미검증**이다. 다음은 사용자 재빌드→새 프로세스 메모리 검사 재시도다. 실패한 이전 DLL로 반복 실행하지 않는다. 입력 묶음과 전체R7은 미완료다.

---

## 0.41 최신 재개 — 2026-10-10 QuickSlot 원본 그래프 메모리 검증 통과 / Action 참조 보완 빌드 대기

- 사용자 재빌드는 **20:44:46 시작·14.94초 Succeeded**, 플러그인DLL **20:45:00(639,488 bytes)** 갱신으로 확인했다. 이전 abstract UObject/잘못된 소유자 충돌은 이후 재현되지 않았다. Codex는 빌드·패키징을 실행하지 않았다.
- **20:53:12 `R7Slot-bridge_G.log`/`slot_bridge_G.json` complete·종료0:** 원본 함수12노드+입력10노드의 복사, Blueprint Compile, 원본 노드/핀/타입/기본값 비교, 중복 적용 거부/대상 불변, 실제 원본 그래프 불변, 보호 파일 불변을 통과했다. 저장0/준비용파일 미생성이다. 전체 입력 또는 PIE 성공은 아니다.
- 비교 전 교정한 실행 이력: bridge_B는 bool+out의 Python 반환이 tuple이 아니라 성공시str/실패시None인 점, C/D는 CloneGraph 결과가 EdGraph_2/3 자동 이름이라 함수 등록이 안 된 점을 드러냈다. 기존 엔진 RenameGraph로 사본의 이름/등록을 복원하고 비교했다. E/F의 원본 차이는 OnTeamOrCosmeticsChanged의 MakeStruct 임시객체1개가 GC로 사라진 것이며 **실제 Graph.Nodes에 없고 연결도 없음**을 공개 API로 확인했다. 이를 제외한 등록 노드 집합을 대조했으며 삭제된 실제 원본 노드를 무시한 것이 아니다. 기존 원본 중복Cue 등 경고와 별개로 G의 Error/ensure/Fatal은0이다.
- 승인된 후속 준비를 `Scripts/Editor/r7_quickslot_assets.py`로 시작했으나 **20:55:51 prepare_A는 K2Node_EnhancedInputAction.InputAction의 Python protected 접근에서 저장 전에 중단**됐다. 원본 입력 그래프/GA/순환Action의 메모리 사본만 만들었고 에셋 저장0, 보호파일 불변이다. 보이는 출력핀만 바꾸면 실제 키 바인딩은 안 바뀔 수 있어 그 우회는 하지 않았다.
- 기존 한정 Editor 연결함수에 `RetargetQuickSlotInputActions`를 추가했다. 고정 준비용BP/QuickSlotInput의 원본 이벤트5개와 정확한 원본→이식 Action 경로5쌍만 허용한다. 이미 로드한 Action의 클래스/ValueType을 검사하고 해당 새 그래프 안에서 protected 바인딩과 출력핀 참조를 함께 치환·확인한다. Load/Compile/Save나 게임 규칙 대체는 없다. 새 범용 복사기는 아니다.
- **새 참조 보완 C++는 아직 빌드/실행 미검증**이다. Python2개 AST와 정적검사만 통과했다. Config/기존 에셋/1~3바인딩·활성 HUD는 그대로다. 다음은 사용자 빌드→`r7_quickslot_assets.py`의 source prepare(새 run=prepare_B)→성공 receipt를 명시한 verify/migrate→대상 준비/통합 Pawn 연결이다. 실패한 prepare_A를 baseline으로 쓰지 않는다. R7은 여전히 진행 중이다.

---

## 0.42 최신 재개 — 2026-10-10 QuickSlot 실제 연결·재로드·PIE 통과 / 정식 HUD는 다음 범위

- 사용자 빌드 **21:03:12 시작·15.78초 Succeeded**, 플러그인DLL **21:03:27(649,216 bytes)**를 확인했다. 이번에는 C++를 추가 수정하거나 Codex 빌드·패키징을 실행하지 않았다.
- **21:04 prepare_B:** 원본 그래프/실제 Action 바인딩·출력핀 비교 후 새 준비용BP/GA/순환Action2개, 총4개를 저장했다. **21:08 migrate_C:** 별도 재로드/Compile/참조검사 후 정확한4개만 DreamCatcher로 이식했다. verify_D에서 원본 매핑7행과 AbilitySet의 GA 부여값(Level1, InputTag.Ability.Quickslot)도 읽었다. 기존 원본/공용 Action3개는 보존했다.
- 대상 **21:16 target_prepare_C**에서 새BP2개의 native Redirect·원본 그래프/정책을 확인하고 저장했다. 원본 GA의 `Ability.Type.Passive.ChangeQuickbarSlot` 누락을 실제 로드 경고/원본Config로 확인한 뒤 사용자가 추가 등록을 승인했다. 입력 이벤트 태그3개와 합쳐 Config에 원본 태그4개를 등록했다. 원본 태그를 지우지 않았다.
- **21:25 preview_D**에서 메모리 사본 검증, **21:28 apply_A**에서 통합 `BP_DC_RiflePawn`/`DA_DC_RiflePawn` 및 신규 `IMC_DC_QuickSlots`/`AS_DC_QuickSlots`4개 저장, **21:31 verify_A**에서 새 프로세스 재로드/원본 노드·핀·기본값/매핑7행·트리거/부여항목/기존 매핑 보존을 통과했다. 통합 사본의 구형1=장착/2=해제/3=퍼짐 디버그 바인딩만 해제했고 키4·공용 캐릭터/원본 테스트BP·맵·Controller/HUD는 변경하지 않았다.
- **21:37:58 PIE 최종 통과:** `Saved/Automation/R7QuickSlotInput_20261010_B/index.json` 성공1/실패0/경고0, `R7Slot-PIE_B.log` Error/Fatal/ensure0. 실제 Pawn에서 EnhancedInput Action→이식 원본 발신 그래프→원본GA→QuickBar/Equipment 경로로 선택·빈슬롯 거부·정/역순환·원본 Widget 클릭 핸들러·반복을 확인했다. Pawn 교체 전후7개씩14관찰과 기존 발사30→27/재장전30·조준·Root/ReticleHost 수명·사망 정리 회귀가 통과했다. 두 번째 라이플과 QuickBar 위젯은 PIE에만 임시 생성/제거했고 지급 정책/정식HUD로 저장하지 않았다. 검사 후 보호86경로 불변이다.
- 실패 이력은 분리한다: migrate_B는 Commandlet에서 늦게 적용된 CVar 설정 때문에 복사 전 중단(후속C의 프로세스 ini override로 해결), target_prepare_A/B는 누락태그/타입 표시명 비교, preview_A~C는 삭제한 입력선의 기대값과 미리보기 복사본의 현지화ID 차이였다. 실제 출력문구는 같고 실제 Pawn 적용에서는 기존 텍스트ID도 보존했다. PIE_A는 휠을 키처럼 여러 프레임 주입한 검사 오류로 실패했으며, 원본 wheel Action은 Pressed trigger가 없으므로1프레임 펄스로 교정한 B가 최신이다. 게임 순환 규칙은 수정하지 않았다.
- **현재 완료 범위:** 승인된 QuickSlot 입력/에셋 연결과 위 재로드·PIE 경로. **미검증:** 물리키/OS 마우스 포커스·실기기 게임패드/터치·네트워크 복제·패키징. 원본 클릭 함수 직접 호출은 실제 화면 클릭/포커스 완료가 아니다. 현재 기본 지급은 라이플1개여서 빈2/3슬롯과 순환으로 변화가 없는 것이 정상이다. 기존 원본의 슬롯0 유효성 조건도 그대로다.
- **다음:** R7 정식 HUD 경로의 QuickBar→HUD.Slot.Equipment 연결 범위를 제시한다. 원본 LAS_ShooterGame_StandardHUD는 Layout1/Widget11개와 ShooterCore 의존성이 있으므로 QuickBar만 영구 AddToPlayerScreen으로 바꾼 것을 원본 전체HUD 완료로 취급하거나 나머지를 삭제하지 않는다. 정확한 원본 의존성/변경안 확인 뒤 승인받고 진행한다. 이번에는 최종 HUD·Experience 경로를 바꾸지 않았으며 전체 R7은 진행 중이다.

---

## 0.43 이전 제안 — 2026-10-10 정식 HUD 의존성 확인 / 순서는 아래 0.44에서 승인

- 이번 요청은 다음 변경안 확인으로 처리했다. 게임 C++/Config/에셋/활성 경로·빌드·패키징 변경은 없다. `Saved/Diagnostics/R7QuickBar/hud_route_current.json` 및 `R7HUD-RouteRead.log`22:08:42 읽기 전용 조회는 종료0/저장0/명시적Compile0이며 조회 대상 보호 파일도 불변이다.
- 실제 통합 GameMode의 HUDClass는 `/Script/Engine.HUD`, GameStateClass는 DCLyraGameState다. Controller의 WeaponReticleHostClass는 이미 이식한 Host이고 HUDWidgetClass는None이다. GameMode 코드도 아직 Experience 선택 대신 PawnData를 직접 공급한다. 이미 이식된 DCGameFeatureAction_AddWidgets는 DCLyraHUD의 receiver/ready 이벤트를 대상으로 하므로 클래스 지정만과 Action/Experience 활성화를 따로 확인해야 한다.
- 원본 StandardHUD/Layout 에셋2개의 현재 해시가 10/8 읽기 전용 감사와 일치한다. 원본은 Layout1/Widget11개·QuickBar→HUD.Slot.Equipment·Reticle→HUD.Slot.Reticle이고 ShooterCore 활성화를 요구한다. HUDLayout은 GameAndMenu/CapturePermanently 및 원본 Escape/장치연결 메뉴 참조를 가진다. 기존 감사의 참조 폐쇄를 다시 계산하면 Layout484개/StandardHUD541개(Engine/Script 제외, 이미 있는 파일 포함)이며 새 복사 대상 수로 해석하지 않는다.
- 원본 LyraSettingScreen은 GameSettings의 GameSettingScreen과 LyraGameSettingRegistry에 직접 의존한다. 대상에는 GameSettings 플러그인/대응 SettingScreen·Registry가 없고, Registry는 입력/비디오/오디오 외 Replay/DLC 분기도 참조한다. 전체 원본 HUD/메뉴까지 보존하는 연결은 R13 성격의 의존성 및 Experience 연동을 동반한다. SettingsLocal/Shared와 AddWidgets/HUD/HUDLayout 기반은 이미 있어 재작성 대상이 아니다. 전체 로드·활성화·실제 메뉴 작동은 아직 미검증이며 이식 불가라고 판정하지 않았다.
- **사용자 결정 전 제안:** 실제 전투 작업 우선 원칙에 맞춰, R7의 남은 정식 HUD/메뉴/Experience를 R13 통합 묶음에 미완료로 보존하고 R8 애니메이션 현황 점검/변경안으로 이동하는 안을 권고한다. 대안은 지금 R7+필수 R13 의존성을 함께 처리하는 것이다. 어느 안도 메뉴/장치/Widget 항목 삭제, 가짜 빈 클래스, ShooterCore flag 제거로 범위를 축소하지 않는다. 전체 완료 목표/10월16일 범위는 유지한다. **아직 순서를 변경하거나 R8 구현을 시작하지 않았고 R7을 완료로 표시하지 않는다.**

---

## 0.44 이전 인계 — 2026-10-10 R7 잔여 HUD를 R13과 묶는 순서 승인 / R8 변경안 검토

- 사용자가 **1번안: R7의 남은 HUD 작업을 미완료로 남겨 R13과 묶고 R8 변경안부터 확인**하는 순서를 승인했다. 0.43의 순서 선택 대기는 해소됐지만 R8 게임 코드·Config·에셋 수정 승인은 아니다. 이번에는 읽기 전용 소스/파일/기존 검증 기록 확인과 이 상태 문서만 갱신했다. C++ 빌드·패키징·Unreal 실행·에셋 저장은 하지 않았다.
- **R7은 부분 완료/잔여 미완료**다. 검증된 Reticle·탄약·명중 표시·Root 기반·QuickSlot 연결을 유지하고, StandardHUD/Layout 및 QuickBar 정식 HUD.Slot.Equipment 연결, 원본 나머지 위젯·메뉴·장치 화면과 Experience/AddWidgets 수명을 R13 통합 범위에 남긴다. 항목 삭제나 전체 R7 완료 처리는 하지 않는다. 물리 입력/포커스·게임패드/터치·복제·패키징의 기존 미검증도 유지한다. 10월16일 전체 목표를 축소하지 않는다.
- **R8 현재 근거:** 원본 LyraAnimInstance/CharacterMovementComponent h/cpp와 대상 DCAnimInstance/DCCharacterMovementComponent h/cpp 4개를 재대조했다. 클래스·include·모듈 export·CVar/stat·에디터 Category 이름을 대응하면 일치한다. 새 기반 C++를 중복 이식하지 않는다. 기본 AnimBP, Item Layer 인터페이스/기반, Rifle/Feminine/Unarmed Layer, 재장전 Montage2개, AN_Reload 총9개와 이미 이식한 재장전 Ability3개가 대상에 있다. 기존 단일 PIE의 발사/재장전 성공과 전체 애니메이션 완료는 구분한다.
- 위 애니메이션 관련9개는 현재 원본과 바이너리 해시가 다르다. 저장/이식에 따른 차이와 그래프/설정 손상을 구분할 수 없는 증거이므로 **현재 의미적 동일성은 미검증**이다. 무조건 재복사하거나 손상으로 판정하지 않는다. 원본 LyraCharacter와 현재 DreamCatcherCharacter의 native 이동/캡슐 기본값도 다르지만, 실제 BP override/화면 영향 확인 전에는 공용 캐릭터 값을 원본으로 덮어쓰지 않는다.
- **다음 승인 요청 묶음:** 기존 원본/대상 AnimBP·Layer·재장전 에셋과 통합 BP_DC_RiflePawn의 실제 연결/설정 비교 → 기존 PIE 절차를 재사용한 이동/정지/전환/점프·낙하/웅크림/조준·발사, 장착·해제·교체의 Layer 수명 및 재장전 완료·취소·사망·교체 탄약/태그 관찰 → 사용자 화면·조작감 확인이다. 새 범용 도구는 만들지 않는다. 현재 확정된 새 게임 C++/Config 수정은 없으며, 실제 불일치가 확인되면 정확한 대상/원본 근거/제약/보존 대안/동작 차이를 먼저 제시하고 수정 승인을 받는다. 원본 재장전 규칙 변경, 대량 Retarget/재이식, 공용 메시·디자이너 맵 변경, R7/R13 HUD 활성화는 이 묶음에 포함하지 않는다.

---

## 0.45 이전 인계 — 2026-10-10 R8 첫 읽기 전용 비교·PIE 관찰 완료 / 아래 이동값 교정 승인·적용

- 사용자 승인 범위는 **기존 에셋 비교·실제 동작 검증, 수정 필요 시 변경안 먼저**다. 게임 C++/Config/에셋을 수정하지 않았고 빌드·패키징도 실행하지 않았다. 기존 도구를 재사용한 한정 검사/보고서만 `Saved/Diagnostics/R8/`에 만들었다. R7 잔여 정식 HUD는 계속 미완료로 R13과 묶는다.
- 원본/대상 각15개 조회가 **22:35:35/39 KST** 완료(`R8-SourceRead_C.log`, `R8-TargetRead_C.log`, `source_C/report.json`, `target_C/report.json`)됐다. 기본 AnimBP/인터페이스/Item Layer 기반/Rifle·Feminine·Unarmed Layer의 기존 reader 노드·핀 기록 및 CDO가 일치한다. 다만 reader는 loaded nodes by outer이고 AnimGraph 비-K2 노드 설정 전체를 읽는 범용 증명은 아니다. 전체 T3D의 디버그 PoseWatch/객체 순서/컴파일 산출물 차이도 보존했으며 바이너리/포즈 완전 동일성으로 과장하지 않는다.
- WeaponInstance/재장전 GA는 등록 native 이름·표시명/기존 Hero 타입 대응과 현지화 텍스트ID 차이를 구분했다. 실제 재장전 Montage `/Game/Weapons/Rifle/Animations/AM_MM_Rifle_Reload`를 추가 추적해 **22:41:08/20** 양쪽 조회 성공, 길이2.2초·T3D·Notify5개 동일, reflected 값은 프로세스별 transient AnimDataController 이름만 다르다. Additive sequence는 주요 속성/길이2.2초/66프레임/67키가 같지만 editor 데이터 모델 표현 차이가 있어 전체 raw pose 동일성은 미검증이다. FootPlant Rig는 로드되지만 export 차이를 다 해석하지 않았고 기본 AnimBP의 EnableControlRig/UseFootPlacement는 양쪽 false다. IK 시각/전체 Rig 동등성을 완료 처리하지 않는다.
- **22:40:55 단일 PIE 관찰 완료:** `pie_B.json` 42구간/396관찰, `R8Observations_B/index.json` **22:41:40 성공1/실패·보고서경고0**, 최종 로그 Error/Fatal/ensure0. `evaluation.json`의 한정 후처리 조건20개가 통과했다(추가 기능20개를 구현했다는 뜻 아님). 이동/정지/좌우전환/점프·낙하·착지/웅크림, ADS·사격·재장전 태그→AnimBP, Rifle→Unarmed→Rifle 활성 Layer 전환, 수동 재장전26→30/예비60→56을 관찰했다. 자동 재장전 후 계속 사격하고 놓은 뒤 R은27→30/예비26→23으로 정상이다. 사격 취소는 원본처럼 재장전을 취소하며 조기 탄약 지급0, 재장전 도중 교체·사망에서는 양쪽 아이템 탄약 보존/태그·Montage 정리를 확인했다. 점프는 Character.Jump 호출이고 다른 입력은 EnhancedInput 주입이다. 실기기/복제/화면 포즈/사운드 재검증은 아니다.
- 검사 실패/교정 이력은 구분한다: read_A는 기존 스크립트 top-level 실행 때문에 에셋 조회 전 중단, read_B는 AnimBP 중첩 그래프에 부적합한 기존 orphan 필터로 중단했다. read_C는 원문을 보존하고 그 한계를 명시했다. PIE_A는 자동 재장전 완료 전에 버튼을 놓아 해당 과거 증상 조건이 불충분했고 B에서 실제 refill 후 재사격까지 기다렸다. 후처리의 순간 ammo30 샘플/프로세스별 transient controller 비교는 즉시 다음 발사와 인스턴스명 차이를 원자료로 확인한 뒤 조건을 교정했다. 게임 규칙을 바꿔 통과시킨 것이 아니다. 보호545경로 불변을 확인했다.
- **다음 미적용 변경안:** `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn`의 CharMoveComp override6개만 실제 원본 B_Hero_ShooterMannequin CDO 값으로 맞춘다: MaxAcceleration2048→1200, BrakingDecelerationWalking1800→1400, BrakingFriction0→3, BrakingFrictionFactor2→1, AirControl0.2→0.4, JumpZVelocity420→500. MaxWalkSpeed600/GravityScale1/CrouchedHalfHeight65는 유지한다. 원본 일치 목적이며 현재 애니메이션 고장이 재현됐다는 뜻은 아니다. 신규 C++/Config/공용 Character 변경은 제안하지 않는다. 승인 후 Editor 설정·저장·재로드/기존 PIE 회귀와 사용자 시각 확인 순서이며 이 범위만이면 C++ 재빌드는 필요 없다.
- 별도 제약: 원본은 SKM_Manny_Invis/메시Z=-88/캡슐반지름35·반높이90, 대상은 보이는 SKM_Manny/Z=-90/42·96이다. 보이는 파트 연결을 확인하지 않고 Invis로 바꾸면 표시가 사라질 수 있으므로 자동 교체하지 않는다. 대안은 기존 테스트 표시를 유지하며 이동값만 맞추거나 원본 CharacterParts·메시/캡슐·카메라를 함께 별도 검증/승인하는 것이다. 현재 첫 검증 묶음은 완료지만 전체 R8은 이 차이와 사용자 화면/IK 등 미검증을 남긴 진행 중이다.

---

## 0.46 이전 인계 — 2026-10-10 R8 통합 Pawn 이동값6개 원본화·재로드·PIE 관찰 통과

- 사용자가 이동값6개 교정안을 승인했다. `Scripts/Editor/r8_movement_defaults.py`로 **BP_DC_RiflePawn 한 에셋만** Unreal 내부에서 설정·Blueprint Compile·저장했다. MaxAcceleration1200, BrakingDecelerationWalking1400, BrakingFriction3, BrakingFrictionFactor1, AirControl0.4, JumpZVelocity500이다. C++/Config/공용 Character/디자이너 맵/재장전 규칙은 변경하지 않았고 C++ 빌드·패키징도 실행하지 않았다.
- **22:49:28 Apply_B 저장1개 성공**, **22:50:33 Verify_A 별도 프로세스 재로드·저장0 성공**. `Saved/Diagnostics/R8/movement_Apply_B/report.json`, `movement_Verify_A/report.json` 및 `R8Movement-Apply_B.log`, `R8Movement-Verify_A.log`를 근거로 한다. 승인6개 외 CDO 텍스트와 Blueprint 그래프가 동일하고, 별도 재로드에서도 저장 직후 CDO/그래프가 일치했다. 적용/재로드 동안 보호562경로 불변이다. Apply_A는 부모에서 상속한 기존값이 export에 생략되어 비교에서 중단(저장0)됐고, 실제 추가된6줄만 확인한 뒤 비교를 교정한 B가 성공본이다.
- **22:51:48 단일 PIE 관찰 완료:** `pie_Movement_A.json`42구간/396관찰 및 `evaluation_Movement_A.json` 한정 조건22개 통과/실패0. 런타임 이동값6개와 유지값600/1/65, visible SKM_Manny와 캡슐42/96, 이동/정지/방향전환/점프·낙하·착지/웅크림, ADS·사격·재장전 태그 전달, 장착·해제 Layer, 수동/자동 재장전 후R 및 교체·사망 정리가 회귀 통과했다. 이후 보호579경로(승인된 Pawn 저장 후 해시 포함)가 불변이다. 추가 구현22건이 아니며 기존 검사 재사용이다.
- **22:52:33 에디터 자동 검사 종료 확인:** `Saved/Automation/R8Movement_A/index.json` 성공1/실패·보고서경고0, `R8-PIE_Movement_A.log` Error/Fatal/ensure0 및 프로세스 종료0이다. 위 관찰 완료와 에디터 검사 최종 종료를 함께 확인했다.
- **사용자 다음 확인:** C++ 재빌드 없이 `L_DC_RifleIntegration`을 열고 이동 시작/멈춤·좌우전환, 점프/착지, 웅크림·조준 중 이동의 화면·조작감을 짧게 확인한다. 발 미끄러짐/자세 튐/손과 총의 어긋남이 없는지 본다. 자동 검사는 단일 로컬PIE·입력 주입(점프는 Character.Jump 호출)·Cue비활성/무음이므로 시각/사운드·실기기·네트워크·패키징을 새로 완료 처리하지 않는다.
- 전체 R8은 사용자 화면/전체 Rig·포즈 등 미검증과 기존 메시/캡슐 표시 구조 차이를 남긴 **진행 중**이다. 원본 Invis 메시/CharacterParts·캡슐·카메라를 임의로 교체하지 않았다. R7 잔여 HUD는 미완료로 R13과 묶는 결정을 유지한다. 추가 게임 변경은 다시 범위·근거·차이를 제시하고 승인받는다.

---

## 0.47 이전 인계 — 2026-10-10 SpaceBar 점프가 FoundationTest로 연결됨 / 아래 코드 범위 승인·적용

- 사용자는 SpaceBar가 점프 대신 디버그 로그만 출력한다고 보고했다. `Saved/Logs/DreamCatcher.log`22:56의 `GA_DC_FoundationTest_C_0: Foundation Input Pressed/Released`와 읽기 전용 `Saved/Diagnostics/R8/jump_target/report.json`22:59:41을 대조해 원인을 확인했다. **IMC_Player.SpaceBar→/Game/Input/Actions/IA_Jump→DA_DC_RifleInput의 InputTag.Jump→AS_DC_FoundationTest가 부여한 GA_DC_FoundationTest**다. 해당 GA는 PrintString/WaitInputRelease/EndAbility만 있고 실제 Jump 호출이 없다. Hero 원본 입력 경로에서는 Character의 legacy 직접 Jump 바인딩도 사용하지 않는다. 이동값6개 문제가 아니라 남은 테스트 Ability 연결 문제다.
- 이전 R8 검사의 점프는 명시적으로 Character.Jump 직접 호출이었다. 이동/공중 상태 검증 통과를 SpaceBar→Ability 점프 입력 성공으로 해석하지 않는다. **실제 입력 점프는 현재 실패/미완료**이며 사용자 화면 확인을 마감 조건에서 제거하지 않는다.
- 원본 `LyraGameplayAbility_Jump.h/.cpp`를 읽고 `jump_source/report.json`22:59:57에서 `/Game/Characters/Heroes/Abilities/GA_Hero_Jump` 부모/그래프/부여값을 확인했다. 원본은 CanJump 검사, LocalPredicted/InstancedPerActor, 시작 시 UnCrouch→Jump, 해제/종료 시 StopJumping을 처리하며 BP에 StartAbilityState/WaitInputRelease·UIExtension 등록/해제와 W_JumpTouchButton 참조가 있다. 대상에는 native Jump 클래스와 GA_Hero_Jump가 없다. 입력태그와 Ability.Type.Action.Jump는 이미 등록되어 있다. 양쪽 조회 저장0/명시적Compile0/보호파일 불변이며 게임 수정·빌드·패키징은 하지 않았다.
- **미적용 제안:** 원본 native Jump h/cpp2개를 기존 DCGameplayAbility/Character 타입에 최소 대응하고 정확한 ClassRedirect1개를 추가한다. 원본 GA_Hero_Jump와 W_JumpTouchButton 참조/필요 의존성을 보존해 이식한다(세부 참조 재사용/이식 목록은 후속 확인 대상이며 임의 삭제하지 않음). 통합 전용 AbilitySet을 기존 AS_DC_FoundationTest에서 만들고 **Jump 태그의 FoundationTest 항목만 원본 Jump로 교체**, 같은 세트의 GA_Hero_Death 및 기타 부여값은 보존한다. DA_DC_RiflePawn의 해당 세트 참조만 통합 전용 세트로 교체하고 공용 FoundationTest 에셋/다른 테스트 맵은 그대로 둔다. 단순 SpaceBar→Jump 직접 연결은 작지만 원본 CanActivate/Ability 수명/해제·터치 경로를 우회하므로 권고하지 않는다. C++ 적용 승인 후 빌드는 사용자 담당이며, 다음 검사는 Character.Jump를 직접 호출하지 않고 InputAction 경로부터 수행한다. 아직 이 변경 묶음은 승인 전이다.

---

## 0.48 이전 인계 — 2026-10-10 원본 Jump native 코드·Redirect 적용 / 아래 사용자 빌드 확인

- 사용자는 **원본 Jump 이식 코드부터 진행, 사망 Ability 유지, 빌드는 직접 수행**을 승인했다. `Source/DreamCatcher/AbilitySystem/Abilities/DCLyraGameplayAbility_Jump.h/.cpp`2개와 `Config/DefaultEngine.ini`의 정확한 ClassRedirect1개(`/Script/LyraGame.LyraGameplayAbility_Jump`→`/Script/DreamCatcher.DCLyraGameplayAbility_Jump`)만 게임 구현 범위에 반영했다. 문서도 상태를 갱신했다.
- 원본 h/cpp를 기존 UDCGameplayAbility/ADreamCatcherCharacter/GetDCCharacterFromActorInfo 및 파일/include 이름에 대응했다. 원본의 UCLASS(Abstract), InstancedPerActor/LocalPredicted, Avatar/CanJump/Super 활성 조건, 로컬 제어·bPressedJump 검사, UnCrouch→Jump와 입력 종료/취소 시 StopJumping을 그대로 보존했다. CharacterJumpStart/Stop의 BP 함수명과 Category도 유지했다. 새 ActivateAbility 본체/입력 직접 바인딩/별도 점프 규칙을 만들지 않았다.
- **정적 확인:** 이름/include/getter 대응 후 원본2파일과 텍스트 일치(줄끝 공백/개행 차이 제외), Redirect 정확히1개, 그1행을 제외한 Config의 정규화 해시 불변, whitespace 검사 통과. 사망 native2파일·GA_Hero_Death·AS_DC_FoundationTest·GA_DC_FoundationTest·통합 Pawn/PawnData/Input·Build.cs·게임DLL·Git index 등 보호11파일 불변이다. 원본/다른 사용자 변경도 건드리지 않았다.
- **아직 미검증:** UHT/C++ 빌드·native 클래스/Redirect 실제 로드·원본 BP 재부모화/이식·입력 점프. 이번에는 에디터/PIE/빌드/패키징을 실행하지 않았고 바이너리 에셋은 전혀 수정하지 않았다. 따라서 **지금 빌드만 해도 SpaceBar가 즉시 고쳐지는 것은 아니다**. 아직 기존 FoundationTest 연결이 남아 있다.
- **다음:** 사용자가 Development Editor/Win64 빌드 → 새 native 클래스/Redirect와 원본 BP 의존성 확인 → 승인된 후속 에셋 범위에서 GA_Hero_Jump/터치 참조 보존 이식 및 통합 전용 AbilitySet의 Jump 항목 교체 → 실제 입력 경로/회귀 검사다. 이번 요청은 코드 우선 승인이고 후속 에셋 수정 권한까지 확대하지 않는다. 같은 AbilitySet의 **GA_Hero_Death 부여는 반드시 보존**한다. R8 진행 중/실입력 점프 미완료, R7 잔여 HUD는 R13과 묶는 상태다.

---

## 0.49 이전 인계 — 2026-10-10 Jump 사용자 빌드·native 조회 완료 / 아래 에셋 승인 후 입력 불일치 발견

- 사용자 UBT 로그 **23:06:53 시작·24.41초 Succeeded**, `DCLyraGameplayAbility_Jump.cpp` 및 생성 모듈 컴파일·DLL 링크 성공을 확인했다. 게임DLL은 **23:07:16/4,400,640 bytes**다. 사용자가 Git index에 staged 변경을 추가한 상태를 보존했으며 Codex는 빌드·패키징·stage/commit을 실행하지 않았다.
- **23:13:10 `R8-JumpBuildTarget_B.log`/`Saved/Diagnostics/R8/jump_build_target_B/report.json`:** 새 native 클래스 로드, CharacterJumpStart/Stop 함수 반영, InstancedPerActor/LocalPredicted/OnInputTriggered/Independent 기본값, FoundationTest 및 GA_Hero_Death 부여 유지 확인. 종료0/저장0/명시적Compile0/ensure0/보호파일 불변이다. 원본 native 정책은 source 조회와 대응한다. **실제 Jump BP의 Redirect import/입력 동작은 아직 미검증**이다.
- 검사 이력: target_A에서 원본 `/Script/LyraGame.LyraGameplayAbility_Jump`를 Python load_class로 바로 로드하려다 실패했다. 기존 native 등록 검사는 FSoftObjectPath::FixupCoreRedirects 후 로드한다. B는 새 클래스 로드와 정확한 Redirect 설정1행만 확인하고 실제 원본 BP import는 미검증으로 분리했다. 이를 Redirect 런타임 성공으로 보고하거나 Config를 임의 수정하지 않았다.
- **23:10:29 source 읽기 전용 조회 완료:** GA_Hero_Jump/W_JumpTouchButton의 참조45개(Engine/Script 제외)를 확인했다. 새로 필요한 후보는 GA_Hero_Jump, W_JumpTouchButton, MI_UI_TouchCard_Jump, T_UI_Icon_MobileOnly_Jump4개다. 나머지41개는 기존 ADS/QuickBar/공용 에셋 경로에서 재사용 후보를 찾았지만 존재만으로 그래프/참조/시각 동등성을 확정하지 않는다. 특히 W_ActionTouchButton/MobileOnly는 ADS 이식본, 현재 IA_Jump는 실제 SpaceBar 바인딩 객체를 유지할 후보이며 적용 전에 비교한다. W_JumpTouchButton의 AssociatedAction과 ButtonBrush, Input.Touch 가시성 조건을 보존한다.
- **다음 제안(아직 미승인):** 신규 `/Game/LyraMigration/Jump/` 아래 위4개를 원본에서 이식하고41개 기존 후보는 비교 후 참조 재사용한다. 재사용 불일치가 있으면 기존 에셋을 덮어쓰지 말고 추가 변경안을 먼저 낸다. 통합 전용 `/Game/DreamCatcher/GAS/Test/R5/Integration/AS_DC_RifleJumpAndDeath`를 AS_DC_FoundationTest 기반으로 만들고 Jump의 GA_DC_FoundationTest만 원본 GA_Hero_Jump(Level1/InputTag.Jump)로 교체한다. **GA_Hero_Death/다른 부여값은 보존**하고 `DA_DC_RiflePawn.AbilitySets`의 FoundationTest 세트 참조1곳만 교체한다. 입력키/IMC/다른4세트/공용 FoundationTest/메시·카메라·원본 UI 규칙은 변경하지 않는다. 승인 후 저장·재로드/Compile·실제 IA_Jump 경로 검증이며, 현재 범위에서는 추가 C++ 수정/재빌드를 제안하지 않는다.
- 이번에는 진단/문서만 변경했다. **코드 기반 준비는 완료, 실사용 Jump 연결은 아직 미완료**라 SpaceBar의 Foundation 로그는 현재 정상적으로 남아 있는 이전 연결 결과다. R8 진행 중이며 R7 잔여 HUD/R13 및 장치/전체Rig 등 기존 미검증을 유지한다.

---

## 0.50 이전 인계 — 2026-10-11 Jump 재사용 검사에서 기존 IA_Jump 트리거 불일치 / 아래 추가 범위 승인·적용

- 사용자가 신규 원본4개·통합 전용 AbilitySet·PawnData1참조 연결 묶음을 승인했다. 조건은 기존41개 후보를 비교하고 불일치 시 추가 변경안을 먼저 제시하는 것이다. **실제 에셋 생성/저장 전에 입력 Action 불일치를 발견해 중단**했다. 이번 작업은 읽기 전용 조회/진단·문서뿐이며 코드/Config/에셋/빌드/패키징/Git index는 변경하지 않았다.
- `Saved/Diagnostics/R8/jump_reuse_source_A/report.json`, `jump_reuse_target_A/report.json`과 `R8-JumpReuseSource_A.log` **00:22:23 KST**, `R8-JumpReuseTarget_A.log` **00:22:13**에서 각41개를 조회했고 저장0/명시적Compile0/ensure0/보호파일 불변이다. 재사용 후보는 ADS/Common 및 QuickBar 중 실제 존재하는 경로다. 노드·핀/프로퍼티/소재 export를 보존했다. W_ActionTouchButton의 표시 타입 차이는 기존 LyraInteractionDurationMessage→DCInteractionDurationMessage Redirect 대응이다. 텍스처 Oodle SDK 메타데이터와 export 순서/GUID 차이 등도 남으므로 전체41개를 완전 동일로 보고하지 않는다.
- **확정 불일치:** 현재 `/Game/Input/Actions/IA_Jump`는 InputTriggerPressed+InputTriggerReleased를 가지며 PlayerMappableKeySettings=None이다. 원본 `/Game/Input/Actions/IA_Jump`는 Triggers0개 및 Name/DisplayName=Jump의 PlayerMappableKeySettings를 가진다. 원본형 DCInputComponent는 Triggered→AbilityPressed, Completed→AbilityReleased로 바인딩하므로 기존 Action을 그대로 재사용하면 원본 WaitInputRelease의 유지/해제 타이밍이 달라질 가능성이 있다. 아직 원본 GA 연결 후 런타임 오동작을 재현한 것은 아니며 구성 불일치와 그 영향을 구분한다.
- 대안으로 이미 있는 `/Game/LyraMigration/BaseInput/Actions/IA_Jump`를 **00:25:09** 읽기 전용 확인했다(`jump_existing_action/report.json`, `R8-JumpExistingAction.log`). Trigger0/Jump 키설정과 원본 export가 경로·Jump 표시문구의 현지화 namespace/key만 제외하면 같다. 이 기존 이식 Action을 수정 없이 사용하도록 권고한다. 물리 입력 동작은 연결 후 검증한다.
- **추가 미승인 범위3개:** 새 통합 `IMC_DC_RifleBase`를 공용 IMC_Player에서 복제해 **SpaceBar의 Jump Action 참조만 BaseInput/IA_Jump로 교체**, 나머지 매핑 유지. `DA_DC_RifleInput`의 InputTag.Jump Action 참조를 같은 객체로 교체. `BP_DC_RiflePawn.Hero.DefaultInputMappings`의 IMC_Player 참조1곳을 새 통합 IMC로 교체하되 우선순위/등록옵션/나머지 컨텍스트 유지. 앞서 승인된 W_JumpTouchButton의 AssociatedAction도 같은 원본 이식 Action에 연결한다. 공용 IMC_Player/기존 IA_Jump/다른 맵에는 손대지 않는다. 총 제안은 기존 승인 신규5개+PawnData1개에 새 IMC1개와 기존 Input/Pawn2개를 더한 **신규6개/기존3개**다.
- 공용 IA_Jump의 트리거를 직접 제거하는 대안은 작지만 다른 테스트/맵 입력에도 영향을 줄 수 있어 권고하지 않는다. 원본 트리거를 재사용하도록 연결하는 안은 키를 바꾸지 않고 원본 누름·유지·해제 의미를 보존하는 목적이다. GA_Hero_Death와 기타 부여값은 반드시 유지하며 사망/이동/카메라/재장전 코드·Config 추가 변경이나 C++ 재빌드는 이 제안에 필요하지 않다. 현재 SpaceBar는 여전히 FoundationTest이며 실제 Jump 연결/전체 R8은 미완료다. 승인 전 추가 입력 변경을 실행하지 않는다.

---

## 0.51 이전 인계 — 2026-10-11 원본 Jump/입력 에셋 연결 완료 / 아래 웅크림 코드 교정 승인·적용

- 사용자가 통합 전용 입력3개 추가를 승인했고, 진행 중 발견한 원본 태그 **HUD.Slot.RightSideTouchInputs 1개 등록도 명시적으로 추가 승인**했다. 대상 import에서 빈 태그가 됨을 `R8-JumpTagCheck.log`01:05:27에 확인한 뒤 DefaultGameplayTags.ini에 원본1행만 추가했다. 기존 태그/사망 부여는 유지했다. C++·빌드·패키징·Git index는 변경/실행하지 않았다.
- `Scripts/Editor/r8_jump_assets.py`로 신규 `/Game/LyraMigration/Jump/`의 GA_Hero_Jump/W_JumpTouchButton/MI_UI_TouchCard_Jump/T_UI_Icon_MobileOnly_Jump **4개**를 준비·이식했다. **01:07:28 Prepare_A 저장4, 01:09:42 Migrate_A 이식4, 01:10:34 Target_A 저장4, 01:12:21 Verify_A 별도 재로드/Compile·저장0**이 성공했다. Target_A의 PackageRedirect는 프로세스에만 주어 기존41개 경로를 재사용했으며 persistent Redirect 추가는 없다. Verify_A는 임시 패키지 별칭 없이 source graph/tree/default와 대응했고 **실제 GA 부모가 DCLyraGameplayAbility_Jump**로 로드됨을 확인했다. 기존 원본/공용 에셋 파일 불변이다.
- `Scripts/Editor/r8_jump_integration.py`의 **01:15:19 Apply_B 저장5**, **01:17:01 Verify_A 별도 재로드/저장0** 성공. 신규 AS_DC_RifleJumpAndDeath/IMC_DC_RifleBase2개와 기존 DA_DC_RiflePawn/DA_DC_RifleInput/BP_DC_RiflePawn3개를 승인 범위로 저장했다. Jump만 원본GA(Level1/InputTag.Jump)로 바꾸고 **GA_Hero_Death/다른4세트/원본 공용 FoundationTest 유지**. SpaceBar/터치가 BaseInput/IA_Jump를 공유하며 공용 IMC_Player/IA_Jump는 그대로다. 다른 매핑/옵션·Pawn 그래프·이동값/메시/캡슐/카메라도 유지했다. Apply_A는 EditDefaultsOnly 구조체 직접 set 제한으로 저장0 중단했고, 기존 방식의 새 struct import→배열 대입으로 교정한 B가 성공본이다.
- **01:21:52 실제 입력 PIE 관찰:** `pie_Jump_B.json`50구간/442관찰, Character.Jump 직접 호출 없이 IA_Jump→ASC→원본GA를 사용했다. 서서 점프·해제 후 재점프·공중 추가 점프 방지·누른 채 착지 후 자동 반복 없음·사망 차단은 통과했다. 기존 발사/수동·자동 재장전 후R/Layer/슬롯 교체/사망 회귀도 한정 관찰 조건 통과다. **웅크림 점프는 수직속도0·IsCrouched=true·CanJump=false로 실패**했다. `R8JumpInput_B/index.json` **01:22:32 PIE 실패1**은 이 실제 실패를 보고한 결과이며 전체 성공으로 표시하지 않는다. `evaluation_Jump_B.json` 조건26통과/1실패, 보호996경로 불변이다(검사 수는 구현 진척 아님).
- 검사 한계/교정: Jump_A는 엔진 보호 속성 bIsActive 읽기에서 중단했고 B는 공개 점프 상태·속도·입력 해제 후 다음 활성화로 관찰했다. 취소 발사 관찰의26→24/예비23유지는 정상인데 과거 고정27→25 기대값이 처음 거부해 실제 취소 전 탄약 기준의 무충전·소비·예비탄 보존 조건으로 교정했다. 게임을 바꿔 통과시키지 않았다. 물리 SpaceBar/터치, 입력차단 태그의 독립 사례, 화면/전체Rig/사운드/복제/패키징은 미검증이다.
- **새 확인 원인/다음 미적용 C++ 제안:** 원본 LyraCharacter.cpp:461의 CanJumpInternal_Implementation은 `JumpIsAllowedInternal()`을 반환해 crouch 검사만 생략한다. 대상 DreamCatcherCharacter에는 이 override가 없어 엔진 ACharacter.cpp:313의 `!IsCrouched() && JumpIsAllowedInternal()`을 사용한다. 그래서 원본 Jump GA의 CanActivateAbility가 먼저 거부되어 CharacterJumpStart의 UnCrouch→Jump에 도달하지 않는다. 다음 제안은 **DreamCatcherCharacter.h/.cpp2개에 원본 override1개 이식**이다. 기본 점프 조건은 유지하고 crouch 금지만 제거한다. 공용 Character/파생 Pawn의 CanJump 판정에 영향을 주므로 승인을 먼저 받으며 적용 후 사용자 C++ 빌드가 필요하다. GA에서 CanActivate를 우회하거나 현재 에셋에 임시 직접 Jump를 넣지 않는다.
- 현재 **서 있는 입력 점프 연결은 자동 관찰 통과, 전체 Jump/R8은 웅크림 실패와 기존 미검증 때문에 미완료**다. 후속 C++는 아직 수정하지 않았다. R7 남은 HUD는 R13과 묶는 상태를 유지한다.

---

## 0.52 이전 인계 — 2026-10-11 원본 CanJumpInternal override 적용 / 아래 빌드·실행 확인

- 사용자가 웅크림 점프 수정안의 진행을 승인했다. **DreamCatcherCharacter.h/.cpp2개에 CanJumpInternal_Implementation override1개만 추가**했다. 원본 LyraCharacter와 같이 `return JumpIsAllowedInternal();`을 사용해 crouch 검사만 제거하고 점프 횟수/이동 모드 등 나머지 조건을 유지한다. 원본 Jump Ability의 UnCrouch→Jump/해제 처리, 사망·입력·에셋은 변경하지 않았다.
- 선언은 엔진의 const virtual 시그니처에 override를 명시했다. 구현은 클래스 이름 치환 후 원본 함수와 정확히 일치하고 diff/whitespace 정적 검사를 통과했다. 변경 전 보호14파일 중 의도한 Character2개만 바뀌었고 나머지12개(사망 native/에셋, 통합 Jump/Death 세트·PawnData·Input·Pawn·IMC, Config2개, DLL, Git index)는 불변이다. 기존 사용자 staged 변경도 유지했다.
- **미검증:** 새 C++ 빌드·런타임 웅크림 점프. 이번에는 에디터/PIE·빌드·패키징을 실행하지 않았다. 이전 Jump_B의 웅크림 실패를 새 코드로 해결됐다고 미리 완료 처리하지 않는다. 다음은 사용자 Development Editor/Win64 빌드 확인 → 실제 IA_Jump 경로의 서기/웅크림/해제·공중·사망·입력차단 및 기존 전투 회귀다. 공용 Character와 파생 Pawn에 적용되는 원본 판정이므로 legacy 입력 경로 영향은 별도 확인한다. R8은 계속 진행 중이며 R7 잔여 HUD/R13 범위도 유지한다.

---

## 0.53 이전 인계 — 2026-10-11 웅크림 Jump 수정 빌드·입력 회귀 통과 / 아래 사용자 확인 완료

- 사용자 빌드는 **01:44:06 시작·94.48초 Succeeded**, DreamCatcherCharacter/Jump CPP 컴파일과 게임DLL **01:45:40/4,401,664 bytes** 갱신으로 확인했다. Codex는 빌드/패키징을 실행하지 않았다. 이번에는 기존 진단 절차 보완·실행과 상태 문서만 변경했고 게임 코드·Config·에셋은 추가 수정하지 않았다.
- **01:56:35 관찰 완료**, `Saved/Diagnostics/R8/pie_Jump_CrouchFix_B.json`55구간/464관찰에서 실제 IA_Jump→ASC→GA 경로의8조건이 모두 통과했다. 서기/해제 후 재점프/공중 추가 점프 방지/누른 채 착지 후 자동 반복 없음/**웅크림 점프**/입력차단/차단해제 후 재점프/사망 차단이다. 웅크림 점프 시 crouched=false·수직속도 약451, 차단 중0·차단 해제 후 약426을 관찰했다. 직접 Character.Jump 호출을 사용하지 않았다.
- `Saved/Automation/R8JumpCrouchFix_B/index.json` **01:57:19 성공1/실패·보고서경고0**, `R8-JumpCrouchFix_B.log` Error/Fatal/ensure0·프로세스 종료0이다. `evaluation_Jump_CrouchFix_B.json`의 기존 발사/수동·자동 재장전 후R/Layer·교체·사망 회귀를 포함한 한정 조건29개도 실패0이며, 사용자 빌드 후 기준으로 보호996경로 불변이다. 조건 수는 추가 구현 진척이 아니다. 앞선 A는 추가한 차단 검사에서 재사용 helper의 중첩 scope 경로를 잘못 조회해 중단됐고, 진단 참조만 교정한 B가 최종 결과다.
- **현재 완료 범위:** 발견됐던 실제 입력 점프 연결 및 웅크림 실패의 코드 수정·사용자 빌드·단일 로컬PIE 회귀. 사망 Ability/공용 입력·기존 에셋 유지. **사용자는 이번 물리 SpaceBar/웅크림→점프/착지 후 재점프 및 화면·조작감은 아직 확인하지 않았다고 답했다.** 이 수동 확인은 미완료로 둔다.
- 다음은 최신 빌드의 L_DC_RifleIntegration에서 위3동작을 짧게 확인하고 손·총 어긋남/발 미끄러짐/자세 튐 여부를 알려받는 것이다. 전체 R8의 포즈·FootPlant Rig/표시 구조 차이·다른 legacy 파생 Pawn 영향, 터치/복제/패키징은 기존 미검증으로 남긴다. 단일 PIE·입력 주입·Cue비활성/무음 검사를 전체 시각/실기기 검증으로 승격하지 않는다. 추가 코드/에셋 변경 또는 R9 활성 구현은 아직 승인하지 않았으며 R7 잔여 HUD/R13 통합도 그대로다.

---

## 0.54 최신 재개 — 2026-10-11 Jump 물리키·화면 사용자 확인 완료 / R9-1 원본 Dash 변경안

- 사용자가 **서서 점프·웅크림 후 점프·착지 후 재점프가 모두 정상이며 자세 튐/발 미끄러짐/손·총 어긋남도 없음**을 확인했다. 이 Jump 수정 묶음은 코드/사용자 빌드/실제 입력 자동검사/물리키·해당 화면 확인까지 완료다. 이전 “아직 확인하지 않음”은 해소됐다. R8의 기존 라이플/Layer/재장전 및 이번 Jump 핵심 경로 검증은 유지하되 전체 FootPlant Rig/원본 표시 구조·다른 legacy 파생 Pawn·모든 장치/복제/패키징을 완료로 올리지는 않는다. 남은 R8 미검증은 R8에 남기고 R9-1 변경안을 검토한다.
- **원본 Dash 읽기 전용 조사:** `Saved/Diagnostics/R9/source_A/report.json`02:03:22 및 `source_B/report.json`02:08:34 KST, `R9-DashRead_A/B.log` 종료0/저장0/명시적Compile0/ensure0/보호파일 불변. B는 핵심11에셋과 참조224개(Game/ShooterCore, Engine/Script/외부 Niagara 별도)를 조사했다. 동일 상대경로 파일171개가 대상에 있지만 재사용 확정/신규 복사 수가 아니다. 범용 native contract API는 승인22개 밖인 Dash를 거부했으므로 그 API의 native 전체 감사 성공을 주장하지 않는다. 실제 BP 그래프/핀·CDO/export와 C++를 근거로 삼았다.
- 원본 GA_Hero_Dash는 GA_AbilityWithWidget 자식, LocalPredicted/InstancedPerActor/OnInputTriggered/Independent이며 Strength1850, RootMotionDuration0.25, AbilityDuration0.55다. ApplyRootMotionConstantForce는 additive=true/gravity=false/종료속도 Clamp1000이고 전후좌우4개 Montage를 선택한다. GE_HeroDash_Cooldown은1.5초다. 방향은 캐릭터 facing/이동 입력/카메라 forward 및 AI 분기를 쓰므로 폐기한 자체8방향/무입력 규칙으로 바꾸지 않는다. 정확한 무입력·공중·벽·취소 동작은 아직 원본/대상 PIE 미검증이다. 확인한 GA/GE에 무적 부여를 찾지 못했지만 방어 전체 검증은 R9-2로 분리한다.
- **확인된 연결 차이:** Dash는 LyraCharacter.CameraComponent와 LyraCameraComponent 타입을 직접 읽는다. 대상은 실제 활성 모드 카메라인 DreamCatcherCharacter.ModeCamera/DCCameraComponent를 사용하며 해당 Class/Property Redirect는 없다. 기존 GetCameraView는 최종 모드 뷰의 위치/회전을 컴포넌트에 반영한다. 따라서 원본 방향 계산을 유지하려면 이 정확한2이름 대응이 필요하다. 로드/Compile/접근성 검증은 후속이며 실패 시 getter 우회나 카메라 재설계를 임의로 하지 않는다.
- **02:11:26 태그 조회:** Ability.Dash.Duration.Message, GameplayCue.Character.Dash, GameplayCue.Character.Dash.Cooldown, InputTag.Ability.Dash, HUD.Slot.Reticle, HUD.Slot.ExtraEquipment6개는 빈 태그로 import됐고 Ability.Type.Action.Dash/Event.Movement.Dash는 유효했다. 원본 입력은 LeftShift 및 Gamepad_LeftThumbstick(추가 Pressed mapping trigger)이며 Action 자체도 Pressed trigger/Jump와 별개의 InputTag.Ability.Dash다. Dash UI 등록 슬롯은 Reticle/ExtraEquipment로, 현재 R7 정식 HUD 미연결 때문에 실제 표시 완료는 R7/R13과 연결해서 확인해야 한다. UI 참조/등록 그래프는 제거하지 않는다.
- **다음 미적용 제안:** R9-1 원본 Dash 묶음만 처리한다. Config의 위 Class/Property Redirect2개·원본 태그6개·새 Dash Cue 스캔 경로1개, `/Game/LyraMigration/Dash/`의 원본 GA/GE/Cue/위젯 및 필요한 Montage/VFX/사운드 의존성, 통합 전용 AS_DC_RifleDash 및 기존 DA_DC_RiflePawn/Input/IMC_DC_RifleBase의 Dash 연결이다. 현재 LeftShift의 테스트 Dodge 항목을 원본 IA_Ability_Dash/InputTag.Ability.Dash로 바꾸고 원본 게임패드 키/트리거도 보존한다. Jump·Death·다른 능력과 공용 IMC/다른 맵은 유지한다. 기존 에셋은 비교 후 재사용하고 덮어쓰지 않으며, 필요 신규 에셋은 새 Dash 폴더에 제한한다. 새 타입/참조 불일치가 나오면 확대 전에 근거/대안을 제시한다. 원본 Blueprint를 C++로 재작성하거나 무적/궁극기를 이 묶음에 추가하지 않는다. 현재 새 C++ 필요성은 확인되지 않았으며 코드 변경 시 별도 승인/사용자 빌드다.
- 이번에는 사용자 확인 기록·원본/태그 조사·문서만 갱신했고 게임 코드/Config/에셋·빌드/패키징을 변경·실행하지 않았다. R9-1 실제 이식은 승인 대기, R9-2 방어/R9-3 궁극기는 미착수이며 R7 잔여 HUD/R13은 기존 승인대로 미완료다.

---

## 0.55 최신 재개 — 2026-10-11 승인된 R9-1 Dash 실제 이식·통합 연결

- 사용자가 0.54의 R9-1 묶음을 승인해 진행했다. **새 C++·빌드·패키징·공용 입력·기존 Jump/Death 수정 없음.** `r9_dash_assets.py`와 `r9_dash_integration.py`는 이 묶음의 제한된 일회성 UE 에셋 처리 절차이며 범용 도구 확장이 아니다.
- 원본 의존성224개 중 기존200개를 조회/비교하고 신규24개만 `/Game/LyraMigration/Dash/`에 이식했다. 기존200개를 저장하거나 덮어쓰지 않았다. 공용 위젯 타입 표시명·미연결 delegate 주소·MetaSound 복제 식별자 차이를 구분했다. Texture Oodle 버전·SKM_Manny_Invis의 SourceModels 캐시·CR_Mannequin_Procedural의 RequiredPlugins 차이도 기록했으며 전체 메시/리그/픽셀 동등성은 여전히 미검증이다.
- **복사·재로드 근거:** `assets_Prepare_B`02:55:44는 신규24개 저장/비교 성공 후 프로세스 종료 중 AnimSequence 압축 작업 중복 assertion으로 종료3이었다. 이 receipt의 complete만으로 성공 처리하지 않았다. **`assets_SourceVerify_A`02:57:18 별도 원본 재로드/비교·저장0·정상종료0**으로 저장본을 확인했다. `assets_Migrate_A`02:56:14는 UE API로 정확한24개만 이식/기존 파일 보호/정상종료0이다. 원본/공용 보호 파일은 유지됐다.
- **카메라 대응 검증:** 영구 Config에는 승인된 Class/Property Redirect2개·원본 태그6개·Dash Cue 스캔1개만 추가했다. 처음 대상 로드에서 클래스 변환 후 CameraComponent를 찾지 못했다. 엔진 MemberReference.cpp의 변환된 소유 클래스 기반 탐색과 저장 없는 `CameraInspect_A`로 확인하고, **이번 저장 프로세스에만 DreamCatcherCharacter.CameraComponent→ModeCamera 대응**을 적용했다. 새 GA의 핀은 ModeCamera로 저장됐으며 영구 Redirect 추가/새 getter/방향 계산 변경은 없다. 새 GCNL_Dash의 `replicate_using_registered_sub_object_list`만 원본 CDO의 true로 명시 보존했다(대상 기본값 false; 전역 설정 변경 아님).
- **`assets_Target_C`03:01:09 새24개 저장/비교, `assets_Verify_A`03:01:52 임시 Package/Property 옵션 없는 별도 재로드·BP Compile·원본 데이터/그래프 비교가 정상종료0으로 통과**했다. 최종 verify의 Error/Fatal/ensure0, 에셋 저장0·보호파일 불변이다. 원본 수치1850/0.25/0.55/쿨다운1.5,4방향 Montage, Cue/위젯 참조를 유지했다. native 표시명/복제 identity/주소 외 값을 임의로 누락해 통과시키지 않았다.
- **통합 연결:** `integration_Apply_B`03:04:14 저장4개, `integration_IntegrationVerify_A`03:05:15 새 프로세스 조회/저장0·정상종료0. 새 AS_DC_RifleDash 1개를 기존5개 AbilitySet 뒤에 추가했다. 통합 DA_DC_RifleInput의 Dodge 항목과 IMC_DC_RifleBase의 LeftShift만 원본 IA_Ability_Dash/InputTag.Ability.Dash로 교체하고 원본 Gamepad_LeftThumbstick/Pressed trigger를 추가했다. 기존 Jump·Death 세트/나머지 입력·Pawn BP/공용 IMC_Player/IA_Dodge는 유지한다. legacy 자체 Dodge 코드는 다른 경로의 비교/복구용으로 남아 있으며 프로젝트 전체 제거는 하지 않았다.
- PIE 및 사용자 화면 확인 결과는 아래 후속 판정을 따른다. 최종 Dash HUD 슬롯 표시는 **R7/R13 미완료**, 게임패드 실기기·원본 프로젝트와 나란히 실행한 비교·복제/패키징도 미검증이다. R9-2 방어/무적, R9-3 궁극기는 이번에 추가하지 않았다.

**R9-1 PIE 최종 판정 / 사용자 확인 대기:**

- **03:16:35 `assets_Verify_B` 최종 읽기 전용 검사:** 새24개 모두 프로젝트 하드/소프트 package 참조가 원본→대상 대응표와 일치(누락/추가0), 임시 대응 옵션 없이 재로드/Compile/데이터 비교·저장0·보호파일 불변·정상종료0. 기존 ShooterCore 참조를 영구 PackageRedirect로 숨기지 않았다.
- `Saved/Diagnostics/R9/pie_Dash_B.json` **03:13:28 관찰 종료**, `evaluation_Dash_B.json` 한정52조건 실패0,106구간/789관찰. 실제 IA_Ability_Dash→ASC→원본 GA/RootMotion 경로이며 직접 Dodge/Launch/RootMotion 호출을 쓰지 않았다. 단독 무입력 전방1850, 이동 중 전후좌우 방향/각 Montage·쿨다운 거부/해제 및 누른 채 반복 없음, 웅크림 해제/공중 사용·조준/사격/재장전 중 실행을 확인했다. 임시 PIE 벽 앞 x약146.98에서 캡슐이 멈췄다(벽 앞190/반경42). 입력 차단/해제, Dash 도중 사망 취소 및 사망 후 차단, 기존 Jump/웅크림/조준/사격/수동·자동 재장전 후R/장비 교체·Layer 회귀도 해당 조건에서 통과했다.
- `Saved/Automation/R9Dash_B/index.json` **03:14:26 성공1/실패·보고서경고0**, `R9Dash-PIE_B.log` Error/Fatal/ensure0·종료0. 보호2073경로 불변·에셋 저장0이다. **B는 Cue비활성·무음·입력 주입의 단일 로컬PIE**이며 물리키/화면/소리/복제를 검증한 것은 아니다.
- 앞선 `Dash_A`는 Cue가 Pawn을 대상으로 실행되고4방향 Montage를 선택한 관찰은 있지만, 무음 상태에서 기존 Rifle WeaponAudioFunctions의 빈 TapIDs 관련 경고1470건과 동시 Git 조회 구간의 `.git/index` 바이트 변경을 검출해 **자동검사 실패1**이었다. 게임·Config·에셋 보호 경로 변경은 없었다. A를 통과로 승격하지 않으며 B는 새 보호 스냅샷/동시 Git 조회 없이 게임플레이 검증을 분리했다. 원본 사운드/시각 정상 여부는 사용자 확인 대기다.
- **사용자 다음 확인:** 에디터 재시작 후 L_DC_RifleIntegration에서 LeftShift를 무입력/전후좌우 이동 중 사용하고, 웅크림·점프 중 사용/연속 입력 시 쿨다운, 조준·사격·재장전 및 Dash 중 사망을 짧게 확인한다. 방향/몽타주·이펙트/소리·카메라가 자연스러운지 답변받는다. 이번에는 C++ 변경이 없어 재빌드 요구 없음. 최종 Dash HUD는 표시되지 않아도 현재 R7/R13 미완료 경계와 구분한다. 정상 확인 후 R9-2의 **원본 방어 효과 근거·프로젝트 확장안**을 먼저 제시한다. 임의 무적·성공 피드백·궁극기 제작은 승인하지 않았다.

---

## 0.56 최신 재개 — 2026-10-11 무입력 Dash 조건 누락 정정 / 수정 선택 대기

- 사용자가 무입력 Dash 미작동을 보고했다. 기존 `dash_pie_probe.py`는 각 시험 전 ControlRotation을0으로 초기화했으므로 0.55의 “무입력 통과”는 **월드 Yaw0의 한정 조건**이다. 모든 방향의 무입력 동작을 검증한 것이 아니며 앞선 일반화는 정정한다.
- 원본 및 이식본의 `SelectDirectionalMontage`를 이동벡터0·Facing Yaw0/30/44/46/90/135/180/-90에서 저장 없이 실행했다. `NoInputSource_A.json`/`NoInputTarget_A.json`의 반환값은 경로 치환 외 일치했다. 0/30/44는 Forward/BiasForward=true, 나머지는 Left/Right/Backward 및 false였다. 원본 EventGraph는 false일 때0인 LastMovementInput을 Direction으로 선택한 뒤 NearlyZero 검사에서 Commit 전에 Cancel한다. 무입력 벡터의 Rotation From X Vector가0도가 되어 월드 방향에 의존하는 원본 함수 경계 동작이다. 원본 프로젝트 전체 PIE 재현과는 구분한다.
- 대상 실제 PIE `pie_NoInput_B.json`에서 기록된 실제 Pawn Yaw/LastInput0을 확인하며 같은 결과를 재현했다. 0/30/44는 속도1850·Dash/쿨다운 적용,46/90/135/180/-90는 속도0·Dash/쿨다운0. `Saved/Automation/R9NoInput_B/index.json` **03:28:14 성공1/오류·경고0/종료0**, 보호파일 불변이다. 여기서 자동검사 성공은 **진단 완료**이지 무입력 기능의 정상 판정이 아니다. 앞선 NoInput_A는 Rotator 위치 인수로 실제 Yaw가 설정되지 않아 각도 검증 근거에서 제외하고 B에서 명명된 인수/실제 Yaw 기록으로 교정했다.
- 현재는 진단만 했고 게임 C++/Config/에셋 수정·빌드·패키징을 하지 않았다. **변경안:** 대상 GA_Hero_Dash의 SelectDirectionalMontage에서 이동벡터가 NearlyZero일 때만 Forward Montage/BiasForward=true를 반환해 기존 카메라 전방 벡터를 쓰도록 한다. 입력이 있는 원본4방향 분기·힘/시간·쿨다운·Cue는 유지한다. 이는 원본의 조건부 무입력 취소와 달라지는 최소 보정이므로 사용자 승인 전 적용하지 않는다. 대안은 원본을 그대로 유지하고 현재 각도 의존 동작을 명시하는 것이다.
- 별도 미완료: 사용자의 실제 `DreamCatcher.log`03:19~03:20 PIE에서 Dash MetaSound preset의 `Could not find graph with registry graph key` 오류를 확인했다. 기존 무음/Cue비활성 검사로 오디오 정상 여부를 보증하지 않는다. 원인/정확한 변경안은 별도 조사 필요이며 이번에 음원이나 엔진 DebugExecBindings를 임의 변경하지 않았다. Shift 디버그 로그도 있지만 키 상태는 엔진에서 먼저 누적하므로 그 로그만으로 입력 소비가 원인이라고 단정하지 않는다.

---

## 0.57 최신 재개 — 2026-10-11 승인된 무입력 Dash 보정 완료 / 음원 변경안 승인 대기

- 사용자는 **무입력만 카메라 전방으로 보정하고 나머지 원본 동작 유지**를 승인했다. 음원은 **원인 조사와 변경안 제시까지만** 요청했다. 에디터 종료를 사용자에게 확인받고 대상 `GA_Hero_Dash`1개만 저장했다. 원본 Lyra/음원·Cue/Config/게임 C++는 변경하지 않았고 C++ 빌드·패키징도 실행하지 않았다.
- `Scripts/Editor/r9_dash_noinput_fix.py`: SelectDirectionalMontage에 Vector_IsNearlyZero(Tolerance0.0001)와 Branch2노드만 추가했다. true는 기존 Forward Montage/BiasForward=true 분기, false는 기존 방향 비교 분기로 연결한다. EventGraph/Get Direction 및 기존 카메라 벡터(Z보정 포함)·힘1850/0.25/0.55초/쿨다운1.5와 다른 defaults는 동일하다. 기존 노드/핀/연결은 승인된4개 핀의 연결 외 그대로 비교했다.
- `idle_fix_IdleApply_A` **03:36:28 저장1**, `idle_fix_IdleVerify_A` **03:38:34** 별도 재로드/Compile/저장0·정상종료0. 무입력8각도는 Forward/true, 이동 입력 있는64조합은 수정 전후 같았다. `pie_IdleFixed_B.json` **03:41:49**, `evaluation_IdleFixed_B.json`: 실제 InputAction/ASC 경로의8각도 모두 활성·속도1850, 수평 카메라/캐릭터 정렬 조건에서 이동방향 일치, 보호2073경로 불변이다. `Saved/Automation/R9IdleFixed_B/index.json` **03:42:08 성공1/오류·경고0/종료0**. 첫 IdleFixed_A는 ExecCmds의 쉼표 파싱 때문에 검사 스크립트가 실행되지 않아 검증 근거에서 제외했다. 실제 물리키/연출·비수평 카메라·복제는 사용자/별도 검증으로 남긴다.
- **음원 원인 확인:** `AudioSource_A/B`, `AudioTarget_A/B`는 읽기 전용·저장0·파일 불변이다. 부모 경로는 양쪽 `/Game/Audio/MetaSounds/sfx_RandomStereo_nl_meta`로 같고 GetReferencedPresetAsset도 유효하지만 부모 Class ID는 원본 `C7725AA14BADE5EB7F9F54894DB8BE03`, 대상 `5A9AD0844AAAEB75E9111BAC4F8BFFC3`로 다르다. 대상 preset의 stock T3D export38행에는 `ReferencedAssetClassKeys`가 원본 C772…로 남아 있다. 보호 속성 직접 읽기는 거부되어 우회하지 않았으며 일반 UE text export와 실제 로그를 근거로 삼았다. `DreamCatcher.log`2655~2662행은 원본 키 미등록→대상 부모 키 AutoUpdate/registry cache 실패→runtime graph 생성 실패를 기록한다. 단순 경로/그래프 기본값 동등성만으로 재사용하면 내부 MetaSound 클래스 참조까지 보존된다고 판단했던 점이 누락이었다.
- **음원 미적용 제안:** 새 Dash preset `/Game/LyraMigration/Dash/sfx_DashStereo_nl_meta_Preset`1개만 UE MetaSound Editor/Builder로 실제 대상 부모에 다시 연결하고 참조/등록 캐시를 재생성한다. 이미 동등하게 확인한 원본 SoundWave2개, Min1/Max4/Gain0.3, Stereo·감쇠·SoundClass 등 설정을 보존한다. 부모·공용 오디오/다른 preset·GCNL_Dash·발사 간격·Config는 변경하지 않는다. 엔진 ConvertToPreset은 부모를 실행용 등록한 뒤 Template Parent를 설정/Configure하므로 후보로 확인했지만 **보정 실행 및 새 오디오 장치 PIE 성공은 아직 미검증**이다. literal GUID 직접 치환·WavePlayer 직접 대체·전체 오디오 덮어쓰기는 하지 않는다. 대안인 원본 부모의 별도 재이식은 추가 에셋/참조 관리가 필요해 우선하지 않는다. 승인받은 뒤 메모리 비교→대상 preset1개 저장→새 프로세스 재로드→오디오 활성 PIE→사용자 실제 소리 확인 순서로 진행한다.

---

## 0.58 최신 재개 — 2026-10-11 Dash preset 참조 복구·오디오 활성 PIE 통과

- 사용자가 0.57의 **Dash preset1개 부모 재연결/내부 참조 재생성**을 승인했다. 에디터 미실행을 확인한 뒤 `/Game/LyraMigration/Dash/sfx_DashStereo_nl_meta_Preset`만 저장했다. 새 C++/Config/공용 부모/GCNL_Dash/원본 Lyra/사격 오디오/무입력 보정 GA는 변경하지 않았다. C++ 빌드·패키징도 실행하지 않았다.
- `Scripts/Editor/r9_dash_audio_fix.py`는 UE MetaSound Builder로 현재 부모에 ConvertToPreset 후 원본의4개 override(Sounds/Min/Max/Gain)를 복원했다. SoundWave2개·Min1/Max4/Gain0.3·preset 자체 Class ID·부모 경로·Stereo/감쇠/SoundClass 등 RootMetasoundDocument 외 설정을 비교/보존했다. OnPlay는 inherited 상태를 유지했다. 보호 속성 직접쓰기/바이너리 수정/GUID 문자열 치환은 사용하지 않았다.
- **중간 저장 이력과 최종 결과 구분:** AudioApply_A/AudioFinish_A에서는 문서의 부모ID는 교체됐지만 직렬화 참조 키가 C772…+5A9… 둘 다 남아 검사 실패였다. 같은 preset1개 외 파일은 불변이었다. 별도 비표시 표준 Editor에서 해당 MetaSound만 열어 실행한 재등록 경로가 참조 집합을 재생성했다. 첫 AudioEditorFinish_A는 직전 저장본이 아닌 이전 receipt와 노드ID를 비교해 저장 전 중단됐으며, 최신 receipt를 기준으로 한 **AudioEditorFinish_B 03:55:25 저장1/정상종료0**가 최종본이다. 최종 `ReferencedAssetClassKeys`는 대상 부모 **External_None.5A9AD0844AAAEB75E9111BAC4F8BFFC3_1.0 하나만**이며 옛 키는 제거됐다. 원본/공용/다른 게임 파일을 덮어쓰지 않았다.
- **새 프로세스 재로드:** `Saved/Diagnostics/R9/audio_fix_AudioVerify_A/report.json` **03:56:48**, 저장0·설정/입력/문서/참조 키 비교·보호파일 불변·정상종료0. 재생 때 부모를 미리 등록하는 새 게임 코드나 초기화 우회는 추가하지 않았다.
- **오디오 활성 검증:** `Saved/Diagnostics/R9/pie_DashAudio_A.json` **03:58:15 관찰 종료**. -nosound/Cue비활성 옵션 없이 WASAPIMixer와 PIE AudioDevice가 초기화된 새 프로세스에서 실제 IA_Ability_Dash→GA→GCNL_Dash→수정 preset 경로를3회 실행했다. AudioComponent3개의 Playing 상태(33표본)와 최종 재생 종료, 무입력 Dash3회, 보호파일 불변을 확인했다. `Saved/Automation/R9DashAudio_A/index.json` **03:58:30 성공1/실패·경고0**, `R9Dash-AudioPIE_A.log` Error/Fatal/ensure0 및 이전 MetaSound registry 오류0·프로세스 종료0이다. 에셋 저장0이다. 실제 사람이 들은 소리의 품질/크기/타이밍, 화면/물리키/복제 완료로 확대하지 않는다.
- **다음 사용자 확인:** 새 에디터의 L_DC_RifleIntegration에서 정지한 채 방향을 바꿔 LeftShift Dash가 나가는지, 이동 중 원래 방향 Dash 및 효과음이 자연스럽게 한 번 재생/종료되는지 확인한다. C++ 변경이 없어 이번 재빌드 요구 없음. 확인 전 R9-1 사용자 수용/연출 전체 완료로 표시하지 않는다. R9-2 방어/R9-3 궁극기와 R7/R13 최종 Dash HUD는 미완료로 유지한다.

---

## 0.59 최신 재개 — 2026-10-11 R9-1 사용자 확인 완료 / R9-2 첫 변경안

- 사용자가 **무입력·이동 중 Dash와 효과음 모두 정상**을 확인했다. 직전 재로드/무입력 보정/오디오 활성 자동검사 기록과 현재 핵심8에셋의 해시 일치도 확인했다. R9-1의 PC 통합 Dash·승인된 무입력 보정·효과음 복구 묶음은 자동검사와 해당 사용자 수용 확인까지 완료다. 최종 Dash HUD(R7/R13), 게임패드/터치·복제·패키징 등의 기존 미완료 범위와 R9-2/3는 완료로 올리지 않는다.
- **이번에는 조사/문서만 갱신:** 새 게임 코드/Config/에셋 수정이나 빌드/패키징은 없다. 사용자는 다음 변경안을 먼저 요청했으므로 아래 내용은 미적용·승인 대기다.
- **원본 근거:** 원본 GA_Hero_Dash/GE_HeroDash_Cooldown/부모·Cue의 조사 범위에서는 Gameplay.DamageImmunity 부여가 없으며 GE의 immunity query도 비어 있다. 원본 LyraHealthSet.cpp82 및 대상 DCHealthSet.cpp81에는 이미 일반 양수 Damage를 해당 태그로 차단하고 SelfDestruct는 예외로 두는 로직이 있다. 기존 대상 PIE에서도 Dash 활성 중 시험 치명피해→Death 전이가 관찰됐다. 원본 전체 게임 PIE의 모든 피해 유형까지 검증했다는 뜻은 아니다.
- **R9-2 첫 묶음 제안:** 기존 DamageImmunity 차단 로직을 재사용하고, 별도 `DCAbilityTask_DashDefenseWindow`와 태그 전용 `GE_DC_DashDefense`를 추가한다. 원본 Dash 이동을 C++로 재작성하지 않는다. Commit 성공이 확인된 권한 경로에서만 방어 창을 시작하고, **RootMotionDuration과 같은 시간(현재0.25초)**을 초기안으로 제안한다. Ability 전체0.55초 방어는 더 긴 별도 대안이며 현재 채택/승인되지 않았다. 원본 대비 변경은 이 구간의 일반 Damage를 차단하는 프로젝트 추가 규칙이다.
- Task는 자기 GE handle과 구독/타이머만 소유하고 만료·Ability 종료/취소·사망·ASC/Avatar 해제 때 정리한다. 다른 무적 효과/태그를 일괄 삭제하지 않는다. GE 자체 만료도 안전장치로 둔다. 단순 Dash 활성 태그 관찰만으로 Commit 전 면역을 부여하지 않으며, 원본 서버 SendInfo의 Commit 반환값이 현재 미사용인 점을 고려해 새 방어 시작만 성공 결과로 제한한다.
- DCHealthSet에는 기존 차단 결정 이후의 알림 hook만 추가하고 피해 계산/예외는 유지한다. 실제 양수 피해가 이번 방어 창 때문에 차단될 때만 기존 GameplayEvent.Dodge.Success를 내며, 첫 묶음은 Dash1회당 최초1회 성공 알림을 제안한다. 0피해/치유·SelfDestruct·다른 무적/치트에 의한 차단을 성공으로 오인하지 않는 보수적 판정과 중첩 테스트를 포함한다. 시작/종료/성공 delegate를 제공하되 전용 시각/사운드 성공 연출은 이번 첫 묶음에 임의 추가하지 않고 **R9-2의 남은 작업**으로 둔다.
- 예상 변경 범위: Tasks/DCAbilityTask_DashDefenseWindow.h/.cpp 신규, Attributes/DCHealthSet.h/.cpp 최소 hook, Tests/DCDashDefenseAutomationTests.cpp, `/Game/DreamCatcher/GAS/Effects/GE_DC_DashDefense` 신규, 대상 GA_Hero_Dash의 방어 Task 연결. 기존 태그를 재사용해 현재 Config 변경을 계획하지 않는다. Cue/오디오/카메라/이동·원본 쿨다운/legacy 전체 삭제/R9-3은 범위 밖이다. 승인 시 코드 묶음→사용자 C++ 빌드→GE/BP 자동화 및 실제 Damage 경로 검증으로 진행한다.

---

## 0.60 최신 재개 — 2026-10-11 R9-2 방어 Task 코드 준비 / 사용자 빌드 대기

- 사용자가 0.59의 첫 묶음을 승인했다. 이번에는 **코드와 상태 문서만** 작성했고 C++/UHT 빌드·패키징·Unreal 검사 실행·GE/BP 저장을 하지 않았다. GE_DC_DashDefense와 GA_Hero_Dash 연결은 아직 미적용이므로 게임에 방어 창이 활성화됐다고 보고하지 않는다.
- 신규 `AbilitySystem/Tasks/DCAbilityTask_DashDefenseWindow.h/.cpp`: 실제 Commit 성공값·유효한 양수 Duration·활성 Ability·현재 ASC/Avatar·서버 권한을 요구한다. 입력 Duration은 후속 BP에서 기존 RootMotionDuration(0.25)을 전달한다. 비주기/비중첩 HasDuration·DamageImmunity1태그·무수정자/무Execution GE만 허용하고 Spec duration을 잠근다. 자신이 적용한 GE handle/구독/타이머만 정리하며 같은 Ability의 겹치는 중복 적용을 거부한다. 다른 무적/치트가 겹친 피해는 회피 성공으로 귀속하지 않는다.
- 기존 DCHealthSet.h/.cpp에는 원본 면역 결정/피해0 처리 후 동기 OnDamageBlocked 알림만 추가했다. SelfDestruct 예외·일반 피해 계산/사망 로직은 그대로다. Task는 실제 양수 차단 후 기존 GameplayEvent.Dodge.Success와 OnDamageAvoided를 최초1회 보낸다. payload에는 Instigator/Avatar/차단량/EffectContext/캡처 태그를 담으며 재진입 전에 성공 플래그를 세운다. 이벤트/Task delegate는 권한 측 알림이고 별도 클라이언트 성공 연출/보상은 추가하지 않았다.
- 만료·외부 GE 제거·Ability 종료/취소·Health0·Avatar EndPlay는 정리 콜백으로 처리한다. 프로젝트의 정상 Ability 해제 외에 **ASC 직접 ClearActorInfo/Avatar 교체만 수행되는 경우는 짧은 Task Tick에서 다음 프레임까지 정리**하며 GE 자체 만료를 안전장치로 둔다. 이를 동기 즉시 정리로 과장하지 않는다. OnClosed는 살아 있는 Ability로만 보내되 소유 Ability의 종료 delegate를 통해 정상 EndTask 정리가 먼저 실행되게 했다.
- 신규 `Tests/DCDashDefenseAutomationTests.cpp`와 검사 전용 `DCDashDefenseTestTypes.h`: production 에셋/CDO가 아닌 전용 Ability/GE fixture와 별도 월드를 사용한다. `DreamCatcher.R9.DashDefense`4개 검사에 방어 전/중/후 피해, 치유/0피해, 중복/재진입 성공 방지, 다른 무적/치트 및 다른 GE handle 보존, 실패 Commit/잘못된 시간/비권한 거부, SelfDestruct/사망, 외부 제거/Avatar 교체/해제를 작성했다. 실행 시 `-DCR9DefenseIsolatedAutomation`이 필요하다. **아직 컴파일·실행 결과가 없으며 검사 작성은 통과가 아니다.**
- 로컬 UE5.8의 Task/GE/delegate API 대조와 텍스트/diff 점검만 수행했다. 기존 Config·Dash/음원/Cue 에셋·게임DLL 보호 해시가 유지됐다. Build.cs/태그 등록 변경도 없다. 다음은 **사용자 Development Editor/Win64 빌드→신규 독립검사 및 기존 회귀→승인된 GE1개 생성과 Dash의 Commit 성공 경로 Task 연결→실제 피해/취소/사망 PIE**다. R9-2 성공 전용 시각/사운드 연출 및 R9-3, R7/R13 잔여 HUD는 미완료다.

---

## 0.61 최신 재개 — 2026-10-11 R9-2 방어 GE/Dash 연결·자동검증 통과 / 사용자 연출 확인 대기

- **사용자 빌드 확인:** UBT `Log.txt`의 13:35:40 KST 시작, Development Editor/Win64 **51.31초 성공**, 게임 DLL 13:36:31 갱신(4,491,264 bytes)을 확인했다. Codex는 C++ 빌드·패키징을 실행하지 않았고 이번 턴에는 게임 C++/Config를 추가 변경하지 않았다.
- **독립 검사:** `Saved/Automation/R9DefenseCode_A/index.json` 13:41:41의 방어4건 성공/실패·보고서경고0. 최종 `R9DefenseRegression_A/index.json` 13:54:45는 방어4＋기존 무기3＋Equipment/QuickBar1 **8건 성공/실패·보고서경고0**, 프로세스 종료0이다. 임시 native 월드 검사이며 실제 PIE와 구분한다. 회귀 로그의 기존 B_LyraGameInstance 원본 Script import 경고는 별도 남아 있다.
- **승인된 에셋2개만 저장:** 신규 `/Game/DreamCatcher/GAS/Effects/GE_DC_DashDefense`(GameplayEffect 직계, HasDuration0.25, 비중첩/비주기, DamageImmunity1태그 전용)와 기존 `/Game/LyraMigration/Dash/GA_Hero_Dash`. 실제 latent AbilityTask2개/RootMotionDuration getter2개를 추가하고 로컬 Commit 성공 분기 및 서버 SendInfo Commit 뒤에 연결했다. 각각 실제 Commit 반환값을 전달하며 서버 원본 이동의 성공/실패 실행 정책은 바꾸지 않았다. 연출/보상 delegate는 아직 미연결이다.
- `Scripts/Editor/r9_dash_defense_setup.py`: `defense_Apply_C` **13:49:57 저장2개**, `defense_Verify_A` **13:50:46 별도 재로드/Compile/저장0** 통과. 허용한 핀 연결 외 기존 그래프/부모/기본값 및 방향 선택72조건이 일치한다. 최종 조회의 보호2483경로(없는 동반 파일 포함)는 불변이다. 원본 이동·몽타주·쿨다운·카메라·음원/Cue·기존 무입력 보정을 유지한다. 앞선 조회 API/주석 노드 처리 진단 실패는 저장0으로 중단했으며 게임 동작 실패로 혼동하지 않는다.
- **실제 PIE 통과:** `Scripts/Editor/r9_dash_defense_probe.py`, `Saved/Diagnostics/R9/pie_DefensePIE_B.json` 및 `Saved/Automation/R9DefensePIE_B/index.json` **13:54:07 성공1/실패·보고서경고0/종료0**. 실제 IA_Dash→이식 Ability/Task 경로에서 전 피해100→75, 방어 중20/15 피해75유지, 창 종료 후75→65, 쿨다운 재입력 면역0/추가피해65→55, 취소 즉시 면역0/55→50, 창 중 SelfDestruct50→0/면역0, 사망 후 재입력 면역0을 확인했다. Dash 활성 태그가 남아 있어도 창만 종료되는 표본도 확인했다. 시작/종료 로그 각3회, 성공 차단 로그1회이고 Error/ensure/Fatal0이다. 이후 원본8초 Death 종료도 로그에서 확인했다.
- **검증 경계:** PIE는 RenderOffscreen, Cue 비활성/무음으로 피해·수명을 검사했다. 기존 AutoReload Script import/PoseAsset/드라이버 시작 경고는 여전히 남으며 새 연출·물리키·원격 SendInfo RPC/복제·패키징 검증은 아니다. 첫 PIE_A는 콘솔 `__file__` 조회 실패로 방어 probe 자체가 미실행됐으므로 통과 근거에서 제외했다. 교정한 PIE_B가 유효 근거다.
- 사용자는 최신 연결 후 방향·몽타주·효과음을 **아직 확인하지 않았다고 답했다.** 재빌드 없이 새 에디터 L_DC_RifleIntegration의 짧은 LeftShift 확인만 남긴다. 0.25초 피해 타이밍 수동 재현은 요구하지 않는다. **R9-2 첫 기능 묶음 구현/자동검증 완료, 사용자 수용 확인 대기**다. 다음은 기존 Dash 연출과 분리한 회피 성공 전용 피드백의 후보 조사/변경안이며 별도 승인 전 임의 연출·보상/충전을 추가하지 않는다. R9-2 전용 성공 연출, R9-3, R7/R13 잔여 HUD 및 장치/복제/패키징은 미완료다.

---

## 0.62 최신 결정 — 2026-10-11 회피 성공 전용 연출 제외 / R9-3 궁극기 설계 확인

- 사용자가 **회피 성공 전용 시각·음향 피드백은 현재 필요 없으므로 스킵**하도록 명시했다. 이는 사용자 승인에 따른 현재 작업 범위 제외이며 구현 완료나 검증 통과를 뜻하지 않는다. 0.61까지의 해당 연출 제작 예정/미완료 기록보다 이 결정을 우선하며, 새 요청 없이 다시 필수 제작 항목으로 제안하지 않는다.
- 기존 Dash 시작 연출·효과음, 0.25초 방어 GE/Task, 실제 차단 판정과 GameplayEvent.Dodge.Success/시작·종료·성공 hook은 유지한다. 이번 요청은 성공 판정 코드 삭제나 궁극기 충전/보상 추가 승인이 아니다. 방어 연결 후의 사용자 방향·몽타주·음향 확인은 이전 답변대로 아직 미검증이며, 연출 제외를 그 확인의 통과로 바꾸지 않는다.
- 다음 순서는 **R9-3 궁극기**다. 읽기 전용으로 확인한 `DCResourceSet`은 기본0/최대100 및 범위 제한/소유자 복제를 제공하고 PlayerState에 생성된다. legacy `DCCombatComponent::TryUltimate`는 별도 게이지의 최대치 도달 검사→조준 해제→게이지0→요청 delegate이며 Character는 BP_OnUltimateRequested로 전달한다. 이것을 새 GAS 궁극기의 실제 효과/획득 규칙 완성으로 보지 않는다. 관련 BP 실제 효과는 아직 미검증이다.
- 기존 원본 기반 GameplayAbility의 비용/Commit·GE를 재사용할 수 있으나 궁극기 효과·충전원/수치·소모/쿨다운을 확정한 근거가 없다. 원본 PlayerTagStack 비용은 정수 태그 스택이므로 ResourceSet의 float 자원과 그대로 동일하다고 가정하지 않는다. 로컬 Lyra Source/LyraGame 텍스트 및 ShooterCore 에셋 이름의 Ultimate 검색 결과만으로 모든 원본 BP에 동일 궁극기가 없다고 단정하지 않는다.
- **현재 R9-3은 기획 확인 대기/미구현**이다. 사용자에게 효과, 게이지 획득 방식, 소모량·별도 쿨다운을 확인했다. 정해진 기획을 받은 뒤 정확한 코드·에셋 변경안을 제시한다. 미정이면 명세대로 R9-3만 설계 대기로 유지하고 R10 적 GAS/공격 텔레그래프의 조사·변경안 선행을 제안한다. 이번에는 두 상태 문서만 갱신하며 코드·Config·에셋 저장·빌드·패키징은 하지 않는다.

---

## 0.63 최신 결정 — 2026-10-11 R9-3은 게이지 없는 캐릭터별 쿨다운형 고유 기술

- 사용자가 기존에 궁극기라고 부르던 기능을 **캐릭터별 실제 효과가 다른 고유 기술**로 설명했다. 게이지 획득/소모 규칙은 없고, 사용하면 일정 시간 쿨다운이 생긴다. VEILED EXPERTS의 시그니처/레인보우 식스 시즈의 특수 기술은 방향성 비유이며 해당 게임의 횟수·재충전·라운드 규칙까지 복제하라는 요청으로 해석하지 않는다.
- 이 결정은 과거 UltimateCharge/MaxUltimateCharge 및 ResourceSet 연동을 필수로 보던 기획보다 우선한다. 새 기술에 피해·처치·회피 성공 충전이나 게이지100 도달/소모 조건을 넣지 않는다. **기존 게이지 C++·HUD·Blueprint 참조는 조사/전환 승인 없이 삭제하지 않는다.** 기존 InputTag.Ultimate/Ability.Action.Ultimate/Cooldown.Ultimate 이름도 일괄 변경하지 않는다.
- 원본 근거: 로컬 UE5.8 GameplayAbility.cpp의 CommitAbility→CommitCheck/CommitExecute, CheckCooldown/GetCooldownTags, ApplyCooldown 및 원본 기반 DCGameplayAbility의 비용 경로를 확인했다. 기존 GAS 쿨다운 GE와 AbilitySet/InputTag 부여 체계를 재사용할 수 있다. 캐릭터별 효과는 프로젝트 기획이며 완성된 Lyra 고유 기술을 확인/이식했다고 주장하지 않는다.
- **미적용 변경안:** 기존 Q/InputTag 연결을 유지하고 캐릭터별 GA와 HasDuration 쿨다운 GE를 연결한다. CostGameplayEffectClass=None/AdditionalCosts 비움으로 게이지 비용을 배제한다. 사용 성립 시 Commit으로 쿨다운을 시작하고 실패 시 적용하지 않는 기본안을 제시한다. 효과/지속시간/쿨다운 수치는 캐릭터별 에셋에서 조정하며, 입력 차단·사망·취소·재사용을 검사한다. 최종 HUD는 R7/R13에서 충전 게이지가 아니라 사용 가능/남은 쿨다운 기준으로 연결한다.
- 공통 **무자원·쿨다운 규칙은 확정**, 첫 캐릭터의 실제 기술 효과/쿨다운 시간과 시전·중단 세부 규칙은 아직 미제공이다. 효과 없이 공통 쿨다운만 검사하더라도 전체 R9-3 완료로 표시하지 않는다. 기술별 대상 검증/취소 시 쿨다운 환불/사망·캐릭터 교체 때 쿨다운 유지 여부는 사용자 기획 확인 후 결정한다. 이번에는 읽기 전용 조사 및 문서만 갱신하며 게임 C++/Config/에셋·빌드·패키징은 변경/실행하지 않는다.

---

## 0.64 최신 재개 — 2026-10-11 R9-3 공통 쿨다운 기반·테스트 연결 검증 완료

- 사용자가 캐릭터 효과/시간은 미정으로 두고 **기반까지만 구현**하도록 승인했다. 기존 Lyra/GAS Ability·Commit·쿨다운 GE를 재사용해 **새 C++/Config 변경과 빌드·패키징 없이** Blueprint/데이터 에셋으로 구현했다. 생산용 기술 효과/쿨다운 밸런스·게이지 충전·보상·HUD는 만들지 않았다.
- 신규4개: `/Game/DreamCatcher/GAS/Abilities/Signature/GA_DC_SignatureBase`(추상 부모, 무비용·OnInputTriggered·Independent, Action/기존 Ultimate 분류 및 사망/입력차단), `/Game/DreamCatcher/GAS/Test/R9/Signature/`의 `GA_DC_SignatureSmoke`, `GE_DC_SignatureCooldownSmoke`, `AS_DC_SignatureSmoke`. **2초 쿨다운/0.35초 Task/CommitCount/Print는 검증용 자식에만** 있으며 부모에 생산용 시간·실제 효과를 지정하지 않았다. 미래 자식은 자신의 사용 성립 지점에서 CommitAbility를 호출하고 쿨다운 GE/효과/종료를 설정한다.
- 기존 통합 `DA_DC_RiflePawn`에 테스트 Set1개만 추가(6→7). 기존6개 Set·Death·Dash·사격/재장전·공용 입력/게이지 코드는 유지했다. `r9_signature_foundation.py`의 `signature_Apply_D`15:55:47 저장5개 및 `signature_Verify_A`15:57:40 새 프로세스 재로드/Compile/저장0 통과. 보호526경로 불변. 앞선 수학 노드 타입 확정/태그 Struct 비교 진단 실패3회는 저장0이다.
- **추가 사용자 승인:** 기존 IA_Ultimate/Triggered 경로에서 Q 유지2.6초→2초 쿨다운 종료 후 자동 재실행을 PIE_A에서 확인하고 선택을 요청했다. 사용자는 **한 번 누를 때 한 번만 실행**을 선택했다. `r9_signature_input.py`로 통합 전용 `IMC_DC_RifleBase`의 Q 행에 InputTriggerPressed1개(기본 threshold0.5)만 추가했다. `signature_InputApply_A`16:03:01 저장1개, `signature_InputVerify_A`16:04:38 재로드/저장0 통과. 공용 IA_Ultimate/다른 키/Config는 불변이다. 새 실제 맵에 연결할 때도 이 입력 계약을 적용하며 공용 IMC를 무단 변경하지 않는다.
- **최종 실제 PIE:** `Saved/Diagnostics/R9/pie_SignaturePIE_C.json`, `Saved/Automation/R9SignaturePIE_C/index.json`16:08:04 성공1/실패·보고서경고0/종료0. 게이지0 유지/무소모 실행, 쿨다운 중 재입력 거부·만료 후 재사용, 취소 시 Task 종료, Dash/입력차단 중 거부, 사망 시 실행 종료/사망 후 거부, 라이플 탄약30→27→30을 확인했다. Q 유지2.6초는 CommitCount2→3으로 **1회만 실행**됐다. 기존 엔진 기본 동작대로 Commit 후 취소·사망 직후 쿨다운은 유지하다 만료하며, 이것을 캐릭터별 최종 환불/재생성 정책으로 확정하지 않는다.
- PIE_B는 Action 주입이 IMC 트리거를 우회해 유지 테스트에서 실패했다(보고서 실패1). 게임 에셋을 재변경하지 않고, PIE_C에서 승인된 Q 매핑의 실제 Pressed 설정을 지속되는 runtime trigger에 복사해 엔진 입력 주입에 전달했다. 따라서 **Action＋매핑 트리거 자동검사이지 물리 Q키 검증은 아니다.** RenderOffscreen/Cue비활성/무음이며 기존 import/PoseAsset/드라이버 시작 경고는 남아 있다. 새 기술의 실제 시각/음향 효과·복제·패키징은 검증하지 않았다.
- **상태:** R9-3 공통 기반/통합 테스트 구현·자동검증 완료, 물리 Q키 확인은 대기. 실제 캐릭터 기술은 설계 대기이며 R9 전체 완료가 아니다. 재빌드 없이 L_DC_RifleIntegration에서 Q의 TEST 로그/2초내 거부/길게 누르면1회/놓고 재입력을 확인할 수 있다. 회피 성공 전용 연출은 계속 범위 제외, R9-2의 사용자 수용 확인과 R7/R13 HUD·장치/복제 경계도 유지한다. 다음 후보는 R10 적 GAS·공격 텔레그래프 조사와 변경안이며 이 작업에서 직접 구현하지 않았다.
- 최종 `signature_Verify_B`16:09:14에서 입력 추가 승인 기록까지 합쳐 새 프로세스 Compile/4에셋·PawnData/매핑 비교를 재통과했다(저장0/보호 파일 변경0/종료0). `r9_signature_foundation.py` 재검사에는 `-DCR9Applied=Apply_D -DCR9InputApplied=InputApply_A`를 함께 사용한다.

---

## 0.65 최신 재개 — 2026-10-11 R9-3 기반 사용자 확인 완료 / R10 변경안 제시

- 사용자가 **물리 Q키를 길게 눌러도1회만 실행되고 쿨다운 후 재사용도 정상**이라고 확인했다. 기존 `R9SignaturePIE_C`/`signature_Verify_B` 성공과 핵심7파일(신규4에셋·테스트 PawnData/IMC·DLL) 해시 불변도 대조했다. R9-3 공통 기반 묶음은 해당 사용자 수용까지 완료다. 실제 캐릭터별 효과/시간은 계속 설계 대기이며 R9 전체 완료가 아니다. R9-2 사용자 확인 대기를 이번 Q 확인으로 대신 닫지 않는다.
- **이번 범위는 R10 조사·변경안/문서뿐:** 게임 C++/Config/에셋 저장·빌드·패키징은 없다. `Saved/Diagnostics/R10/inspect_existing.py`로 대상 적/스포너/인카운터 BP3개와 원본 Bot Controller/Shoot service/Spawner BP3개를 저장 없이 조회했다. `target_A`16:30:00, `source_A`16:31:26 complete/저장0/검사대상 uasset3개씩 불변, 양쪽 로그 Error/ensure/Fatal0/종료0. 읽은 것은 현재 디스크 저장본이며 디자이너 main의 미커밋/원격 최신 상태나 맵 인스턴스 override를 확인한 것은 아니다.
- **현재 적 근거:** ADCEnemyCharacter는 ACharacter 직계/UDCHealthComponent 비GAS 체력, TryFireAtTarget의 즉시 Visibility Trace＋ApplyDamage, 사망 시 OnEnemyDeath1회→2초 LifeSpan이다. 저장 BP_EnemyCharacter는 HP100/이속350/사거리1400/간격1.25/피해8/DestroyDelay2, SKM_Quinn_Simple, 조회된 native mesh CDO의 AnimClass 미지정이며 피격 Print만 연결돼 있다. 전체 맵 배치/외부 컴포넌트·미저장 연출까지 없는 것으로 확대하지 않는다. Spawner는 TSubclassOf<ADCEnemyCharacter>/typed delegate, Encounter는 OnSpawnerCleared를 구독한다.
- **원본 근거·대안:** LyraPlayerBotController는 bWantsPlayerState=true, LyraPlayerState가 ASC/HealthSet/CombatSet을 소유하며 BotCreationComponent는 GameMode 초기화/RestartPlayer로 생성한다. 원본 Bot BP는 ExperienceReady/BT/Perception, BTS_Shoot는 Inventory/Equipment/탄약 검사 후 InputTag.Weapon.Fire 이벤트를 사용한다. 전체 원본 봇을 유지하는 대안은 이 PlayerState/GameMode/BT/장비 경로까지 통합해야 하며 이식 불가가 아니다. 현재 스포너의 단일 수명 NPC에 맞춘 **Pawn 소유 ASC**는 프로젝트 명세가 허용한 별도 대안이며, 원본과의 수명/처치 메시지 Target 차이를 명시하고 승인받는다.
- **권장 첫 묶음(미승인·미적용): R10-1 피격·사망·집계.** 기존 적을 먼저 바꾸지 않고 별도 `AI/DCGASEnemyCharacter.h/.cpp`에서 이미 이식된 ASC/HealthSet/CombatSet/LyraHealthComponent·Death Ability를 재사용한다. 적 Actor의 팀 인터페이스로 원본 팀/피해 계산을 보존한다. `DCEnemySpawner.h/.cpp`에는 선택적 GAS 적 클래스/전용 수신부를 추가하되 기존 EnemyClass/외부 OnSpawnerCleared·Encounter 계약을 유지하고 actor별 사망 집계를 멱등 처리한다. 새 `/Game/DreamCatcher/GAS/Test/R10/`의 적/스포너/Death 설정/AbilitySet 및 분리 테스트 맵에서 라이플 GE 피격→사망→다음 스폰/클리어를 검증한다. 기존 적/공용 BP·디자이너 맵·StageDirector·전역 GameMode 교체/legacy 삭제는 하지 않는다. 새 경로 검증 전 기존 경로를 제거하지 않는다.
- **첫 묶음 경계:** 원본 Death의 Start/Finish/Cancellation은 재사용하되 새 적 전용 데이터에서 플레이어 사망 카메라를 켜지 않고 기존 적 정리 시간2초를 보존하는 안이다. PlayerState 없는 적에서 원본 처치 메시지 Target이 null일 수 있으므로 기존 처치 UI 회귀를 확인하고, 문제 시 별도 최소 변경안을 제시한다. 레거시 체력과 GAS 체력을 이중으로 차감/구독하지 않는다. NPC 공격 전환/본격 예고 연출은 첫 묶음의 완료 항목으로 간주하지 않는다.
- **후속 R10-2/3:** 현재 즉시 공격을 의도→선딜→명중 구간→후딜로 나누고, 사망/취소/타깃 상실/엄폐 시 피해·대기 Task를 정리한다. 원본 BTS_Shoot를 텔레그래프 완성품으로 단정하지 않으며, 기존 원본 발사/피해/Task/Cue 재사용 가능 범위를 확인 후 구현안을 좁힌다. 공격 예고 때 조준점을 고정할지/언제까지 추적할지와 시간값은 변경안에서 명시하고 승인받는다. 예고 VFX/SFX/애니메이션은 아직 후보 미선정/미검증이다. 승인은 첫 묶음 단위로 받고 C++ 빌드는 사용자가 수행한다.

---

## 0.66 최신 재개 — 2026-10-11 R10-1 코드 작성 / 사용자 빌드 대기

- 후속 C++ 빌드에서 `DCEnemyGASAutomationTests.cpp:178`의 PlayerState.GetActorLocation 호출이 C2248로 실패했다. 엔진 PlayerState가 위치 함수를 숨기는 것을 확인하고, 발사 Pawn/거리 감쇠 검증이 없는 해당 fixture의 Context 원점만 FVector::ZeroVector로 명시했다. NPC source를 쓰는 별도 검사의 위치 조회와 실제 피해 계산 코드는 유지한다. 빌드/검사 실행은 하지 않았으며 사용자 재빌드 대기다.
- 후속 사용자 빌드에서 `DCGASEnemyCharacter.h:84`의 UFUNCTION 매개변수 `Instigator`가 AActor 멤버를 가려 UHT가 중단됐다. 해당 이름만 `DamageInstigator`로 교정했다. CPP의 미사용·무명 AActor* 매개변수와 타입/순서는 동일하며 동작 변경은 없다. Codex는 빌드를 실행하지 않았고 재빌드·검사 실행은 대기 중이다.
- 사용자가 0.65의 R10-1 묶음을 승인했다. 이번에는 **게임 코드7파일(신규4/기존3)과 상태 문서만** 작성했다. C++/UHT 빌드·패키징·Unreal 실행·새 에셋 생성/저장은 하지 않았다. 기존 맵/적 BP가 새 GAS 적으로 전환된 상태가 아니다.
- 신규 `AI/DCGASEnemyCharacter.h/.cpp`: ACharacter 직계의 별도 NPC가 기존 UDCASC/HealthSet/CombatSet/LyraHealthComponent를 소유한다. Game world의 PostInitializeComponents에서 Blueprint BeginPlay 전에1회 초기화한다. concrete 원본 파생 DeathAbilityClass가 필수이며, 선택적 추가 AbilitySet의 handle을 관리한다. Health/Combat/Death가 중복되면 실패 처리한다. 팀 인터페이스/기존 무기 Trace 채널·복제 기본 연결을 제공하지만 네트워크는 미검증이다. 공격/추적/텔레그래프는 아직 없고 legacy 체력 컴포넌트도 두지 않는다.
- DeathStarted에서 이동/충돌 정지 및 authority OnEnemyDeath1회→BP_OnDeath를 호출한다. 실제 사망 지연은 **후속 적 전용 Death BP의 Duration**이 소유하며 native actor에 별도2초 타이머를 중복 추가하지 않았다. DeathFinished는 다음 tick에 제거하고 EndPlay에서 구독·타이머·부여 능력/효과·ActorInfo를 정리한다. 테스트 Death fixture의0.05초는 production 설정이 아니다.
- `AI/DCEnemySpawner.h/.cpp`: 기존 EnemyClass는 유지하고 선택적 GASEnemyClass가 있을 때만 새 경로를 우선한다. 약한 Actor 집합으로 사망1회만 집계하고, deferred spawn의 BeginPlay 전에 구독/집계를 등록한다. 생성 중 즉시 사망한 경우 완료 통지를 다음 tick으로 미뤄 기존 Encounter 초기 등록 스택에 재진입하지 않는다. 잘못된 GAS 설정·사망 알림 없는 강제 제거는 실패로 정지하고 클리어로 보고하지 않는다. EndPlay는 구독/스폰 타이머만 해제하며 다른 살아 있는 적을 일괄 제거하지 않는다. 기존 EnemyCharacter·Encounter·StageDirector 소스는 불변이다.
- **추가 승인된 원본 최소 보완:** 원본 LyraTeamSubsystem.cpp349와 대상 CanCauseDamage는 두 NPC의 PlayerState가 모두 null이면 동일인으로 오인했다. 원본 전체 PlayerState 모델을 추가하는 대안/차이를 설명했고 사용자가 수정과 회귀 검사를 승인했다. `Teams/DCTeamSubsystem.cpp`의 자기 피해 예외만 실제 non-null 동일 Actor 또는 non-null 동일 PlayerState 비교로 보완했다. 그 밖의 팀 비교/피해 공식은 유지했다.
- 신규 `Tests/DCEnemyGASTestTypes.h`/`DCEnemyGASAutomationTests.cpp`: `DreamCatcher.R10.Enemy` **5건 작성**, `-DCR10EnemyIsolatedAutomation` 별도 프로세스 필요. 원본 Execution의 팀/피해·면역/Death·handle 정리, PS 없는 NPC의 동일/적대 팀·실제 자기/유효 PS 공유 판정, 스포너/인카운터·중복/즉시 사망, 잘못된 설정/강제 제거, legacy 경로/EndPlay를 포함한다. 전용 native fixture/임시 월드이며 production CDO·에셋을 수정하지 않는다. **작성만 했으며 컴파일/통과 결과는 없다.**
- 엔진 API/헤더·범위 대조, 공백/diff 검사는 수행했다. 초기 보호459파일 중 추가 승인된 TeamSubsystem1개를 제외한 **458파일 해시 불변**(나머지 소스·Config·공용/테스트 에셋·uproject·게임DLL)이다. 다음은 사용자 **Development Editor/Win64 빌드→R10 검사5건/관련 회귀→새 R10 적/Death/스포너/테스트 맵 에셋 생성·재로드·실제 라이플 PIE**다. DeathAbilityClass가 아직 지정된 새 적 BP가 없으므로 native 기본 클래스를 바로 배치해 완성 NPC로 테스트하지 않는다. 원본 적 카메라/정리2초 설정·처치 UI는 후속 에셋/PIE에서 검증하며 R10-2/3 공격/연출은 아직 미착수다.

---

## 0.67 최신 재개 — 2026-10-11 R10-1 native 검사 3성공·2실패 / 테스트 시간 교정·사용자 재빌드 대기

- 후속 사용자 승인으로 `Tests/DCEnemyGASAutomationTests.cpp`의 OriginalDamageAndDeath와 SpawnerAndEncounter에 Pending 활성화용 `TickTestWorld(.001f)`를 각각1회 추가했다. 기존0.06/0.02초 진행과 모든 assertion·실제 적/스포너/Death 로직·지연값은 유지한다. 정적 대조/공백 검사만 수행했으며 Codex 빌드·Unreal 실행·에셋 변경은 없다. **교정 후 컴파일/실행 미검증**, 다음은 사용자 Development Editor/Win64 재빌드→동일5건 재검사다. 아래3성공/2실패와 기존9성공은 교정 전 DLL 결과로 보존한다.
- 사용자 UBT `DreamCatcherEditor Win64 Development`는 **17:28:01 시작·8.60초 성공**, 게임 DLL **17:28:09 / 4,647,424 bytes**다. 앞선 UHT 이름 충돌/C2248 교정 후 컴파일·링크 성공을 확인했다. Codex는 빌드·패키징을 하지 않았다.
- **17:29:31 KST `Saved/Automation/R10EnemyNative_A/index.json`: 성공3/실패2/보고서경고0/notRun0.** TeamIdentityWithoutPlayerState, ImmediateDeathAndInvalidSetup, LegacyAndTeardown은 통과했다. OriginalDamageAndDeath는 DeathStarted까지 통과했으나 지연 제거/ASC 정리4항목이 실패했고, SpawnerAndEncounter는 다음 적 생성 확인1항목에서 실패했다. 프로세스 종료0을 전체 성공으로 해석하지 않는다.
- **실패의 시간 진행 근거:** 엔진 TimerManager.cpp712~725는 첫 Tick 전 SetTimer를 Pending에 넣고,1377~1388은 Tick 끝에서 현재 InternalTime을 더해 활성화한다. 교정 전 테스트는 Death의0.05초 타이머 등록 후0.06→0.02초만 진행해 만료 전 검사했고, 스폰0.01초 타이머도 첫0.02초 Tick 직후 검사했다. 엔진 WaitDelay/실제 적·스포너 시간을 바꾸지 않고 해당 테스트2곳에 Pending 활성화용 짧은 Tick을 추가하는 안을 제시한 뒤 위 승인으로 반영했다. **교정 후 실행 미검증**이며 실제 PIE 정상의 증거로 대신하지 않는다.
- **17:30:30 KST `Saved/Automation/R10EnemyRegression_A/index.json`: 기존9건 성공/실패·보고서경고0/notRun0.** Equipment/QuickBar1·TeamColor1·Weapon3·DashDefense4이며 로그 Error/ensure/Fatal0이다. native 임시 월드 검사이지 새 NPC 에셋/PIE·사용자 화면·복제 검증은 아니다.
- 위 검증 당시에는 상태 문서만 변경했고 보호419파일(Source/Config/uproject/게임DLL)의 SHA256 불변을 확인했다. 후속 승인은 위 테스트 CPP1파일 교정으로 한정했다. 새 R10 에셋 생성·저장/테스트 맵 변경은 하지 않았다. **R10-1 미완료**로 유지한다. 다음은 사용자 재빌드→실패2건 포함 native 재검사→기승인 새 R10 에셋/실제 라이플 PIE다. R10-2/3으로 넘어가거나 기존 적을 교체하지 않는다.

---

## 0.68 최신 재개 — 2026-10-11 R10-1 검증·사용자 화면 확인 완료 / R10-2 변경안 승인 대기

- 사용자가 새 `L_DC_EnemyIntegration`에서 적3마리의 피격·사망/약2초 제거/다음 등장·플레이어 카메라 유지가 **모두 정상**이라고 확인했다. Verify_A의 신규4에셋＋사용자 DLL5파일은 재대조에서도 불변이다. **R10-1은 합의한 테스트 연결 범위에서 사용자 수용까지 완료**로 기록한다. 실맵 전환·기존 구현 제거·공격/추적/애니메이션·복제/패키징·전용 처치UI는 별도 미완료다. 이번 후속 턴은 기존 근거 대조/상태 문서/다음 변경안뿐이며 코드·Config·에셋 변경/빌드/Editor 실행은 없다.
- 사용자 재빌드는 **17:34:10 KST 시작·8.20초 성공**, DLL **17:34:17 / 4,647,424 bytes**다. **17:35:06 `Saved/Automation/R10EnemyNative_B/index.json` 성공5/실패·보고서경고0/notRun0**, 동명 로그 Error/ensure/Fatal0이다. 이전 A의 시간 진행 실패2건을 포함해 Death 정리·스포너/인카운터·팀 판정·legacy 회귀가 통과했다. Codex 빌드/패키징은 없다.
- 기승인 `/Game/DreamCatcher/GAS/Test/R10/`에 **4개**를 새로 저장했다: `GA_DC_EnemyDeath`, `BP_DC_GASEnemy`, `BP_DC_GASEnemySpawner`, `L_DC_EnemyIntegration`. Death는 기존 이식 `GA_Hero_Death`를 상속해 원본 그래프/태그/Start/Finish를 유지하고 Camera Mode=None, Duration=기존 적 DestroyDelay2초만 설정했다. 적은 native DCGASEnemyCharacter/HP100/팀1이며 기존 Quinn Simple 메시·Z-85/Yaw-90·캡슐34/88·AnimClass=None을 보존한다. 중복 AbilitySet은 추가하지 않았다. 스포너3마리/동시1/간격1초, 새 맵의 기존 native Encounter가 자동 시작한다. 플레이어는 기존 RifleGameMode/PawnData를 재사용하며 공용 BP/디자이너 맵·Config·C++·기존 적은 불변이다.
- 일회성 `Scripts/Editor/r10_enemy_setup.py`의 Apply_A는4개 저장 후 위치 비교 API의 Python 인수 오류로 최종 판정 실패였다. 에셋을 재작성하지 않고 조회만 교정한 **Verify_A 별도 프로세스 재로드/Compile/설정·맵 연결 검사 complete=true·저장0**으로 확인했다. Inspect_A/B의 변수명 조회 실패도 저장0이다. **보호449파일 SHA256 불변**이며 신규4개도 Verify 이후 유지됐다.
- **실제 PIE 최종 `Saved/Diagnostics/R10/pie_PIE_B.json` complete=true**, `Saved/Automation/R10EnemyPIE_B/index.json` **17:47:22 성공1/실패·보고서경고0/notRun0**, 로그 Error/ensure/Fatal/빈 TapIDs0. 실제 InputAction→라이플 GA/Trace/GE로 같은 팀 HP100 유지, 적대 팀 피격/3마리 사망, 각 OnEnemyDeath1회, 스포너/Encounter 완료 각각1회,2.000~2.014초 후 DeathFinished→다음 tick 제거를 확인했다. 적 사망 시 플레이어 ViewTarget/비사망 상태 및 Reticle/AmmoHost 유지, R재장전30발·Dash20피해 차단/만료·Q유지1회/재입력·플레이어 사망도 통과했다. 같은 팀 설정은 PIE 인스턴스만0→1로 바꿨고 Q는Action+Pressed 트리거 주입이며 물리키/복제 검증이 아니다.
- 앞선 PIE_A도 관찰 assertion은 통과했지만 `-nosound`의 기존 WeaponAudioFunctions 빈 TapIDs 등 **검사 경고1410건**이 있어 무경고 성공으로 보고하지 않는다. B는 오디오 장치와 GameplayCue를 활성화했고 해당 경고가 재현되지 않았다. 기동 시 기존 PoseAsset/import/드라이버 경고까지 전부0이라는 뜻은 아니며 실제 화면·소리 품질 확인은 사용자 담당이다. 두 독립 PIE에서 반복 흐름을 통과했고 기존 native 회귀9건은17:30의 결과(이번에 재실행하지 않음)로 구분한다.
- 앞선 구현 턴의 작성은 위 에셋4개·제한된 생성/PIE 스크립트2개·상태 문서다. 당시 자동 검증 후 요청한 화면 확인은 위 사용자 응답으로 완료됐다. 적의 공격/추적/새 애니메이션은 아직 없고 R10-2/3·실맵 전환·legacy 제거·복제/패키징은 미완료다. 원본 EliminationMessage.Target은 PS 없는 NPC에서 null일 수 있는 계약을 그대로 유지한다. 현재 무기 Host 회귀를 전용 처치 목록 UI/R13 전체 검증으로 확대하지 않는다.
- 다음은 R10-2 변경안 승인이다. 원본 RangedWeapon은 Equipment SourceObject를 요구하고 현재 NPC에는 장비/AnimClass가 없으므로 원본 발사 GA 전체를 즉시 붙여 완료 처리하지 않는다. 원본 장비·봇 전체 연결 대안과, 기존 프로젝트 적 공격을 원본 GAS/WaitDelay/DamageExecution 기반의 선딜→명중→후딜로 옮기는 최소 확장을 비교한다. 조준 고정 시점·시간·취소/장애물 규칙을 먼저 제시하며 이번 턴에는 R10-2 코드를 작성하지 않았다.
- **R10-2 첫 묶음 제안(미승인):** 현재 피격 전용4에셋을 유지하고 별도 공격 테스트 자식/맵에서 서버 적 전용 단발 Ability를 연결한다. 기존8피해/1400사거리, 예고 시작 시 조준점 고정, 선딜0.5초→실제 Trace1회→후딜0.75초를 테스트 초기값으로 제안하며 최종 밸런스로 확정하지 않는다. 선딜 사망/취소/타깃 파괴·사망/가림은 공격을 취소하고, 고정 사선에서 벗어난 경우 강제 피해를 주지 않는다. 원본 Death의 능력 취소와 WaitDelay/GE·팀·면역 계산을 재사용하되 원본 라이플의 장비/탄약/퍼짐 전체 이식과 구분한다. 정지 상태 공격부터 검증하며 추적 이동/내비게이션·본격 VFX/SFX/Montage는 별도 범위로 남긴다. 제안 대상은 DCGASEnemyCharacter의 선택적 공격 연결, 새 EnemyRangedAttack Ability h/cpp·관련 검사, R10 하위 GA/GE/AbilitySet/공격 자식BP/별도 맵과 제한된 자동화다. 공용 무기/기존 적/기존4에셋/Config·태그/디자이너 맵은 변경하지 않는 안이며 구현 전 승인을 받는다.

---

## 0.69 최신 재개 — 2026-10-11 R10-2 첫 코드 묶음 승인·작성 / 사용자 빌드 대기

- 사용자가 R10-1의 최초1마리＋추가2회＝총3마리 순차 흐름을 재확인한 뒤 앞서 제시한 R10-2 코드 작업 진행을 승인했다. **게임 코드6파일(신규4/기존2)과 상태 문서만** 변경했다. Codex 빌드/UHT·패키징·Unreal 실행·에셋/Config 변경은 없다. R10-1 수용 상태는 유지하며 기존 맵의 적은 아직 공격하지 않는다.
- 신규 `AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.h/.cpp`: 원본 기반 DCGameplayAbility의 서버 전용/InstancedPerActor, 기존 Ability.Type.Action/사망·입력차단 태그, 엔진 WaitDelay2개를 사용한다. **조준점/발사 원점 고정→선딜0.5→Visibility Trace1회→후딜0.75**, 기본 SetByCaller 피해8/사거리1400이다. 원본 Equipment/Rifle 발사 Ability 전체 이식이 아니라 승인된 NPC 확장이며 원본 공통 GAS·Death·DamageExecution·태그 등록 코드는 수정하지 않았다.
- GE는 **Instant·직접 Modifier 없음·DCLyraDamageExecution1개·Source BaseDamage snapshot에 Additive SetByCaller.Damage1개** 계약을 확인한다. NPC CombatSet 기본 BaseDamage0에서 기본 피해8이며 기존 팀/면역·Health/Death 계산을 통과한다. Trace 미명중을 강제 피해로 바꾸지 않는다. 설정 누락/비정상 시간·범위·피해, 같은 팀/자기/죽은 타깃/가림·사거리 밖이면 시작을 거부한다.
- 선딜의 유효성/시야는0.05초 타이머와 발사 직전 재검사, 대상 Death/EndPlay로 감시한다. 취소·사망/입력차단·타깃 소실·가림 시 대기 Task와 그 Task 소유 타이머·delegate만 정리한다. 명중으로 타깃이 죽은 경우에는 후딜을 생략하지 않는다. Blueprint Intent/Shot/Ended 훅 및 Phase 조회를 제공하되 연출 에셋은 아직 연결하지 않았고 원본 Montage/Cue를 이식했다고 보고하지 않는다.
- 수정 `AI/DCGASEnemyCharacter.h/.cpp`: 선택적 RangedAttackAbilityClass와 기본false의 bAutoRangedAttack, 서버 TryStartRangedAttack을 추가했다. 능력은 기존 AbilitySets에서 정확히1개 부여받아야 하며 중복 grant를 만들지 않는다. 자동 요청은 기존 단일 플레이어 정책인 PlayerPawn0을0.25초마다 확인한다. 실제 재공격에는 후딜 종료 후 최대 한 확인 간격이 추가될 수 있고 추적/MoveTo/BT는 없다. Death/EndPlay 시 요청 타이머와 임시 타깃도 해제한다. null/false인 기존 R10-1 자식은 계속 피격 전용이다.
- 신규 `Tests/DCEnemyAttackTestTypes.h`/`DCEnemyAttackAutomationTests.cpp`: `DreamCatcher.R10.Attack` **6건 작성**, `-DCR10EnemyIsolatedAutomation` 필요. 선딜/단발/후딜·치명타 후딜/재사용, 고정 조준 회피/초기 및 도중 엄폐/범위 이탈, 외부·예고 콜백 중 취소/타깃 사망·제거, 원본 Death/공격자 EndPlay, 팀/면역/설정·입력차단, opt-in 자동 요청을 포함한다. 작은 월드 Tick으로 Pending 타이머 활성화 경계를 고려한다. **UHT/컴파일/검사6건 실행은 아직 미검증**이다.
- 정적 API/선언/공백·범위 검사만 수행했다. 허용한 NPC2파일 외 기존 보호 **440파일 해시 불변**, 신규 Source는 위4개뿐이다. 다음은 사용자 **Development Editor/Win64 빌드→R10.Enemy5＋R10.Attack6 및 관련 회귀→별도 공격 GA/GE/AbilitySet/자식BP/맵 에셋 연결·PIE**다. 새 클래스 컴파일만으로 기존 적이 공격하는 상태로 바뀌지 않는다. R10-3 본격 예고 연출·추적 이동/실맵/복제·패키징은 여전히 별도다.

---

## 0.70 최신 재개 — 2026-10-11 R10-2 빌드 성공·20건 중19성공 / 취소 테스트 교정·재빌드 대기

- 사용자 UBT는 **18:15:17 KST 시작·43.09초 성공**, DLL **18:15:59 / 4,747,776 bytes**다. `Saved/Automation/R10AttackNative_A/index.json` **18:17:18 결과는 성공19/실패1/보고서경고0/notRun0**이다. 기존 R10.Enemy5·관련 무기/장비/팀/Dash9 모두 통과, 신규 Attack6 중5개 통과/CancelAndTargetLoss1개 실패다. 종료0을 전체 성공으로 보고하지 않는다. R10-1 수용은 유지하며 R10-2는 미완료다.
- 실패 원인은 테스트의 `ASC.CancelAbility(F.Ability)` 호출이다. 엔진 AbilitySystemComponent.h1051은 CDO 인수를 요구하며 CPP1304~1312는 Spec.Ability 포인터와 비교한다. 테스트는 per-actor 실행 인스턴스를 전달하여 취소 대상이 일치하지 않았다. 첫 공격이 실제 실행돼 HP92/Shot1이 되었고 뒤의 Shot0 assertion3개도 누적값으로 실패했다. 실제 공격 코드의 다른 취소 경로 성공과 외부 취소 검증 실패를 구분한다.
- 사용자에게 근거/정확한 범위를 설명하고 **테스트 교정 승인**을 받았다. `Tests/DCEnemyAttackAutomationTests.cpp`1파일에서 GetCurrentAbilitySpecHandle의 유효성을 확인한 뒤 CancelAbilityHandle을 호출하고 즉시 IsActive=false 검사만 추가했다. 기존 피해/Shot/End 및 후속 assertion은 그대로다. 원본 엔진/게임 공격/Death/Config/에셋은 변경하지 않았다.
- **교정 코드의 UHT/C++ 빌드·검사 실행은 아직 미검증**이며 Codex 빌드/패키징은 없다. 새 공격 GA/GE/AbilitySet/BP/맵 자동화·저장은 시작하지 않았다. 다음은 사용자 Development Editor/Win64 재빌드→동일20건 재검증→통과 시 기승인 R10-2 공격 전용 에셋/분리 맵·PIE다. 검사 조건을 완화하거나 실패를 숨기지 않는다.

---

## 0.71 최신 재개 — 2026-10-11 R10-2 native20·신규5에셋 통과 / 실제 플레이어 충돌 누락 발견

- 사용자 재빌드는 **18:20:43 KST 시작·7.67초 성공**, DLL **18:20:50 / 4,748,288 bytes**다. `Saved/Automation/R10AttackNative_B/index.json` **18:21:50 성공20/실패·보고서경고0/notRun0**, 로그 Error/ensure/Fatal0이다. 취소 API 교정을 포함한 R10.Enemy5/Attack6와 기존 Equipment/TeamColor/Weapon/Dash9가 모두 통과했다. Codex 빌드/패키징은 없다.
- 승인된 `/Game/DreamCatcher/GAS/Test/R10/Attack/`의 GA_DC_EnemyRangedAttack·GE_DC_EnemyRangedDamage·AS_DC_EnemyRangedAttack·BP_DC_GASEnemyRanged·L_DC_EnemyAttackIntegration **5개를 새로 저장**했다. `r10_attack_setup.py` Apply_B18:26:01 complete/5에셋, Verify_A18:28:15 새 프로세스 재로드/Compile/설정·맵 비교 complete/저장0이다. Apply_A는 EditDefaultsOnly 구조체 설정 API 오류로 저장0이었고 기존 UE import 방식으로 교정했다. GE는 기존 Basic_SetByCaller 자식이며 Execution/Modifier/Cue를 보존한다. 적은 수용된 R10-1 BP의 자식/자동 공격true/선딜0.5·후딜0.75·피해8·범위1400, 새 맵은 공격 적1마리다. BP의 TEST 로그/Intent·Shot·End 카운터만 추가했으며 본격 R10-3 연출은 없다. 기존 R10-1의4에셋·디자이너 맵·게임 C++/Config는 불변이다.
- **실제 PIE는 실패/미완료:** `R10AttackPIE_A`에서 선딜 후 Shot은 실행되지만 실제 플레이어 HP가100에서 줄지 않았다. `attack_pie_AttackPIE_A.json` complete=false이며 후속 회피/엄폐/Dash/처치 시나리오는 아직 검증하지 못했다. native20 성공을 실플레이 성공으로 확대하지 않는다.
- **원인 확인:** `attack_pie_Trace_C.json`은 기존 플레이어 Capsule=Pawn, Mesh=CharacterMesh, 둘 다 Visibility Ignore임을 수집했다. 동일 사선은 원래 miss, PIE 인스턴스 응답만 임시 Block으로 바꾸면 실제 적 공격 **8피해**와 플레이어 메시(lowerarm_l) Hit가 확인됐다. 진단 후 원래 프로필을 복원했고 저장0/보호파일 변경0이다. `diagnostic_complete=true`, reason=trace_confirmed이지만 complete=false는 전체 전투 검사 통과가 아니기 때문이다. Trace_A/B의 Python HitResult 출력 API 실패는 조사 이력으로 보존한다. Verify 기준 **507파일 해시 불변**을 재확인했다.
- **원본 근거와 다음 변경안(미승인):** 원본 LyraCharacter 생성자는 LyraPawnCapsule/LyraPawnMesh를 사용하고, 원본 DefaultEngine.ini에 두 프로필이 정의돼 있으나 대상 Config에는 없다. 신규 적은 현재 Visibility로 실제 hit를 요구하므로 이 누락을 드러냈다. 임시 Visibility만 Block하는 대안도 피해는 확인했지만 원본의 무기/카메라/물리 응답 전체를 복원하는 것은 아니다. 권장안은 **Config/DefaultEngine.ini에 원본 프로필2개를 그대로 추가 + 통합 테스트 BP_DC_RiflePawn의 Capsule/Mesh 프로필만 각각 연결**이다. 원본대로 Capsule은 QueryAndPhysics→QueryOnly, 무기/카메라/물리 채널 응답도 원본값을 따른다. 크기/메시/이동 수치·공용 Pawn/CharacterMesh 프로필·기존 메인 플레이어BP·적 공격 C++/수치는 변경하지 않는다. R5/R8/R9/R10 통합 맵의 같은 테스트 플레이어에 영향이 있으므로 이동/점프/웅크림/카메라/발사·재장전/Dash와 적 실제 피해를 회귀 확인한다. 승인 전에는 Config/플레이어 BP를 수정하지 않는다.
- 다음은 이2파일 범위 승인→Unreal 내부 제한 저장/재로드→동일 R10 공격 PIE·플레이어 회귀다. C++ 변경/재빌드가 필요한 안이 아니며 사용자 에디터 동시 저장은 피한다. **R10-2 미완료**, R10-1 수용은 기존 테스트 범위로 유지한다. 추적/정식 예고 연출·실맵·복제/패키징 완료도 아니다.

---

## 0.72 최신 재개 — 2026-10-11 원본 플레이어 충돌 연결·R10-2 자동/사용자 검증 완료

- 사용자가0.71의 **원본 프로필2개＋통합 플레이어2컴포넌트** 변경안을 승인했다. `Config/DefaultEngine.ini` CollisionProfile에 LyraPawnCapsule/LyraPawnMesh의 원본 정의를 그대로 추가하고, `/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn`의 CollisionCylinder/CharacterMesh0에 각각 연결했다. 캡슐/메시 QueryOnly, Visibility Block, Camera Ignore 및 원본 무기 채널 응답을 확인했다. 크기·메시·애니메이션·이동 수치·공용 Pawn/CharacterMesh 정의·게임 C++는 수정하지 않았다. 추가2줄을 제외한 Config 내용도 동일함을 해시로 확인했고 민감정보는 출력하지 않았다.
- `Scripts/Editor/r10_player_collision.py`: Apply_A는 Compile로 재할당된 Health delegate의 메모리 주소만 달라 저장 전에 중단했다. 실제 수신 객체/함수 이름은 보존해 비교하고 주소만 분리한 **Apply_B18:53:41 KST 저장1개/complete**, **Verify_A18:55:31 새 프로세스 재로드/Compile/채널 비교 complete·저장0**을 확인했다. 충돌 BodyInstance에서도 프로필/응답/활성형식 이외의 물리 값이 동일했다. 실패본은 이력으로 보존한다.
- **R10-2 실제 공격 PIE 통과:** `Saved/Diagnostics/R10/attack_pie_AttackPIE_B.json` complete=true/보호파일 변경0, `R10AttackPIE_B/index.json` **18:58:49 성공1/실패·보고서경고0**. 임시 응답 보정 없이 저장된 프로필로 선딜0.500초 후 HP100→92, 후딜0.751초, 옆 이동으로 고정 사선 회피, 엄폐·외부 취소 후 무피해, Dash의 실제8피해 차단 delegate1회, 라이플 처치 시 선딜 중단·사망/스포너/Encounter각1회·2.009초 후 정리를 통과했다.
- **기존 조작 회귀 통과:** `r10_collision_regression_probe.py`가 기존 R8 Jump/전투 시퀀스를 재사용했다. `collision_pie_Jump_CrouchFix_Collision_A.json`55단계/473표본, 이동·프로필·카메라·전투12조건＋점프8조건 모두true, 저장0/보호파일 변경0이다. FOV80→70→80/시점 대상 유지, 서기·웅크림 점프/착지/재점프·공중 재입력 차단,4방향 이동/정지, 수동·자동 재장전/자동 재장전 후R·장비 전환/사망 정리를 확인했다. `R10CollisionRegression_A/index.json` **19:01:49 성공1/실패·보고서경고0**이다. 이 회귀는 Cue비활성/무음이며 물리키·화면 검증과 구분한다.
- **독립 회귀 재통과:** `R10CollisionNative_A/index.json` **19:01:38 성공20/실패·보고서경고0/notRun0**. R10.Enemy5/Attack6와 기존 무기·장비·팀·Dash9다. 최종 Unreal 로그 Error/ensure/Fatal0이고, 별도 콘솔에 발생한 IDE 연결 종료 메시지를 게임플레이 실패로 해석하지 않는다. 기존 startup PoseAsset/import 등 경고 해결까지 주장하지 않는다.
- 사용자가 새 `L_DC_EnemyAttackIntegration`에서 **이동·점프·웅크림·조준 카메라·대시가 자연스럽고 적 처치 후 제거도 모두 정상**이라고 확인했다. **R10-2의 정지 적 단발 예고 공격 기반/자동검증·사용자 수용 완료**다. 현재 맵은 공격 적1마리, 예고는 TEST 로그뿐이며 R10-3 실제 VFX/SFX/Montage·추적 이동·실맵·복제/패키징은 여전히 미완료다. 기존 R10-1의3마리 피격 맵도 유지한다.
- Codex는 C++ 빌드/패키징/기존 적 삭제를 하지 않았다. 이번 작성은 승인된 Config/BP와 제한된 스크립트2개·상태 문서다. 보호502파일 중501개는 불변, `DCHealthSet.h`는 작업 중 외부에서 `// Store the health before any changes` 주석 끝에 w1자가 추가됐다. 해당1자를 메모리상 제외한 해시가 작업 전과 같아 실행 코드 변경은 없음을 확인했으며 **사용자 변경으로 보존했다**. 이 소스를 이번 작업 변경으로 덮어쓰거나 되돌리지 않는다. 다음은 R10-3 예고 연출 변경안 제시이며 자동 승인/구현하지 않는다.

---

## 0.73 이전 제안 — 2026-10-11 R10-3 첫 시각/음향 묶음 / 0.74 결정으로 적용 중단

> 아래는 당시 조사·제안 이력이다. 사용자가 일반 적과 예고 공격을 분리하도록 결정하여 일반 적용 경고선 묶음은 적용하지 않는다. 최신 재개 기준은 0.74다.

- 사용자의 진행 요청에 따라 **변경안 준비만** 했다. `Saved/Diagnostics/R10/presentation_read.py`는 기존 조회기를 재사용한 제한된 읽기 전용 조사이며 게임 C++/Config/에셋 생성·수정/Compile/저장·빌드/패키징은 없다. target_A19:15:12 후보13개, source_A19:16:47 원본10개, source_Weapon_A19:31:09 원본 B_Weapon/B_Rifle2개, target_Effect_A19:33:49 공통 B_WeaponFire1개를 조회했다. 모두 complete/저장0/조회 파일 해시 불변이며 원본에 없는 Template Quinn_Simple은 missing으로 기록했다. 이것은 실제 효과/음향/포즈 실행 검증이 아니다.
- **확인한 재사용 후보:** 원본 NS_WeaponFire_Tracer/NS_WeaponFire_MuzzleFlash_Rifle, 공통 B_WeaponFire, 원본 MSS_Weapons_Rifle2_Fire. Tracer는 User.MuzzlePosition/ImpactPositions/Trigger를 사용한다. 원본 B_Weapon은 B_WeaponFire를 만들고, 대상의 공통 Actor는 ImpactPositions·Direction·Trigger를 Niagara에 전달하며 재트리거 지연3초 뒤 제거한다. 원본 TriggerFireAudio는 AudioComponent 생성/재사용 후 Fire 트리거를 보낸다. 단순 PlaySound 호출과 동등하다고 추정하지 않는다. 원본과 대상 에셋의 바이트 동일성/렌더·오디오 동일성은 미검증이며 B_WeaponFire는 적용 전 필요한 원본/대상 그래프 대조와 실제 실행을 확인한다.
- **전체 Cue 제약:** GCN_Weapon_Rifle_Fire는 Lyra Get Weapon→B_Weapon.Fire/TriggerFireAudio 및 EarlyReflections/WhizBy/SendWeaponFire를 호출한다. 현재 R10 NPC에는 그 장비/무기 Actor 연결이 없어 전체 Cue를 그대로 실행해 성공한다고 보지 않는다. 원본 장비·무기 계층까지 연결하는 대안은 가능하지만 별도 범위다. 이번 첫 제안은 기존 NPC의 Intent/Shot/Ended 훅에서 공통 발사 효과·MetaSound의 필요한 입력/수명을 연결하는 최소 어댑터이며 전체 무기 Cue/주변음/카메라셰이크·진동의 완전 동등성을 주장하지 않는다.
- **애니메이션 제약/보존 대안:** 현재 Quinn_Simple의 Skeleton은 `/Game/Characters/Mannequins/Meshes/SK_Mannequin`, 원본 Rifle Idle/Fire와 Hero Quinn은 `/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin`이며 양쪽 CompatibleSkeletons는 빈 배열이다. R10 NPC는 AnimClass=None이다. 원본 Fire는 Mesh-space additive, 몽타주0.533초/FullBodyAdditivePreAim Slot/AN_PlayWeaponMontage Notify가 있다. 기존 Skeleton에 호환 등록을 강제로 하거나 원본 Notify를 지우지 않는다. 별도 표현 자식에서 원본 Hero Quinn·AnimBP/무기 연결을 검증하거나 명시적으로 리타겟하는 대안이며 **현재 실제 재생/무기 Notify 준비는 미검증**이다. C++ DCAnimInstance의 GroundDistance 갱신도 DreamCatcherCharacter만 대상으로 하므로 전체 AnimBP를 무검증 대입하지 않는다.
- **첫 묶음 제안(미승인):** 0.5초 동안 잠긴 실제 사선 방향/범위의 빨간 경고선(색/폭 조정 가능), Shot 시 경고 제거＋원본 기반 탄도/발사원점 섬광/Fire 음원1회, 취소·사망·EndPlay 시 소유한 경고/효과/오디오 정리. 고정 예고선은 프로젝트 신규 표현 제안이다. 확인한 원본 탄도 재질은 Niagara Sprite 및 Sprite_Capsule/MF_Sprite 함수 기반이며 고정 예고선으로 그대로 사용할 수 있는지 아직 검증하지 않았다. 원본 유사 예고 자산의 부재/이식 불가를 단정하지 않으며, 새 단순 emissive 재질/비충돌 메시 경고선을 승인받는 안이다. 발사/선딜/후딜/피해/사거리/조준 고정 규칙은 변경하지 않는다.
- 대상은 새 `/Game/DreamCatcher/GAS/Test/R10/Presentation/` 아래 경고 재질·표현 Actor·GA/AbilitySet/적 자식·별도 맵의 **6개 에셋과 제한된 설정/검사 스크립트**다. 기존 Attack GA/맵/원본 FX·음원·공용 Skeleton은 보존한다. Blueprint/에셋 연결 우선이며 새 C++/Config/태그 변경이 실제로 필요하면 근거를 설명하고 추가 승인을 받는다. **이 묶음만으로 R10-3 전체 완료가 아니며**, 몸체·무기·원본 애니메이션 묶음을 미완료로 유지한다. 다음은 첫 시각/음향 묶음 승인→필요한 공통 Actor 입력 사전 검증→에셋 연결/재로드/PIE→사용자 화면·소리 확인이다.

## 0.74 확정 방향 — 2026-10-11 일반 적 원본 사격 / 강공격·보스 예고 분리

- **최신 사용자 결정:** 일반 적은 Lyra 원본처럼 사격하며, 예고는 강한 단발 공격이나 보스 패턴에만 적용한다. 아래 과거의 모든 적 공격 선딜 분리·즉시 피해 제거 지침보다 이 결정이 우선한다. 플레이어 사격·재장전·조준은 변경 대상이 아니다.
- **상태 재분류:** R10-1의 피격·사망·스포너 검증은 유지한다. 기존 R10-2의 조준 고정/선딜0.5초/후딜0.75초/피해8/사거리1400 구현·검증은 **별도 예고 공격 프로토타입**의 결과로 보존한다. 일반 적의 원본 무기 발사 완료를 뜻하지 않는다. 이 시간값·빨간 경고선은 원본 규칙이나 강공격/보스의 확정 기획값이 아니다.
- **중단 범위:** 0.73의 Presentation6에셋 생성 전 읽기 전용 확인 중 방향이 변경됐다. Presentation 폴더/에셋은 생성하지 않았으며 기존 Attack 코드·5에셋/맵은 삭제·수정하지 않는다. 강공격/보스별 예고 방식·시간이 정해지면 그 범위의 연출안을 다시 제시한다.
- **다음 우선 작업:** 기이식 Inventory/Equipment/라이플 발사·재장전 Ability를 별도 일반 적에 연결하는 변경안이다. 기존 예고 Ability의 선딜만0으로 바꿔 원본 발사라고 보고하지 않는다. 원본 BTS_Shoot의 장비·탄약 확인/발사 이벤트와 실제 무기 Ability의 Controller·Equipment SourceObject·Item Instigator 계약을 보존한다. 현재 NPC의 장비/아이템·원본 몸체/AnimBP 연결은 미검증이므로 먼저 대조한다. 원본 전체 봇/BT/PlayerState 수명 도입은 자동 포함하지 않는다.
- 이번 방향 반영은 두 상태 문서와 읽기 전용 소스 확인뿐이다. 새로운 게임 코드·Config·에셋 수정이나 빌드·패키징은 하지 않는다. 실제 일반 적 연결은 정확한 변경 경로·필요한 최소 어댑터·원본 대비 차이·검증 범위를 제시한 후 승인받는다. R10 전체와 일반 적 사격/강공격 연출/실맵·복제·패키징은 미완료다.

## 0.75 이전 제안 — 2026-10-11 일반 적 최소 Controller / 원본 AI 묶음 이식으로 대체

> 사용자가 이후 Lyra AIController 및 AI 행동 묶음 자체의 재사용을 명시했다. 아래 자체 Controller/판단 타이머/정지 적5에셋 안은 작성 전 대체됐으며 최신 범위는0.76이다.

- 사용자 진행 요청에 따라 변경안을 구체화했다. 게임 코드·Config·에셋은 수정하지 않았고 Editor/빌드/패키징도 실행하지 않았다. 기존 R8 source_C/target_C 그래프와 현재 소스를 재사용해 조사했으며 B_WeaponInstance_Base·GA_Weapon_ReloadMagazine·ABP_Mannequin_Base의 대상 uasset 해시가 당시 조사본과 같음을 확인했다. 과거 보고서 전체가 최신이라고 주장하지 않는다.
- **실제 제약:** B_WeaponInstance_Base.OnUnequipped는 원본 Hero 타입의 HealthComponent를 읽고, 이식본은 DreamCatcherCharacter 타입을 요구한다. 현재 ACharacter 직계 DCGASEnemyCharacter는 해당 타입이 아니다. DCAnimInstance의 GroundDistance 갱신 역시 DreamCatcherCharacter/DCCharacterMovement를 사용한다. 이에 일반 적은 별도 DreamCatcherCharacter 파생 BP＋PlayerState ASC/PawnExtension/Hero를 쓰는 안을 권장한다. 기존 단순 NPC의 ASC 소유 구조를 일괄 변경하지 않는다. 현재 NPC 유지＋무기 해제/AnimInstance 최소 보완도 가능하지만 원본 소비처 변경이 더 필요하며 불가능한 대안은 아니다.
- **제안 코드 범위:** 신규 AI/DCRifleEnemyController.h/.cpp(엔진 AIController 기반, 원본 봇의 PlayerState 소유/팀 연결과 기존 Inventory·QuickBar·WeaponState 사용, 준비 후 장착/발사 요청/해제·단일 수명 정리), 기존 AI/DCEnemySpawner.h/.cpp에 선택적 RifleEnemyClass 경로와 원본 HealthComponent.DeathStarted1회 집계 추가, 관련 Tests/DCRifleEnemyAutomationTests.cpp 및 필요한 테스트 전용 타입1파일. 플레이어 Character/Controller·원본 발사/재장전/피해/무기 공용 코드·Config·기존 예고 Ability는 변경하지 않는 안이다. 추가 의존성이 확인되면 이 범위를 임의 확대하지 않는다.
- **제안 에셋 범위:** 새 `/Game/DreamCatcher/GAS/Test/R10/RifleEnemy/` 아래 BP_DC_RifleEnemy, BP_DC_RifleEnemyController, DA_DC_RifleEnemyPawn, AS_DC_RifleEnemyBase, L_DC_RifleEnemyIntegration의5개와 제한된 설정/PIE 스크립트. 원본 Manny/AnimBP/LinkedLayer, 기이식 ID_Rifle→WID_Rifle→발사/재장전/AutoReload·Cue를 공유하고 무기 데이터·기존 승인된 보완을 되돌리지 않는다. 기존 적 Death BP(Camera=None/Duration2)를 사용하고 별도 테스트 맵에 정지 적1마리만 연결한다. 플레이어 전용 Q/Dash/ADS·UI·입력은 적에게 임의 부여하지 않는다.
- **원본과의 차이/검사:** 장비·탄약·퍼짐·발사 간격·재장전/Notify·피해·발사 연출은 기존 원본 이식 경로다. AI의 대상 선택/준비/스포너 정리는 프로젝트용 최소 어댑터이며 전체 Perception/BT/Experience/봇 재생성의 이식이 아니다. 테스트에서는 기존처럼 Player0/획득거리1400을 사용하되 원본 무기 MaxDamageRange를1400으로 덮어쓰지 않는다. BTS_Shoot의 Interval0.5/RandomDeviation0.1은 AI 판단 주기이지 고정조준 선딜이 아니며, 그 기준으로 요청하고 무기 Ability가 실제 발사 간격/비용을 결정한다. 신규0.5초 예고/0.75초 후딜/피해8은 일반 적에 적용하지 않는다. 검증은 단일 장착/SourceObject·Item 연결, 실제 반복 사격·탄약 감소/자동 재장전·Notify, 엄폐·타깃 사망·적 사망·해제 후 중단/자원 정리, 스포너 집계1회 및 플레이어 회귀다. 실제 NPC 실행은 아직 미검증이며 구현 승인→사용자 빌드→새 에셋·PIE→화면/음향 확인 순서다.

## 0.76 조사 이력 — 2026-10-11 Lyra AI 자체 이식 결정 / 실제 의존성 조사

- **사용자 결정:** AI는 플레이어처럼 대시·웅크림을 별도로 사용하도록 만들 필요가 없으며 Lyra AI를 그대로 가져온다. 신규 AIController/표적 선택/사격 판단 타이머 재구현안 대신 **원본 LyraPlayerBotController＋B_AI_Controller_LyraShooter＋BT/Blackboard/Perception/EQS/서비스**를 재사용한다. 일반 적에 프로젝트 선딜/고정 조준/경고선을 넣지 않는0.74 결정은 유지한다. 원본 AI에 없는 플레이어 전용 행동을 추가하지 않으며 공용 능력/플레이어 기능을 삭제하라는 권한으로 해석하지 않는다.
- **실제 조회:** 기존 읽기 전용 presentation_read.py를 재사용해 원본 Bot 폴더21에셋을 조회/텍스트 export했다. `Saved/Diagnostics/R10/presentation_source_AI_B/report.json`은 complete=true/21개/missing0/asset_saves0/보호 파일 변경0, `Saved/Logs/R10LyraAI-source-B.log`의 Error/ensure/Fatal 검색0이다. AI_A는 초기 Registry 조회에서21개가 모두 missing이라 유효한 조사 성공이 아니며, Bot 경로 동기 스캔 후 AI_B에서 확인했다. 원본/대상 게임 코드·Config·바이너리 에셋 저장·빌드/패키징은 없다.
- 원본 실제 BT에는 적 감지/탐색, 이동 위치 EQS, MoveTo, 사격, 재장전, 탄약 소진 시 무기 탐색이 있다. 조회한21개 텍스트/그래프에서는 Dash/Crouch/Jump 호출을 찾지 못했으며 전체 게임의 부재를 단정하지 않는다. 주 Controller의 Bot 하위 의존 묶음은15개이며 폴더21개 전체를 실행 필수로 간주하지 않는다. 별도 Passive/성능 테스트/EQS Tester/거점용 Query는 주 전투 트리와 구분한다.
- **보존에 필요한 연결:** WaitForExperienceReady/ObserveTeam native, GameMode.ControllerCanRestart와 PlayerSpawningManager/PlayerStart, BotCreation의 Experience 완료/GenericPlayerInitialization/RestartPlayer, BTDecorator의 CanPlayerBotsAttack/원본 개발 설정이 필요하다. 원본 무기 탐색 분기는 B_WeaponSpawner/B_AbilitySpawner 및 LyraWeaponSpawner/PickupDefinition에도 의존한다. 현재 없는 기능을 삭제·상수true·무한탄약으로 숨기지 않는다. 이미 있는 ExperienceManager도 실제 로드 선택 연결과 별도이며, AI 실행에 직접 필요한 R13/픽업 선행 범위를 설명하고 승인받는다.
- **다음 작업 기준:** 원본 Controller를 DCLyraPlayerBotController 별도 이름으로 최소 타입/include/모듈 대응 이식한다. 서버 재시작 함수를 삭제하지 않고 필요한 원본 GameMode 계약을 함께 연결한다. AI BP/BT/EQS는 새 전용 경로에서 노드·핀/참조/기본값을 비교하고 기이식 무기·Character/PlayerState를 재사용한다. 원본 BotCreation의 경쟁전 봇 생성/재시작과 DreamCatcher의 유한 인카운터 수명 연결은 분리 설명한다. 전체 ShooterCore 게임 모드/거점 규칙/성능 테스트를 무조건 활성화하지 않는다. 기존 맵/스포너·플레이어·예고 프로토타입의 활성 경로는 검증 전에 덮어쓰지 않는다. **아직 이식 코드/에셋은 미작성·실행 미검증**이며 새 의존성이 포함된 정확한 변경안을 승인받는 것이 다음이다.

## 0.77 작성 이력 — 2026-10-11 Lyra AI 필수 native 이식 / 후속 결과는0.78

- **승인 범위:** 사용자가 원본 AI와 필수 GameMode·Experience·픽업 및 Config 이식을 승인했다. 향후 온라인 PVP 계획은 있으나 **온라인 호스팅·리플레이는 후속 작업**으로 명시적으로 보류했다. 기존 플레이어와 테스트 맵을 유지하고 C++ 빌드는 사용자가 한다. 이번에는 코드/Config만 작성했으며 Unreal 실행·에셋 생성/저장·빌드·패키징은 하지 않았다.
- **원본 기반22파일 추가:** Player/Lyra의 DCLyraPlayerBotController·DCLyraPlayerSpawningManagerComponent·DCLyraPlayerStart, GameModes/Lyra의 DCLyraBotCreationComponent·DCLyraAsyncAction_ExperienceReady·DCLyraWorldSettings, Teams/DCLyraAsyncAction_ObserveTeam, Weapon/Lyra/DCLyraWeaponSpawner, Equipment/Lyra/DCLyraPickupDefinition(기본/무기 픽업2타입), Development/DCLyraDeveloperSettings, System/DCLyraDevelopmentStatics의 각h/cpp. 원본 팀/ASC 해제/재시작/Experience 대기/픽업 권한·쿨다운·FX 경로를 보존하고 이름/include/module export·기이식 타입만 대응했다.
- **별도4파일 추가:** GameModes/Lyra/DCLyraGameMode h/cpp는 원본 gameplay/AI 함수19개 중18개를 타입 치환 외 그대로 사용한다. 선택 함수는 온라인 로그인 대신 새 GameMode의 명시적 DefaultExperience를 최종 fallback으로 사용하며 원본 프로젝트 B_LyraDefaultExperience를 강제로 선택하지 않는다. 생성자도 기이식 클래스와 신규 Player/Lyra/DCLyraExperiencePlayerState를 선택한다. 이 PlayerState 파생형만 원본 ExperienceReady→PawnData 부여를 복원하고 기존 ADCPlayerState의 ASC/속성을 재사용하며 중복 부여하지 않는다. 기존 GameMode/PlayerState/Character·Controller 소스는 불변이다.
- **원본 대비 명시 차이/후속:** GameMode.TryDedicatedServerLogin/HostDedicatedServerMatch/OnUserInitializedForDedicatedServer, 온라인 GameSession/ReplaySpectator 기본값은 아직 이식하지 않았다. 이후 원본 GameSession/UserFacingExperience/CommonSession 연결을 대조한다. 개발 설정의 플레이어 진동 필드는 보존하되 아직 없는 LyraPC.ShouldAlwaysPlayForceFeedback CVar 메타데이터를 연결하지 않고 편집 비활성화했다. 엔진 DeveloperSettingsBackedByCVars가 이 미등록 CVar를 로드 시 Error로 기록함을 소스에서 확인했으며 가짜 CVar/플레이어 기능 변경으로 우회하지 않았다. 봇 공격 허용/생성 수 설정과 그 소비 함수는 원본 그대로다.
- **기존 변경3파일:** Build.cs에 AssetRegistry 및 Editor 한정 MessageLog 의존성을 추가했다. DefaultEngine.ini에는 새 AI/스폰/픽업/GameMode/Experience 타입의 CoreRedirect18개만 추가하고 기존 매핑을 유지했다. DefaultGame.ini에는 DCLyraExperienceDefinition/ActionSet의 검색 경로를 `/Game/LyraMigration/AI`, `/Game/DreamCatcher/GAS/Test/R10/LyraAI`로만 추가했다. 기본 GameMode/WorldSettings/GameInstance·기존 맵·온라인 서비스 설정은 변경하지 않았다. 신규 DefaultExperience는 아직 미지정이며 새 AI 맵을 만들 때 실제 에셋을 연결해야 한다.
- **검사 코드2파일/4건 작성:** Tests/DCLyraAIFoundationTestTypes.h 및 DCLyraAIFoundationAutomationTests.cpp의 `DreamCatcher.R10.LyraAI.NativeDefaultsAndSettings`, `TeamAndUnPossess`, `ExperienceWaitContract`, `PickupCooldown`. 별도 unattended Editor의 `-DCR10LyraAIIsolatedAutomation`을 요구한다. 임시 native 월드/설정 복원만 사용하며 실제 BP·BT/EQS·Experience 로드 완료/픽업 GiveWeapon·복제 검사가 아니다. **UHT/C++ 컴파일 및 검사4건 실행은 아직 미검증**이다.
- **정적 확인:** 이름/include/명시적 메타데이터 차이를 허용한22파일 대조 및 GameMode18함수 본문 대조 통과, 새28파일의 quoted include 경로/마지막 generated include 확인, CoreRedirect 중복 없음. 보호518파일 중 변경은 위 Build.cs/Config2개뿐이고 나머지515개(기존 게임 소스·플레이어/테스트 에셋·DLL)는 해시 불변이다. `Saved/Diagnostics/R10/lyra_ai_native_static.json`, `lyra_ai_native_before.json`에 근거를 남겼다. 정적 성공을 빌드/실행 성공으로 보고하지 않는다.
- **다음:** 사용자 Development Editor/Win64 빌드→새4건 및 관련 R10/R5/R9 회귀→원본 AI15에셋과 직접 참조 픽업 에셋을 새 경로에 이식/재로드→별도 GameMode/Experience/PawnData·NavMesh 테스트 맵에서 실제 감지/이동·사격·재장전·무기 탐색·사망 정리. 기존 스포너/플레이어/맵 연결은 아직 교체하지 않았으며 R10 전체 미완료다. 온라인 PVP 기반 재사용 의도와 온라인 동작 완료를 구분한다.

## 0.78 검사 이력 — 2026-10-11 Lyra AI native23/24 / 후속 교정 결과는0.79

- 사용자 빌드는 **20:41:43 KST 시작·71.51초·Result:Succeeded**, DLL **20:42:54 / 5,011,968 bytes**로 확인했다. Codex는 빌드/패키징을 실행하지 않았다.
- 별도 에디터의 `Saved/Automation/R10LyraAINative_A/index.json` **20:54:55 KST 결과는 성공23/실패1/보고서 경고0/notRun0**이다. 새 LyraAI4건 중 NativeDefaultsAndSettings/ExperienceWaitContract/PickupCooldown3건과 기존20건은 통과했다. TeamAndUnPossess에서 PlayerState 팀1→2 변경 후 관찰값이1로 남은 assertion1건이 실패했다. 이후 재바인딩/해제 조건에는 추가 실패가 없었다. 프로세스 종료0을 전체 성공으로 해석하지 않는다.
- 엔진 `Actor.cpp:1513`의 ProcessEvent는 World.AreActorsInitialized()가 false이면 Actor의 동적 콜백을 실행하지 않는다. 해당 테스트는 CreateTestWorld만 하고 BeginPlayInTestWorld를 호출하지 않아 봇 OnPlayerStateChangedTeam 전달이 차단되는 구성이다. UObject 관찰자의 초기 통지는 이 Actor 경로를 거치지 않는다. 이 근거를 설명하고 **테스트 초기화 교정 승인**을 받았다.
- 승인 후 **Tests/DCLyraAIFoundationAutomationTests.cpp 한 파일**의 TeamAndUnPossess에 기본 AGameModeBase 선택→BeginPlayInTestWorld→AreActorsInitialized 확인을 추가했다. 팀1→2 assertion을 포함한 기존 검사 조건과 원본 Controller/팀/ASC/픽업 코드는 그대로다. **교정본 컴파일/실행은 아직 미검증**이며 사용자 재빌드가 필요하다.
- 검사 전 보호546파일은 실행 종료 후 모두 해시 불변이었다. 교정 후에는 승인한 테스트 CPP만 달라졌고 게임 소스·Config·에셋·맵·DLL은 유지한다. 근거는 `Saved/Logs/R10LyraAINative-A.log`, `lyra_ai_postbuild_A_before.json`이다. 원본 AI/픽업 에셋 저장·새 맵 연결은 아직 하지 않았다.
- 다음은 사용자 재빌드→동일24건 재검증→통과 시 기승인 원본 AI/픽업 에셋 이식과 별도 Experience/맵 연결이다. 전체 R10/원본 AI 플레이·복제/온라인 호스팅·리플레이/패키징을 완료 처리하지 않는다.

## 0.79 작성 이력 — 2026-10-11 Lyra AI native24건 / 픽업 GamePhase 의존성 (후속0.80)

- 사용자 재빌드는 **20:59:16 KST 시작·7.60초·Result:Succeeded**, DLL **20:59:23 / 5,012,480 bytes**다. `Saved/Automation/R10LyraAINative_B/index.json`은 **21:06:49 KST 성공24/실패·보고서경고0/notRun0**, 로그 Error/ensure/Fatal0이다. TeamAndUnPossess의 실제 월드 초기화 교정도 통과했다. 원본 AI BP/BT·픽업·PIE 전체 완료와 구분한다. 빌드/패키징은 Codex가 실행하지 않았다.
- 다음 에셋 작업의 원본 B_WeaponSpawner/B_AbilitySpawner2개를 저장 없이 조회했다. `presentation_source_Pickup_A/report.json` **21:07:57 complete/저장0/missing0/보호 파일 변경0**이다. B_AbilitySpawner는 B_WeaponSpawner 자식이며, 무기 탐색 EQS가 둘을 참조한다. 원본 GiveWeapon의 **LyraGamePhaseSubsystem.IsPhaseActive(ShooterGame.GamePhase.PostGame)** 조회와 미등록 픽업 Cue 태그, 이미 이식한 실패 메시지/Verb helper의 미등록 타입 매핑을 확인했다. 조회 실패를 이유로 원본 분기를 삭제하거나 false 상수로 바꾸지 않는다.
- 근거와 범위를 설명한 뒤 사용자가 **필수 의존성 추가 진행**을 승인했다. 새 AbilitySystem/Phases/DCLyraGamePhaseSubsystem h/cpp·DCLyraGamePhaseAbility h/cpp·DCLyraGamePhaseLog.h **원본5파일**을 이름/기존 DC 타입/include만 대응해 추가했다. 원본 중첩 단계·종료·관찰 및 GameState ASC 경로를 보존한다. Subsystem은 Game/PIE에 생성되지만 어떤 phase도 자동 시작하지 않는다.
- DefaultEngine.ini에 Phase2클래스/enum, 기존 DCVerbMessageHelpers/기존 DCAbilitySimpleFailureMessage의 **Redirect5개**를 추가했다. DefaultGameplayTags.ini에 원본 ShooterCoreTags.ini의 **ShooterGame.GamePhase.PostGame, GameplayCue.ShooterGame.Interact.WeaponPickup 2개**를 추가했다. 기존 매핑·태그를 유지하며 현재 플레이어/스포너/맵·GameMode·게임 종료 흐름은 바꾸지 않았다.
- **검사2건 작성/미실행:** Tests/DCLyraGamePhaseTestTypes.h 및 DCLyraGamePhaseAutomationTests.cpp의 `DreamCatcher.R10.LyraAI.PhaseDataValidation`, `PhaseNestedLifecycleAndGate`. 원본 태그 유효성, 자동 단계 미시작, 부모·PostGame 자식 공존, 자식 취소 시 픽업 gate 해제/부모 유지, 종료 통지를 임시 GameState ASC에서 확인하는 코드다. 실제 픽업 BP/BT/EQS/복제/전체 라운드 검사가 아니다. 새 코드의 UHT/C++ 컴파일 및 실행은 **사용자 재빌드 대기**다.
- 원본5파일 변환 대조/정적 확인을 했으며 근거는 `lyra_ai_phase_static.json`이다. 이번 실행 전 보호546파일 중 작업 종료 시 변화는 승인한 Config2개뿐이며 기존 소스·플레이어/맵/무기 에셋·DLL은 불변이다. 신규7소스와 상태 문서, 한정 읽기 전용 진단만 추가/갱신했다. 에셋 생성·저장은 아직0이다.
- 다음은 사용자 재빌드→기존24＋새Phase2=**26건** 검증→승인된 원본 AI/픽업 에셋 이식·재로드와 별도 Experience/맵 연결이다. 온라인 호스팅·리플레이 보류와 기존 활성 경로 보존은 유지한다.

## 0.80 검증 이력 — 2026-10-11 native26건 / 원본 AI·픽업62개 (다음 연결 조사0.81)

- **사용자 빌드/검사:** UBT21:19:37 KST 시작·40.84초 Succeeded, 게임 DLL21:20:18/5,078,528 bytes. `Saved/Automation/R10LyraAINative_C/index.json`21:21:54 **성공26/실패·보고서경고0/notRun0**, 로그 Error/ensure/Fatal0. 기존24＋Phase2 모두 통과했다. Codex는 C++ 빌드·패키징을 실행하지 않았다.
- **실제 이식:** 원본 AI/BT/BB/EQS/서비스·BotSpawner·픽업 BP/데이터/GE/Cue 및 시청각 의존성 **62개**를 Unreal Migrate→RenameAssets로 `/Game/LyraMigration/AI/{ShooterCore,Game}/...`에 가져왔다. 기존 에셋 덮어쓰기·원본 프로젝트 저장·바이너리 외부 복사는 없다. 원본 ID_Rifle은 이미 검증한 R5/R6 준비본을 참조하며 기존 무기 보완을 유지한다. 기존 공용5개는 값/태그/텍스처 EXR 등 대조 후 재사용했고 실제 음향·화면 동등성까지 검증한 것은 아니다.
- **워크플로 변경 승인/실패 이력:** 일반 Duplicate API는 EQS 생성자 오류와 편집 정보 차이를 검출해 Prepare_A에서 **저장0**으로 중단했다. 사용자가 **Migrate 후 전용 경로 이동**을 승인했다. MigrateOriginal_A21:53:28, Relocate_A21:57:31은 성공/정상종료0이다. target 최초 로드의 미등록 InventoryManager·TeamSubsystem·TeamComparison은 기존 이식 타입3개를 **프로세스 한정** 대응해 해결하고 새 에셋에 저장했다. 영구 Config/C++ 변경은 없다.
- **원본 값/참조 보존:** 프로젝트 기본값 차이로 달라진 신규 BP4개의 `bReplicateUsingRegisteredSubObjectList`를 원본true로 보존했다(전역 설정 미변경). 새 픽업 MetaSound3개의 공용 Random 음원 참조 ID와 이동한 BaseLayer의 참조 캐시를 Unreal 내부에서 정리했다. Finalize_B22:04:32 저장3/정상종료0, 공유 음원 불변, 문서는 정확한 참조 ID 대응 외 동일함을 확인했다. 실제 재생 청취는 미검증이다.
- **최종 정적 검증:** `Saved/Diagnostics/R10/ai_assets_Verify_C/report.json`, `Saved/Logs/R10LyraAI-assets-Verify_C.log` **22:08:24 KST**: 임시 Package/Class 대응 없는 새 프로세스에서62개 로드, Blueprint14개 Compile, 저장0/정상종료0, Error/ensure/Fatal0. 옛 package 의존0, 중간 redirector0, 보호 경로 변경0. 작업 전 **기존553파일 해시도 불변**이다. 기존 GA_Weapon_AutoReload의 LyraGame import 경고는 남아 있으며 새 무경고 실행으로 보고하지 않는다.
- **비교 범위/남은 문제:** 원본 그래프·기본값/필드 비교는 타입·경로·MetaSound ID 대응과 이름 있는 CDO 컴포넌트 출력 순서를 구분했다. Compile이 메모리에서 버리는 미연결 pure BreakStruct1개는 실행 연결이0이며 최종 검증에서 저장하지 않았다. BB/4EQS 내부 export도 일치한다. **BT 편집용 ClassData.GeneratedClassPackage7곳은 옛 문자열로 남아 있다**(원본에도 오래된 CanFireWeapon 이름1개 존재). 실제 NodeInstance는 이식된 클래스 경로를 참조하지만 실제 BT 실행 성공은 아직 아니다. Python 보호 필드라 Metadata_A의 정리 시도는 저장0으로 중단했고 추가 C++ 도구를 만들지 않았다. 이7곳의 편집 메타데이터 정리를 미완료로 유지한다.
- **다음:** 별도 `/Game/DreamCatcher/GAS/Test/R10/LyraAI/`의 실제 Experience·GameMode·PawnData/봇 구성·NavMesh/픽업 테스트 맵 연결안을 먼저 제시한다. 원본 StandardComponents/시작 Inventory·팀/스폰 조립의 필수 범위를 확인하고, 기존 플레이어/테스트 맵/스포너를 유지한 채 원본 BT 감지·이동·사격·재장전·무기 탐색·사망을 검증한다. 현재62개는 기존 활성 경로에 연결하지 않았다. **R10 전체/AI PIE·복제·온라인 호스팅·리플레이·패키징은 미완료**다. R7 HUD/R13 보류와 디자이너 병합 미실행도 유지한다.
- 한정 레시피는 `Scripts/Editor/r10_lyra_ai_assets.py`, `r10_lyra_ai_audio.py`. 성공한 이식/이동을 다시 실행하거나 지금 재빌드를 요구하지 않는다. 원본/기존 파일 보호와 새 경로를 유지하고, `ai_assets_Prepare_A`·`TargetInspect_A`·`Verify_A`·`ai_audio_Finalize_A`·`ai_metadata_Metadata_A`의 진단 실패를 성공 이력과 혼동하지 않는다.

## 0.81 조사 이력 — 2026-10-11 AI 조립 원본 / 후속 승인·작성0.82

- 사용자가 전용 GameMode/Experience/NavMesh 맵 연결 진행을 요청했다. 변경 전에 원본 `ShooterCore`, `LAS_ShooterGame_StandardComponents`, Elimination/Perf Experience, QuickBar/팀 생성/팀 스폰/Pawn·PawnData를 조회했다. `presentation_source_Assembly_A/report.json`22:15:46은3개, `presentation_source_AssemblyParts_A/report.json`22:18:12는7개 조회/저장0/missing0/보호변경0, 양쪽 로그 Error/ensure/Fatal0·정상종료0이다. 중복을 포함한 조회 건수이지 구현 진척 개수가 아니다. 기존553파일도 해시 불변이며 새 맵·C++·Config·에셋은 아직 변경하지 않았다.
- 원본 ShooterCore GameFeature는 Controller에 InventoryManager/WeaponState를 추가하고 StandardComponents는 B_QuickBarComponent(3슬롯)를 추가한다. Experience는 GameState에 BotSpawner/TeamSetup/TeamSpawningRules를 추가한다. 원본 B_Hero_ShooterMannequin에 AddInitialInventory(24노드)/ClearInventory(23노드)/WaitForInventoryReadiness(12노드)가 존재한다. 현재 테스트 Controller의 단일 라이플 타이머를 원본 초기 장비 그래프라고 보고하거나 봇 시작 무장을 임의 생략하지 않는다. 해당 Pawn의 전체 이식은 Emote·Cosmetics·Pistol 등 별도 의존성을 가지므로 필수 그래프와 전체 Pawn 이식 범위를 구분한다.
- **확인한 필수 누락3개:** (1) B_TeamSetup_TwoTeams의 부모 LyraTeamCreationComponent가 없음. 원본은 Experience HighPriority→팀 생성→기존/신규 PlayerState 균등 배정이며 데이터 팀ID는1/2다. (2) B_TeamSpawningRules의 부모는 단순 PlayerSpawningManager가 아니라 ShooterCoreRuntime.TDM_PlayerSpawningManagmentComponent이며 아직 없음. 원본은 적 Pawn에서 먼 비점유 시작점/점유 fallback을 고른다. (3) 현재 DreamCatcherPlayerController는 IDCTeamAgentInterface가 없어 원본 BotController.GetTeamAttitudeTowards의 Controller cast가 실패하고 **코드 경로상 Neutral**이다. 실제 PIE 관찰 결과와 구분한다. ACommonPlayerController 부모 연결만으로 이 팀 계약까지 이식된 것은 아니다.
- **제안(아직 미승인·미작성):** Teams/DCTeamCreationComponent h/cpp2, Player/Lyra/DCLyraTDMPlayerSpawningManagementComponent h/cpp2를 원본 본문/타입 대응으로 추가한다. 기존 TeamInfo 헤더의 UDCTeamCreationComponent friend 선언을 재사용한다. Development/DCLyraAIIntegrationController h/cpp2는 ADCRifleIntegrationController를 상속하고 원본 LyraPlayerController의 PlayerState 팀 조회·변경 구독/해제 부분만 이식해 새 AI 테스트 Controller에서 사용한다. 기존 플레이어 Controller/입력·카메라·무기·맵은 수정하지 않는다. Tests/DCLyraAIAssemblyTestTypes.h 및 DCLyraAIAssemblyAutomationTests.cpp2개에서 팀 배정/스폰 선택/플레이어-봇 팀 판정·구독 수명 관련 검사를 추가하고 기존26건을 회귀한다.
- Config 변경안은 DefaultEngine.ini의 위 원본 TeamCreation/TDM 부모 **ClassRedirect2개**, DefaultGame.ini의 신규 픽업 Cue 폴더 `/Game/LyraMigration/AI/ShooterCore/GameplayCues` **등록1개**다. 전역 GameMode/원본 AI 행동·전투 수치/기존 태그는 그대로다. 기존 PlayerController 자체에 팀 인터페이스를 더하는 대안은 모든 기존 맵에 영향을 주므로 이번에는 테스트 전용 파생형을 권장한다. 온라인/Replay/Elimination 점수 규칙·정식 HUD를 통째로 활성화하는 대안은 기존 보류 범위를 넘으므로 분리한다.
- 추가 코드/Config 승인을 받은 뒤 사용자 빌드→원본 TeamSetup/TeamSpawningRules/팀 Display 데이터 및 별도 Experience·봇 장비·NavMesh/픽업 맵 연결로 이어간다. 현재 보고된3개가 모든 통합 문제의 끝이라고 확약하지 않는다. 새 의존성은 근거/정확한 범위를 다시 설명한다. BT 편집 metadata7곳, 실제 AI·픽업·복제와 전체R10 미완료 상태는 유지한다. 이번에는 읽기 전용 진단 스크립트와 상태 문서만 갱신했다.

## 0.82 작성 이력 — 2026-10-11 AI 팀·스폰 코드 / 후속 검증0.83

- 사용자가0.81의 추가 코드·Config 묶음을 승인했다. **신규 게임 코드6파일＋검사2파일**, Config2파일을 작성했다. C++/UHT 빌드·패키징·Unreal 실행·에셋 저장은 하지 않았다. 직전 native26건/에셋62개 검증과 이번 새 코드의 미검증 상태를 구분한다.
- Teams/DCTeamCreationComponent h/cpp는 원본 팀 생성, Experience HighPriority 구독, 기존/신규 PlayerState 팀 배정, 관전자 해제·최소 인원/ID 동률 규칙을 그대로 보존했다. 원본 팀2개/ID1·2는 후속 BP 데이터에서 설정하며 코드에 강제하지 않는다. 기존 TeamInfo friend 선언을 사용하고 기존 헤더를 바꾸지 않았다.
- Player/Lyra/DCLyraTDMPlayerSpawningManagementComponent h/cpp는 원본 ShooterCore TDM 스폰 규칙의 거리·점유/occupancy·fallback을 보존했다. 이름/경로/module export 및 명시적인 Controller/Pawn 등 include만 대응했다. GameMode/기존 테스트 맵의 활성 스폰 규칙을 교체하지 않았다.
- Development/DCLyraAIIntegrationController h/cpp는 ADCRifleIntegrationController를 상속하고 IDCTeamAgentInterface를 구현한다. 원본 팀 관련8함수 본문과 OnRep_PlayerState의 Super→팀 재바인딩을 사용한다. **원본 OnRep의 별도 클라이언트 ASC 재시도 코드는 추가하지 않았으며 전체 PlayerController 이식이 아니다.** 기존 GAS 준비 경로/플레이어 입력·카메라·무기·UI와 기존 Controller/Blueprint는 유지한다. 새 클래스는 아직 어떤 맵에도 연결하지 않았다.
- DefaultEngine.ini에는 승인한 TeamCreation/TDM **ClassRedirect2개**, DefaultGame.ini에는 신규 픽업 Cue 폴더 **등록1개**만 추가했다. 기존 경로/태그/전역 GameMode·게임 수치 불변이다. 런타임 Cue 등록 성공은 빌드 후 검사한다.
- Tests/DCLyraAIAssemblyTestTypes.h 및 DCLyraAIAssemblyAutomationTests.cpp에 `DreamCatcher.R10.LyraAI.Assembly` **3검사**를 작성했다: TeamCreationAndAssignment, TeamSpawnSelection, PlayerTeamRelayAndAttitude. 실제 BeginPlay/AreActorsInitialized 뒤 native 규칙·팀 구독을 검사한다. fixture는 저장 Experience 로드와 플레이어 HUD/초기 무기 실행을 제외하며, 실제 AI/Navigation/에셋/복제 검사로 보고하지 않는다. **컴파일/실행 모두 미검증**, 기존26＋신규3=29건을 사용자 빌드 후 실행한다.
- 정적 대조: 원본4파일은 명시적인 타입/include/export 대응 후 일치, Controller8함수도 원본 대응 일치. generated include 위치·실제 엔진 API·Redirect 중복0 및 신규 등록 개수를 확인했다. 기존 **615파일 중 승인한 Config2개만 변경**, 나머지613개(기존 소스/플레이어·맵/AI62개·DLL 등)는 해시 불변이다. 근거 `Saved/Diagnostics/R10/lyra_ai_assembly_before.json`, `lyra_ai_assembly_static.json`. 전체 git diff 공백 검사도 통과했으며 기존 dirty/staged 변경은 보존했다.
- 다음은 **사용자 Development Editor/Win64 빌드** 확인→기존26＋신규3 검증→원본 팀 BP/Display 데이터 및 별도 Experience·장비·NavMesh/픽업 맵 연결이다. 현재 R10 통합 준비이며 전체R10/실제 AI 플레이·복제·온라인·Replay·패키징은 미완료, BT 편집 metadata7곳도 그대로다.

## 0.83 최신 재개 — 2026-10-11 native29건·팀 에셋5개 검증 완료 / 새 채팅 인계

- 사용자 빌드는22:52:35 KST 시작·36.08초 Succeeded, DLL22:53:10/5,168,128 bytes. `Saved/Automation/R10LyraAIAssembly_A/index.json`22:54:30 **성공29/실패·보고서경고0/notRun0**, 로그 Error/ensure/Fatal0·종료0이다. 신규 팀 배정/스폰 선택/플레이어 팀 전달3건과 기존26건을 모두 통과했다. Codex는 C++ 빌드·패키징을 실행하지 않았다.
- 원본 B_TeamSetup_TwoTeams/B_TeamSpawningRules/B_QuickBarComponent 및 TeamDA_Red/Blue **5개**를 Unreal Migrate→새 `/Game/LyraMigration/AI/{ShooterCore,Game}/...` 경로로 이동·저장했다. 기존 에셋·원본 파일을 덮어쓰지 않았다. Public/Private TeamInfo 타입은 준비 프로세스에서만 대응해 새 에셋에 저장했다. 새 영구 Config/C++ 변경은 없다.
- Relocate_A는5개 저장 후 Registry 의존성 조회가 None을 반환해 실패했다(complete=false/보호변경0). 이를 동기 스캔 후 조회하도록 교정하고 새 프로세스의 `ai_teams_Verify_A/report.json`·`R10LyraAI-Teams-Verify_A.log` **23:09:42**에서5개 재로드/저장0/종료0/엔진 Error·ensure·Fatal0을 확인했다. 원본 그래프·기본값·필드5개도 타입/경로 대응 후 모두 일치했다(`ai_team_source_comparison_A.json`). 기존 보호 **623파일 변경0**이다. 성공한 이식/이동이나29건을 다시 실행하지 않는다.
- 반복 스트림 오류는 앱 서버 로그에서 /responses HTTP200 시작 후 일반 처리 오류/재시도5회 소진으로 확인했다. 요청ID `0b5cc340-6a79-4f79-8b1f-019926b7bea5`23:10:11, `7a53ba48-e578-45d4-b861-7c96e6225a6a`23:16:16 KST. 명시적 인증/사용량 제한/네트워크/컨텍스트 초과 오류는 확인되지 않았으며 서버 내부 원인·대화 길이의 인과관계는 미확정이다. Unreal 검사는 스트림 중단 뒤에도 완료·저장됐다. 앱 설정은 수정하지 않았다.
- **다음은 실제 연결:** 새 `/Game/DreamCatcher/GAS/Test/R10/LyraAI/`는 아직 없고 Experience/GameMode/PawnData·NavMesh/픽업 맵도 미생성이다. 원본 BotCreation/팀 생성/스폰/QuickBar와 장비 조립, 원본 초기 Inventory 그래프 보존을 연결한 후 실제 BT/Perception/Navigation·사격/재장전·픽업/사망을 검증한다. 검사 목적의 빈 인벤토리 시작을 원본 초기 장비 생략으로 완료 처리하지 않는다. BT 편집 metadata7곳/기존 AutoReload import 경고와 실제 AI·복제·전체R10 미완료는 유지한다.
- 사용자 요청에 따라 `docs/handoffs/r10-resume-2026-10-11.md`에 새 채팅 재개 문구와 완료·미완료/승인 범위를 정리했다. 새 채팅에서 이 파일과 최신 명세부터 읽고 중복 실행 없이 진행한다. 지금 사용자 재빌드는 필요 없다.

---

## 0.84 이력 — 2026-10-12 AI 테스트 맵·SettingsLocal 회귀 / 초기 인벤토리 UI 단계 대기

- 0.83의 사용자 빌드/native29건 및 AI62개·팀5개 Migrate/Relocate는 반복하지 않았다. `GAS-System`과 기존 dirty/staged 변경을 유지하며 Codex C++ 빌드·패키징·Git stage/commit/merge는0이다.
- **신규 실제 연결5개:** `/Game/DreamCatcher/GAS/Test/R10/LyraAI/`에 `BP_DC_LyraAIPlayerController`, `DA_DC_LyraAIPawn`, `B_DC_LyraAIExperience`, `BP_DC_LyraAIGameMode`, `L_DC_LyraAIIntegration`을 생성했다. 원본 팀1/2·TDM 스폰·봇3명과 봇 Controller의 Inventory/WeaponState/QuickBar를 Experience AddComponents로 조립했다. 기존 테스트 플레이어 Pawn/라이플/UI를 재사용하며 Native/기존 컴포넌트를 중복 생성하지 않는다. 맵은 시작점4개·원본 Rifle 픽업4개·중앙 엄폐·Dynamic Recast/Bounds를 포함한다. 이 테스트용 맵 구성은 원본 Lyra 맵의 복제본이 아니다.
- `Saved/Diagnostics/R10/map_Bind_F.json`과 `R10LyraAIMap-Bind_F.log`의 **10월11일23:51:37 KST** 재로드에서 실제 DefaultExperience ID와 연결을 확인했다. 초기 Apply_A/B는 Python API 교정 전 저장0, Apply_C는 연결 에셋4개 저장 후 NavMesh 배치 실패, Resume_D는 저장0, Resume_E는 맵 저장 성공이다. Bind_F는 새 GameMode의 잘못 비어 있던 ID 표현을 교정했다. 실패 기록을 성공으로 덮어쓰거나 기존 에셋을 재마이그레이션하지 않았다.
- **실제 기동 제약 재현·승인된 해결:** 첫 PIE_A는 Experience 완료의 SettingsLocal `CastChecked`에서 기본 `GameUserSettings` 타입 때문에 Fatal이 났다. 호출을 제거하지 않고 프로세스 한정 원본 클래스 선택으로 원인을 분리했다. 사용자가 영구 등록과 회귀를 승인하여 DefaultEngine.ini의 `/Script/Engine.Engine`에 `GameUserSettingsClassName=/Script/DreamCatcher.DCLyraSettingsLocal` **1줄만** 추가했다. 기존 전역 GameMode/맵·플레이어 에셋·C++는 유지했다.
- **관찰 결과와 한계:** `ai_pie_C.json`은 프로세스 한정 SettingsLocal/무음 조건에서 팀1/2·봇3명·감지·이동·픽업·사격·탄약 소모·피해/사망을 관찰했다. Item 짧은 이름이 Controller 간 겹치므로 탄약 집계는 `ai_pie_C_owner_audit.json`의 소유자별 재분석을 근거로 한다(봇 탄창/예비탄 `0/60→30/30` 등). C Automation은 성공1/실패0이나 **경고2031개**로, 무음 실행 중 WeaponAudioFunctions의 빈 TapIDs 경고가 다수다. 이후 영구 Config·오디오 활성 PIE_D는 성공1/실패0/경고15이며 기동·팀·감지·이동을 확인했으나32초 내 픽업/사격은 관찰되지 않았다. D까지 전체 전투 성공이라고 확대하지 않는다. 원본 초기 인벤토리 미연결 상태의 독립 픽업 검사다.
- **기존 플레이어 회귀 통과:** `settings_player_A.json` 및 `Saved/Automation/R10SettingsPlayer_A/index.json`은 **10월12일00:04:32 KST 성공1/실패·경고0**, Error/ensure/Fatal0이다. 영구 설정 선택(별도 진단 GameUserSettings 저장 경로)·이동164.8cm·시점yaw14.5도·Hip→Shoulder(FOV70)→Hip·탄창30→27→수동 재장전30·기존 Reticle/Ammo UI를 확인했다. 물리키·화면 품질·사용자 설정값 전체 이관 검증은 아니다.
- **초기 인벤토리 미완료:** 원본 B_Hero_ShooterMannequin의 초기값은 `ID_Pistol`1개다. AddInitialInventory/ClearInventory/WaitForInventoryReadiness와 Possession/사망/Reset 호출을 선별한 메모리 사본의 노드/핀 비교는 일치했지만, Emote 인터페이스를 남기면 함수가 다시 등록되고 Python `ImplementedInterfaces` 읽기가 보호되어 저장 전 중단했다. 새 범용 도구/C++는 추가하지 않았다. 새 테스트 Pawn만 Rifle 기본값 및 봇 한정 호출을 사용하여 기존 플레이어 초기장착과 중복되지 않게 연결하는 후속 스크립트를 준비했으며 **아직 실행·이식·활성 연결하지 않았다**.
- **UI 재개 위치:** 원본 Lyra 프로젝트의 신규 `/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn`은 전체 Hero 임시 사본이며 **이식 준비 완료본이 아니다**. 원본 Hero는 그대로다. `inventory_ui_stage.json`과 `R10InventoryUI-B.log`를 참고한다. Computer Use의 UnrealEditor 화면 접근은 자동 승인 검토에서 거부됐고, 사용자가 약20분 후 허용하겠다고 했다. 허용 전 UI 재시도/다른 경로로의 권한 우회는 하지 않는다. 신규 사본의 Class Settings에서 Emote 인터페이스만 제거·Compile·해당 사본만 저장한 후 원본 Editor를 닫고 `r10_lyra_inventory_prepare.py` → `r10_lyra_inventory_target.py`의 정확한 신규1개 이식·연결을 진행한다. 기존62/5개는 반복하지 않는다.
- 이 턴의 맵 연결·인벤토리 스크립트와 상세 재개는 `docs/handoffs/r10-resume-2026-10-11.md`를 따른다. 보호590개 기존 파일 중 현재 변화는 승인한 DefaultEngine.ini뿐이고 Git 인덱스·기존 소스/플레이어/맵은 불변이다. 원본 초기 장비/사망 정리·탄약 소진/픽업 완주·복제·사용자 화면/음향·전체R10은 미완료다. 10월12일 R11/R12 흐름 체크포인트 대비 지연 위험이 있으며 마감 범위를 축소하지 않는다.

---

## 0.85 이력 — 2026-10-12 원본 초기 인벤토리 연결·실제 AI 수명 검증 통과

- 사용자 Computer Use 재요청 후 새 source 사본의 Emote 인터페이스만 UI로 제거·Compile·저장했다. unattended 모드의 자동 Cancel은 일반 Editor 재실행으로 해결했다. **원본 Hero는 불변이며 source Editor는 정상 종료했다.** 사용자가 이후 Computer Use를 끄겠다고 했고 더 이상 UI를 사용하지 않았다. 기존 native29/AI62/팀5개 이식은 반복하지 않았다.
- `inventory_source_D.json` **00:43:00 KST**은 AddInitialInventory/ClearInventory/WaitForInventoryReadiness와 lifecycle 호출의 원본 노드·핀·기본값 비교, 원본 그래프/보호 파일 불변, 신규 준비본1개 저장을 통과했다. 준비본의 직접 의존성에는 ShooterCore/Pistol/Emote가 없다. `inventory_target_Migrate_A.json`으로 **신규 Pawn1개만** Unreal Migrate했다. 0.84의 전체 Hero 임시 사본/미이식 기록보다 이 상태를 우선한다.
- **대상 연결:** `/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn`을 기존 BP_DC_RiflePawn의 자식으로 연결하고 기존 메시·충돌·이동 템플릿을 보존했다. 원본 Pistol1개 기본값은 이 테스트 Pawn에서 검증된 Rifle1개로 선택한다. Possession/DeathFinished/Reset에 IsBotControlled 분기3개만 추가하여 기존 플레이어의 초기장착/정리와 중복되지 않게 했다. 원본 인벤토리 함수·준비 매크로는 타입 대응 외 그대로다. 기존 플레이어/맵을 교체하지 않고 이번 새 DA_DC_LyraAIPawn만 새 Pawn을 선택한다.
- Apply_A는 InventoryManager 클래스 대응 누락으로 Compile 실패/저장0이다. Apply_B는 **프로세스 한정** LyraInventoryManagerComponent→DCInventoryManagerComponent 대응으로 **00:47:25 KST 새 Pawn·새 PawnData2개 저장**을 통과했다. 영구 Redirect/Config·C++는 추가하지 않았다. **00:48:58 Verify_A**와 **01:04:32 Verify_B**는 임시 대응 없는 별도 프로세스 재로드·Compile·그래프 비교/저장0/보호변경0/Error·ensure·Fatal0이다.
- **실제 AI PIE:** Inventory_E(00:51:19)에서 초기0.84초에 봇3명+플레이어 모두 Inventory/Equipment 각1개·Instigator 연결, 모든 픽업 미사용을 확인했다. 이후 감지·이동·사격·재장전·픽업·피해·사망, 원본 약8초 Death 지연 뒤 봇과 플레이어의 Inventory/슬롯/Equipment0을 확인했다. Restart_F(00:57:48)는 자연 사망한 봇에 검사자가 원본 RequestPlayerRestartNextFrame을1회 호출하여 같은 팀/새 Pawn/HP100/라이플·Equipment1개를 확인했다. **자동 부활 정책을 설치하거나 검증한 것은 아니다.** 같은 실행에서 탄약0/0→OutOfAmmo→픽업 예비탄60→재장전30/30→사격 재개도 관찰했다. 근거는 `ai_pie_Inventory_E.json`, `ai_pie_Restart_F.json`, `inventory_integration_summary.json`이다. E의 초기 아이템 획득 boolean은 초기부여와 픽업을 구분하지 못하므로 픽업 사용은 패드 availability와 F의 탄약/BB 전이를 근거로 한다.
- **새 맵 플레이어 회귀:** NewPawn_B(00:54:41)는 원본 개발 설정으로 검사 프로세스의 봇만0으로 두고 새 Pawn의 중복 아이템 없음·이동155.5cm/yaw28.5도·Hip→Shoulder(FOV70)→Hip·30→27→수동 재장전30·Reticle/Ammo UI를 통과했다. 원본 맵의 이전 회귀와 구분한다. 실제 게임 데이터의 봇3명은 유지했다.
- 세 PIE 보고서는 각각 성공1/실패0/경고16이다. 경고는 기존 Manny PoseAsset14개, 시작 시 CrowdManager/Recast1개, 이번 안내 주석 NodeGuid1개였다. 마지막 경고는 GUID 보호 속성 때문에 Comment_C 저장0 후, **Comment_D에서 이번에 추가한 선택적 주석만 제거**하고 실행 노드/연결 불변을 대조했다. Verify_B에서 missing NodeGuid0을 확인했으며 런타임 변경이 없어 전체 PIE를 반복하지 않았다. Pose/Nav 경고와 기존 AutoReload import/BT 편집 metadata 이력은 별도다.
- **현재 사용자 확인:** `L_DC_LyraAIIntegration`을 열어 PIE의 봇/플레이어 메시·애니메이션·발사/픽업 FX·음향·조작감을 확인한다. C++ 재빌드는 이번 변경에 필요 없다. 복제/네트워크·유한 DCEnemySpawner/Encounter 실사용 연결·자동 부활 정책·전체R10은 미완료다. 다음 코드 범위는 이 검증된 AI를 유한 인카운터에 연결하는 변경안으로 먼저 제시한다. 기존 예고 공격 프로토타입·맵/스포너는 보존한다. Codex C++ 빌드/패키징·Git stage/commit/merge는0이다.

---

## 0.86 이력 — 2026-10-12 AI 이동 중 정지 포즈 원인 확인·원본 경로 가속도 복구

- 사용자는 AI 이동 모션을 제외한 PIE 동작이 정상이라고 확인했고, 정지 자세로 미끄러지는 AI의 조사·수정을 요청했다. **이동 AnimBP가 미연결인 상태가 아니었다.** 원본 Lyra Hero 두 BP의 `NavMovementProperties.bUseAccelerationForPaths=True`와 달리 테스트 Pawn은 False였다. 실제 이동 중 가속도0/AnimBP `HasAcceleration=false`를 확인했다.
- `/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn`의 CharMoveComp에서 **해당 bool 하나만 True로 복구**했다. 원본 값을 사용하며 다른 이동 수치·메시/AnimBP·인벤토리/lifecycle 그래프는 유지했다. `r10_ai_locomotion_assets.py`의 Apply_C19:49:44 및 Reload_E19:57:20 KST에서 Compile·저장1개·새 프로세스 재로드/그래프 비교를 통과했다. 보호 기준597개 중 변경은 이 Pawn1개뿐이다. 기존 플레이어/맵·C++·Config·DLL·Git 인덱스·원본 Hero는 불변이다.
- **실제 원인 분리:** Trial_B19:47:09의 한 봇에서 설정 전 이동22샘플은 가속도0/HasAcceleration0회였고, 런타임 bool만 바꾼 뒤 이동64샘플 중63회 가속도/플래그가 활성화됐다(최대1200). 왼발의 메시 기준 위치 변화 폭은 최대1.49cm→112.94cm이며 Rifle layer·Equipment1개를 유지했다. Trial_A의 기본 Editor 알림은 라이브 컴포넌트를 재설정했으므로 수정 성공 근거에서 제외한다.
- **저장본 PIE:** Verify_F20:00:44에서 런타임 설정 변경 없이 AI3개 모두 경로 가속도/AnimBP 플래그·다리 포즈 변화를 확인했고 라이플1개를 유지했다. 플레이어 이동 후 정지도 통과했다. 관찰 스크립트 없는 Plain_G20:06:26도 통과했다. 각 Automation은 성공(경고 포함)1/실패0, 기존 PoseAsset14·초기 Recast1 경고이며 로그 Error/ensure/Fatal0이다. 세부 수치는 `Saved/Diagnostics/R10/locomotion_evaluation.json`을 따른다.
- **실패 이력 유지:** Verify_D19:52:51은 Editor 그래프/CDO 조회와 PIE를 같은 시작 스크립트에서 수행하다 장비 `AddEntry`에서 접근 위반으로 종료했다. 원인은 확정하지 않았다. Editor 조회를 별도 commandlet으로 분리한 Reload_E·PIE Verify_F·일반 PIE Plain_G는 통과했으며 장비 C++를 수정해 우회하지 않았다. 실패 로그는 보존한다.
- **후속 사용자 확인 완료:** 사용자가 `L_DC_LyraAIIntegration`의 PIE에서 이동 모션이 정상이라고 확인했다. 이 문제의 자동검사와 사용자 시각 확인은 완료했으며 반복하지 않는다. Computer Use·C++ 빌드·패키징은 실행하지 않았다. 완료된 native29/62·5개 마이그레이션도 반복하지 않았다.
- **범위 유지:** 자동 리스폰이나 테스트 맵의 새 팀 분리는 추가하지 않는다. 원본 Elimination의 `AbilitySet_Elimination → GA_AutoRespawn`과 현재 테스트의 미연결 상태를 구분한다. 사용자는 유한 스포너 생성까지 묶는 안과 사망 집계/정리만 연결하는 안의 차이를 먼저 설명하라고 했으므로 **DCEnemySpawner/Encounter 연결 수정은 아직 시작하지 않았다.** 그 범위 결정·실맵 통합·복제·전체R10은 남았다. 상세 재개는 `docs/handoffs/r10-resume-2026-10-11.md` 최신 부분을 따른다.

---

## 0.87 이력 — 2026-10-12 유한 Lyra 생성·사망 수명·인카운터 보강 코드 작성 / 사용자 빌드 대기

- 사용자는 두 생성 방식의 차이를 확인한 뒤 **유한 스포너에 Lyra 생성 경로 추가 / 처치 집계와 객체 정리 분리 / 인카운터 완료 처리 보강**을 명시적으로 승인했다. 0.86의 범위 결정 대기 기록은 이 승인으로 해소됐다. 이동 모션은 이미 사용자 PIE 확인까지 완료했으며 반복하지 않는다.
- **기존 코드4개 수정:** `AI/DCEnemySpawner.h/.cpp`, `Stage/DCEncounterController.h/.cpp`. 새 `LyraBotControllerClass`를 지정할 때만 Lyra 경로가 우선하며 미지정 시 기존 GAS/legacy 경로를 유지한다. 서버에서 Experience 완료와 팀 초기화를 기다린 뒤 원본 Controller 생성·GenericPlayerInitialization·PawnData/초기화 흐름을 사용한다. 원본 RestartPlayer 대신 엔진 RestartPlayerAtTransform으로 스포너 위치를 사용하며 총수/동시 생존 수/간격은 기존 설정을 따른다. 새 팀 분리·자동 리스폰은 추가하지 않았다.
- **사망 집계/정리:** Health.OnDeathStarted에서 생존 집합만1회 제거한다. 별도 소유 목록은 DeathFinished와 원본 Pawn 정리까지 유지하고 Pawn.OnDestroyed에서 해당 봇 Controller를 제거한다. PlayerState/ASC는 엔진 Controller 정리를 따른다. 실패한 생성의 해당 Controller 재시작 타이머를 취소하고 자기 객체만 정리한다. 스포너 EndPlay가 사망 콜백에서 발생해도 ASC를 같은 스택에서 파괴하지 않도록 소유 객체 정리는 다음 tick에 수행한다. 예기치 않은 살아 있는 Pawn/Controller 제거는 성공 클리어가 아니다.
- **인카운터:** 전체 고유 스포너 등록 → 활성화 → 시작 이벤트 → 완료 판정 순서로 변경했다. 중복/외부/위조 완료 알림, 중복 배열 항목, 이미 완료한 스포너, 반복 시작과 지연 타이머 재설정을 처리한다. `OnSpawnerFailed`/`OnEncounterFailed` 및 상태 조회를 추가하며 실패/진행 중 스포너 제거를 다음 단계로 진행시키지 않는다. 기존 클래스 없는 legacy 스포너의 즉시 완료 규칙은 보존했다. StageDirector 자체는 수정하지 않았다.
- **검사 준비:** `Tests/DCLyraFiniteSpawnerTestTypes.h`와 `DCLyraFiniteSpawnerAutomationTests.cpp`의 native6건을 작성했다. prefix `DreamCatcher.R10.LyraAI.FiniteSpawner`, 별도 unattended Editor의 `-DCR10FiniteLyraIsolatedAutomation`을 요구한다. 임시 Pawn의 사망 이벤트·실제 Controller/PlayerState 수명, 인카운터 순서/중복, 비정상 제거, 소유자 종료, Experience 대기와 생성 실패 후 원본 재시작 취소를 검사한다. 성공한 실제 Blueprint 생성/장비 연결을 이 fixture 검사로 대신하지 않는다.
- **빌드 뒤 에셋/PIE 준비:** `Scripts/Editor/r10_lyra_finite_setup.py`는 신규 `/Game/DreamCatcher/GAS/Test/R10/LyraEncounter/`의 Experience/GameMode/Spawner BP/맵4개만 생성한다. 기존 맵을 재저장하지 않고 기존 맵 구성 helper를 재사용한다. 새 Experience에서 자동 BotCreation 항목1개만 제외하여 이중 생성을 막고 Inventory/QuickBar/팀 설정은 그대로다. 스포너2개(총수2+1, 각각 동시1, 간격1초)·인카운터1개를 연결한다. `r10_lyra_finite_probe.py`는 실제 PIE의 유한 생성·초기 라이플·GAS 사망·정리·완료1회를 관찰하도록 준비했다. 검사 프로세스에서만 플레이어 피해 면역과 봇에 자폭 태그를 붙인 기존 피해 GE를 적용하므로 실제 무기 처치·시각/조작감 검증과 구분한다.
- **현재 확인 범위:** 두 Python 스크립트 AST와 변경 파일 공백 검사를 통과했고, `finite_spawner_before_A.json`의 보호595개 중 허용된 기존 소스4개 외591개가 유지됐다. 새 검사/자동화는 **작성만 완료**했으며 C++ 빌드·native6건·새 에셋 생성·PIE는 아직 실행하지 않았다. Config/기존 에셋·플레이어·DLL·Git 인덱스 불변, Computer Use·빌드·패키징·Git stage/commit/merge0이다.
- **다음:** 사용자 Development Editor/Win64 빌드 확인 → native6건과 영향받는 기존 스포너 회귀3건 → 신규4개 apply/verify → 실제 유한 인카운터 PIE → 사용자 짧은 플레이 확인. 빌드 전 새 API가 없는 DLL로 자동화를 실행하지 않는다. 이전 native29/AI62/팀5개 이식 및 정상 이동 검증을 일괄 반복하지 않는다. 실제 에셋 연결·실맵 통합·복제·전체R10은 미완료이며 자세한 재개는 인계 문서 최신 부분을 따른다.

---

## 0.88 이력 — 2026-10-12 유한 Lyra 인카운터 연결·실제 PIE·기존 플레이어 회귀 통과

- **사용자 빌드 확인:** UBT `21:53:49 KST` 시작,55.19초 성공, DLL `21:54:43` 갱신(5,274,112 bytes)이다. 승인한 소스/검사 파일은 작성본과 일치한다. 이번 실행에서 Codex C++ 수정·빌드·패키징은0이다.
- **native 검사:** `Saved/Automation/R10FiniteNative_A/index.json` **22:03:06 KST 성공9/실패0/notRun0**(새6+영향받는 기존3). 보고서 경고1개는 EncounterRegistration fixture의 기존 빈 legacy 스포너 즉시 완료 안내다. 로그 Error/ensure/Fatal0이며 성공한 실제 Blueprint 생성·장비 연결과 구분한다.
- **새 연결4개 생성/최종 위치:** Experience는 `/Game/DreamCatcher/GAS/Test/R10/LyraAI/FiniteEncounter/B_DC_LyraFiniteExperience`, 나머지는 `/Game/DreamCatcher/GAS/Test/R10/LyraEncounter/`의 `BP_DC_LyraFiniteGameMode`, `BP_DC_LyraFiniteSpawner`, `L_DC_LyraFiniteEncounter`다. 새 Experience만 자동 BotCreation1항목을 제외하고 기존 Inventory/WeaponState/QuickBar/팀5항목·PawnData를 재사용한다. 스포너A 총2/동시1, B 총1/동시1, 간격1초·인카운터1개다. 기존 맵/AI Experience는 그대로다.
- **발견·수정한 연결 문제:** Apply_A와 Verify_A는 직접 에셋 조회에 성공했지만 최초 PIE_A는 새 Experience가 AssetManager 검색 범위 밖이라 SetCurrentExperience의 AssetClass assertion으로 종료3이었다. 기존 Config의 검색 범위가 LyraAI 하위임을 확인하고 **이번에 만든 Experience1개만** 위 새 하위 경로로 Unreal 내부 이동했다(Relocate_B). Config/C++는 바꾸지 않았다. Verify_C22:21:53은 새 프로세스에서 실제 PrimaryAssetId→생성 클래스 해석까지 통과했다. 이전 직접 로드 성공을 런타임 성공으로 취급하지 않는다.
- **실제 유한 PIE 통과:** PIE_B는 관찰 lambda의 기본 인자를 Unreal이 두 번째 인자로 판정하여 실패했다(프로세스0/Automation실패1). 한 인자 closure로 검사 스크립트만 교정했다. `finite_pie_C.json` 및 `R10FinitePIE_C/index.json` **22:25:43 KST 성공1/실패0**에서 초기2명→1회 보충으로 총3명, 스포너별 생존 상한1, 초기 라이플 각1개, 스포너 완료2회·인카운터 완료1회, 마지막 봇 Controller0을 확인했다. 마지막 처치/완료는 PIE7.755초, 원본 Death8.001~8.005초 후 Pawn 제거 알림에서 Controller/PlayerState까지 정리되어15.865초에 모두 없어졌고17.866초까지 추가 생성이 없었다. 사망은 검사 프로세스의 기존 GE+원본 자폭 태그로 유도했으며 플레이어에게 임시 면역을 부여/회수했다. 실제 무기 처치·사용자 조작감·복제 확인으로 확대하지 않는다.
- **기존 플레이어 회귀:** `settings_player_FiniteRegression_D.json`, `R10FinitePlayer_D/index.json` **22:36:26 KST 성공1/실패·경고0**. 기존 RifleIntegration 맵에서 이동155.4cm/시점14.5도, Hip→Shoulder(FOV70)→Hip, 탄창30→27→수동재장전30, Reticle UI를 확인했다. 유한 PIE_C의 기존 Manny PoseAsset14/초기 Recast1 경고는 남아 있으며 C/D 로그 Error/ensure/Fatal0이다. 이전 AutoReload import/BT metadata 이력은 별도 보존한다.
- **보존·근거:** 빌드 후 기준 `finite_postbuild_before_A.json`의598파일(기존 소스·Config·테스트 에셋·AI 에셋·DLL·Git 인덱스·uproject)은 모두 유지됐다. 신규 실에셋은4개이며 완료된 마이그레이션을 반복하지 않았다. 통합 요약 `Saved/Diagnostics/R10/finite_integration_summary_A.json`, 상세 재개는 인계 문서 최신 부분을 따른다. 이번 수정은 Python 관찰/배치 레시피와 새 에셋 경로 교정이며 추가 C++ 빌드는 필요 없다.
- **후속 사용자 수용 완료:** 사용자가 새 유한 인카운터의 정상 작동을 확인했다. 처음 봇2명/누적 봇3명이며 플레이어는 이 수에 포함하지 않는 구성을 설명했고 확인받았다. 이번 생성·사망 정리·인카운터 연결은 사용자 PIE까지 완료했으며 반복하지 않는다. 기존 팀 분배와 자동 리스폰 없음은 유지한다. R11/실맵 통합·복제·전체R10 완료와 구분한다.
- **다음 R11 읽기 전용 조사:** `Saved/Diagnostics/R11/inspect_stage_A.json`에서 기존 Level1의 로드된 배치 액터1108개를 확인했다. StageDirector/Encounter/EnemySpawner 배치는0, legacy EnemyCharacter1개, PlayerStart1개, GameMode는 기존 BP_DreamCatcherGameMode다. 기존 Stage/Encounter/Spawner BP3개는 기본 이벤트 노드만 있는 native 파생 클래스다. 에셋 저장0/보호변경0/ensure0이며 런타임 LevelScript 동작을 검증한 것은 아니다. 다음 변경안은 기존 Level1의 별도 R11 통합 사본, 기존 StageDirector의 진행/종료 보강, R11 전용 승패·재시작/임시 결과 UI, 해당 사본에만 기존 팀 API로 플레이어/적 배정을 연결하는 범위다. **새 R11 코드·에셋은 아직 미작성/미승인**이며 원본 Level1을 덮어쓰지 않는다. 0.4에 따라 이 변경 묶음을 설명·승인받은 뒤 구현한다.

---

## 0.89 최신 재개 — 2026-10-13 승인된 R11 순서·승패·재시작 코드 작성 / 사용자 빌드 대기

- 사용자는 0.88의 R10 유한 인카운터를 정상 확인한 뒤, 설명한 **Level1 별도 통합 사본 / 전투1→전투2→보스 Placeholder→결과 / 실패와 명시적 재시작 / R11에만 기존 팀 API로 플레이어·적 고정 배정** 묶음을 승인했다. R10 완료 검증은 반복하지 않는다.
- **기존4소스 수정:** `Stage/DCStageDirector.h/.cpp`는 기존 Phases/PhaseType을 유지하고 RunState·현재 단계 조회·진행/종료 delegate를 추가했다.0초 Delay와 전투 완료 후 다음 단계는 다음 tick에 처리하며 중복/외부 알림·종료 후 재진입·잘못된 단계 데이터를 차단한다. `DCEncounterController.h/.cpp`의 명시적 AbortEncounter는 자기 스포너만 중단하고 R10의 소유 객체 정리를 재사용한다. 이미 클리어한 인카운터의 사망 처리 중 봇도 정리할 수 있으며 성공 클리어를 위조하지 않는다. 사용하지 않는 Delay/Complete의 Encounter 참조 및 다른 World는 정리 대상에서 제외한다.
- **신규 런타임4소스:** `GameModes/DCStageRunGameMode.h/.cpp`, `UI/Stage/DCStageResultWidget.h/.cpp`. R11 전용 GameMode만 원본 Experience/팀 생성 완료 뒤 PlayerState API로 플레이어1/봇2를 배정하고, 플레이어 ASC/Health 준비 후 수동 시작 설정의 Director를 시작한다. 준비 제한15초는 Blueprint 조정 가능하다. 플레이어 사망/진행 실패를 연결하고 결과에서만 명시적 다시 시작/돌아가기를 허용하며 중복 요청을 막는다. 이동은 다음 tick의 OpenLevel로 전체 World를 새로 연다. 자동 리스폰·기존 GameMode/팀 정책 교체는 없다.
- **임시 UI/입력 수명:** Native UMG로 단계 제목·클리어/실패·다시 시작/돌아가기만 표시한다. 별도 결과 Widget 에셋이나 정식 HUD 교체는 필요하지 않다. 결과 때 자기 입력 차단 태그/이동·시점 무시를 추가하고 취소 가능한 입력 Ability만 정리한다. 원본 Death Ability는 유지하며 UI 제거/World 종료 때 자기 태그·입력 무시·delegate를 해제한다. R13 정식 HUD/메뉴 완료가 아니다. 레벨 이동은 현재 standalone 범위다.
- **새5검사 작성:** `Tests/DCStageFlowTestTypes.h`, `DCStageFlowAutomationTests.cpp`, prefix `DreamCatcher.R11.Stage`.0초/순차 단계·지연/외부 알림·종료1회, 소유 인카운터 중단/지연 취소, 잘못된 설정, Director 제거, 고정 팀/명시적 이동 요청을 임시 native 월드에서 검사하도록 작성했다. 빌드·실행은 아직 하지 않았다. 실제 UI 버튼/레벨 재로드·GAS/HUD 정리를 이 fixture 검사로 대체하지 않는다.
- **빌드 후 자동화 준비:** `Scripts/Editor/r11_stage_setup.py`는 신규 `/Game/DreamCatcher/GAS/Test/R11/L_DC_Level1Flow`와 `BP_DC_StageRunGameMode`2개만 만든다. 엔진 NewLevelFromTemplate으로 기존 Level1을 복제하고 환경 액터 클래스/Transform을 비교한다. 사본의 legacy 적1개만 제외하고 기존 일반 PlayerStart를 같은 위치의 Lyra PlayerStart로 교체한다(원본 Lyra 스폰 관리자는 Lyra PlayerStart만 수집). 기존 적 위치 근처의 바닥/캡슐 여유가 확인되는3곳에 단계별1명씩 총3명의 임시 전투를 연결하고 Dynamic NavMesh를 추가한다. 이 수량/위치와 임시 보스는 최종 밸런스/보스 패턴이 아니다. 기존 Experience/PawnData/스포너/Stage BP를 재사용하며 Config는 바꾸지 않는다.
- `r11_stage_probe.py`는 `clear`와 `fail-retry-return` 실제 PIE를 준비했다. 전자는 단계별 생성·팀·Nav 경로·GAS 사망/최종 정리/결과, 후자는 플레이어 원본 사망 후 수동 재시작·입력/초기 장비/단계 UI 복원·기존 유한 인카운터 맵 복귀를 확인하도록 작성했다. 진단 GE·임시 면역·입력 주입을 사용하므로 실제 무기 처치/물리 버튼 입력과 구분한다. OpenLevel 사이에 Python World/Actor/widget 참조를 보관하지 않는다.
- **현재 상태:** 기존4소스＋신규 런타임4/검사2/Python2, 문서를 작성했다. 두 Python AST·공백·로컬 include 점검과 기존 보호609개 중 허용된4개 외605개 불변을 확인했다. 근거 `Saved/Diagnostics/R11/before_A.json`, `static_B.json`. **UHT/C++ 빌드·native5건·에셋 생성·실제 PIE는 아직 미검증**이다. 기존 플레이어·맵·Config·DCEnemySpawner/전투 소스·R10 에셋·DLL·Git 인덱스는 유지했다. Computer Use·C++ 빌드·패키징·Git stage/commit/merge0이다.
- **다음:** 사용자 Development Editor/Win64 빌드 → 새5건＋변경된 Encounter에 관련된 R10 회귀9건 → 새 사본 apply/verify → 두 실제 PIE 시나리오 → 사용자 화면/버튼 확인. 10월12일 R11 흐름 실행 기준은 아직 미달이며 사용자 빌드/실맵 검사만큼 지연된 상태다. 완료 목표를 축소하거나 R11/실제 디자이너 맵 활성화·R12·복제를 완료 표시하지 않는다. 상세 재개는 인계 문서 최신 부분을 따른다.

---

## 0.90 이력 — 2026-10-13 R11 별도 Level1 연결·실제 PIE 자동 검사 통과 / 사용자 화면·버튼 확인 대기

- **사용자 빌드 확인:** 01:05:05 KST 시작,130.51초 `Succeeded`, `UnrealEditor-DreamCatcher.dll`01:07:15 갱신(5,420,032 bytes)을 확인했다. 이번 턴의 추가 게임 C++/Config 수정·Codex 빌드/패키징은0이며 재빌드는 필요 없다.
- **관련 native14건 통과:** `Saved/Automation/R11StageNative_A/index.json`,01:12:17 KST. 새 R11 Stage5＋R10 finite6/공용 Encounter 회귀3 성공14/실패0. 경고는 기존 빈 legacy 스포너 fixture1건이며 로그 Error/ensure/Fatal0이다. 완료된 native29·AI 마이그레이션·이동 모션 작업은 반복하지 않았다.
- **신규2에셋 저장/재로드:** `/Game/DreamCatcher/GAS/Test/R11/L_DC_Level1Flow`, `BP_DC_StageRunGameMode`. 기존 Level1을 엔진 내부 NewLevelFromTemplate으로 복제하고 사본에만 전투1→전투2→임시 보스→완료, 단계별1명/총3명, 기존 Lyra Experience/PawnData/스포너·Stage/Encounter BP, 전용 팀1/2·결과 UI를 연결했다. 기존 일반 PlayerStart를 같은 위치의 Lyra PlayerStart로 교체했고 legacy 적1개를 사본에서만 제외했다. Dynamic NavMesh와 바닥/캡슐 여유가 확인된 도로의3지점 `(335,165,71)`, `(635,165,71)`, `(1035,165,71)`을 사용한다. 원본 환경1106액터의 클래스/Transform은 새 프로세스에서도 일치했다.
- **실제 에셋 자동화 교정 이력:** Apply_A는 비노출 캡슐 getter, Resume_B/C는 commandlet의 비어 있는 scene query로 중단했다. 일반 editor에서 같은 바닥 query가 정상임을 확인해 로드 후 tick을 기다리는 방식으로 전환했다. Resume_D는 ExecCmds의 쉼표 분리, E는 빈 overlap의 None 반환, F는 저장 중 callback 재진입 및 Experience 선로드 순서 문제를 기록했다. F에서 신규2개만 저장됐으며 기존 보호파일 변경0이다. 관찰/호출을 교정한 **Verify_G의 새 프로세스 재로드01:24 KST, complete=true/저장0/보호변경0/ensure0**으로 최종 연결을 확인했다. 실패 이력은 삭제하지 않으며 F의 complete=false를 성공으로 바꾸지 않는다. `r11_stage_setup.py`의 apply/resume은 `-DCR11StageEditor`가 있는 ticking editor, verify는 commandlet을 사용한다. 알려진 template-only 부분 저장만 해시 검증 후 resume하며 완성본에 apply/resume을 다시 실행하지 않는다.
- **클리어 실제 PIE 통과:** `pie_Clear_D.json` 및 `R11StageClear_D/index.json`, **01:35:12 KST 성공1/실패0**. 각 단계의 초기 라이플1개·플레이어1/적2·유효한 비부분 Nav 경로를 확인했다. PIE3.061/5.175/7.285초 처치 후 단계0→1→2→결과, 원본 사망 수명 후15.417초에 봇 Controller0/스포너3개 각 생성1·클리어를 확인했다. 경고15(PoseAsset14/초기 Recast1), 로그 Error/ensure/Fatal0이다. Clear_A는 UI Python wrapper, B는 NavigationSystem CDO의 World 조회 ensure, C는 보호된 NavigationSystem 속성 접근으로 실패했다. 공개 UFunction을 실제 PIE World 소유 NavigationSystem에 호출하도록 교정한 D만 최종 통과로 삼는다.
- **실패·재시작·복귀 실제 PIE 통과:** `pie_Travel_A.json`, `R11StageTravel_A/index.json`, **01:37:46 KST 성공1/실패0**. 플레이어 GAS 사망2.093초→실패/봇·스포너 정리→자동 부활 없이10.420초에 명시적 재시작1회 수락/중복 거부→실제 World 재로드→HP100/라이플1/입력 차단 해제/단계 UI1개→이동68.84cm→명시적 R10 유한 맵 복귀를 확인했다. 복귀 후 R11 UI0/이동·시점·Ability 입력 차단0이다. 경고17(PoseAsset14/세 World 초기 Recast3), 로그 Error/ensure/Fatal0이다.
- **보존/검증 경계:** 빌드 후 기준 `Saved/Diagnostics/R11/postbuild_before_A.json`의615파일(기존/이번 빌드 소스·Config·기존 테스트/AI/디자이너 맵·DLL·Git 인덱스·uproject)은 유지했다. 추가 변경은 R11 Python2/새 에셋2/문서이며 기존 플레이어·맵·팀 정책은 보존했다. 제어된 피해 GE·임시 면역·입력 주입을 사용한 standalone PIE다. 물리 버튼 클릭·실제 무기 처치·화면/조작감·복제·실제 디자이너 맵 활성화·R12 보스 패턴·R13 정식 결과 UI 완료는 아니다. 원본 GI import/Niagara metadata 등의 기동 경고 이력도 남긴다. Computer Use·Git stage/commit/merge0이다.
- **다음:** 추가 빌드/수동 에셋 설정 없이 새 `L_DC_Level1Flow`에서 단계 제목/결과 화면, 실제 사격으로3단계 진행, 사망 후 `다시 시작`/`돌아가기` 클릭을 사용자가 짧게 확인한다. 마지막 적은 같은 Lyra 봇인 Placeholder다. 이 확인 후 R12 실제 보스 변경 범위를 먼저 설명·승인받으며 자동으로 범위를 확대하지 않는다. 10월12일의 R11/R12 실행 판단 기준 중 R11 자동 흐름은13일01:37에 확보했으나 R12·사용자 수용·디자이너 통합은 미완료다. 상세 근거는 인계 문서와 `integration_summary_A.json`을 따른다.

---

## 0.91 이력 — 2026-10-13 R11 사용자 수용 완료 / R12 변경안 제시, 구현 승인 전

- 사용자가 `L_DC_Level1Flow`의 PIE 정상 작동을 확인하고 **“R11은 정상 확인한 것으로 정리”**하도록 요청했다. **승인한 R11-1~3의 별도 Level1 전투1→전투2→보스/결과 Placeholder, 실패·수동 재시작·맵 복귀 묶음은 자동 검사와 사용자 수용까지 완료**다. 자동 검사 근거는0.90을 유지하고, 사용자 확인을 Codex의 추가 물리 버튼/시각 검사로 표현하지 않는다. 완료된 검사·사본 생성은 반복하지 않는다.
- `돌아가기`는 기존 R10 유한 스포너 맵 `L_DC_LyraFiniteEncounter`를 임시 ReturnMap으로 사용한다는 설명을 사용자가 이해했다. 로비/스테이지 선택 화면은 이번에 변경하지 않는다. R12 실제 보스·R13 정식 UI·복제·디자이너 맵 활성화/병합까지 완료된 것은 아니다.
- **R12 제안/아직 미승인:** 보스 전용 Character/페이즈 조율/설정 데이터/체력 UI, 패턴2개(조준 고정 예고 단발＋지면 위치 고정 예고 범위 공격), 체력 전환1회, 기존 사망→인카운터→결과 연결을 한 묶음으로 제안한다. 제자리 전투·기존 임시 외형·HP50% 전환은 승인받을 시작안이며 BIG BROTHER 최종 설계가 아니다. 기존 `DCGASEnemyCharacter`의 원본 기반 GAS/Health/Death, `DCGameplayAbility_EnemyRangedAttack`의 선딜/조준 고정/명중/후딜/취소를 재사용하는 안이다. 일반 적의 Lyra BT/무기 경로는 그대로 유지한다. 보스 패턴·페이즈는 프로젝트 고유 규칙으로 명시한다.
- **범위/근거:** 스포너 EndPlay는 현재 Lyra 소유 객체만 정리하고 일반 GAS 경로는 사망 집계 시 구독을 해제한다. 기존 GASEnemyClass로 보스를 생성하려면 보스에 한정한 opt-in 소유 추적/지연 정리와 현재 스테이지 플레이어 조회 훅을 함께 보강해야 한다. 새 `/Game/DreamCatcher/GAS/Test/R12/L_DC_Level1Boss` 사본에서 마지막 스포너를 보스로 연결하고 R11 원본은 보존하는 안이다. 로컬 원본 C++/에셋명 검색에서 동일 보스 계층은 확인하지 못했으나 전체 BP 그래프 부재를 증명한 것은 아니다. 원본 Health/Cue/Animation/UI 재사용과 신규 예고/범위 판정을 구분한다. 자세한 코드·에셋·검증 범위는 기능 명세 R12 제안 절을 따른다.
- 이번 턴은 **AGENTS·기능 명세·인계 문서3개 갱신과 로컬 소스 읽기만** 수행했다. R12 코드·Config·에셋·자동화 신규 작성/Editor 실행·빌드·패키징·Computer Use·Git 조작0이다. 다음은 R12 묶음에 대한 사용자 승인/설계 조정이며, 승인 후 코드/에셋 자동화 준비→사용자 빌드→연결/자동 검사→사용자 짧은 플레이 확인 순서다.

---

## 0.92 이력 — 2026-10-13 승인된 R12 보스 코드·자동화 준비 / 사용자 빌드 대기

- 사용자가 제안한 R12 묶음을 승인하고 **기존 플레이어와 R11 맵 유지**를 재확인했다. R11 사용자 수용 완료는 유지하며 완료된 마이그레이션/PIE를 반복하지 않는다. 현재 R12는 코드와 빌드 후 레시피 작성까지이며 **UHT/C++ 빌드·새 native/에셋/PIE는 미검증**이다.
- **기존3소스 변경:** `AI/DCEnemySpawner.h/.cpp`에 기본false인 `bCleanupOwnedGASNPCs`를 추가했다. R12가 켜면 GAS NPC의 처치 집계와 소유 추적을 분리하고 DeathFinished/OnDestroyed 이후 또는 실패/중단 시 자기 Pawn/AI Controller만 지연 정리한다. 기존 Lyra/legacy 설정은 유지한다. `GameModes/DCStageRunGameMode.h`에는 현재 스테이지 플레이어 조회 getter만 추가했다. 기존 Stage/Encounter·플레이어·NPC/원본 단발 본체·Config·R11 에셋은 수정하지 않았다.
- **신규 런타임14소스:** `AI/Boss/DCBossCharacter`, `DCBossData`, `DCBossPhaseComponent`, `DCBossTelegraph`, `AbilitySystem/Abilities/DCGameplayAbility_BossLineShot`, `DCGameplayAbility_BossGroundStrike`, `UI/Boss/DCBossHealthWidget`의 h/cpp7쌍. 기존 GAS NPC에서 원본 기반 Health/ASC/Death/팀을 재사용한다. 제자리 보스는 별도 Controller/BT 없이 Stage 플레이어를 대상으로 두 Ability를 순서대로 요청한다. 단발 본체는 R10 클래스를 상속하고 새 코드에는 예고/발사 표시 훅과 동시 실행 차단만 둔다. 범위 공격은 고정 지면·선딜·적대 대상 중복 제거/원형 거리·발 위치 높이·차폐 검사 후 기존 DamageExecution GE로1회 적용한다.
- **페이즈/UI/예고:** HP50% 전환을 한 번만 잠그고 진행 중 두 공격을 취소한 뒤 전환 안내,2페이즈부터 짧은 패턴 간격을 사용한다. 체력0은 전환보다 사망을 우선하며 원본 Death Ability를 취소하지 않는다. Health 이벤트로 보스 이름/체력/페이즈를 native UMG에 표시하고 사망·실패·World 종료에서 제거한다. 선/원형 예고는 충돌/피해/Nav 영향이 없는 transient mesh actor다. 새 예고/보스 UI 배치는 프로젝트 구현이며 원본 보스 시스템 이식으로 표현하지 않는다. 기존 검증된 NPC 메시/Anim/Death를 연결할 레시피를 작성했다. 원본 UI 시각 자산 후보의 추가 재사용/최종 공격 모션·가독성은 미검증이다.
- **임시 조정값:** Health600, 전환50%/1.5초, 첫 공격1.5초 대기, 패턴 사이 간격1→0.35초. 단발 피해15/선딜1.2/후딜0.65초, 범위 피해18/반경220cm/선딜1.4/후딜0.7초, 사거리2500cm. 수치는 BP/DataAsset에서 조정하며 최종 밸런스가 아니다. 자동 리스폰/기존 ReturnMap 변경은 없다.
- **새 검사5건 작성:** `Tests/DCBossTestTypes.h`, `DCBossAutomationTests.cpp`, prefix `DreamCatcher.R12.Boss`. 두 패턴의 선딜/실제 GE 명중·상호 배제, 지면 고정/범위 밖/차폐/원본 면역, 전환1회/치명타 우선, 타깃 사망/취소·예고 정리, opt-in GAS 소유 중단/사망 집계 이후 정리를 검사하도록 작성했다. `-DCR12BossIsolatedAutomation -DCLyraRuntimeDiagnostics`가 있는 별도 unattended editor에서 실행할 예정이며 아직 실행하지 않았다.
- **신규 Python2/에셋 계획:** `r12_boss_setup.py` apply/verify는 새 `/Game/DreamCatcher/GAS/Test/R12/`의 `L_DC_Level1Boss`, Boss/Spawner BP2개, DataAsset/AbilitySet2개, GA2개, 예고/명중 Material2개 **총9에셋**만 생성한다. R11에서 내부 Template 사본을 만들고 마지막 스포너만 바꾸며 NPC 캡슐 차이만큼 높이를 보정한다. 기존 R11 GameMode/Experience/플레이어/첫 두 전투를 재사용한다. 새 경로가 이미 있으면 덮어쓰지 않고 중단한다. `r12_boss_probe.py`의 combat/fail-retry-return 실제 PIE 시나리오를 준비했다. 제어 GE/임시 면역/입력 주입과 plain-data 여행 상태를 사용하므로 물리 사격·버튼·시각/복제 확인과 구분한다.
- **정적 보존:** `Saved/Diagnostics/R12/before_A.json`에 기존617파일 해시/수정3소스 원문을 보존했고 허용3개 외614개 불변이다. 신규18파일(런타임14＋검사2＋Python2), 기존3소스, 문서3개를 작성했다. 최종 정적 근거 `static_C.json`: Python AST/공백/generated include/로컬 include·기본 구문 대조 통과. C++/UHT 성공이 아니며 R12 에셋 폴더도 아직 없다. 빌드/패키징/Editor/Computer Use/Git stage·commit·merge0이다.
- **다음:** 사용자 Development Editor/Win64 빌드 → 새R12 5건＋직접 영향 R10 finite6/Enemy3 → 신규9에셋 apply/별도 프로세스 verify → 새 맵의120초 실제 PIE combat와fail-retry-return → 사용자 가독성/실제 사격/화면 확인. 상세 실행/부분 실패 처리와 제한은 인계 문서 최신 절을 따른다. R12 완료·시각 품질·복제·디자이너 통합으로 확대하지 않는다.

---

## 0.93 이력 — 2026-10-13 R12 보스 에셋·실제 PIE 자동 검사 통과 / 사용자 플레이 확인 대기

- **빌드/native 확인:** Slot→BossPanelSlot 교정 후 사용자 빌드6.80초 `Succeeded`, DLL02:37:02 KST/5,603,328 bytes. 원문은 `Saved/Diagnostics/R12/user_build_A.txt`다. **02:38:23 KST `R12BossNative_A/index.json` 성공14/실패0**(R12 5＋관련 R10 9), 기존 빈 스포너 fixture 경고1/로그 Error·ensure·Fatal0을 확인했다. Codex는 빌드·패키징하지 않았다.
- **신규9에셋 연결:** `/Game/DreamCatcher/GAS/Test/R12/L_DC_Level1Boss` 및0.92의 보스/스포너/GA2/Data/AbilitySet/Material2를 생성했다. Apply_A 저장/Verify_A 별도 재로드가 통과했다. R11 환경1106액터 클래스/Transform은 일치하며 첫 두 전투·GameMode/Experience/ReturnMap을 유지했다. 마지막 보스 스폰은 `(1035,165,63)`이며 기존 Lyra Pawn 대비 캡슐 반높이−8cm를 반영했다. 기존 플레이어/R11 맵은 저장하지 않았다.
- **실행에서 발견한 문제와 해결:** Combat_A는 보스 두 패턴/전환/정리를 관찰했지만 보스 자폭 진단의 Elimination.Instigator가 NPC였고 기존 Reticle의 PlayerState 가정으로 오류가 났다. 새 Material2개의 InstancedStaticMeshes 사용 플래그도 누락돼 기본 재질 경고가 발생했다. R12 Material2개만 사용 플래그를 설정·재컴파일/저장했고 Verify_B에서 재로드 통과했다. 보스 처치 진단은 플레이어 ASC→보스 피해로 교정했다. 이어 **Travel_A의 보스→플레이어 치명 피해에서도 동일 Reticle 오류가 재현**되어 진단 전용 문제라는 초기 범위를 정정했다. 두 A 보고서는 실패로 보존하며 probe의 complete=true만으로 완료 처리하지 않는다.
- **사용자 추가 승인 UI 수정:** 사용자가 공용 `/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/W_Reticle_Rifle`의 최소 가드 수정을 승인했다. 현재 파일이 기존 R7 그래프 감사와 같은 해시임을 확인하고, Elimination 콜백과 기존 조건 분기 사이에 `K2Node_DynamicCast_1.bSuccess`를 검사하는 Branch1개(`K2Node_IfThenElse_5`)만 추가했다. 성공은 기존 분기로, 실패는 피드백 없이 종료한다. 정상 PlayerState 처치 처리/화면 배치·기존 노드/기본값/다른 연결은 유지했다. `reticle_guard_Apply_A.json` Compile/저장, `reticle_guard_Verify_A.json`03:02 KST 별도 재로드/정확한 그래프 차이 검증 통과. 플레이어 Pawn/Controller·R11 맵·C++/Config 수정은 없다.
- **보스 클리어 실제 PIE:** `pie_Combat_B.json`, `R12BossCombat_B/index.json` **03:00:11 KST 성공1/실패0/경고15**. 보스HP600, 단발/범위 예고,12.269초 임계값 피해 중 기존 예고 취소,13.808초2페이즈,2페이즈 두 공격,17.902초 치명 피해→19.954초 보스/봇/예고/체력 UI 정리·결과를 확인했다. 이 검사는 공용 UI 가드 적용 전이며 플레이어 공격자 경로를 사용했다.
- **수정 후 양쪽 공격자·여행 PIE:** `pie_Travel_B.json`, `R12BossTravel_B/index.json` **03:06:20 KST 성공1/실패0/경고17**. 가드 적용 후 플레이어가 앞선 적2명을 처치하는 정상 PlayerState 경로와 보스가 플레이어를 처치하는 NPC 경로를 모두 확인했다. 보스 예고 중 플레이어 사망12.227초→보스/예고/UI/스포너 정리→자동 부활 없이20.537초 명시적 retry/중복 거부→새 World의 HP/라이플/입력·UI 복원/이동104.39cm→기존 R10 유한 맵 복귀를 확인했다. 두 최종 PIE 로그 Error/ensure/Fatal0이고 재질 사용 플래그 경고도0이다. 경고는 기존 PoseAsset14＋World 초기 Recast1/3이며 기동 GI import/Niagara metadata 이력도 유지한다.
- **보존/경계/다음:** 빌드 후 기준633파일은 모두 불변이며 추가로 승인한 공용 Reticle1개만 기존 에셋 예외로 수정했다. 신규 R12 에셋9개/Python2/문서와 진단 영수증을 정리했다. 근거 `Saved/Diagnostics/R12/integration_summary_A.json`. C++/Config 추가 수정·재빌드 필요·Computer Use·Git 조작0이다. 제어된 GE 피해/임시 면역/입력 주입을 사용한 standalone 검사이며 실제 무기 처치·물리 버튼·예고의 화면 가독성/공격 모션·조작감·복제는 사용자/후속 검증이다. **현재 다음은 추가 빌드나 수동 에셋 설정 없이 새 `L_DC_Level1Boss`의 짧은 사용자 플레이 확인**이다. R11 완료 확인을 다시 요구하지 않는다. 임시 모델·수치/R13 정식 HUD·로비/디자이너 통합을 완료로 확대하지 않는다.

---

## 0.94 이력 — 2026-10-13 R12 사용자 수용 완료 / R13 변경안 제시, 승인 전

- 사용자가 **“PIE에서 보스전 정상 작동 확인”, “R12는 정상 확인한 것으로 정리”**를 명시했다. 승인한 최소 보스의 두 패턴·체력 전환·HUD·사망/결과 연결 묶음은0.93의 자동 검사와 사용자 PIE 수용까지 완료다. 이를 Codex의 추가 물리 입력/화면 검사로 표현하지 않는다. R11/R12 완료 검사·마이그레이션은 반복하지 않으며 임시 모델/수치를 최종 보스 연출·밸런스/복제·디자이너 통합 완료로 확대하지 않는다.
- **R13 현재 사실:** 원본 기반 ExperienceManager/GameMode/HUD/HUDLayout/AddWidgets/UIExtension/Root 및 QuickSlot 입력은 이미 있다. 현재 Reticle은 DCRifleIntegrationController가 직접 생성하고, QuickBar 표시는 이전 검사에서만 생성했으므로 정식 HUD 슬롯 연결은 남아 있다. R11/R12는 이미 Experience를 쓰며 과거 R5 맵의 미연결 상태로 되돌려 판단하지 않는다.
- **원본 근거/제약:** 기존 UIAudit_20261008_A3의 StandardHUD/Layout 원본 파일 해시가 현재와 일치한다. Layout1＋extension widget11, Reticle→HUD.Slot.Reticle/QuickBar→HUD.Slot.Equipment, GameAndMenu/CapturePermanently, Esc 메뉴/장치 연결 화면을 유지하는 방향이다. 원본 GameSettings 플러그인/SettingScreen/SettingRegistry는 대상에 없으며 원본 Registry의 설정·Replay/DLC 관련 의존성도 검토해야 한다. StandardHUD의 ShooterCore plugin 요구를 그대로 활성화하면 대상에서 찾지 못하는 경로가 있으므로, UI 전용 프로젝트 Feature/ActionSet으로 명시적으로 대응하고 기존 게임플레이 컴포넌트를 중복 부여하지 않는 안이다. 원본 기능을 삭제해 로드를 통과시키지 않는다.
- **제안/아직 미승인:** R13-A 원본 HUD·공통 위젯·Esc/설정·장치 의존성 이식 및 Experience/ActionSet/AddWidgets 연결, R13-B 단계/보스/결과 표시의 UI 레이어·입력 수명 통합, R13-C 남은 원본 이식 목록 및 확정된 추가 무기/기술 표시 점검 순서다. 새 `/Game/DreamCatcher/GAS/Test/R13/L_DC_Level1UI` 사본과 전용 Controller/GameMode/Experience/ActionSet을 사용하고 기존 플레이어/R11/R12 맵은 보존한다. 새 Controller 설정에서만 직접 Reticle 표시를 끄며 기능 확인 전 기존 경로를 삭제하지 않는다. 추가 무기 수·종류, 로비/스테이지 선택·온라인 FrontEnd·보상/저장 체계는 임의 포함하지 않는다. 정확한 변경 범위/미검증 항목은 기능 명세 R13 제안 절을 따른다.
- 이번 턴은 문서3개 갱신과 로컬 소스/기존 감사/원본 해시 읽기만 수행했다. R13 코드·Config·에셋·플러그인 등록/Editor·검사 재실행·빌드·패키징·Computer Use·Git 조작0이다. 다음은 변경안에 대한 사용자 승인/조정이다. 승인 후 원본 의존성/매핑 확정→코드 준비→사용자 빌드→새 에셋 연결/자동 검사→사용자 플레이 확인 순서를 따른다.

---

## 0.95 이력 — 2026-10-13 승인된 R13 1·2번 코드 준비 / 사용자 빌드 대기

- 사용자가 제안한 **원본 HUD·Esc/설정 메뉴 연결＋스테이지 UI/입력 수명 통합** 진행을 승인했다. R11/R12 사용자 수용은 유지하며 기존 플레이어/두 맵/보스·전투 에셋을 보존한다. 현재는 아래 코드·Config·플러그인 준비와 원본 읽기 전용 조사까지다. **새 코드의 UHT/C++ 빌드·target native/에셋/PIE는 미검증이고 새 R13 UI 에셋/맵은 아직 없다.**
- **원본 재사용:** 원본 GameSettings 플러그인 텍스트62파일을 그대로 추가하고, Settings Registry/CustomSettings/화면·키 입력행, 공통 UI/확인창/메시징, 터치, 성능 표시 및 직접 Replay 의존성71 h/cpp를 별도 DC 이름으로 이식했다. 이름/include/export 대응 뒤70파일 본문이 일치하며 SettingScreen h1개만 미적용 설정 취소용 공개 연결을 추가했다. DLC 코드를 삭제하지 않고 엔진 PlatformDLC 의존성을 사용한다. Replay Subsystem은 원본 구현을 보존하며 자동 녹화/온라인 FrontEnd 전체 활성화·검증을 완료로 보고하지 않는다. 원본 이식/대응 목록은 `Saved/Diagnostics/R13/native_applied_A.json`이다.
- **R13 표시/입력 연결 코드:** 신규 StageResultLayer/StageOverlayWidget/StageUIComponent/StageUIBlueprintLibrary h/cpp8개. CommonUI 레이어에 기존 결과 버튼을 감싸고, 새 HUD 슬롯에서 단계/보스 상태를 표시하며 Menu/Modal의 표시 이벤트에 맞춰 자기 입력 차단 태그/이동·시점 무시만 관리한다. 결과는 Back/Esc로 닫히지 않으며 기존 명시적 retry/return API를 사용한다. 사망/전환에서는 미적용 설정만 취소하고 원본 Death Ability는 유지한다. 아직 실제 레이어/메뉴/비동기 로딩·포커스 검증은 아니다.
- **기존 코드 연결:** StageRunGameMode에 기본false `bUseExperienceUI`, 상태 조회/메뉴 복귀 API를 추가했다. true인 새 R13 구성만 직접 StageWidget 대신 레이어 표시를 사용하고, 보스도 직접 HUD 생성을 생략한다. 기존 R11/R12 기본경로·게임 판정은 유지한다. StageResultWidget에는 기본true인 제목 표시 옵션만 추가했다. DCLocalPlayer에는 원본 GetLocalSettings/오디오 장치 변경 훅을 복원했고 CDO에는 구독하지 않는다. SettingsLocal/Shared 본체를 교체하지 않았다.
- **설정/플러그인:** Build.cs에 GameSettings/NetworkReplayStreaming/PlatformDLC/RHI/RenderCore 의존성, .uproject에 GameSettings/PlatformDLC/DCStageUI를 등록했다. UI 전용 DCStageUI는 Content plugin/ExplicitlyLoaded/초기 Installed이며 아직 FeatureData 에셋/활성화는 없다. DefaultGame에 R13 Experience/ActionSet 경로, DefaultEngine에 신규 native Redirect38개, DefaultGameplayTags에 원본 HUD 슬롯10개, DefaultInput에 원본 UI.Action.Escape(Escape/게임패드 Start)와 EnhancedInput 사용자 설정 지원을 추가했다. 전투 IMC/기존 키 배치·전역 GameMode·기존 UI Policy/ReturnMap은 변경하지 않았다. 키 재지정 UI/설정 저장은 후속 실제 연결과 사용자 설정 보존 조건으로 검증한다.
- **원본 읽기 전용 조사:** `r13_ui_dependency_inspect.py`/`ui_dependencies_Source_A.json`은 원본539패키지의 레지스트리 의존성과 native parent를 조사했다. `menu_graph_A.json`에서 ReturnButton 확인창→기존 OpenLevel 및 FrontEndMapReference/FrontEndExperienceReference를 확인했다. 원본 메뉴→FrontEnd 맵→FrontendStateComponent 경로를 새 메뉴 사본에서 승인된 Stage 복귀 API로 대응하면 UI 범위308패키지다(신규 복사 수 아님). 원본 에셋 저장/명시적Compile0. 기존 R7 위젯·Root 재사용 및 새 사본의 복귀 실행선/미사용 FrontEnd 기본값 대응 계획은 `asset_plan_A.json`이다. 원본에서 해당 기능을 지운 것이 아니다.
- **검사 준비/보존:** 새 `DreamCatcher.R13.UI`2건(MenuReturnIsOptIn/MenuInputOwnership)을 작성했다. 명시적·opt-in 복귀/중복 차단과 자기 태그·입력 무시의 재진입/EndPlay 회수를 검사하며 실제 UI/설정/맵 이동은 별도다. `r13_native_verify.py`는 빌드 후 클래스 등록/default 검사용이며 설정 저장/에셋 저장은 하지 않는다. `before_A.json`의 기존863파일 중 허용13파일만 변경, 나머지850개 및 기존 Content/DLL/Git 인덱스는 불변이다. 신규146파일(원본 게임71＋UI연결8＋검사2＋Python2＋GameSettings62＋UI Feature descriptor1)을 추가했다. `static_A.json`은 정적/원본 조사 결과이지 UHT/실행 성공이 아니다. Codex 빌드/패키징/Computer Use/Git 조작0.
- **다음:** 사용자 Development Editor/Win64 빌드 → R13 native2건/변경된 Stage 관련 회귀와 native 등록 확인 → 308패키지의 기존 재사용/새 경로 매핑·충돌 확인 및 실제 UI 사본 준비/이식 → 새 FeatureData·Experience/ActionSet·Controller/GameMode·맵 연결 → 메뉴/설정/NPC 메시지/레이어/입력·여행 실제 PIE → 사용자 확인. 대규모 이식 도구를 새로 만든 상태가 아니며 에셋 자동화의 구체적 저장/그래프 연결과 후속 실행 검증은 아직 남았다. 상세 재개는 인계 문서 최신 부분을 따른다.

---

## 0.96 이력 — 2026-10-13 R13 기반 검사 통과 / 알림 의존성 보완, 사용자 재빌드 대기

- 사용자 빌드 `user_build_A.txt`는 **7.08초 성공**, DLL은 **22:23:28 KST / 6,537,216 bytes**다. 앞선 enum 이름 및 PropertyPath 링크 누락 교정 후 결과다. **22:25:40 R13 UI2＋관련 R11 Stage5 총7건 성공/실패·보고서경고0**, 로그 Error/ensure/Fatal0. `native_verify_Verify_A.json`의 클래스44개 등록과 기존 Stage 기본false도 통과했다. 이 검사를 다시 일괄 반복하지 않는다.
- 실제 HUD 조회에서 이전 parent 조사에 빠진 **ShooterCoreRuntime.LyraAccoladeHostWidget**과 알림 데이터/Relay의 의존성을 확인했다. 승인된 원본 HUD/알림 연결 범위에서 원본 AccoladeDefinition/Host4, NotificationMessage/GameplayMessageProcessor4, ElimChain/ElimStreak/AssistProcessor6의 **14 h/cpp**를 추가했다. 이름/include/API 및 같은 모듈 내부 tag 변수 충돌 방지 이름만 대응하고 본문 로직은 유지했다. 근거 `notification_native_A.json`. **이 추가14개는 아직 사용자 빌드 전**이며 앞선7건 성공을 새 코드 검증으로 확대하지 않는다.
- Build.cs에 DataRegistry, UI Feature descriptor에 DataRegistry 의존성을 추가했다. DefaultEngine native Redirect11개, DefaultGameplayTags 원본 알림/처치 아이콘 태그14개를 추가했다. 새 무기/피해 규칙을 만드는 것이 아니며 기존 전투 판정/플레이어/맵은 변경하지 않았다. 원본 Feature의 Inventory/Equipment/팀 컴포넌트 전체를 중복 등록하지 않는다.
- `r13_ui_assets.py`의 원본 HUD/설정/확인창 조사 및 알림 보완으로 **새 후보214 / 기존 재사용49**를 확정했다. 실제 이식 완료 수가 아니다. 공용17개의 속성/텍스트를 비교했고16개는 동일, `sfx_RandomStereo_nl_meta`는 root graph class ID만 다르다. 해당 공용 오디오는 보존하며 후속 새 preset의 reference ID 대응/재로드·실행은 아직 남는다.
- 원본 프로젝트의 새 `/Game/LyraMigration/UI/R13/Game/UI/Hud/W_LyraGameMenu` **사본1개만 저장**했다. FrontEndMapReference/ExperienceReference를 비웠으며 확인 후 실행을 프로젝트 ReturnFromStageMenu에 연결하는 단계는 대상 DLL 뒤에 남는다. `Prepare_D`/`MenuVerify_A`에서 그래프·위젯 구조/문구 보존, 별도 재로드/Compile, 원본 로비 package 의존 제거를 확인했다. 복제로 재생성된 확인창 text key2개만 명시적으로 구분했으며 번역 동등성 검증은 아니다. 원본/기존 에셋은 불변이다.
- **현재 다음은 사용자 Development Editor/Win64 재빌드다.** 새14개 등록은 `r13_native_verify.py -DCR13NativeScope=notification -DCR13NativeRun=<fresh>`로 클래스6/struct5만 확인한다. 이후 원본214개를 기존 Unreal 내부 이식 방법으로 새 R13 경로에 연결하고 FeatureData·Experience/ActionSet·새 Controller/GameMode/맵을 조립한다. **대상 R13 에셋/맵은 아직 없고 실제 HUD·메뉴·설정 저장·NPC 처치 피드·입력/여행 PIE는 미검증**이다. R11/R12 수용 유지. 세부 실행 근거와 재개 제한은 인계 문서0.96을 따른다.
- 보존: 이번 빌드 후825파일 중 허용4개(Build.cs/DefaultEngine/DefaultGameplayTags/DCStageUI descriptor) 외821개 불변. 신규14코드·에셋 recipe1/기존 native 확인 recipe 보완·문서3개 작성. Codex 빌드/패키징/Computer Use/Git stage·commit·merge0이다.

---

## 0.97 이력 — 2026-10-14 R13 HUD·메뉴·맵 연결 및 자동 PIE 통과 / 사용자 화면 확인 대기

- **추가 빌드는 필요 없다.** 사용자 알림14파일 빌드32.10초/DLL10월13일22:57:46, 후속 원본 `DCPlayerState.ClientBroadcastMessage` 알림 RPC 빌드54.90초/DLL23:14 갱신을 확인했다. RPC는 원본의 Client/Unreliable 및 NM_Client에서만 전달하는 본문을 유지한다. 기존 플레이어 입력·전투/팀·스폰 로직을 바꾸지 않았다. `LyraGame.LyraTeamSubsystem`→기존 DCTeamSubsystem Redirect도 연결했다.
- 원본 HUD/메뉴/설정·알림214개와 Config 지정 Back 입력 데이터/Windows 키보드·XSX·PS5 아이콘/RichText/Throbber82개를 **새 R13 경로에 이식·재로드/Compile**했다. 기존 R7 Root/QuickBar/ReticleHost/입력 에셋은 재사용한다. 첫214개의 Blueprint66개와 후속82개를 임시 alias 없이 검증했다. 기존 공용 Reticle 가드는 보존했다. 이동 중 생성된 새 중간 경로 redirector는 삭제하지 않았으며 최종 저장 에셋의 구경로 package/pin 의존은 점검했다.
- 신규 조립7개: `/Game/DreamCatcher/GAS/Test/R13/`의 `BP_DC_StageUIController`, `BP_DC_StageUIGameMode`, `B_DC_StageUIExperience`, `LAS_DC_StageHUD`, `LAS_DC_StageNotifications`, **`L_DC_Level1UI`**, 그리고 `/DCStageUI/DCStageUI` FeatureData. 원본 Layout1/Widget11＋단계·보스 표시1, 원본 KillFeed/Accolade/Registry와 새 UI 입력 수명 연결을 Experience에서 관리한다. 기존 팀/Inventory/Equipment/스폰은 중복 등록하지 않는다.
- 새 R13 Controller만 직접 Reticle 표시를 끄고, 새 GameMode만 bUseExperienceUI=true다. R12 환경1106액터/인카운터·보스 구성은 보존했다. UMG 내부 GUID를 조작하지 않고 원본의 비어 있던 `HUD.Slot.ModeStatus`를 새 R13 사본에서 전체 화면 Overlay로 사용한다. 원본 메뉴 확인창 후 기존 Stage 복귀 API를 호출하며 ReturnMap은 기존 R10 유한 스포너 맵이다.
- 새 사본8개(메뉴/Layout/오디오6개)의 연결 및 새 처치 피드2개를 보완했다. Relay는 원본의 PlayerState 가정을 안전한 이름 조회로 교정하여 정상 PlayerState 이름은 유지하고 null/non-PlayerState는 `Enemy`로 표시한다. 팀은 메시지 객체를 기존 TeamSubsystem에 조회하며 null은 NoTeam이다. EntryWidget의 표시용 색상 인덱스만 기존0~2 범위로 제한해 NoTeam 경고를 제거했다. 메시지/피해/사망 집계·실제 팀은 바꾸지 않았다. 재현된 오류/경고에 대한 새 R13 사본2개만 수정했다.
- **최종 자동 PIE 통과:** `R13Lifecycle_B` 성공1/실패0/경고17, `R13CombatUI_C` 성공1/실패0/경고15. 경고는 기존 Manny PoseAsset14와 Recast3/1이며 두 로그 Error/ensure/Fatal0. HUD 단일 생성/설정125항목/메뉴 차단·해제/메뉴 중 사망→결과/명시적 retry/원본 확인창 버튼→R10 복귀와 입력·활성 UI 정리, 일반 처치 피드/연속 처치 알림/HP600 보스 표시/NPC 보스 처치→클리어를 확인했다. 제어된 GE 피해·면역/버튼 handler 호출이며 물리 입력·실제 무기·화면 가독성·설정 적용/저장·게임패드/터치·복제 수용은 아니다. 사용자 설정 파일 저장/변경0.
- Config는 새 확인/오류 대화상자와 원본 CommonUI InputData/RichText/Throbber, 원본 Windows UI glyph/지원 trait에 연결했다. 기존 전투 IMC/키 매핑과 플레이어/R11/R12 에셋은 그대로다. **현재 다음은 새 R13 맵의 사용자 PIE 화면/조작 확인**이며 R13 전체/리플레이 자동 녹화/새 로비·온라인/디자이너 통합·R14 완료로 확대하지 않는다. 완료한 import/setup/자동 검사는 반복하지 않는다. 상세 근거/실패 이력/재개 제한은 인계0.97과 `Saved/Diagnostics/R13/integration_summary_B.json`을 따른다. Codex C++ 빌드/패키징·Computer Use·Git stage/commit/merge0.

---

## 0.98 이력 — 2026-10-14 R13 사용자 부분 수용 / 상태 HUD 배치 교정

- 사용자는 보스 체력·현재 페이즈 표시의 왼쪽 잘림과 Esc가 PIE를 종료하는 문제를 제외한 나머지 동작을 정상 확인했다. 그 수용을 유지하며 완료된 마이그레이션/전투/결과 흐름 검사를 반복하지 않는다. 메뉴·설정 수용을 이 보고에서 추정하지 않는다.
- 원인은 새 R13 `W_ShooterHUDLayout.ExtensionPoint_ModeStatus`의 비어 있는 `SpacingPattern`이다. UE5.8 `DynamicEntryBoxBase.cpp`의 Overlay 경로는 패턴이 비면 기존 Fill 정렬을 EntrySpacing 방향의 Left/Top으로 덮어쓴다. 전체 Canvas 크기가 필요한 StageOverlay의 중앙 기준과 맞지 않는다. 기존 Fill 값은 유지하고 새 R13 사본의 패턴만 `(0,0)` 한 항목으로 설정했다. 엔진/UIExtension 코드·기존 플레이어·R11/R12/R13 맵·Config는 수정하지 않았다.
- `status_layout_fix_B.json`은 사본1개 저장/그래프·트리·Canvas 설정 보존/보호 파일 변경0, `status_layout_verify_A.json`은 새 프로세스 재로드·Compile/Fill·0패턴 확인/저장0이다. 기존 setup recipe에도 같은 값을 반영했지만 setup을 재실행하지 않았다. C++ 재빌드가 필요 없다.
- 자동 화면 좌표 측정은 검증 완료가 아니다. GeometryBefore_A는 Python 클래스 이름 문제로 미실행, Before_B는 offscreen PIE의 HUD 전체 캐시 크기가0이라 판정 불가였다. Fix_A는 사용자가 열어둔 Editor의 파일 잠금 때문에 저장 실패했고, 사용자가 에디터 종료를 확인한 뒤 Fix_B로 저장했다. 실패 이력은 보존한다.
- **다음은 기존 R13 맵에서 두 표시의 상단 중앙 배치와 Esc 메뉴만 사용자 확인**이다. UE 기본 `Play World (PIE/SIE) > Stop`이 Esc라 메뉴 입력보다 먼저 PIE가 종료된다. Editor Preferences의 Keyboard Shortcuts에서 Stop의 Esc를 다른 조합으로 바꾸어 확인한다. Codex는 사용자의 편집기 단축키를 바꾸지 않았다. Computer Use OFF, 빌드/패키징·Git 조작0. R13 전체/설정 적용·저장/장치·복제/R14 수용으로 확대하지 않는다.

---

## 0.99 이력 — 2026-10-14 R13 사용자 수용 완료 / R14 변경안 제시

- 사용자는 보스 체력·페이즈 표시와 Esc 메뉴의 정상 동작을 추가 확인하고 R13 정리를 요청했다. **승인된 R13 HUD·메뉴·Experience·맵 연결 묶음은 자동 검사와 사용자 PIE 수용까지 완료**다. 이전의 배치/메뉴 확인 대기는 해소됐다. 완료한 이식·연결·검사를 다시 실행하지 않는다. 설정 변경의 재실행 후 영속성, 모든 장치/복제, 추가 무기·온라인/자동 리플레이 등 별도 미검증 항목까지 완료한 뜻은 아니다.
- 이번 요청에서는 상태 문서3개만 수정했다. C++/Config/에셋·기존 플레이어/맵·Git 상태를 변경하지 않았고 Editor 실행/빌드/패키징/Computer Use도 하지 않았다. 기존 작업 변경은 그대로 유지한다.
- R14의 첫 묶음은 **참조 조사와 정리 대상 확정**으로 제안한다. 현재 R13의 PawnData/Experience/GameFeature/Inventory·Equipment/Ability/UI 경로를 기준으로, 구형 Combat·Health·장비/무기/Reticle·입력/카메라와 Blueprint 부모·soft/hard package 참조·Config·태그/Redirect를 확인한다. 기존 조회 도구를 재사용하며 새 범용 도구를 만들지 않는다. 삭제/대량 치환은 파일별 근거와 범위를 정한 후 별도 승인한다.
- 읽기 전용 사전 근거: DreamCatcherCharacter가 DCCombatComponent를 생성하고, DCEnemyCharacter/구형 Death Ability가 DCHealthComponent를 사용하며, DCGASTestTarget과 DCRifleIntegrationActors가 구형 Equipment를 참조한다. 따라서 구형 이름/폴더만으로 삭제 가능 판정을 내리지 않는다. DefaultEngine의 시작 맵은 여전히 기존 Level_1이고 기본 GameMode도 기존 BP다. R13 수용은 제출 실행 경로의 활성 교체 완료가 아니다.
- 후속 R14-2는 승인된 미사용/대체 완료 항목만 제거하고 기존 맵에 필요한 호환 항목은 참조·이유·전환 조건과 함께 보존한다. R14-3은 변경 영향에 맞춘 회귀와 최종 Editor/패키지 완주 확인이다. 빌드·패키징은 계속 사용자 담당이며 완료된 원본 이식은 반복하지 않는다. 제출 시작/복귀 맵·Cook 범위 및 디자이너 브랜치 실제 병합은 별도 구체적 변경안/승인이 필요하다. R14 전체 구현·삭제·병합은 아직 시작하지 않았다.

---

## 0.100 이력 — 2026-10-14 R14-1 읽기 전용 조사·분류 / R14-2A 제안

- 사용자 승인 범위는 R14-1 참조 조사와 정리 목록 작성이다. **게임 C++/Config/에셋 수정·삭제0**, 빌드/패키징·Computer Use·Git 조작0. R13 사용자 수용과 기존 플레이어·맵을 유지했다. 상세 결과와 정확한 후속 대상은 `docs/handoffs/r14-reference-audit-2026-10-14.md`를 따른다.
- `references_D`: AssetRegistry2,898개, R13 명시적 시작점5개의 package 의존1,291개, 선택 BP26개/부모·그래프·CDO, PawnData3개/보존맵14개 조사. Source/Config/native 참조를 함께 분류했다. 저장0, 주 조사 Error/ensure/Fatal0, 파일 상태 변화0, 보호 SHA642개 최종 불변. package 참조 집합을 실제 실행·Cook 목록으로 보지 않는다. Config Cue 검색도 별도로 조사했다.
- 실제 R13 Pawn은 Hero Legacy 입력/카메라=false, 원본 ADS=true, DefaultWeaponDefinition=None이다. 구형 Combat/Equipment 컴포넌트는 상속으로 남아 있으나 이중 발사/피해를 재현한 것은 아니다. 부모 BP_DC_RiflePawn의 실행선/출력 없는 `K2Node_CallFunction_8` EquipItem과 이에만 연결된 `K2Node_VariableGet_0`이 구형 WID_DC_Rifle package 참조를 남기는 것을 확인했다.
- 전체 BP 타입319개 중318개 조회 성공, 이전 `/Game/LyraMigration/Death/GCNL_Death`1개는 깨진 매크로로 로드 시 Compile 오류2개다. `type_references_A`는 실패/종료1이며 전체 성공으로 표시하지 않는다. 이 Cue와 GA_DC_Death, CM_DC_Hip3개는 별도 registry 조회에서 referencer0/Source·Config·Plugin 텍스트 참조0이다. 활성 사망 Cue는 `/Game/GameplayCueNotifies/DCMigration/Death/GCNL_Death`로 별개다.
- **R14-2A 제안/아직 미승인:** BP_DC_RiflePawn의 위 노드2개만 제거하고, 미사용 GA_DC_Death·CM_DC_Hip·오류가 있는 비활성 GCNL_Death3에셋을 Unreal API로 제거하는 총4에셋 범위다. 삭제 직전 해시/참조/dirty·복구 가능 상태를 확인한다. C++/Config/맵은 이 묶음에 포함하지 않으며 추가 C++ 빌드도 필요 없다. 구형 Death native2파일은 후속 후보이고, 기존 플레이어가 사용하는 장비/Health/HUD/Camera·CoreRedirect/태그는 보존한다.
- 기록 교정: R13 계획상의 중간295경로는 현재 파일/Registry 모두0이다. 이전 ‘중간 redirector 유지’ 기록을 현재 잔존 사실로 사용하지 않는다. 이번에 삭제한 것은 아니다. 프로젝트의 기존 redirector97개와 R13에서 참조되는 재질 redirector1개는 별개이며 보존한다. R14-2 구현·R14 전체·제출 시작/복귀/Cook·디자이너 병합/패키징은 아직 미완료다.

---

## 0.101 이력 — 2026-10-14 R14-2A 에셋 정리·자동 회귀 통과

- 사용자가 승인한 에셋4개를 처리했다. `BP_DC_RiflePawn`의 끊긴 EquipItem/전용 getter 노드2개만 제거/저장했고, 참조 없는 GA_DC_Death·CM_DC_Hip·비활성 `/Game/LyraMigration/Death/GCNL_Death`3개는 Unreal API로 삭제했다. 현재 파일/Registry 모두 제거됐고 기존 플레이어의 실행 그래프·맵·게임 C++/Config는 유지했다. 추가 C++ 빌드는 필요 없다.
- 변경 전 대상4개는 커밋 `7a10c75b0b2af4be2f15f7c104c1bbfe82c4bbe9`의 LFS 포인터/로컬 payload와 현재 파일의 해시 일치를 확인해 복구 가능 상태를 기록했다. 바이너리 직접 복사·Git stage/commit/merge0. 사용자 Editor 종료를 확인한 뒤 작업했다.
- 기본 Editor 삭제 API는 Registry만 제거하고 파일을 남겨 완료 판정을 거부했다. 엔진의 비교용 참조 알고리즘 `Editor.UseLegacyGetReferencersForDeletion=1`을 해당 프로세스에서만 사용하여 실제 삭제하고 종료 전0으로 복원했다. 외부 참조/해시/범위 보호와 엔진·Config 불변을 유지했다. 삭제 대상 broken Cue 로드 시 기존 오류2개는 삭제 실행 이력으로 남기며 새 프로세스 검증과 구분한다.
- **새 프로세스 재로드/Compile7개 성공**, 원본 장착·사망·카메라와 기존 플레이어 보존, 수정 Pawn의 구형 WID 직접 참조 제거. **R13 영향 범위 PIE 성공1/실패0/오류0/경고15**(기존 Manny14＋Recast1). 원본 장비1/구형0, 탄약30→29, 견착→Hip, 원본8초 Death→Pawn 제거/실패 결과/장비 정리 통과. 입력 주입·검사용 피해/면역이며 화면/음향·물리 입력·복제 수용은 아니다.
- 상세 근거 `Saved/Diagnostics/R14/cleanup_summary_A.json` 및 `docs/handoffs/r14-reference-audit-2026-10-14.md`. **현재 다음은 추가 빌드 없이 R13 맵의 짧은 발사·조준·사망 시각 확인**이다. apply/delete/완료한 자동검사를 반복하지 않는다. 구형 Death native2파일·잔존 컴포넌트·Config/redirector/태그·디자이너 병합/최종 패키징 및 R14 전체는 후속 승인 범위다. Codex 빌드/패키징·Computer Use0.

---

## 0.102 이력 — 2026-10-14 R14-2A 사용자 확인 대기 / R14-2B 제안

- 사용자는 R14-2A 이후 발사·조준·사망 연출 PIE를 **아직 확인 전**이라고 답했다. 2A 자동 회귀 성공과 사용자 미확인을 구분하며 수용 완료로 올리지 않는다. 이번 후속 요청에서는 문서/검토용 패치만 작성했고 게임 소스·Config·에셋 수정, Editor/빌드/패키징·Git 작업은0이다.
- 다음 제안 R14-2B는 구형 `Source/DreamCatcher/AbilitySystem/Abilities/DCGameplayAbility_Death.h/.cpp`2파일 제거다. Source/Config/Plugin 외부 참조0, 유일한 구형 자식 에셋은2A에서 삭제됐고 prior BP 타입 조회에서 해당 타입 참조0이었다. 검토용 `Saved/Diagnostics/R14/native_death_proposal_A.patch`만 작성했으며 **실제 삭제는 아직 미승인·미적용**이다.
- 활성 원본 `DCLyraGameplayAbility_Death`/관련 CoreRedirect, 기존 Health/캐릭터/맵/Config/태그는 보존한다. 승인 후 사용자 Development Editor/Win64 빌드1회와 새 DLL에서 원본 클래스·현재 Death Blueprint 로드/Compile 확인이 필요하다. 완료된 이식/검사는 반복하지 않는다. 상세 근거는 R14 전용 인계 상단이다.

---

## 0.103 이력 — 2026-10-14 R14-2A 사용자 수용 / R14-2B 소스 제거·빌드 대기

- 사용자가 R14-2A의 PIE 정상 확인과 R14-2B 제안 진행을 명시했다. **R14-2A는 사용자 수용 완료**다. 기존 에셋 정리·이식/완료한 자동 PIE를 반복하지 않는다.
- 승인된 `Source/DreamCatcher/AbilitySystem/Abilities/DCGameplayAbility_Death.h/.cpp`2개를 제거했다(157줄). 정확한 타입/include/클래스 경로의 Source·Config·Plugin 외부 참조0, 제거 후 잔여 참조0이다. 활성 원본 DCLyraGameplayAbility_Death·Health/캐릭터/맵·에셋/Config/태그는 유지하고 새 대체 구현이나 Redirect를 추가하지 않았다.
- `native_death_removed_A.json`: 다른 보호 파일784개 해시, 에셋 파일2,897개 상태 불변. DLL/Git index/uproject 보존. 문서와 빌드 후 확인용 `r14_native_death_verify.py`를 준비했다. Python AST/공백 검사는 통과했지만 C++/UHT/링크·새 native 등록 검증은 아직 전이다. Codex Editor/빌드/패키징/Computer Use·Git 조작0.
- **다음은 Editor를 닫고 사용자 Development Editor/Win64 빌드1회.** 완료 후 새 DLL에서 구형 native 미등록/원본 등록과 현재 Death BP·Pawn/Boss4개 로드/Compile을 저장 없이 확인한다. `-DCR14NativeDeath=Verify_A` / `R14NativeDeath-Verify_A.log`를 사용하며 삭제 전 DLL 해시가 그대로면 검사를 거부한다. 완료한2A 검사는 반복하지 않는다. R14-2B 검증 및 R14 전체/디자이너 통합·패키징은 미완료다.

---

## 0.104 이력 — 2026-10-14 R14-2B 검증 완료 / R13 패키지 경로 준비

- 사용자 Development Editor/Win64 빌드 **02:50:12 시작·32.46초 성공**, DLL **02:50:44 / 6,625,792 bytes** 갱신을 확인했다. 새 commandlet `native_death_verify_Verify_A.json`은 구형 native 미등록/원본 등록 및 Death2·Pawn·Boss4개 로드/Compile 통과, 종료0/Error·ensure·Fatal0/저장0이다. **R14-2B 승인 범위 검증 완료**, R14-2A 사용자 수용 유지. 기존 PIE/이식/정리는 반복하지 않았다.
- 사용자가 다음 패키지 검증을 **R13 맵으로 우선 준비**하도록 선택했다. DefaultEngine의 GameDefaultMap만 R13으로 바꾸고, DefaultGame의 ProjectPackagingSettings.MapsToCook에 R13 시작/R10 기존 복귀 맵을 추가했다. EditorStartupMap=기존 Level_1, 기존 전역 GameMode/GameInstance·ReturnMap·맵/플레이어/게임 C++·uproject는 유지했다.
- `package_config_Verify_A.json`: 유효 맵/패키징 설정, 두 맵 존재, R13 World GameMode/Experience, 기존 ReturnMap, Experience/ActionSet AlwaysCook 및 DCStageUI enabled 확인. 종료0/Error·ensure·Fatal0/저장0, 에셋 상태 불변, 이번 변경은 정확히 Config2개다. `package_prepare_summary_A.json` 및 R14 전용 인계 상단을 따른다.
- **현재 다음은 사용자 Windows Development 패키징·실행 확인**이다. 추가 Editor C++ 빌드는 필요 없다. 기본 실행 R13, HUD/메뉴·완주/실패·재시작·R10 복귀를 패키지에서 확인한다. Codex Cook/빌드/패키징 실행0이며 설정 조회를 패키징 성공으로 표현하지 않는다. 디자이너 통합·구형 호환 계층 후속 전환·최종 제출 및 R14 전체 완료는 남아 있다. Computer Use·Git stage/commit/merge0.

---

## 0.105 이력 — 2026-10-14 패키징 보류 / R14 후 main 병합 / UI 해상도 확인 대기

- 사용자가 **패키징 과정은 건너뛰고, R14 종료 후 디자이너 main과 병합**하도록 순서를 정했다. 지금 패키징을 다시 요구하지 않는다. 패키징은 보류·미검증으로 분리하며 성공/최종 제출 완료로 표현하지 않는다. 실제 병합은 아직 하지 않았고 방향·원격 최신/디자이너 미커밋 상태는 병합 단계에서 확인한다.
- 2A 에셋 정리/사용자 PIE 수용과2B native 제거/사용자 빌드·연결 검증은 완료 상태로 유지한다. 기존 플레이어·맵에 필요한 구형 호환 계층은 보존한다. R13의 입력·장비/발사·사망 활성 원본 경로 검증과 잔존 비활성 컴포넌트의 존재를 구분한다. 전역 구형 코드 일괄 삭제 완료를 뜻하지 않는다.
- 패키징을 제외한 최종 점검으로 `/Game` Blueprint 계열 **316개 SkipSave Compile 성공/실패0, Error·ensure·Fatal0, 종료0**. 경고29(Manny14＋Quinn14＋B_LyraGameInstance 이전 script package1)는 남는다. `final_blueprints_A.json`/`R14FinalBlueprints-A.log`, 종합 `premerge_closeout_A.json`. 에셋/소스·Config/DLL·Git index 불변, 에셋 저장/빌드/Cook/패키징/PIE/병합0. 외부 플러그인·맵 내부 LevelScript 전체 검증으로 확대하지 않는다.
- **현재 남은 R14 항목은 창 크기 변경 시 UI 배치의 사용자 확인**이다. 사용자가 아직 확인 전이라고 답했다. HUD·보스 체력/페이즈·Esc 메뉴·결과 버튼의 잘림/겹침을 확인한 뒤 현재 브랜치 R14를 마감하고 main 병합 단계로 넘어간다. 기존 발사/조준/사망/완주 확인은 반복하지 않는다. R13 GameDefaultMap/MapsToCook는 준비 상태를 유지하며 임의 원복하지 않는다. 전체 R14 완료·패키징 검증·디자이너 통합은 현재 완료 표시하지 않는다.

---

## 0.106 이력 — 2026-10-14 R14 완료 / main 병합 준비

- 사용자가 창 크기 변경 시 UI 정상 동작을 확인하고 R14 완료 정리를 요청했다. **패키징을 제외한 현재 GAS-System의 R14 정리·검증·사용자 수용 완료**다. 2A/2B 검증과 최종 Blueprint316개 Compile 통과, 기존 경고29 및 필요한 기존 맵 호환 계층 보존을 그대로 기록한다. 모든 해상도/장치·복제/패키징까지 검증 완료로 확대하지 않는다. 패키징은 계속 보류·미검증이다.
- 사용자는 디자이너의 병합 대상 작업이 main에 모두 커밋·푸시됐다고 확인했다. GitHub 양방향 compare와 로컬 Git에서 **main=ebb169c(10/13 22:26 KST), GAS-System=7a10c75(10/13 03:39), 공통조상50ef6ac**, main전용23/GAS전용18커밋을 확인했다. 원격 파일 응답300개 제한은 전체 분석 근거로 쓰지 않고 로컬 객체로 전 경로를 비교했다.
- 준비 스냅샷: main변경964경로(추가891/수정60/삭제13), GAS저장변경2,211경로, 미커밋512파일(추적28/신규484, staged0). 후속 문서 추가 뒤 실행 전 목록을 갱신한다. 미커밋 R13/R14를 먼저 checkpoint에 보존해야 한다. 새 pycache1개는 커밋 제외 후보이며 삭제하지 않았다. 프로젝트/관련 플러그인 모듈 BuildId55116800 일치. Git stage/commit/checkout/merge/push0.
- **동일 변경 경로·대소문자/파일-폴더 충돌0이어도 에셋 호환 문제는 있다.** main은 Box_1 자산13개를 Prop/Mesh/Box_1로 재배치했고 R13 의존에는 옛경로5개가 남는다. 권장안은 디자이너의 새 경로를 반영하면서 옛 Box_1 묶음13개를 GAS checkpoint 기준으로 통합 브랜치에 보존하는 것이다. 실제 적용/참조 교체는 아직 하지 않았다.
- main의 추가/수정 LFS payload951개 약635MB가 로컬에 없으며 실제 통합 단계에서 고정 커밋의 데이터를 가져와야 한다. 이번에는 다운로드하지 않았다. 다음 제안은 **checkpoint 커밋→codex/integrate-main-20261014 통합 브랜치→main/LFS 반영＋옛 Box_1 보존→영향 검증**이다. 실제 병합 범위 승인은 아직 전이며 main 직접 커밋/push, 디자이너 원본 맵 gameplay 재조립, 패키징은 별도다. 상세는 `docs/handoffs/main-merge-preparation-2026-10-14.md`.

---

## 0.107 이력 — 2026-10-14 통합 PR #1 / 리뷰 수정 자동검증 완료

- 사용자가 checkpoint 커밋과 main 병합·Box_1 옛13개 보존·통합 브랜치 publish를 직접 수행했다. 현재 브랜치는 **`integrate-main-20261014`**, 원격 PR은 **https://github.com/Shiny-Shine/DreamCatcher/pull/1**, base=`main`이다. main=`ebb169c`, 통합 HEAD=`5dfb0e6`까지 확인했다. main 최종 반영은 아직 하지 않았다.
- 통합 검사: 디자이너 LFS951개 해시, 보존 Box_1 옛13개, 변경 Blueprint8개 Compile/Box26개 로드, 맵9개 로드 및 R13 실제 PIE 발사·조준·사망/정리 검사를 완료했다. PIE는 실패0/기존 경고15(Manny PoseAsset14＋RecastNavMesh 초기화1)다. 완료한 검사를 처음부터 반복하지 않는다.
- 사용자가 폐기 배치라고 확인한 `Building_Samples.LevelInstance_1`→`Instances/NewMap1`, `Level_1.LevelInstance_0`→`RoadLine_IS`만 Unreal 내부 API로 제거했다. 두 맵 새 프로세스 재로드·나머지 액터 배치/부착·유효 `BPP_NewMap` 보존 및 누락 참조0을 확인했고, 사용자가 **`5dfb0e6 fix: remove obsolete level instances`로 커밋·푸시**했다. 근거 `Saved/Diagnostics/MergeValidation/integration_summary_A.json`.
- PR 자동 리뷰의 P2 두 건을 실제 코드/로컬 Lyra 원본과 대조하고 사용자에게 차이를 설명한 뒤 **수정 승인을 받았다**. 원본에도 같은 조건문이 있다. `DCHeroComponent.cpp`는 설정 등록만 조건부로 두고 모든 로드된 기본 입력 매핑을 활성화한다. `DCLyraGameplayAbility_RangedWeapon.cpp`는 각 탄환 시작 시 결과 개수를 기록하여 해당 탄환이 결과를 추가하지 못했을 때만 끝점 결과를 보충한다. 기존 명중·퍼짐·우선순위·무기 수치·에셋/맵/Config는 유지한다.
- 새 `Tests/DCMergeReviewTestTypes.h`와 `DCMergeReviewAutomationTests.cpp`는 임시 Hero/무기 fixture와 검사2건을 제공한다. `DreamCatcher.IntegrationReview.InputMappingRegistration`: 등록 on/off, 기존 컨텍스트 초기화, 우선순위, 설정 객체 없는 경우. `DreamCatcher.IntegrationReview.PerBulletMissResults`: 단발 명중/빗나감, 다발 전부 명중/빗나감, 명중·빗나감 혼합 시 실제 충돌 결과와 끝점 보존. 별도 프로세스의 `-DCMergeReviewIsolatedAutomation`이 필요하며 사용자 저장 설정/에셋을 변경하지 않는다.
- **후속 빌드/검사:** 검사 CPP의 `ON_SCOPE_EXIT` 세미콜론 누락 교정 후 사용자 Development Editor/Win64 빌드가 **16:43:51 시작·9.36초 성공, DLL16:44:00/6,636,544bytes**로 확인됐다. **16:45 native 검사 결과는 성공1/실패1**이다. 탄환6조합은 성공/경고0이며, 입력 검사는 `ULocalPlayer`의 Within=Engine 규칙을 어긴 테스트 객체 생성 때문에 ensure가 발생했다. 실패를 허용하지 않고 테스트 CPP에 `Engine/Engine.h`와 Outer=`GEngine` 교정을 적용했다. 게임 코드2파일은 추가 수정하지 않았다.
- **R13 실제 PIE 영향 회귀는16:47 성공**했다. 발사/탄약30→29, 견착/Hip 복귀, 사망/객체·장비 정리, ensure/Error0을 확인했다. 보고서 경고16(Manny14＋RecastNavMesh1＋긴 프레임의 이동 시뮬레이션 제한1)은 별도로 기록한다. 에셋/Config/사용자 저장 설정/DLL/Git index는 검사 중 유지됐으며, 의도된 변화는 테스트 CPP의 Outer 교정뿐이다. 근거 `Saved/Diagnostics/MergeValidation/pr1_review_checks_A.json`.
- **16:50~16:51 후속 검증 완료:** 사용자 증분 빌드 **16:50:02 시작·8.97초 성공**, DLL16:50:10/6,636,544bytes, SHA256 `d5b30577e3aa9c5fc585a8a30abf96b87f979824fdff2ea835485a39f4aea099`. `MergeReviewInput_B`의 **입력 검사1건 성공/실패·보고서 경고0, Error·ensure·Fatal0, 종료0**을 확인했다. 시작 시 기존 B_LyraGameInstance script package 경고1은 별개다. 탄환 검사/PIE는 코드 불변 근거로 기존 성공 결과를 재사용했고 재실행하지 않았다.
- **현재 다음은 사용자 짧은 화면/입력 확인 → 같은 `integrate-main-20261014`에 코드·검사·문서·필요 DLL을 커밋/푸시하여 기존 PR #1 갱신**이다. 이번 리뷰2건 수정의 자동검증은 완료했다. 원격 PR은 아직 수정 전5dfb0e6이며 main 병합은 사용자가 진행한다. 통합 맵 외형 확인·물리 입력/조작감 수용 및 패키징까지 자동검증 완료로 확대하지 않는다. Codex 빌드·패키징·Git stage/commit/push/merge·PR 댓글/리뷰 Resolve는 하지 않았다. 패키징은 계속 보류하며 디자이너 `Level_1`의 R13 gameplay 재조립은 별도 범위다. 상세는 `docs/handoffs/main-merge-preparation-2026-10-14.md` 상단을 따른다.

---

## 0.108 이력 — 2026-10-14 main 병합 완료 / Level_1 전환·레거시 정리 조사

- 사용자가 리뷰 수정본 PIE 정상 동작을 확인하고 `bc453e6` 커밋/푸시, PR #1 병합까지 완료했다. 현재 `main`/`origin/main`은 **`3665f3ba690bab286784da7aae183b7485932cd0`**이며 병합 시각은10월14일18:13 KST다. GAS-System의 모든 커밋도 main에 포함됐음을 확인했다. 앞선 ‘PR 갱신/병합 대기’는 이력이다. 검증된 코드·DLL·기존 플레이 확인을 다시 수행하지 않는다. 패키징은 계속 사용자 지시로 보류한다.
- **새 사용자 방향:** 디자이너 `DreamCatcher_ENV/Map/Main/Level_1`의 환경 배치를 유지하고 **그 맵의 기존 플레이 흐름은 폐기**, 검증된 R13 플레이 흐름으로 교체한다. 테스트 맵을 정식 맵으로 삼지 않는다. 현재 요청은 프로젝트를 살펴 정리할 코드·에셋·Blueprint·테스트/레거시 후보를 찾는 조사이며 실제 대량 삭제·이관·Config/게임 코드 변경은 아직 하지 않았다.
- 읽기 전용 조사: package3,786개 참조, BP36개(22개는 기존 해시 일치 근거 재사용/14개 새 조회), PawnData3개, 맵3개와 활성 테스트 부여/숨겨진 LevelScript·NavMesh 확인. 저장/Compile/PIE/빌드/패키징0, Error·ensure0, 에셋3,788개 및 보호 소스·Config·DLL·Git index 불변. 열린 사용자 Editor의 미저장 상태는 조사 대상이 아니다.
- Level_1은 구형 BP GameMode/플레이어·HUD와 적1개, NavMeshBoundsVolume0이다. R13은 Experience/StageUI GameMode, 일반 Lyra 스포너2개＋보스1개, Encounter3개＋StageDirector1개, NavMesh1개다. **환경 맵 전체를 덮어쓰지 않고 게임플레이 연결과 배치만 교체**하는 변경안이 다음이다.
- `GAS/Test`83에셋(맵8개) 중 맵을 제외한 R13 게임플레이 package 참조에38개가 남는다. 그중 실사용31개는 정식 경로로 이관할 후보이며, R2 Tag 테스트3개·SignatureSmoke3개·빈 R4 AbilitySet1개는 PawnData 부여 해제/제거 후보다. SignatureSmoke는 Q 입력의 TEST 출력/검증용 쿨다운만 제공하므로 제거 시 그 반응이 없어지는 차이를 기록한다. 정식 시그니처 효과를 임의 제작하지 않는다.
- 그 밖의 후보: 구형 전투/장비/체력/HUD·카메라/적14계열28파일, 테스트 표적4파일, native Tests37파일, Variant 템플릿76파일, 없는 Start_Contents를 가리키는 redirector63개, 이식 스크립트87개/pycache4개, 백업13파일, 마지막에 정리할 Editor 이식 플러그인26파일, 테스트 맵 폐기 후 옛 Box_1 13개다. 서로 겹칠 수 있으며 자동 삭제 승인 목록이 아니다.
- 현재 Controller는 `Development/DCRifleIntegrationActors`→`DCLyraAIIntegrationController`를 사용한다. Character/PlayerController 기반 전체, Boss가 상속하는 DCGASEnemyCharacter, StageDirector/Encounter/Spawner의 Lyra·GAS 경로와 원본 런타임 플러그인은 보존한다. Test/Migration/Diagnostics 이름만으로 삭제하지 않는다. 기존 Source 테스트는 핵심 회귀 보존과 과거 이식 검사 정리를 구분한다.
- **남은 결정/다음 변경안:** Level_1 게임플레이 전환 및 실제 전투 배치/NavMesh, 정식 자산 경로와 DefaultPawnData/Experience/Cue/Cook 설정, R10 테스트 맵 대신의 `돌아가기` 목적지를 먼저 구체화한다. 정식 복귀 맵은 아직 확인되지 않았으며 버튼을 없애거나 null로 두어 기능을 축소하지 않는다. 이후 활성 테스트 부여 해제→레거시/과거 테스트 맵·자산/코드 제거→개발 잔재 정리 순서다. 상세 목록/근거/한계는 `docs/handoffs/r14-reference-audit-2026-10-14.md` 최신 조사와 `Saved/Diagnostics/Level1Cleanup/`를 따른다.

---

## 0.109 최신 재개 — 2026-10-14 Level_1 R13 흐름·NavMesh 연결 / 사용자 PIE 확인 대기

- 사용자가 **Level_1의 환경 배치를 유지하며 구형 플레이 흐름을 R13으로 교체하고 NavMesh를 연결**하는 첫 묶음을 승인했다. `돌아가기`는 당분간 같은 맵을 다시 열고, 추후 메인화면을 만들면 그곳으로 연결하도록 명시했다. 과거 테스트 맵/에셋의 일괄 삭제나 실사용31개 경로 이관까지 이번에 확대한 것은 아니다.
- 기존 `Level_1.umap`의 구형 적1개와 Engine PlayerStart1개를 교체하고, Lyra PlayerStart1개, StageDirector1개, Encounter3개, 일반 Lyra 스포너2개＋보스 스포너1개, NavMeshBoundsVolume/Recast를 연결했다. **기존 환경 액터4,642개의 클래스·변환값은 새 프로세스 재로드까지 불변**이며 R13 원본/테스트 에셋도 유지됐다. 일반 스폰은(335,165,81)/(635,165,81), 보스는(1035,165,73)cm로 현재 바닥·캡슐 여유 공간을 확인했다. 시작 구역 배치이며 전체 레벨의 최종 전투 배치 디자인을 완료했다는 의미는 아니다.
- R13 GameMode를 Unreal API로 복제한 **`/Game/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode`**를 만들고 ReturnMap만 Level_1으로 override했다. Experience/Controller/HUD·팀/결과 설정은 원본과 동일하다. Level_1 WorldSettings가 새 BP를 사용한다. `DefaultEngine.ini`의 GameDefaultMap과 `DefaultGame.ini`의 명시적 MapsToCook도 Level_1로 변경했다. EditorStartupMap은 기존 Level_1 유지, 다른 맵에 쓰일 수 있는 전역 GameMode/fallback PawnData는 이번에 바꾸지 않았다.
- NavMesh는 충돌 환경 전체에500cm 여유를 둔 범위, **Dynamic**으로 설정했다. 새 프로세스 재로드에서 연결을 확인했고 **19:49 최종 `Level1Flow_D` 실제 PIE 성공1/실패0/Error·ensure0**을 확인했다. 실제 AIController가 부분 경로를 허용하지 않고 세 전투 지점까지 이동(구간 변위 합992.85cm), 발사30→29/견착→Hip, 일반전투2→보스·HUD→클리어/정리, 결과 돌아가기와 메뉴 확인창 돌아가기로 같은 Level_1 새 실행 및 이동 입력 복구를 확인했다. 진단 입력·피해·일시적 AI 이동 명령이며 사용자 물리 입력/시각 수용과 구분한다. 보고서 경고17은 Manny PoseAsset14＋월드 초기화 Recast/Crowd3이다.
- 앞선 PIE A/B는 검사 스크립트의 Navigation 정적 호출이 CDO World ensure를 발생시켰고 A는 private UI 속성 조회도 실패했다. C는 설정 SoftObjectPath의 단순 str 표현 검사에 실패했다. 이 결과는 성공으로 보지 않으며, 실제 AIController 이동과 기존 encode/export 방식으로 교정한 D만 최종 성공 근거다. 게임 자산/C++를 이 검사 교정 때문에 다시 변경하지 않았다.
- 실제 게임 변경은 **맵1개＋새 GameMode BP1개＋Config2개**다. 일회성 `Scripts/Editor/level1_r13_flow_setup.py`/`level1_r13_flow_probe.py`와 문서를 추가/갱신했다. C++/DLL 변경·빌드·패키징·Git stage/commit/push0. 근거 `Saved/Diagnostics/Level1Integration/{setup_Apply_A,setup_Verify_A,pie_D,completion_A}.json`, `Saved/Automation/Level1Flow_D/index.json`.
- **다음은 추가 빌드 없이 사용자의 Level_1 PIE 짧은 확인**: 환경/시작 위치, 실제 AI 이동·전투→보스·HUD, 결과/메뉴 돌아가기의 같은 맵 재시작. 이후 새 작업 브랜치에서 사용자 커밋/푸시한다(main 직접 커밋하지 않음). 기존 테스트 맵과 실사용 테스트 경로·레거시 native 정리는 그 다음 묶음이다. 추후 메인화면이 생기면 새 GameMode의 ReturnMap 및 Cook 대상을 갱신한다. 상세는 `docs/handoffs/r14-reference-audit-2026-10-14.md` 최상단이다.

---

## 0.110 최신 재개 — 2026-10-14 Level_1 정식 경로·레거시 정리 적용 / C++ 빌드 대기

- 사용자가 Level_1 PIE 정상 동작을 확인하고 실사용 경로 이관과 테스트/레거시 정리를 승인했다. 환경4642개, 일반전투2→보스→완료, 자동 리스폰 없음, 돌아가기→같은 맵 재시작을 유지한다. 이전0.109의 사용자 수용 대기는 해소됐다.
- 실사용782개를 DreamCatcher의 Blueprints/Data/GAS/Input/Experiences/FX/Audio/Lyra와 개발 전용 `/Game/Tests/Equipment`로 이관했다. Config의 PawnData/Experience/Cue/UI/장치 경로와 DCStageUI GameFeature 참조를 갱신했다. basename/Experience ID는 유지한다. 전역 구형 GameMode fallback은 Engine.GameModeBase이며 Level_1은 명시적 BP_DC_Level1GameMode를 유지한다.
- 테스트 맵9개를 포함한 구형/테스트 에셋98개와 잔존 사본·redirector178개 =276패키지를 Unreal API로 삭제했다. 실사용 Pawn의 옛 전투 노드47개와 구형 EquipmentManager1개를 제거했다. SignatureSmoke 반응과 빈/검사용 부여를 해제하되 실제 시그니처 기반과 QuickBar·Lyra 장비·메시·AnimClass는 보존했다. 삭제 직전 확인된 실사용 라이플 MetaSound1개는 정식 경로로 보존했다.
- 중간 이동/삭제 guard 실패와 PIE Cleanup_A/B 실패는 성공이 아니다. 새 프로세스 재로드·핵심 그래프/메시 보존·직접 영향 검사, 플러그인 참조 보완, NavMesh 재생성 후 **21:41 Level1Flow_Cleanup_C 성공1(경고 포함)/실패0/Error·ensure0**이다. 실제 AI3지점 도달(1289.85cm), 발사/견착/Hip, 일반전투2→보스/HUD→클리어/정리, 결과·메뉴의 같은 맵 재시작을 통과했다. 기존 경고17(Manny14+Recast/Crowd3)은 유지한다.
- Nav 저장 전용 프로세스에서만 자동 비동기 대기를 껐고 영구 Config·볼륨·AgentRadius35/AgentHeight144/Dynamic은 유지했다. 최종 PIE는 그 옵션 없이 일반 설정으로 검증했다. 환경4642개와 사용자 저장 파일을 보존했다.
- **위 PIE 이후** 구형 native36파일과 템플릿76파일을 제거했다. Character/Controller 자체와 ASC·Health·사망·ADS·입력·카메라 및 ACharacter native Tick을 보존하고 옛 분기만 제거한다. 실사용 Loadout/Team Controller는 Player 폴더로 옮기되 serialized 클래스명/API를 유지한다. 스포너의 구형 EnemyClass만 제거했고 GAS/Lyra 및 처치·정리·완료 처리는 보존한다. 삭제 타입/이전 경로 참조 및 diff 정적 검사는 통과했지만 **새 UHT/컴파일/링크와 새 DLL 기준 Blueprint·PIE는 미검증**이다.
- C++ 회귀37파일과 장비 fixture4개(NeverCook)는 보존하며 관련 테스트의 경로/legacy 부분만 조정했다. DCRifleMigrationTools 플러그인/등록, 과거 Editor 스크립트·계약검사·pycache 및 백업13파일을 정리했다(도구 묶음 추적130파일). 레시피97파일은 `Saved/Diagnostics/Level1Cleanup/retired_editor_recipes.zip`으로 해시 검증 보존했다. 현재 재현은 플러그인 의존 없는 `Scripts/Editor/level1_gameplay_probe.py`를 사용한다.
- **다음은 사용자 Development Editor / Win64 빌드**다. 새 DLL 확인 후 직접 영향 BP/핵심 회귀와 짧은 Level_1 PIE를 마무리한다. 완료한 R0~R14 전체 이식 검사는 반복하지 않는다. 게임 DLL/인덱스 불변이며 C++ 빌드·패키징·stage/commit/branch/merge/push는 실행하지 않았다. 패키징은 계속 보류한다. 상세/정확한 목록/복구 근거는 `docs/handoffs/level1-production-cleanup-2026-10-14.md` 및 연결된 JSON 목록을 따른다.

---

## 0.111 최신 재개 — 2026-10-14 새 DLL 기준 정리 후 검증 통과 / 사용자 최종 수용 대기

- 사용자 Editor/Win64 빌드가21:57 성공(8.75초)했고 include 누락 오류는 해결됐다. DLL5,724,672 bytes, SHA256 `f863512bb8eb923a450e349f692f7e4eb21b618290ec9a1b3b8505d6710bffaf`를 기준으로 검증했다. 0.110의 새 C++ 빌드/검증 대기는 해소됐다.
- 직접 영향 BP11개 Compile 성공, 검사한 삭제 native19종 등록0, Game/DCStageUI의 폐기 package 참조0, 실제 Pawn의 구형 컴포넌트0, 환경4642개 보존. 입력/탄환·장비·GAS 적·유한 Lyra 스포너·스테이지·보스 소유권/메뉴의 기존 회귀22건 성공/실패0이다. 빈 optional 스포너의 의도된 즉시 완료 경고1건은 기록한다.
- BP_DC_RiflePawn/BP_DC_LyraInventoryPawn 저장본에 남은 구형 CombatComponent export의 로드 경고2건을 확인해 **이 두 BP만** 새 DLL로 Compile/저장했다. 그래프/메시/AnimClass/캡슐/이동/입력 설정 보존과 별도 재로드 경고0을 확인했다. C++/Config/uproject/DLL/맵/사용자 SaveGames 및 Git 인덱스는 변경하지 않았다.
- **22:23 Level1Flow_PostBuild_B 실제 PIE 성공1(경고 포함)/실패0/Error·ensure0**. 발사, 견착/Hip·스코프/Hip 입력 전환, 웅크리기/해제 API, 실제 AI3지점 이동1521.18cm, 일반전투2→보스/HUD→완료/정리, 결과·메뉴의 같은 맵 복귀, 플레이어 치명 피해→실패 결과→Pawn/장비/AI 정리→자동 리스폰 없이 대기→명시적 재시작과 체력/탄약/입력 복구까지 확인했다. 물리 입력·화면 수용과 구분한다.
- PostBuild_A는 사망 검사에서 detach 직후 파괴를 요구하여 원본0.1초 lifespan 유예 전에 실패했다. 검사만 기다리도록 교정했으며 게임 코드 변경/실패 은폐는 없다. B를 최종 근거로 사용한다. 기존 PIE 경고18(Manny14+월드 초기화 Crowd/Recast4), LyraGame import 경고와 UBT DataRegistry 명시적 의존성 경고는 별도 기록한다.
- **추가 빌드는 필요 없다. 다음은 사용자의 최종 Level_1 화면/조작감 확인**이다. 정상 수용 후 사용자가 작업 브랜치에서 검토·커밋한다. 완료한 이식/전체 맵 검사를 반복하지 않고 패키징은 계속 보류한다. Codex는 빌드·패키징·stage/commit/branch/merge/push를 실행하지 않았다.
- 최신 상세와 다음 단계는 `docs/handoffs/level1-production-cleanup-2026-10-14.md`. 근거: `Saved/Diagnostics/Level1Cleanup/{user_build_A.txt,postbuild_Assets_A.json,pawn_resave_recovery_A.json,pawn_resave_Save_A.json,pawn_resave_Verify_A.json,postbuild_closeout_A.json}`, `Saved/Automation/{Level1PostBuildCore_A,Level1Flow_PostBuild_B}/index.json`. 재현 스크립트는 `Scripts/Editor/level1_gameplay_probe.py`다.

---

## 0.112 최신 재개 — 2026-10-14 Level_1 정식 전환·레거시 정리 사용자 수용 완료

- 사용자가 새 DLL 기준 자동검증 안내 후 **“정상 작동을 확인했어”**라고 최종 확인했다. 현재 Level_1 전환·실사용 정식 경로 이관·승인된 테스트/레거시 정리 묶음의 구현, 사용자 빌드, 자동검증 및 사용자 수용을 완료로 기록한다. 0.111의 사용자 최종 수용 대기는 해소됐다.
- 완료 근거는0.110의 이관/정리,0.111의 새 DLL/BP11개·핵심 회귀22건·Level1Flow_PostBuild_B 통과와 이번 사용자 확인이다. 기존 경고 및 패키징 보류·미검증 상태는 유지하며 전체 플랫폼/장치 검증으로 확대하지 않는다.
- 이번에는 문서와 수용 기록만 갱신했다. 완료한 검증/이식 재실행, C++/Config/에셋 추가 변경, 빌드·패키징, Git stage/commit/branch/merge/push는 하지 않았다.
- 다음은 사용자가 GitHub Desktop에서 작업 브랜치의 변경을 검토·커밋/푸시하는 단계다. 후속 기능은 새 변경 범위를 먼저 정한다. 상세는 `docs/handoffs/level1-production-cleanup-2026-10-14.md`, 사용자 수용 근거는 `Saved/Diagnostics/Level1Cleanup/user_acceptance_A.json`이다.

---

## 0.113 최신 재개 — 2026-10-14 잔여 플레이어 애니메이션·입력·블록아웃 정리 완료

- 사용자가 남은 레거시 여부를 확인하고 추가 정리를 승인했다. 0.112는 지정한 정리 묶음의 완료이며 프로젝트 전체 미사용 에셋0 판정이 아님을 명확히 한다.
- 최신 AssetRegistry/코드·Config 참조를 읽어 옛 플레이어 AnimBP/레이어/롤5개, 옛 입력 매핑/미사용 Action/템플릿 Touch9개, Test_Map과 전용/고립 generated 메시9개 =23개를 정확한 목록으로 삭제했다. Unreal API만 사용했고 사전 외부 참조0/대상 경로 문자열 참조0 및 Git LFS 복구 payload23개를 확인했다.
- Test_Map은2월26일 Build Test Map 1-1 커밋의 액터25개 블록아웃이며 GameMode override/외부 참조/실행 연결이 없었다. 디자이너 Asset.umap 및 연결 Box_34DB3452, 실사용 Input Action5개와 CoreRedirect 호환 Struct_UIMessaging, 보스/Level_1/Level_2가 쓰는 Mannequins 자료는 보존했다. 이름/폴더만으로 공용 라이브러리를 삭제하지 않았다.
- **22:49 새 프로세스 검증 성공**, 삭제23개 외 에셋 변화0, 끊어진 참조0/Error·ensure0, Source/Config/프로젝트/DLL/사용자 저장/Git 인덱스 변경0이다. 실사용 Pawn2개의 SKM_Manny/ABP_Mannequin_Base와 현재 입력 로드를 확인했다. Level_1 파일도 그대로이므로 기존 수용/검증을 유지하며 빌드·PIE 반복은 필요 없다.
- 이번 묶음 완료 후 사용자가 작업 브랜치에서 검토·커밋/푸시한다. 패키징은 계속 보류·미검증이며 Codex는 빌드·패키징·Git 조작을 실행하지 않았다. 전체 프로젝트의 모든 미사용 원본/제작 자료를 제거한 것으로 확대하지 않는다.
- 목록: `docs/handoffs/level1-residual-cleanup-2026-10-14.json`. 상세: `docs/handoffs/level1-production-cleanup-2026-10-14.md`. 근거: `Saved/Diagnostics/ResidualLegacy/{before_A.json,inspect_A.json,manifest_A.json,cleanup_Delete_A.json,cleanup_Verify_A.json,closeout_A.json}`.

---

## 0.114 최신 재개 — 2026-10-15 복구 main 병합·디자이너 수용 완료 및 마무리 작업 인계

- 사용자 확인: `codex/restore-level1-assets-20261014`를 커밋·푸시하고 PR #2로 main에 병합했으며 디자이너 PC에서도 복구 성공. 확인한 로컬/원격 main은 **`13aa3b0f9b2c312dc303544625166d575cffb8f0`**(10월14일23:36 KST), 복구 커밋은`349048a`다. 이번 문서 작업 시작 시 작업 폴더 clean이었다.
- `7f53325`에서 Level_1과 MI_Concrete_3의 실제 LFS payload가 Stash 충돌 문구를 포함한280/274bytes 텍스트로 바뀌었다. 승인된 두 파일만 정상본으로 복원하고 신규 디자인10개와 앞선 추가분을 유지했다. 맵/재질/신규10개 Unreal 로드, 보호479개 불변, Error/ensure0 확인 후 위 병합·디자이너 수용을 마쳤다. 이 복구를 다시 수행하지 않는다.
- R14 및 현재 PC PIE 수용, Level_1 환경 보존/R13 흐름·Dynamic NavMesh, 정식 경로782개 이관, 승인된 테스트/레거시 및 잔여23개 정리를 완료 상태로 유지한다. 기존 플레이어·AI 이동 애니메이션·보스·HUD·ESC 메뉴·화면 크기 대응은 사용자 확인 완료다. 전체 공용 라이브러리 미사용0 또는 패키징/전체장치/온라인 검증 완료를 뜻하지 않는다.
- 현재 실제 플레이는 **Level_1의 일반 인카운터2→보스→결과**, 자동 리스폰 없음이다. 결과/ESC의 돌아가기는 같은 Level_1 재시작이며, 메인화면 추가 후 새 목적지로 연결한다. 플레이어와 일반 봇의 PawnData/Pawn 공유 및 보스 전용 AnimBP 미지정 상태도 새 인계에 기록했다.
- **다음은 메인화면·아웃게임 씬 추가와 UI 마무리**다. 이 요청은 현재 상태/새 채팅 인계 문서 작성이며 새 코드·Config·에셋 구현의 포괄 승인이 아니다. 원본 Lyra/CommonUI 재사용을 조사하고 화면 구성·연관 변경 범위를 먼저 설명한 뒤 승인된 묶음으로 진행한다. 빌드·패키징·화면 확인과 GitHub Desktop 커밋/푸시/병합은 사용자 담당이다. 패키징은 기존 지시대로 보류한다.
- 디자이너의 별도 Stash 원본2개는 당시 원격404/로컬 부재였으며 확보를 확인하지 못했다. 이번 복구 성공을 그 별도 원본 회수까지 완료했다는 뜻으로 확대하지 않는다. 충돌 기록/객체ID는 복구 인계에 보존했으며 기존 Stash/백업을 건드리지 않는다. 새 단서 없이 마무리 작업을 막거나 조사부터 반복하지 않는다.
- **새 채팅의 기준 문서:** `docs/handoffs/project-finish-resume-2026-10-15.md`. 모델/UI 경로는`docs/handoffs/designer-asset-integration-2026-10-14.md`, 복구 이력은`docs/handoffs/level1-stash-conflict-recovery-2026-10-14.md`다. 아래 오래된 R5 등의 대기 기록보다 이 최신 상태를 우선한다. 이번에는 문서만 갱신했고 빌드·PIE·에셋 변경·Git 조작을 하지 않았다.

---

## 0.115 최신 재개 — 2026-10-15 승인된 2D 메인메뉴 코드 준비 / 사용자 빌드 대기

- 사용자가 첨부 UI 초안처럼 **2D 카드·버튼 방식**을 지정하고 3D 배경/이동 로비를 제외했다. 이어 **프리 모드는 우선 건너뛰고 진행**하도록 승인했다. 첫 범위는 스토리 카드·설정·종료와 `메뉴→Level_1→결과/ESC→메뉴`다. 별도 타이틀 미정으로 모드 선택을 실행 첫 화면으로 사용하는 가정을 안내했다.
- 로컬 main은 `819ac19`이며 작업 시작 전부터 `Level_1.umap`에 미커밋 변경이 있었다. 기존 맵/전투/AI/보스/디자인 에셋을 수정하거나 이전 이식·복구 검증을 반복하지 않았다. Git 작업도 하지 않았다.
- 원본 Frontend 에셋6개를 Unreal 내부 API로 읽어 실제 그래프/설정/의존성을 확인했다(저장/명시적Compile0, 대상6파일 해시 불변). 원본 선택 화면의 온라인 로그인/세션 경로를 확인했으며, 기존 CommonUI·버튼·설정/확인창을 재사용하는 DreamCatcher 카드 화면을 작성한다. 원본 이식 불가 판정이 아니다.
- 원본 FrontendState/메뉴 성능 Action4파일과 별도 메뉴 GameMode·화면·카드6파일, 검사2파일을 추가했다. ControlFlows 플러그인/모듈을 연결하고 기존 결과 복귀 버튼 문구를 `메인화면으로`로 바꿨다. 메뉴는 전투 PawnData/PlayerState 생성 경로를 사용하지 않으며, 스토리 시작은 다음tick 이동·중복 차단·이동 실패 재시도를 처리한다. **UHT/C++ 빌드와 신규 검사2건 실행 전**이다.
- `Scripts/Editor/frontend_menu_setup.py`는 신규7개 생성→새 프로세스 재로드→기존 Level1 GameMode.ReturnMap 및 시작 맵/Cook/로딩 표시 연결을 분리한다. 현재는 **스크립트만 준비**했고 실제 에셋/DefaultEngine.ini/DefaultGame.ini는 변경하지 않았다. 엔진 기본 UMGToolSet의 저장 없는 임시 위젯 생성/Compile API 확인만 통과했다. 메뉴·PIE·화면 완료가 아니다.
- UI는 편집 가능한 UMG 트리로 만들며 카드 Artwork는 미지정 상태로 시작한다. 초안의 전체 이미지를 자르거나 최종 아트로 이식한 것은 아니다. 다음은 사용자 Development Editor/Win64 빌드→새 DLL 검사·에셋7개 생성/재로드/연결→변경 영향 왕복 확인이다. 빌드·패키징·화면 확인과 GitHub Desktop 작업은 사용자 담당이다.
- 상세 경로·원본 대비 차이·미완료 항목은 `docs/handoffs/frontend-menu-2026-10-15.md`를 따른다. 0.114의 R14/Level_1/복구 완료 상태는 유지한다.

---

## 0.116 최신 재개 — 2026-10-15 2D 메뉴 에셋·진입/복귀·탄약 보완 PIE 통과 / 사용자 화면 확인 대기

- 사용자 증분 빌드 성공을 확인했다(DLL10월15일18:26:28KST,5,848,576bytes, SHA256 `dd465a67a2a6c8211f7a20dde16ca0cfd9682cb854fc9330c33fdca095a9b429`). 새 native검사2건 성공/실패·보고서경고0. Codex의 C++ 빌드/패키징 실행은 없다.
- 승인된 신규 메뉴7개를 Unreal 내부 API로 생성/Compile/저장하고 별도 프로세스 재로드를 통과했다. 메인화면은 `/Game/DreamCatcher/Maps/L_DC_FrontEnd`, 스토리 카드·설정·종료 구성이다. 프리 모드와 3D 배경/이동 로비는 추가하지 않았다. 카드 Artwork는 미지정으로, 최종 디자인 완성이 아니다.
- `BP_DC_Level1GameMode.ReturnMap`과 `GameDefaultMap`을 새 메뉴로 연결하고 Cook목록에 메뉴를 추가했다. `EditorStartupMap`은 Level_1 유지, 결과 재시작은 Level_1 새 판이다. 현재는 실제 메인화면 복귀가 연결돼 있으므로 과거의 “돌아가기=Level_1 재시작”을 최신 상태로 해석하지 않는다.
- 초기 실제 PIE B에서 재진입 중 `W_AmmoCounter_Rifle.InventoryInstance` Accessed None2건이 발생했다. 사용자에게 기존 HUD1개 변경 범위를 설명하고 **추가 승인을 받은 뒤**, 해당 위젯의 Tick/무기 초기화 앞에 유효성 분기2개만 추가했다. 기존 노드/수치/나머지 핀/위젯 기본값 보존과 별도 재로드/Compile 통과. 무기·전투 C++는 변경하지 않았다.
- 보완 후 `Saved/Diagnostics/Frontend/pie_C.json`은 **complete=true, Error0/ensure0/실패0**이다. 메뉴/설정/종료 취소→스토리 진입→ESC 복귀→재진입→결과 재시작→결과 메뉴 복귀를 실제 PIE에서 확인했고 3회 진입의 탄약 Bar/Glow도 인벤토리30/30과 일치했다. 결과는 `FailStage()`로 유도했으며 기존 전투/보스 전체 검증을 반복한 것은 아니다. 물리 입력·화면/최종 아트·패키징 수용은 사용자 담당이다.
- 기존 사용자 미커밋 Level_1 파일은 SHA256 `c075fae64f043905b7adcd853d03682a0908bd7b014b4d96a7ddf7eae78e42a7`로 전후 불변이다. PIE의 보호 파일과 사용자 SaveGames도 불변. 직접 Git 변경 명령·커밋·푸시·병합은 실행하지 않았다. 신규 위젯/BP6개가 staged 상태인 것은 별도 기록했으며 임의로 되돌리지 않았다. 후속 에셋 스크립트는 `-SCCProvider=None`과 실제 비활성 상태를 요구한다.
- **별도 미완료:** CommonLoadingScreen은 기존 진단 전용 조건 때문에 일반 PIE/패키징에서 관리자를 생성하지 않는다. 로딩 위젯/Config는 준비됐으나 일반 실행 표시 활성화는 미완료로 기록했다. 메뉴 왕복과 구분하며, 필요 시 플러그인 활성 경로 변경안·사용자 빌드를 별도로 진행한다. 다른 진단 조건까지 일괄 해제하지 않는다.
- 다음은 사용자가 `L_DC_FrontEnd`를 열고 새 화면·클릭/키보드·설정/복귀를 확인하는 단계다. 상세 근거와 실패 이력, 정확한 에셋/스크립트 경로는 `docs/handoffs/frontend-menu-2026-10-15.md`에 있다. 이미 완료한 R14/Level_1/복구는 반복하지 않는다.

---

## 0.117 최신 재개 — 2026-10-15 전체 부모·참조 조사와 누락 레거시33개 정리 완료

- 사용자 요청으로 B_DC_Rifle을 포함한 잔여 레거시 전체를 조사했다. 앞선 정리에서 부모 DCWeaponActor만 삭제하고 BP를 남긴 누락을 인정한다. 현재 메뉴·플레이어·맵 미커밋 변경과 staged6개를 보호한 상태로 조사/삭제했다.
- 등록 패키지3,712개, native타입경로177개, 제거한 native클래스명50개와 전체 에셋파일3,714개 문자열을 확인했다. 실제 제거된 부모를 참조하는 BP는 B_DC_Rifle1개였다. 과거 폐기 경로/목록 재등장0. 원본 타입 태그6개는 실제 로드로 현재 클래스/구조체에 정상 대응함을 확인했다.
- 삭제: 구형 B_DC_Rifle1＋임시 발사음2＋이식용 BP_QuickSlotInputSource1＋미사용 초기 LevelPrototyping29 = **33개**. 외부 참조/실행 경로 문자열0, Git LFS 원본33개 복구 가능 상태를 확인하고 Unreal 내부 API로 삭제했다. 새 프로세스의 잔여3,679개 에셋에서 삭제 대상 참조0/제거된 부모 참조0, 실제 라이플/플레이어/메뉴7개 로드 성공이다.
- 지정 대상 외 에셋 변화0, 보호 데이터파일663개 및 기존 Git staged내용 불변, Error/ensure/Fatal0. 최종 Git 조회 단계의 인덱스 메타데이터 변경은 별도 대조해 전체 경로/blob/mode와 staged6개 payload가 유지됨을 확인했다. C++/Config/DLL·Level_1·Frontend 맵·현재 메뉴 작업은 그대로이며 빌드/패키징/PIE 재실행 및 Git stage/commit/push/merge는 하지 않았다. 추가 빌드는 필요 없다.
- 현재 사용하는 초기화/장비/조준 호환 API와 Config 참조, 회귀 fixture4개·C++검사, 시그니처 확장점, 원본 Lyra/캐릭터 및 디자이너 자료를 유지했다. 유효한 경로 호환 Redirector51개는 보존했다. 원본 모션5개의 미사용 편집소스/메시 경로 누락은 별도 보존 목록에 남겼으며 전체 미사용 라이브러리0으로 확대하지 않는다.
- 상세 삭제/보존/주의 목록과 근거: `docs/handoffs/legacy-sweep-2026-10-15.md` 및 동일명JSON. 후속 메뉴 작업의 화면 확인 대기·로딩 표시 미완료는 0.116 및`docs/handoffs/frontend-menu-2026-10-15.md`를 따른다. 이번 정리를 다시 시작하지 않는다.

---

## 0.118 최신 재개 — 2026-10-15 원본 애니메이션 누락 경로 조사와 미사용5개 추가 정리

- 사용자 승인으로 0.117 보존 목록의 Attack4개/MM_WallJump1개를 추가 조사했다. 모두 최초 커밋부터 변경이 없고 설치된 UE5.8 기본 템플릿과 해시가 동일하다. 실제 로드/export에서 누락 경로는 편집용 `AnimSequenceLevelSequenceLink.PathToLevelSequence`와 `PreviewSkeletalMesh`임을 확인했다. Skeleton/모션 자체는 정상이며 현재 플레이의 필수 연결이 아니었다.
- 최신 에셋 외부 참조0, 코드·설정·스크립트의 경로/에셋명 소비0, 다른 바이너리 에셋의 경로 문자열0을 확인했다. 승인대로 미사용 템플릿5개만 Unreal 내부 API로 삭제했다. 정상 원본 템플릿과 Git LFS 복구 payload를 사전 확인했으며 활성 애니메이션/공유 Skeleton/캐릭터/디자인 데이터는 보존했다.
- 새 프로세스의 등록 패키지3,674개에서 `/Game`·`/DCStageUI` 누락 패키지 의존성과 삭제 대상 참조0. 현재 AnimBP/라이플 레이어/대기/발사/재장전5개 로드 성공, 대상 외 에셋 및 보호 데이터663개 변화0/Error·ensure·Fatal0, 기존 Git staged 항목 내용 불변이다. 빌드·패키징·PIE·Git stage/commit/push/merge는 하지 않았다.
- **누적 삭제33＋5＝38개**이며 0.117의 애니메이션 누락 경로 보존5건도 해결됐다. 정확한 삭제/복구/검증 기록은`docs/handoffs/legacy-sweep-2026-10-15.md`와 동일명JSON, 로컬 근거는`Saved/Diagnostics/LegacyAnimationLinks`다. 전체 미사용 디자인 라이브러리0이라는 뜻으로 확대하지 않는다. 추가 빌드 없이 기존0.116 메뉴 화면 수용/로딩 표시 후속을 이어간다.

---

## 0.119 최신 재개 — 2026-10-15 사용자 요청으로 애니메이션5개 복원·보존

- 사용자가 향후 필요할 수 있으므로 방금 삭제한 원본 애니메이션5개를 복원하라고 요청했다. `MM_Attack_01/02/03`, `MM_ChargedAttack`, `MM_WallJump`를 삭제 전 커밋819ac19의 원래 경로/내용으로 복원했고 모든 SHA256이 일치했다. **사용자 지시 없이 이5개를 다시 미사용 레거시로 삭제하지 않는다.**
- 앞선 B_DC_Rifle 등33개 삭제와 현재 메뉴/맵/디자이너 변경을 유지했다. 다른 에셋·보호 데이터663개·Git staged 항목 내용은 불변이며 코드/Config/에셋 재저장·빌드·PIE·커밋/푸시는 하지 않았다. 원본과 바이트 단위로 같아 완료된 로드 검사를 반복하지 않았다.
- 편집용 Level Sequence/프리뷰 메시 누락 참조는 원본 그대로 보존했다. 향후 실제 사용 시 필요한 연결을 점검하며,0.118의 “누적38개 삭제/누락 의존성0”은 삭제 당시 이력이다. 현재 정리 대상은33개 삭제＋5개 복원/보존으로 구분한다.
- 최신 목록은`docs/handoffs/legacy-sweep-2026-10-15.md`와 동일명JSON, 복원 근거는`Saved/Diagnostics/LegacyAnimationLinks/restore_A.json`이다.0.116의 메뉴 사용자 수용 대기/일반 로딩 표시 미완료 상태는 유지한다.

---

## 0.120 최신 재개 — 2026-10-15 보존 애니메이션5개 편집 연결 보완 완료

- 사용자가 누락 참조를 지금 수정할 수 있는지 요청했다. 같은 Skeleton의 SKM_Manny_Simple을5개 애니메이션의 Preview Skeletal Mesh로 연결했다. 원본 제작 Level Sequence는 프로젝트/설치 템플릿에서 찾지 못해 존재하지 않는 편집용 AnimSequenceLevelSequenceLink만 제거했다. 제작 시퀀스 자체를 복구하거나 애니메이션을 새로 bake하지 않았다.
- Unreal 내부 API 저장 후 새 프로세스 검증을 마쳤다. 허용한 PreviewSkeletalMesh/LevelSequence 링크 변경만 제외한 전체 export와 AnimationSequencerDataModel/다른 AssetUserData가 변경 전후·재로드에서 동일해 모션 키·커브·노티파이·Skeleton·설정을 보존했다. 해당5개 옛 누락 경로 참조0, 최종 Save_B/Verify_B Error·ensure·Fatal0, 대상 외 에셋/보호 데이터/staged 내용 불변이다.
- Save_A는 보호 Notifies 직접 조회가 불가능해 저장 전 중단했고 에셋 변경0이었다. 전체 export 비교로 보완한 B가 최종 성공 근거다. 원본 Git LFS payload와 초기 export는 유지했다. C++/Config 수정·빌드·PIE·커밋/푸시는 하지 않았다.
- 현재 상태는 **앞선33개 삭제 유지＋애니메이션5개 보존/편집 참조 보완**이다.5개를 추가 지시 없이 다시 삭제하지 않는다. 문서는`docs/handoffs/legacy-sweep-2026-10-15.md`와 동일명JSON, 근거는`Saved/Diagnostics/AnimationMetadataRepair/{save_B.json,verify_B.json,closeout_B.json}`이다. 추가 빌드 없이0.116 메뉴 사용자 수용/로딩 표시 후속을 이어간다.

---

## 0.121 최신 재개 — 2026-10-16 HARU weapon_r 소켓 손 부착 오프셋 교정

- 사용자가 HARU hand_r 아래 weapon_r 소켓과 기존 WID_Rifle Yaw−90°를 고려한 수정을 요청했다. 사용자 직전 생성본은 HARU Skeleton에 이름/부모가 정상이고 변환이0인 소켓1개였다. 원본 Manny의 weapon_r은 hand_r 자식 본이며 그 레퍼런스 포즈 오프셋을 기준으로 삼았다.
- 삭제·재생성 시도는 에디터 SET 명령 제한으로 저장 전 중단(파일 변화0)됐다. 제한을 우회하지 않고 지원되는 소켓 부모/로컬 변환 API로 기존 항목을 다시 설정했다. **소켓 객체를 삭제·재생성한 것이 아니다.** 변경은 HARU Skeleton1개이며 메시/다른 Skeleton 데이터/WID/사용자 작업/staged 내용은 보존했다.
- weapon_r 부모hand_r, 상대위치(-1.163383,1.317055,-0.519517)cm, 회전(Pitch15.808370,Yaw−8.517089,Roll−11.617612)°, 크기1이다. WID_Rifle의 Yaw−90°는 유지했고 원본 손 기준 최종 라이플 변환과 일치하도록 확인했다.
- 저장/새 프로세스 재로드, 소켓 밖 export 일치, 실제 B_Rifle을 쓰는 임시 Editor 월드의 부착·캐릭터 이동/회전 추종 검사를 통과했다. 최종 Error/ensure/Fatal0. **PIE·시각 수용 및 HARU 애니메이션 리타깃 검증은 수행하지 않았다.** 손가락/왼손 정밀 위치는 사용자 화면 확인을 따른다.
- C++/Config 변경·빌드·패키징·Git 커밋/푸시는 하지 않았고 추가 빌드는 필요 없다. 상세는`docs/handoffs/haru-weapon-socket-2026-10-16.md`, 근거는`Saved/Diagnostics/HaruSocketRepair/{save_D.json,verify_D.json,closeout_D.json}`다.0.120의33개 삭제/애니메이션5개 보존·참조 보완과0.116의 메뉴 화면 수용/일반 로딩 표시 대기는 유지한다.

## 0.122 최신 재개 — 2026-10-16 HARU Lyra 무기 본·가상 본·미리보기 복원

- 사용자가 실제 총 위치가 크게 틀어진 것을 확인했다. 0.121은 원본 레퍼런스 포즈만 복사한 고정 소켓이라 실제 라이플 애니메이션의 weapon_r 트랙을 재현하지 못했다. 해당 수정을 최종 해결로 재사용하지 않는다.
- HARU 메시/Skeleton2개에 원본 weapon_r 본과 가상 본3개·weapon_r_muzzle을 연결했다. 새 무기 본의 기준 회전에는 HARU의 손 축 차이를 반영하고 기존118개 본/모델/웨이트/재질/모프 목록·커브 메타데이터는 유지했다. WID_Rifle·B_Rifle·Pawn·맵·공용 AnimBP·원본 애니메이션은 불변이다.
- 잘못 연결된 SM_GUN_SMG 미리보기를 실제 SK_Rifle로 바꾸고, weapon_r_preview 소켓에 WID의−90°를 반영했다. SMG 에셋은 삭제하지 않았다. 실제 무기와 미리보기의 부착 Transform이 같다.
- 제한된 에디터 명령 CPP1개 `Editor/DCHaruRifleRigRepair.cpp`는 사용자 빌드 완료(DLL03:02:56 KST). 자체 에셋 저장은 하지 않고 unattended/전용 플래그/정확한 HARU 구조를 요구한다. Codex는 빌드/패키징을 실행하지 않았다.
- 저장/새 프로세스 재로드, 애니메이션6개×Raw/Compressed×2시점=24건씩 통과. 실제 Level_1 PIE J의 대기·이동8개 표본(최대속도600cm/s)에서 무기 본 애니메이션·라이플 부착·미리보기 일치와 파일/사용자 SaveGames 불변을 확인했다. 엔진 Error/ensure0, 확인 PNG도 수집했다. 사용자 화면 수용, 손가락 접촉 및 전체 리타깃 검증은 미완료다.
- 복구본과 검사 근거는 `Saved/Diagnostics/HaruSocketRepair/grip_*` 및 위 인계 문서. 새 FBX 재임포트 시 weapon_r 본을 유지해야 한다. 기존0.120 보존/정리와0.116 메뉴 후속은 그대로다.

## 0.123 최신 재개 — 2026-10-16 실제 무기와 미리보기 SMG 외형 교체

- 사용자 명시 승인 범위는 실제 플레이와 메시 에디터 미리보기 모두 SM_GUN_SMG로 교체다. B_Rifle에 SMGVisual을 추가하고 원본 크기50%, Yaw90°, 위치(0,−0.5,−6)cm로 손 기준을 맞췄다. WID의weapon_r/Yaw−90°와0.122의HARU 무기 본·가상 본은 유지한다.
- 기존 스켈레탈 컴포넌트는 표시하지 않고 원본 AnimBP·발사/재장전 소켓 계약을 유지한다. 별도 SK_Rifle_SMGSocketDriver의 Mesh Socket3개만 SMG 총구·탄피 위치에 맞췄다. 공용 원본 SK_Rifle과 디자이너 SM_GUN_SMG 파일은 보존했다.
- HARU의weapon_r_preview를 실제 SMG의 최종 부착 Transform과 일치시켰고 픽업 DisplayMesh/Scale/Offset도 교체했다. 변경 에셋은 기존3＋신규1이다. 스태틱 모델 자체의 탄창/노리쇠 애니메이션은 없고 캐릭터 재장전·무기 성능은 유지한다.
- 기존 에디터 보조 CPP1개 추가분은 사용자 빌드 완료(DLL03:33:04 KST). 저장/재로드와 무기 그래프·HARU 본·픽업 게임플레이 데이터 보존을 확인했다. 최종 내부 메시 Visible=False 저장본의 실제 PIE F에서도30→29→30발과 발사 이펙트·이동·총구 일치가 통과했다(Error/ensure0, 에셋/사용자 SaveGames 불변). 확인 PNG와 그립/크기 사용자 수용은 인계 문서를 따른다.
- 상세 편집 위치와 증거는`docs/handoffs/smg-visual-2026-10-16.md`. 사용자 그립/크기/화면 수용은 별도이며 현재 맵에는 해당 픽업 스포너가 없어 실제 획득 연출 검증으로 확대하지 않는다. 기존 메뉴·레거시 보존 결정은 그대로다.

## 0.124 최신 재개 — 2026-10-16 HARU 첫 대시 Blend Out 본 인덱스 오류 수정

- 사용자 첫 PIE의 BoneContainer.h616 ensure를 실제 대시에서 재현했다. 원본 FastFeet49개 항목 중 HARU에 없는38개 본이 Blend Out의 FillBoneScalesArray에서 잘못된 compact-pose 인덱스로 전달된다. 진단 ensure 상태 초기화 후 반복 대시에서도 재현돼, 첫 실행만 조용해지는 현상은 오류 보고 제한과 구분했다.
- HARU Skeleton에 실제11개 본의0.5 값을 유지한 FastFeet_Haru, HARU 전용 몽타주 사본4개를 만들었다. GA_Hero_Dash에 스켈레톤별 대안4개와 순수 선택 노드1개만 추가해 HARU에서만 사용한다. Manny와 다른 캐릭터는 원본을 유지한다. 대시 속도/거리/시간/방어 기본값과 기존 실행 핀107개는 불변이다.
- DCGameplayAbility.h/.cpp의 선택 함수·설정 배열 및 에디터 보조 CPP/Editor 모듈 의존성은 사용자 빌드 완료(DLL17:26:34 KST). 기존2＋신규4개 에셋 적용 후 원본 모션/곡선/노티파이·기존 HARU 데이터·대시 기본값의 저장/재로드 비교가 통과했다.
- 새 프로세스 PIE E1에서 첫 실제 대시·ensure 상태 초기화 후 반복·HARU4방향 재생/Blend Out·Manny 원본 선택/재생을 통과했다. 최종 Error/ensure0, 에셋/사용자 SaveGames 불변이다. 검사 중 피해 면역과 일시적 Manny 메시 전환을 사용했고 HARU로 복원했다. 사용자 새 에디터 첫 대시 화면 확인과 멀티플레이/패키징은 별도다.
- 자세한 원인·수정 범위·복구/검사 기록은`docs/handoffs/haru-dash-blend-fix-2026-10-16.md`. 이전 SMG/그립 편집·메뉴 후속·레거시 보존 결정은 유지한다. 엔진 검사 비활성화나 원본 FastFeet 삭제는 하지 않았다.

---

## 0.125 최신 재개 — 2026-10-16 Level_1 디자이너 배치와 Stash 변경 통합 복구

- 사용자 승인: 두 맵을 비교해 디자이너 배치와 기존 작업을 모두 유지하고 원본 두 개·Stash를 보존한다. 시작 HEAD/origin/main `1055f87`의 Level_1은 충돌 문구280bytes가 LFS payload로 커밋돼 있었고, 작업 인덱스도 해당 맵만 UU였다.
- 공통 기준819ac19/디자이너461577c/Stash d02c1c9를 Unreal에서 각각 비교했다. 디자이너는 건물14개 추가와 Building_26/27 높이 변경, Stash는 StaticMeshActor_4440의 잘못된 M_CalcDivergence OverrideMaterials(0) 해제1개다. 변경은 겹치지 않으며 나머지 게임플레이 액터 설정은 같다. Stash의 다른94개 변경은 이미 HEAD에 반영돼 있었다.
- 디자이너 맵을 Git으로 복원하고 Unreal 내부 API로 재질 항목1개만 반영해 Level_1만 저장했다. 저장 전후 액터5080개 상태 보존과 별도 프로세스의 전체 내보낸 액터5084개 기대값 일치를 확인했다. 생성형 자식 GUID/ISM 지연 경계 캐시와 실제 배치 데이터를 구분했다. 내보내기의 임시 부착 변경이 저장되지 않도록 비교·저장 프로세스를 분리했다.
- 실제 PIE F에서 HARU·장비·HUD·첫 인카운터 시작, AI의 완전 경로 요청/671.27cm 이동, 메인화면 복귀/AI 정리를 통과했다. 최종 Error/ensure/Fatal0, 다른 에셋·Source/Config/DLL·사용자 SaveGames 불변. 전체 전투/대시 검사를 반복하지 않았고 물리 입력·새 디자인 화면 수용은 사용자 확인으로 남긴다.
- 원본 두 개와 공통 기준의 실제 LFS 해시를 보존했고 refs/codex-recovery/level1-20261016 아래4개 복구 참조를 추가했다. Stash d02c1c9는 그대로이며 재적용할 필요가 없다. 복구 맵 하나만 Git 충돌 해결 표시하고 커밋/푸시·빌드/패키징은 사용자에게 남긴다. 상세와 최종 해시는`docs/handoffs/level1-merge-recovery-2026-10-16.md`를 따른다.

---

## 0.140 최신 재개 — 2026-10-24 PV 3에셋 적용·재로드·실제 반복/재진입/실패 경로 통과 / 사용자 수용 대기

- 사용자가 Development Editor/Win64 빌드를 성공했다. DLL은 **03:19:39 KST**, 7,273,472바이트, SHA256 `130387eace2134845140849bedd9ddb0074b984d9d391a763833714c53324890`다. 준비한 C++ 소스와 일치하며 이번 적용/검사에서 C++·Build.cs·Config를 추가 수정하거나 빌드/패키징을 실행하지 않았다. 아래0.139의 사용자 빌드·에셋 연결 대기는 이전 이력이다.
- `frontend_intro_setup.py` apply A는 종료0·complete=true, **기존 PV FileMediaSource·FrontendStateComponent 두 개와 신규 W_DC_FrontendIntro 한 개만 저장**했다. PV의 FilePath는 `./Movies/DREAMCATCHER_PV.mp4`, 신규 위젯의 IntroMediaSource는 기존 PV, FrontendStateComponent의 IntroScreenClass는 새 위젯이다. 기존 MainScreenClass/PressStartScreenClass·전체 CDO의 허용 필드 외 데이터·기존 그래프는 유지됐다. 별도 verify A는 저장0·종료0·complete=true이며 세 에셋의 전체 속성/해시·영상 존재/Validate를 확인했다.
- 새 `DreamCatcher.Frontend.Intro.KeyboardReleaseAndCleanup`·`MouseReleaseAndCleanup` **native2개 성공, 테스트 실패/경고0**이다. 건너뛴 Enter의 반복·해제/다음 입력, 마우스 해제와 미디어/화면 소유 입력 자원 해제를 검사했다. 실제 하드웨어 키 입력·게임패드 장치 수용과 구분한다.
- 실제 렌더링 PIE **loop_A**에서 PV **1920×1080, 길이36.9056666초**, 초기 재생0.1001초를 확인했다. 실제 미디어를 끝0.5초 전으로 seek한 뒤 0.0667333초로 돌아가는 반복을 관찰했다. 위젯 API로 종료하면 모드 선택 화면이 표시되고 미디어 참조가 해제됐다. 같은 `L_DC_FrontEnd`를 OpenLevel로 재실행한 별도 월드에서도 다시0.1001초부터 재생했고 두 번째 종료 후 메뉴가 표시됐다. 전체 클립 두 번 완주·실제 키 입력·플레이 후 Level_1 복귀를 재시험한 것은 아니다.
- 별도 PIE **failure_A**는 존재하지 않는 소스를 **메모리에서만** 지정해 모드 선택 메뉴 진행을 확인하고 원래 CDO 참조를 복원했다. 두 PIE는 각각 프로세스 종료0·후처리 complete=true·Engine Error/ensure/Fatal0·Automation Success다. 정상 모드는 테스트 경고0, 실패 모드는 예상한 `Missing PV media source/file; continuing to mode selection` 경고1개가 있어 무경고로 보고하지 않는다.
- 독립 후처리에서 Automation index의 UTF-8 BOM을 발견해 `frontend_intro_probe.py`의 JSON 읽기만 `utf-8-sig`로 보완했다. 기존 두 실제 PIE 결과를 정상 확정했으며 게임 코드/에셋 동작 수정이나 추가 재빌드는 없다. 에셋3개 적용 뒤 다른 콘텐츠·소스·Config·DLL·최신 Level_1/2/3·모드 선택 맵·기존 HARU/SMG/탄창·AI/보스·사용자 reticle 작업을 보존했다. Git raw index 해시는 갱신됐지만 **5,073개 스테이징 경로·모드·객체는 동일**함을 확인했고 두 Stash도 남았다. Codex는 커밋·푸시·병합을 하지 않았다.
- 근거: `Saved/Diagnostics/FrontendPV20261024/final_result_A.json`, `post_build_baseline_A.json`, `assets_A/apply.json`·verify 보고서, `pie_loop_A.json`·`pie_failure_A.json`, `index_final_A.json`, `Saved/Automation/FrontendPVNative_A`·`FrontendPVPIE_loop_A`·`FrontendPVPIE_failure_A`. **다음은 사용자 Editor에서 L_DC_FrontEnd를 실행해 키보드/마우스/게임패드 건너뛰기·재진입·화면 비율/반복·소리를 확인**하는 단계다. 추가 C++ 빌드는 필요 없으며 패키징과 물리 입력/시각/청취 수용은 자동 검사 완료에 포함하지 않는다.

## 0.139 이력 — 2026-10-24 매 모드 선택 맵 진입 PV 코드 준비 / 사용자 빌드 대기

- 사용자는 PV TODO를 1차 승인하고 **최초 실행 한 번이 아니라 `/Game/DreamCatcher/Maps/L_DC_FrontEnd` 맵에 진입할 때마다** 재생하도록 정정했다. 상세 1~5번(영상 경로·패키징, 전체 화면 위젯, 입력·종료, 메뉴 흐름 연결, 검사·기록)을 최종 승인했으며 C++ 빌드는 사용자 담당이다. 직접 순차 작성했고 서브에이전트를 사용하지 않았다.
- 앞선 Git 병합은 사용자 커밋 `dc51ea6`과 원격 `f2ba38f`의 병합 `be828d5`로 완료됐다. 새 PV·최신 저장 맵·reticle 작업을 별도 Stash로 먼저 보존한 뒤 기존 Stash 39개와 새 변경 5개를 복원했다. 겹친 Level_2는 사용자가 **현재 저장본 우선**으로 선택했고, 이어 다른 중복 파일도 현재 상태를 우선하라고 지시했다. 실제 43개 고유 파일의 해시·스테이징 상태를 확인했으며 두 Stash와 이전 Level_2도 남겼다. Codex는 커밋·푸시·병합 커밋을 만들지 않았다. 근거: `Saved/Diagnostics/Level1LocalKeep20261024/restore_A.json`.
- 신규 `UI/Frontend/DCFrontendIntroScreen.h/.cpp`는 기존 MissionClear의 엔진 MediaPlayer·MediaTexture·비공간 UI 오디오 사용 방식을 참고한다. 원본 비율·검은 여백, 실제 반복 재생, 입력 즉시 미디어 해제, 재생 실패 시 메뉴 진행, 재활성화 초기화를 구현했다. 건너뛴 키의 반복/해제 또는 마우스 해제가 메뉴 선택으로 이어지지 않도록 해당 입력만 해제까지 보관하며, 해제/창 비활성화 후 입력 프로세서를 정리한다. 마우스 이동·아날로그 축 이동은 건너뛰기 입력에 포함하지 않는다.
- `DCLyraFrontendStateComponent.h/.cpp`에 **로그인/세션 처리 뒤·모드 선택 앞의 별도 Intro 단계**와 `IntroScreenClass`를 추가했다. 기존 Press Start는 로그인된 사용자에게 생략되므로 PV 용도로 재사용하지 않았다. GameInstance에 재생 완료 플래그를 두지 않아 맵 재진입마다 실행하며 설정 창 닫기만으로는 다시 실행하지 않는다. 기존 CommonUI·로딩·메인 화면·스토리 진입 경로는 유지한다.
- `/Game/DreamCatcher/Screens/DREAMCATCHER_PV`는 읽기 전용 UE 조회에서 FileMediaSource, 원본 `C:/Users/Min/Downloads/DREAMCATCHER_PV.mp4` 존재/Validate를 확인했다. 원본 MP4를 `Content/Movies/DREAMCATCHER_PV.mp4`로 복사했다(41,079,464바이트, SHA256 `76c64ec27aa07960b3142f4cc544d9ea4e004fa15d99ab496645af60748e7f5e`, 재인코딩 없음). 복원된 `DefaultGame.ini`에 `+NonUFSMovies=DREAMCATCHER_PV` 한 줄만 추가하고 기존 MissionClear·맵 설정을 보존했다. MediaAssets 의존성은 기존 Stash에 이미 있어 Build.cs를 추가 수정하지 않았다.
- 기존 `DCFrontendAutomationTests.cpp`에 키 반복/해제와 마우스 해제·자원 정리 native 검사 2개를 추가했다. `Scripts/Editor/frontend_intro_setup.py`는 사용자 빌드 후 기존 PV·FrontendStateComponent 두 에셋과 신규 `W_DC_FrontendIntro` 한 개만 Unreal 내부 API로 설정·Compile·저장하며 별도 verify에서 재로드한다. 다른 에디터·활성 소스컨트롤·자동 Compile 저장을 거부하고 전체 CDO·기존 그래프/보호 파일·Git 인덱스를 확인한다. `frontend_intro_probe.py`는 실제 미디어 끝 근처 seek 후 반복 관찰·위젯 API 종료·같은 맵 재진입 재생과 별도 잘못된 미디어 모드를 준비한다. 물리 입력/화면/소리 수용과 구분하며 종료 후 로그·Automation 결과까지 최종 판정한다.
- **현재는 C++ 코드와 자동화 작성 단계다.** 에셋 저장0이며 PV 에셋은 아직 Downloads 원본 경로를 참조한다. 신규 위젯 생성·기존 두 에셋 연결·별도 재로드·새 검사 실행·PIE·패키징은 미실행이다. 사용자 빌드 이후 같은 최종 승인 범위에서 에셋 apply→verify→native 검사/실제 PIE로 이어간다. 기존 HARU·SMG/탄창·AI/전투·보스·최신 Level_1/2·사용자 reticle 작업은 보존한다. 코드 작성 중 별도 외부 빌드로 DLL이 갱신됐지만 그 뒤 소스도 수정됐으므로 이번 최종 소스의 빌드 성공으로 간주하지 않는다. Codex는 C++ 빌드·패키징을 실행하지 않았다.

## 0.138 최신 재개 — 2026-10-23 직접 작업 우선·서브에이전트 사용 조건 변경

- 사용자 요청에 따라 기본 작성 방식을 Codex 직접 작업으로 변경했다. 서브에이전트 2개 이상에게 맡길 수 있는 독립 작업으로 나눠 실제 병렬 처리할 수 있을 때만 서브에이전트를 사용하며, 그 외에는 직접 작업한다.
- 서브에이전트1개에만 단독 위임하거나 병렬 분담을 위해 작업을 억지로 나누지 않는다. 하위 재위임을 포함한 동시 활동 최대3개와 결과 회수 후 실행 상태 확인·정리 기준은 유지한다.
- `AGENTS.md` 0.4와 `docs/specs/gas-lyra-migration.md`의 현재 작업 절차를 같은 기준으로 맞췄다. 아래 0.137의 단일 서브에이전트 운영은 당시 이력이며 현재 작업 방식은 이 항목과 0.4를 따른다.
- TODO 두 단계 승인과 사용자 C++ 빌드·패키징·수동 확인·Git 작업 분담은 유지한다.

## 0.137 이력 — 2026-10-19 서브에이전트 수·병렬 조건·결과 회수 후 정리 규칙 반영

- 사용자는 요구사항 1차 승인과 문서별 수정 범위·단일 서브에이전트 순차 분담안을 최종 승인했다. 이번 범위는 `AGENTS.md`와 `docs/specs/gas-lyra-migration.md` 두 문서이며, 서브에이전트1개가 순차 작성하고 부모가 통합 검토한다.
- 0.4에 동시 활동 서브에이전트 **최대3개(하위 재위임 합산·부모 조율자 제외), 보통2개 이하·작은 작업1개**를 반영했다. 상호 소통·중간 결과 전달이 거의 필요 없는 독립 작업만 병렬 분담하고, 같은 파일·에셋·미확정 공통 API/설계·선행 결과 의존 작업은 순차 처리하며 대기 전용 에이전트를 미리 만들지 않는다.
- 결과 회수 후 사용 가능한 종료 수단으로 정리하고 실행 상태를 확인한다. 현재 완전 종료 도구가 없는 한계와 `interrupt_agent`의 실행 중단 의미를 구분하며, 완료 확인 후 불필요한 호출·대기를 중단한다. 완료/중단을 세션 완전 종료로 과장하거나 완료 세션의 존재만으로 지속 토큰 사용을 단정하지 않는다.
- 기존 TODO·이해한 동작 보고→1차 승인→상세 검토→최종 확인→서브에이전트 작성→부모 통합 검증과 사용자 C++ 빌드·패키징·수동 화면/소리/조작감·GitHub Desktop 작업 분담은 유지한다. **0.136의 BaseDamage 검증 완료·수동 수용 한계, 0.135·0.134 과거 이력은 보존**한다. 코드·Config·에셋·인계 문서 변경이나 빌드·Git 작업은 이번 범위에 포함하지 않는다. 부모가 변경 범위·해시·diff를 최종 검증한다.

## 0.136 최신 재개 — 2026-10-18 BaseDamage 적용·재로드·자동 검사·실제 사격·재진입 검증 완료 / 사용자 Editor 수치 확인 가능

- 사용자가 Development Editor/Win64 빌드를 완료했으며 DLL 갱신은 **23:27:27 KST**다. 새 DLL의 `DataValidation`·`Initialization`, 실제 저장 GE의 `SavedRifleExecution` **native 검사3개가 성공(실패0·경고0)**했다. 아래0.135의 빌드·에셋 적용 대기는 이력이며 같은 최종 승인 범위로 이어갔다. Codex는 C++ 빌드·패키징을 하지 않았다.
- `/Game/DreamCatcher/Experiences/B_DC_StageUIExperience`의 **Class Defaults → Combat → Damage**에서 역할별 설정 사용을 켜고 **Player Base Damage=0 / Enemy Base Damage=12**를 저장했다. `/Game/Weapons/Rifle/GE_Damage_RifleAuto`의 기존 Source BaseDamage 보정에 **Source Tags → Ignore Tags = `Gameplay.Damage.IgnoreWeaponBonus`** 하나를 추가했다. Rifle 보정12·기존 Lyra Execution·거리/약점·팀/면역·피격 피드백은 유지한다.
- 에셋 스크립트의 첫 3회 시도는 UE Python 접근 차이 때문에 중단됐다. A는 SaveOnCompile enum 변환에서 저장 전 중단, B는 Experience 1개 저장 후 nested SourceTags setter에서 중단, C는 nested/standalone struct export 차이로 GE 저장 전에 중단했다. 이를 실제 적용 성공으로 취급하지 않는다. 실제 Settings CDO text 조회와 독립 struct 복사·공개 `import_text`의 필드 지정·역변환 전체 비교로 보완했으며 **게임 C++·Config 수정이나 추가 재빌드는 없다.**
- 마지막 apply D는 **GE 1개만 저장·종료0·complete=true**, 별도 verify A는 **저장0·종료0·complete=true**다. Experience는 B에서 한 번 저장한 뒤 journal의 SHA·CDO 증거로 확인해 재저장하지 않았다. 최종 두 에셋의 전체 CDO와 허용 필드 외 데이터, 보호 inventory·해시가 일치한다. 근거는 `Saved/Diagnostics/BaseDamage20261018/assets_A/state.json` 및 같은 폴더의 attempt별 보고서, `assets_apply_D.log`, `assets_verify_A.log`다.
- 실제 사격 PIE **defaults_B**는 플레이어 몸12/머리18·적 몸12/머리18, **equal8_A**는 플레이어20/30·적8/12, **zero_enemy_A**는 플레이어20/30·적0/0을 확인했다. 모든 사격은 탄약1개를 소비했다. defaults_B는 원본 `FailStage → RequestRestartStage`로 Level_1을 재시작한 뒤 새 실제 PlayerState·적의0/12·태그와 비누적을 확인했다. 검사용8/8·적0은 메모리에서만 적용·복원했으며 저장된 설정은 **사용=True / Player0 / Enemy12**로 유지한다.
- PIE3모드는 각각 프로세스 종료0·보고서 complete=true·Engine Error/ensure/Fatal0이며 Automation은 **기존 경고를 포함한 성공**이다. NavMesh origin 정렬 재생성과 시작 시 CrowdManager의 RecastNavMesh 미발견 경고가 defaults의 두 번 진입에서4개, 나머지 모드에서각2개 발생했다. 무경고로 보고하지 않는다. 초기 **defaults_A는 검사 기록 함수 인자 충돌로 사격 전에 실패**했으며 Python 기록부 보완 후 defaults_B가 성공했다. 이 실패를 성공 건수에 포함하지 않고 게임 C++·에셋 추가 변경과 구분한다.
- **Editor에서 수정:** `/Game/DreamCatcher/Experiences/B_DC_StageUIExperience` → **Class Defaults → Combat → Damage → Player Base Damage / Enemy Base Damage**. 역할별 설정 사용을 켠 현재 상태에서 플레이어 입력값에는 라이플12가 더해지고 적 입력값은 그대로 사용된 뒤 거리·약점 배율이 적용된다. 예: 둘 다8이면 근거리 몸통20/8이다. 수정 후 Compile·Save하고 PIE를 다시 시작하면 새 플레이·적 초기화에 반영된다. 추가 C++ 빌드는 필요 없다.
- 최종 근거는 `Saved/Diagnostics/BaseDamage20261018/final_result_A.json`, `pie_defaults_B.json`·`pie_equal8_A.json`·`pie_zero_enemy_A.json`, `Saved/Automation/BaseDamageNative_A`·`BaseDamageRifle_A` 및 각 PIE 보고서다. 사용자 빌드 이후 기준에서 게임 에셋 변경은 정확히 Experience·GE **2개**, 예상 밖 변경0이다. 런타임/검사 C++5개·Config·DLL·기존 HARU·SMG/탄창·Level_1·보스·사용자 ABP 변경, 기존 NumberPop native 파일·미승인 인계 문서·Git HEAD/index를 보존했다.
- **이 묶음의 자동 적용·검증은 완료**이며 다음은 사용자 Editor에서 원하는 수치를 조절·확인하는 단계다. 사용자 화면·소리·조작감 수용, 패키징·네트워크 복제 검증을 완료한 것은 아니며0.133의 사용자 화면·소리 대기도 유지한다. 피격 인계 문서와 JSON은 수정하지 않았다.

## 0.135 이력 — 2026-10-18 BaseDamage 코드·검사·에셋 스크립트 작성 및 정적 검토 완료 / 사용자 빌드 대기

- 사용자가 플레이어·일반 적 BaseDamage 조절 TODO를 1차 승인하고, 상세 변경안 **1~5번(Experience 설정, PlayerState 초기화, 일반 적의 라이플 보정 제외, 관련 검사, 서브에이전트 분담·기록)**을 최종 승인했다. 아래 0.134의 BaseDamage 제안·승인 대기는 당시 이력이며 이번 승인이 우선한다. `docs/handoffs/hit-zones-and-damage-feedback-2026-10-18.md`와 JSON 수정은 계속 제외한다.
- 승인한 Editor 설정 위치는 `/Game/DreamCatcher/Experiences/B_DC_StageUIExperience`의 **Class Defaults → Combat → Damage**다. `bUseRoleBasedBaseDamage`를 이 Experience에서만 켜고 **Player Base Damage=0 / Enemy Base Damage=12**로 연결한다. native 기본값은 사용 안 함이며 다른 Experience에는 자동 활성화하지 않는다. 설정 변경은 새 플레이·새 적 초기화에 적용한다.
- 기존 각 PlayerState의 `DCCombatSet.BaseDamage`를 서버에서 역할별로 한 번 초기화한다. 플레이어 Controller와 일반 적 Bot Controller를 구분하고 반복 호출로 값·태그가 누적되지 않게 한다. 일반 적에게만 native 태그 `Gameplay.Damage.IgnoreWeaponBonus`를 연결하며 보스의 기존 초기화·패턴 공격은 유지한다.
- `/Game/Weapons/Rifle/GE_Damage_RifleAuto`의 기존 **Executions → Calculation Modifiers → SourceTags → IgnoreTags**에 해당 태그 하나를 추가하는 범위다. 기존 Source BaseDamage snapshot·Additive ScalableFloat **12**, `DCLyraDamageExecution`, 거리 감쇠·약점 배율·팀/면역·피격 Cue를 보존한다. 결과는 플레이어 **`(BaseDamage + 12) × 거리 배율 × 약점 배율`**, 일반 적 **`BaseDamage × 거리 배율 × 약점 배율`**이며 기본 0/12에서 현재 근거리 몸통12를 유지한다.
- 에이전트 A는 `DCLyraExperienceDefinition.h/.cpp`, `DCLyraExperiencePlayerState.h/.cpp`의 4개 C++ 파일, B는 전용 자동 검사 CPP 1개·실제 사격 스크립트 1개, C는 `Scripts/Editor/base_damage_setup.py`와 이 문서·GAS 명세를 병렬 작성했다. 기존 런타임 4개와 신규 검사 CPP 1개·스크립트 2개의 작성, 부모 통합 정적 검토·독립 C++ 정적 검토를 마쳤다. 같은 파일·에셋 저장은 병렬 처리하지 않았다.
- **코드·검사·에셋 스크립트 작성 및 정적 검토 완료이며 사용자 빌드 대기**다. C++ 빌드·패키징·자동 검사 실행·Unreal 에셋 저장·PIE 실행은 모두 미실행이다. 에셋 스크립트는 정확히 위 2개 기존 에셋만 내부 API로 설정하며 apply/verify, native 새 필드·태그, 독립 에디터/SCC 비활성, 변경 전후·저장 재개 기록과 보호 파일 검사를 구분한다. 기존 `DCLyraDamageExecution`·Config·NumberPop native 파일, HARU·SMG/탄창·Level_1·보스·현재 사용자 에셋 변경은 보존한다.
- 다음 실행 순서는 **사용자 Development Editor/Win64 빌드 → `DataValidation`·`Initialization` native 검사 2개 → 동일 승인 범위 에셋 2개 apply·별도 프로세스 verify → `SavedRifleExecution` 검사 → 실제 PIE `defaults`·`equal8`·`zero_enemy` 3모드**다. 기본0/12의 몸12/12, 검사용8/8의 몸20/8·약점30/12, 적0의 피해0과 재초기화·새 적의 비누적을 확인한다. `defaults`는 원본 `FailStage → RequestRestartStage`로 같은 맵을 재시작한 뒤 새 실제 PlayerState·적의0/12·태그도 확인하도록 준비했다. 프로세스 종료·로그 Error/ensure/Fatal·Automation 결과와 스크립트 보고서를 함께 판정한다. 아직 빌드·에셋 적용·실제 피해·수명 검증의 성공을 뜻하지 않으며 0.133의 사용자 화면·소리 확인도 별도로 남아 있다.

## 0.134 이력 — 2026-10-18 TODO 승인·서브에이전트 운영 규칙 반영 / 문서 1~2번 최종 승인

- 사용자가 앞으로의 작업을 **요구사항 → TODO·이해한 동작 보고 → 1차 승인 → 항목별 상세 검토 → 최종 확인 → 서브에이전트 작성 → 통합 검증·결과 보고**로 진행하도록 요청했다. 1차 승인 후 문서별 수정 내용과 병렬 분담안을 보고했으며, 사용자는 그중 **1번 `AGENTS.md`와 2번 `docs/specs/gas-lyra-migration.md`만 최종 승인**했다.
- 에이전트 A는 이 문서의 0.4 운영 절차·0.1/0.3 연결·최신 기록을, 에이전트 B는 기능 명세의 운영 절차 연결과 BaseDamage 후속 제안 상태를 담당한다. 서로 다른 문서이므로 병렬 작성하고 부모 에이전트가 통합 검토한다. **3번 `docs/handoffs/hit-zones-and-damage-feedback-2026-10-18.md` 수정은 미승인으로 기존 내용을 보존한다.**
- 플레이어와 일반 적의 BaseDamage를 Editor에서 각각 조절하고, 플레이어는 라이플 고정 보정 `+12`를 포함하며 일반 적은 BaseDamage만 사용하도록 하는 안은 **후속 제안·구현 승인 대기**다. 거리 감쇠·약점 배율 등 기존 Lyra 계산을 유지하는 방향이지만 세부 연결·설정 위치·수치는 구현 전에 새 절차로 검토·확인한다. 이번 문서 승인은 코드·Config·에셋 구현 승인이 아니다.
- 0.133의 약점 재질·피격 Cue/NumberPop 연결과 실제 PIE의 HARU/보스 몸12·머리18·빗나감0, Error/ensure/Fatal0 기록은 유지한다. **사용자 화면·소리 수용은 여전히 대기**이며 완료한 검증을 반복하지 않는다. HARU·SMG·탄창·Level_1 및 기존 미커밋 변경을 보존한다.

## 0.133 이력 — 2026-10-18 약점/피격 피드백 적용·실제 사격 확인 / 사용자 화면·소리 대기

- 사용자가 NumberPop 코드 빌드를 완료했다. 새 DLL의 클래스3개와 요청 구조체가 로드됐으며 이후 C++/Config 수정이나 Codex 빌드·패키징은 없다. 아래0.132의 빌드 대기는 이력이다.
- HARU Physics Asset18Body에 일반/약점 재질을 연결했다(약점: neck_01/head). 보스의 현재 PA22Body를 `/Game/DreamCatcher/Physics/PA_DC_BossHitZones`로 복제해 neck_01/neck_02/head만 약점으로 지정하고 Boss BP의 Mesh.PhysicsAssetOverride만 연결했다. 저장 전후 및 새 프로세스 비교에서 기존 도형/본/제약이 유지됐다.
- 원본 Cue·숫자·관련 누락48개를 Unreal Migrate/rename API로 이식했다. Cue는 `/Game/DreamCatcher/Lyra/GameplayCues/DamageTaken`, 나머지는 `/Game/DreamCatcher/Lyra/Feedback/DamageTaken` 아래에 있다. 원본 프로젝트와 기존 공유220개는 저장하지 않았다. DCStageUI AddComponents에 현재 StageUIController용 원본 NumberPop BP 항목1개를 추가했다. 메뉴/AI에는 추가하지 않았다.
- 실제 사격에서 확인된 MetaSound 식별자 불일치는 **새 피격 사운드5개만** 수정했다. 기존 sfx_RandomStereo_nl_meta는 원본과 RootGraph ClassName만 달라 옛 식별자로 참조하면 재생 오류가 났다. Unreal의 RootMetasoundDocument 속성 API로 의존 클래스 식별자만 대응하고 소리/입력값·공유 원본은 보존했다. Cue의 연결된 그래프와 일반/약점 사운드·숫자 선택은 원본을 유지한다.
- `Saved/Diagnostics/HitZones20261018/pie_Q.json` 실제 PIE는 HARU 몸12/머리18, 보스 몸12/머리18, 빗나감0 및 각1발 소비를 통과했다. 실제 NumberPop 호출4회와 명중 위치에 NS_DamageNumbers 생성도 확인했다. 최종 로그 Error/ensure/Fatal0, Automation 성공1(기존 경고 포함)/실패0이다. 무기 채널 진단 trace의 본/재질과 실제 발사를 함께 기록했으며 가짜 HitResult로 피해를 주지 않았다. Niagara가 소비하는 raw 배열·화면의 치명타 색/마커·소리 청취·복제 수용은 별도로 남긴다.
- 변경은 기존 에셋3개(HARU PA/Boss BP/DCStageUI)와 신규49개(피격48+보스 PA1)다. Level_1, HARU Mesh/Skeleton, B_Rifle의 SMG·탄창·원본 애니메이션, Git HEAD/index 및 사용자 SaveGames는 유지했다. 커밋/푸시/병합은 실행하지 않았다. **다음은 사용자 Editor 재실행 후 몸·머리 명중의 마커/숫자/소리와 피격 화면 확인**이며 추가 C++ 빌드는 필요 없다. 완료한 마이그레이션/탄창/맵 복구 검사는 반복하지 않는다.

## 0.132 이력 — 2026-10-18 약점/피격 피드백 A/B 승인·코드 준비 / 후속 적용은0.133

- 사용자가 두 묶음을 모두 승인하고 원본 Lyra 방식의 연결, HARU/SMG/탄창/Level_1 보존, 사용자 C++ 빌드를 명시했다. 중단을 알려 현재 파일을 확인했으며, 이미 작성된 NumberPop5파일과 Config2개를 다시 생성하거나 덮어쓰지 않았다.
- `Feedback/NumberPops/DCLyraNumberPopComponent.*`, `DCLyraNumberPopComponent_NiagaraText.*`, `DCLyraDamagePopStyleNiagara.h`는 이름/include/module API/로그를 역정규화한 원본5파일 전체 비교가 일치했다. DefaultEngine에 클래스3/구조체1 Redirect, DefaultGame에 DamageTaken Cue 스캔 경로가 준비됐다. 기존 모듈 의존성을 재사용하며 Build.cs/uproject 변경은 없다.
- `code_prepare_A.json`과 재개 후 `resume_code_check_B.json` 기준 보호10개 SHA256 및 Content3,829개 파일 상태가 불변이다. HARU Physics Asset/Boss BP/DCStageUI도 아직 변경하지 않았다. DLL/Git HEAD/index/Level_1/B_Rifle/HARU Mesh·Skeleton 유지, 에셋 저장·C++ 빌드·패키징·Git 변경 명령0이다.
- **다음은 사용자 Development Editor/Win64 빌드**다. 성공 후 새 native 타입 로드 확인→승인된 물리 재질/보스 사본 연결과 원본 피격 Cue·NumberPop 에셋 이식→재로드·실제 몸/머리 사격 검증을 진행한다. A/B 승인을 다시 묻지 않되 기존 공유 에셋 덮어쓰기나 새 범위로 임의 확대하지 않는다. 현재는 코드 준비 완료이며 약점/피격 피드백 전체 완료가 아니다.
- 상세와 후보48개/공유220개 목록은 `docs/handoffs/hit-zones-and-damage-feedback-2026-10-18.md` 및 JSON을 따른다. 0.130 완료 검사를 반복하지 않는다.

## 0.131 이력 — 2026-10-18 부위 피해·피격 피드백 조사 / 후속 A/B 승인·코드 준비는0.132

- 사용자 요청으로 모든 부위의 피해/효과가 같은 이유를 읽기 전용으로 확인했다. 시작 main `13447f7`/clean이며 0.130의 HARU/SMG/탄창/Level_1은 유지했다. 게임 C++/Config/에셋 저장·빌드·패키징·Git 변경 명령은 없다.
- 현재 무기의 `Gameplay.Zone.WeakSpot=1.5` 배율, 무기 trace의 물리 재질 반환, 피해 계산의 재질 배율 적용, 약점 Critical 히트마커 브러시는 원본과 연결돼 있다. 반면 실제 HARU Physics Asset18Body와 보스 템플릿 PA22Body에는 부위별 물리 재질 지정이 없다. 원본 Lyra PA는 일반 `PhysMat_Player`, 목/머리3Body `PhysMat_Player_WeakSpot`을 쓰며 두 재질은 현재 프로젝트에도 존재한다. 본 이름만으로 자동 배율을 정하는 구조가 아니다.
- 원본 `GCNL_Character_DamageTaken`과 일반/약점 명중·피격음/캐릭터 피격/숫자 피드백은 현재 누락돼 있다. 원본 그래프의 EvaluateWeakSpot→사운드 선택/치명타 숫자 연결을 확인했다. 일반/약점별 별도 캐릭터 파티클 선택으로 확대하지 않는다. Cue+NumberPop의 Game 의존성268 중 현재 경로 부재48(피격40+숫자8), 기존220개는 재사용 후보이며 동등성을 아직 전부 검증한 것은 아니다.
- 제안 A: 기존 HARU PA의 재질 연결＋현재 보스 PA의 별도 사본/재질 지정＋Boss BP의 PhysicsAssetOverride 연결. 현재 충돌 도형/본/제약을 유지한다. 원본 PA 전체 교체는 실제 치수 차이가 있어 제안하지 않았다.
- 제안 B: 원본 NumberPop C++5파일/타입 Redirect/Cue 스캔＋누락 피격/숫자 에셋과 현재 플레이어 Controller 연결. 원본 Cue의 숫자 호출을 삭제하거나 빈 클래스로 대체하지 않는다. **A/B 모두 아직 승인·구현 전**이며 B는 사용자 C++ 빌드가 필요하다. 실제 부위 사격·화면/소리 검증도 미완료다.
- 상세/정확한 경로/의존성 후보는 `docs/handoffs/hit-zones-and-damage-feedback-2026-10-18.md`와 같은 이름의 JSON, 로컬 원본/현재 그래프·CDO·Physics export는 `Saved/Diagnostics/HitZones20261018/`를 따른다. 기존 0.130의 완료 검사를 반복하지 않는다.

## 0.130 최신 재개 — 2026-10-18 SMG 탄창 외형 연결 / 원본 Magazine 재장전 유지

- 사용자 지정 `/Game/DreamCatcher/DreamCatcher_ENV/Prop/Gun_Magazine/GUN_Magazine`을 B_Rifle의 새 엔진 기본 StaticMeshComponent **SMGMagazineVisual**에 연결했다. 부모는 기존 숨겨진 SkeletalMesh, Parent Socket은 원본 **Magazine** 본이다. 바뀐 에셋은 B_Rifle1개이고 C++/Config/DLL 변경·빌드·패키징·Git 커밋/푸시는 없다.
- 원본 드라이버 본·ABP_Weap_Rifle/무기 재장전 몽타주·캐릭터 재장전·탄약 처리·게임플레이 그래프를 유지했다. 탄창은 원본 본의 이동/회전/스케일을 직접 따른다. 메시 원본/피벗/재질은 보존하고 SMG 삽입구에 맞춘 상대 위치 약(-0.364458,0.000012,-2.650894), 회전(Roll0,Pitch-2.96619,Yaw0), Scale0.5를 사용한다. 편집 위치는 B_Rifle→SMGMagazineVisual이다.
- Python의 보호된 Parent Socket 속성 때문에 기본 컴포넌트/Transform을 준비한 뒤 네이티브 에디터를 사용했다. computer-use의 팝업 입력 문제가 있어 사용자가 Magazine 선택·Compile/Save·에디터 종료를 확인했다. 새 C++ 보조 기능이나 자체 편집 UI는 추가하지 않았다.
- Saved/Diagnostics/SMGMagazine20261018의 verify_G2는 재로드/Compile/실제 액터 부착·기존 그래프/설정 보존을, pie_I는 실제 Level_1의646표본에서 탄창의 위치/회전/크기 추종과 원본 AM_Weap_Rifle_Reload·탄약30→29→30·대시/부착 유지를 확인했다. 위치 오차 최대8.14e-10cm, 회전/크기 오차0, Error/ensure0, SaveGames 불변. Automation은 기존 NavMesh/종료·검사 deprecation 경고 포함 성공1/실패0이다.
- presentation_H/magazine_H.png에서 저장된 무기 외형을 확인했고 새 맵 저장0이다. closeout_J에서 보호23개·복구 LFS·DLL·Git index/HEAD 보존과 검사 프로세스 종료를 확인했다. **추가 빌드·동일 검사 반복 불필요.** 사용자 손 접촉/미술적 수용·온라인/패키징은 별도다. HARU의 기존 소켓 Preview Asset은 SMG 단일 메시라 탄창 컴포넌트를 표시하지 않으며 이번에 수정하지 않았다.0.129 기능 제거를 유지한다. 상세는 `docs/handoffs/smg-magazine-2026-10-18.md`를 따른다.

## 0.129 이력 — 2026-10-17 자체 SMG 그립 편집 기능 제거 / 새 DLL·PIE 검증 완료

- 사용자가 자체 미리보기/그립 편집 기능이 필요 없다고 명시해 제거를 승인했고 에디터 저장·종료를 확인했다. **에셋/소스 제거와 사용자 재빌드·새 DLL·짧은 실제 PIE 검증 완료**다. 사용자 빌드는15:47:48 KST DLL 갱신/51.73초 성공, 크기5,879,808바이트. Codex는 빌드·패키징·커밋/푸시를 실행하지 않았다.
- 공용 ABP_ItemAnimLayersBase의 native 부모를 원래 UAnimInstance로 돌리고 추가 Modify Bone1개/Getter2개를 삭제했다. 기존 오른손 IK 출력→왼손 IK 입력과 사용자 공용 Effector(5,-0.5,-6)를 유지한다. 라이플 레이어2개의 공유 설정 참조도 제거했다. 전체 그래프 핀/기존 애니메이션 노드·기본값을 비교하고 이3개만 저장했다.
- L_HaruSMGGripPreview, BP_HaruSMGGripPreview, ABP_HaruSMGGripPreview, DA_HaruSMGGrip4개를 별도 재로드/외부 참조0 확인 후 Unreal API로 삭제했다. Animation/Grip5파일＋Editor 설정 CPP＋전용 검사 CPP7개, 최초 생성 스크립트, Build.cs의 추가 에디터 의존성2개도 제거했다.0.128 사용 안내는 과거 이력이며 다시 생성하지 않는다.
- 삭제 전 사용자 DA 값은 위치(-4,-2.5,0), 회전(Pitch0,Yaw-3.5,Roll5)로 기록했다. 이 추가 보정은 제거되어 원래 공용 IK 자세로 돌아간다. 최신 weapon_r_preview 등 원래 Unreal의 소켓/미리보기와 HARU 리깅·0.127 가중치·실제 SMG/WID/총구·몽타주·Level_1·Config는 유지했다. 무기 소켓 값의 실사용 동기화는 별도 미실행이다.
- 복구 LFS/소스·제거 근거는 Saved/Diagnostics/SMGGripRemoval20261017의 baseline_A, memory_B5, save_C, delete_D, verify_E2, closeout_F다. 후속 verify_G에서 새 DLL의 전용 native 클래스3개 부재·기존 AnimBP3개 로드/컴파일·Game/GameFeature 잔여 참조0을 확인했다. pie_H에서 원래 라이플 레이어·SMG 장착·발사30→29/재장전30·원본 대시/부착 유지가 통과했다. Error/ensure0, 에셋 저장0, SaveGames 불변이다. Automation은 NavMesh/종료/검사 속성 deprecation 경고를 포함한 성공1/실패0이며 경고0은 아니다.
- closeout_I에서 보호12개/복구 원본·Git index/HEAD 보존과 검사 프로세스 종료를 확인했다. **추가 빌드·같은 자동 검사 반복은 불필요**하며 사용자 물리 입력·미술적 수용은 별도로 기록한다. 상세 삭제 목록·복구·검증 한계는 `docs/handoffs/smg-grip-removal-2026-10-17.md`를 따른다.

## 0.128 이력 — 2026-10-17 HARU SMG 그립 편집 환경 / 이후0.129에서 사용자 요청으로 제거

- 사용자 승인 범위의 코드7파일·Build.cs 에디터 의존성, 신규4/기존3개 에셋을 저장했다. 사용자 빌드는03:04:26 KST/71.89초, 에디터 자동 갱신 보완은04:04:39 KST/6.05초 성공이다. C++ 계산 검사1건, 저장·새 프로세스 재로드와 실제 Level_1 PIE가 통과했다. **다음은 사용자 그립 조정·화면 수용이며 추가 빌드 불필요**다. Codex는 빌드·패키징·커밋/푸시를 실행하지 않았다.
- `/Game/Developers/SMGGrip/L_HaruSMGGripPreview`에서 액터의 LeftHandTarget을 W/E로 이동·회전한다. Preview Aiming (ADS)로 자세를 전환하고 Apply Grip to Game Settings→Save All로 `/Game/DreamCatcher/Animations/Haru/Grip/DA_HaruSMGGrip`에 반영한다. Reload Saved Grip은 현재 공유 DA 값으로 되돌린다. 같은 HARU/SMG인 플레이어·일반 AI에 적용된다. 고정 자세 편집기이며 전체 플레이 시뮬레이터는 아니다.
- 공용 ABP_ItemAnimLayersBase native 부모를 UAnimInstance 하위 타입으로 확장하고 왼손 IK 앞 표준 Modify Bone1개/Getter2개만 추가했다. 라이플 레이어2개가 공유 설정을 지정한다. 사용자 기존 Effector(5,-0.5,-6)·HandIK_Left_Alpha/커브는 유지하며 초기 보정은 항등이다. 재장전/대시와 몽타주 블렌드 아웃 동안 새 보정만0으로 한다. 원래 팔 길이 제한도 유지한다.
- `Saved/Diagnostics/SMGGripEditor20261017/pie_X.json`의 실제 PIE에서 초기 경로 동일성·공유 값의 왼손 반영·오른손/무기/기타 본 보존·HARU/보이는 SMG 조건·재장전/대시 블렌드 아웃이 통과했다. Error/ensure0, 에셋 저장0, SaveGames 불변. 두 번째 빌드는 PreviewActor.cpp의 전용 Editor Tick만 바꿨고 게임 검사를 반복하지 않았다. `preview_Y.json`은 자동 갱신·Apply/Undo/Redo/Reload·대기/ADS 전환, `final_AA.json`은 최종 맵 재로드/초기 시점과 자세 전환 중 편집값 보존에 성공했다. 사용자 물리 입력·미술적 수용은 별도다.
- 원본 공용 BP 사용자 수정본 SHA10b9acef…35a40은 Git LFS 객체로 보존했다(`original_lfs_G.json`). 계획했던 복제 Backup BP는 컴파일 제약으로 저장하지 않았다. 기존 HARU/스켈레톤/가중치·무기·Level_1·애니메이션/몽타주·Config/uproject와 Git index/HEAD는 보존한다. 최초 생성용 `Scripts/Editor/smg_grip_editor_setup.py`는 현재 상태에서 재실행하지 않는다. 상세 경로·검증 근거·편집/복구 방법은 `docs/handoffs/smg-grip-preview-2026-10-17.md`를 따른다. 완료된0.127 가중치·GAS/맵 검증을 반복하지 않는다.

## 0.127 이력 — 2026-10-17 HARU 근육 가중치 전이 적용 / 사용자 화면 수용 대기

- 사용자가 HARU 사본에 원본 Lyra 가중치를 전이·검증 후 실사용에 적용하고 얼굴·머리카락·손가락·장비 연결을 보존하도록 승인했다. 실제 원본과 일치를 확인한 Manny의 표면 가중치를 사용했다. 단순 전체 전이는 소매/코트의 큰 변형으로 거부했고, 관절별 기존 비율·영향 슬롯·CC 비틀림 연결을 보존하며 근육 보정만 전이했다. 원본과 먼 의상은 거리 기반 혼합으로 원래 가중치를 더 남긴다. 구체적 수치·근거는 새 인계를 따른다.
- 실사용 `/Game/DreamCatcher/Main_Charcher/Haru/SKM_HARU`의21,999정점 중2,917정점에 새 근육 본35개 가중치를 연결했다. 보호18,656정점의16비트 가중치는 정확히 일치한다. 얼굴 모프 목록·재질·PhysicsAsset·Skeleton/Post Process·실제208＋가상3개 본·기존 부모/기준 자세와 형상은 유지했다. 원본에 가중치가 있던 새 본56개 중 비틀림16개는 기존 CC 연결, 손목4개는 손 보호, upperarm_in_r은 보호/기존 슬롯 우선 조건상 추가 가중치0이다. 모든 본에 강제로 가중치를 넣은 것이 아니다.
- 최종 사본N→실제 PIE O의9자세/18표본 비교→보존 사본 생성 및 실사용 저장P→별도 프로세스 재로드/실제 PIE Q를 통과했다. 보호 표면 변형·기존 본 변환 차이0, 정의한 기준의 신규 심한 늘어짐/면 붕괴0, Error/ensure0. 실제 원본 대시 활성/Blend Out도 통과하고 게임 화면을 확인했다. 사용자 물리 입력·미술적 수용과 모든 애니메이션/표정/패키징 검증은 별도다.
- 적용 전/검증 사본은 `/Game/Developers/Codex/HaruWeightTransfer/`의 `SKM_Haru_BeforeWeightTransfer`, `SKM_Haru_LyraWeights_Candidate`다. 원본 복구는 Unreal SkinWeightModifier로 가중치만 수행하며 맵/리깅을 되돌리지 않는다. 이번에는 C++/Config/원본 리깅/맵·Git index를 변경하지 않았고 추가 빌드 불필요. 소스 FBX는 갱신하지 않았으므로 과거 FBX 재임포트는 새 본과 가중치를 소실시킬 수 있다.
- 최종 SHA256 `e7467b197b7cd74e08ac25c00638f54998d837d66b5544aae8aca85be3c77348`. 상세·검증 한계·실패 후보·재개 근거: `docs/handoffs/haru-lyra-weight-transfer-2026-10-17.md`. 사용자는 Level_1에서 어깨/팔꿈치/무릎과 손가락/SMG 접촉을 확인하면 된다. 기존 GAS/R14/맵 통합 검증은 반복하지 않는다.

## 0.126 이력 — 2026-10-16 HARU 전체 Lyra 본·보정 구동 적용

- 사용자가 HARU에 없는38개 다리 보정 본을 포함해 Lyra 누락 본 전체 추가와 원본 리깅 재사용을 승인했다. 작업 전 실제119개＋가상3개에서89개를 추가해 실제208개＋가상3개가 됐다. 원본 Manny/Quinn 실제161개＋가상3개의 누락은0개며 기존119개 본의 부모/기준 자세는 유지했다.
- 실제 Lyra 프로젝트와 현재 이식본의 Manny/Quinn 전체 본·기준 변환·가중치, 원본 Control Rig176개 노드/핀/연결, Pose Driver14개 설정 일치를 확인했다. HARU 고유47개 본과 기존119개 본은 보존한다. 피부 가중치가 있는 cc_base 비틀림16개에는 원본 구동을 신체 구간 위치/기준 회전 오프셋을 반영해 연결한다. 체형이 다른 Manny의 피부 가중치를 무작정 복사하지 않는다.
- 원본176노드＋HARU 회전 제약16개 사본과 원본14개 Pose Driver를 유지한 Post Process 사본을 연결했다. 기존 HARU Mesh/Skeleton/GA_Hero_Dash3개와 신규 Rig/Post Process2개만 저장했다. 기존 형상·전체 가중치·모프 이름/재질/소켓/프로필·가상 본과 원본 그래프 보존을 별도 재로드에서 확인했다. 신규 근육 보정 본 전체의 HARU 피부 가중치를 새로 제작한 것은 아니다.
- SkeletonModifier 이후 메모리 메시의 가상 본 목록 갱신 문제를 공식 PostLoad와 같은 RebuildRefSkeleton 호출로 해결했다. `Editor/DCHaruLyraRigRepair.cpp`1개는 에디터/정확한 HARU208＋3/전용 플래그를 검사하고 저장하지 않는다. 사용자 Development Editor/Win64 빌드19:27:12 KST 성공, DLL5,879,808bytes 확인. Build.cs·런타임 플레이 코드 변경0이다.
- 최종 PIE P에서 대기/ADS/앉기/걷기/재장전5종의 주 관절/weapon_r 회전차0°·위치차 부동소수 오차 수준, 기존 중간 비틀림 본8개 실제 구동을 확인했다. 원본 GAS 전방 대시와 나머지3방향 몽타주 Blend Out이 통과했다. GA_Hero_Dash의 HARU override4개를 해제했고 이전 전용 사본/프로필은 보존했다. 저장/재로드/최종PIE Error·ensure·Fatal0, 다른 에셋·맵·Config·SaveGames·Git 인덱스/HEAD 불변이다. 사용자 화면 수용·전체애니메이션/온라인/패키징은 별도이며 상세는`docs/handoffs/haru-lyra-full-rig-2026-10-16.md`를 따른다.

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

> 2026-10-11 결정: 아래 분리는 예고가 필요한 강한 단발 공격/보스 패턴에만 적용한다. 일반 적의 원본 무기 사격에 추가 선딜을 강제하지 않는다. 최신 범위는 0.74를 따른다.

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

> 2026-10-11 결정: 일반 적은 원본 무기 발사 흐름을 사용한다. 아래 목표는 예고가 필요한 강공격/보스 전용이며 일반 적의 즉시 발사 자체를 결함으로 보지 않는다.

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
**2026-10-11 사용자 결정:** 실제 피해 차단/성공 판정은 유지하되, 회피 성공 전용 시각·음향 피드백 제작은 현재 범위에서 제외합니다. 기존 Dash 시작 연출·효과음을 삭제하는 결정이 아닙니다.

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

적용 범위는 예고가 필요한 강한 단발 공격/보스 패턴이다(2026-10-11 사용자 결정). 일반 적의 원본 발사 경로는 별도로 연결한다.

- 공격 의도, 선딜, 명중, 후딜 분리
- 연출 이벤트
- 예고 공격에서 예고 전 데미지 방지
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
