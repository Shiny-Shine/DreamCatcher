# 자동화·잔여 코드 정리 — 2026-10-27

## 범위와 현재 상태

사용자는 완료된 일회성 도구, 현재 구조와 맞지 않는 검사, 확실한 미사용 코드와 캐시만 삭제하도록 승인했다. 처음 제안한 59개 일괄 퇴역안은 철회했다. 아래 1차 정리는 완료됐으며, 활성 UI의 Editor 이관은 후속 승인 범위로 구분한다.

- Python 52개 중 **31개 삭제**, 검사 후보 13개와 그 참조를 보수적으로 따라간 지원 파일 8개, **총21개 유지**.
- 추적 중이던 Python 캐시 1개, 완료된 제작용 Editor C++ 4개 삭제. **총36개 파일 삭제**.
- C++ 테스트 **49개 전부**, 읽기 전용 `DCReticleLayoutCapture.cpp` 유지. Inspector 리타게팅 검사도 유지한다.
- `DCModeCardButton`에서 실제 카드에 없는 텍스트·외곽선 처리만 제거했다. 사용자 확인을 받은 1.03배·0.15초 호버 구현은 보존했다.
- 제작용 C++에서만 쓰던 Editor 모듈 의존성 6개 제거. Inspector 검사에 필요한 IKRig/IKRigEditor/AnimationBlueprintLibrary와 UnrealEd/MessageLog는 유지했다.
- Python 바이트코드의 재추적을 막도록 `.gitignore`를 보완했다.
- **사용자 Development Editor / Win64 빌드 및 카드3개 재로드·Compile 확인 완료**. 사용자 빌드 로그는 `Succeeded / 70.51 seconds`, DLL은 2026-10-27 02:43:07 KST / 6,929,408 bytes다. Codex는 빌드·패키징·커밋·푸시를 실행하지 않았다.

기존 에셋과 `Saved/Diagnostics`·`Saved/Automation` 근거는 삭제하지 않았다. 유지한 검사는 재사용 가능성 때문에 보존한 것으로, 이번 정리에서 모든 검사를 현재 버전으로 재실행했다는 뜻은 아니다.

## 0.149 공통 카드·UMG 이관·기존 코드 정리 / 최종 사용자 수용 완료

**2026-10-29 최종 승인 반영:** 사용자는 공통 `W_DC_ModeCard` 하나로 통합하고 이미지 크기·여백·기울기를 통일하도록 승인했다. 최종 회신에서 **이미지 위치가 달라져도 공통 값을 적용하고 추가 이미지 처리는 하지 말 것**을 명시했다. 따라서 투명 여백을 보충한 텍스처 사본2개 제안은 철회했다. 기존 세 텍스처를 그대로 공통 크기로 표시하므로 프리·챌린지의 이전 위치·표시 비율과 미세한 차이가 생길 수 있다. 원본 PNG·텍스처 가공, 추가 이미지 에셋, 보정용 런타임 코드는 없다.

### 자동 적용 완료

- `/Game/DreamCatcher/UI/Frontend/W_DC_ModeCard`: 기존 Story 카드를 Unreal 내부 API로 복제해 공통 카드로 제작했다. 부모는 기존 `DCModeCardButton`, 트리는 `CardContent → ArtworkImage`다.
- `/Game/DreamCatcher/UI/Frontend/W_DC_Frontend`: StoryButton·FreeButton·ChallengeButton 모두 공통 클래스 인스턴스로 교체했다. 이름·부모 위치와 기존 C++ 클릭/키보드 이동 연결은 유지한다. 이미지 참조만 각각 기존 Storymode·Freemode·Challengemode다.
- 세 버튼의 크기는380×824, Render Shear X는-13.774172, Pivot은(0,0)으로 통일했다. 프리·챌린지 클릭 영역 높이는822→824다.
- ArtworkImage는 크기624×862, Canvas 위치(-215.96117,-8), Shear X=13.774172, Pivot=(0.5,0.5), 고정 Render Translation=(105.65777,0)이다. 이 공통 중심 설정 덕분에 **애니메이션에는 Scale만 키를 넣는다.** 이전 카드별 Translation 애니메이션 값은 더 이상 사용하지 않는다.
- 최초 제작 때 공통 기본값과 세 인스턴스의 **Hover Scale=1**을 적용해 C++ 확대와의 중복 실행을 막았다. 이후 아래 C++ 정리로 이 속성을 제거했으므로 현재는 UMG 애니메이션만 사용한다.
- 초기 통합에서 배경 타일링을 포함한 다른 위젯 설정 및 원본 카드3개·텍스처3개·카드 C++2개 해시 보존을 확인했다. 이후 승인된 C++/옛 카드 정리는 아래 별도 결과를 따른다.

**제작 당시 자동 검증:** `Saved/Diagnostics/ModeCardCommon20261029/apply.json`은 위젯2개 저장·교체 참조 누락0, `verify.json`은 별도 프로세스 재로드·Compile·클래스/이미지/배치 비교 통과·저장0이다. apply_C와 verify_A 모두 종료0이고 Python/Blueprint Error·Fatal·ensure는 검출되지 않았다. 초기 apply_A는 브러시 자료형 조회, apply_B는 빈 선택적 상속 바인딩 조회 때문에 모두 저장 전에 중단됐다. 해당 진단은 보존한다. `result.json`에 제작 당시 두 에셋 해시와 보호 결과를 기록했으며 실행 참조 확인 후 일회성 `common_card.py`는 삭제했다. 이 보고서는 아래 사용자 애니메이션 편집 이전의 근거다.

**2026-10-29 사용자 수용·C++ 제거:** 사용자는 아래 공통 UMG 애니메이션·Forward/Reverse·복원 연결, Compile·Save 및 PIE 확인에 대해 **“적용했고 세 카드 모두 정상”**이라고 답했다. 이 수용을 반영해 승인된 `DCModeCardButton.h/.cpp`의 확대용 Tick·마우스 추적·보간·중심 보정·HoverScale/HoverDuration·상태·lifecycle override와 불필요한 include를 제거했다. Artwork·ArtworkImage 및 NativePreConstruct 이미지 설정은 유지했다. 활성 Source/Scripts/Config에서 제거한 심볼의 잔여 참조가 없고 diff 공백 검사를 통과했다. 새 런타임 코드나 테스트는 추가하지 않았다.

