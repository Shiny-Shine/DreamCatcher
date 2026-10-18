# HARU의 Lyra 전체 본과 보정 리깅 확장

**2026-10-17 후속 0.127:** 이 문서의 가중치 보존은 본 추가 당시의 이력이다. 이후 승인된 사본 검증을 거쳐 근육 본35개 가중치를 실사용 HARU의2,917정점에 전이했다. 얼굴/머리카락/손가락/장비와 기존 비틀림 연결은 유지한다. 현재 상태·복구 사본·검증 범위는 [가중치 전이 인계](haru-lyra-weight-transfer-2026-10-17.md)가 우선한다.

현재 상태는 **사용자 빌드·5개 에셋 적용/저장·새 프로세스 재로드·실제 PIE 검증 완료, 사용자 화면 수용 대기**다. HARU에 없는 Lyra 본89개를 추가해 실제208개＋기존 가상3개＝211개가 됐다. 원본 Manny/Quinn의 누락 본은0개다. 추가 빌드는 필요 없다.

## 확인한 원본과 변경 범위

- 작업 전 HARU는 실제 본119개＋가상 본3개였다. Lyra Manny와 Quinn은 실제 본161개＋가상 본3개이며 두 원본의 본 이름 집합은 같다.
- 누락됐던89개에는 대시 FastFeet의38개뿐 아니라 상체 보정·비틀림 본과 weapon_l·interaction·center_of_mass 등이 포함된다. 기존 공통 본의 부모 계층 차이는0이었다.
- HARU 전용 cc_base 본47개는 보존한다. 이 중 팔·다리 비틀림16개에 실제 피부 가중치가 있다. 원본 Manny에는 누락89개 중56개에 가중치가 있지만, 체형과 정점 구성이 다른 HARU에 그 가중치를 그대로 복사하지 않는다.
- 실제 LyraStarterGame을 읽어 현재 이식본과 Manny/Quinn의 전체 본·기준 변환·가중치 해시 일치를 확인했다. 원본 Control Rig176개 노드/핀/연결도 현재 이식본과 동일하다. Post Process의 원본 Pose Driver14개 설정도 일치한다(내보내기에는 작성/컴파일 사본28개가 포함됨).

승인된 작업은 기존 HARU Mesh/Skeleton에89개를 추가하고, 원본 기반 HARU 사본2개로 기존 비틀림 본을 구동하는 것이다. 기존119개 본의 계층·기준 자세, 메시 형상·가중치·얼굴 모프·무기 본/소켓·SMG 연결은 유지한다. 새 본은 원본 이름/부모를 따르며 기준 위치는 대응하는 HARU 신체 구간 길이에 맞춘다. weapon_l은 앞서 weapon_r에서 검증한 손 축 대응을 따른다.

변경한 에셋은 기존3개＋신규2개다.

- 기존 `/Game/DreamCatcher/Main_Charcher/Haru/SKM_HARU`
- 기존 `/Game/DreamCatcher/Main_Charcher/Haru/하루리깅_Skeleton`
- 신규 `/Game/DreamCatcher/Animations/Haru/Rig/CR_Haru_LyraProcedural`
- 신규 `/Game/DreamCatcher/Animations/Haru/Rig/ABP_Haru_LyraPostProcess`
- 기존 `/Game/DreamCatcher/Lyra/Dash/GA_Hero_Dash`: HARU 전용 몽타주 override4개를 해제해 원본 몽타주/전체 FastFeet 프로필을 사용한다. 선택 함수/그래프와 다른 기본값은 유지했다. 이전 HARU 사본4개와 FastFeet_Haru는 복구용으로 보존했고 삭제하지 않았다.

원본 Manny/Quinn·원본 Control Rig/Post Process/PoseAsset·맵·Config는 수정하지 않는다. 원본의 모든 근육 부피 보정에 HARU의 새 스킨 가중치를 제작하는 작업까지 자동 완료로 간주하지 않는다. 원본 구동 그래프는 유지하고, 현재 HARU에 이미 존재하는 비틀림 가중치를 연결한다.

