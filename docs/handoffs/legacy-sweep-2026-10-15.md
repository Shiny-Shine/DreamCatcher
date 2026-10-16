# 잔여 레거시 전체 조사와 정리

2026-10-15 사용자가 `B_DC_Rifle`의 invalid parent 오류를 보고 남은 레거시 전체 조사·정리를 승인했다. 이전 정리에서 부모 C++ `DCWeaponActor`만 제거하고 이 BP를 남긴 것은 누락이었다. 이번에는 경로별 후보 목록에만 의존하지 않고 전체 에셋의 타입·부모·참조를 조사했다.

**최신 — 보존 애니메이션5개 편집용 누락 참조 보완 / 0.120**

사용자가 누락 참조를 지금 고칠 수 있는지 요청해, 같은5개에 한정하여 편집 연결을 보완했다. 동일 Skeleton을 사용하는 `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`을 Preview Skeletal Mesh로 지정했다. 원본 `LS_Attack_*`/`LS_ChargedAttack` 제작 시퀀스는 프로젝트와 설치 템플릿에서 찾지 못했으므로, `AnimSequenceLevelSequenceLink`의 없는 파일 링크만 제거했다. 제작 시퀀스 자체를 복원한 작업은 아니다.

엔진 내부 API로 저장하고 새 프로세스에서 재로드했다. 위 두 편집 변경만 제외한 전체 T3D export, AnimationSequencerDataModel, 다른 AssetUserData의 해시가 변경 전·변경 후·새 프로세스에서 모두 일치한다. 따라서 모션 키·커브·노티파이·Skeleton·재생 설정을 유지했다.5개 모두 정상 미리보기 메시를 참조하고 옛 시퀀스/메시 누락 의존성은0이다. 대상 외 에셋/보호 데이터/staged 내용 변화0, 최종 Save_B/Verify_B Error·ensure·Fatal0이다. C++ 빌드·PIE는 하지 않았다.

최초 Save_A는 보호된 Notifies 속성을 직접 조회할 수 없어 **저장 전** 중단됐다(에셋 변경0). 전체 텍스트 export 비교 방식으로 전환한 Save_B/Verify_B가 최종 성공 근거다. 기존 원본 Git LFS payload와 초기 export는 복구 근거로 유지한다. 현재 상태는 **33개 삭제 유지＋애니메이션5개 보존/편집 참조 보완**이다. 근거는 `Saved/Diagnostics/AnimationMetadataRepair/{before_A.json,save_B.json,verify_B.json,closeout_B.json}`이다.

**이력 — 애니메이션5개 원본 복원과 향후 재사용 보존 / 0.119**

사용자가 나중에 필요할 수 있으므로 방금 삭제한 애니메이션5개를 복원하도록 요청했다. `MM_Attack_01/02/03`, `MM_ChargedAttack`, `MM_WallJump`만 삭제 전 커밋의 원래 경로·내용으로 복원했고5개 SHA256이 모두 원본과 일치한다. 현재 유효한 삭제 목록은 앞선33개이며, 이5개는 향후 재사용할 보존 자료다. 사용자 지시 없이 다시 미사용 레거시로 삭제하지 않는다.

편집용 Level Sequence·프리뷰 메시의 누락 참조는 원본 그대로 남아 있다. 실제 모션 채택 시 필요한 편집 연결을 점검한다. 다른 에셋 전체·보호 데이터663개·기존 staged 내용과 앞선33개 삭제는 유지했다. 바이트 단위 원본 복원이므로 빌드/PIE/동일 에셋 로드 검사를 반복하지 않았다. 근거는 `Saved/Diagnostics/LegacyAnimationLinks/restore_A.json`이다.

**이력 — 애니메이션 누락 경로 조사와 추가5개 정리 / 0.118**

사용자가 이전 보존 목록의 원본 애니메이션5개도 조사해 필요한 연결은 고치고 미사용 레거시면 정리하도록 승인했다. 정확한 대상은 `Characters/Mannequins/Anims/Unarmed/Attack`의 `MM_Attack_01`, `MM_Attack_02`, `MM_Attack_03`, `MM_ChargedAttack`과 `Jump/MM_WallJump`다.