**빌드·에셋 정리 완료:** 사용자 Development Editor/Win64 빌드 **Succeeded /41.97초**, DLL **2026-10-29 18:14:03 KST /6,894,080 bytes**를 확인했다. 새 DLL에서 공통 카드와 부모 화면을 Compile·재저장하고, Asset Registry의 hard/soft/manage/searchable 참조 및 EditorAssetLibrary 확인 결과가 모두 빈 목록인 기존 Story/Free/Challenge 카드3개만 Unreal 내부 API로 삭제했다. 원본 이미지3개와 C++ 파일은 이 에셋 작업에서 그대로 유지했다.

- `cleanup_apply.json`: 참조0, 위젯2개 저장, 옛 카드3개 삭제, HoverScale/HoverDuration 제거 확인.
- `cleanup_verify.json`: 새 프로세스 재로드·Compile·삭제 확인 성공, 저장0·삭제0. HoverZoom 및 Forward/Reverse/StopAnimation/SetRenderScale 호출, 세 공통 인스턴스·원본 이미지 연결 유지.
- `common_cleanup_apply.t3d`와 `common_cleanup_verify.t3d`: 사용자가 작성한 **EventGraph·HoverZoom·WidgetTree 블록이 정확히 일치**한다. 전체 내보내기의 차이는 컴파일러 중간 노드 핀 GUID와 생성된 함수 그래프6개의 GUID다.
- 최종 `cleanup_result.json`에 빌드·저장·삭제·현재 에셋 해시·사용자 작성 내용 비교를 기록했다. cleanup_apply_C/cleanup_verify_A는 종료0, Python/Blueprint Error·Fatal·ensure 검출0이다. 초기 A의 enum 변환과 B의 보호된 Animations 조회 오류는 모두 저장·삭제 전에 중단됐으며 근거를 보존했다.
- 일회성 `postbuild_cleanup.py`는 실행 참조 확인 후 삭제했다. 기존 `common_card.py`도 삭제 상태이며 로그·JSON·엔진 내보내기만 보존한다.

### 공통 UMG 제작 절차 — 사용자 완료, 이후 편집 참고용

1. **Content Browser → `/Game/DreamCatcher/UI/Frontend/W_DC_ModeCard` 열기.** 공통 카드에서만 편집한다. 기존 Story/Free/Challenge 카드3개와 native Hover Scale 속성은 정리됐으며, 위 공통 배치 값은 적용돼 있다.
2. **Designer → Animations → + Animation → `HoverZoom`.** ArtworkImage의 Transform 트랙에서 **Scale X/Y**에0초=(1,1),0.15초=(1.03,1.03) 키를 넣고 재생 범위 끝도0.15초로 맞춘다. 프레임 단위로 편집한다면60fps에서9프레임이다. 부드러운 출발/정지를 원하면 Cubic 보간·수평 접선을 사용한다. **Translation·Shear·Pivot은 고정 설정을 유지한다.** 별도 패널·변수·Tick은 만들지 않는다.
3. **Graph → On Mouse Enter → Play Animation Forward**, **On Mouse Leave → Play Animation Reverse**로 연결한다. 두 노드의 In Animation=`HoverZoom`, Playback Speed=1, Restore State=false다. Enter는 `Get Is Enabled`·`Is Interaction Enabled`가 참이고 Mouse Event의 `Is Touch Event`가 거짓일 때 재생한다. Leave도 터치 이벤트는 제외한다. 키보드 포커스만으로 확대되지 않도록 CommonUI On Hovered 대신 실제 마우스 이벤트를 쓴다.
4. 같은 Graph에서 공통 복원 이벤트 하나를 만들어 **Stop Animation(HoverZoom) → ArtworkImage의 Set Render Scale(1,1)**을 연결하고, On Disabled·Construct·Destruct에서 호출한다. 고정 Translation을0으로 바꾸면 그림이 이동하므로 **Scale만 복원한다.** 이것은 재사용·비활성화 복원이고 별도 보간 코드가 아니다.
5. **Compile·Save.** 이어 `/Game/DreamCatcher/UI/Frontend/W_DC_Frontend`도 Compile·Save한다. 공통 카드의 한 애니메이션이 세 인스턴스에 적용된다.
6. **`/Game/DreamCatcher/Maps/L_DC_FrontEnd` → PIE.** 세 카드의 표시, 마우스 확대·복귀와 빠른 왕복, 키보드 이동/확인, Story 진입과 Free/Challenge 안내, 화면 재진입 후 복원을 확인한다. 이 제작·정리 작업의 사용자 확인은 아래 최종 수용 기록으로 완료됐으며 반복할 필요는 없다.

**분담 이유:** 반복되는 공통 위젯/세 인스턴스 설정·교체는 기존 UMGToolSet으로 자동화했다. 작은 애니메이션과 이벤트 연결은 Editor에서 한 번 직접 작업하는 편이 효율적이다. 기존 도구에 애니메이션 생성 API가 없고 이전 메모리 조사에서 Python의 WidgetAnimation.MovieScene 설정이 보호 속성으로 거부됐으므로 신규 제작용 C++나 우회 도구를 추가하지 않는다. 확대·복귀는 사용자가 선택한 UMG Forward/Reverse 기본 동작이며 HoverProgress·native setter·방향 전환마다0.15초 재보간은 추가하지 않는다.

### 최종 사용자 수용 — C++ 제거·옛 카드 삭제 이후에도 정상

**2026-10-29 사용자가 “정리 후에도 모두 정상”이라고 확인했다.** 이는 기존 확대 C++와 옛 카드3개를 제거한 새 DLL 기준으로 `/Game/DreamCatcher/Maps/L_DC_FrontEnd` PIE의 확대·복귀·빠른 진입/이탈·클릭·화면 재진입 후 기본 크기 복원을 확인한 후속 수용이다. 앞선 UMG 제작 당시 수용과 구분하며, 공통 카드 통합·UMG 이관·기존 코드/에셋/일회성 도구 정리를 이 확인까지 포함해 완료한다.

추가 빌드·재제작·완료 검사 반복은 필요 없다. cleanup_result.json의 `user_PIE_after_native_cleanup=false`는 자동 단계 당시 기록이며, 이후 사용자 확인은 이 문서와 AGENTS·기능 명세에 반영했다. 원본 이미지3개와 CommonUI 클릭 계약은 유지한다. 사용자 수용을 Codex의 자동 화면 검사나 패키징 확인으로 확대하지 않는다. Codex는 C++ 빌드·패키징·커밋·푸시를 실행하지 않았다. 다른 UI 이관과 프로젝트의 남은 정리는 후속 범위다.

