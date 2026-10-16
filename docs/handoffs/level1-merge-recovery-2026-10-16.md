# Level 1 디자이너 배치와 Stash 작업 통합 복구

2026-10-16 사용자가 두 맵의 변경을 모두 유지하며 Level_1을 복구하고, 원본 두 개와 Stash를 보존하도록 승인했다. 디자이너 맵을 기준으로 기존 재질 수정 한 항목을 반영해 저장·별도 프로세스 재로드·짧은 실제 PIE 검사를 통과했다. 게임 에셋 변경은 Level_1 하나다. 사용자 화면 확인과 복구 커밋·푸시는 남아 있다.

## 충돌 원인과 비교 결과

작업 시작 HEAD와 로컬 origin/main은 `1055f87b28c1fa5e2f22558b514b15ac1a87dec2`였다. 그 커밋의 Level_1 LFS payload는 정상 맵 대신 `<<<<<<< Updated upstream` 등이 들어 있는 280바이트 텍스트였다. 이후 Stash 복원 충돌로 작업 파일에도 충돌 문구가 남고 Git 인덱스가 UU 상태였다.

공통 기준 `819ac19`와 디자이너 `461577c`, Stash `d02c1c9`의 맵을 각각 Git으로 작업 경로에 복원한 뒤 Unreal 내부 API로 읽었다. 원본 LFS 데이터는 변경하지 않았다.

| 변경 출처 | 공통 기준 대비 변경 | 복구본 처리 |
|---|---|---|
| 디자이너 | 건물 StaticMeshActor 14개 추가 | 이름·GUID·메시·재질·변환·배치 유지 |
| 디자이너 | Building_26/27 (`StaticMeshActor_4372/4373`)의 높이 변경 | 각각 Z −174.680756 / −824.225525 유지 |
| 기존 Stash | `StaticMeshActor_4440` (`SM_Building_400x671`)의 OverrideMaterials(0)를 None으로 변경 | 잘못 연결된 Water 시뮬레이션 재질 해제를 유지 |

추가 건물의 액터 이름은 `StaticMeshActor_472`, `480`, `487`, `488`, `490`, `491`, `505`, `506`, `547`, `552`, `554`, `615`, `620`, `621`이다. 정확한 경로·변환은 비교 보고서에 있다. 양쪽에서 같은 액터를 다르게 수정하거나 삭제한 충돌은 없었다. GameMode·StageDirector·Encounter·Spawner·PlayerStart·NavMeshBoundsVolume의 내보낸 설정은 공통 기준부터 유지돼 있었다. RecastNavMesh의 로드 시 위치는 디자이너 버전을 유지했다.

Stash 변경 95개 중 맵을 제외한 94개는 이미 HEAD와 같은 내용이다. 대시 수정·SMG·HARU 리깅·메뉴·기존 정리 결과를 다시 적용하거나 되돌리지 않았다. 전체 Stash를 다시 Restore할 필요가 없다.

## 원본 보존

Stash `stash@{0}` / `d02c1c97ef95fbf109f810af9c8970ba518f786b`를 적용·삭제·변경하지 않았다. 다음 원본의 실제 데이터가 로컬 `.git/lfs/objects`에 있고 크기·SHA256을 확인했다.

| 원본 | SHA256 | bytes |
|---|---|---:|
| 디자이너 `461577c05280442ad3964067a42683f1922a63f5` | `741b861086ec656d68e5ffeb0cdf07f06f42877ad359825ad56121462d7a2bb5` | 11,692,672 |
| Stash `d02c1c9` | `c075fae64f043905b7adcd853d03682a0908bd7b014b4d96a7ddf7eae78e42a7` | 11,662,422 |
| 공통 기준 `819ac19` | `8af060a8caab0968672f58f34f9469a07e9d0c79690e28596e4c18ed1e363e10` | 10,688,060 |

추가 보존용 로컬 Git 참조는 `refs/codex-recovery/level1-20261016/{base,designer,local,damaged-head}`다. 각각 공통 기준·디자이너 커밋·Stash 커밋·시작 HEAD를 가리킨다. Git 이력과 Stash, LFS 원본을 정리하지 않는다. 충돌 텍스트와 원래 인덱스의 세 단계도 `before.json`에 기록했다. 이 로컬 보존을 별도 원격 백업 완료로 해석하지 않는다.

## 저장과 검증

