# JVG — Jun Vector Graphics

## 언어 가이드 & 스펙 v1.0

> "모든 그림은 선(Line) 하나로 표현한다."

| 항목 | 내용 |
|---|---|
| 버전 | 1.0 |
| 작성일 | 2026-07-28 |
| 상태 | 공식 스펙 |
| 파일 확장자 | `.jvg` |
| 인코딩 | UTF-8 텍스트 |

---

## 목차

1. 소개
2. 빠른 시작
3. 파일 형식
4. 명령 레퍼런스
5. 좌표
6. Color — 색상
7. Weight — 선 굵기
8. Line — 선
9. Fill — 채우기
10. 상태 모델
11. JVG가 제공하지 않는 것
12. 컴파일 모델
13. 구현 규칙 (컴파일러 제작자용)
14. 흔한 실수
15. 완전한 예제
16. 버전 관리와 미래
17. 부록 A — 문법 요약

---

## 1. 소개

JVG (Jun Vector Graphics)는 사람이 읽기 쉽고, 작성하기 쉽고, 처음부터 간결하도록 설계된 벡터 그래픽 언어입니다.

JVG는 XML, JSON 등의 데이터 포맷을 대체하기 위한 형식이 아닙니다. **그림을 표현하기 위한 언어**입니다.

### 설계 원칙

- **사람 중심 문법** — 자연스럽게 읽히는 구문
- **높은 가독성** — 구조가 한눈에 보임
- **처음부터 간결** — 장황한 마크업 없음
- **배우기 쉬움** — 명령은 단 5개
- **구현이 쉬움** — 단순한 파싱과 명확한 시맨틱
- **모든 것이 선** — 기본 도형 없음, 오직 Line
- **AI 친화적** — 생성과 검증이 쉬운 형식

가장 중요한 원칙은 **"모든 것이 선"**입니다. JVG에는 Circle, Rectangle 같은 도형 명령이 존재하지 않습니다. 모든 형태는 선을 이어 그립니다. 이 단순함이 JVG를 배우기도, 구현하기도 쉽게 만듭니다.

---

## 2. 빠른 시작

아래 코드는 파란색 사각형을 그립니다.

    JVG("1.0")

    Color("0066FF")

    Line(100,100,300,100)
    Line(300,100,300,300)
    Line(300,300,100,300)
    Line(100,300,100,100)

    Fill()

- `JVG("1.0")` — 버전 헤더. 첫 줄에 필수입니다.
- `Color("0066FF")` — 색을 파랑으로 설정합니다.
- 4개의 `Line` — 사각형의 네 변을 그립니다.
- `Fill()` — 경로가 닫혀 있으므로 내부를 파랑으로 채웁니다.

---

## 3. 파일 형식

- 확장자는 `.jvg` 입니다.
- UTF-8 기반 텍스트 형식입니다.
- 첫 번째 명령은 반드시 `JVG("1.0")` 헤더여야 하며, 중복될 수 없습니다.
- 한 줄에 한 명령을 쓰는 것을 권장합니다 (필수는 아님).
- 빈 줄과 공백은 자유롭게 사용할 수 있습니다.

---

## 4. 명령 레퍼런스

JVG의 명령은 정확히 5개입니다.

| 명령 | 형태 | 역할 | 상태 |
|---|---|---|---|
| `JVG` | `JVG("1.0")` | 버전 헤더 | — |
| `Color` | `Color("RRGGBB")` | 색상 설정 | 재정의까지 유지 |
| `Weight` | `Weight(n)` | 선 굵기 설정 | 재정의까지 유지 |
| `Line` | `Line(x1,y1,x2,y2)` | 선분 1개 그리기 | — |
| `Fill` | `Fill()` | 닫힌 경로 채우기 | — |

모든 명령은 함수 형태 `Command(...)` 를 사용합니다.

---

## 5. 좌표

모든 좌표는 `(x,y)` 형식으로 씁니다.

    (0,0)
    (120,45)
    (-50,200)
    (10.5,33.2)

규칙:

- X와 Y는 쉼표(`,`)로 구분합니다.
- 좌표는 픽셀이 아닌 **벡터 단위**입니다. 화면 배치는 컴파일러/렌더러가 결정합니다 (뷰포트 피팅).
- 음수와 소수 모두 허용됩니다.
- Y축은 아래 방향이 증가합니다 (화면 좌표계와 동일).

---

## 6. Color — 색상

    Color("FF0000")
    Color("00FF00")
    Color("0000FF")

규칙:

- **6자리 HEX**만 지원합니다.
- `#`는 사용하지 않습니다.
- 대소문자는 구분하지 않으나, 공식 문서에서는 **대문자**를 권장합니다.
- 한 번 설정한 색은 다시 정의할 때까지 이후 모든 명령에 적용됩니다.
- `Line`의 선과 `Fill`의 채우기 모두에 적용됩니다.
- 기본값 (Color 호출 전): `FFFFFF`

상태 유지 예시:

    JVG("1.0")

    Color("FF0000")
    Line(0,0,100,0)

    Line(0,20,100,20)

    Color("0000FF")
    Line(0,40,100,40)

두 번째 선도 빨간색으로 그려집니다. 색은 재정의 전까지 유지되기 때문입니다.

---

## 7. Weight — 선 굵기

    Weight(5)

규칙:

- **0보다 큰 숫자**만 허용합니다 (소수 가능).
- `Color`와 마찬가지로 재정의 전까지 유지됩니다.
- 기본값 (Weight 호출 전): `1`
- 굵기는 벡터 단위입니다. 화면이 확대되면 같이 커집니다.
- `Fill`은 굵기와 무관하게 항상 내부를 채웁니다.

---

## 8. Line — 선

    Line(100,100,300,100)

- `(x1,y1)` 에서 `(x2,y2)` 로 선분 하나를 그립니다.
- 모든 `Line`은 **독립적**입니다. "현재 위치에서 이어 그리기" 명령은 존재하지 않습니다.
- 연결된 형태를 그리려면 끝점을 공유하는 `Line`을 반복합니다.

삼각형 그리기:

    Line(100,100,300,100)
    Line(300,100,200,260)
    Line(200,260,100,100)

왜 이어 그리기가 없나요?

- **파서가 단순**해집니다 — 모든 명령이 자기완결적입니다.
- **순서 자유도**가 높아집니다 — 선의 순서를 바꿔도 의미가 유지됩니다.
- **AI가 생성하기 쉽습니다** — "현재 위치" 상태를 추적할 필요가 없습니다.

---

## 9. Fill — 채우기

    Fill()

`Fill()`은 닫힌 경로의 내부를 채웁니다.

### 9.1 판단 기준

`Fill()`은 **그 시점까지 그려진 선분만** 보고 판단합니다. `Fill()` 이후의 명령은 참조하지 않습니다.

### 9.2 닫힌 루프 감지

- 선분 체인을 따라갔을 때 **시작점으로 돌아오면** 닫힌 것으로 판단합니다.
- 이전 선분의 끝점과 다음 선분의 시작점이 아주 작은 오차(엡실론, 권장 `1e-6`) 이내이면 연결된 것으로 봅니다.
- 닫힌 루프는 다각형 데이터로 변환되어 현재 `Color`로 채워집니다.
- 닫힌 루프가 여러 개라면 각각 감지되어 모두 채워집니다.

### 9.3 열린 경로 — 무시

`Fill()` 시점에 경로가 닫혀 있지 않다면:

- `Fill()`은 **무시**됩니다. 오류가 발생하지 않습니다.
- 그려진 선분은 삭제되지 않고 **유지**됩니다.
- 이후 선을 더 그려서 닫은 뒤 다시 `Fill()`을 호출하면 그때 채워집니다.

이것은 **"무시 = 부작용 없음"** 원칙입니다.

늦게 닫기 예시:

    JVG("1.0")

    Color("5CE0A5")

    Line(80,80,240,80)

    Fill()

    Line(240,80,240,220)
    Line(240,220,80,220)
    Line(80,220,80,80)

    Fill()

- 첫 번째 `Fill()` — 선분 1개, 열려 있음 → 무시 (선분은 유지)
- 세 선분을 더 그린 후 두 번째 `Fill()` — 사각형이 완성되어 채워짐

### 9.4 소비되는 선분

닫힌 루프에 포함된 선분은 해당 `Fill()`에서 **소비**됩니다. 이후 `Fill()` 판단에서 제외됩니다. 루프에 포함되지 않은 열린 선분은 판단 대상에 남습니다.

### 9.5 채워지지 않는 경우

- 고유 꼭짓점이 3개 미만인 루프(점, 또는 왕복 선분)는 면적이 없으므로 채워지지 않습니다.

---

## 10. 상태 모델

JVG는 **상태 기반(state-based)** 언어입니다. `Color`와 `Weight`는 개별 명령이 아니라 문서의 "현재 상태"에 속합니다.

| 상태 | 기본값 | 변경 | 해제 |
|---|---|---|---|
| Color | `FFFFFF` | `Color("RRGGBB")` | 없음 — 재정의까지 유지 |
| Weight | `1` | `Weight(n)` | 없음 — 재정의까지 유지 |

컴파일러는 각 `Line` 명령 시점의 상태 값을 해당 그리기 연산에 **기록(bake)** 합니다. 따라서 JVG의 실행 결과는 실행 환경과 무관하게 결정적입니다.

---

## 11. JVG가 제공하지 않는 것

다음은 v1.0에서 **의도적으로 제공하지 않습니다.**

- 기본 도형: `Circle`, `Rectangle`, `Ellipse`, `Polygon`, `Triangle`, `Star`
- 이어 그리기: `LineTo`, `MoveTo`
- 주석: `//`, `/* */`
- 투명도: 알파 채널 (6자리 HEX만 지원)
- 변환·그룹: `Translate`, `Rotate`, `Scale`, `Group`, `Layer`
- 이미지와 텍스트

모든 것은 선을 이어 표현합니다. 이 목록의 기능은 v1.1 이후 검토 대상입니다.

---

## 12. 컴파일 모델

JVG는 인터프리팅이 아닌 **컴파일**을 전제로 설계되었습니다.

    소스 (.jvg)
        │
        ▼
    [1] LEXER ──────── 토큰
        │
        ▼
    [2] PARSER ─────── AST (명령 목록)
        │
        ▼
    [3] CODEGEN ────── IR (그리기 연산)
        │
        ▼
    [4] RENDER ─────── 화면 (캔버스)

### 12.1 컴파일 타임 Fill

Fill 판단은 렌더 시점이 아니라 **컴파일 시점(CODEGEN)** 에 수행됩니다. 컴파일러는 `Fill()`을 만나는 순간 누적된 선분들에서 닫힌 루프를 미리 계산해 다각형 연산으로 방출합니다.

### 12.2 상태 베이킹

`Color`와 `Weight` 상태는 컴파일 시 각 LINE 연산에 기록됩니다. IR에는 상태 명령이 전혀 남지 않습니다.

IR 예시 (빠른 시작의 사각형을 컴파일한 결과):

    00  LINE  100,100 → 300,100   #0066FF · w1
    01  LINE  300,100 → 300,300   #0066FF · w1
    02  LINE  300,300 → 100,300   #0066FF · w1
    03  LINE  100,300 → 100,100   #0066FF · w1
    04  FILL  polygon(4)          #0066FF

### 12.3 장점

- **빠른 렌더링** — 렌더러는 IR을 순서대로 그리기만 하면 됩니다.
- **조기 오류 감지** — 문법·의미 오류를 컴파일 타임에 line:col과 함께 잡습니다.
- **검증 용이** — 컴파일 결과(IR)를 텍스트로 확인할 수 있습니다.

---

## 13. 구현 규칙 (컴파일러 제작자용)

### 13.1 필수

- `Line` 명령을 해석할 수 있어야 한다.
- `Fill`과 닫힌 루프 자동 감지를 지원해야 한다.
- 6자리 HEX 색상을 지원해야 한다 (대소문자 무관).
- 열린 경로에 대한 `Fill()`은 오류 없이 무시해야 한다.
- `Fill()`은 열린 경로의 선분을 삭제해서는 안 된다.
- `JVG("1.0")` 헤더를 첫 명령으로 검증해야 한다.
- 오류에는 줄(line)과 열(col) 정보를 포함해야 한다.