## 0.148 사용자 PIE 수용까지 완료: HUD Designer 조정 안내

**2026-10-28 Designer 제작·등록·재로드·기존 검사와 사용자 PIE 수용까지 완료.** C++ 연결부 빌드 이후 사용자가 반복 제작의 자동화를 승인했다. 설치된 UMGToolSet으로 위젯5개(기존1·신규4), ActionSet1개와 GameMode4개를 저장했다. 추가 C++·Config 수정이나 빌드는 없다. **아래1~6을 다시 제작할 필요는 없으며, 저장된 에셋의 배치·스타일 조정 참고로 사용한다.**

- C++20파일(기존 검사3개 포함)과 기존 Python 검사4개를 조정했다. 숨겨진 StageDisplay는 제거했고 보스/미션/결과 내용은 명명된 Designer 위젯을 사용한다. 보스 HUDStyle은 제거하고 미션 HUDStyle에는 시간2개와 EntryWidgetClass만 남겼다.
- 미션 행 높이는 저장된 Blueprint의 실제 폰트·텍스트 속성·여백·아이콘 크기를 사용한다. 페이지당 transient 측정 TextBlock을 재사용하며 저장 템플릿을 변경하지 않는다. 목록·페이지 트리는 제거/재생성하지 않고 기존 진행·전환·취소 로직을 보존한다.
- 결과 CommonUI 호스트는 기존 GameMode.ResultWidgetClass 설정을 사용한다. 아래4개 GameMode를 W_DC_StageResult에 연결했고 LAS_DC_StageHUD의 기존 HUD.Slot.ModeStatus 항목을 W_DC_StageOverlay로 교체했다. 별도 결과 관리자·결과 레이어 BP는 없다.
- 최초 빌드의 C4458 Padding/Slot 이름 충돌5곳을 고쳤으며 사용자 재빌드는 **Succeeded /9.94초**, DLL **00:24:29 KST /6,898,688 bytes**다. 후속 제작은 이 DLL을 사용했다. 기존 보스2·미션6·메뉴 입력 소유권1 검사가 모두 통과했고, 이후 사용자 실제 PIE 수용도 완료됐다.
- **저장·검증 근거:** `Saved/Diagnostics/HUDDesigner20261028/apply.json`의10개 저장과 `verify.json`의 저장0·트리/속성/참조 일치, `Saved/Automation/HUDDesigner20261028/index.json`의 성공9·실패/테스트 경고0. 미션 DataAsset4개는 저장하지 않았고 기존 EntryWidgetClass·완료0.6/전환0.4초·파일 해시를 보존했다.
- apply_E는 기존 MissionEntry를 채우는 중의 필수 바인딩 오류로 종료1이나 최종 Compile·저장은 완료됐다. 새 프로세스 verify_B와 native_A는 종료0, Error/Fatal/ensure0이다. 초기 C/D 실행은 Python의 설정 enum/내보내기 API 노출 문제로 저장 전에 중단됐고, verify_A는 tuple/JSON list 비교 오류만 교정했다. 실제 JSON 구조·값은 동일했다. 전체 근거는 `Saved/Diagnostics/HUDDesigner20261028/final_result.json`에 있다.
- 일회성 `apply_hud.py`는 검증 및 실행 참조 확인 후 삭제했다. 로그·JSON·설정 내보내기는 근거로 보존한다. 이 도구를 게임 실행에 요구하지 않는다.

### 공통 작업 방법

**2026-10-28 사용자 승인 분담 보완:** 아래1~6은 제작 순서·위젯 계약과 수동 작업 절차다. 반복되는 트리 생성·정해진 속성 입력은 기존 도구·엔진 API를 활용한 제작 자동화 후보로 두고 전체 비용을 비교한다. 화면을 보며 배치·스타일을 조정하는 부분은 사용자가 Editor에서 직접 작업한다. 해당 자동화를 위해 새 제작용 C++ 작성과 여러 차례 사용자 빌드가 필요해지면 수동 Editor 작업을 선택한다. 기존의 전체 수동 제작·새 제작 스크립트 일괄 제외 안내는 이 기준으로 대체한다. 자동 제작 결과도 저장된 Widget Blueprint로 남아 Designer에서 수정할 수 있어야 한다. 일회성 도구를 사용한다면 기존 정책에 따라 검증·정리까지 범위에 포함한다.

위 분담 기준에 따라 반복 제작·등록·기존 검사와 아래7의 사용자 화면 확인까지 완료했다. 이후 배치·스타일은 저장된 Widget Blueprint의 Designer에서 직접 수정한다.

수동 작업은 Editor를 새로 열고 PIE를 종료한 상태에서 아래1~6을 진행한다. 제작 중에는 필수 BindWidget 누락 메시지가 나올 수 있으며, 해당 위젯을 모두 만든 뒤 Compile한다. **이름은 대소문자까지 그대로 사용하고, 아래 명명된 위젯의 Is Variable을 켠다.** 임의의 추가 변수나 Event Graph의 클릭/진행 로직을 만들 필요는 없다. 각 Widget Blueprint는 제작 후 Compile·Save한다.

신규 Widget Blueprint는 Content Browser의 `/Game/DreamCatcher/UI/HUD`에서 생성하고 부모 클래스를 아래 표대로 지정한다. 생성 창에서 부모를 선택하지 않았다면 Class Settings의 Parent Class에서 지정한다.

| 위젯 | 작업 | 부모 클래스 |
|---|---|---|
| W_DC_MissionEntry | 기존 파일 편집, 부모 유지 | DCMissionEntryWidget |
| W_DC_BossHealth | 신규 | DCBossHealthWidget |
| W_DC_MissionObjective | 신규 | DCMissionObjectiveWidget |
| W_DC_StageResult | 신규 | DCStageResultWidget |
| W_DC_StageOverlay | 신규 | DCStageOverlayWidget |

### 1. 기존 W_DC_MissionEntry 제작

`/Game/DreamCatcher/UI/HUD/W_DC_MissionEntry`를 열고 Designer에서 아래 트리를 만든다. 루트는 Canvas가 아닌 SizeBox다. RowContents 이름은 임의로 정해도 되며 나머지4개 이름은 필수다.

