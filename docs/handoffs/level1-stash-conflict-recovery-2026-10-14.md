# Level 1 맵과 재질의 Stash 충돌 복구

2026-10-14 사용자가 신규 디자인 에셋을 유지하면서 `Level_1`과 `MI_Concrete_3`만 정상본으로 복원하고 디자이너의 Stash 작업을 보존하도록 승인했다.

**현재 상태**

- **2026-10-15 사용자 확인: 복구 브랜치 커밋·푸시 및 PR #2 main 병합 완료, 디자이너도 복구 성공.** 복구 커밋은 `349048a5994d762d42fa1cbbdde227a8bf22416e`, 병합 main은 `13aa3b0f9b2c312dc303544625166d575cffb8f0`다. 병합 시각은 10월14일23:36 KST다.
- 사용한 복구 브랜치: `codex/restore-level1-assets-20261014`. 현재 작업 기준은 위 main이다.
- 브랜치 기준: 원격 최신 `7f533259c9abf5f660ed59286964e5920d92285b` (`26.10.14_Asset_2`).
- 정상 원본 기준: `57200f8220dd13823639c6f9ba8a2eae7e3d1690`.
- 게임 파일 변경은 아래 두 개뿐이다. Git 원본 복원으로 정상 payload를 되돌렸으며 바이너리 내부 편집·재저장은 하지 않았다.
- 새 디자인 에셋 10개와 앞선 추가분을 유지했다. 두 추가 커밋의 환경 에셋 27개를 확인했으며, 그중 손상됐던 `MI_Concrete_3`만 승인대로 직전 정상 버전을 사용한다.
- 복구 두 에셋과 디자이너 안내/이 문서가 main에 반영됐다. 현재 맵/재질/DLL 해시는 복구 검증본과 일치한다.
- **디자이너 Stash 원본 데이터는 아직 확보하지 못했다.** 로컬 정상본 복원과 별개인 미완료 항목이다.

| 파일 | 정상 데이터 SHA256 | 정상 크기 |
|---|---|---:|
| `Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap` | `339853e578404fc970dad9350b4132d2e8bbd18f30b7fbe08a170f8728ae3041` | 10,699,233 bytes |
| `Content/DreamCatcher/DreamCatcher_ENV/Tile/Materials/Garden/01_warm_concrete/MI_Concrete_3.uasset` | `8e27e0c21939877607baee26727fb53bab7dbebff07f539f2cfa945de38426ec` | 11,803 bytes |

**손상 원인과 보존한 기록**

최신 커밋의 두 파일은 각각 280/274 bytes의 텍스트였다. LFS 포인터 안의 `<<<<<<< Updated upstream`, `=======`, `>>>>>>> Stashed changes` 충돌 구분자가 해결되지 않은 채 다시 LFS payload로 올라갔다. Unreal 에셋을 삭제한 커밋은 아니지만 해당 텍스트를 맵/재질로 읽을 수 없었다.

손상 payload는 커밋 `7f53325`와 로컬 LFS 캐시에 남아 있다. 두 텍스트 전체는 `Saved/Diagnostics/PullRecovery/before_A.json`에도 보존했다. 기존 Git Stash 항목을 적용·삭제·정리하지 않았다.

충돌 텍스트의 디자이너 `Stashed changes` 쪽은 다음 원본을 가리킨다.

| 대상 | Stash 쪽 원본 SHA256 | 기록된 크기 | 확보 상태 |
|---|---|---:|---|
| Level_1 | `43d3524adf3fafa632a36a9fbc798a45745738ef1206087726c9765ceb34b705` | 10,129,803 bytes | 원격 LFS HTTP 404, 이 PC 캐시에도 없음 |
| MI_Concrete_3 | `151110a4d2003dc21160e93ba7707184116ef2085c359c65391312124c7d4639` | 14,084 bytes | 원격 LFS HTTP 404, 이 PC 캐시에도 없음 |

사용자는 디자이너 PC의 두 캐시 파일이 일부 또는 모두 없다고 답했고, Stash/자동저장본도 없거나 확인이 어렵다고 답했다. 따라서 식별 정보·충돌 기록 보존을 실제 Stash 원본 백업 완료로 간주하지 않는다. 디자이너 PC에서 올바른 저장소 폴더와 Git 공용 디렉터리, 기존 Stash, `Saved/Autosaves`·`Saved/Backup` 또는 별도 백업을 확인해야 한다. 남아 있는 저장소·Stash·자동저장본을 삭제하거나 덮어쓰지 않는다. 원본을 찾으면 정상 복구본과 별도로 보존한 뒤 환경 배치 차이를 비교하며, 검증된 게임플레이 연결을 통째로 대체하지 않는다.

**검증**

- 정상 두 파일의 실제 SHA256/크기가 위 원본과 일치한다.
- 최신 커밋 대비 추적 파일 변경은 정확히 두 개이고 Git 인덱스는 비어 있다.
- Unreal 별도 프로세스에서 두 파일 및 신규 디자인 에셋 10개 로드 성공. `Level_1` 월드의 액터 4,652개를 읽었다.
- 검사 중 에셋 저장 0, Blueprint Compile 0, 파일 변경 0, 보호 대상 479개 변경 0, Error/ensure/Fatal 0.
- C++ 빌드·패키징·PIE는 반복하지 않았다. 복원된 두 파일은 기존 정상 수용본과 바이트 단위로 같다.
- 근거: `Saved/Diagnostics/PullRecovery/{before_A.json,stash_probe_A.json,editor_verify_A.json,closeout_A.json}`, `Saved/Logs/RestoreLevel1Assets-A.log`.

브랜치 전환 중 열린 Editor가 맵 파일을 잠가 `unable to unlink` 경고가 났다. 그때 맵은 정상 해시로 유지됐고, 사용자가 Editor를 닫은 뒤 두 경로를 Git 원본으로 다시 복원해 검증했다. 이 경고를 정상 브랜치 전환만으로 해결됐다고 판단하지 않았다.

**복구 종료 후 다음 작업**

main 반영과 디자이너의 정상본 수용은 완료됐으므로 복구 브랜치 생성/복원/검증을 반복하지 않는다. 다음은 [메인화면·아웃게임·UI 마무리 인계](project-finish-resume-2026-10-15.md)를 따른다. 디자이너의 별도 Stash 원본 확보는 여전히 확인되지 않았으며, 정상본 복구 성공과 구분해 위 식별 정보를 보존한다. 새 단서나 사용자 요청 없이 해당 복구 조사를 다시 시작하지 않는다.
