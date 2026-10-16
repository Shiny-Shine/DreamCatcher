# HARU 첫 대시 BoneContainer 오류 수정

현재 상태는 **원인 재현·코드 사용자 빌드·에셋 적용·새 프로세스 재로드·실제 PIE 검사 통과**다. 사용자가 에디터를 새로 열고 첫 대시의 끊김과 로그를 확인하는 단계가 남아 있다.

## 원인

사용자 로그의 `SkeletonToCompactPose.IsValidIndex(SkeletonIndex)`는 대시 몽타주가 종료되며 Blend Out을 계산할 때 발생했다. 크래시 기록의 모듈 오프셋을 DLL export와 대조해 `UBlendProfile::FillBoneScalesArray → FAnimInstanceProxy::SlotEvaluatePoseWithBlendProfiles → FAnimNode_Slot` 경로를 확인했다.

대시4개는 원본 Manny의 `FastFeet`를 BlendProfileOut으로 사용한다. 이 프로필49개 항목 중 HARU에 없는 다리 보정 본이38개다. UE5.8.1의 해당 경로는 스켈레톤 remapping 결과가 INDEX_NONE인 항목도 구형 compact-pose 변환 함수에 넘기며, 그 함수에서 ensure가 발생한다. HARU에 실제 있는11개 항목의 값은 모두0.5다.

새 프로세스에서 입력 없이 대기할 때는 오류0, 첫 실제 대시에서 동일 오류가 재현됐다. 진단 프로세스에서만 `core.ResetEnsureState`를 호출하고 다시 대시하자 재차 보고됐다. 따라서 두 번째 PIE가 조용한 것은 일반 ensure의 호출 위치별 보고 제한으로 설명되며, 문제가 없어졌다는 근거가 아니다. 앞선 HARU/SMG 검증에는 이 대시 종료 경로가 빠져 있었다.

## 수정 범위

- HARU Skeleton에 `FastFeet_Haru`를 추가했다. 현재 스켈레톤에 존재하는11개 본과 원본0.5 값을 유지한다. 기존 본·가상 본·소켓·미리보기·모프 메타데이터는 바꾸지 않았다.
- `/Game/DreamCatcher/Animations/Haru/Dash/AM_Haru_Dash_{Forward,Backward,Left,Right}` 사본4개를 추가했다. 원본 몽타주의 시퀀스·커브·노티파이·블렌드 시간·슬롯·기타 설정은 동일하며 BlendProfileOut만 HARU 프로필로 연결했다. 원본 대시 몽타주4개와 Manny Skeleton/FastFeet는 그대로다.
- `UDCGameplayAbility`에 명시적으로 등록한 스켈레톤별 몽타주 대안을 반환하는 순수 함수와 기본값 배열을 추가했다. `GA_Hero_Dash`만 HARU용4개 대안을 등록하고 기존 MontageToPlay 입력 앞에 선택 노드1개를 연결했다. 현재 Avatar가 HARU일 때만 사본을 반환하며 Manny/다른 스켈레톤은 원본을 반환한다.
- 대안 선택을 AbilityTask 생성 전에 하므로 재생·취소·종료가 같은 몽타주 포인터를 사용한다. 실행 핀107개와 기존 노드90개, 대시의 속도/RootMotionDuration/AbilityDuration/쿨다운/방어 기본값은 보존했다. 최종 작성 노드는91개다.
- 에디터 전용 `DCHaruDashMontageRepair.cpp`는 지정한 두 Skeleton과 GA_Hero_Dash만 처리하며 unattended/전용 플래그를 요구한다. 자체 저장은 하지 않는다. Build.cs의 UnrealEd/BlueprintGraph 의존성도 Editor 빌드에만 추가했다.

변경 에셋은 기존2개(HARU Skeleton, GA_Hero_Dash)와 신규 몽타주4개다. SMG·HARU 메시·맵·Config·원본 Lyra 에셋은 보존했다. 엔진 소스/DLL이나 오류 보고 설정을 수정하지 않았다.

## 검증

- 사용자 Development Editor/Win64 빌드 후 DLL2026-10-16 17:26:34 KST/5,878,272bytes를 확인했다. 최초 에디터 보조 코드의 GetAllGraphs 호출 오류는 실제 UE5.8 API인 `UBlueprint::GetAllGraphs`로 교정한 뒤 빌드됐다. Codex는 C++ 빌드·패키징을 실행하지 않았다.
- 저장 D와 별도 프로세스 재로드 D에서 모든 몽타주 내용을 정규화 비교하고 프로필 연결 외 차이가 없음을 확인했다. 기존 HARU 데이터·대시 기본값·작성된 실행 그래프도 일치했다.
- 새 프로세스 PIE E1: 실제 GAS 첫 대시가 HARU 사본을 사용하고 이동·종료했다. ensure 보고 상태 초기화 후 반복 대시도 통과했다. HARU4방향 몽타주 각각의 재생/Blend Out, Manny로 임시 전환했을 때 원본4개 선택과 원본 전방 대시 Blend Out도 통과했다. 종료 전 HARU로 복원했다.
- 최종 엔진 로그 Error/ensure0, 에셋·보호 코드/설정·사용자 SaveGames 불변이다. 진단 중 피격 간섭을 막는 임시 DamageImmunity를 사용했으므로 피해 회피/방어 성능 시험으로 확대하지 않는다. 방향 선택 함수의 기존 분기와 데이터는 유지했으며4방향 검사는 각 몽타주 재생/종료 검사다.
- 사용자 미커밋/스테이징 상태를 유지했고 기존 대상2개는 LFS 캐시에 복구 가능하게 보존했다. 커밋·푸시·패키징은 하지 않았다.

## 재개 및 근거

사용자는 새 에디터의 첫 PIE에서 대시를 확인하면 된다. 코드 빌드는 이미 반영됐으므로 에셋 변경 후 추가 빌드는 필요 없다. SMG 손 위치/크기·왼손 IK 편집 기능과0.116 메뉴 후속은 이번 범위에 포함하지 않았다.

근거는 `Saved/Diagnostics/BoneEnsure/{exports_stack.json,dash_inspect_A.json,repro_A.json,before_fix_B.json,fix_save_D.json,fix_verify_D.json,fixed_pie_E1.json,closeout_E1.json}`와 `Saved/Logs/BoneEnsure-*.log`다. 재현 A의 ensure는 수정 전 의도한 재현 결과이며 통과 검사로 취급하지 않는다. Save_B의 상대 경로 검사와 Save_C의 컴파일 임시 그래프 포함 문제는 저장 전에 중단됐다. D는 실제 작성 그래프만 분리해 기존90개 노드·107개 실행 핀 보존을 확인했다.