```text
MissionRow                     SizeBox
└─ RowContents                 HorizontalBox
   ├─ MissionCheckboxBox       SizeBox
   │  └─ MissionCheckbox       Image
   └─ MissionText              TextBlock
```

| 선택 대상 | Details 설정 |
|---|---|
| MissionRow | Min Desired Height 체크·40. 고정 Height Override는 끔 |
| MissionCheckboxBox | Width Override20, Height Override20. HorizontalBox Slot의 Vertical Alignment=Center, Padding Right15·나머지0 |
| MissionCheckbox | Brush Image=`/Game/DreamCatcher/Screens/InGame_UI/Check_False`, Draw As=Image, Image Size20×20, 흰색 |
| MissionText | HorizontalBox Slot Size=Fill, Vertical Alignment=Center, Slot Padding Top6/Bottom6/Left0/Right0 |
| MissionText 폰트 | `/Game/DreamCatcher/UI/Font/Paperlogy-6SemiBold_Font`, Typeface Default, Size12, 흰색. 기존16픽셀 설정과 대응 |
| MissionText 줄바꿈 | Wrapping Policy=Allow Per Character Wrapping. 텍스트 내부 Margin은0. 런타임 Wrap Text At은 본문 폭에 맞춰 계산 |

Class Defaults → Mission → Appearance → **Completed Brush**의 Image를 `/Game/DreamCatcher/Screens/InGame_UI/Check_True`로 지정하고 Draw As=Image·Image Size20×20·흰색으로 맞춘다. Designer에서 보여줄 MissionText는 임시 문구를 넣어도 되며 실제 내용은 C++가 전달한다. 부모에서 User Object List Entry 인터페이스를 이미 구현하므로 직접 추가하지 않는다. Compile·Save한다.

### 2. W_DC_BossHealth 제작

```text
BossRoot             CanvasPanel
└─ BossPanel         CanvasPanel
   ├─ BossName       TextBlock
   ├─ BossSubtitle   TextBlock
   └─ BossBar        CanvasPanel
      ├─ BossBarBorder  Image
      └─ BossBarFill    Image
```

BossRoot 이름은 자유이며 나머지6개는 필수다. BossPanel 및 내부 텍스트/바의 Canvas Slot Anchors는 상단 중앙(0.5,0), Alignment는(0.5,0)으로 한다.

| 대상 | Position | Size / 표시 |
|---|---|---|
| BossPanel | 0,48 | 720×86 |
| BossName | 0,0 | 720×32, 기본 Regular24, 흰색, 가운데 정렬 |
| BossSubtitle | 0,32 | 720×20, 기본 Regular12, 색상(0.8,0.8,0.8,1), 가운데 정렬 |
| BossBar | 0,62 | 640×18 |

BossBarBorder·BossBarFill은 BossBar 전체 Stretch, 네 Offset0으로 한다. 브러시 경로는 아래와 같다.

- Border: `/Game/DreamCatcher/Lyra/UI/Shared/Equipment/LyraStarterGame/UI/Hud/Art/MI_UI_HealthBar_BarBorder` → Draw As=Box, Image Size24×24, Margin 네 방향0.5, 흰색.
- Fill: 같은 폴더의 `MI_UI_HealthBar_Fill` → Draw As=Box, Image Size12×12, Margin 네 방향0.5, 흰색. Render Transform Pivot=(0,0.5), Scale=(1,1).

이름·부제는 Designer에 미리보기 문구를 넣어도 되지만 실제 게임에서는 Boss Data 값을 사용한다. 배치·폰트·브러시는 C++가 덮어쓰지 않는다. Compile·Save한다.

### 3. W_DC_MissionObjective 제작

Panel 폭과 최소/최대 높이를 Designer에서 관리하도록 **MissionPanelSize SizeBox**를 사용한다. 아래의 DC 타입은 일반 WidgetSwitcher/ListView로 대체하지 않는다.

```text
MissionRoot                      CanvasPanel
└─ MissionPanelSize              SizeBox
   └─ MissionPanel               Border
      └─ MissionContents         VerticalBox
         ├─ MissionHeaderSpace   SizeBox
         └─ MissionBodySwitcher DCMissionTransitionSwitcher
            ├─ MissionBodyPage0 VerticalBox
            │  ├─ MissionStageTitle0 TextBlock
            │  └─ MissionListArea0   SizeBox
            │     └─ MissionList0    DCMissionListView
            └─ MissionBodyPage1 VerticalBox
               ├─ MissionStageTitle1 TextBlock
               └─ MissionListArea1   SizeBox
                  └─ MissionList1    DCMissionListView
```

MissionRoot 외에는 모두 이름·타입·부모 관계가 필수다. 페이지0을 만든 뒤 복제하여 페이지1로 만들고 내부 이름의 끝자리도1로 바꾼다. Switcher 자식 순서는0→1이다.

| 대상 | Details 설정 |
|---|---|
| MissionPanelSize Canvas Slot | Anchors 오른쪽 위(1,0), Alignment(1,0), Position(-64,288), Auto Size 켬 |
| MissionPanelSize | Width Override373, Min Desired Height205, Max Desired Height392 모두 체크. Height Override는 끔 |
| MissionPanel | Padding 네 방향21. Content 가로/세로 Alignment=Fill |
| MissionPanel Brush | Image=`/Game/DreamCatcher/Screens/InGame_UI/Mission_UI`, Draw As=Box, Image Size373×205, Margin L0.343164/T0.234146/R0.042895/B0.078049, 흰색 |
| MissionHeaderSpace | Height Override43 체크, 자식 없음. 이미지에 제목이 들어 있어 추가 MISSION 텍스트를 넣지 않음 |
| MissionStageTitle0/1 | 기본 Regular14·흰색, Visibility=Collapsed, VerticalBox Slot Padding Bottom8·나머지0 |
| MissionListArea0/1 | 미리보기 Width Override331, Height Override120. Content 가로/세로 Alignment=Fill |
| MissionList0/1 | Entry Widget Class=`W_DC_MissionEntry`. 일반 ListView가 아닌 DCMissionListView 사용. FactoryRules에는 DCMissionEntryFactory1개, 그 EntryWidgetForClass 매핑은 DCMissionEntryData→W_DC_MissionEntry. 나머지 입력/스크롤 기본값 유지 |