## 준비한 구성과 통과 범위

`Saved/Diagnostics/HaruLyraRig20261016/extend_rig.py`는 지정된 에셋에만 작동하며 메모리 검증 후5개만 저장했다. GeometryScript의 정점22,044개/삼각형41,241개 형상 해시와 SkinWeightModifier의 전체21,999정점 가중치 해시가 변경 전·저장 전·새 프로세스 재로드에서 같다. 두 정점 수 차이는 조회 데이터 표현의 차이다.

- 형상 해시: `ea131f10c2ed4af8e6e7fd9903bc95ecb9927042c512bffc0727cca980439b27`
- 이름 기준 가중치 해시: `510e85500b5c70bc4bcf5209df06cd4831934cbf3c260c8929dc18f317b12601`

원본 Control Rig176개 노드는 그대로 복제하고, 마지막 실행 분기 뒤에 회전 제약16개를 연결했다. 기존 cc_base 비틀림 본의 신체 구간상 위치에 따라 원본 주 본/비틀림 본 사이를 보간한다. 기준 회전 오프셋과 기존 가중치를 유지하고, 손·몸통·주 관절을 바꾸지 않는지를 실제 자세 비교에서 확인한다. Post Process 사본은 원본14개 Pose Driver와 원본 PoseAsset을 유지하고 Control Rig 참조만 HARU 사본으로 바꾼다.

메모리 G의 구성/Compile과 메모리 L의 실제 자세 검사 후 저장 N·재로드 O를 통과했다. 원본176개 노드/핀/연결과14개 Pose Driver 설정, 추가 회전 제약16개, 기존 본119개·가상 본3개, 모프 이름/재질/소켓/프로필 및 대시의 override 외 기본값을 보존했다.

## 가상 본 갱신과 사용자 빌드 완료

실제 PIE I에서 본 수가211이 아닌208로 나와 검사 단계에서 중단했다. Skeleton의 가상 본3개 배열과 GUID는 보존돼 있었지만, `USkeletonModifier::CommitSkeletonToSkeletalMesh` 이후 메모리 메시의 최종 참조 본 목록에 가상 본이 포함되지 않았다. UE5.8의 `USkeletalMesh::PostLoad`는 `GetRefSkeleton().RebuildRefSkeleton(GetSkeleton(), false)`로 이를 다시 구성한다.

이 공식 엔진 호출을 저장 전 메모리 검사에서도 수행하도록 **`Source/DreamCatcher/Editor/DCHaruLyraRigRepair.cpp` 1개**를 추가했다. `WITH_EDITOR`, unattended 및 `-DCHaruLyraRigRepair`가 모두 필요하다. 정확한 HARU 경로·실제 본208개·기존 이름/연결의 가상 본3개를 검사하고, 갱신 후211개 및 원래 실제 본 데이터 불변을 확인한다. 콘솔 명령은 `DC.HaruRig.RefreshLyraVirtualBones`이며 자체 저장을 하지 않는다. Build.cs·런타임 플레이 코드는 변경하지 않았다.

사용자 **Development Editor / Win64 빌드 성공**, DLL2026-10-16 **19:27:12 KST**, 5,879,808bytes, SHA256 `d79f2c72f684ae2d464fdf1a51f9eadd3bf618a8f95bb29e5b51d443181cacf7`를 확인했다. UBT 결과 Succeeded/69.59초다. 새 명령이 메모리 메시를208＋3으로 갱신하는 것을 확인했고, 별도 재로드에서는 정상 PostLoad만으로211개가 로드됐다. Codex는 C++ 빌드를 실행하지 않았다.

## 저장본의 실제 PIE와 마무리