### 13.2 오류 처리

다음은 반드시 컴파일 오류로 처리합니다.

| 경우 | 예시 |
|---|---|
| 헤더 누락 | `Line(...)`으로 시작하는 파일 |
| 헤더 중복 | `JVG("1.0")`가 두 번 등장 |
| 지원하지 않는 버전 | `JVG("2.0")` |
| 알 수 없는 명령 | `Circle(100,100,50)` |
| 인자 개수 오류 | `Line(0,0,100)` |
| 인자 타입 오류 | `Weight("5")` |
| 잘못된 HEX | `Color("#FF0000")`, `Color("FF00")` |
| 잘못된 Weight | `Weight(0)`, `Weight(-2)` |

### 13.3 구현 재량

다음은 구현체가 자유롭게 결정합니다.

- 닫힘 판단 엡실론 (권장 `1e-6`)
- 여러 루프가 꼭짓점을 공유할 때의 탐색 순서
- 뷰포트 피팅 방식과 최대 확대 배율
- 채우기 규칙 (nonzero winding 권장)
- 애니메이션/정적 렌더링 방식

---

## 14. 흔한 실수

### 14.1 헤더를 잊음

    Line(0,0,100,100)
    Fill()

→ 오류: `JVG("1.0")` 헤더가 첫 줄에 있어야 합니다.

### 14.2 색상에 #를 붙임

    Color("#FF0000")

→ 오류: JVG는 `#`를 사용하지 않습니다. `Color("FF0000")`이 올바릅니다.

### 14.3 Line 인자 개수 오류

    Line(100,100,300)

→ 오류: `Line`은 4개의 인자(x1,y1,x2,y2)가 필요합니다.

### 14.4 이어 그리기를 기대함

    Line(0,0,100,0)
    Line(100,100)

→ 오류: JVG에는 `LineTo`가 없습니다. `Line(100,0,100,100)`으로 양 끝점을 명시하세요.

### 14.5 열린 경로에서 채우기를 기대함

    Line(0,0,100,0)
    Line(100,0,50,80)
    Fill()

→ 무시: 오류는 아니지만 아무것도 채워지지 않습니다. 선분은 유지되므로, 나중에 닫고 다시 `Fill()` 할 수 있습니다.

---

## 15. 완전한 예제

### 15.1 사각형 (스펙 예제)

    JVG("1.0")

    Color("0066FF")

    Line(100,100,300,100)
    Line(300,100,300,300)
    Line(300,300,100,300)
    Line(100,300,100,100)

    Fill()

### 15.2 집 — 여러 채우기와 상태 변경

    JVG("1.0")

    Color("FFB454")
    Line(120,240,120,140)
    Line(120,140,300,140)
    Line(300,140,300,240)
    Line(300,240,120,240)
    Fill()

    Color("4FD8EB")
    Line(105,140,210,55)
    Line(210,55,315,140)
    Line(315,140,105,140)
    Fill()

    Color("5CE0A5")
    Line(190,240,190,185)
    Line(190,185,230,185)
    Line(230,185,230,240)
    Line(230,240,190,240)
    Fill()

각 `Fill()`은 이전 `Fill()` 이후 그려진 선분만 판단합니다. 닫힌 루프는 이미 소비되었기 때문입니다.

### 15.3 별 — 하나의 경로, 열 개의 선분

    JVG("1.0")

    Color("FFD23F")

    Line(160,60,185,126)
    Line(185,126,255,129)
    Line(255,129,200,173)
    Line(200,173,219,241)
    Line(219,241,160,202)
    Line(160,202,101,241)
    Line(101,241,120,173)
    Line(120,173,65,129)
    Line(65,129,135,126)
    Line(135,126,160,60)

    Fill()

### 15.4 JVG 글자 — 채우기 없는 스트로크

    JVG("1.0")

    Weight(7)

    Color("FFB454")
    Line(40,60,105,60)
    Line(88,60,88,170)
    Line(88,170,68,188)
    Line(68,188,44,172)

    Color("4FD8EB")
    Line(135,60,163,188)
    Line(163,188,191,60)

    Color("5CE0A5")
    Line(272,78,252,60)
    Line(252,60,222,60)
    Line(222,60,204,78)
    Line(204,78,204,168)
    Line(204,168,222,186)
    Line(222,186,252,186)
    Line(252,186,272,168)
    Line(272,168,272,128)
    Line(272,128,242,128)

`Fill()`이 없으므로 모든 글자는 선(스트로크)으로만 렌더링됩니다.

---

## 16. 버전 관리와 미래

### 버전 협상

- 컴파일러는 헤더의 버전 문자열로 지원 대상을 확인합니다.
- v1.0 컴파일러는 `1.0`이 아닌 버전을 거부해야 합니다.

### v1.1 검토 후보 (미확정)

- 변환: `Translate`, `Rotate`, `Scale`
- 그룹: `Group(...)`
- 곡선
- 주석
- 투명도

기능이 추가되더라도 **"처음부터 간결"**, **"모든 것은 선"** 원칙을 따라야 합니다.

---

## 부록 A — 문법 요약

    Program  := Header Command*
    Header   := JVG( "1.0" )

    Command  := Color | Weight | Line | Fill

    Color    := Color( HEX )
    Weight   := Weight( Number )
    Line     := Line( Number , Number , Number , Number )
    Fill     := Fill( )

    HEX      := 6자리 16진수 문자열 ("00"~"FF", 대소문자 무관)
    Number   := 정수 또는 소수, 음수 허용

---