VerticalBox의 HeaderSpace·Switcher·페이지 내부 슬롯은 Auto 크기를 사용한다. 부모 Border의43높이 공간은 숨겨진 제목의 자리를 유지하기 위한 공간이다. 실제 미션 데이터의 HUDStyle.EntryWidgetClass도 기존 W_DC_MissionEntry를 계속 가리킨다. 런타임은 목록의 폭·높이와 패널 Height Override만 콘텐츠에 맞춰 계산한다. 전체 위치·폭·최소/최대 높이는 MissionPanelSize에서, 여백·브러시는 MissionPanel에서 조절한다. 별도 UMG 전환 애니메이션이나 Event Graph는 만들지 않는다. 기존 CommonUI 전환이 시간0.6/0.4를 사용한다. Compile·Save한다.

### 4. W_DC_StageResult 제작

```text
StageRoot                CanvasPanel
├─ PhasePanel            Border
│  └─ StageTitle         TextBlock
└─ ResultPanel           Border
   └─ ResultWidth        SizeBox
      └─ ResultRows      VerticalBox
         ├─ ResultTitle  TextBlock
         ├─ RetryButton  Button
         │  └─ RetryLabel TextBlock
         └─ ExitButton   Button
            └─ ExitLabel TextBlock
```

필수 이름은 PhasePanel·StageTitle·ResultPanel·ResultTitle·RetryButton·ExitButton6개다. 다른 이름은 자유다. Root Canvas Visibility는 Not Hit-Testable(Self Only)로 하고 Button은 Visible·Is Focusable=true를 유지한다.

| 대상 | 설정 |
|---|---|
| PhasePanel | 상단 중앙 Anchor/Alignment(0.5,0), Position(0,24), Canvas Auto Size 켬. Padding L/R22·T/B12, Brush Color 선형RGBA(0.025,0.04,0.06,0.92) |
| StageTitle | 기본 Regular22·흰색. 미리보기 단계 제목 |
| ResultPanel | 화면 중앙 Anchor/Alignment(0.5,0.5), Position(0,0), Canvas Auto Size 켬. Padding32, Brush Color(0.025,0.04,0.06,0.98) |
| ResultWidth | Width Override360 |
| ResultTitle | 기본 Bold30·흰색·가운데 정렬, VerticalBox Slot Padding Bottom24. 미리보기 문구는 스테이지 실패 |
| RetryButton/ExitButton | Background Color(0.2,0.65,0.7,1). VerticalBox Slot Padding Top/Bottom8 |
| RetryLabel/ExitLabel | 각각 다시 시작 / 메인화면으로. 기본 Regular22·검정·가운데 정렬 |

PhasePanel은 비 Experience 경로에서 쓰므로 삭제하지 않는다. CommonUI 결과 레이어에서는 C++가 숨긴다. 결과 문구·표시 여부·버튼 이벤트·중복 요청 차단은 C++에 있으므로 Event Graph에 재구현하지 않는다. Compile·Save한다.

### 5. W_DC_StageOverlay 제작

루트 CanvasPanel에 W_DC_BossHealth와 W_DC_MissionObjective 두 UserWidget을 배치한다. 이름을 각각 **BossDisplay**, **MissionDisplay**로 바꾸고 Is Variable을 켠다. 두 Canvas Slot을 전체 Stretch·네 Offset0으로 한다. 루트/HUD의 Visibility는 Not Hit-Testable(Self & All Children)를 사용한다. 결과 위젯은 여기에 추가하지 않는다. Compile·Save한다.

### 6. 마지막 등록·클래스 선택

1. `/Game/DreamCatcher/Experiences/ActionSets/LAS_DC_StageHUD` → Actions → Add Widgets → Widgets에서 **Slot ID가 HUD.Slot.ModeStatus인 기존 항목**을 찾는다. 그 Widget Class만 W_DC_StageOverlay로 교체하고 Save한다. 항목을 추가하거나 다른 Slot ID·Layout을 바꾸지 않는다.
2. `/Game/DreamCatcher/Blueprints/Core/`의 BP_DC_StageRunGameMode, BP_DC_Level1GameMode, BP_DC_Level2GameMode, BP_DC_Level3GameMode를 각각 연다. Class Defaults → Stage → UI → **Result Widget Class**를 W_DC_StageResult로 선택/확인하고 Compile·Save한다. 부모에서 상속되더라도 네 클래스의 최종 선택값을 확인한다.
3. 미션 DataAsset4개의 HUDStyle.EntryWidgetClass가 W_DC_MissionEntry인지 확인한다. Completed Hold Duration0.6과 Mission Transition Duration0.4는 기존 값을 유지한다. 목표 정의·게임플레이 수치·미디어 설정은 이 작업에서 변경하지 않는다.

### 7. 제작 후 사용자 확인 완료

사용자가 제작·Compile·Save·연결 완료를 회신한 뒤, 현재 에셋14개가 재로드 검증본과 일치함을 확인해 통과한 검사9개는 반복하지 않았다. 이어 보스 이름·HP 표시, 미션 체크·완료 유지·전환, 실패 결과의 재시작·메인화면 복귀·키보드 포커스에 대해 **“모두 정상 동작함”**이라고 확인했다. 이 이관 묶음은 사용자 수용까지 완료됐다. 추가 빌드·재제작·완료 검사 반복은 필요 없다. 자동 보고서의 PIE 수용=false는 작성 당시 상태이며 이 사용자 확인은 후속 기록이다. 사용자 확인을 Codex의 자동 화면 검사나 패키징 검증으로 확대하지 않는다.

## 후속 정리: Editor 직접 작업 기준과 Saved 일회성 도구5개

사용자는 후속 TODO를1차 승인하고, AGENTS.md·기능 명세·이 인계 문서와 아래5개 삭제를 최종 승인했다. 이 묶음은 완료했다. C++·Config·에셋은 변경하지 않았으며 새 스크립트·검사 체계·빌드·PIE 실행도 없다.

공통 기준은 AGENTS0.4와 `docs/specs/gas-lyra-migration.md`의2026-10-27 작업 절차에 반영했다. 짧은 Editor 조작은 사용자 직접 작업을 우선하고 정확한 경로·메뉴·값·저장·확인 방법을 안내한다. 자동화는 작성부터 실행 준비·디버깅·검증·정리까지의 총 비용으로 선택하며, 긴 인라인 명령과 Saved 임시 스크립트에도 같은 기준을 적용한다. 검증은 변경 범위에 맞추고 완료 검사를 반복하지 않는다.