- 최종 PIE P에서 Post Process OFF/ON을 대기·ADS·앉기·걷기·재장전5개 시점에서 비교했다. 주 관절/weapon_r의 최대 회전 차이0°, 위치 차이는 부동소수 오차 수준(6×10^-14cm 미만)이었다. 기존 cc_base 중간 비틀림 본8개에는 실제 회전 변화가 있고 구간 시작8개는 기존 방향을 유지한다. 제약은16개다.
- 원본 GA_Hero_Dash를 실제 GAS로 실행해 원본 전방 몽타주/전체 FastFeet 종료를 확인했다. 후방·좌·우 원본 몽타주도 각각 재생·Blend Out을 확인했다. 후자3개는 직접 몽타주 재생이며 물리 방향 입력 검증으로 확대하지 않는다.
- 저장 N·재로드 O·PIE P의 전체 종료 로그에 Error/ensure/Fatal0, Automation Project.Maps.PIE는 Success다. 사용자 SaveGames·보호 파일495개·다른 콘텐츠·맵·Config와 Git 인덱스/HEAD는 유지됐다. 기존 Manny PoseAsset 등의 경고는 별도다.
- 실제 화면 `Saved/Diagnostics/HaruLyraRig20261016/pose_P.png`에서 캐릭터와 부착 무기를 확인했다. 팔·다리 표면의 미술적 수용과 물리 입력은 사용자가 확인한다. 신규 근육 보정 본 전체의 가중치 제작이나 모든 애니메이션/멀티플레이/패키징 검증 완료를 의미하지 않는다.
- 원본3개는 시작 커밋 `df1de39`와 해시 검증한 로컬 LFS 캐시에 보존돼 있다. 정확한 원본/저장본 해시는 `before_save_M.json`, `save_N.json`, `closeout_P.json`에 있다. 커밋·푸시·스테이징은 하지 않았다.

다음은 추가 빌드 없이 에디터를 열어 팔·다리 비틀림, 손/SMG 부착, 대시를 짧게 확인하는 것이다. `SKM_HARU`의 Post Process Anim Blueprint는 `ABP_Haru_LyraPostProcess`다. 원본 공용 Control Rig 대신 HARU 전용 사본을 편집한다. 소스 FBX는 수정하지 않았으므로 재임포트할 때 추가89개와 기존 weapon_r/가상 본 연결을 보존해야 한다. 저장 스크립트를 재실행하지 않는다.

## 검사 중 API 교정 기록

Audit A는 보호된 LODInfo 조회, B는 보호된 Blueprint ParentClass 조회에서 중단했다. 공개 Unreal 텍스트 내보내기로 교정했고 B의 완료된 메시/가중치 자료는 해시 대조 후 C에서 재사용했다. API E의 링크 조회는 핀의 공개 링크 목록으로 교정했다. PIE H는 검사 로그 파일명 불일치, I는 위 가상 본 목록 문제, K는 Python에 독립 메서드로 노출되지 않은 BlueprintSetter 호출 문제였다. K의 전환은 공식 UFunction 호출로 교정했고 실패 콜백도 즉시 해제하도록 보완했다. 저장 M은 Python 튜플/JSON 목록 표현 차이로 저장 전에 중단했다. 실제 값이 모두 같음을 확인해 표현을 정규화한 N이 저장했다. 이 중단들은 모두 에셋 저장0이며 최종 성공 N/O/P와 구분한다.

근거: `Saved/Diagnostics/HaruLyraRig20261016/{audit_C.json,audit_Original_D.json,original_graph_comparison_J.json,memory_L.json,pose_L.json,before_save_M.json,save_N.json,verify_O.json,pose_P.json,closeout_P.json}`와 `Saved/Logs/HaruLyraRig-*.log`.

앞선 Level_1 복구는 사용자 정상 확인 후 `df1de39`로 커밋·푸시됐으며 다시 수행하지 않는다. 현재 리깅 작업에서는 Git stage/commit/push와 C++ 빌드/패키징을 실행하지 않았다.