- 5개 모두 프로젝트 최초 커밋 이후 변경이 없으며, 설치된 UE5.8 `Templates/TemplateResources/High/Characters/Content/Mannequins/Anims/Unarmed`의 기본 템플릿과 SHA256이 완전히 일치했다. 디자이너의 수정 모션이 아니다.
- 실제 Unreal 로드/텍스트 export에서 누락 경로가 `AnimSequenceLevelSequenceLink.PathToLevelSequence`와 `PreviewSkeletalMesh`임을 확인했다. 전자는 엔진 클래스의 `IsEditorOnly()`가 true인 편집 소스 연결이고, 후자는 미리보기용 메시다. 유효한 Skeleton과 재생 길이(약0.47~1.83초)는 남아 있었다. 현재 재생 경로를 복구해야 하는 문제가 아니었다.
- 최신 AssetRegistry 외부 참조0, 현재 Source/Config/Scripts/Plugins의 경로·에셋명 소비0, 다른 바이너리 에셋의 경로 문자열0을 확인했다. 사용하지 않는 원본 템플릿으로 확정해 **이5개만 추가 삭제**했다. 정상 동작 중인 애니메이션이나 공유 Skeleton/메시/리깅 데이터는 변경하지 않았다.
- 원본 템플릿 및 Git LFS 복구본을 해시 검증한 뒤 Unreal 내부 삭제 API를 사용했다. 새 프로세스에서 등록 패키지3,674개를 조회해 `/Game`·`/DCStageUI`의 누락 패키지 의존성과 삭제 대상 잔여 참조가 모두0이었다.
- 현재 기본 AnimBP, 라이플 AnimLayer, 라이플 대기·발사·재장전 에셋5개 로드 성공. 보호 데이터663개와 기존 Git 인덱스의 경로/blob/mode 항목은 그대로이며, 대상 외 에셋 변화0/Error·ensure·Fatal0이다. C++ 빌드·패키징·PIE는 반복하지 않았다.

당시 누적 삭제는33＋5＝38개였으나 위0.119 사용자 요청으로5개를 복원했다. 아래33개 삭제는 유지한다. 삭제 당시 근거는 `Saved/Diagnostics/LegacyAnimationLinks/{before_A.json,inspect_A.json,literal_refs_A.json,delete_A.json,verify_A.json,closeout_A.json}` 및 `Saved/Logs/LegacyAnimationLinks-*.log`다. 현재33개 삭제와5개 복원/보존을 [정리 목록](legacy-sweep-2026-10-15.json)에 구분했다.

**완료한 변경**

| 삭제 묶음 | 수 | 근거 |
|---|---:|---|
| `/Game/DreamCatcher/Blueprints/Weapons/B_DC_Rifle` | 1 | 삭제된 DCWeaponActor를 부모로 사용. 현재 장착 경로는 별도의 Lyra `B_Rifle`이며 외부 참조 없음 |
| `SC_SMG_Fire_Placeholder`, `SW_SMG_Fire_Placeholder` | 2 | 8월12일 구형 사격 기능의 임시 발사음. Cue→Wave 외 사용처 없음 |
| `Lyra/UI/Shared/Equipment/Diagnostics/BP_QuickSlotInputSource` | 1 | 이식 때 사용한 입력 그래프 원본용 진단 Pawn. 현재 사용처 없음. 실제 QuickSlot Input Action들은 보존 |
| `/Game/LevelPrototyping` | 29 | 초기 UE 프로토타입 문·점프패드·표적 BP와 전용 메시/이펙트/재질/텍스처. 묶음 외 참조 및 실행 코드 경로 참조 없음 |
| 합계 | **33** | Unreal 내부 삭제 API로 제거 |

정확한 경로·각 파일 SHA256·복구 기준 커밋은 [정리 목록](legacy-sweep-2026-10-15.json)에 있다. `819ac1947cf8abda15e70135c271fe646533177c`의 Git LFS 원본33개가 로컬에 존재하고 현재 파일과 같음을 확인한 뒤 삭제했다. 일반 파일 I/O로 uasset을 변경하거나 삭제하지 않았다.

**전체 조사와 검증 범위**