추가 삭제5개는 모두 `Saved/Diagnostics/` 아래의 일반 파일이다.

| 파일 | 삭제 근거 |
|---|---|
| `PlayerStatusDesigner20261023/active_alpha_trace.py` | 10/21 고정 내보내기 결과를 추적하던 완료된 일회성 조사 |
| `PlayerStatusDesigner20261023/mi_readonly_diagnosis.py` | 같은 과거 내보내기의 특정 노드 조사 |
| `PlayerStatusDesigner20261023/readonly_material_function.py` | 삭제된 `player_status_designer_setup.py`를 불러오는 과거 조사 |
| `ShieldDepleted20261025/inspect.py` | 완료된 쉴드 제작 직전 상태의 고정 경로 내보내기 |
| `ShieldDepleted20261025/materials.py` | 완료된 제작/재로드 도구이며 현재 HP 옵션 변경 이전 상태를 기준으로 함 |

현재 Source·Config·Scripts와 다른 진단 Python에서 위 파일로 들어오는 실행 참조는 찾지 못했다. 각 파일의 내용을 확인하고, 삭제 직전 명시한 경로가 프로젝트의 Saved/Diagnostics 안에 있는 일반 파일임을 확인한 뒤 비재귀 삭제했다. 파일5개 부재와 유지 Python21개를 확인했다. 로그·JSON·내보내기 결과는 그대로다.

재조사 시 Saved 진단 Python은250개였으며 삭제 후245개다. 전부 미사용이라는 뜻이 아니다. 예를 들어 유지 중인 `mission_hud_setup.py`는 `Saved/Diagnostics/Level1Merge20261016/apply_merge.py`를 SHA 근거로 읽는다. 나머지는 실제 참조와 작업 완료 상태에 따라 후속 분류한다.

활성 UI에서는 `DCStageOverlayWidget`의 StageDisplay가 결과 화면 객체를 만들지만 결과 표시 호출은 없고 제목도 매 갱신 숨겨지는 중복을 확인했다. 실제 결과 화면은 StageResultLayer와 GameMode의 별도 경로에서 사용되므로 `DCStageResultWidget` 클래스 자체를 삭제하지 않는다. 이 중복 제거는 보스·미션·결과 UI 이관의 연결 변경과 묶어 빌드 왕복을 줄일 대상으로 남겼다. 레이더 투영·GAS·미디어 수명과 필요한 원본 Slate 구현도 단순 UI 배치와 구분한다.

## 후속 요청: HP 감소 구간을 회색으로 유지

**단일 머티리얼 옵션 적용·재로드 및 사용자 PIE 수용까지 완료.** HP·아머 Designer 단일화의 사용자 수용 이후, 사용자는 HP의 감소한 빈 구간도 현재 쉴드처럼 회색으로 남도록 요청했다. 1차·최종 승인을 받았으며, 최종 동작은 잠시 후 없어지는 잔상이 아니라 **빈 구간을 회색으로 계속 유지하고 HP 회복 시 다시 채우기**다.

- 대상: `/Game/DreamCatcher/UI/HUD/Art/MI_DC_HUD_HealthArt_Fill` 한 에셋.
- 변경: 기존 static switch `ShowDepletedShield`만 false→true. 기존 공통 회색 `ShieldDepletedColor=(0.18,0.18,0.18,1)`과 애니메이션을 사용한다.
- 근거: 현재 쉴드3개는 해당 옵션이 true, HP는 false였다. Unreal API의 현재 값·참조 조회와 기존 적용 근거/현재 해시로 확인했다. HP의 BarFill이 이 인스턴스를 사용하며, BarBorder·BarGlow는 현재 NoDrawType이다.
- 검증: apply B 저장1·새 프로세스 verify A 저장0·모두 종료0. 전체 HP 파라미터에서 이 옵션만 달라졌고 쉴드3개의 전체 조회 파라미터 및 다른 Content4,961개 파일의 크기/수정시각이 보존됐다. 결과는 `Saved/Diagnostics/HPDepleted20261027/result.json`, 로그는 `Saved/Logs/HPDepletedApply20261027_B.log`·`HPDepletedVerify20261027_A.log`다.
- 저장본 SHA256: `a8c4c4106b39055ea8fa4e642329539829fb37fb16ef4bf504c585b75c439889`.
- 초기 apply A는 API setter가 False를 반환해 저장 전에 중단했다. 후속 실행에서 반환값과 달리 조회 값은 true임을 확인했고, 실제 값·다른 파라미터 보존·재로드 결과로 판정했다. 최종 apply/verify의 Error/ensure/Fatal은0이며 기존 LyraGame import 종료 경고는 남는다.

새 스크립트 파일·C++·Config·위젯/마스터 머티리얼 그래프 변경은 없다. 후속 PIE 확인에서 사용자가 **“회색 영역과 회복 표시 모두 정상”**이라고 답했다. 이 표시 변경도 사용자 수용까지 완료됐으며 추가 빌드·완료 검사 반복은 필요 없다. 사용자 화면 확인과 자동 옵션/보존 검사는 구분한다.

## 후속 승인: HP·아머 UI Designer 단일화

**코드·에셋 적용, 자동 검증 및 사용자 PIE 수용까지 완료.** 사용자 Development Editor·Win64 빌드는 40.54초에 성공했고 DLL은 2026-10-27 21:46:59 KST / 6,909,440 bytes다. 사용자가 실제 배치·수치·피해/회복 애니메이션이 기존과 같고 정상 동작함을 확인했다. 추가 빌드·완료 검사 반복은 필요 없다.

사용자는 다음 범위를 최종 승인했다.

- `Source/DreamCatcher/UI/Player/DCPlayerStatusWidget.h/.cpp`의 native 자동 생성·배치 경로와 전용 설정 제거.
- `Source/DreamCatcher/Tests/DCPlayerStatusDesignerAutomationTests.cpp`의 기존 검사5개를 새 구조에 맞춰 유지.
- 사용자 빌드 후 `/Game/DreamCatcher/UI/HUD/W_DC_PlayerStatus` 한 에셋만 Compile·재저장·재로드하고 배치·브러시·남은 기본값을 확인.
- 이 인계 문서와 AGENTS.md·gas-lyra-migration.md 갱신. 새 스크립트·검사 프레임워크 추가 없음.

