# JVideo Studio

**단일 HTML 파일로 동작하는 픽셀아트/영상 편집기 + `.jv` 텍스트 압축 포맷.**
외부 라이브러리·CDN·서버·설치 없이, 브라우저 하나로 그리고, 레이어로 편집하고,
MP4를 프레임으로 뽑아 오고, 최대한 압축된 텍스트 파일로 저장한다.

```
jvideo_studio.html   ← 이 파일 하나가 전부 (HTML+CSS+JS, 오프라인 동작)
```

---

## 목차

1. [설계 원리](#설계-원리)
2. [주요 기능](#주요-기능)
3. [빠른 시작](#빠른-시작)
4. [단축키](#단축키)
5. [아키텍처](#아키텍처)
6. [`.jv` 포맷 개요](#jv-포맷-개요) (전체 그래머는 [`JV_Format_Spec.md`](./JV_Format_Spec.md) 참고)
7. [압축 파이프라인](#압축-파이프라인)
8. [알려진 한계 & 로드맵](#알려진-한계--로드맵)
9. [기여 방법](#기여-방법)

---

## 설계 원리

| 원칙 | 의미 |
|---|---|
| **단일 파일** | HTML 하나에 CSS/JS 전부 포함. `npm install` 없음, 빌드 스텝 없음. |
| **오프라인** | 인터넷 연결 없이도 편집·재생·저장 전부 가능 (MP4 가져오기 포함 — 로컬 파일만 사용). |
| **압축률 우선** | `.jv`는 사람이 읽을 수 있게 설계됐지만, 필요하면 가독성보다 압축률을 우선한다 (v2가 그 방향 전환점). |
| **텍스트 기반 · 결정론적 문법** | Git으로 diff 가능, AI가 생성/파싱하기 쉬운 고정 문법. |
| **레이어는 편집 전용** | 레이어 구성(개수/순서/이름/가시성)은 저장 시 항상 합성(flatten)되어 사라진다. 편집 세션에만 존재하는 개념. |
| **확장 가능한 압축 파이프라인** | 각 압축기(SameFrame/Diff/ObjectMove/Delta/RLE/…)는 독립된 함수. 새 압축기를 끼워 넣기 쉽게 짜여 있다. |
| **팔레트 상한(7색)** | 데이터 모델 자체의 제약. 사진/영상 소스는 k-means로 이 안에 양자화된다. |

## 주요 기능

- **픽셀아트 에디터**: 브러시, 지우개, 사각형, 원, 채우기, 선, 스포이드, 선택(드래그) 툴, 줌/팬
- **레이어**: 추가/삭제/순서변경/가시성 토글, 저장 시 자동 flatten
- **팔레트**: 이비스페인트 스타일 원형 색상 휠(HSV) + 최대 7색 제한, 열기(Open) 시 항상 초기화
- **프레임 관리**: 추가/복제/삭제/순서변경, 씬(scene) 단위 애니메이션 작업
- **MP4 가져오기**: 영상 프레임 추출 → k-means 색상 양자화(7색) → 최근접 매핑으로 픽셀 프레임 생성
- **재생 모드**: 편집기와 완전히 분리된 뷰어. `.jv` 파일 하나만 골라 즉시 순환 재생
- **압축 분석 패널**: 원본 대비 실제 압축률을 프로젝트 단위로 즉시 확인
- **선택/클립보드**: 드래그 선택 + 복사·붙여넣기·이동
- **단일 파일 저장/열기/내보내기/가져오기**: 전부 `.jv` 텍스트

## 빠른 시작

1. `jvideo_studio.html`을 브라우저(Chrome/Edge 최신)로 연다.
2. 상단에서 캔버스 크기(W/H)·FPS·최대 프레임 수를 정하고 **적용**.
3. 좌측 **팔레트**에서 색을 고르고(또는 색상 휠 클릭), 중앙 캔버스에 그린다.
4. 좌측 **프레임 목록**에서 `+ 추가`로 다음 프레임을 만들고 계속 그린다.
5. 하단 재생바로 미리보기, **압축 분석 실행**으로 결과 확인.
6. 완성되면 **저장/내보내기**로 `.jv` 파일을 내려받는다.
7. 사진/영상이 있다면 **MP4 가져오기**로 자동 변환도 가능 (팔레트가 7색으로 양자화됨에 유의).
8. 만든 파일을 그냥 재생만 하고 싶다면 **재생 모드** → 파일 선택.

## 단축키

| 단축키 | 동작 |
|---|---|
| `Ctrl+C` | 선택 영역 복사 |
| `Ctrl+V` | 붙여넣기 |
| `Ctrl+X` | 재생/일시정지 토글 |
| `Ctrl+S` (누르는 동안) | 선택 영역 안에서 드래그 시 내용 이동 |
| `Ctrl+Q` + 숫자키 | 두 키를 함께 누른 채 숫자키 입력 → 그 번호 프레임으로 이동 |
| `Esc` | 선택 해제 |

## 아키텍처

```
Palette ─┐
Layer ───┼─▶ Frame ──▶ Project ──▶ CanvasEditor ──▶ (DOM: mainCanvas)
         │              │              │
         │              │              └─ 브러시/도형/채우기/선택 툴, 줌/팬
         │              └─ addFrame/duplicateFrame/deleteFrame/resizeAll
         └─ 최대 7색, flatten() 시 아래→위 합성

JVFile.serialize/parse ──▶ encodeFrame/decodeFrame
                             ├─ encodeFullFrame/decodeFullFrame   (RLE·TransparentSkip·LineRepeat·Dictionary)
                             └─ encodeDiffFrame/decodeDiffFrame   (ObjectMove·Run-length Delta)

PlaybackController        : 편집 중인 Project를 재생 (재생바)
PlaybackModeController     : Project와 무관하게 .jv 텍스트만 읽어 재생 (재생 모드, 완전 분리)
kMeansPalette/extractVideoFrames : MP4 가져오기 (video+canvas → 색상 양자화 → 인덱스 프레임)
UIController               : 위 전부를 DOM에 배선
```

핵심 클래스는 파일 하나 안에 이 순서로 정의되어 있다 (검색 시 참고):

`Logger` → `JVReader` → (FULL/DIFF 인코더·디코더 함수들) → `Palette` → `Layer` →
`Frame` → `Project` → `CanvasEditor` → `PlaybackController` → `PlaybackModeController` →
(MP4/색상 유틸 함수들) → `UIController`

## `.jv` 포맷 개요

- 텍스트 기반, `0-9`, `A-F`, 그리고 정해진 기호(`~ , . ( ) & % # @ ? ^ * $ ! + -`)만 사용
- 구조: `#2~<width,height,fps,maxFrames>~<팔레트>~<프레임1>~<프레임2>~...`
- 팔레트: 최대 7색, 인덱스 0은 항상 투명(테이블에 없음)
- 프레임 종류(첫 글자로 구분): `#` SAME · `&` FULL · `%` DIFF — 매 프레임마다 FULL/DIFF 중 더 작은 쪽을 자동 선택

전체 문법(모든 오프코드, 왕복 예시, 알려진 한계, 확장 방법)은
**[`JV_Format_Spec.md`](./JV_Format_Spec.md)** 에 상세히 정리되어 있다.

## 압축 파이프라인

```
원본 프레임
   │
   ├─ 직전 프레임과 완전히 같음? ──▶ SameFrame (#)
   │
   └─ 다름 ──▶ FULL 인코딩과 DIFF 인코딩을 둘 다 계산 ──▶ 더 짧은 쪽 채택
                 │                              │
                 ▼                              ▼
        RLE + TransparentSkip           ObjectMove (다중, 연결 성분 기반, 최대 6개)
        LineRepeat / BlockMerge         Run-length Delta (연속 인덱스+동일색 run)
        Dictionary (프레임 내부)
```

- **ObjectMove**: 변경된 픽셀을 4-연결 성분으로 나눠 서로 다른 방향으로 움직이는 물체를
  각각 독립적으로 탐지한다 (경계상자 하나로만 찾으면 다중 이동 탐지가 실패하기 때문).
- **Run-length Delta**: 이동으로 설명 안 되는 나머지 변경 픽셀 중, 인덱스가 연속이고
  색이 같은 구간은 픽셀 하나하나 나열하지 않고 run 하나로 묶는다.
- 변경 영역이 프레임의 절반을 넘으면 비용이 큰 ObjectMove 탐색을 생략한다
  (그런 경우는 대개 FULL이 더 유리해서 압축률 손해가 거의 없다).

## 알려진 한계 & 로드맵

| 항목 | 상태 |
|---|---|
| ObjectMove 최대 6개(연결 성분 기준) | 물체 7개 이상 동시 이동 시 초과분은 픽셀 단위 처리 |
| Dictionary가 프레임 내부 전용 | 프레임 간 공유 사전 없음 (향후 확장 후보) |
| 팔레트 최대 7색 | 데이터 모델 고정값 |
| 회전/스케일 압축 없음 | ObjectMove는 평행이동만 지원 |
| 이비스페인트 색상 휠 | 구현 완료 (원형 hue/sat 휠 + value 슬라이더) |
| 재생 모드 | 구현 완료 (편집기와 완전 분리된 뷰어) |

## 기여 방법

새 압축기, 새 툴, 새 임포터를 추가할 때 지키는 원칙:

1. **재현 후 수정** — 버그는 먼저 최소 재현 케이스로 확인한 뒤 고친다.
2. **생략/자리표시자 금지** — `TODO`, 더미 구현을 남기지 않는다. 못 끝내면 스코프를 줄인다.
3. **Additive 우선** — 기존 동작을 깨지 않는 범위에서 patch 형태로 추가한다.
4. **왕복 검증 필수** — 포맷/압축 관련 변경은 encode→decode 왕복이 원본과 완전히 같은지
   반드시 확인한다 (Node로 순수 로직만 추출해 검증하고, HTML에는 Node 코드를 넣지 않는다).
5. **문법 검증** — `node --check`로 스크립트 문법을, DOM `id` 참조는 HTML과 교차 검증한다.
6. **레이어는 파일에 안 남는다는 원칙 유지** — 새 기능이 레이어 관련이면 `Frame.flatten()`
   결과만 저장 경로에 노출시킨다.
   # JVideo (.jv) 파일 포맷 스펙 v2

JVideo Studio가 사용하는 `.jv` 파일 포맷의 공식 명세다. **v2는 압축률을 최우선으로
설계**했다 — 사람이 눈으로 따라 읽기는 v1보다 어렵지만, 같은 내용을 더 적은 바이트로
표현한다. **v1과 파일 호환되지 않는다** (매직 문자열이 `#1` → `#2`로 바뀜).

---

## 1. 설계 목표

- 압축률 최우선 (읽기 편의성보다 우선순위 높음 — v2 기준)
- 텍스트 기반 (UTF-8, 실제로는 ASCII 서브셋만 사용), Git diff 가능
- 결정론적 문법 (AI가 생성/파싱하기 쉬움)
- 오프라인 단일 파일 편집·재생 (JVideo Studio 자체가 단일 HTML)
- 확장 가능한 압축 파이프라인

## 2. 문자 집합 (Character Set)

파일 내부의 모든 데이터는 아래 문자만 사용한다. 그 외의 알파벳(G-Z 등)은
**절대 나타나지 않는다** — 구조를 나타내는 오프코드도 알파벳 대신 기호를 쓴다.

| 종류 | 문자 |
|---|---|
| 숫자 | `0123456789` |
| 영문(hex 전용) | `ABCDEF` |
| 구조 기호 | `~ , . ( ) & % # @ ? ^ * $ ! + -` |

hex 값은 항상 대문자다. 색상 값, 인덱스, 개수(count), run 길이 등 모든 수치는
16진수로 인코딩된다.

## 3. 전체 파일 구조

```
#2~<헤더>~<팔레트>~<프레임1>~<프레임2>~...~<프레임N>
```

`~` (FRAME_SEP) 로 구분되는 고정 순서의 섹션이다. 프레임 데이터 자체에는 `~`가
등장하지 않으므로 파일 전체를 `~` 기준으로 단순 split 하면 안전하게 파싱된다.

### 3.1 매직/버전

첫 섹션은 `#2` 고정 문자열이다. 파서는 이 값이 일치하지 않으면 오류를 낸다
(예: v1 파일 `#1...`을 v2 파서에 넣으면 즉시 거부됨 — 그래머가 달라 자동 변환 안 됨).

### 3.2 헤더

```
<width>,<height>,<fps>,<maxFrames>
```

4개 필드 모두 hex, `,`(LIST_SEP)로 구분. 예: `20,20,C,C8` = 32×32, 12fps, 최대 200프레임.

### 3.3 팔레트

```
<count>,<color1>,<color2>,...
```

- `count` : 팔레트 색상 개수 (hex). **최대 7 (`PALETTE_MAX`)**.
- 각 `colorN` 은 `RRGGBB` 형식의 6자리 hex.
- 인덱스는 1부터 시작. **인덱스 0은 항상 "투명"으로 예약**되며 테이블에 없다.

### 3.4 프레임

프레임 문자열은 첫 글자로 종류가 결정된다.

| 첫 글자 | 의미 |
|---|---|
| `#` | SAME — 직전 프레임과 완전히 동일 |
| `&` | FULL — 프레임 전체를 명시적으로 기술 |
| `%` | DIFF — 직전 프레임과의 차이만 기술 |

인코더는 프레임마다 FULL과 DIFF를 **둘 다 계산해서 더 짧은 쪽을 선택**한다.
디코더는 첫 글자만 보고 바로 분기한다.

## 4. 압축 파이프라인

### 4.1 SameFrame

직전 프레임과 픽셀이 완전히 같으면 `#` 한 글자로 끝난다.

### 4.2 FULL 프레임 (`&`)

행(row) 단위 인코딩 + 3가지 하위 압축기.

**RLE + TransparentSkip**
- 일반 색상 run: `<colorHex(2)>@<countHex>.` 예: `03@05.`
- 투명 run(더 저렴): `?<countHex>.` 예: `?0A.`

한 행은 `width` 픽셀을 다 채울 때까지 run 토큰을 이어붙인다. 별도 종료 기호 없이
디코더가 채운 픽셀 수를 세면서 읽는다.

**LineRepeat / BlockMerge**
연속된 행이 완전히 같으면, 그 행 내용을 한 번만 쓰고 `^<countHex>.` 로
"다음 N개 행은 방금 행과 동일"을 표시한다.

**Dictionary**
프레임 안에서 어떤 행의 run 토큰 문자열이 비연속적으로 2회 이상 등장하면,
처음 등장 시 `*<runTokens>` (등록), 이후 재등장 시 `$<idxHex>.` (참조)로 대체한다.
사전 인덱스는 등장 순서대로 암묵적으로 부여된다 (별도 저장 없음).

| 마커 | 의미 |
|---|---|
| `=` | 그냥 이 행 그대로 |
| `*` | 사전에 등록하며 그대로 씀 |
| `$N.` | 사전 N번 항목 복사 |
| `^N.` | 직전 행을 N번 반복 |

### 4.3 DIFF 프레임 (`%`) — v2에서 가장 크게 바뀐 부분

```
%(<moveCount>.<move>;...)<groupCount>.<group>,<group>,...
```

#### ObjectMove — 다중 이동 지원 (v2 신규)

v1은 프레임당 이동을 **1개만** 찾았다. v2는 다음과 같이 **여러 개(최대 6개)**를 찾는다:

1. 변경된 픽셀 중 "새로 나타난"(투명이 아닌) 픽셀들을 **4-연결 성분(connected
   component)** 기준으로 서로 다른 덩어리로 분리한다. 이 단계가 핵심이다 —
   성분을 나누지 않고 전체 경계상자 하나로만 찾으면, 서로 다른 방향으로 움직이는
   물체가 둘 이상일 때 어떤 단일 이동 벡터도 맞지 않아 탐지 자체가 실패한다.
2. 성분을 크기순으로 정렬해, 각 성분의 경계상자가 이전 프레임의 다른 위치를
   (dx, dy)만큼 평행이동한 것과 정확히 일치하는지 탐색한다 (탐색 범위 ±32px,
   경계상자가 프레임 면적의 50% 이하일 때만 시도).
3. 성공하면 그 경계상자를 "이동"으로 확정하고 변경 목록에서 제거한 뒤,
   나머지 성분들에 대해 반복한다 (최대 6회 반복).

```
!<destX>,<destY>,<w>,<h>,<sdx>,<sdy>;
```

`sdx`/`sdy` 는 부호 있는 hex(`+`/`-` 접두). `moveCount`가 2 이상이면 이 토큰이
그만큼 이어진다.

#### Run-length Delta — 연속 변경 픽셀 묶기 (v2 신규)

이동으로 설명되지 않는 나머지 변경 픽셀은 인덱스 오름차순으로 정렬한 뒤,
**인덱스가 연속(+1)이고 색이 같은 구간을 하나의 그룹(run)으로 묶는다.**
v1은 이런 구간도 픽셀 하나하나를 나열했다.

```
<idx 또는 부호+델타>[@<runLenHex>].<colorHex(2)>
```

- 첫 그룹: 절대 인덱스로 시작
- 이후 그룹: 직전 그룹의 **마지막** 인덱스 기준 부호+델타
- `@<runLenHex>` 는 runLen이 2 이상일 때만 붙는다 (없으면 길이 1)
- 예: `1E@D.05` = 인덱스 0x1E부터 13(0xD)개 연속 픽셀이 색상 5

## 5. v1 → v2 실측 비교 (예시)

64×64, 10프레임, 서로 다른 방향으로 움직이는 물체 2개 + 가로 막대가 있는 장면:

| 버전 | 특징 | 결과 |
|---|---|---|
| v1 | 이동 1개만 탐지, 나머지는 픽셀 단위 나열 | 물체 2개 중 1개만 이동으로 처리, 나머지+막대는 개별 나열 |
| v2 | 다중 이동 + run-length delta | 두 물체 모두 이동 벡터로, 막대는 run 1개로 처리 |

내부 검증 결과 원본(비압축 1byte/px 가정) 대비 크게 개선됨 — 정확한 수치는
프로그램 내 "압축 분석 실행" 버튼으로 실제 프로젝트 기준 확인 가능.

## 6. 알려진 한계 (v2)

- ObjectMove는 **연결 성분 기준 최대 6개**까지만 탐지한다. 물체가 7개 이상 동시에
  움직이면 초과분은 픽셀 단위로 처리된다.
- 이동 성분이 서로 다른 색을 포함해도 됨(스프라이트 통째로 이동 가능)이지만,
  회전/스케일 변형은 지원하지 않는다(순수 평행이동만).
- Dictionary는 여전히 **프레임 내부에서만** 동작한다 (프레임 간 공유 사전 없음).
- 팔레트는 **최대 7색**으로 고정. 색이 다양한 사진/영상 소스는 k-means 양자화로
  강하게 근사된다.
- 탐색 비용 절감을 위해 변경 영역이 프레임의 50%를 넘으면 이동 탐색 자체를
  건너뛴다 (그런 경우는 대개 FULL이 더 유리하므로 압축률 손해는 거의 없음).

## 7. 확장 방법

새 압축기를 추가하려면:

1. `encode*(...)` / `decode*(...)` 함수 쌍을 순수 함수로 작성한다.
2. 사용할 opcode 기호가 §2 문자 집합 안에 있고 기존 opcode와 겹치지 않는지 확인한다.
3. `encodeFrame`/`decodeFrame` 분기에 새 프레임 타입을 추가하거나, 기존 FULL/DIFF
   내부 하위 단계에 끼워 넣는다.
4. 인코드→디코드 왕복 결과가 원본과 완전히 같은지 반드시 검증한다.

향후 확장 후보(아직 미구현, §6의 한계를 줄이는 방향):
- 프레임 간(cross-frame) 공유 Dictionary
- 이동 성분 개수 상한 확대 또는 동적 조정
- 회전/스케일까지 포함하는 일반화된 Transform 압축기

## 8. 레이어와의 관계

레이어는 **편집 세션에만 존재**하며 `.jv` 파일에는 전혀 기록되지 않는다.
저장/내보내기 시점에는 보이는 레이어를 아래→위로 합성(flatten)한 단일 픽셀
배열만 저장된다. 파일을 다시 열면 항상 단일 레이어로 복원된다.

## 9. 구현 현황

### 완료
| 기능 | 비고 |
|---|---|
| 픽셀아트 편집 (브러시/지우개/사각형/원/채우기/선/스포이드) | 활성 레이어 대상 |
| 레이어 | 편집 전용, 저장 시 flatten |
| 팔레트 최대 7색 + 열기 시 초기화 | `PALETTE_MAX` |
| 레이아웃 버그(색 늘어나면 화면 늘어짐) 수정 | `#right` height 제약 추가 |
| 선택(드래그) 툴 + Ctrl+C/V | 클립보드는 프로젝트 전환 시 초기화(팔레트 인덱스 불일치 방지) |
| Ctrl+S(누르는 동안) 선택영역 이동, Ctrl+X 재생 토글, Ctrl+Q+숫자 프레임 이동 | |
| MP4 가져오기 | video+canvas 프레임 추출 → k-means 팔레트 7색 양자화 |
| 압축 v2 | 다중 ObjectMove(연결 성분 기반, 최대 6개) + Run-length Delta |
| 버그 수정 | 프로젝트 교체/해상도 변경 시 선택영역 잔존 문제 |

### 보류
| 기능 | 상태 |
|---|---|
| 재생 모드 (파일 선택 → 재생 전용 화면) | 미착수 |
| 이비스페인트 스타일 색상 휠 | 미착수 — 현재는 네이티브 `<input type=color>` |
# CODE
```<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>JVideo Studio</title>
<style>
  :root{
    --bg:#0a0b0e;
    --bg-panel:#131519;
    --bg-panel-2:#181b21;
    --bg-raised:#1e222a;
    --border:#272b34;
    --border-soft:#20232b;
    --text:#e7e9ee;
    --muted:#8a90a1;
    --muted-2:#5f6675;
    --accent:#6c8cff;
    --accent-dim:#3d4a8a;
    --accent-soft:#232a45;
    --danger:#ff5c7a;
    --warn:#ffb454;
    --ok:#5ee6a8;
    --mono: ui-monospace, 'SFMono-Regular', Menlo, Consolas, 'Courier New', monospace;
    --sans: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Pretendard', 'Malgun Gothic', sans-serif;
  }
  *{box-sizing:border-box;}
  html,body{height:100%;margin:0;padding:0;background:var(--bg);color:var(--text);font-family:var(--sans);overflow:hidden;user-select:none;}
  button{font-family:inherit;}
  input,select{font-family:inherit;}
  ::-webkit-scrollbar{width:9px;height:9px;}
  ::-webkit-scrollbar-track{background:transparent;}
  ::-webkit-scrollbar-thumb{background:#2c313c;border-radius:5px;}
  ::-webkit-scrollbar-thumb:hover{background:#3a4050;}

  #app{
    display:grid;
    grid-template-columns: 236px 1fr 288px;
    grid-template-rows: 50px 1fr 176px;
    grid-template-areas:
      "topbar topbar topbar"
      "left center right"
      "log log log";
    height:100vh;
    width:100vw;
  }

  /* ---------- Topbar ---------- */
  #topbar{
    grid-area: topbar;
    display:flex;align-items:center;gap:14px;
    padding:0 14px;
    background:linear-gradient(180deg,#14161b,#111318);
    border-bottom:1px solid var(--border);
  }
  #brand{display:flex;align-items:center;gap:8px;margin-right:6px;flex-shrink:0;}
  #brand .mark{
    width:20px;height:20px;border-radius:5px;
    background:conic-gradient(from 220deg, var(--accent), #a06cff, var(--accent));
    box-shadow:0 0 0 1px rgba(255,255,255,.08) inset;
  }
  #brand .name{font-weight:700;font-size:13.5px;letter-spacing:.2px;}
  #brand .name b{color:var(--accent);}
  .toolbar-group{display:flex;align-items:center;gap:4px;padding:0 8px;border-right:1px solid var(--border-soft);height:30px;}
  .toolbar-group:last-of-type{border-right:none;}
  .tbtn{
    display:flex;align-items:center;gap:5px;
    background:transparent;border:1px solid transparent;color:var(--text);
    font-size:12px;padding:5px 9px;border-radius:6px;cursor:pointer;white-space:nowrap;
  }
  .tbtn:hover{background:var(--bg-raised);border-color:var(--border);}
  .tbtn:active{background:var(--accent-soft);}
  .tbtn.primary{background:var(--accent-soft);color:var(--accent);border-color:var(--accent-dim);}
  .tbtn.primary:hover{background:var(--accent-dim);color:#fff;}
  .tbtn:disabled{opacity:.35;cursor:not-allowed;}
  #settingsInline{display:flex;align-items:center;gap:10px;margin-left:auto;flex-shrink:0;}
  .field-mini{display:flex;align-items:center;gap:5px;font-size:11px;color:var(--muted);}
  .field-mini input{
    width:46px;background:var(--bg-raised);border:1px solid var(--border);color:var(--text);
    border-radius:5px;padding:4px 6px;font-size:11px;font-family:var(--mono);
  }
  .field-mini input:focus{outline:none;border-color:var(--accent-dim);}

  /* ---------- Panels shared ---------- */
  .panel{background:var(--bg-panel);display:flex;flex-direction:column;overflow:hidden;}
  .panel-head{
    padding:9px 12px 7px;font-size:10.5px;font-weight:700;letter-spacing:.6px;color:var(--muted);
    text-transform:uppercase;display:flex;align-items:center;justify-content:space-between;
    border-bottom:1px solid var(--border-soft);
  }
  .panel-body{flex:1;overflow-y:auto;padding:10px 12px;}
  .mini-btn{
    background:var(--bg-raised);border:1px solid var(--border);color:var(--muted);
    font-size:10.5px;padding:3px 7px;border-radius:5px;cursor:pointer;
  }
  .mini-btn:hover{color:var(--text);border-color:var(--accent-dim);}

  /* ---------- Left column ---------- */
  #left{grid-area:left;border-right:1px solid var(--border);display:flex;flex-direction:column;overflow:hidden;}
  #left > .panel{border-bottom:1px solid var(--border);}
  #left > .panel:last-child{border-bottom:none;flex:1 1 auto;min-height:0;}
  #projectPanel .panel-body{padding:10px 12px 12px;}
  .kv-row{display:flex;align-items:center;justify-content:space-between;margin-bottom:7px;font-size:11.5px;}
  .kv-row label{color:var(--muted);}
  .kv-row input, .kv-row select{
    width:78px;background:var(--bg-raised);border:1px solid var(--border);color:var(--text);
    border-radius:5px;padding:4px 6px;font-size:11.5px;font-family:var(--mono);text-align:right;
  }
  .kv-row input:focus{outline:none;border-color:var(--accent-dim);}
  #projectName{
    width:100%;background:var(--bg-raised);border:1px solid var(--border);color:var(--text);
    border-radius:5px;padding:6px 8px;font-size:12px;margin-bottom:10px;
  }
  #projectName:focus{outline:none;border-color:var(--accent-dim);}

  #frameListPanel{flex:1 1 220px;min-height:120px;}
  #frameList{list-style:none;margin:0;padding:0;display:flex;flex-direction:column;gap:5px;}
  .frame-item{
    display:flex;align-items:center;gap:7px;
    padding:5px 7px;border-radius:7px;border:1px solid transparent;cursor:pointer;
  }
  .frame-item:hover{background:var(--bg-raised);}
  .frame-item.active{background:var(--accent-soft);border-color:var(--accent-dim);}
  .frame-sprocket{display:flex;flex-direction:column;gap:2px;flex-shrink:0;}
  .frame-sprocket i{width:3px;height:3px;border-radius:50%;background:var(--border);}
  .frame-item.active .frame-sprocket i{background:var(--accent);}
  .frame-thumb{
    width:38px;height:30px;border-radius:4px;border:1px solid var(--border);
    background:
      linear-gradient(45deg,#20232b 25%,transparent 25%,transparent 75%,#20232b 75%),
      linear-gradient(45deg,#20232b 25%,transparent 25%,transparent 75%,#20232b 75%);
    background-size:8px 8px;background-position:0 0,4px 4px;background-color:#15171c;
    flex-shrink:0;image-rendering:pixelated;
  }
  .frame-meta{display:flex;flex-direction:column;gap:1px;flex:1;min-width:0;}
  .frame-meta .idx{font-size:11px;font-weight:600;}
  .frame-meta .tag{font-size:9.5px;color:var(--muted-2);font-family:var(--mono);}
  .frame-actions{display:flex;gap:3px;}
  .icon-btn{
    width:20px;height:20px;display:flex;align-items:center;justify-content:center;
    background:transparent;border:none;color:var(--muted);border-radius:4px;cursor:pointer;font-size:12px;
  }
  .icon-btn:hover{background:var(--bg-panel-2);color:var(--text);}
  .icon-btn.danger:hover{color:var(--danger);}
  #frameToolbar{display:flex;gap:6px;padding:8px 12px;border-top:1px solid var(--border-soft);}

  #compressionPanel{flex:0 0 auto;max-height:260px;}
  .comp-row{display:flex;align-items:center;gap:7px;font-size:11.5px;padding:4px 0;}
  .comp-row input[type=checkbox]{accent-color:var(--accent);}
  .comp-row .name{flex:1;}
  .comp-row .ratio{font-family:var(--mono);color:var(--muted);font-size:10.5px;}
  #compressionStats{
    margin-top:8px;padding:8px 9px;background:var(--bg-raised);border:1px solid var(--border-soft);
    border-radius:7px;font-size:10.5px;font-family:var(--mono);color:var(--muted);line-height:1.7;
  }
  #compressionStats b{color:var(--ok);}
  #runAnalysis{width:100%;margin-top:8px;}

  /* ---------- Center ---------- */
  #center{grid-area:center;display:flex;flex-direction:column;overflow:hidden;background:#08090b;}
  #canvasToolbar{
    display:flex;align-items:center;gap:8px;padding:8px 12px;background:var(--bg-panel);
    border-bottom:1px solid var(--border);flex-wrap:wrap;position:relative;
  }
  .tool-btn{
    width:30px;height:30px;display:flex;align-items:center;justify-content:center;
    background:var(--bg-raised);border:1px solid var(--border);border-radius:7px;
    color:var(--muted);cursor:pointer;font-size:14px;
  }
  .tool-btn:hover{color:var(--text);}
  .tool-btn.active{background:var(--accent-soft);border-color:var(--accent);color:var(--accent);}
  #colorSwatch{
    width:30px;height:30px;border-radius:7px;border:1px solid var(--border);cursor:pointer;
    position:relative;background:#ff5c7a;
  }
  #colorPickerInput{display:none;}
  #colorWheelPopover{
    position:absolute;z-index:50;background:var(--bg-panel);border:1px solid var(--border);
    border-radius:12px;padding:14px;box-shadow:0 12px 40px rgba(0,0,0,.55);
    display:flex;flex-direction:column;align-items:center;gap:10px;width:198px;
  }
  #colorWheelCanvas{border-radius:50%;cursor:crosshair;touch-action:none;}
  #colorWheelControls{width:100%;display:flex;flex-direction:column;gap:8px;}
  #colorValueRange{width:100%;accent-color:var(--accent);}
  #colorWheelRow{display:flex;align-items:center;gap:8px;width:100%;}
  #colorWheelPreview{width:26px;height:26px;border-radius:7px;border:1px solid var(--border);flex-shrink:0;}
  #colorWheelHex{font-family:var(--mono);font-size:11px;color:var(--muted);flex:1;text-align:center;}
  .sep{width:1px;height:22px;background:var(--border-soft);margin:0 3px;}
  #zoomLabel{font-size:11px;font-family:var(--mono);color:var(--muted);width:44px;text-align:center;}

  #canvasWrap{
    flex:1;position:relative;overflow:hidden;
    background:
      radial-gradient(circle at 50% 0%, #101218 0%, #08090b 60%);
    display:flex;align-items:center;justify-content:center;
  }
  #canvasStage{position:relative;}
  #mainCanvas{
    image-rendering:pixelated;image-rendering:crisp-edges;
    border:1px solid var(--border);
    box-shadow:0 8px 30px rgba(0,0,0,.5);
    cursor:crosshair;
  }

  #playbar{
    display:flex;align-items:center;gap:10px;padding:8px 14px;
    background:var(--bg-panel);border-top:1px solid var(--border);
  }
  #playbar .tbtn{font-size:14px;padding:6px 10px;}
  #scrubber{flex:1;-webkit-appearance:none;height:4px;border-radius:2px;background:var(--border);outline:none;}
  #scrubber::-webkit-slider-thumb{-webkit-appearance:none;width:12px;height:12px;border-radius:50%;background:var(--accent);cursor:pointer;}
  #frameCounter{font-family:var(--mono);font-size:11px;color:var(--muted);width:64px;text-align:center;flex-shrink:0;}

  /* ---------- Right ---------- */
  #right{grid-area:right;border-left:1px solid var(--border);display:flex;flex-direction:column;overflow:hidden;min-height:0;}
  #right > .panel{height:100%;min-height:0;}
  .prop-block{margin-bottom:16px;}
  .prop-title{font-size:10.5px;font-weight:700;color:var(--muted);letter-spacing:.5px;text-transform:uppercase;margin-bottom:8px;}
  #paletteGrid{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;max-height:140px;overflow-y:auto;}
  #paletteCountLabel{font-size:10px;color:var(--muted-2);margin-top:5px;font-family:var(--mono);}
  #layerList{list-style:none;margin:0;padding:0;display:flex;flex-direction:column;gap:4px;max-height:130px;overflow-y:auto;}
  .layer-item{
    display:flex;align-items:center;gap:6px;padding:4px 6px;border-radius:6px;
    border:1px solid transparent;font-size:11px;cursor:pointer;
  }
  .layer-item:hover{background:var(--bg-raised);}
  .layer-item.active{background:var(--accent-soft);border-color:var(--accent-dim);}
  .layer-item .lname{flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}
  .layer-item input[type=checkbox]{accent-color:var(--accent);}
  #layerToolbar{display:flex;gap:5px;margin-top:6px;}
  #layerNote{font-size:9.5px;color:var(--muted-2);margin-top:5px;line-height:1.4;}
  .swatch{
    aspect-ratio:1;border-radius:5px;border:1px solid var(--border);cursor:pointer;position:relative;
  }
  .swatch.selected{outline:2px solid var(--accent);outline-offset:1px;}
  .swatch.transparent-swatch{
    background:
      linear-gradient(45deg,#20232b 25%,transparent 25%,transparent 75%,#20232b 75%),
      linear-gradient(45deg,#20232b 25%,transparent 25%,transparent 75%,#20232b 75%);
    background-size:6px 6px;background-position:0 0,3px 3px;background-color:#15171c;
  }
  #brushSizeRange{width:100%;accent-color:var(--accent);}
  .prop-row{display:flex;align-items:center;justify-content:space-between;font-size:11.5px;margin-bottom:6px;color:var(--muted);}
  .prop-row span.val{color:var(--text);font-family:var(--mono);}
  #framePropsBox{
    background:var(--bg-raised);border:1px solid var(--border-soft);border-radius:8px;
    padding:9px 10px;font-size:11px;line-height:1.8;color:var(--muted);font-family:var(--mono);
  }
  #framePropsBox b{color:var(--text);}

  /* ---------- Log ---------- */
  #log{grid-area:log;border-top:1px solid var(--border);display:flex;flex-direction:column;}
  #logBody{flex:1;overflow-y:auto;padding:8px 14px;font-family:var(--mono);font-size:11px;}
  .log-line{display:flex;gap:8px;padding:2px 0;color:var(--muted);}
  .log-line .t{color:var(--muted-2);flex-shrink:0;}
  .log-line.info .m{color:var(--text);}
  .log-line.ok .m{color:var(--ok);}
  .log-line.warn .m{color:var(--warn);}
  .log-line.error .m{color:var(--danger);}

  input[type=file]{display:none;}
  .hidden{display:none !important;}

  /* ---------- 재생 모드 (파일 선택 -> 재생 전용) ---------- */
  #playbackModeOverlay{
    position:fixed;inset:0;z-index:500;background:#050608;
    display:flex;flex-direction:column;
  }
  #playbackModeBar{
    display:flex;align-items:center;gap:12px;padding:10px 16px;
    background:var(--bg-panel);border-bottom:1px solid var(--border);flex-shrink:0;
  }
  #playbackModeTitle{font-weight:700;font-size:13px;color:var(--accent);}
  #playbackModeInfo{font-family:var(--mono);font-size:11px;color:var(--muted);}
  #playbackModeStage{
    flex:1;display:flex;align-items:center;justify-content:center;position:relative;
    background:radial-gradient(circle at 50% 40%, #101218 0%, #050608 65%);
  }
  #playbackModeCanvas{image-rendering:pixelated;box-shadow:0 10px 40px rgba(0,0,0,.6);border:1px solid var(--border);display:none;}
  #playbackModeEmpty{color:var(--muted-2);font-size:13px;}
</style>
</head>
<body>
<div id="app">
  <!-- 상단 툴바 -->
  <div id="topbar">
    <div id="brand"><div class="mark"></div><div class="name">JVideo <b>Studio</b></div></div>

    <div class="toolbar-group">
      <button class="tbtn" id="btnNew" title="새 프로젝트">새 프로젝트</button>
      <button class="tbtn" id="btnOpen" title="열기 (.jv)">열기</button>
      <button class="tbtn" id="btnSave" title="저장">저장</button>
      <button class="tbtn" id="btnSaveAs" title="다른 이름으로 저장">다른 이름 저장</button>
    </div>
    <div class="toolbar-group">
      <button class="tbtn primary" id="btnExport" title="내보내기 (.jv)">내보내기</button>
      <button class="tbtn" id="btnImport" title="가져오기 (.jv)">가져오기</button>
      <button class="tbtn" id="btnImportMp4" title="MP4를 프레임으로 변환해 가져오기">MP4 가져오기</button>
    </div>
    <div class="toolbar-group">
      <button class="tbtn" id="btnPlaybackMode" title="파일을 선택하면 재생만 해주는 전용 화면">재생 모드</button>
    </div>
    <div id="mp4Progress" class="hidden" style="font-size:11px;color:var(--warn);font-family:var(--mono);"></div>

    <div id="settingsInline">
      <div class="field-mini"><label>W</label><input type="number" id="cfgWidth" min="1" max="512" value="32"></div>
      <div class="field-mini"><label>H</label><input type="number" id="cfgHeight" min="1" max="512" value="32"></div>
      <div class="field-mini"><label>FPS</label><input type="number" id="cfgFps" min="1" max="60" value="12"></div>
      <div class="field-mini"><label>MAX</label><input type="number" id="cfgMax" min="1" max="9999" value="240"></div>
      <button class="tbtn" id="btnApplySettings">적용</button>
    </div>
  </div>

  <input type="file" id="fileInputOpen" accept=".jv,text/plain">
  <input type="file" id="fileInputImport" accept=".jv,text/plain">
  <input type="file" id="fileInputMp4" accept="video/mp4,video/*">
  <input type="file" id="fileInputPlaybackMode" accept=".jv,text/plain">

  <!-- 재생 모드: 파일을 고르면 재생만 하는 전용 화면. 편집기와 완전히 분리되어 있다. -->
  <div id="playbackModeOverlay" class="hidden">
    <div id="playbackModeBar">
      <span id="playbackModeTitle">재생 모드</span>
      <button class="tbtn" id="btnPmPick">파일 선택 (.jv)</button>
      <span id="playbackModeInfo"></span>
      <button class="tbtn" id="btnPmExit" style="margin-left:auto;">편집 모드로 돌아가기</button>
    </div>
    <div id="playbackModeStage">
      <canvas id="playbackModeCanvas"></canvas>
      <div id="playbackModeEmpty">재생할 .jv 파일을 선택하세요</div>
    </div>
  </div>

  <!-- 좌측 패널 -->
  <div id="left">
    <div class="panel" id="projectPanel">
      <div class="panel-head">프로젝트</div>
      <div class="panel-body">
        <input id="projectName" type="text" value="untitled">
        <div class="kv-row"><label>해상도</label><span id="resLabel" style="font-family:var(--mono);font-size:11.5px;">32×32</span></div>
        <div class="kv-row"><label>FPS</label><span id="fpsLabel" style="font-family:var(--mono);font-size:11.5px;">12</span></div>
        <div class="kv-row"><label>프레임 수</label><span id="frameCountLabel" style="font-family:var(--mono);font-size:11.5px;">1 / 240</span></div>
      </div>
    </div>

    <div class="panel" id="frameListPanel">
      <div class="panel-head">프레임 목록<span class="mini-btn" id="btnFrameHelp" title="프레임 툴바 사용">?</span></div>
      <div class="panel-body"><ul id="frameList"></ul></div>
      <div id="frameToolbar">
        <button class="tbtn" id="btnFrameAdd">+ 추가</button>
        <button class="tbtn" id="btnFrameDup">복사</button>
        <button class="tbtn" id="btnFrameDel">삭제</button>
        <button class="tbtn" id="btnFrameUp">▲</button>
        <button class="tbtn" id="btnFrameDown">▼</button>
      </div>
    </div>

    <div class="panel" id="compressionPanel">
      <div class="panel-head">압축 분석</div>
      <div class="panel-body">
        <div id="compressorList"></div>
        <button class="mini-btn" id="runAnalysis">분석 실행</button>
        <div id="compressionStats">분석을 실행하면 결과가 여기에 표시됩니다.</div>
      </div>
    </div>
  </div>

  <!-- 중앙: 캔버스 -->
  <div id="center">
    <div id="canvasToolbar">
      <button class="tool-btn active" data-tool="brush" title="브러시">✎</button>
      <button class="tool-btn" data-tool="eraser" title="지우개">▢</button>
      <button class="tool-btn" data-tool="rect" title="사각형">▭</button>
      <button class="tool-btn" data-tool="circle" title="원">◯</button>
      <button class="tool-btn" data-tool="fill" title="채우기">▨</button>
      <button class="tool-btn" data-tool="line" title="선">╱</button>
      <button class="tool-btn" data-tool="picker" title="색 선택(스포이드)">◉</button>
      <button class="tool-btn" data-tool="select" title="선택(드래그) - Ctrl+C/V 복사·붙여넣기, Ctrl+S 누른 채 드래그하면 이동">⬚</button>
      <div class="sep"></div>
      <div id="colorSwatch"><input type="color" id="colorPickerInput" value="#ff5c7a"></div>
      <div id="colorWheelPopover" class="hidden">
        <canvas id="colorWheelCanvas" width="150" height="150"></canvas>
        <div id="colorWheelControls">
          <div id="colorWheelRow">
            <div id="colorWheelPreview"></div>
            <div id="colorWheelHex">#FF5C7A</div>
          </div>
          <input type="range" id="colorValueRange" min="0" max="100" value="100" title="명도(Value)">
        </div>
      </div>
      <div class="sep"></div>
      <button class="tool-btn" id="btnZoomOut" title="축소">−</button>
      <span id="zoomLabel">800%</span>
      <button class="tool-btn" id="btnZoomIn" title="확대">+</button>
      <button class="tool-btn" id="btnZoomFit" title="화면에 맞춤" style="font-size:11px;">FIT</button>
      <div class="sep"></div>
      <button class="tool-btn" id="btnPan" title="이동(팬) - 스페이스+드래그 로도 가능">✥</button>
    </div>

    <div id="canvasWrap">
      <div id="canvasStage">
        <canvas id="mainCanvas" width="320" height="320"></canvas>
      </div>
    </div>

    <div id="playbar">
      <button class="tbtn" id="btnFirst" title="처음">⏮</button>
      <button class="tbtn" id="btnPlay" title="재생/일시정지">▶</button>
      <button class="tbtn" id="btnStop" title="정지">⏹</button>
      <button class="tbtn" id="btnLast" title="끝">⏭</button>
      <input type="range" id="scrubber" min="0" max="0" value="0" step="1">
      <span id="frameCounter">1 / 1</span>
      <div class="sep"></div>
      <label class="field-mini"><input type="checkbox" id="chkLoop" checked style="width:auto;"> 루프</label>
      <div class="field-mini"><label>FPS</label><input type="number" id="playFps" min="1" max="60" value="12"></div>
    </div>
  </div>

  <!-- 우측 패널 -->
  <div id="right">
    <div class="panel">
      <div class="panel-head">속성</div>
      <div class="panel-body">
        <div class="prop-block">
          <div class="prop-title">브러시 크기</div>
          <input type="range" id="brushSizeRange" min="1" max="12" value="1">
          <div class="prop-row"><span>크기</span><span class="val" id="brushSizeVal">1px</span></div>
        </div>
        <div class="prop-block">
          <div class="prop-title">레이어<span class="mini-btn" id="layerHelp" title="레이어는 편집 중에만 존재합니다. .jv로 저장/내보내기 하면 화면에 보이는 대로 한 장으로 합쳐(flatten)져 저장되며, 레이어 구성 자체는 파일에 남지 않습니다.">?</span></div>
          <ul id="layerList"></ul>
          <div id="layerToolbar">
            <button class="mini-btn" id="btnLayerAdd">+ 추가</button>
            <button class="mini-btn" id="btnLayerDel">삭제</button>
            <button class="mini-btn" id="btnLayerUp">▲</button>
            <button class="mini-btn" id="btnLayerDown">▼</button>
          </div>
          <div id="layerNote">레이어는 저장 시 합쳐지며 파일에는 남지 않습니다.</div>
        </div>
        <div class="prop-block">
          <div class="prop-title">팔레트 (최대 7색)</div>
          <div id="paletteGrid"></div>
          <div id="paletteCountLabel">0 / 7</div>
        </div>
        <div class="prop-block">
          <div class="prop-title">현재 프레임</div>
          <div id="framePropsBox">
            인덱스: <b id="propFrameIdx">1</b><br>
            타입: <b id="propFrameType">FULL</b><br>
            사용 색상 수: <b id="propColorsUsed">0</b>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- 하단 로그 -->
  <div id="log">
    <div class="panel-head">로그<span class="mini-btn" id="btnClearLog">지우기</span></div>
    <div id="logBody"></div>
  </div>
</div>

<script>
'use strict';
/* ==========================================================================
   JVideo Studio
   단일 HTML 파일 - 외부 라이브러리/CDN/프레임워크 없음, 오프라인 동작.

   구조 (모듈처럼 분리):
     1) 유틸/Logger
     2) JV 압축 코어 (SameFrame / Diff / ObjectMove / Delta / RLE /
        TransparentSkip / LineRepeat·BlockMerge / Dictionary / Palette)
     3) JV 파일 포맷 (직렬화/역직렬화)
     4) 데이터 모델 (Palette, Frame, Project)
     5) CanvasEditor (렌더링 + 툴)
     6) PlaybackController (재생)
     7) UIController (DOM 배선)
   ========================================================================== */

/* --------------------------------------------------------------------------
   1) 유틸 / Logger
   -------------------------------------------------------------------------- */
function byteHex(n) { return n.toString(16).toUpperCase().padStart(2, '0'); }
function encodeSigned(n) { return (n < 0 ? '-' : '+') + Math.abs(n).toString(16).toUpperCase(); }
function arraysEqual(a, b) {
  if (a.length !== b.length) return false;
  for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
  return true;
}
function clamp(v, min, max) { return Math.max(min, Math.min(max, v)); }

class Logger {
  constructor(el) { this.el = el; }
  _push(type, msg) {
    const line = document.createElement('div');
    line.className = 'log-line ' + type;
    const t = new Date();
    const ts = t.toLocaleTimeString('ko-KR', { hour12: false });
    line.innerHTML = '<span class="t"></span><span class="m"></span>';
    line.querySelector('.t').textContent = ts;
    line.querySelector('.m').textContent = msg;
    this.el.appendChild(line);
    this.el.scrollTop = this.el.scrollHeight;
    while (this.el.children.length > 400) this.el.removeChild(this.el.firstChild);
  }
  info(msg) { this._push('info', msg); }
  ok(msg) { this._push('ok', msg); }
  warn(msg) { this._push('warn', msg); }
  error(msg) { this._push('error', msg); }
}

/* --------------------------------------------------------------------------
   2) JV 압축 코어
   문자 집합 제약: 내부 데이터 표현에는 숫자 0-9, 영문 A-F, 그리고
   지정된 기호(! @ # $ % ^ & * ( ) + - _ = ` ~ . , < > / ?)만 사용한다.
   구조를 나타내는 오프코드도 알파벳 대신 기호를 사용해 이 제약을 지킨다.
   -------------------------------------------------------------------------- */

class JVReader {
  constructor(str) { this.s = str; this.i = 0; }
  peek() { return this.s[this.i]; }
  next() { return this.s[this.i++]; }
  eof() { return this.i >= this.s.length; }
  readHex() {
    const start = this.i;
    while (!this.eof() && /[0-9A-F]/.test(this.s[this.i])) this.i++;
    if (this.i === start) throw new Error('JV 파싱 오류: hex 값이 필요합니다 (위치 ' + start + ')');
    return parseInt(this.s.slice(start, this.i), 16);
  }
  readSigned() {
    const sign = this.next();
    if (sign !== '+' && sign !== '-') throw new Error('JV 파싱 오류: 부호(+/-)가 필요합니다 (위치 ' + (this.i - 1) + ')');
    const val = this.readHex();
    return sign === '-' ? -val : val;
  }
  expect(ch) {
    if (this.s[this.i] !== ch) throw new Error("JV 파싱 오류: '" + ch + "' 예상, 실제 '" + this.s[this.i] + "' (위치 " + this.i + ")");
    this.i++;
  }
}

// ---- RLE + TransparentSkip (행 단위) ----
// 팔레트 index 0 은 항상 "투명"으로 예약된다.
function buildRowRunToken(pixels, offset, width) {
  let out = '';
  let i = 0;
  while (i < width) {
    const color = pixels[offset + i];
    let count = 1;
    while (i + count < width && pixels[offset + i + count] === color) count++;
    if (color === 0) out += '?' + count.toString(16).toUpperCase() + '.';
    else out += byteHex(color) + '@' + count.toString(16).toUpperCase() + '.';
    i += count;
  }
  return out;
}
function decodeRowRun(reader, pixels, offset, width) {
  let i = 0;
  while (i < width) {
    const c = reader.peek();
    if (c === '?') {
      reader.next();
      const count = reader.readHex(); reader.expect('.');
      for (let k = 0; k < count; k++) pixels[offset + i + k] = 0;
      i += count;
    } else {
      const color = reader.readHex(); reader.expect('@');
      const count = reader.readHex(); reader.expect('.');
      for (let k = 0; k < count; k++) pixels[offset + i + k] = color;
      i += count;
    }
  }
}

// ---- FULL 프레임: RLE/Skip + LineRepeat·BlockMerge('^') + Dictionary('*'/'$') ----
function encodeFullFrame(pixels, width, height) {
  const rowTokens = [];
  for (let r = 0; r < height; r++) rowTokens.push(buildRowRunToken(pixels, r * width, width));

  const freq = new Map();
  for (const t of rowTokens) freq.set(t, (freq.get(t) || 0) + 1);

  const dictIndexOf = new Map();
  let dictCounter = 0;
  function emitEntry(token) {
    if (freq.get(token) >= 2) {
      if (dictIndexOf.has(token)) return '$' + dictIndexOf.get(token).toString(16).toUpperCase() + '.';
      dictIndexOf.set(token, dictCounter++);
      return '*' + token;
    }
    return '=' + token;
  }

  let out = '&';
  let r = 0;
  while (r < height) {
    let rep = 0, j = r + 1;
    while (j < height && rowTokens[j] === rowTokens[r]) { rep++; j++; }
    out += emitEntry(rowTokens[r]);
    if (rep > 0) out += '^' + rep.toString(16).toUpperCase() + '.';
    r = rep > 0 ? j : r + 1;
  }
  return out;
}
function decodeFullFrame(reader, width, height) {
  reader.expect('&');
  const pixels = new Uint8Array(width * height);
  const dictArr = [];
  let r = 0;
  while (r < height) {
    const marker = reader.next();
    if (marker === '=') { decodeRowRun(reader, pixels, r * width, width); r++; }
    else if (marker === '*') {
      const rowPixels = new Uint8Array(width);
      decodeRowRun(reader, rowPixels, 0, width);
      pixels.set(rowPixels, r * width);
      dictArr.push(rowPixels);
      r++;
    } else if (marker === '$') {
      const idx = reader.readHex(); reader.expect('.');
      pixels.set(dictArr[idx], r * width);
      r++;
    } else throw new Error("알 수 없는 행 마커 '" + marker + "'");
    if (reader.peek() === '^') {
      reader.next();
      const rep = reader.readHex(); reader.expect('.');
      const srcRow = pixels.slice((r - 1) * width, r * width);
      for (let k = 0; k < rep; k++) { pixels.set(srcRow, r * width); r++; }
    }
  }
  return pixels;
}

// ---- DIFF 프레임 v2: 다중 ObjectMove('!') + Run-length Delta(부호+hex, '@'런길이) ----
// v1과 달리 이동은 프레임당 여러 개(연결 성분별로 최대 6개)까지 찾고,
// 남는 변경 픽셀도 연속 인덱스+동일 색이면 런(run)으로 묶어 저장한다.
// 압축률을 우선한 설계이므로 사람이 눈으로 따라가며 읽기는 v1보다 어렵다.

// 도착(변경 후 값이 0이 아닌) 픽셀들을 4-연결 기준으로 묶어 서로 다른 물체를 분리한다.
function connectedComponents(indices, width, height) {
  const set = new Set(indices);
  const visited = new Set();
  const comps = [];
  for (const start of indices) {
    if (visited.has(start)) continue;
    const comp = [];
    const stack = [start];
    visited.add(start);
    while (stack.length) {
      const idx = stack.pop();
      comp.push(idx);
      const x = idx % width;
      const neighbors = [];
      if (x > 0) neighbors.push(idx - 1);
      if (x < width - 1) neighbors.push(idx + 1);
      if (idx - width >= 0) neighbors.push(idx - width);
      if (idx + width < width * height) neighbors.push(idx + width);
      for (const n of neighbors) {
        if (set.has(n) && !visited.has(n)) { visited.add(n); stack.push(n); }
      }
    }
    comps.push(comp);
  }
  return comps;
}

function encodeDiffFrame(curr, prev, width, height) {
  const changedSet = new Set();
  for (let i = 0; i < curr.length; i++) if (curr[i] !== prev[i]) changedSet.add(i);

  const moves = [];
  const MAX_MOVES = 6;
  const MAXSHIFT = 32;

  for (let iter = 0; iter < MAX_MOVES; iter++) {
    const arrival = [];
    for (const idx of changedSet) if (curr[idx] !== 0) arrival.push(idx);
    if (arrival.length === 0) break;

    // 서로 다른 물체(연결 성분)별로 나눠서 각각 이동인지 시도한다 (multi-move).
    // 성분을 합친 하나의 경계상자로만 찾으면 서로 다른 방향으로 움직이는
    // 물체가 둘 이상일 때 어떤 이동 벡터도 맞지 않아 탐지에 실패하기 때문.
    const comps = connectedComponents(arrival, width, height).sort((a, b) => b.length - a.length);
    let matchedThisIter = false;

    for (const comp of comps) {
      let minX = width, minY = height, maxX = 0, maxY = 0;
      for (const idx of comp) {
        const x = idx % width, y = (idx / width) | 0;
        if (x < minX) minX = x; if (x > maxX) maxX = x;
        if (y < minY) minY = y; if (y > maxY) maxY = y;
      }
      const w = maxX - minX + 1, h = maxY - minY + 1;
      const bboxArea = w * h, totalArea = width * height;
      if (!(w <= 96 && h <= 96 && bboxArea <= totalArea * 0.5)) continue;

      let found = null;
      outer:
      for (let dy = -MAXSHIFT; dy <= MAXSHIFT; dy++) {
        for (let dx = -MAXSHIFT; dx <= MAXSHIFT; dx++) {
          if (dx === 0 && dy === 0) continue;
          const sx = minX - dx, sy = minY - dy;
          if (sx < 0 || sy < 0 || sx + w > width || sy + h > height) continue;
          let match = true;
          for (let yy = 0; yy < h && match; yy++) {
            for (let xx = 0; xx < w; xx++) {
              const cIdx = (minY + yy) * width + (minX + xx);
              const pIdx = (sy + yy) * width + (sx + xx);
              if (curr[cIdx] !== prev[pIdx]) { match = false; break; }
            }
          }
          if (match) { found = { destX: minX, destY: minY, w, h, dx, dy }; break outer; }
        }
      }
      if (found) {
        moves.push(found);
        for (let yy = 0; yy < found.h; yy++)
          for (let xx = 0; xx < found.w; xx++)
            changedSet.delete((found.destY + yy) * width + (found.destX + xx));
        matchedThisIter = true;
        break; // 이번 iteration엔 하나만 확정, 나머지 성분은 다음 iteration에서 재시도
      }
    }
    if (!matchedThisIter) break;
  }

  const remaining = [...changedSet].sort((a, b) => a - b);

  // 연속 인덱스 + 동일 색상을 하나의 run으로 묶는다 (Run-length Delta).
  const groups = [];
  let gi = 0;
  while (gi < remaining.length) {
    const startIdx = remaining[gi];
    const color = curr[startIdx];
    let runLen = 1;
    while (gi + runLen < remaining.length && remaining[gi + runLen] === startIdx + runLen && curr[remaining[gi + runLen]] === color) runLen++;
    groups.push({ idx: startIdx, runLen, color });
    gi += runLen;
  }

  let out = '%';
  out += '(' + moves.length.toString(16).toUpperCase() + '.';
  for (const mv of moves) {
    out += '!' + mv.destX.toString(16).toUpperCase() + ',' + mv.destY.toString(16).toUpperCase() + ',' +
      mv.w.toString(16).toUpperCase() + ',' + mv.h.toString(16).toUpperCase() + ',' +
      encodeSigned(mv.dx) + ',' + encodeSigned(mv.dy) + ';';
  }
  out += ')' + groups.length.toString(16).toUpperCase() + '.';
  let prevEndIdx = null;
  for (const g of groups) {
    if (prevEndIdx === null) out += g.idx.toString(16).toUpperCase();
    else out += encodeSigned(g.idx - prevEndIdx);
    if (g.runLen > 1) out += '@' + g.runLen.toString(16).toUpperCase();
    out += '.' + byteHex(g.color);
    prevEndIdx = g.idx + g.runLen - 1;
  }
  return out;
}

function decodeDiffFrame(reader, prevPixels, width, height) {
  reader.expect('%');
  const pixels = prevPixels.slice();
  reader.expect('(');
  const moveCount = reader.readHex(); reader.expect('.');
  const moves = [];
  for (let i = 0; i < moveCount; i++) {
    reader.expect('!');
    const destX = reader.readHex(); reader.expect(',');
    const destY = reader.readHex(); reader.expect(',');
    const w = reader.readHex(); reader.expect(',');
    const h = reader.readHex(); reader.expect(',');
    const dx = reader.readSigned(); reader.expect(',');
    const dy = reader.readSigned(); reader.expect(';');
    moves.push({ destX, destY, w, h, dx, dy });
  }
  for (const mv of moves) {
    for (let yy = 0; yy < mv.h; yy++) {
      for (let xx = 0; xx < mv.w; xx++) {
        const destIdx = (mv.destY + yy) * width + (mv.destX + xx);
        const srcIdx = (mv.destY + yy - mv.dy) * width + (mv.destX + xx - mv.dx);
        pixels[destIdx] = prevPixels[srcIdx];
      }
    }
  }
  reader.expect(')');
  const groupCount = reader.readHex(); reader.expect('.');
  let prevEndIdx = null;
  for (let i = 0; i < groupCount; i++) {
    let idx;
    if (prevEndIdx === null) idx = reader.readHex();
    else { const d = reader.readSigned(); idx = prevEndIdx + d; }
    let runLen = 1;
    if (reader.peek() === '@') { reader.next(); runLen = reader.readHex(); }
    reader.expect('.');
    const color = reader.readHex();
    for (let k = 0; k < runLen; k++) pixels[idx + k] = color;
    prevEndIdx = idx + runLen - 1;
  }
  return pixels;
}

// ---- 프레임 단위 통합 (SameFrame '#' 포함) ----
// 변경이 있는 프레임은 DIFF가 항상 유리한 것은 아니다(예: 장면 전환처럼
// 픽셀 대부분이 바뀌면 개별 변경점을 나열하는 DIFF보다 RLE/LineRepeat/
// Dictionary가 적용되는 FULL이 더 작을 수 있다). 그래서 두 인코딩을 모두
// 계산해 더 짧은 쪽을 채택한다. 디코더는 선행 마커('&' 또는 '%')로
// 어떤 방식인지 이미 구분하므로 디코드 쪽은 변경할 필요가 없다.
function encodeFrame(curr, prev, width, height) {
  if (prev && arraysEqual(curr, prev)) return '#';
  if (!prev) return encodeFullFrame(curr, width, height);
  const diffStr = encodeDiffFrame(curr, prev, width, height);
  const fullStr = encodeFullFrame(curr, width, height);
  return fullStr.length < diffStr.length ? fullStr : diffStr;
}
// 프레임이 실제로 어떤 방식으로 인코딩될지(SAME/FULL/DIFF)를
// UI 라벨에서 그대로 재사용할 수 있도록 노출한다. encodeFrame과 동일한
// 판단 기준(둘 다 계산해 더 짧은 쪽 채택)을 그대로 따른다.
function frameEncodingType(curr, prev, width, height) {
  if (prev && arraysEqual(curr, prev)) return 'SAME';
  if (!prev) return 'FULL';
  const diffStr = encodeDiffFrame(curr, prev, width, height);
  const fullStr = encodeFullFrame(curr, width, height);
  return fullStr.length < diffStr.length ? 'FULL' : 'DIFF';
}
function decodeFrame(reader, prevPixels, width, height) {
  const marker = reader.peek();
  if (marker === '#') { reader.next(); return prevPixels.slice(); }
  if (marker === '&') return decodeFullFrame(reader, width, height);
  if (marker === '%') return decodeDiffFrame(reader, prevPixels, width, height);
  throw new Error("알 수 없는 프레임 마커 '" + marker + "'");
}

/* --------------------------------------------------------------------------
   3) JV 파일 포맷 (header~palette~frame~frame~...)
   -------------------------------------------------------------------------- */
const JVFile = {
  MAGIC: '#2', // v2: 다중 ObjectMove + Run-length Delta 도입 (v1과 파일 호환 안됨)
  serialize(project) {
    const { width, height, fps, maxFrames } = project;
    const paletteColors = project.palette.colors;
    let out = this.MAGIC + '~';
    out += width.toString(16).toUpperCase() + ',' + height.toString(16).toUpperCase() + ',' +
      fps.toString(16).toUpperCase() + ',' + maxFrames.toString(16).toUpperCase() + '~';
    out += paletteColors.length.toString(16).toUpperCase();
    for (const c of paletteColors) out += ',' + c;
    out += '~';
    let prev = null;
    const frameStrs = [];
    for (const f of project.frames) {
      const flat = f.flatten(); // 레이어 합성 결과만 저장 (레이어 구성은 파일에 남지 않음)
      frameStrs.push(encodeFrame(flat, prev, width, height));
      prev = flat;
    }
    out += frameStrs.join('~');
    return out;
  },
  parse(text) {
    const parts = text.split('~');
    if (parts[0] !== this.MAGIC) throw new Error('JV 파일 형식이 아니거나 버전이 다릅니다.');
    const headerPart = parts[1], palettePart = parts[2];
    const framesPart = parts.slice(3);
    const [wHex, hHex, fpsHex, maxHex] = headerPart.split(',');
    const width = parseInt(wHex, 16), height = parseInt(hHex, 16);
    const fps = parseInt(fpsHex, 16), maxFrames = parseInt(maxHex, 16);
    const paletteFields = palettePart.split(',');
    const paletteCount = parseInt(paletteFields[0], 16);
    const paletteColors = paletteFields.slice(1, 1 + paletteCount);
    let prev = null;
    const frames = [];
    for (const fstr of framesPart) {
      const reader = new JVReader(fstr);
      const pixels = decodeFrame(reader, prev, width, height);
      frames.push(pixels);
      prev = pixels;
    }
    return { width, height, fps, maxFrames, paletteColors, frames };
  }
};

/* --------------------------------------------------------------------------
   4) 데이터 모델: Palette / Frame / Project
   -------------------------------------------------------------------------- */
const PALETTE_MAX = 7;
class Palette {
  constructor() { this.colors = []; } // index 0 은 항상 투명(예약), colors[0] == 실제 index 1
  indexOf(hex) { return this.colors.indexOf(hex.toUpperCase()); }
  // 이미 있는 색이면 해당 인덱스, 새 색이고 자리가 있으면 추가 후 인덱스,
  // 가득 찼는데 새 색이면 null(호출부에서 사용자에게 알림)을 반환한다.
  getOrAdd(hex) {
    hex = hex.toUpperCase();
    const i = this.indexOf(hex);
    if (i >= 0) return i + 1;
    if (this.colors.length >= PALETTE_MAX) return null; // 팔레트 상한(7색) 도달
    this.colors.push(hex);
    return this.colors.length; // 새 인덱스
  }
  colorAt(index) {
    if (index === 0) return null; // 투명
    return '#' + (this.colors[index - 1] || '000000');
  }
}

// 레이어는 편집 중에만 존재하는 개념이다. .jv 파일에는 절대 기록되지 않으며,
// 저장/내보내기/썸네일/압축 분석 등 "확정된 데이터"가 필요한 모든 곳에서는
// 반드시 Frame.flatten()으로 합성한 단일 픽셀 배열만 사용한다.
class Layer {
  constructor(width, height, name) {
    this.name = name || '레이어';
    this.pixels = new Uint8Array(width * height);
    this.visible = true;
  }
  clone(width, height) {
    const l = new Layer(width, height, this.name);
    l.pixels.set(this.pixels);
    l.visible = this.visible;
    return l;
  }
}

class Frame {
  constructor(width, height) {
    this.width = width; this.height = height;
    this.layers = [new Layer(width, height, '레이어 1')];
    this.activeLayerIndex = 0;
  }
  get activeLayer() { return this.layers[this.activeLayerIndex]; }
  clone(width, height) {
    const f = new Frame(width, height);
    f.layers = this.layers.map(l => l.clone(width, height));
    f.activeLayerIndex = this.activeLayerIndex;
    return f;
  }
  // 보이는 레이어들을 아래→위 순서로 합성한다(투명 픽셀=0은 아래 레이어를 그대로 통과).
  // overrideIndex/overridePixels 는 도형 드래그 중 "미리보기"를 위한 임시 치환용.
  flatten(overrideIndex = -1, overridePixels = null) {
    const out = new Uint8Array(this.width * this.height);
    for (let li = 0; li < this.layers.length; li++) {
      const layer = this.layers[li];
      if (!layer.visible) continue;
      const src = (li === overrideIndex && overridePixels) ? overridePixels : layer.pixels;
      for (let i = 0; i < out.length; i++) if (src[i] !== 0) out[i] = src[i];
    }
    return out;
  }
  usedColorCount() {
    const flat = this.flatten();
    const set = new Set();
    for (const p of flat) if (p !== 0) set.add(p);
    return set.size;
  }
  addLayer(afterIndex = this.activeLayerIndex) {
    const l = new Layer(this.width, this.height, '레이어 ' + (this.layers.length + 1));
    this.layers.splice(afterIndex + 1, 0, l);
    this.activeLayerIndex = afterIndex + 1;
    return l;
  }
  deleteLayer(index = this.activeLayerIndex) {
    if (this.layers.length <= 1) return false;
    this.layers.splice(index, 1);
    this.activeLayerIndex = clamp(this.activeLayerIndex, 0, this.layers.length - 1);
    return true;
  }
  moveLayer(index, dir) {
    const target = index + dir;
    if (target < 0 || target >= this.layers.length) return false;
    const tmp = this.layers[index]; this.layers[index] = this.layers[target]; this.layers[target] = tmp;
    this.activeLayerIndex = target;
    return true;
  }
}

class Project {
  constructor(width = 32, height = 32, fps = 12, maxFrames = 240) {
    this.name = 'untitled';
    this.width = width; this.height = height; this.fps = fps; this.maxFrames = maxFrames;
    this.palette = new Palette();
    this.frames = [new Frame(width, height)];
    this.currentFrameIndex = 0;
  }
  get currentFrame() { return this.frames[this.currentFrameIndex]; }
  addFrame(afterIndex = this.currentFrameIndex) {
    if (this.frames.length >= this.maxFrames) return false;
    const f = new Frame(this.width, this.height);
    this.frames.splice(afterIndex + 1, 0, f);
    this.currentFrameIndex = afterIndex + 1;
    return true;
  }
  duplicateFrame(index = this.currentFrameIndex) {
    if (this.frames.length >= this.maxFrames) return false;
    const f = this.frames[index].clone(this.width, this.height);
    this.frames.splice(index + 1, 0, f);
    this.currentFrameIndex = index + 1;
    return true;
  }
  deleteFrame(index = this.currentFrameIndex) {
    if (this.frames.length <= 1) return false;
    this.frames.splice(index, 1);
    this.currentFrameIndex = clamp(this.currentFrameIndex, 0, this.frames.length - 1);
    return true;
  }
  moveFrame(index, dir) {
    const target = index + dir;
    if (target < 0 || target >= this.frames.length) return false;
    const tmp = this.frames[index];
    this.frames[index] = this.frames[target];
    this.frames[target] = tmp;
    this.currentFrameIndex = target;
    return true;
  }
  resizeAll(width, height) {
    const ow = this.width, oh = this.height;
    const cw = Math.min(width, ow), ch = Math.min(height, oh);
    for (const old of this.frames) {
      const newLayers = old.layers.map(oldLayer => {
        const nl = new Layer(width, height, oldLayer.name);
        nl.visible = oldLayer.visible;
        for (let y = 0; y < ch; y++)
          for (let x = 0; x < cw; x++)
            nl.pixels[y * width + x] = oldLayer.pixels[y * ow + x];
        return nl;
      });
      old.width = width; old.height = height;
      old.layers = newLayers;
    }
    this.width = width; this.height = height;
  }
  toJVFileText() { return JVFile.serialize(this); }
  static fromJVFileText(text) {
    const parsed = JVFile.parse(text);
    const p = new Project(parsed.width, parsed.height, parsed.fps, parsed.maxFrames);
    p.palette = new Palette(); // 열기(Open) 시 팔레트는 항상 초기화된 뒤 파일의 팔레트로 새로 채워진다
    p.palette.colors = parsed.paletteColors.slice();
    p.frames = parsed.frames.map(px => {
      const f = new Frame(parsed.width, parsed.height);
      f.layers = [new Layer(parsed.width, parsed.height, '레이어 1')];
      f.layers[0].pixels = px; // 파일에는 레이어 정보가 없으므로 단일 레이어로 복원
      f.activeLayerIndex = 0;
      return f;
    });
    p.currentFrameIndex = 0;
    return p;
  }
}

/* --------------------------------------------------------------------------
   5) CanvasEditor: 렌더링 + 브러시/지우개/도형/채우기/스포이드 + 줌/팬
   -------------------------------------------------------------------------- */
class CanvasEditor {
  constructor(canvasEl, project, logger) {
    this.canvas = canvasEl;
    this.ctx = this.canvas.getContext('2d');
    this.project = project;
    this.logger = logger;
    this.zoom = 10; // px per pixel
    this.tool = 'brush';
    this.brushSize = 1;
    this.currentColorIndex = 1; // 0 = 투명(지우개)
    this.offscreen = document.createElement('canvas');
    this.offCtx = this.offscreen.getContext('2d');
    this.drawing = false;
    this.previewPixels = null; // 도형 미리보기용 임시 버퍼
    this.selection = null; // {x,y,w,h} - 선택(드래그) 툴의 현재 선택 영역
    this.clipboard = null; // {w,h,pixels} - Ctrl+C 로 복사된 내용
    this.ctrlSHeld = false; // Ctrl+S 를 누르고 있는 동안만 선택영역 "이동" 모드
    this._moveMode = false;
    this.onChange = null; // 콜백: 픽셀 변경시 (속성 패널 갱신 등)
    this._bindEvents();
  }

  setProject(project) {
    this.project = project;
    // 이전 프로젝트의 좌표계/팔레트에 종속된 상태를 모두 초기화한다.
    // (특히 클립보드는 팔레트 "색상"이 아니라 "인덱스"를 담고 있어서, 팔레트가
    // 다른 새 프로젝트에 그대로 남겨두면 붙여넣을 때 엉뚱한 색이 나오는 버그가 있었다)
    this.selection = null;
    this.previewPixels = null;
    this._moveMode = false;
    this.clipboard = null;
    this.render();
  }

  screenToPixel(clientX, clientY) {
    const rect = this.canvas.getBoundingClientRect();
    const x = Math.floor((clientX - rect.left) / this.zoom);
    const y = Math.floor((clientY - rect.top) / this.zoom);
    return { x, y };
  }

  inBounds(x, y) { return x >= 0 && y >= 0 && x < this.project.width && y < this.project.height; }

  render() {
    const { width, height } = this.project;
    this.offscreen.width = width; this.offscreen.height = height;
    const imgData = this.offCtx.createImageData(width, height);
    const frame = this.project.currentFrame;
    const pixels = this.previewPixels
      ? frame.flatten(frame.activeLayerIndex, this.previewPixels)
      : frame.flatten();
    const palette = this.project.palette;
    for (let i = 0; i < pixels.length; i++) {
      const idx = pixels[i];
      const o = i * 4;
      if (idx === 0) { imgData.data[o + 3] = 0; continue; }
      const hex = palette.colors[idx - 1] || '000000';
      imgData.data[o] = parseInt(hex.slice(0, 2), 16);
      imgData.data[o + 1] = parseInt(hex.slice(2, 4), 16);
      imgData.data[o + 2] = parseInt(hex.slice(4, 6), 16);
      imgData.data[o + 3] = 255;
    }
    this.offCtx.putImageData(imgData, 0, 0);

    this.canvas.width = width * this.zoom;
    this.canvas.height = height * this.zoom;
    this.ctx.imageSmoothingEnabled = false;
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    this.ctx.drawImage(this.offscreen, 0, 0, width, height, 0, 0, this.canvas.width, this.canvas.height);
    this._drawSelectionOverlay();
  }

  _paintAt(x, y, colorIndex) {
    const half = Math.floor(this.brushSize / 2);
    const pixels = this.project.currentFrame.activeLayer.pixels;
    const { width, height } = this.project;
    for (let dy = -half; dy <= half; dy++) {
      for (let dx = -half; dx <= half; dx++) {
        const px = x + dx, py = y + dy;
        if (px >= 0 && py >= 0 && px < width && py < height) pixels[py * width + px] = colorIndex;
      }
    }
  }

  _floodFill(x, y, colorIndex) {
    const { width, height } = this.project;
    const pixels = this.project.currentFrame.activeLayer.pixels;
    const target = pixels[y * width + x];
    if (target === colorIndex) return;
    const stack = [[x, y]];
    while (stack.length) {
      const [cx, cy] = stack.pop();
      if (cx < 0 || cy < 0 || cx >= width || cy >= height) continue;
      const i = cy * width + cx;
      if (pixels[i] !== target) continue;
      pixels[i] = colorIndex;
      stack.push([cx + 1, cy], [cx - 1, cy], [cx, cy + 1], [cx, cy - 1]);
    }
  }

  _drawLine(pixels, width, height, x0, y0, x1, y1, colorIndex) {
    let dx = Math.abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    let dy = -Math.abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    let err = dx + dy;
    for (;;) {
      if (x0 >= 0 && y0 >= 0 && x0 < width && y0 < height) pixels[y0 * width + x0] = colorIndex;
      if (x0 === x1 && y0 === y1) break;
      const e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  _drawRect(pixels, width, height, x0, y0, x1, y1, colorIndex, filled) {
    const minX = Math.min(x0, x1), maxX = Math.max(x0, x1);
    const minY = Math.min(y0, y1), maxY = Math.max(y0, y1);
    for (let y = minY; y <= maxY; y++) {
      for (let x = minX; x <= maxX; x++) {
        if (!filled && x !== minX && x !== maxX && y !== minY && y !== maxY) continue;
        if (x >= 0 && y >= 0 && x < width && y < height) pixels[y * width + x] = colorIndex;
      }
    }
  }

  _drawCircle(pixels, width, height, cx, cy, r, colorIndex, filled) {
    for (let y = -r; y <= r; y++) {
      for (let x = -r; x <= r; x++) {
        const d = Math.sqrt(x * x + y * y);
        const onEdge = d <= r && d >= r - 1;
        if (filled ? d <= r : onEdge) {
          const px = cx + x, py = cy + y;
          if (px >= 0 && py >= 0 && px < width && py < height) pixels[py * width + px] = colorIndex;
        }
      }
    }
  }

  _bindEvents() {
    let startPos = null;
    const getPixel = (e) => this.screenToPixel(e.clientX, e.clientY);

    this.canvas.addEventListener('mousedown', (e) => {
      const { x, y } = getPixel(e);
      if (!this.inBounds(x, y)) return;
      this.drawing = true;
      const eraseMode = e.button === 2;
      const colorIndex = eraseMode ? 0 : this.currentColorIndex;

      if (this.tool === 'brush' || this.tool === 'eraser') {
        this._paintAt(x, y, this.tool === 'eraser' ? 0 : colorIndex);
        this.render(); this._notifyChange();
      } else if (this.tool === 'fill') {
        this._floodFill(x, y, this.tool === 'eraser' ? 0 : colorIndex);
        this.render(); this._notifyChange();
        this.drawing = false;
      } else if (this.tool === 'picker') {
        const idx = this.project.currentFrame.flatten()[y * this.project.width + x];
        if (idx !== 0) {
          this.currentColorIndex = idx;
          if (this.onColorPicked) this.onColorPicked(this.project.palette.colorAt(idx));
        }
        this.drawing = false;
      } else if (this.tool === 'rect' || this.tool === 'circle' || this.tool === 'line') {
        startPos = { x, y };
        this.previewPixels = this.project.currentFrame.activeLayer.pixels.slice();
      } else if (this.tool === 'select') {
        this._selectMouseDown(x, y, e);
      }
      e.preventDefault();
    });

    this.canvas.addEventListener('mousemove', (e) => {
      if (!this.drawing) return;
      const { x, y } = getPixel(e);
      if (this.tool === 'brush' || this.tool === 'eraser') {
        if (this.inBounds(x, y)) {
          this._paintAt(x, y, this.tool === 'eraser' ? 0 : this.currentColorIndex);
          this.render();
        }
      } else if ((this.tool === 'rect' || this.tool === 'circle' || this.tool === 'line') && startPos) {
        const base = this.project.currentFrame.activeLayer.pixels.slice();
        const { width, height } = this.project;
        const cx = clamp(x, 0, width - 1), cy = clamp(y, 0, height - 1);
        if (this.tool === 'rect') this._drawRect(base, width, height, startPos.x, startPos.y, cx, cy, this.currentColorIndex, e.shiftKey);
        else if (this.tool === 'line') this._drawLine(base, width, height, startPos.x, startPos.y, cx, cy, this.currentColorIndex);
        else this._drawCircle(base, width, height, startPos.x, startPos.y, Math.round(Math.hypot(cx - startPos.x, cy - startPos.y)), this.currentColorIndex, e.shiftKey);
        this.previewPixels = base;
        this.render();
      } else if (this.tool === 'select') {
        this._selectMouseMove(x, y, e);
      }
    });

    window.addEventListener('mouseup', (e) => {
      if (!this.drawing) return;
      if ((this.tool === 'rect' || this.tool === 'circle' || this.tool === 'line') && this.previewPixels) {
        this.project.currentFrame.activeLayer.pixels.set(this.previewPixels);
        this.previewPixels = null;
        this._notifyChange();
      } else if (this.tool === 'select') {
        this._selectMouseUp(e);
      }
      this.drawing = false;
      startPos = null;
      this.render();
    });

    this.canvas.addEventListener('contextmenu', (e) => e.preventDefault());
  }

  _notifyChange() { if (this.onChange) this.onChange(); }
  setZoom(z) { this.zoom = clamp(z, 2, 64); this.render(); }
  // 선택(드래그) 툴 - 다음 단계에서 실제 내용을 채운다.
  // 선택 영역 경계를 점선으로 표시한다.
  _drawSelectionOverlay() {
    if (!this.selection) return;
    const { x, y, w, h } = this.selection;
    this.ctx.save();
    this.ctx.strokeStyle = '#6c8cff';
    this.ctx.lineWidth = 1.5;
    this.ctx.setLineDash([4, 3]);
    this.ctx.strokeRect(x * this.zoom + 0.75, y * this.zoom + 0.75, w * this.zoom - 1.5, h * this.zoom - 1.5);
    this.ctx.restore();
  }

  _pointInSelection(x, y) {
    const s = this.selection;
    return !!s && x >= s.x && x < s.x + s.w && y >= s.y && y < s.y + s.h;
  }

  _selectMouseDown(x, y, e) {
    if (this.ctrlSHeld && this._pointInSelection(x, y)) {
      // Ctrl+S 를 누른 채 선택 영역 안에서 드래그 시작 -> 내용물을 떼어내 이동시키는 모드
      this._moveMode = true;
      this._moveStart = { x, y };
      this._moveOrigin = { x: this.selection.x, y: this.selection.y };
      const layer = this.project.currentFrame.activeLayer;
      const { width } = this.project;
      const { w, h } = this.selection;
      this._moveBuffer = new Uint8Array(w * h);
      for (let yy = 0; yy < h; yy++)
        for (let xx = 0; xx < w; xx++)
          this._moveBuffer[yy * w + xx] = layer.pixels[(this.selection.y + yy) * width + (this.selection.x + xx)];
      this.previewPixels = layer.pixels.slice();
      for (let yy = 0; yy < h; yy++)
        for (let xx = 0; xx < w; xx++)
          this.previewPixels[(this.selection.y + yy) * width + (this.selection.x + xx)] = 0;
    } else {
      // 새 선택 영역을 드래그로 그리기 시작
      this._moveMode = false;
      this._selStart = { x, y };
      this.selection = { x, y, w: 1, h: 1 };
    }
    this.render();
  }

  _selectMouseMove(x, y, e) {
    const { width, height } = this.project;
    if (this._moveMode && this._moveBuffer) {
      const { w, h } = this.selection;
      const dx = x - this._moveStart.x, dy = y - this._moveStart.y;
      const nx = clamp(this._moveOrigin.x + dx, 0, width - w);
      const ny = clamp(this._moveOrigin.y + dy, 0, height - h);
      const base = this.project.currentFrame.activeLayer.pixels.slice();
      for (let yy = 0; yy < h; yy++)
        for (let xx = 0; xx < w; xx++)
          base[(this.selection.y + yy) * width + (this.selection.x + xx)] = 0;
      for (let yy = 0; yy < h; yy++) {
        for (let xx = 0; xx < w; xx++) {
          const v = this._moveBuffer[yy * w + xx];
          if (v !== 0) base[(ny + yy) * width + (nx + xx)] = v;
        }
      }
      this._pendingMovePos = { x: nx, y: ny };
      this.previewPixels = base;
      this.render();
    } else if (this._selStart) {
      const cx = clamp(x, 0, width - 1), cy = clamp(y, 0, height - 1);
      const x0 = Math.min(this._selStart.x, cx), x1 = Math.max(this._selStart.x, cx);
      const y0 = Math.min(this._selStart.y, cy), y1 = Math.max(this._selStart.y, cy);
      this.selection = { x: x0, y: y0, w: x1 - x0 + 1, h: y1 - y0 + 1 };
      this.render();
    }
  }

  _selectMouseUp(e) {
    if (this._moveMode && this.previewPixels) {
      this.project.currentFrame.activeLayer.pixels.set(this.previewPixels);
      if (this._pendingMovePos) { this.selection.x = this._pendingMovePos.x; this.selection.y = this._pendingMovePos.y; }
      this.previewPixels = null;
      this._moveMode = false; this._moveBuffer = null; this._pendingMovePos = null;
      this._notifyChange();
    }
    this._selStart = null;
    this.render();
  }

  // Ctrl+C: 선택 영역의 내용을 클립보드에 복사
  copySelection() {
    if (!this.selection) return false;
    const { width } = this.project;
    const layer = this.project.currentFrame.activeLayer;
    const { x, y, w, h } = this.selection;
    const buf = new Uint8Array(w * h);
    for (let yy = 0; yy < h; yy++)
      for (let xx = 0; xx < w; xx++)
        buf[yy * w + xx] = layer.pixels[(y + yy) * width + (x + xx)];
    this.clipboard = { w, h, pixels: buf };
    return true;
  }

  // Ctrl+V: 클립보드 내용을 (선택 영역이 있으면 그 위치, 없으면 좌상단에) 붙여넣기
  pasteClipboard() {
    if (!this.clipboard) return false;
    const { width, height } = this.project;
    const layer = this.project.currentFrame.activeLayer;
    const at = this.selection ? { x: this.selection.x, y: this.selection.y } : { x: 0, y: 0 };
    const { w, h, pixels } = this.clipboard;
    for (let yy = 0; yy < h; yy++) {
      for (let xx = 0; xx < w; xx++) {
        const px = at.x + xx, py = at.y + yy;
        if (px >= 0 && py >= 0 && px < width && py < height) layer.pixels[py * width + px] = pixels[yy * w + xx];
      }
    }
    this.selection = { x: at.x, y: at.y, w, h };
    this.render();
    return true;
  }

  clearSelection() { this.selection = null; this.render(); }
}

/* --------------------------------------------------------------------------
   6) PlaybackController
   -------------------------------------------------------------------------- */
class PlaybackController {
  constructor(project, onFrame) {
    this.project = project;
    this.onFrame = onFrame;
    this.playing = false;
    this.loop = true;
    this.fps = project.fps;
    this._timer = null;
  }
  play() {
    if (this.playing) return;
    this.playing = true;
    const step = () => {
      if (!this.playing) return;
      let next = this.project.currentFrameIndex + 1;
      if (next >= this.project.frames.length) {
        if (this.loop) next = 0; else { this.pause(); return; }
      }
      this.project.currentFrameIndex = next;
      this.onFrame();
      this._timer = setTimeout(step, 1000 / this.fps);
    };
    this._timer = setTimeout(step, 1000 / this.fps);
  }
  pause() { this.playing = false; if (this._timer) clearTimeout(this._timer); }
  stop() { this.pause(); this.project.currentFrameIndex = 0; this.onFrame(); }
  first() { this.project.currentFrameIndex = 0; this.onFrame(); }
  last() { this.project.currentFrameIndex = this.project.frames.length - 1; this.onFrame(); }
}

/* --------------------------------------------------------------------------
   6.4) 재생 모드: 편집기와 완전히 분리된 최소 뷰어.
   .jv 파일을 고르면 JVFile.parse() 결과(팔레트+프레임 원본 배열)를 바로 순환
   재생한다. Project/Frame/Layer 등 편집용 객체를 전혀 만들지 않는다.
   -------------------------------------------------------------------------- */
class PlaybackModeController {
  constructor(canvasEl, infoEl, emptyEl) {
    this.canvas = canvasEl;
    this.ctx = canvasEl.getContext('2d');
    this.infoEl = infoEl;
    this.emptyEl = emptyEl;
    this.offscreen = document.createElement('canvas');
    this.offCtx = this.offscreen.getContext('2d');
    this.data = null;
    this.idx = 0;
    this._timer = null;
  }
  load(text) {
    this.stop();
    const parsed = JVFile.parse(text);
    this.data = parsed;
    this.idx = 0;
    this.infoEl.textContent = parsed.width + '×' + parsed.height + ' · ' + parsed.frames.length + '프레임 · ' + parsed.fps + 'fps';
    this.canvas.style.display = 'block';
    this.emptyEl.classList.add('hidden');
    this._renderFrame();
    this.play();
  }
  _renderFrame() {
    if (!this.data) return;
    const { width, height, paletteColors, frames } = this.data;
    this.offscreen.width = width; this.offscreen.height = height;
    const imgData = this.offCtx.createImageData(width, height);
    const pixels = frames[this.idx];
    for (let i = 0; i < pixels.length; i++) {
      const idx = pixels[i], o = i * 4;
      if (idx === 0) { imgData.data[o + 3] = 0; continue; }
      const hex = paletteColors[idx - 1] || '000000';
      imgData.data[o] = parseInt(hex.slice(0, 2), 16);
      imgData.data[o + 1] = parseInt(hex.slice(2, 4), 16);
      imgData.data[o + 2] = parseInt(hex.slice(4, 6), 16);
      imgData.data[o + 3] = 255;
    }
    this.offCtx.putImageData(imgData, 0, 0);

    const stage = this.canvas.parentElement;
    const zoom = Math.max(1, Math.floor(Math.min((stage.clientWidth - 40) / width, (stage.clientHeight - 40) / height)));
    this.canvas.width = width * zoom; this.canvas.height = height * zoom;
    this.ctx.imageSmoothingEnabled = false;
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    this.ctx.drawImage(this.offscreen, 0, 0, width, height, 0, 0, this.canvas.width, this.canvas.height);
  }
  play() {
    if (!this.data || this.data.frames.length === 0) return;
    const step = () => {
      this.idx = (this.idx + 1) % this.data.frames.length;
      this._renderFrame();
      this._timer = setTimeout(step, 1000 / Math.max(1, this.data.fps));
    };
    this._timer = setTimeout(step, 1000 / Math.max(1, this.data.fps));
  }
  stop() { if (this._timer) clearTimeout(this._timer); this._timer = null; }
  reset() {
    this.stop();
    this.data = null;
    this.canvas.style.display = 'none';
    this.emptyEl.classList.remove('hidden');
    this.infoEl.textContent = '';
  }
}

/* --------------------------------------------------------------------------
   6.5) MP4 가져오기: 비디오 프레임 추출 + 색상 양자화(k-means)
   픽셀아트 전용 도구였던 것을 실사/영상 소스도 받아들이도록 확장하는 부분.
   팔레트가 최대 PALETTE_MAX(7)색으로 제한되므로, 비디오의 실제 색상 분포에서
   대표색 7개를 뽑아내는 k-means 군집화를 거쳐 각 프레임을 그 팔레트로
   근사(최근접 색상 매핑)한다.
   -------------------------------------------------------------------------- */

// k-means++ 초기화 + Lloyd 반복으로 RGB 샘플들을 k개의 대표색으로 군집화한다.
function kMeansPalette(samples, k, iterations) {
  if (samples.length === 0) return [[0, 0, 0]];
  k = Math.min(k, samples.length);
  const dist2 = (a, b) => (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2;

  const centers = [samples[Math.floor(Math.random() * samples.length)].slice()];
  while (centers.length < k) {
    const dists = samples.map(s => {
      let min = Infinity;
      for (const c of centers) { const d = dist2(s, c); if (d < min) min = d; }
      return min;
    });
    const sum = dists.reduce((a, b) => a + b, 0);
    if (sum === 0) { centers.push(samples[Math.floor(Math.random() * samples.length)].slice()); continue; }
    let r = Math.random() * sum, idx = 0;
    for (; idx < dists.length - 1; idx++) { r -= dists[idx]; if (r <= 0) break; }
    centers.push(samples[idx].slice());
  }

  const assign = new Array(samples.length).fill(0);
  for (let iter = 0; iter < iterations; iter++) {
    for (let i = 0; i < samples.length; i++) {
      let best = 0, bestD = Infinity;
      for (let c = 0; c < centers.length; c++) {
        const d = dist2(samples[i], centers[c]);
        if (d < bestD) { bestD = d; best = c; }
      }
      assign[i] = best;
    }
    const sums = centers.map(() => [0, 0, 0, 0]);
    for (let i = 0; i < samples.length; i++) {
      const a = assign[i], s = samples[i], acc = sums[a];
      acc[0] += s[0]; acc[1] += s[1]; acc[2] += s[2]; acc[3]++;
    }
    for (let c = 0; c < centers.length; c++) {
      if (sums[c][3] > 0) centers[c] = [sums[c][0] / sums[c][3], sums[c][1] / sums[c][3], sums[c][2] / sums[c][3]];
    }
  }
  return centers.map(c => [Math.round(c[0]), Math.round(c[1]), Math.round(c[2])]);
}

function nearestColorIndex(r, g, b, centers) {
  let best = 0, bestD = Infinity;
  for (let i = 0; i < centers.length; i++) {
    const c = centers[i];
    const d = (r - c[0]) ** 2 + (g - c[1]) ** 2 + (b - c[2]) ** 2;
    if (d < bestD) { bestD = d; best = i; }
  }
  return best;
}

function rgbToHex(r, g, b) { return byteHex(r) + byteHex(g) + byteHex(b); }

// RGB(0-255) -> HSV(h:0-360, s/v:0-1). 스포이드로 고른 색을 휠 위치에 반영할 때 사용.
function rgbToHsv(r, g, b) {
  r /= 255; g /= 255; b /= 255;
  const max = Math.max(r, g, b), min = Math.min(r, g, b), d = max - min;
  let h = 0;
  if (d !== 0) {
    if (max === r) h = 60 * (((g - b) / d) % 6);
    else if (max === g) h = 60 * ((b - r) / d + 2);
    else h = 60 * ((r - g) / d + 4);
  }
  if (h < 0) h += 360;
  const s = max === 0 ? 0 : d / max;
  const v = max;
  return [h, s, v];
}

// HSV(h: 0-360, s/v: 0-1) -> RGB(0-255). 이비스페인트 스타일 원형 색상 휠에서 사용.
function hsvToRgb(h, s, v) {
  h = ((h % 360) + 360) % 360;
  const c = v * s;
  const x = c * (1 - Math.abs((h / 60) % 2 - 1));
  const m = v - c;
  let r, g, b;
  if (h < 60) [r, g, b] = [c, x, 0];
  else if (h < 120) [r, g, b] = [x, c, 0];
  else if (h < 180) [r, g, b] = [0, c, x];
  else if (h < 240) [r, g, b] = [0, x, c];
  else if (h < 300) [r, g, b] = [x, 0, c];
  else [r, g, b] = [c, 0, x];
  return [Math.round((r + m) * 255), Math.round((g + m) * 255), Math.round((b + m) * 255)];
}

// <video>+오프스크린 캔버스로 project.fps 간격마다 프레임을 잘라 ImageData 배열로 반환한다.
// 비디오는 캔버스 해상도에 꽉 차도록(cover) 중앙 크롭해서 그린다.
function extractVideoFrames(file, width, height, fps, maxFrames, onProgress) {
  return new Promise((resolve, reject) => {
    const url = URL.createObjectURL(file);
    const video = document.createElement('video');
    video.muted = true; video.playsInline = true; video.preload = 'auto';
    video.src = url;

    video.addEventListener('error', () => { URL.revokeObjectURL(url); reject(new Error('비디오 파일을 읽을 수 없습니다.')); }, { once: true });

    video.addEventListener('loadedmetadata', async () => {
      try {
        const duration = video.duration;
        const frameCount = Math.max(1, Math.min(maxFrames, Math.floor(duration * fps)));
        const off = document.createElement('canvas');
        off.width = width; off.height = height;
        const octx = off.getContext('2d');
        const rawFrames = [];
        const vw = video.videoWidth, vh = video.videoHeight;
        const scale = Math.max(width / vw, height / vh);
        const sw = width / scale, sh = height / scale;
        const sx = (vw - sw) / 2, sy = (vh - sh) / 2;

        for (let i = 0; i < frameCount; i++) {
          const t = Math.min(Math.max(duration - 0.001, 0), i / fps);
          await new Promise((res) => {
            const onSeeked = () => { video.removeEventListener('seeked', onSeeked); res(); };
            video.addEventListener('seeked', onSeeked, { once: true });
            video.currentTime = t;
          });
          octx.clearRect(0, 0, width, height);
          octx.drawImage(video, sx, sy, sw, sh, 0, 0, width, height);
          rawFrames.push(octx.getImageData(0, 0, width, height));
          if (onProgress) onProgress(i + 1, frameCount);
        }
        URL.revokeObjectURL(url);
        resolve(rawFrames);
      } catch (err) {
        URL.revokeObjectURL(url);
        reject(err);
      }
    }, { once: true });
  });
}

// 추출된 프레임들을 k-means로 만든 팔레트(최대 PALETTE_MAX색)에 매핑해 project에 반영한다.
// project.palette / project.frames 를 완전히 교체한다 (호출 전에 사용자 확인을 받는다).
async function importMp4ToProject(file, project, onProgress) {
  const { width, height, fps, maxFrames } = project;
  const rawFrames = await extractVideoFrames(file, width, height, fps, maxFrames,
    (done, total) => onProgress('프레임 추출 중... ' + done + '/' + total));

  const samples = [];
  const SAMPLE_TARGET = 6000;
  const totalPixels = rawFrames.length * width * height;
  const stride = Math.max(1, Math.floor(totalPixels / SAMPLE_TARGET));
  let counter = 0;
  for (const imgData of rawFrames) {
    const d = imgData.data;
    for (let p = 0; p < width * height; p++) {
      counter++;
      if (counter % stride !== 0) continue;
      const o = p * 4;
      if (d[o + 3] < 16) continue; // 거의 투명한 픽셀은 팔레트 후보에서 제외
      samples.push([d[o], d[o + 1], d[o + 2]]);
    }
  }
  onProgress('색상 팔레트 계산 중 (k-means, ' + samples.length + '개 샘플)...');
  const centers = kMeansPalette(samples, PALETTE_MAX, 8);

  project.palette = new Palette();
  project.palette.colors = centers.map(c => rgbToHex(c[0], c[1], c[2]));

  const newFrames = [];
  for (let fi = 0; fi < rawFrames.length; fi++) {
    const d = rawFrames[fi].data;
    const frame = new Frame(width, height);
    const pixels = frame.activeLayer.pixels;
    for (let p = 0; p < width * height; p++) {
      const o = p * 4;
      pixels[p] = (d[o + 3] < 16) ? 0 : (nearestColorIndex(d[o], d[o + 1], d[o + 2], centers) + 1);
    }
    newFrames.push(frame);
    if (fi % 4 === 0) onProgress('프레임 변환 중... ' + (fi + 1) + '/' + rawFrames.length);
  }

  project.frames = newFrames;
  project.currentFrameIndex = 0;
  return project;
}

/* --------------------------------------------------------------------------
   7) UIController: DOM 배선
   -------------------------------------------------------------------------- */
class UIController {
  constructor() {
    this.logger = new Logger(document.getElementById('logBody'));
    this.project = new Project(32, 32, 12, 240);
    this.canvasEl = document.getElementById('mainCanvas');
    this.editor = new CanvasEditor(this.canvasEl, this.project, this.logger);
    this.playback = new PlaybackController(this.project, () => this._onFrameChanged());
    this.currentFileName = 'untitled.jv';

    this.editor.onChange = () => this._refreshFramePropsAndThumb();
    this.editor.onColorPicked = (hex) => { if (hex) document.getElementById('colorPickerInput').value = hex; };

    this._bindTopbar();
    this._bindCanvasToolbar();
    this._bindPlaybar();
    this._bindRightPanel();
    this._bindLeftPanel();
    this._buildCompressorList();
    this._bindShortcuts();
    this._bindPlaybackMode();
    this._bindColorWheel();

    this._syncSettingsInputs();
    this._renderFrameList();
    this._refreshPalette();
    this.editor.render();
    this._refreshFramePropsAndThumb();
    this.logger.ok('JVideo Studio 준비 완료. 새 프로젝트가 생성되었습니다.');
  }

  /* ---------- 상단 파일 메뉴 ---------- */
  _bindTopbar() {
    document.getElementById('btnNew').onclick = () => this._newProject();
    document.getElementById('btnOpen').onclick = () => document.getElementById('fileInputOpen').click();
    document.getElementById('btnImport').onclick = () => document.getElementById('fileInputImport').click();
    document.getElementById('btnSave').onclick = () => this._saveFile(this.currentFileName);
    document.getElementById('btnSaveAs').onclick = () => {
      const name = prompt('저장할 파일 이름을 입력하세요', this.currentFileName.replace(/\.jv$/, '')) ;
      if (name === null) return;
      this.currentFileName = (name.trim() || 'untitled') + '.jv';
      this._saveFile(this.currentFileName);
    };
    document.getElementById('btnExport').onclick = () => this._saveFile(this.currentFileName, true);

    document.getElementById('fileInputOpen').addEventListener('change', (e) => this._loadFile(e, false));
    document.getElementById('fileInputImport').addEventListener('change', (e) => this._loadFile(e, true));

    document.getElementById('btnImportMp4').onclick = () => document.getElementById('fileInputMp4').click();
    document.getElementById('fileInputMp4').addEventListener('change', (e) => this._importMp4(e));

    document.getElementById('btnApplySettings').onclick = () => {
      const w = clamp(parseInt(document.getElementById('cfgWidth').value, 10) || 32, 1, 512);
      const h = clamp(parseInt(document.getElementById('cfgHeight').value, 10) || 32, 1, 512);
      const fps = clamp(parseInt(document.getElementById('cfgFps').value, 10) || 12, 1, 60);
      const max = clamp(parseInt(document.getElementById('cfgMax').value, 10) || 240, 1, 9999);
      if (w !== this.project.width || h !== this.project.height) {
        this.project.resizeAll(w, h);
        this.editor.selection = null; // 이전 해상도 기준 선택영역은 더 이상 유효하지 않으므로 해제
        this.editor.previewPixels = null;
      }
      this.project.fps = fps; this.project.maxFrames = max;
      this.playback.fps = fps;
      document.getElementById('playFps').value = fps;
      this._syncSettingsInputs();
      this.editor.render();
      this._renderFrameList();
      this.logger.info('영상 설정이 적용되었습니다 (' + w + '×' + h + ', ' + fps + 'fps, 최대 ' + max + '프레임).');
    };

    document.getElementById('projectName').addEventListener('change', (e) => {
      this.project.name = e.target.value.trim() || 'untitled';
    });

    document.getElementById('btnClearLog').onclick = () => { document.getElementById('logBody').innerHTML = ''; };
  }

  _newProject() {
    if (!confirm('새 프로젝트를 시작할까요? 저장하지 않은 변경사항은 사라집니다.')) return;
    this.playback.pause();
    this.project = new Project(32, 32, 12, 240);
    this.editor.setProject(this.project);
    this.playback = new PlaybackController(this.project, () => this._onFrameChanged());
    this.currentFileName = 'untitled.jv';
    document.getElementById('projectName').value = 'untitled';
    this._syncSettingsInputs();
    this._renderFrameList();
    this._refreshPalette();
    this._refreshFramePropsAndThumb();
    this.logger.ok('새 프로젝트를 생성했습니다.');
  }

  _saveFile(filename, isExport) {
    try {
      const text = this.project.toJVFileText();
      const blob = new Blob([text], { type: 'text/plain' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url; a.download = filename;
      document.body.appendChild(a); a.click(); document.body.removeChild(a);
      URL.revokeObjectURL(url);
      this.logger.ok((isExport ? '내보내기 완료: ' : '저장 완료: ') + filename + ' (' + text.length + ' bytes)');
    } catch (err) {
      this.logger.error('저장 실패: ' + err.message);
    }
  }

  _loadFile(e, isImport) {
    const file = e.target.files[0];
    e.target.value = '';
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => {
      try {
        const proj = Project.fromJVFileText(reader.result);
        proj.name = file.name.replace(/\.jv$/i, '');
        this.playback.pause();
        this.project = proj;
        this.editor.setProject(this.project);
        this.playback = new PlaybackController(this.project, () => this._onFrameChanged());
        this.currentFileName = file.name.endsWith('.jv') ? file.name : file.name + '.jv';
        document.getElementById('projectName').value = this.project.name;
        this._syncSettingsInputs();
        this._renderFrameList();
        this._refreshPalette();
        this._refreshFramePropsAndThumb();
        this.logger.ok((isImport ? '가져오기 완료: ' : '열기 완료: ') + file.name + ' (' + this.project.frames.length + ' 프레임)');
      } catch (err) {
        this.logger.error('JV 파일을 읽는 중 오류: ' + err.message);
      }
    };
    reader.readAsText(file);
  }

  async _importMp4(e) {
    const file = e.target.files[0];
    e.target.value = '';
    if (!file) return;
    if (!confirm('MP4를 가져오면 현재 프로젝트의 모든 프레임과 팔레트가 교체됩니다 (해상도 ' + this.project.width + '×' + this.project.height + ', ' + this.project.fps + 'fps 기준으로 추출). 계속할까요?')) return;

    const progressEl = document.getElementById('mp4Progress');
    progressEl.classList.remove('hidden');
    const onProgress = (msg) => { progressEl.textContent = msg; };
    this.playback.pause(); document.getElementById('btnPlay').textContent = '▶';
    this.logger.info('MP4 가져오기 시작: ' + file.name);
    try {
      await importMp4ToProject(file, this.project, onProgress);
      this.editor.setProject(this.project);
      this._syncSettingsInputs();
      this._renderFrameList();
      this._refreshPalette();
      this._refreshFramePropsAndThumb();
      this.logger.ok('MP4 가져오기 완료: ' + this.project.frames.length + '프레임, 팔레트 ' +
        this.project.palette.colors.length + '색으로 양자화됨 (최대 ' + PALETTE_MAX + '색 제한)');
    } catch (err) {
      this.logger.error('MP4 가져오기 실패: ' + err.message);
    } finally {
      progressEl.classList.add('hidden');
    }
  }

  _syncSettingsInputs() {
    document.getElementById('cfgWidth').value = this.project.width;
    document.getElementById('cfgHeight').value = this.project.height;
    document.getElementById('cfgFps').value = this.project.fps;
    document.getElementById('cfgMax').value = this.project.maxFrames;
    document.getElementById('playFps').value = this.project.fps;
    document.getElementById('resLabel').textContent = this.project.width + '×' + this.project.height;
    document.getElementById('fpsLabel').textContent = this.project.fps;
  }

  /* ---------- 캔버스 툴바 ---------- */
  _bindCanvasToolbar() {
    document.querySelectorAll('.tool-btn[data-tool]').forEach(btn => {
      btn.addEventListener('click', () => {
        document.querySelectorAll('.tool-btn[data-tool]').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        this.editor.tool = btn.dataset.tool;
      });
    });
    document.getElementById('colorPickerInput').addEventListener('input', (e) => {
      const hex = e.target.value.slice(1).toUpperCase();
      const idx = this.project.palette.getOrAdd(hex);
      if (idx === null) {
        this.logger.warn('팔레트가 가득 찼습니다 (최대 ' + PALETTE_MAX + '색). 기존 색을 지우고 다시 시도하세요.');
        document.getElementById('colorPickerInput').value = this.project.palette.colorAt(this.editor.currentColorIndex) || '#ff5c7a';
        return;
      }
      this.editor.currentColorIndex = idx;
      document.getElementById('colorSwatch').style.background = e.target.value;
      this._refreshPalette();
    });
    document.getElementById('btnZoomIn').onclick = () => { this.editor.setZoom(this.editor.zoom + 2); this._updateZoomLabel(); };
    document.getElementById('btnZoomOut').onclick = () => { this.editor.setZoom(this.editor.zoom - 2); this._updateZoomLabel(); };
    document.getElementById('btnZoomFit').onclick = () => {
      const wrap = document.getElementById('canvasWrap');
      const z = Math.max(2, Math.floor(Math.min(wrap.clientWidth / this.project.width, wrap.clientHeight / this.project.height)) - 1);
      this.editor.setZoom(z); this._updateZoomLabel();
    };
    document.getElementById('btnPan').addEventListener('click', () => {
      this.logger.info('팬: 캔버스 위에서 스페이스바를 누른 채 드래그하면 화면을 이동할 수 있습니다.');
    });
    this._enableSpacePan();
    this._updateZoomLabel();
  }

  _enableSpacePan() {
    const wrap = document.getElementById('canvasWrap');
    let spaceDown = false, panning = false, lastX = 0, lastY = 0;
    window.addEventListener('keydown', (e) => { if (e.code === 'Space') spaceDown = true; });
    window.addEventListener('keyup', (e) => { if (e.code === 'Space') spaceDown = false; });
    wrap.addEventListener('mousedown', (e) => {
      if (!spaceDown) return;
      panning = true; lastX = e.clientX; lastY = e.clientY;
      e.preventDefault();
    });
    window.addEventListener('mousemove', (e) => {
      if (!panning) return;
      wrap.scrollLeft -= (e.clientX - lastX);
      wrap.scrollTop -= (e.clientY - lastY);
      lastX = e.clientX; lastY = e.clientY;
    });
    window.addEventListener('mouseup', () => { panning = false; });
  }

  _updateZoomLabel() { document.getElementById('zoomLabel').textContent = (this.editor.zoom * 100) + '%'; }

  /* ---------- 재생바 ---------- */
  _bindPlaybar() {
    document.getElementById('btnPlay').addEventListener('click', () => {
      if (this.playback.playing) { this.playback.pause(); document.getElementById('btnPlay').textContent = '▶'; }
      else { this.playback.play(); document.getElementById('btnPlay').textContent = '⏸'; }
    });
    document.getElementById('btnStop').onclick = () => { this.playback.stop(); document.getElementById('btnPlay').textContent = '▶'; };
    document.getElementById('btnFirst').onclick = () => this.playback.first();
    document.getElementById('btnLast').onclick = () => this.playback.last();
    document.getElementById('chkLoop').addEventListener('change', (e) => { this.playback.loop = e.target.checked; });
    document.getElementById('playFps').addEventListener('change', (e) => {
      const fps = clamp(parseInt(e.target.value, 10) || 12, 1, 60);
      this.playback.fps = fps; this.project.fps = fps;
      document.getElementById('cfgFps').value = fps;
      document.getElementById('fpsLabel').textContent = fps;
    });
    document.getElementById('scrubber').addEventListener('input', (e) => {
      this.playback.pause(); document.getElementById('btnPlay').textContent = '▶';
      this.project.currentFrameIndex = parseInt(e.target.value, 10);
      this._onFrameChanged();
    });
  }

  _onFrameChanged() {
    this.editor.render();
    document.getElementById('scrubber').value = this.project.currentFrameIndex;
    document.getElementById('frameCounter').textContent = (this.project.currentFrameIndex + 1) + ' / ' + this.project.frames.length;
    this._highlightActiveFrame();
    this._refreshFramePropsAndThumb(false);
  }

  /* ---------- 우측 속성 패널 ---------- */
  _bindRightPanel() {
    document.getElementById('brushSizeRange').addEventListener('input', (e) => {
      this.editor.brushSize = parseInt(e.target.value, 10);
      document.getElementById('brushSizeVal').textContent = this.editor.brushSize + 'px';
    });
    document.getElementById('btnLayerAdd').onclick = () => {
      this.project.currentFrame.addLayer();
      this._renderLayerList();
      this.editor.render();
      this.logger.info('레이어를 추가했습니다. (저장 시 합쳐집니다)');
    };
    document.getElementById('btnLayerDel').onclick = () => {
      if (this.project.currentFrame.deleteLayer()) {
        this._renderLayerList();
        this.editor.render();
        this.logger.info('레이어를 삭제했습니다.');
      } else this.logger.warn('마지막 남은 레이어는 삭제할 수 없습니다.');
    };
    document.getElementById('btnLayerUp').onclick = () => {
      const f = this.project.currentFrame;
      if (f.moveLayer(f.activeLayerIndex, 1)) { this._renderLayerList(); this.editor.render(); }
    };
    document.getElementById('btnLayerDown').onclick = () => {
      const f = this.project.currentFrame;
      if (f.moveLayer(f.activeLayerIndex, -1)) { this._renderLayerList(); this.editor.render(); }
    };
  }

  // 레이어 목록은 위쪽이 화면상 위 레이어가 되도록 인덱스 역순으로 표시한다.
  // 레이어 구성(개수/이름/순서/가시성)은 편집 세션에만 존재하며 .jv 파일에는 저장되지 않는다.
  _renderLayerList() {
    const ul = document.getElementById('layerList');
    ul.innerHTML = '';
    const frame = this.project.currentFrame;
    for (let i = frame.layers.length - 1; i >= 0; i--) {
      const layer = frame.layers[i];
      const li = document.createElement('li');
      li.className = 'layer-item' + (i === frame.activeLayerIndex ? ' active' : '');
      li.innerHTML =
        '<input type="checkbox" ' + (layer.visible ? 'checked' : '') + '>' +
        '<span class="lname"></span>';
      li.querySelector('.lname').textContent = layer.name;
      li.querySelector('input').addEventListener('change', (e) => {
        layer.visible = e.target.checked;
        this.editor.render();
      });
      li.addEventListener('click', (e) => {
        if (e.target.tagName === 'INPUT') return;
        frame.activeLayerIndex = i;
        this._renderLayerList();
      });
      ul.appendChild(li);
    }
  }

  _refreshPalette() {
    const grid = document.getElementById('paletteGrid');
    grid.innerHTML = '';
    const trans = document.createElement('div');
    trans.className = 'swatch transparent-swatch';
    trans.title = '투명(지우개)';
    trans.onclick = () => { this.editor.currentColorIndex = 0; };
    grid.appendChild(trans);
    this.project.palette.colors.forEach((hex, i) => {
      const sw = document.createElement('div');
      sw.className = 'swatch';
      sw.style.background = '#' + hex;
      sw.title = '#' + hex;
      if (this.editor.currentColorIndex === i + 1) sw.classList.add('selected');
      sw.onclick = () => {
        this.editor.currentColorIndex = i + 1;
        document.getElementById('colorPickerInput').value = '#' + hex;
        document.getElementById('colorSwatch').style.background = '#' + hex;
        this._refreshPalette();
      };
      grid.appendChild(sw);
    });
    document.getElementById('paletteCountLabel').textContent = this.project.palette.colors.length + ' / ' + PALETTE_MAX;
  }

  _refreshFramePropsAndThumb(refreshList) {
    const idx = this.project.currentFrameIndex;
    const frame = this.project.currentFrame;
    document.getElementById('propFrameIdx').textContent = (idx + 1);
    document.getElementById('propFrameType').textContent = frameEncodingType(frame.flatten(), idx === 0 ? null : this.project.frames[idx - 1].flatten(), this.project.width, this.project.height);
    document.getElementById('propColorsUsed').textContent = frame.usedColorCount();
    document.getElementById('frameCountLabel').textContent = (idx + 1) + ' / ' + this.project.frames.length;
    document.getElementById('frameCounter').textContent = (idx + 1) + ' / ' + this.project.frames.length;
    document.getElementById('scrubber').max = Math.max(0, this.project.frames.length - 1);
    document.getElementById('scrubber').value = idx;
    this._renderLayerList();
    if (refreshList !== false) this._renderFrameList(); else this._updateThumb(idx);
  }

  /* ---------- 단축키 ----------
     Ctrl+C  : 선택(드래그) 영역 복사
     Ctrl+V  : 붙여넣기
     Ctrl+X  : 재생/일시정지 토글 (사용자 요청에 따른 매핑 - 일반적인 '잘라내기'가 아님)
     Ctrl+S  : 누르고 있는 동안, 선택 영역 안에서 드래그하면 내용을 이동. 브라우저 저장창은 막는다.
     Ctrl+Q + 숫자키 : 두 키를 함께 누른 상태에서 숫자키를 누르면 그 번호의 프레임으로 이동
  -------------------------------------------------------------------------- */
  _bindShortcuts() {
    let qHeld = false;
    window.addEventListener('keydown', (e) => {
      const tag = (e.target.tagName || '').toUpperCase();
      if (tag === 'INPUT' || tag === 'TEXTAREA' || e.target.isContentEditable) return; // 입력 중엔 단축키 무시
      const ctrl = e.ctrlKey || e.metaKey;
      const key = e.key.toLowerCase();

      if (key === 'q') qHeld = true;

      if (ctrl && qHeld && /^[0-9]$/.test(e.key)) {
        e.preventDefault();
        const n = parseInt(e.key, 10);
        const target = clamp(n - 1, 0, this.project.frames.length - 1);
        this.playback.pause(); document.getElementById('btnPlay').textContent = '▶';
        this.project.currentFrameIndex = target;
        this._onFrameChanged();
        return;
      }
      if (ctrl && key === 'c') {
        e.preventDefault();
        if (this.editor.copySelection()) this.logger.info('선택 영역을 복사했습니다.');
        else this.logger.warn('복사할 선택 영역이 없습니다. 먼저 선택(⬚) 툴로 드래그하세요.');
        return;
      }
      if (ctrl && key === 'v') {
        e.preventDefault();
        if (this.editor.pasteClipboard()) { this._refreshFramePropsAndThumb(false); this.logger.info('붙여넣기 했습니다.'); }
        else this.logger.warn('붙여넣을 클립보드가 비어 있습니다. 먼저 Ctrl+C로 복사하세요.');
        return;
      }
      if (ctrl && key === 'x') {
        e.preventDefault();
        document.getElementById('btnPlay').click();
        return;
      }
      if (ctrl && key === 's') {
        e.preventDefault();
        this.editor.ctrlSHeld = true;
        return;
      }
      if (key === 'escape') {
        this.editor.clearSelection();
      }
    });
    window.addEventListener('keyup', (e) => {
      const key = e.key.toLowerCase();
      if (key === 'q') qHeld = false;
      if (key === 's') this.editor.ctrlSHeld = false;
    });
  }

  /* ---------- 재생 모드 (편집기와 완전 분리된 전용 재생 화면) ---------- */
  _bindPlaybackMode() {
    const overlay = document.getElementById('playbackModeOverlay');
    const canvas = document.getElementById('playbackModeCanvas');
    const info = document.getElementById('playbackModeInfo');
    const empty = document.getElementById('playbackModeEmpty');
    this.playbackModeCtl = new PlaybackModeController(canvas, info, empty);

    document.getElementById('btnPlaybackMode').onclick = () => {
      this.playback.pause(); document.getElementById('btnPlay').textContent = '▶'; // 편집기 재생은 멈춰둔다
      overlay.classList.remove('hidden');
    };
    document.getElementById('btnPmExit').onclick = () => {
      this.playbackModeCtl.reset();
      overlay.classList.add('hidden');
    };
    document.getElementById('btnPmPick').onclick = () => document.getElementById('fileInputPlaybackMode').click();
    document.getElementById('fileInputPlaybackMode').addEventListener('change', (e) => {
      const file = e.target.files[0];
      e.target.value = '';
      if (!file) return;
      const reader = new FileReader();
      reader.onload = () => {
        try {
          this.playbackModeCtl.load(reader.result);
          this.logger.ok('재생 모드: ' + file.name + ' 재생을 시작합니다.');
        } catch (err) {
          this.logger.error('재생 모드: 파일을 읽을 수 없습니다 - ' + err.message);
        }
      };
      reader.readAsText(file);
    });
  }

  /* ---------- 이비스페인트 스타일 색상 휠 ---------- */
  _bindColorWheel() {
    const canvas = document.getElementById('colorWheelCanvas');
    const ctx = canvas.getContext('2d');
    const popover = document.getElementById('colorWheelPopover');
    const valueRange = document.getElementById('colorValueRange');
    const preview = document.getElementById('colorWheelPreview');
    const hexLabel = document.getElementById('colorWheelHex');
    const size = canvas.width, cx = size / 2, cy = size / 2, radius = size / 2 - 2;
    let hue = 0, sat = 1;

    const drawWheel = () => {
      const v = valueRange.value / 100;
      const imgData = ctx.createImageData(size, size);
      for (let y = 0; y < size; y++) {
        for (let x = 0; x < size; x++) {
          const dx = x - cx, dy = y - cy;
          const r = Math.sqrt(dx * dx + dy * dy) / radius;
          const o = (y * size + x) * 4;
          if (r > 1) { imgData.data[o + 3] = 0; continue; }
          const angle = (Math.atan2(dy, dx) * 180 / Math.PI + 360) % 360;
          const [rr, gg, bb] = hsvToRgb(angle, Math.min(r, 1), v);
          imgData.data[o] = rr; imgData.data[o + 1] = gg; imgData.data[o + 2] = bb; imgData.data[o + 3] = 255;
        }
      }
      ctx.putImageData(imgData, 0, 0);
      // 현재 선택 위치 표시
      const ang = hue * Math.PI / 180;
      const px = cx + Math.cos(ang) * sat * radius, py = cy + Math.sin(ang) * sat * radius;
      ctx.beginPath();
      ctx.arc(px, py, 5, 0, Math.PI * 2);
      ctx.strokeStyle = '#fff'; ctx.lineWidth = 2; ctx.stroke();
      ctx.beginPath();
      ctx.arc(px, py, 5, 0, Math.PI * 2);
      ctx.strokeStyle = '#000'; ctx.lineWidth = 1; ctx.stroke();
    };

    const applyColor = () => {
      const v = valueRange.value / 100;
      const [r, g, b] = hsvToRgb(hue, sat, v);
      const hex = rgbToHex(r, g, b);
      preview.style.background = '#' + hex;
      hexLabel.textContent = '#' + hex;
      // 기존 팔레트 상한 체크/스와치 갱신 로직을 그대로 재사용한다.
      const input = document.getElementById('colorPickerInput');
      input.value = '#' + hex;
      input.dispatchEvent(new Event('input', { bubbles: true }));
    };

    const pickFromEvent = (e) => {
      const rect = canvas.getBoundingClientRect();
      const x = (e.clientX - rect.left) * (size / rect.width);
      const y = (e.clientY - rect.top) * (size / rect.height);
      const dx = x - cx, dy = y - cy;
      const r = Math.min(1, Math.sqrt(dx * dx + dy * dy) / radius);
      hue = (Math.atan2(dy, dx) * 180 / Math.PI + 360) % 360;
      sat = r;
      drawWheel();
      applyColor();
    };

    let dragging = false;
    canvas.addEventListener('mousedown', (e) => { dragging = true; pickFromEvent(e); });
    window.addEventListener('mousemove', (e) => { if (dragging) pickFromEvent(e); });
    window.addEventListener('mouseup', () => { dragging = false; });
    valueRange.addEventListener('input', () => { drawWheel(); applyColor(); });

    document.getElementById('colorSwatch').addEventListener('click', (e) => {
      e.stopPropagation();
      const isHidden = popover.classList.contains('hidden');
      if (isHidden) {
        const swatchRect = document.getElementById('colorSwatch').getBoundingClientRect();
        const toolbarRect = document.getElementById('canvasToolbar').getBoundingClientRect();
        popover.style.left = (swatchRect.left - toolbarRect.left) + 'px';
        popover.style.top = (swatchRect.bottom - toolbarRect.top + 8) + 'px';
        popover.classList.remove('hidden');
        drawWheel();
      } else {
        popover.classList.add('hidden');
      }
    });
    document.addEventListener('click', (e) => {
      if (!popover.classList.contains('hidden') && !popover.contains(e.target) && e.target.id !== 'colorSwatch') {
        popover.classList.add('hidden');
      }
    });

    // 스포이드(picker 툴)로 고른 색도 휠의 위치/명도/미리보기에 그대로 반영한다.
    // (생성자에서 임시로 넣어둔 단순 버전을 여기서 최종 버전으로 덮어쓴다)
    this.editor.onColorPicked = (hex) => {
      if (!hex) return;
      const clean = hex.replace('#', '');
      document.getElementById('colorPickerInput').value = '#' + clean;
      const r = parseInt(clean.slice(0, 2), 16), g = parseInt(clean.slice(2, 4), 16), b = parseInt(clean.slice(4, 6), 16);
      const [h, s, v] = rgbToHsv(r, g, b);
      hue = h; sat = s;
      valueRange.value = Math.round(v * 100);
      preview.style.background = '#' + clean.toUpperCase();
      hexLabel.textContent = '#' + clean.toUpperCase();
      if (!popover.classList.contains('hidden')) drawWheel();
    };
  }

  /* ---------- 좌측: 프레임 목록 / 압축 분석 ---------- */
  _bindLeftPanel() {
    document.getElementById('btnFrameAdd').onclick = () => {
      if (this.project.addFrame()) { this._renderFrameList(); this._onFrameChanged(); this.logger.info('프레임을 추가했습니다.'); }
      else this.logger.warn('최대 프레임 수(' + this.project.maxFrames + ')에 도달했습니다.');
    };
    document.getElementById('btnFrameDup').onclick = () => {
      if (this.project.duplicateFrame()) { this._renderFrameList(); this._onFrameChanged(); this.logger.info('프레임을 복제했습니다.'); }
      else this.logger.warn('최대 프레임 수에 도달했습니다.');
    };
    document.getElementById('btnFrameDel').onclick = () => {
      if (this.project.deleteFrame()) { this._renderFrameList(); this._onFrameChanged(); this.logger.info('프레임을 삭제했습니다.'); }
      else this.logger.warn('마지막 남은 프레임은 삭제할 수 없습니다.');
    };
    document.getElementById('btnFrameUp').onclick = () => {
      if (this.project.moveFrame(this.project.currentFrameIndex, -1)) { this._renderFrameList(); this._onFrameChanged(); }
    };
    document.getElementById('btnFrameDown').onclick = () => {
      if (this.project.moveFrame(this.project.currentFrameIndex, 1)) { this._renderFrameList(); this._onFrameChanged(); }
    };
    document.getElementById('btnFrameHelp').onclick = () => {
      this.logger.info('프레임 목록 항목을 클릭하면 해당 프레임으로 이동합니다. + 추가 / 복사 / 삭제 / ▲▼ 로 순서를 조정하세요.');
    };
    document.getElementById('runAnalysis').onclick = () => this._runCompressionAnalysis();
  }

  _renderFrameList() {
    const ul = document.getElementById('frameList');
    ul.innerHTML = '';
    this.project.frames.forEach((frame, i) => {
      const li = document.createElement('li');
      li.className = 'frame-item' + (i === this.project.currentFrameIndex ? ' active' : '');
      li.innerHTML =
        '<span class="frame-sprocket"><i></i><i></i><i></i></span>' +
        '<canvas class="frame-thumb" width="1" height="1" data-idx="' + i + '"></canvas>' +
        '<span class="frame-meta"><span class="idx">#' + (i + 1) + '</span><span class="tag"></span></span>' +
        '<span class="frame-actions">' +
        '<button class="icon-btn" data-act="dup" title="복사">⧉</button>' +
        '<button class="icon-btn danger" data-act="del" title="삭제">✕</button>' +
        '</span>';
      // 프레임 목록은 항목 수만큼 매번 다시 그려지므로, 여기서는 저비용 판정(SAME/CHANGED)만
      // 표시한다. 정확한 FULL/DIFF 판정(ObjectMove 탐색 등 비용이 큰 계산 포함)은
      // 우측 속성 패널에서 "현재 선택된 프레임 1개"에 대해서만 수행한다.
      li.querySelector('.tag').textContent = i === 0 ? 'FULL' : (arraysEqual(frame.flatten(), this.project.frames[i - 1].flatten()) ? 'SAME' : 'CHANGED');
      li.addEventListener('click', (e) => {
        if (e.target.closest('[data-act]')) return;
        this.project.currentFrameIndex = i;
        this._onFrameChanged();
      });
      li.querySelector('[data-act="dup"]').addEventListener('click', (e) => {
        e.stopPropagation();
        if (this.project.duplicateFrame(i)) { this._renderFrameList(); this._onFrameChanged(); }
      });
      li.querySelector('[data-act="del"]').addEventListener('click', (e) => {
        e.stopPropagation();
        if (this.project.deleteFrame(i)) { this._renderFrameList(); this._onFrameChanged(); }
        else this.logger.warn('마지막 남은 프레임은 삭제할 수 없습니다.');
      });
      ul.appendChild(li);
      this._paintThumb(li.querySelector('.frame-thumb'), frame);
    });
  }

  _updateThumb(idx) {
    const el = document.querySelector('.frame-thumb[data-idx="' + idx + '"]');
    if (el) this._paintThumb(el, this.project.frames[idx]);
  }

  _paintThumb(canvasEl, frame) {
    const w = this.project.width, h = this.project.height;
    canvasEl.width = w; canvasEl.height = h;
    const ctx = canvasEl.getContext('2d');
    const imgData = ctx.createImageData(w, h);
    const flat = frame.flatten();
    for (let i = 0; i < flat.length; i++) {
      const idx = flat[i], o = i * 4;
      if (idx === 0) { imgData.data[o + 3] = 0; continue; }
      const hex = this.project.palette.colors[idx - 1] || '000000';
      imgData.data[o] = parseInt(hex.slice(0, 2), 16);
      imgData.data[o + 1] = parseInt(hex.slice(2, 4), 16);
      imgData.data[o + 2] = parseInt(hex.slice(4, 6), 16);
      imgData.data[o + 3] = 255;
    }
    ctx.putImageData(imgData, 0, 0);
  }

  _highlightActiveFrame() {
    document.querySelectorAll('.frame-item').forEach((el, i) => {
      el.classList.toggle('active', i === this.project.currentFrameIndex);
    });
  }

  /* ---------- 압축 분석 패널 ---------- */
  _buildCompressorList() {
    const list = [
      ['SameFrame', '동일 프레임은 # 로 저장'],
      ['Diff', '이전 프레임과의 변경분만 저장'],
      ['ObjectMove', '사각영역 이동만 벡터로 저장'],
      ['Delta', '변경 인덱스를 증분(+/-)으로 저장'],
      ['RLE', '동일 색 반복 구간 압축'],
      ['TransparentSkip', '투명 영역은 별도 저비용 토큰'],
      ['LineRepeat/BlockMerge', '동일한 행 연속 반복 병합'],
      ['Dictionary', '비연속 반복 행을 사전으로 치환'],
      ['Palette', '색상을 팔레트 인덱스로 치환'],
    ];
    const box = document.getElementById('compressorList');
    box.innerHTML = '';
    list.forEach(([name, desc]) => {
      const row = document.createElement('div');
      row.className = 'comp-row';
      row.innerHTML = '<input type="checkbox" checked disabled><span class="name" title="' + desc + '">' + name + '</span>';
      box.appendChild(row);
    });
    const note = document.createElement('div');
    note.style.cssText = 'font-size:10px;color:var(--muted-2);margin-top:4px;';
    note.textContent = '현재 파이프라인은 항상 적용되며(체이닝 고정), 새 압축기는 코드에 클래스를 추가해 확장 가능합니다.';
    box.appendChild(note);
  }

  _runCompressionAnalysis() {
    try {
      const raw = this.project.frames.length * this.project.width * this.project.height; // 1 byte/pixel 비압축 가정
      const serialized = this.project.toJVFileText();
      const compressed = serialized.length;
      const ratio = raw === 0 ? 0 : (compressed / raw * 100);

      let sameCount = 0, diffCount = 0;
      for (let i = 1; i < this.project.frames.length; i++) {
        if (arraysEqual(this.project.frames[i].flatten(), this.project.frames[i - 1].flatten())) sameCount++;
        else diffCount++;
      }

      const box = document.getElementById('compressionStats');
      box.innerHTML =
        '원본(비압축) 추정: <b>' + raw + ' B</b><br>' +
        'JV 직렬화 크기: <b>' + compressed + ' B</b><br>' +
        '압축률: <b>' + ratio.toFixed(1) + '%</b> (원본 대비)<br>' +
        'SAME 프레임: <b>' + sameCount + '</b> / DIFF 프레임: <b>' + diffCount + '</b><br>' +
        '팔레트 색상 수: <b>' + this.project.palette.colors.length + '</b>';
      this.logger.ok('압축 분석 완료: ' + compressed + ' / ' + raw + ' bytes (' + ratio.toFixed(1) + '%)');
    } catch (err) {
      this.logger.error('압축 분석 실패: ' + err.message);
    }
  }
}

window.addEventListener('DOMContentLoaded', () => {
  window.__jv = new UIController();
});
</script>
</body>
</html>