**JVG v1.0** — Jun Vector Graphics
모든 그림은 선 하나로 표현한다.
## CODE
```<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>JVG — Jun Vector Graphics Compiler</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Chakra+Petch:wght@500;600;700&family=IBM+Plex+Sans+KR:wght@400;500;700&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
<style>
:root{--bg:#071829;--panel:#0C2440;--panel2:#0A1F36;--line:#1C4368;--line-soft:#143452;--ink:#D9EAF8;--mut:#6E93B8;--dim:#41678C;
--cyan:#4FD8EB;--amber:#FFB454;--green:#5CE0A5;--red:#FF6B7A;--yellow:#FFD23F;
--mono:'JetBrains Mono','IBM Plex Sans KR',monospace;--disp:'Chakra Petch','IBM Plex Sans KR',sans-serif;}
*{box-sizing:border-box}
html,body{height:100%}
body{margin:0;color:var(--ink);font-family:'IBM Plex Sans KR',sans-serif;background:
 radial-gradient(900px 520px at 88% -12%,rgba(79,216,235,.09),transparent 60%),
 radial-gradient(760px 520px at -8% 112%,rgba(255,180,84,.07),transparent 60%),var(--bg);}
body::before{content:'';position:fixed;inset:0;pointer-events:none;z-index:0;background:
 linear-gradient(rgba(79,216,235,.028) 1px,transparent 1px) 0 0/44px 44px,
 linear-gradient(90deg,rgba(79,216,235,.028) 1px,transparent 1px) 0 0/44px 44px;}
::selection{background:rgba(79,216,235,.28)}
::-webkit-scrollbar{width:9px;height:9px}
::-webkit-scrollbar-thumb{background:#1B3E60;border-radius:5px;border:2px solid var(--panel2)}
::-webkit-scrollbar-track{background:transparent}
.app{position:relative;z-index:1;display:flex;flex-direction:column;height:100vh}
.top{display:flex;align-items:center;gap:14px;padding:11px 18px;flex:0 0 auto;border-bottom:1px solid var(--line-soft);
 background:linear-gradient(180deg,rgba(13,38,66,.92),rgba(9,26,46,.92));}
.logo svg{display:block}
.logo .lp{stroke-dasharray:60;stroke-dashoffset:60;animation:dash 3.4s ease-in-out infinite alternate}
.logo .nd{animation:ndpulse 2.2s ease-in-out infinite}.logo .nd2{animation-delay:1.1s}
@keyframes dash{to{stroke-dashoffset:0}}
@keyframes ndpulse{0%,100%{opacity:.45}50%{opacity:1}}
.brand h1{margin:0;font:700 25px/1 var(--disp);letter-spacing:.05em}
.brand h1 em{font-style:normal;color:var(--cyan)}
.brand p{margin:3px 0 0;font:500 9.5px var(--disp);letter-spacing:.22em;color:var(--mut);text-transform:uppercase}
.top-right{margin-left:auto;display:flex;align-items:center;gap:9px;flex-wrap:wrap;justify-content:flex-end}
.badge{font:600 9.5px var(--disp);letter-spacing:.14em;padding:4px 8px;border:1px solid var(--line);border-radius:4px;color:var(--mut)}
.status{display:flex;align-items:center;gap:8px;font:500 11px var(--mono);color:var(--mut);min-width:170px;justify-content:flex-end}
.led{width:9px;height:9px;border-radius:50%;background:#33587C;flex:0 0 auto;transition:.2s}
.led.ok{background:var(--green);box-shadow:0 0 9px rgba(92,224,165,.7)}
.led.busy{background:var(--amber);box-shadow:0 0 9px rgba(255,180,84,.7);animation:ledp .55s infinite alternate}
.led.err{background:var(--red);box-shadow:0 0 9px rgba(255,107,122,.7)}
@keyframes ledp{from{opacity:.35}to{opacity:1}}
.main{flex:1;min-height:0;display:grid;grid-template-columns:minmax(350px,440px) 1fr;gap:14px;padding:14px 18px 0}
.panel{display:flex;flex-direction:column;min-height:0;overflow:hidden;background:linear-gradient(180deg,var(--panel),var(--panel2));
 border:1px solid var(--line);border-radius:10px;box-shadow:0 12px 34px rgba(2,10,20,.5);
 animation:rise .55s cubic-bezier(.2,.7,.3,1) both;transition:box-shadow .4s,border-color .4s}
.panel.glow{border-color:rgba(92,224,165,.7);box-shadow:0 0 0 2px rgba(92,224,165,.55),0 12px 34px rgba(2,10,20,.5)}
.stage-panel{animation-delay:.07s}
.pipe{margin:14px 18px 12px;height:224px;flex:0 0 auto;animation-delay:.14s}
@keyframes rise{from{opacity:0;transform:translateY(14px)}to{opacity:1;transform:none}}
.ph{display:flex;align-items:center;gap:10px;padding:0 12px;height:42px;flex:0 0 auto;border-bottom:1px solid var(--line-soft)}
.ph .dot{width:8px;height:8px;border-radius:2px;background:var(--cyan);box-shadow:0 0 8px rgba(79,216,235,.6)}
.ph h2{margin:0;font:600 11px var(--disp);letter-spacing:.2em;color:var(--ink)}
.ph .sub{font:400 10px var(--mono);color:var(--dim)}
.ph .right{margin-left:auto;display:flex;align-items:center;gap:8px}
.ed{flex:1;display:flex;min-height:0;font:400 13px/22px var(--mono)}
.gutter{width:46px;padding:12px 8px 60px 0;text-align:right;color:var(--dim);overflow:hidden;user-select:none;
 background:rgba(7,24,41,.55);border-right:1px solid var(--line-soft);font-size:11px;line-height:22px}
.gutter span{display:block}
.gutter span.err{color:var(--red);font-weight:700;text-shadow:0 0 7px rgba(255,107,122,.55)}
textarea{flex:1;background:transparent;border:0;outline:0;resize:none;color:var(--ink);font:inherit;
 padding:12px 14px;caret-color:var(--amber);white-space:pre;overflow:auto;tab-size:2}
textarea::selection{background:rgba(79,216,235,.25)}
.ef{display:flex;align-items:center;gap:10px;padding:7px 12px;flex:0 0 auto;flex-wrap:wrap;
 border-top:1px solid var(--line-soft);font:400 10px var(--mono);color:var(--dim)}
.ef .hint{margin-left:auto;letter-spacing:.05em}.ef .hint b{color:var(--amber);font-weight:700}
.chip{font:500 10px var(--mono);color:var(--mut);border:1px solid var(--line);padding:4px 8px;border-radius:4px;
 background:transparent;cursor:pointer;transition:.18s;letter-spacing:.03em}
.chip:hover{color:#062036;background:var(--cyan);border-color:var(--cyan);transform:translateY(-1px);box-shadow:0 4px 12px rgba(79,216,235,.35)}
.chip.on{color:var(--cyan);border-color:rgba(79,216,235,.55);background:rgba(79,216,235,.07)}
.btn{font:700 11px var(--disp);letter-spacing:.14em;padding:8px 14px;border-radius:6px;border:1px solid transparent;
 cursor:pointer;transition:.18s;display:inline-flex;align-items:center;gap:6px}
.btn-p{background:var(--amber);color:#241305;box-shadow:0 2px 12px rgba(255,180,84,.28)}
.btn-p:hover{transform:translateY(-1px);background:#FFC67E;box-shadow:0 7px 20px rgba(255,180,84,.42)}
.btn-p:active{transform:translateY(0)}
.btn-p.dis{background:#33547A;color:#7E9FBE;box-shadow:none}
.btn-p.dis:hover{transform:none;background:#33547A}
.btn-g{background:transparent;color:var(--mut);border-color:var(--line)}
.btn-g:hover{color:var(--cyan);border-color:rgba(79,216,235,.5);transform:translateY(-1px)}
.btn-s{padding:5px 9px;font-size:10px;letter-spacing:.1em}
@keyframes shake{0%,100%{transform:translateX(0)}25%{transform:translateX(-4px)}75%{transform:translateX(4px)}}
.shake{animation:shake .22s}
.tgl{display:flex;align-items:center;gap:6px;font:500 10px var(--mono);color:var(--mut);cursor:pointer}
.tgl input{display:none}
.tgl i{width:26px;height:14px;border-radius:8px;background:var(--line);position:relative;transition:.2s;flex:0 0 auto}
.tgl i::after{content:'';position:absolute;width:10px;height:10px;border-radius:50%;background:var(--mut);top:2px;left:2px;transition:.2s}
.tgl input:checked+i{background:rgba(92,224,165,.35)}
.tgl input:checked+i::after{left:14px;background:var(--green)}
.stage{flex:1;position:relative;min-height:0;background:
 linear-gradient(rgba(79,216,235,.05) 1px,transparent 1px) 0 0/16px 16px,
 linear-gradient(90deg,rgba(79,216,235,.05) 1px,transparent 1px) 0 0/16px 16px,
 linear-gradient(rgba(79,216,235,.10) 1px,transparent 1px) 0 0/80px 80px,
 linear-gradient(90deg,rgba(79,216,235,.10) 1px,transparent 1px) 0 0/80px 80px,#08203A}
.stage canvas{position:absolute;inset:0;display:block}
.corner{position:absolute;width:14px;height:14px;border:1.5px solid rgba(79,216,235,.4);pointer-events:none}
.c1{top:8px;left:8px;border-right:0;border-bottom:0}.c2{top:8px;right:8px;border-left:0;border-bottom:0}
.c3{bottom:8px;left:8px;border-right:0;border-top:0}.c4{bottom:8px;right:8px;border-left:0;border-top:0}
.progress{position:absolute;left:0;top:0;height:2px;width:100%;background:rgba(79,216,235,.12)}
.progress i{display:block;height:100%;width:0;background:linear-gradient(90deg,var(--cyan),var(--green));
 box-shadow:0 0 9px rgba(79,216,235,.75);transition:width .09s linear}
.stage-note{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);font:500 12px var(--mono);
 color:var(--dim);letter-spacing:.1em;text-align:center;pointer-events:none}
.stage-note.err{color:var(--red)}
.stats{display:grid;grid-template-columns:repeat(6,1fr);flex:0 0 auto;border-top:1px solid var(--line-soft)}
.stat{padding:8px 12px;border-right:1px solid var(--line-soft)}
.stat:last-child{border-right:0}
.stat b{display:block;font:700 17px/1.15 var(--disp);color:var(--cyan)}
.stat:nth-child(even) b{color:var(--amber)}
.stat span{font:600 8.5px var(--disp);letter-spacing:.18em;color:var(--dim)}
.stat.flash{animation:flash .55s}
@keyframes flash{0%{background:rgba(79,216,235,.16)}100%{background:transparent}}
.stages{display:flex;align-items:center;gap:7px;margin-left:18px}
.stg{display:flex;align-items:center;gap:6px;font:600 9.5px var(--disp);letter-spacing:.12em;color:var(--dim);
 padding:4px 9px;border:1px solid var(--line-soft);border-radius:5px;transition:.25s}
.stg i{font-style:normal;width:13px;height:13px;border:1px solid currentColor;border-radius:3px;display:grid;place-items:center;font-size:8px}
.stg.active{color:var(--amber);border-color:rgba(255,180,84,.6);box-shadow:0 0 13px rgba(255,180,84,.28);animation:ledp .6s infinite alternate}
.stg.done{color:var(--green);border-color:rgba(92,224,165,.45)}
.stg.fail{color:var(--red);border-color:rgba(255,107,122,.55)}
.arr{color:var(--dim);font-size:10px}
.tabs{display:flex;gap:2px}
.tab{font:600 10px var(--disp);letter-spacing:.14em;color:var(--mut);background:none;border:none;
 border-bottom:2px solid transparent;padding:6px 9px;cursor:pointer;transition:.18s}
.tab:hover{color:var(--ink)}
.tab.on{color:var(--cyan);border-bottom-color:var(--cyan)}
.views{flex:1;min-height:0;position:relative}
.view{position:absolute;inset:0;overflow:auto;padding:10px 14px;font:400 11.5px/1.75 var(--mono);display:none}
.view.on{display:block}
.tk{display:inline-block;margin:2px 3px 2px 0;padding:1px 6px;border-radius:3px;border:1px solid var(--line-soft);font-size:11px}
.tk-id{color:var(--cyan);border-color:rgba(79,216,235,.3);background:rgba(79,216,235,.06)}
.tk-str{color:var(--amber);border-color:rgba(255,180,84,.3);background:rgba(255,180,84,.06)}
.tk-num{color:var(--green);border-color:rgba(92,224,165,.3);background:rgba(92,224,165,.06)}
.tk-pun{color:var(--dim)}
.view pre{margin:0;font:inherit}
.j-key{color:var(--cyan)}.j-str{color:var(--amber)}.j-num{color:var(--green)}
.ir-line{display:flex;gap:12px;padding:1.5px 4px;align-items:baseline;white-space:nowrap;border-radius:3px;transition:background .12s}
.ir-line:hover{background:rgba(79,216,235,.10)}
.ir-i{color:var(--dim);width:20px;flex:0 0 auto}
.ir-op{font-weight:700;width:42px;flex:0 0 auto}
.op-line{color:var(--cyan)}.op-fill{color:var(--amber)}
.ir-args{color:var(--ink)}
.ir-meta{color:var(--mut);display:flex;align-items:center;gap:5px}
.ir-meta i{width:9px;height:9px;border-radius:2px;display:inline-block;border:1px solid rgba(255,255,255,.28)}
.log-e{display:flex;gap:10px;padding:1.5px 0;align-items:baseline;animation:login .25s ease both}
@keyframes login{from{opacity:0;transform:translateX(-7px)}to{opacity:1;transform:none}}
.log-t{color:var(--dim);font-size:10px;flex:0 0 auto}
.log-tag{font-weight:700;font-size:9.5px;letter-spacing:.12em;width:46px;flex:0 0 auto}
.tag-ok{color:var(--green)}.tag-warn{color:var(--amber)}.tag-err{color:var(--red)}
.tag-info{color:var(--cyan)}.tag-state{color:var(--yellow)}
.log-m{color:var(--ink)}
.log-e.warn .log-m{color:#FFD9A8}.log-e.err .log-m{color:#FFC2CA}
.foot{display:flex;justify-content:space-between;gap:12px;padding:8px 18px;flex:0 0 auto;
 border-top:1px solid var(--line-soft);font:400 10px var(--mono);color:var(--dim);letter-spacing:.05em;flex-wrap:wrap}
.foot b{color:var(--mut);font-weight:500}
.trc-back{position:fixed;inset:0;z-index:50;display:none;align-items:center;justify-content:center;
 background:rgba(4,14,26,.74);-webkit-backdrop-filter:blur(3px);backdrop-filter:blur(3px)}
.trc-back.open{display:flex;animation:fadeIn .2s ease}
@keyframes fadeIn{from{opacity:0}}
.trc-panel{width:min(880px,94vw);height:min(600px,92vh);display:flex;flex-direction:column;
 background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:12px;
 box-shadow:0 30px 80px rgba(0,0,0,.6);animation:popIn .3s cubic-bezier(.2,.9,.3,1.15)}
@keyframes popIn{from{opacity:0;transform:translateY(18px) scale(.96)}}
.trc-body{flex:1;min-height:0;display:grid;grid-template-columns:1.25fr .9fr;gap:14px;padding:14px}
.trc-left{display:flex;flex-direction:column;min-height:0;gap:10px}
.trc-right{display:flex;flex-direction:column;min-height:0;overflow:auto;padding-right:2px}
.dropzone{position:relative;flex:1;min-height:190px;border:1.5px dashed var(--line);border-radius:8px;overflow:hidden;
 cursor:pointer;transition:.2s;background:
 linear-gradient(rgba(79,216,235,.04) 1px,transparent 1px) 0 0/14px 14px,
 linear-gradient(90deg,rgba(79,216,235,.04) 1px,transparent 1px) 0 0/14px 14px,#08203A}
.dropzone:hover{border-color:rgba(79,216,235,.5)}
.dropzone.over{border-color:var(--amber);box-shadow:0 0 0 3px rgba(255,180,84,.16),inset 0 0 40px rgba(255,180,84,.07)}
.dropzone.has-img{border-style:solid;border-color:var(--line)}
.dz-inner{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:5px;
 color:var(--dim);transition:.25s;text-align:center;padding:10px}
.dz-inner p{margin:0;font:500 12px var(--mono);color:var(--mut)}
.dz-ico{font-size:36px;color:var(--cyan);animation:dzfloat 2.6s ease-in-out infinite;line-height:1}
@keyframes dzfloat{0%,100%{transform:translateY(-4px)}50%{transform:translateY(4px)}}
.dz-sub{font:400 9.5px var(--mono)}
.dropzone.has-img .dz-inner{opacity:0;pointer-events:none}
#trcCv{position:absolute;inset:0;width:100%;height:100%;object-fit:contain;display:none}
.dropzone.has-img #trcCv{display:block}
.trc-stats{display:flex;flex:0 0 auto;border:1px solid var(--line-soft);border-radius:6px;overflow:hidden}
.trc-stats span{flex:1;padding:6px 4px;text-align:center;font:600 7.5px var(--disp);letter-spacing:.1em;color:var(--dim);
 border-right:1px solid var(--line-soft);background:rgba(7,24,41,.4)}
.trc-stats span:last-child{border-right:0}
.trc-stats b{display:block;font:700 12.5px/1.2 var(--disp);color:var(--cyan)}
.trc-stats span:nth-child(even) b{color:var(--amber)}
.ctl{display:block;font:500 10.5px var(--mono);color:var(--mut);margin-bottom:11px;transition:opacity .25s}
.ctl.off{opacity:.28;pointer-events:none}
.ctl b{color:var(--cyan);float:right;font-weight:700}
.ctl input[type=range]{width:100%;margin-top:7px;-webkit-appearance:none;appearance:none;height:4px;border-radius:2px;
 background:var(--line);outline:none;cursor:pointer}
.ctl input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:14px;height:14px;border-radius:50%;
 background:var(--amber);border:2px solid #241305;box-shadow:0 0 8px rgba(255,180,84,.5);transition:transform .15s}
.ctl input[type=range]::-webkit-slider-thumb:hover{transform:scale(1.25)}
.ctl input[type=range]::-moz-range-thumb{width:12px;height:12px;border-radius:50%;background:var(--amber);
 border:2px solid #241305;box-shadow:0 0 8px rgba(255,180,84,.5)}
.trc-hint{font:400 9.5px/1.7 var(--mono);color:var(--dim);margin:2px 0 10px;padding:8px 10px;
 border-left:2px solid var(--line);background:rgba(79,216,235,.04);border-radius:0 4px 4px 0}
.ctl-row{display:flex;align-items:center;justify-content:space-between;gap:10px;margin:2px 0 12px}
.seg{display:flex;border:1px solid var(--line);border-radius:6px;overflow:hidden}
.seg button{font:600 9.5px var(--disp);letter-spacing:.08em;padding:6px 10px;background:transparent;color:var(--mut);
 border:0;cursor:pointer;transition:.18s}
.seg button.on{background:var(--cyan);color:#062036}
.seg button:not(.on):hover{color:var(--ink)}
.trc-foot{margin-top:auto;display:flex;justify-content:flex-end;gap:8px;padding-top:12px}
@media(max-width:1020px){
 .app{height:auto;min-height:100vh}
 .main{grid-template-columns:1fr}
 .editor-panel{height:430px}.stage-panel{height:520px}.pipe{height:300px}
 .stats{grid-template-columns:repeat(3,1fr)}.stat:nth-child(3){border-right:0}
 .stages{margin-left:0}
 .ph{flex-wrap:wrap;height:auto;padding:8px 12px;row-gap:6px}
 .trc-body{grid-template-columns:1fr;overflow:auto}
 .trc-panel{height:auto;max-height:94vh}
 .dropzone{min-height:240px}
}
</style>
</head>
<body>
<div class="app">
  <header class="top">
    <div class="logo" aria-hidden="true">
      <svg width="36" height="36" viewBox="0 0 36 36" fill="none">
        <rect x="1.5" y="1.5" width="33" height="33" rx="8" stroke="#1C4368" stroke-width="1.5"/>
        <path class="lp" d="M8 26 L14 9 L21 20 L28 8" stroke="#4FD8EB" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"/>
        <circle class="nd" cx="8" cy="26" r="2.2" fill="#FFB454"/>
        <circle class="nd nd2" cx="28" cy="8" r="2.2" fill="#FFB454"/>
      </svg>
    </div>
    <div class="brand">
      <h1>JV<em>G</em></h1>
      <p>Jun Vector Graphics · Compiler</p>
    </div>
    <div class="top-right">
      <span class="badge">LINE-ONLY</span>
      <span class="badge">COMPILE-TIME FILL</span>
      <span class="badge">IMG TRACE</span>
      <div class="status"><span class="led" id="led"></span><span id="statusTxt">READY</span></div>
    </div>
  </header>

  <main class="main">
    <section class="panel editor-panel">
      <div class="ph">
        <span class="dot" style="background:var(--amber);box-shadow:0 0 8px rgba(255,180,84,.6)"></span>
        <h2>SOURCE</h2><span class="sub">drawing.jvg</span>
        <div class="right">
          <button class="chip" data-s="square">사각형</button>
          <button class="chip" data-s="house">집</button>
          <button class="chip" data-s="star">별</button>
          <button class="chip" data-s="letters">JVG</button>
          <button class="chip" data-s="open">열린경로</button>
          <button class="chip" data-s="late">늦은닫기</button>
        </div>
      </div>
      <div class="ed">
        <div class="gutter" id="gutter"></div>
        <textarea id="src" spellcheck="false" autocomplete="off" autocorrect="off" autocapitalize="off"></textarea>
      </div>
      <div class="ef">
        <span id="metaPos">Ln 1, Col 1</span>
        <span id="metaSize">0 B</span>
        <label class="tgl" style="margin-left:4px"><input type="checkbox" id="auto" checked><i></i>AUTO</label>
        <button class="btn btn-g btn-s" id="traceBtn">🖼 TRACE</button>
        <button class="btn btn-g btn-s" id="dljvg">⬇ .jvg</button>
        <span class="hint"><b>⌃⏎</b> 컴파일</span>
      </div>
    </section>

    <section class="panel stage-panel">
      <div class="ph">
        <span class="dot"></span>
        <h2>OUTPUT</h2><span class="sub">vector canvas · fit-to-bounds</span>
        <div class="right">
          <button class="btn btn-g btn-s" id="dlir">⬇ IR</button>
          <button class="btn btn-g" id="replay">↻ REPLAY</button>
          <button class="btn btn-p" id="compile">▶ COMPILE</button>
        </div>
      </div>
      <div class="stage" id="stageWrap">
        <div class="progress"><i id="prog"></i></div>
        <canvas id="cv"></canvas>
        <span class="corner c1"></span><span class="corner c2"></span>
        <span class="corner c3"></span><span class="corner c4"></span>
        <div class="stage-note" id="note">컴파일 대기 중…</div>
      </div>
      <div class="stats">
        <div class="stat"><b id="stTok">–</b><span>TOKENS</span></div>
        <div class="stat"><b id="stCmd">–</b><span>COMMANDS</span></div>
        <div class="stat"><b id="stOps">–</b><span>IR OPS</span></div>
        <div class="stat"><b id="stLoop">–</b><span>LOOPS</span></div>
        <div class="stat"><b id="stMs">–</b><span>COMPILE MS</span></div>
        <div class="stat"><b id="stBytes">–</b><span>IR SIZE</span></div>
      </div>
    </section>
  </main>

  <section class="panel pipe">
    <div class="ph">
      <span class="dot" style="background:var(--green);box-shadow:0 0 8px rgba(92,224,165,.6)"></span>
      <h2>PIPELINE</h2><span class="sub">state baked · fill precomputed</span>
      <div class="stages">
        <span class="stg" id="sg0"><i>1</i>LEXER</span><span class="arr">→</span>
        <span class="stg" id="sg1"><i>2</i>PARSER</span><span class="arr">→</span>
        <span class="stg" id="sg2"><i>3</i>CODEGEN</span><span class="arr">→</span>
        <span class="stg" id="sg3"><i>4</i>RENDER</span>
      </div>
      <div class="right tabs">
        <button class="tab" data-v="tokens">TOKENS</button>
        <button class="tab" data-v="ast">AST</button>
        <button class="tab on" data-v="ir">IR</button>
        <button class="tab" data-v="log">LOG</button>
      </div>
    </div>
    <div class="views">
      <div class="view" id="v-tokens"></div>
      <div class="view" id="v-ast"></div>
      <div class="view on" id="v-ir"></div>
      <div class="view" id="v-log"></div>
    </div>
  </section>

  <footer class="foot">
    <span><b>JVG</b> — Jun Vector Graphics · 모든 그림은 선(Line)으로 표현한다</span>
    <span>기본값 <b>Color FFFFFF · Weight 1</b> · 열린 Fill()은 무시(선분 유지)</span>
  </footer>
</div>

<div class="trc-back" id="trcBack">
  <div class="trc-panel">
    <div class="ph">
      <span class="dot" style="background:var(--yellow);box-shadow:0 0 8px rgba(255,210,63,.6)"></span>
      <h2>TRACE</h2><span class="sub">PNG · JPEG → JVG · 스캔라인(무손실) / 엣지(Canny)</span>
      <div class="right"><button class="btn btn-g btn-s" id="trcClose">✕ 닫기</button></div>
    </div>
    <div class="trc-body">
      <div class="trc-left">
        <div class="dropzone" id="dropzone">
          <div class="dz-inner">
            <span class="dz-ico">▦</span>
            <p>이미지를 여기에 놓거나 클릭</p>
            <span class="dz-sub">PNG · JPEG · 붙여넣기(Ctrl+V) 지원</span>
          </div>
          <canvas id="trcCv"></canvas>
          <input type="file" id="trcFile" accept="image/*" hidden>
        </div>
        <div class="trc-stats">
          <span><b id="tsPx">–</b>PIXELS</span>
          <span><b id="tsStroke">–</b>STROKES</span>
          <span><b id="tsCmd">–</b>LINE CMD</span>
          <span><b id="tsByte">–</b>BYTES</span>
          <span><b id="tsMs">–</b>TRACE MS</span>
        </div>
      </div>
      <div class="trc-right">
        <div class="ctl-row">
          <div class="seg" id="modeSeg">
            <button data-m="scan" class="on">스캔라인</button>
            <button data-m="mono">엣지 모노</button>
            <button data-m="color">엣지 컬러</button>
          </div>
          <label class="tgl" id="skipWhiteWrap"><input type="checkbox" id="cSkipWhite" checked><i></i>흰 배경 제외</label>
        </div>
        <label class="ctl">해상도 (긴 변) <b id="vDetail">480</b>px
          <input type="range" id="cDetail" min="160" max="800" step="20" value="480">
        </label>
        <label class="ctl">색상 병합 허용오차 <b id="vTol">16</b>
          <input type="range" id="cTol" min="0" max="60" step="2" value="16">
        </label>
        <label class="ctl">엣지 임계값 <b id="vThresh">50</b>
          <input type="range" id="cThresh" min="8" max="200" step="2" value="50">
        </label>
        <label class="ctl">노이즈 제거 <b id="vDenoise">1</b>
          <input type="range" id="cDenoise" min="0" max="3" step="1" value="1">
        </label>
        <label class="ctl">최소 선 길이 <b id="vMin">5</b>px
          <input type="range" id="cMin" min="2" max="24" step="1" value="5">
        </label>
        <label class="ctl">곡선 스무딩 <b id="vSmooth">1</b>
          <input type="range" id="cSmooth" min="0" max="2" step="1" value="1">
        </label>
        <label class="ctl">단순화 (RDP) <b id="vSimp">0.8</b>
          <input type="range" id="cSimp" min="0" max="3" step="0.1" value="0.8">
        </label>
        <p class="trc-hint">스캔라인: 허용오차 <b style="color:var(--cyan)">0</b> = 원본 무손실 · 값↑ = 코드 크기↓ &nbsp;|&nbsp; 엣지: 임계값↓ 로고 · 노이즈 제거↑ 사진</p>
        <div class="trc-foot">
          <button class="btn btn-g" id="trcCancel">취소</button>
          <button class="btn btn-p" id="trcInsert">에디터에 삽입 ▶</button>
        </div>
      </div>
    </div>
  </div>
</div>

<script>
/* JVG Compiler — LEXER → PARSER → CODEGEN → RENDER + IMG TRACER (스캔라인/엣지) */
'use strict';
const $ = id => document.getElementById(id);
const sleep = ms => new Promise(r => setTimeout(r, ms));
const esc = s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
class JvgError extends Error { constructor(msg,line,col){ super(msg); this.line=line; this.col=col; } }

/* ---------- LEXER ---------- */
function lex(src){
  const tokens = []; let i = 0, line = 1, col = 1;
  const push = (type,value,l,c) => tokens.push({type,value,line:l,col:c});
  while (i < src.length){
    const ch = src[i];
    if (ch === '\n'){ line++; col=1; i++; continue; }
    if (/\s/.test(ch)){ i++; col++; continue; }
    if (ch === '('){ push('LPAREN','(',line,col); i++; col++; continue; }
    if (ch === ')'){ push('RPAREN',')',line,col); i++; col++; continue; }
    if (ch === ','){ push('COMMA',',',line,col); i++; col++; continue; }
    if (ch === '"'){
      const l=line,c=col; i++; col++; let s='';
      while (i < src.length && src[i] !== '"'){
        if (src[i] === '\n') throw new JvgError('닫히지 않은 문자열입니다', l, c);
        s += src[i]; i++; col++;
      }
      if (i >= src.length) throw new JvgError('닫히지 않은 문자열입니다', l, c);
      i++; col++; push('STRING', s, l, c); continue;
    }
    if (/[A-Za-z_]/.test(ch)){
      const l=line,c=col; let s='';
      while (i < src.length && /[A-Za-z_0-9]/.test(src[i])){ s += src[i]; i++; col++; }
      push('IDENT', s, l, c); continue;
    }
    if (ch === '-' || /[0-9]/.test(ch)){
      const l=line,c=col; let s='';
      if (src[i] === '-'){ s += '-'; i++; col++; }
      let digits=false, dot=false;
      while (i < src.length && (/[0-9]/.test(src[i]) || (src[i] === '.' && !dot))){
        if (src[i] === '.') dot=true; else digits=true;
        s += src[i]; i++; col++;
      }
      if (!digits) throw new JvgError('잘못된 숫자 "' + s + '"', l, c);
      push('NUMBER', parseFloat(s), l, c); continue;
    }
    throw new JvgError('인식할 수 없는 문자 "' + ch + '"', line, col);
  }
  push('EOF', null, line, col);
  return tokens;
}

/* ---------- PARSER ---------- */
const SIG = { JVG:['string'], Color:['string'], Weight:['number'], Line:['number','number','number','number'], Fill:[] };
function parse(tokens){
  const ast = []; let p = 0;
  const peek = () => tokens[p], next = () => tokens[p++];
  const expect = type => {
    const t = next();
    if (t.type !== type) throw new JvgError(type + '가 필요한 위치에 ' + t.type + '("' + (t.value ?? '') + '")가 있습니다', t.line, t.col);
    return t;
  };
  while (peek().type !== 'EOF'){
    const id = expect('IDENT');
    const sig = SIG[id.value];
    if (!sig) throw new JvgError('알 수 없는 명령 "' + id.value + '"', id.line, id.col);
    expect('LPAREN');
    const args = [];
    if (peek().type !== 'RPAREN'){
      for(;;){
        const t = next();
        const want = sig[args.length];
        if (want === undefined) throw new JvgError(id.value + '() 인자가 너무 많습니다', t.line, t.col);
        if (want === 'string' && t.type !== 'STRING') throw new JvgError(id.value + '()의 ' + (args.length+1) + '번째 인자는 문자열이어야 합니다', t.line, t.col);
        if (want === 'number' && t.type !== 'NUMBER') throw new JvgError(id.value + '()의 ' + (args.length+1) + '번째 인자는 숫자여야 합니다', t.line, t.col);
        args.push(t.value);
        if (peek().type === 'COMMA'){ next(); continue; }
        break;
      }
    }
    expect('RPAREN');
    if (args.length !== sig.length) throw new JvgError(id.value + '()는 인자 ' + sig.length + '개가 필요합니다 (현재 ' + args.length + '개)', id.line, id.col);
    ast.push({ cmd: id.value, args, line: id.line });
  }
  return ast;
}

/* ---------- CODEGEN ---------- */
function codegen(ast, log){
  if (!ast.length || ast[0].cmd !== 'JVG') throw new JvgError('JVG("1.0") 헤더가 첫 번째 명령이어야 합니다', 1, 1);
  if (ast[0].args[0] !== '1.0') throw new JvgError('지원하지 않는 버전 "' + ast[0].args[0] + '"', ast[0].line, 1);
  const close = (a,b) => Math.abs(a[0]-b[0]) < 1e-6 && Math.abs(a[1]-b[1]) < 1e-6;
  const ir = []; let color = 'FFFFFF', weight = 1, pending = [], loops = 0;
  for (const n of ast.slice(1)){
    if (n.cmd === 'JVG') throw new JvgError('JVG() 헤더는 중복될 수 없습니다 (첫 줄에만 허용)', n.line, 1);
    else if (n.cmd === 'Color'){
      const hex = n.args[0];
      if (!/^[0-9a-fA-F]{6}$/.test(hex)) throw new JvgError('잘못된 HEX 색상 "' + hex + '" — 6자리 HEX (예: "FF0000")', n.line, 1);
      color = hex.toUpperCase();
      log('state','STATE','Color → #' + color + '  (line ' + n.line + ')');
    }
    else if (n.cmd === 'Weight'){
      if (!(n.args[0] > 0)) throw new JvgError('Weight는 0보다 커야 합니다', n.line, 1);
      weight = n.args[0];
      log('state','STATE','Weight → ' + weight + '  (line ' + n.line + ')');
    }
    else if (n.cmd === 'Line'){
      const [x1,y1,x2,y2] = n.args;
      ir.push({ op:'LINE', x1, y1, x2, y2, color, weight });
      pending.push({ a:[x1,y1], b:[x2,y2] });
    }
    else if (n.cmd === 'Fill'){
      const res = extractLoops(pending, close);
      for (const lp of res.loops){
        ir.push({ op:'FILL', pts: lp.pts, color }); loops++;
        log('ok','FILL','닫힌 루프 감지 (line ' + n.line + ') : ' + lp.segs + '개 선분 → polygon(' + lp.pts.length + ') #' + color);
      }
      const remain = pending.filter((_, i) => !res.usedMask[i]);
      if (res.loops.length === 0 && pending.length === 0) log('warn','WARN','Fill() (line ' + n.line + ') 무시 — 그려진 선분이 없습니다');
      else if (res.loops.length === 0) log('warn','WARN','Fill() (line ' + n.line + ') 무시 — 열린 경로 (선분 ' + pending.length + '개 미닫힘 · 유지됨)');
      else if (remain.length > 0) log('info','FILL','열린 선분 ' + remain.length + '개 유지 — 다음 Fill()에서 재판정');
      pending = remain;
    }
  }
  log('info','GEN','IR ' + ir.length + ' ops 방출 (색상·굵기 bake 완료)');
  return { ir, loops };
}
function extractLoops(segs, close){
  const n = segs.length, used = new Array(n).fill(false), loops = [];
  let progress = true, guard = 0, budget = 120000;
  while (progress && guard++ <= n){
    progress = false;
    for (let s = 0; s < n; s++){
      if (used[s]) continue;
      const path = [s]; used[s] = true;
      const target = segs[s].a;
      let closed = false;
      const dfs = cur => {
        if (closed || --budget < 0) return;
        if (close(cur, target)){ closed = true; return; }
        for (let j = 0; j < n; j++){
          if (closed || budget < 0) return;
          if (used[j] || !close(segs[j].a, cur)) continue;
          used[j] = true; path.push(j);
          dfs(segs[j].b);
          if (!closed){ used[j] = false; path.pop(); }
        }
      };
      dfs(segs[s].b);
      if (closed){
        const pts = [segs[s].a.slice()];
        for (const idx of path) pts.push(segs[idx].b.slice());
        const clean = [pts[0]];
        for (let k = 1; k < pts.length; k++) if (!close(pts[k], clean[clean.length-1])) clean.push(pts[k]);
        if (clean.length > 1 && close(clean[clean.length-1], clean[0])) clean.pop();
        if (clean.length >= 3){ loops.push({ pts: clean, segs: path.length }); progress = true; break; }
        for (const idx of path) used[idx] = false;
      } else { for (const idx of path) used[idx] = false; }
    }
  }
  return { loops, usedMask: used };
}

/* ---------- RENDER (레이어 캐시) ---------- */
const cv = $('cv'), ctx = cv.getContext('2d'), wrap = $('stageWrap');
const layer = document.createElement('canvas'), lctx = layer.getContext('2d');
let currentIR = null, animToken = 0, isPlaying = false;
const R = { ir: null, bb: null, tf: null, done: -1 };
function sizeCanvas(){
  const r = wrap.getBoundingClientRect(), dpr = window.devicePixelRatio || 1;
  cv.width = Math.max(1, Math.round(r.width * dpr));
  cv.height = Math.max(1, Math.round(r.height * dpr));
  cv.style.width = r.width + 'px'; cv.style.height = r.height + 'px';
}
function bbox(ir){
  let x0=Infinity,y0=Infinity,x1=-Infinity,y1=-Infinity;
  for (const o of ir){
    if (o.op === 'LINE'){
      if (o.x1<x0)x0=o.x1; if (o.x2<x0)x0=o.x2; if (o.y1<y0)y0=o.y1; if (o.y2<y0)y0=o.y2;
      if (o.x1>x1)x1=o.x1; if (o.x2>x1)x1=o.x2; if (o.y1>y1)y1=o.y1; if (o.y2>y1)y1=o.y2;
    } else for (const p of o.pts){
      if (p[0]<x0)x0=p[0]; if (p[0]>x1)x1=p[0]; if (p[1]<y0)y0=p[1]; if (p[1]>y1)y1=p[1];
    }
  }
  return x0===Infinity ? null : {x0,y0,x1,y1};
}
function setIR(ir){
  R.ir = ir; R.bb = bbox(ir); R.tf = null; R.done = -1;
  lctx.setTransform(1,0,0,1,0,0);
  lctx.clearRect(0,0,layer.width,layer.height);
}
function fit(bb, w, h, dpr){
  const pad = Math.min(42, w*0.12, h*0.12);
  const bw = Math.max(bb.x1-bb.x0, 1), bh = Math.max(bb.y1-bb.y0, 1);
  let s = Math.min((w-pad*2)/bw, (h-pad*2)/bh);
  s = Math.max(0.01, Math.min(s, 3.4));
  return { s, ox:(w-bw*s)/2 - bb.x0*s, oy:(h-bh*s)/2 - bb.y0*s, dpr };
}
function getTF(){
  const dpr = window.devicePixelRatio || 1;
  if (R.tf && R.tf.dpr === dpr && layer.width === cv.width && layer.height === cv.height) return R.tf;
  layer.width = cv.width; layer.height = cv.height;
  R.done = -1;
  R.tf = R.bb ? fit(R.bb, cv.width/dpr, cv.height/dpr, dpr) : null;
  return R.tf;
}
function drawOp(c, o, p, tf){
  const tx = x => x*tf.s + tf.ox, ty = y => y*tf.s + tf.oy;
  if (o.op === 'LINE'){
    if (p <= 0) return;
    const ex = o.x1 + (o.x2-o.x1)*p, ey = o.y1 + (o.y2-o.y1)*p;
    c.strokeStyle = '#'+o.color;
    c.lineWidth = Math.max(0.6, o.weight*tf.s);
    c.beginPath(); c.moveTo(tx(o.x1),ty(o.y1)); c.lineTo(tx(ex),ty(ey)); c.stroke();
  } else {
    c.globalAlpha = p; c.fillStyle = '#'+o.color;
    c.beginPath();
    o.pts.forEach((pt,k)=> k ? c.lineTo(tx(pt[0]),ty(pt[1])) : c.moveTo(tx(pt[0]),ty(pt[1])));
    c.closePath(); c.fill(); c.globalAlpha = 1;
  }
}
function bakeUpTo(idx){
  const tf = getTF();
  if (!tf || idx < 0) return;
  lctx.setTransform(tf.dpr,0,0,tf.dpr,0,0);
  lctx.lineCap = lctx.lineJoin = 'round';
  for (let i = R.done+1; i <= idx && i < R.ir.length; i++) drawOp(lctx, R.ir[i], 1, tf);
  if (idx > R.done) R.done = Math.min(idx, R.ir.length-1);
}
function clearMain(){
  const dpr = window.devicePixelRatio || 1;
  ctx.setTransform(dpr,0,0,dpr,0,0);
  ctx.clearRect(0,0,cv.width/dpr,cv.height/dpr);
}
function blitLayer(){
  ctx.setTransform(1,0,0,1,0,0);
  ctx.clearRect(0,0,cv.width,cv.height);
  ctx.drawImage(layer,0,0);
}
function crosshair(x,y){
  ctx.save();
  ctx.strokeStyle = 'rgba(255,180,84,.95)'; ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(x-8,y); ctx.lineTo(x+8,y); ctx.moveTo(x,y-8); ctx.lineTo(x,y+8); ctx.stroke();
  ctx.beginPath(); ctx.arc(x,y,3.5,0,Math.PI*2); ctx.stroke();
  ctx.restore();
}
function drawFrame(i, p, showPen){
  const tf = getTF(); if (!tf) return;
  if (i-1 > R.done) bakeUpTo(i-1);
  blitLayer();
  ctx.setTransform(tf.dpr,0,0,tf.dpr,0,0);
  ctx.lineCap = ctx.lineJoin = 'round';
  const o = R.ir[i];
  if (o){
    drawOp(ctx, o, p, tf);
    if (showPen && o.op === 'LINE'){
      const ex = o.x1+(o.x2-o.x1)*p, ey = o.y1+(o.y2-o.y1)*p;
      crosshair(ex*tf.s+tf.ox, ey*tf.s+tf.oy);
    }
  }
}
function drawStatic(){
  if (!R.ir || !R.ir.length){ clearMain(); return; }
  bakeUpTo(R.ir.length-1);
  blitLayer();
}
function playIR(ir, animate, onDone){
  const token = ++animToken;
  const note = $('note'), prog = $('prog');
  setIR(ir); currentIR = ir;
  if (!ir.length){
    isPlaying = false; clearMain();
    note.textContent = '그리기 명령이 없습니다';
    note.className = 'stage-note'; note.style.display = 'block';
    prog.style.width = '100%';
    onDone && onDone(); return;
  }
  note.style.display = 'none';
  if (!animate){
    isPlaying = false; drawStatic();
    prog.style.width = '100%';
    onDone && onDone(); return;
  }
  isPlaying = true;
  const total = ir.length;
  const perOp = Math.min(150, Math.max(0.01, 1200/total));
  const t0 = performance.now();
  requestAnimationFrame(function frame(now){
    if (token !== animToken) return;
    const el = now - t0;
    const i = Math.min(total-1, Math.floor(el/perOp));
    const p = Math.min(1, (el - i*perOp)/perOp);
    drawFrame(i, p, true);
    prog.style.width = (((i+p)/total)*100).toFixed(1) + '%';
    if (i < total-1 || p < 1) requestAnimationFrame(frame);
    else { isPlaying = false; drawStatic(); onDone && onDone(); }
  });
}

/* ---------- TRACER ---------- */
const SCAN_BUDGET = 200000, EDGE_BUDGET = 25000;
const TR = { img: null, W: 0, H: 0, strokes: [], runs: [], mode: 'scan' };
const dz = $('dropzone');
const trcCv = $('trcCv'), tctx = trcCv.getContext('2d');
const pxCv = document.createElement('canvas');
const pctx = pxCv.getContext('2d', { willReadFrequently: true });
function openTracer(){ $('trcBack').classList.add('open'); }
function closeTracer(){ $('trcBack').classList.remove('open'); }
function loadFile(file){
  if (!file || !file.type.startsWith('image/')) return;
  const url = URL.createObjectURL(file);
  const img = new Image();
  img.onload = () => { URL.revokeObjectURL(url); TR.img = img; dz.classList.add('has-img'); runTrace(); };
  img.onerror = () => {
    URL.revokeObjectURL(url); dz.classList.remove('has-img');
    const p = dz.querySelector('.dz-inner p');
    p.textContent = '⚠ 이미지를 읽을 수 없습니다'; p.style.color = 'var(--red)';
    setTimeout(() => { p.textContent = '이미지를 여기에 놓거나 클릭'; p.style.color = ''; }, 2200);
  };
  img.src = url;
}

/* 스캔라인: 행 단위 런 추출 — 허용오차 내 색 병합, tol=0이면 완전 보존 */
function scanlineRuns(data, W, H, tol, skipWhite){
  const runs = [];
  const hexC = (r,g,b) => ((1<<24)|(r<<16)|(g<<8)|b).toString(16).slice(1).toUpperCase();
  for (let y = 0; y < H; y++){
    let x = 0;
    while (x < W){
      const p = (y*W+x)*4;
      if (data[p+3] < 128){ x++; continue; }
      const br = data[p], bg = data[p+1], bb = data[p+2];
      if (skipWhite && (0.299*br + 0.587*bg + 0.114*bb) > 225){ x++; continue; }
      let x2 = x;
      while (x2+1 < W){
        const p2 = (y*W+x2+1)*4;
        if (data[p2+3] < 128) break;
        if (Math.abs(data[p2]-br) > tol || Math.abs(data[p2+1]-bg) > tol || Math.abs(data[p2+2]-bb) > tol) break;
        x2++;
      }
      runs.push({ x1: x+0.5, y: y+0.5, x2: x2+0.5, color: hexC(br,bg,bb) });
      x = x2+1;
    }
  }
  return runs;
}
function gaussBlur(src, W, H){
  const tmp = new Float32Array(W*H), out = new Float32Array(W*H);
  for (let y = 0; y < H; y++){
    const row = y*W;
    for (let x = 0; x < W; x++){
      const l = src[row+(x>0?x-1:0)], r = src[row+(x<W-1?x+1:x)], c = src[row+x];
      tmp[row+x] = (l + 2*c + r) * 0.25;
    }
  }
  for (let y = 0; y < H; y++){
    for (let x = 0; x < W; x++){
      const u = tmp[(y>0?y-1:0)*W+x], d = tmp[(y<H-1?y+1:y)*W+x], c = tmp[y*W+x];
      out[y*W+x] = (u + 2*c + d) * 0.25;
    }
  }
  return out;
}

function runTrace(){
  if (!TR.img) return;
  const iw = TR.img.naturalWidth, ih = TR.img.naturalHeight;
  if (!iw || !ih) return;
  const t0 = performance.now();
  const detail=+$('cDetail').value, thresh=+$('cThresh').value, minLen=+$('cMin').value,
        simp=+$('cSimp').value, denoise=+$('cDenoise').value, smooth=+$('cSmooth').value, tol=+$('cTol').value;
  const skipWhite = $('cSkipWhite').checked && TR.mode !== 'mono';
  const sc = detail / Math.max(iw, ih);
  const W = Math.max(8, Math.round(iw*sc)), H = Math.max(8, Math.round(ih*sc));
  TR.W = W; TR.H = H;
  pxCv.width = W; pxCv.height = H;
  pctx.drawImage(TR.img, 0, 0, W, H);
  const data = pctx.getImageData(0, 0, W, H).data;
  const fmtB = b => b < 1024 ? b+'B' : b < 1048576 ? (b/1024).toFixed(1)+'K' : (b/1048576).toFixed(1)+'M';

  if (TR.mode === 'scan'){
    let runs = scanlineRuns(data, W, H, tol, skipWhite);
    const over = runs.length > SCAN_BUDGET;
    if (over) runs.length = SCAN_BUDGET;
    TR.runs = runs; TR.strokes = [];
    drawTracePreview();
    setTStat('tsPx', W + '×' + H);
    setTStat('tsStroke', runs.length);
    setTStat('tsCmd', runs.length + (over ? ' ⚠' : ''));
    setTStat('tsByte', fmtB(new Blob([buildScanJVG(runs)]).size));
    setTStat('tsMs', (performance.now()-t0).toFixed(1));
    updateInsertBtn();
    return;
  }

  /* 엣지 모드: Gaussian → Sobel → NMS → Hysteresis → Sub-pixel */
  let gray = new Float32Array(W*H);
  for (let i = 0, p = 0; i < gray.length; i++, p += 4)
    gray[i] = 0.299*data[p] + 0.587*data[p+1] + 0.114*data[p+2];
  for (let k = 0; k < denoise; k++) gray = gaussBlur(gray, W, H);
  const mag = new Float32Array(W*H), dirX = new Float32Array(W*H), dirY = new Float32Array(W*H);
  let maxMag = 0;
  for (let y = 1; y < H-1; y++){
    for (let x = 1; x < W-1; x++){
      const i = y*W+x;
      const p00=gray[i-W-1],p01=gray[i-W],p02=gray[i-W+1],p10=gray[i-1],p12=gray[i+1],p20=gray[i+W-1],p21=gray[i+W],p22=gray[i+W+1];
      const gx = -p00-2*p01-p02+p20+2*p21+p22, gy = -p00+p02-2*p10+2*p12-p20+p22;
      const m = Math.hypot(gx,gy);
      mag[i] = m;
      if (m > 0){ const inv = 1/m; dirX[i] = gx*inv; dirY[i] = gy*inv; }
      if (m > maxMag) maxMag = m;
    }
  }
  const thin = new Float32Array(W*H);
  for (let y = 1; y < H-1; y++){
    for (let x = 1; x < W-1; x++){
      const i = y*W+x, m = mag[i];
      if (m < 1e-3) continue;
      const dx = Math.round(dirX[i]), dy = Math.round(dirY[i]);
      if (dx === 0 && dy === 0){ thin[i] = m; continue; }
      if (m >= mag[i+dy*W+dx] && m >= mag[i-dy*W-dx]) thin[i] = m;
    }
  }
  const hi = maxMag > 0 ? (thresh/255)*maxMag : Infinity, lo = hi*0.4;
  const edges = new Uint8Array(W*H), queue = [];
  for (let i = 0; i < W*H; i++){
    if (thin[i] >= hi){ edges[i] = 2; queue.push(i); }
    else if (thin[i] >= lo) edges[i] = 1;
  }
  let head = 0;
  while (head < queue.length){
    const i = queue[head++], x = i % W, y = (i/W)|0;
    for (let dy = -1; dy <= 1; dy++){
      const ny = y+dy; if (ny < 0 || ny >= H) continue;
      for (let dx = -1; dx <= 1; dx++){
        if (!dx && !dy) continue;
        const nx = x+dx; if (nx < 0 || nx >= W) continue;
        const ni = ny*W+nx;
        if (edges[ni] === 1){ edges[ni] = 2; queue.push(ni); }
      }
    }
  }
  const edgeMask = new Uint8Array(W*H);
  let edgeCount = 0;
  for (let i = 0; i < W*H; i++) if (edges[i] === 2){ edgeMask[i] = 1; edgeCount++; }
  const refX = new Float32Array(W*H), refY = new Float32Array(W*H);
  for (let i = 0; i < W*H; i++){
    const x = i % W, y = (i/W)|0;
    refX[i] = x; refY[i] = y;
    if (!edgeMask[i]) continue;
    const ox = Math.round(dirX[i]), oy = Math.round(dirY[i]);
    if (!ox && !oy) continue;
    const nx = x+ox; if (nx < 0 || nx >= W) continue;
    const i1 = i + oy*W + ox, i2 = i - oy*W - ox;
    if (i1 < 0 || i1 >= W*H || i2 < 0 || i2 >= W*H) continue;
    const m0 = mag[i2], m1 = mag[i], m2 = mag[i1];
    const den = m0 - 2*m1 + m2;
    if (Math.abs(den) < 1e-6) continue;
    let tt = (m0 - m2) / (2*den);
    if (tt < -0.5) tt = -0.5; if (tt > 0.5) tt = 0.5;
    refX[i] = x + tt*dirX[i]; refY[i] = y + tt*dirY[i];
  }
  const paths = traceStrokes(edgeMask, W, H);
  const r1 = v => { const n = Math.round(v*10)/10; return n === 0 ? 0 : n; };
  const out = [];
  let budget = EDGE_BUDGET;
  for (const path of paths){
    if (budget <= 0) break;
    if (path.length < minLen) continue;
    let pts = path.map(i => [refX[i], refY[i]]);
    for (let k = 0; k < smooth; k++) pts = chaikin(pts);
    if (simp > 0) pts = rdp(pts, simp);
    pts = pts.map(p => [r1(p[0]), r1(p[1])]);
    const dd = [pts[0]];
    for (let i = 1; i < pts.length; i++)
      if (pts[i][0] !== dd[dd.length-1][0] || pts[i][1] !== dd[dd.length-1][1]) dd.push(pts[i]);
    pts = dd;
    if (pts.length < 2) continue;
    let color = 'F5FAFF';
    if (TR.mode === 'color'){
      const mid = path[(path.length/2)|0], p = mid*4;
      const q = v => Math.round(v/51)*51;
      const r = q(data[p]), g = q(data[p+1]), b = q(data[p+2]);
      if (skipWhite && (0.299*r + 0.587*g + 0.114*b) > 225) continue;
      color = ((1<<24)|(r<<16)|(g<<8)|b).toString(16).slice(1).toUpperCase();
    }
    out.push({ pts, color });
    budget -= pts.length - 1;
  }
  TR.strokes = out; TR.runs = [];
  drawTracePreview();
  setTStat('tsPx', edgeCount);
  setTStat('tsStroke', out.length);
  setTStat('tsCmd', out.reduce((a,s) => a + s.pts.length - 1, 0) + (budget <= 0 ? ' ⚠' : ''));
  setTStat('tsByte', fmtB(new Blob([buildJVG(out)]).size));
  setTStat('tsMs', (performance.now()-t0).toFixed(1));
  updateInsertBtn();
}

function traceStrokes(edges, W, H){
  const visited = new Uint8Array(W*H), strokes = [];
  const DIRS = [[1,0],[-1,0],[0,1],[0,-1],[1,1],[1,-1],[-1,1],[-1,-1]];
  const walk = start => {
    const path = []; let cur = start, pd = null;
    for(;;){
      const cx = cur % W, cy = (cur/W)|0;
      let best = -1, bd = null, bestScore = -1;
      for (const d of DIRS){
        const nx = cx+d[0], ny = cy+d[1];
        if (nx<0||ny<0||nx>=W||ny>=H) continue;
        const ni = ny*W+nx;
        if (!edges[ni] || visited[ni]) continue;
        let s = 1;
        if (pd) s = (d[0]===pd[0]&&d[1]===pd[1]) ? 4 : ((d[0]*pd[0]+d[1]*pd[1]) > 0 ? 2 : 1);
        if (s > bestScore){ bestScore = s; best = ni; bd = d; }
      }
      if (best === -1) break;
      visited[best] = 1; path.push(best); cur = best; pd = bd;
    }
    return path;
  };
  for (let i = 0; i < W*H; i++){
    if (!edges[i] || visited[i]) continue;
    visited[i] = 1;
    const fwd = walk(i), back = walk(i);
    const path = back.reverse().concat([i], fwd);
    if (path.length >= 2) strokes.push(path);
  }
  return strokes;
}
function chaikin(pts){
  if (pts.length <= 2) return pts;
  const out = [pts[0]];
  for (let i = 0; i < pts.length-1; i++){
    const a = pts[i], b = pts[i+1];
    out.push([a[0]*0.75+b[0]*0.25, a[1]*0.75+b[1]*0.25]);
    out.push([a[0]*0.25+b[0]*0.75, a[1]*0.25+b[1]*0.75]);
  }
  out.push(pts[pts.length-1]);
  return out;
}
function rdp(pts, eps){
  if (pts.length <= 2) return pts.slice();
  const keep = new Uint8Array(pts.length);
  keep[0] = keep[pts.length-1] = 1;
  const stack = [[0, pts.length-1]], e2 = eps*eps;
  while (stack.length){
    const [a, b] = stack.pop();
    let maxD = -1, idx = -1;
    const ax=pts[a][0],ay=pts[a][1],bx=pts[b][0],by=pts[b][1];
    const dx=bx-ax,dy=by-ay,len2=dx*dx+dy*dy;
    for (let i=a+1;i<b;i++){
      const px=pts[i][0]-ax, py=pts[i][1]-ay;
      let d2;
      if (len2 === 0) d2 = px*px+py*py;
      else { const t = Math.max(0,Math.min(1,(px*dx+py*dy)/len2)); const qx=px-t*dx,qy=py-t*dy; d2=qx*qx+qy*qy; }
      if (d2 > maxD){ maxD = d2; idx = i; }
    }
    if (maxD > e2 && idx !== -1){ keep[idx]=1; stack.push([a,idx],[idx,b]); }
  }
  return pts.filter((_,i) => keep[i]);
}

const fNum = v => { const s = v.toFixed(1); return s.endsWith('.0') ? s.slice(0,-2) : s; };
function buildScanJVG(runs){
  const L = ['JVG("1.0")', '', 'Weight(1)'];
  const groups = new Map();
  for (const r of runs){ if (!groups.has(r.color)) groups.set(r.color, []); groups.get(r.color).push(r); }
  for (const [color, rs] of groups){
    L.push('', 'Color("' + color + '")');
    for (const r of rs) L.push('Line(' + fNum(r.x1) + ',' + fNum(r.y) + ',' + fNum(r.x2) + ',' + fNum(r.y) + ')');
  }
  return L.join('\n');
}
function buildJVG(strokes){
  const seg = (s, L) => {
    for (let i=0;i<s.pts.length-1;i++)
      L.push('Line(' + fNum(s.pts[i][0]) + ',' + fNum(s.pts[i][1]) + ',' + fNum(s.pts[i+1][0]) + ',' + fNum(s.pts[i+1][1]) + ')');
  };
  const L = ['JVG("1.0")', '', 'Weight(1)'];
  if (TR.mode === 'mono'){
    L.push('Color("F5FAFF")', '');
    for (const s of strokes) seg(s, L);
  } else {
    const groups = new Map();
    for (const s of strokes){ if (!groups.has(s.color)) groups.set(s.color, []); groups.get(s.color).push(s); }
    for (const [color, sts] of groups){
      L.push('', 'Color("' + color + '")');
      for (const s of sts) seg(s, L);
    }
  }
  return L.join('\n');
}

function drawTracePreview(){
  const W = TR.W, H = TR.H;
  if (!W || !H) return;
  if (trcCv.width !== W*2 || trcCv.height !== H*2){ trcCv.width = W*2; trcCv.height = H*2; }
  tctx.setTransform(2,0,0,2,0,0);
  tctx.clearRect(0,0,W,H);
  tctx.globalAlpha = 0.25;
  tctx.drawImage(TR.img, 0, 0, W, H);
  tctx.globalAlpha = 1;
  tctx.lineCap = tctx.lineJoin = 'round';
  if (TR.mode === 'scan'){
    tctx.lineWidth = 1;
    if (TR.runs.length > 40000){
      tctx.globalAlpha = 0.9;
      tctx.drawImage(TR.img, 0, 0, W, H);
      tctx.globalAlpha = 1;
      return;
    }
    const groups = new Map();
    for (const r of TR.runs){ if (!groups.has(r.color)) groups.set(r.color, []); groups.get(r.color).push(r); }
    for (const [color, rs] of groups){
      tctx.strokeStyle = '#'+color;
      tctx.beginPath();
      for (const r of rs){ tctx.moveTo(r.x1, r.y); tctx.lineTo(r.x2, r.y); }
      tctx.stroke();
    }
    return;
  }
  tctx.lineWidth = 0.9;
  const trace = sts => {
    tctx.beginPath();
    for (const s of sts){
      tctx.moveTo(s.pts[0][0], s.pts[0][1]);
      for (let i = 1; i < s.pts.length; i++) tctx.lineTo(s.pts[i][0], s.pts[i][1]);
    }
    tctx.stroke();
  };
  if (TR.mode === 'mono'){ tctx.strokeStyle = '#4FD8EB'; trace(TR.strokes); }
  else {
    const groups = new Map();
    for (const s of TR.strokes){ if (!groups.has(s.color)) groups.set(s.color, []); groups.get(s.color).push(s); }
    for (const [color, sts] of groups){ tctx.strokeStyle = '#'+color; trace(sts); }
  }
}
function setTStat(id, v){
  const el = $(id); el.textContent = v;
  const box = el.parentElement;
  box.classList.remove('flash'); void box.offsetWidth; box.classList.add('flash');
}
function traceCount(){ return TR.mode === 'scan' ? TR.runs.length : TR.strokes.length; }
function updateInsertBtn(){
  const n = traceCount();
  const btn = $('trcInsert');
  btn.classList.toggle('dis', !n);
  btn.textContent = n ? '에디터에 삽입 ▶' : '추적 결과 없음';
}
function updateModeUI(){
  const scan = TR.mode === 'scan';
  [['cThresh',!scan],['cMin',!scan],['cSmooth',!scan],['cSimp',!scan],['cTol',scan]].forEach(([id,on]) => {
    $(id).closest('.ctl').classList.toggle('off', !on);
  });
  $('skipWhiteWrap').style.opacity = TR.mode === 'mono' ? .35 : 1;
}

/* ---------- UI ---------- */
const ta = $('src'), gutter = $('gutter'), logView = $('v-log');
let errLine = -1, deb = null, compileSeq = 0, lastLines = -1, lastErrLine = -2;
function log(kind, tag, msg){
  const t = new Date().toTimeString().slice(0,8);
  const cls = {ok:'tag-ok',warn:'tag-warn',err:'tag-err',info:'tag-info',state:'tag-state'}[kind] || 'tag-info';
  logView.insertAdjacentHTML('beforeend',
    '<div class="log-e ' + kind + '"><span class="log-t">' + t + '</span><span class="log-tag ' + cls + '">' + tag + '</span><span class="log-m">' + esc(msg) + '</span></div>');
  logView.scrollTop = logView.scrollHeight;
}
function setLED(state){ $('led').className = 'led ' + (state||''); }
function setStatus(txt){ $('statusTxt').textContent = txt; }
function setStage(i, state){
  const el = $('sg'+i);
  el.className = 'stg' + (state ? ' ' + state : '');
  if (state === 'done') el.querySelector('i').textContent = '✓';
}
function resetStages(){ for (let i=0;i<4;i++){ setStage(i,''); $('sg'+i).querySelector('i').textContent = i+1; } }
function setStat(id, val){
  const el = $(id); el.textContent = val;
  const box = el.parentElement;
  box.classList.remove('flash'); void box.offsetWidth; box.classList.add('flash');
}
function updateGutter(){
  const n = ta.value.split('\n').length;
  if (n === lastLines && errLine === lastErrLine) return;
  lastLines = n; lastErrLine = errLine;
  const show = Math.min(n, 4000);
  let h = '';
  for (let i = 1; i <= show; i++) h += '<span' + (i === errLine ? ' class="err"' : '') + '>' + i + '</span>';
  if (n > 4000) h += '<span>…</span>';
  gutter.innerHTML = h;
}
function updateMeta(){
  const v = ta.value;
  $('metaSize').textContent = new Blob([v]).size + ' B';
  const pos = ta.selectionStart, upto = v.slice(0, pos);
  $('metaPos').textContent = 'Ln ' + upto.split('\n').length + ', Col ' + (pos - upto.lastIndexOf('\n'));
}
function renderTokens(tokens){
  const list = tokens.filter(t => t.type !== 'EOF');
  let h = list.slice(0, 800).map(t => {
    if (t.type === 'STRING') return '<span class="tk tk-str">"' + esc(t.value) + '"</span>';
    if (t.type === 'NUMBER') return '<span class="tk tk-num">' + t.value + '</span>';
    if (t.type === 'IDENT') return '<span class="tk tk-id">' + esc(t.value) + '</span>';
    return '<span class="tk tk-pun">' + esc(t.value) + '</span>';
  }).join('');
  if (list.length > 800) h += '<span class="tk tk-pun">… +' + (list.length-800) + ' more</span>';
  $('v-tokens').innerHTML = h || '<span class="tk tk-pun">(empty)</span>';
}
function renderAST(ast){
  let j = esc(JSON.stringify(ast, null, 2));
  j = j.replace(/"([a-zA-Z]+)"(?=:)/g, '<span class="j-key">"$1"</span>')
       .replace(/: "([^"]*)"/g, ': <span class="j-str">"$1"</span>')
       .replace(/: (-?\d+(?:\.\d+)?)/g, ': <span class="j-num">$1</span>');
  $('v-ast').innerHTML = '<pre>' + j + '</pre>';
}
function renderIR(ir){
  const CAP = 2000;
  let html = ir.slice(0, CAP).map((o,i) => {
    const idx = String(i).padStart(2,'0');
    if (o.op === 'LINE')
      return '<div class="ir-line" data-i="' + i + '"><span class="ir-i">' + idx + '</span><span class="ir-op op-line">LINE</span>' +
        '<span class="ir-args">' + o.x1 + ',' + o.y1 + ' → ' + o.x2 + ',' + o.y2 + '</span>' +
        '<span class="ir-meta"><i style="background:#' + o.color + '"></i>#' + o.color + ' · w' + o.weight + '</span></div>';
    return '<div class="ir-line" data-i="' + i + '"><span class="ir-i">' + idx + '</span><span class="ir-op op-fill">FILL</span>' +
      '<span class="ir-args">polygon(' + o.pts.length + ')  ' + o.pts.map(p => p.join(',')).join('  ') + '</span>' +
      '<span class="ir-meta"><i style="background:#' + o.color + '"></i>#' + o.color + '</span></div>';
  }).join('');
  if (ir.length > CAP)
    html += '<div class="ir-line"><span class="ir-i">…</span><span class="ir-op" style="color:var(--dim)">+' + (ir.length-CAP) + '</span>' +
      '<span class="ir-args" style="color:var(--dim)">ops 생략 — ⬇ IR 다운로드로 전체 확인</span></div>';
  $('v-ir').innerHTML = html || '<span class="tk tk-pun">IR 없음</span>';
}

async function compile(source, animate){
  clearTimeout(deb);
  const seq = ++compileSeq;
  animToken++; isPlaying = false;
  errLine = -1; updateGutter();
  setIR([]);
  setLED('busy'); setStatus('COMPILING…');
  resetStages();
  logView.innerHTML=''; $('v-tokens').innerHTML=''; $('v-ast').innerHTML=''; $('v-ir').innerHTML='';
  $('prog').style.width = '0%';
  const note = $('note'); note.style.display='none'; note.className='stage-note';
  try {
    setStage(0,'active');
    const t0 = performance.now();
    const tokens = lex(source);
    const lexMs = performance.now() - t0;
    renderTokens(tokens);
    log('info','LEX','토큰 ' + (tokens.length-1) + '개 · ' + lexMs.toFixed(2) + 'ms');
    setStage(0,'done');
    if (animate){ await sleep(150); if (seq !== compileSeq) return; }
    setStage(1,'active');
    const ast = parse(tokens);
    renderAST(ast);
    log('info','AST','명령 ' + ast.length + '개 파싱 완료');
    setStage(1,'done');
    if (animate){ await sleep(150); if (seq !== compileSeq) return; }
    setStage(2,'active');
    const gen = codegen(ast, log);
    const ms = performance.now() - t0;
    renderIR(gen.ir);
    setStat('stTok', tokens.length-1);
    setStat('stCmd', ast.length);
    setStat('stOps', gen.ir.length);
    setStat('stLoop', gen.loops);
    setStat('stMs', ms.toFixed(1));
    setStat('stBytes', new Blob([JSON.stringify(gen.ir)]).size + 'B');
    setStage(2,'done');
    if (animate){ await sleep(130); if (seq !== compileSeq) return; }
    setStage(3,'active');
    log('ok','OK','컴파일 완료 — ' + gen.ir.length + ' ops · ' + ms.toFixed(1) + 'ms');
    playIR(gen.ir, animate, () => {
      if (seq !== compileSeq) return;
      setStage(3,'done'); setLED('ok');
      setStatus('OK · ' + gen.ir.length + ' ops · ' + ms.toFixed(1) + 'ms');
    });
  } catch (e){
    if (seq !== compileSeq) return;
    if (e instanceof JvgError){
      errLine = e.line; updateGutter();
      log('err','ERR','line ' + e.line + ':' + e.col + ' — ' + e.message);
      for (let i=0;i<4;i++){ const el=$('sg'+i); if (el.classList.contains('active')) setStage(i,'fail'); }
      setLED('err'); setStatus('ERROR · L' + e.line);
      note.textContent = '⚠ 컴파일 오류 — line ' + e.line;
      note.className = 'stage-note err'; note.style.display = 'block';
      clearMain();
      $('prog').style.width = '0%';
    } else {
      log('err','ERR','내부 오류 — ' + (e && e.message ? e.message : String(e)));
      setLED('err'); setStatus('INTERNAL ERROR');
      note.textContent = '⚠ 내부 오류';
      note.className = 'stage-note err'; note.style.display = 'block';
      clearMain();
    }
  }
}

/* ---------- 이벤트 ---------- */
$('compile').addEventListener('click', () => compile(ta.value, true));
$('replay').addEventListener('click', () => { if (currentIR) playIR(currentIR, true); });
ta.addEventListener('input', () => {
  updateGutter(); updateMeta();
  if ($('auto').checked){ clearTimeout(deb); deb = setTimeout(() => compile(ta.value, false), 400); }
});
ta.addEventListener('scroll', () => { gutter.scrollTop = ta.scrollTop; });
ta.addEventListener('click', updateMeta);
ta.addEventListener('keyup', updateMeta);
ta.addEventListener('keydown', e => {
  if (e.key === 'Tab'){
    e.preventDefault();
    const s = ta.selectionStart, ep = ta.selectionEnd;
    ta.value = ta.value.slice(0,s) + '  ' + ta.value.slice(ep);
    ta.selectionStart = ta.selectionEnd = s + 2;
    ta.dispatchEvent(new Event('input', {bubbles:true}));
  }
  if ((e.ctrlKey || e.metaKey) && e.key === 'Enter'){ e.preventDefault(); compile(ta.value, true); }
});
document.querySelectorAll('.tab').forEach(b => b.addEventListener('click', () => {
  document.querySelectorAll('.tab').forEach(x => x.classList.remove('on'));
  document.querySelectorAll('.view').forEach(x => x.classList.remove('on'));
  b.classList.add('on'); $('v-' + b.dataset.v).classList.add('on');
}));
$('v-ir').addEventListener('mouseover', e => {
  const row = e.target.closest('.ir-line');
  if (!row || isPlaying || !currentIR || !currentIR.length) return;
  const o = currentIR[+row.dataset.i];
  if (!o) return;
  drawStatic();
  const tf = getTF(); if (!tf) return;
  ctx.setTransform(tf.dpr,0,0,tf.dpr,0,0);
  ctx.lineCap = ctx.lineJoin = 'round';
  ctx.save();
  ctx.shadowColor = 'rgba(255,255,255,.85)'; ctx.shadowBlur = 12;
  drawOp(ctx, o, 1, tf);
  ctx.restore();
});
$('v-ir').addEventListener('mouseleave', () => { if (!isPlaying) drawStatic(); });
function download(name, text, mime){
  const url = URL.createObjectURL(new Blob([text], {type: mime}));
  const a = Object.assign(document.createElement('a'), {href: url, download: name});
  document.body.appendChild(a); a.click(); a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
$('dljvg').addEventListener('click', () => download('drawing.jvg', ta.value, 'text/plain;charset=utf-8'));
$('dlir').addEventListener('click', () => {
  if (!currentIR || !currentIR.length) return;
  download('drawing.ir.json', JSON.stringify({ format:'JVG-IR', ops: currentIR }, null, 2), 'application/json');
});

/* ---------- TRACER 이벤트 ---------- */
$('traceBtn').addEventListener('click', openTracer);
$('trcClose').addEventListener('click', closeTracer);
$('trcCancel').addEventListener('click', closeTracer);
$('trcBack').addEventListener('click', e => { if (e.target === $('trcBack')) closeTracer(); });
document.addEventListener('keydown', e => { if (e.key === 'Escape') closeTracer(); });
dz.addEventListener('click', () => $('trcFile').click());
$('trcFile').addEventListener('change', e => { loadFile(e.target.files[0]); e.target.value = ''; });
['dragenter','dragover'].forEach(ev => dz.addEventListener(ev, e => { e.preventDefault(); dz.classList.add('over'); }));
['dragleave','drop'].forEach(ev => dz.addEventListener(ev, e => { e.preventDefault(); dz.classList.remove('over'); }));
dz.addEventListener('drop', e => loadFile(e.dataTransfer.files[0]));
document.addEventListener('paste', e => {
  const cd = e.clipboardData;
  if (!cd || !cd.items) return;
  for (let k = 0; k < cd.items.length; k++){
    const it = cd.items[k];
    if (it && it.type && it.type.startsWith('image/')){ openTracer(); loadFile(it.getAsFile()); break; }
  }
});
let trcDeb = null;
['cDetail','cThresh','cMin','cSimp','cDenoise','cSmooth','cTol'].forEach(id => $(id).addEventListener('input', () => {
  $('vDetail').textContent = $('cDetail').value;
  $('vThresh').textContent = $('cThresh').value;
  $('vDenoise').textContent = $('cDenoise').value;
  $('vMin').textContent = $('cMin').value;
  $('vSmooth').textContent = $('cSmooth').value;
  $('vSimp').textContent = (+$('cSimp').value).toFixed(1);
  $('vTol').textContent = $('cTol').value;
  clearTimeout(trcDeb); trcDeb = setTimeout(runTrace, 200);
}));
$('modeSeg').querySelectorAll('button').forEach(b => b.addEventListener('click', () => {
  $('modeSeg').querySelectorAll('button').forEach(x => x.classList.remove('on'));
  b.classList.add('on');
  TR.mode = b.dataset.m;
  updateModeUI();
  runTrace();
}));
$('cSkipWhite').addEventListener('change', runTrace);
$('trcInsert').addEventListener('click', () => {
  if (!traceCount()){
    const btn = $('trcInsert');
    btn.classList.remove('shake'); void btn.offsetWidth; btn.classList.add('shake');
    return;
  }
  ta.value = TR.mode === 'scan' ? buildScanJVG(TR.runs) : buildJVG(TR.strokes);
  document.querySelectorAll('.chip').forEach(x => x.classList.remove('on'));
  updateGutter(); updateMeta();
  closeTracer();
  const ep = document.querySelector('.editor-panel');
  ep.classList.add('glow');
  setTimeout(() => ep.classList.remove('glow'), 1000);
  compile(ta.value, true);
});

/* ---------- 샘플 ---------- */
const SAMPLES = {
square: 'JVG("1.0")\n\nColor("0066FF")\n\nLine(100,100,300,100)\nLine(300,100,300,300)\nLine(300,300,100,300)\nLine(100,300,100,100)\n\nFill()',
house: 'JVG("1.0")\n\nColor("FFB454")\nLine(120,240,120,140)\nLine(120,140,300,140)\nLine(300,140,300,240)\nLine(300,240,120,240)\nFill()\n\nColor("4FD8EB")\nLine(105,140,210,55)\nLine(210,55,315,140)\nLine(315,140,105,140)\nFill()\n\nColor("5CE0A5")\nLine(190,240,190,185)\nLine(190,185,230,185)\nLine(230,185,230,240)\nLine(230,240,190,240)\nFill()',
star: 'JVG("1.0")\n\nColor("FFD23F")\n\nLine(160,60,185,126)\nLine(185,126,255,129)\nLine(255,129,200,173)\nLine(200,173,219,241)\nLine(219,241,160,202)\nLine(160,202,101,241)\nLine(101,241,120,173)\nLine(120,173,65,129)\nLine(65,129,135,126)\nLine(135,126,160,60)\n\nFill()',
letters: 'JVG("1.0")\n\nWeight(7)\n\nColor("FFB454")\nLine(40,60,105,60)\nLine(88,60,88,170)\nLine(88,170,68,188)\nLine(68,188,44,172)\n\nColor("4FD8EB")\nLine(135,60,163,188)\nLine(163,188,191,60)\n\nColor("5CE0A5")\nLine(272,78,252,60)\nLine(252,60,222,60)\nLine(222,60,204,78)\nLine(204,78,204,168)\nLine(204,168,222,186)\nLine(222,186,252,186)\nLine(252,186,272,168)\nLine(272,168,272,128)\nLine(272,128,242,128)',
open: 'JVG("1.0")\n\nColor("FF6B7A")\nWeight(4)\n\nLine(70,70,260,70)\nLine(260,70,165,230)\n\nFill()',
late: 'JVG("1.0")\n\nColor("5CE0A5")\n\nLine(80,80,240,80)\n\nFill()\n\nLine(240,80,240,220)\nLine(240,220,80,220)\nLine(80,220,80,80)\n\nFill()'
};
document.querySelectorAll('.chip').forEach(c => c.addEventListener('click', () => {
  document.querySelectorAll('.chip').forEach(x => x.classList.remove('on'));
  c.classList.add('on');
  ta.value = SAMPLES[c.dataset.s];
  updateGutter(); updateMeta();
  compile(ta.value, true);
}));
new ResizeObserver(() => { sizeCanvas(); if (!isPlaying) drawStatic(); }).observe(wrap);

sizeCanvas();
updateModeUI();
updateInsertBtn();
ta.value = SAMPLES.house;
document.querySelector('.chip[data-s="house"]').classList.add('on');
updateGutter(); updateMeta();
compile(ta.value, true);
</script>
</body>
</html>