실제 저장본은 이미 `bUseDesignerLayout=true`였고, 이번 조회에서 기존 `PlayerStatusDesigner20261023/Visual_G_verify/report.json`의 status SHA256 `296a4ccbadcb5030463bfaa0d09af14093b080b38cc025e89c8fcb458ce1b9e5`와 일치했다. 기존 레지스트리 조사에서 해당 native 부모의 저장된 Widget Blueprint는 이 에셋 하나였다. 저장된 트리와 원본 W_Healthbar를 그대로 사용하는 전환이다.

W_Healthbar의 현재 보호 기준 SHA256은 `bfc65fa56d33d7052c7bb2acd7cf1d789a4e013148b1a5ab5343c617382d8cf4`이며 Git HEAD의 LFS oid와 일치한다. 위 과거 보고서의 health 해시는 현재 저장본과 다르므로 후속 검사의 기준으로 재사용하지 않는다. 이번 단계에서 W_Healthbar를 수정하거나 과거 상태로 복원하지 않았다.

작성 내용은 native 트리 생성·ApplyLayout·동적 HP 자식 생성 제거, Designer 스위치 제거, 배치·폰트·브러시 설정19개 제거다. StatusStyle에는 동적 상태색상4개·glow2개·갱신 주기1개만 남긴다. HP·최대HP·아머 갱신, 아머 애니메이션의 타이밍과 빠른 값 변경, 시그니처 상태·쿨다운, Pawn·Controller 연결/해제·월드 종료 처리는 보존했다. 코드3개 파일 순감소197줄이며 diff 검토·공백 검사를 통과했다.

기존 검사5개는 AuthoredLayoutAndPreview, AuthoredChildAndBrushLifecycle, IncompleteTreePreserved, ArmorAnimationTiming, ArmorAnimationRetargetAndReset이다. 기존 `-DCPlayerStatusDesignerAutomation` 조건으로 실행하며 새 검사 수를 늘리지 않는다. 빌드 후 에셋 검사에서도 원본 W_Healthbar와 재질을 저장하지 않는다. 사용자 화면 확인은 자동 검사와 별도로 남긴다.

2026-10-27 검증 결과:

- 기존 Designer 검사 **성공5·실패0·테스트경고0**, 프로세스 종료0. `Saved/Automation/PlayerStatusDesignerOnly_20261027_A/index.json`과 `Saved/Logs/PlayerStatusDesignerOnlyNative20261027_A.log`가 근거다. 실제 PIE를 실행한 검사가 아니다.
- 최초 apply A는 인라인 명령의 `not in` 문법 누락으로 실행 전 중단됐으며 저장0이다. 문법 AST 확인 후 apply B가 성공했다. 새 영구 스크립트를 추가하지 않고 보존 중인 `ingame_hud_layout_setup.py`의 내보내기·프로세스 확인·보존 검사 함수를 재사용했다.
- `W_DC_PlayerStatus`를 메모리 Compile한 전후의 전체 WidgetTree·CDO와 위젯18개 이름이 같음을 확인한 뒤 이 에셋만 저장했다. 다른 Content4,961개 파일의 크기·수정시각이 같고 W_Healthbar 해시도 보존됐다.
- 새 프로세스 verify A에서 재로드 및 메모리 Compile 후에도 트리·기본값이 같았다. apply 저장1·verify 저장0, 두 프로세스 종료0·Error/ensure/Fatal0이다. 종료 중 기존 B_LyraGameInstance의 LyraGame import 경고는 남는다.
- 최종 status SHA256은 `377c29b582f0f85172567f88543f9a1783e778a7f26f484d1e279c718ba915b8`이다. 결과는 `Saved/Diagnostics/PlayerStatusDesignerOnly20261027/result.json`, 에셋 로그는 `Saved/Logs/PlayerStatusDesignerOnlyApply20261027_B.log`와 `PlayerStatusDesignerOnlyVerify20261027_A.log`다.

후속 사용자 확인: PIE에서 HP·아머 배치와 수치, 피해·회복 애니메이션이 **“기존과 같고 정상 동작함”**이라고 답했다. 이 항목의 실제 화면 수용도 완료됐다. 사용자 확인을 Codex의 추가 자동 PIE 검사로 표현하지 않으며 패키징 검증으로 확대하지 않는다.

## 삭제한 Python 31개

아래 이름은 모두 `Scripts/Editor/` 기준이다. 완료된 제작·수정 레시피 27개와 구형/미완료 관찰 도구 4개다.

```text
damage_feedback_assets.py
damage_feedback_metasound_link.py
fall_death_setup.py
frontend_ammo_guard.py
frontend_background_tiling_setup.py
frontend_intro_setup.py
frontend_menu_probe.py
frontend_menu_setup.py
frontend_mode_art_setup.py
frontend_music_setup.py
hit_zone_physics_setup.py
ingame_hud_art_setup.py
inspector_bot_death_setup.py
inspector_bot_presentation_setup.py
inspector_bot_setup.py
level1_gameplay_probe.py
mission_panel_art_setup.py
mode_card_hover_probe.py
pistol_audio_reference_repair.py
pistol_fire_input_setup.py
pistol_slot_setup.py
player_status_designer_setup.py
player_status_hud_setup.py
radar_setup.py
reticle_layout_capture.py
ui_screen_texture_setup.py
weapon_ammo_hud_setup.py
weapon_ammo_off_style_setup.py
weapon_ammo_richtext_setup.py
weapon_panel_size_setup.py
weapon_screens_hud_setup.py
```

검사 4개의 근거:

- `frontend_menu_probe.py`: 이미 제거된 SettingsButton/QuitButton을 찾는 과거 메뉴 검사.
- `level1_gameplay_probe.py`: GameDefaultMap과 MapsToCook가 Level_1 하나라는 현재와 다른 전제.
- `mode_card_hover_probe.py`: 자동 프레임 관찰은 미완료이며, 세 카드의 실제 확대·복귀는 사용자가 직접 확인했다.
- `reticle_layout_capture.py`: 마지막 기록이 `captured_with_missing_or_invalid_geometry`다. FGeometry를 Python으로 넘기지 않는 기존 native 캡처는 유지했다.

추적 캐시 `Scripts/Editor/__pycache__/base_damage_setup.cpython-311.pyc`도 삭제했다. 원본 `base_damage_setup.py`는 유지한다.