- `/Game` 및 `/DCStageUI`의 등록 패키지3,712개, native 타입 경로177개, 과거 제거한 native 클래스명50개를 조사했다. 전체 에셋 파일3,714개에서 제거한 타입 문자열도 확인했다.
- 삭제된 native 부모가 실제로 남은 BP는 B_DC_Rifle1개였다. 프로젝트 부모 패키지 누락0, 과거 정리 목록 재등장0, 폐기한 `GAS/Test`·`LyraMigration` 등 경로 잔존0이었다.
- 타입 태그가 과거 이름인 라이브러리6개는 실제 로드로 정상임을 확인했다. PhysicalMaterial2개는 현재 DC 클래스, B_LyraGameInstance는 DCLyraGameInstance, 외부 팩 구조체3개는 CoreUObject.UserDefinedStruct로 읽힌다. 이들은 잘못된 부모 BP로 분류하지 않는다.
- 삭제 전 외부 AssetRegistry 참조0, Source/Config/현재 Scripts/Plugins의 대상 경로 문자열0을 확인했다. 삭제 후 남은 바이너리 에셋의 대상 경로 문자열도0이다.
- 새 프로세스에서 등록 에셋3,679개를 재조회해 이번 삭제 대상의 끊어진 참조0, 제거한 부모 클래스 참조0을 확인했다. 실제 라이플 B_Rifle/WID_Rifle/ID_Rifle, 현재 Pawn, Level1/Frontend GameMode, 새 메뉴 위젯을 합친7개 에셋이 정상 로드됐다.
- 삭제 대상 외 에셋 변화0, 보호 데이터 파일663개 변화0, Error/ensure/Fatal0이다. 엔진 검사 시 Git 인덱스 해시도 유지됐지만 마지막 Git 조회 단계에서 인덱스 메타데이터가 달라졌다. 전체 경로/blob/mode와 기존 staged6개 payload를 별도로 대조해 staging 내용은 그대로임을 확인했다. 삭제 시 구형 BP 부모와 프로토타입 표적의 원래 누락 재질 로드 경고는 삭제 대상으로 한정된 이력이며 새 프로세스 검증의 실패와 구분한다.
- C++·Config·DLL과 Level_1/Frontend 맵 및 현재 사용자 에셋 변경을 보존했다. C++ 빌드·패키징·PIE를 반복하지 않았다. Git stage/commit/push/merge와 사용자 Stash 조작도 하지 않았다.

**보존한 항목과 이유**

- 현재 메뉴 작업: 0.116의 신규 메뉴/게임모드/Experience/탄약 위젯과 staged6개를 그대로 유지했다. 사용자 화면 수용과 일반 로딩 표시 활성화의 기존 대기 상태도 그대로다.
- 활성 게임플레이: 플레이어/일반 봇/보스, 라이플과 장비, 맵·환경·디자인 모델, 입력, 설정, 공용 Lyra 라이브러리를 유지했다. `OnAbilitySystemUninitializing` 등 과거 compatibility 주석이 있는 훅은 현재 Hero/Loadout이 실제 사용하므로 제거하지 않았다.
- 회귀 검사: `/Game/Tests/Equipment`4개와 현재 C++ 검사들은 검증 도구다. Config의 NeverCook 정책을 유지했다. 이름에 Test가 있다는 이유로 삭제하지 않았다.
- 확장점: `GA_DC_SignatureBase`는 승인된 시그니처 능력 기반이며 실제 효과 연결은 별도다. Config 호환 경로인 `Struct_UIMessaging`도 유지했다.
- 제작 자료: 디자이너 맵·Haru 등 캐릭터 원본, 재질/텍스처 및 원본 캐릭터 애니메이션은 미사용이라는 이유만으로 폐기하지 않았다. 현재 경로로 이어지는 designer/vendor Redirector51개도 경로 호환 자료로 기록하고 대량 Fix Up/맵 재저장을 하지 않았다.
- 외부 `BlockingStarterPack`은 Git에서 제외된 제작용 팩이다. 등록되지 않은 소형 맵 파일2개도 이 팩의 로컬 파일이며 이번 프로젝트 레거시 삭제 대상에 포함하지 않았다.

전체 원본·제작 라이브러리까지 미사용 자산이0이라고 판정한 작업은 아니다. Attack4개와 WallJump1개는 복원 후 위0.120에서 모션을 유지하며 편집용 누락 참조를 보완했다. 이전 누락 경로는 JSON의 `resolved_library_metadata_issues`, 현재 미해결 목록은 빈 `remaining_library_metadata_issues`로 구분한다. 원본 복구 해시와 수정 후 해시는 `restored_for_future_use`, 보완 근거는 `animation_metadata_repair`에 있다. 이5개와 나머지 정상 원본·디자인 자료는 보존한다.

**다음 작업**

이번 정리/복원/편집 연결 보완은 완료됐으며 추가 빌드가 필요 없다. GitHub Desktop에는 기존 메뉴 작업에 더해 에셋33개 삭제, 애니메이션5개 수정과 이 정리 문서가 표시된다.5개 수정은 미리보기 메시와 끊어진 편집 링크만 바뀐 결과다. 사용자가 작업 묶음을 검토해 커밋·푸시한다. 메뉴 작업은 [기존 인계](frontend-menu-2026-10-15.md)를 이어가고 완료된 마이그레이션이나 정리를 반복하지 않는다.

로컬 근거: `Saved/Diagnostics/LegacySweep/{before_A.json,inspect_A.json,compatibility_A.json,manifest_A.json,delete_A.json,verify_A.json,remaining_literal_refs_A.json,index_audit_A.json,closeout_A.json}` 및 `Saved/Logs/LegacySweep-*.log`.