- 디자이너 정상본을 Git으로 복원하고, Unreal에서 위 액터의 OverrideMaterials 한 항목만 None으로 설정했다. 스켈레탈 메시·무기·Blueprint·C++·Config·다른 맵은 변경하지 않았다.
- 저장 프로세스에서 액터 5,080개의 클래스·이름·변환·부착·폴더와 컴포넌트 메시·재질을 비교했다. 생성형 자식 액터 463개의 GUID는 프로세스마다 바뀌므로 서로 다른 프로세스 비교에서만 제외했다. 같은 저장 프로세스의 수정 전후와 저장 전후에는 GUID를 포함해 의도한 재질 변경 외 일치를 확인했다.
- 별도 재로드 프로세스에서 내보낸 액터 레코드 5,084개가 디자이너 원본＋Stash 재질 수정의 기대값과 일치했다. 이 개수에는 생성형 자식·월드 보조 액터가 포함된다. Packed Level의 ISM 9개에 대한 지연 계산 CachedBounds만 정규화했고 실제 메시·변환·참조는 비교했다. 재로드 검사는 에셋을 저장하지 않았다.
- `BP_DC_Level1GameMode`, 일반 스포너 2개＋보스 스포너 1개, Dynamic NavMesh, 메인화면 `L_DC_FrontEnd` 복귀 연결을 유지했다. Water의 `M_CalcDivergence` 액터 참조는 해소됐다.
- 실제 PIE F에서 HARU 플레이어·장비·HUD·첫 인카운터 시작, AI의 부분 경로를 허용하지 않는 이동 요청과 약 671.27cm 이동, 메뉴 복귀 함수로 메인화면 진입 및 AI 정리를 확인했다. 피해 면역과 일시적인 AI 이동 명령을 사용하는 진단이며, 물리 입력·전체 전투 재검증·새 배치의 미술 수용은 별도다.
- 저장 D·재로드 D·PIE F는 Error/ensure/Fatal 0이며 보호 코드/Config/DLL·사용자 SaveGames에 변경이 없다. 기존 FootstepEffectTagModifier/LyraGame 및 NavMesh 초기화 경고, Manny PoseAsset 경고는 이번 복구의 수정 범위가 아니다.

복구 파일은 `Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap`, SHA256 `7e464520fdaf02b7b8b3abb453a8dac1f41947055b522531ed4011a62f4861e6`, 10,623,594바이트다. 크기 차이로 보존 여부를 판단하지 않고 위 저장·재로드의 액터/속성 비교로 확인했다.

## 검사 중 중단 기록

적용 A는 생성형 자식 GUID, B는 ISM의 지연 계산 CachedBounds 차이에서 저장 전에 중단했다. 적용 C는 월드 T3D 내보내기가 액터를 임시 분리·재부착하며 일부 Static 자식을 Movable 부모에 재부착하지 못하는 엔진 경로를 검출해 저장하지 않았다. 엔진 `EditorExporters.cpp`의 실제 동작을 확인한 뒤, 최종 적용 D는 월드를 내보내지 않는 저장 전용 프로세스로 분리했다. 비교 프로세스는 파일을 저장하지 않고 종료했다. 이 중단들을 최종 게임플레이 실패나 저장된 맵 손상으로 확대하지 않는다.

PIE E는 Experience 완료 직후 아직 StageDirector가 시작되기 전인 시점에 검사해 조기 실패했다. 준비 완료를 기다리도록 검사 코드만 교정한 F가 통과했다. 게임 코드·에셋 추가 수정은 없었다.

## Git 마무리와 다음 작업

검증된 맵 한 파일만 `git add`로 충돌 해결 표시했다. Git 미해결 충돌은 0개이고, 저장된 정상 맵의 SHA256/크기와 인덱스 LFS 포인터, 새 LFS payload 일치를 확인했다. 다른 인덱스 경로·HEAD·Stash는 유지됐다. 커밋·푸시·브랜치 병합·C++ 빌드·패키징은 실행하지 않았다.

사용자는 에디터에서 건물 배치와 기존 플레이 화면을 짧게 확인한 뒤 GitHub Desktop에서 복구 맵과 관련 문서를 새 커밋으로 올리면 된다. 권장 제목은 `fix: recover Level_1 preserving designer and local changes`다. 기존 커밋을 강제로 되돌리거나 Stash를 다시 적용할 필요는 없다.

근거: `Saved/Diagnostics/Level1Merge20261016/{before.json,base.json,designer.json,local.json,comparison.json,apply_D.json,verify_D.json,pie_F.json,closeout.json}`, `Saved/Logs/Level1Merge-*.log`, `Saved/Automation/Level1Merge_F/`.