## 유지한 Python 21개

재사용 검사 후보 13개:

```text
base_damage_gameplay_probe.py
bigbrother_boss_gameplay_probe.py
boss_hud_gameplay_probe.py
frontend_intro_probe.py
frontend_music_probe.py
hit_zone_gameplay_probe.py
level_progression_gameplay_probe.py
mission_hud_gameplay_probe.py
player_status_hud_gameplay_probe.py
radar_gameplay_probe.py
spawner_proximity_probe.py
spawner_registration_probe.py
weapon_ammo_hud_gameplay_probe.py
```

이들의 import·파일명 참조를 보수적으로 따라가 남긴 지원 파일 8개:

```text
base_damage_setup.py
bigbrother_boss_setup.py
boss_hud_setup.py
ingame_hud_layout_setup.py
level_progression_setup.py
mission_hud_setup.py
spawner_proximity_setup.py
spawner_registration_setup.py
```

이름이 `setup`이라는 이유만으로 지원 파일을 삭제하지 않는다. 유지한 파일의 생성/apply 모드를 현재 에셋에 다시 실행하라는 의미도 아니다. 해당 검사에 필요한 함수·기준 데이터를 제공하는 참조가 남아 있다.

## C++ 정리

삭제한 파일은 `Source/DreamCatcher/Editor/`의 다음 4개다.

- `DCHaruDashMontageRepair.cpp`
- `DCHaruLyraRigRepair.cpp`
- `DCHaruRifleRigRepair.cpp`
- `DCInspectorBotAssetSetup.cpp`

이들은 `WITH_EDITOR`의 명시적 명령줄·콘솔 명령으로만 동작하는 제작 보조 코드이며 UCLASS/에셋 부모 타입을 제공하지 않는다. HARU·Inspector의 저장된 리그·애니메이션·무기·봇 에셋은 보존한다.

Build.cs에서 제거한 Editor 의존성은 BlueprintGraph, SkeletalMeshModifiers, ControlRig, ControlRigDeveloper, RigVMDeveloper, RigVM이다. `.uproject`의 플러그인 등록과 런타임 모듈 의존성은 변경하지 않았다.

모드 카드에서는 ModeNumber/ModeLabel/ModeTitle/ModeDescription, AccentColor, CardOutline와 텍스트 바인딩 4개 및 RefreshHighlight/관련 focus·hover override만 제거했다. 세 실제 카드의 트리는 CardContent/ArtworkImage만 가지고 있고, 그림 안에 문구와 테두리가 포함돼 있다. 그림 표시·마우스 확대·클릭 영역 보정은 유지한다.

## 읽기 전용 조사와 검증

- 엔진 레지스트리 조회: `/Game` 에셋4,958개, Widget Blueprint84개. Editor Utility/Python 에셋0개, Python 시작 스크립트0개. 명시적 Compile/에셋 저장0.
- `W_DC_PlayerStatus.bUseDesignerLayout=true` 확인. 모드 카드 native 부모를 사용하는 저장된 위젯은 Story/Free/Challenge 세 개다.
- 근거: `Saved/Logs/CleanupReadOnlyAudit20261027_A.log` 및 이전 기능별 적용/재로드 결과. 과거 인계 문서의 퇴역 파일 경로는 당시 작업 이력으로 보존한다.
- 삭제 직전 36개 파일을 검토 당시 SHA256과 대조하고, 실제 삭제 경로가 프로젝트 내부 파일인지 확인했다.
- 남은 Python21개의 구문과 검토 당시 SHA256 불변을 확인했고, 보존할 검사에서 참조하는 파일을 포함해 유지 목록을 정했다. 기존 Content4,962개 파일의 크기·수정 시각은 이전 호버 검사 기준과 같았다.
- 사용자 빌드 후 `Saved/Logs/CleanupCardsVerify20261027_A.log`에서 카드3개 로드·메모리 Compile, native 부모, CardContent/ArtworkImage 두 위젯, 1.03배·0.15초 기본값을 확인했다. 저장0개·카드 파일 해시 불변·종료0이며 Error/Fatal/ensure는 없었다. 종료 중 기존 B_LyraGameInstance의 LyraGame import 경고는 남는다.
- 이번 정리 후 실제 PIE·패키징이나 전체 C++ 테스트 재실행을 완료했다고 주장하지 않는다. 호버 동작은 정리 전 사용자가 직접 확인했고 해당 동작 본문은 보존했다.

## 다음 단계: Editor에서 관리할 UI

사용자는 불필요하게 코드로 만든 표현을 Editor 작업으로 교체하도록 요청했다. 아래 활성 구현은 이번에 삭제하지 않았으며, 현재 동작과 관련 참조를 확인해 이관한다.

| 대상 | 확인한 상태 | 이관 방향 |
|---|---|---|
| 플레이어 HP/아머 UI | Designer 단일화 코드·빌드·기존 검사5개·단일 에셋 재저장/재로드·사용자 PIE 수용 완료 | 완료, 재검사 불필요 |
| 보스·미션·결과 UI | 중복 StageDisplay 제거·Designer 제작/연결·기존 검사9개·사용자 PIE 수용 완료 | 완료. 이후 배치·스타일은 Designer에서 조정 |
| 모드 카드 확대 | 공통 C++가 사선 보정과 클릭 영역을 유지하며 동작, 사용자 확인 완료 | Editor 애니메이션으로 옮길 때 그림 중심·클릭 영역·빠른 왕복·비활성화 원복을 동일하게 보존 |
| PV/클리어 영상 | native 생성 화면과 미디어·입력·오디오 수명이 연결됨 | 화면 표현을 이관하되 재생/실패/종료·입력 해제 처리를 먼저 구분 |

`DCLyraRuntimeDiagnostics`는 로딩 화면·오디오 믹서·플랫폼 에뮬레이션 실행 여부를 제어한다. 삭제하면 기능이 활성화되는 변경이므로 잔여 코드로 단순 삭제하지 않는다. GAS, 피해·사망, 장비, 미션 상태와 같은 필요한 런타임 로직도 유지한다.

작은 UI 설정·배치·애니메이션은 Editor 기능을 우선한다. 반복되는 대량 작업에 자동화를 사용하더라도, 한 번의 변경마다 장기 보관용 스크립트·검증 프레임워크를 새로 추가하지 않는다.
