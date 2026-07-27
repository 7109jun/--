# LJM Runtime 원리

LJM Viewer가 소스를 화면으로 바꾸는 전 과정과,
그 뒤에 있는 설계 원칙을 설명합니다.

---

## 설계 원칙 3가지

### 1. 보안 우선 — eval 금지

사용자가 쓴 코드는 **절대 JavaScript 엔진에 직접 전달되지 않습니다.**
`eval()`, `new Function()`을 쓰면 문서 하나가 브라우저 전체를 장악할 수 있기 때문입니다.

대신 LJM은 모든 코드를 **AST(추상 구문 트리)로 변환한 뒤,
직접 만든 인터프리터가 노드를 하나씩 순회하며 실행**합니다.
인터프리터가 모르는 명령은 실행 자체가 불가능합니다.

### 2. 해석과 실행의 분리

파서는 의미를 분석만 하고, 렌더러는 그리기만 합니다.
서로를 모르기 때문에 한쪽을 교체해도 다른쪽이 깨지지 않습니다.

### 3. 플러그인 확장

효과(Effect)는 등록(register)되는 플러그인입니다.
본체 코드를 한 줄도 수정하지 않고 효과를 무한히 추가할 수 있습니다.

---

## 전체 파이프라인

```
┌─────────────────────────────────────────────────────┐
│                    LJM Source (.ljm)                │
└──────────────────────────┬──────────────────────────┘
                           ▼
              ┌────────────────────────┐
              │     DocTokenizer       │  문자열 → 토큰
              └────────────┬───────────┘
                           ▼
              ┌────────────────────────┐
              │       DocParser        │  토큰 → 문서 AST
              └────────────┬───────────┘
                           ▼
              ┌────────────────────────┐
              │       Renderer         │  AST → DOM
              └────────────┬───────────┘
                           ▼
              ┌────────────────────────┐
              │       Runtime          │  페이지/이벤트/루프 총괄
              └────────────────────────┘
```

스크립트 블록 `([ ... ])`은 **별도의 파이프라인**을 탑니다:

```
Script Code
    ▼
ScriptTokenizer → ScriptParser → Script AST → ScriptInterpreter
                                                  │
                                    print / goto / camera / timer
                                                  ▼
                                              Runtime API
```

---

## 토큰과 AST, 눈으로 확인하기

이 소스가:

```ljm
(Hello)[40]
```

토크나이저를 거치면:

```js
{ type:'SIZED', text:'Hello', size:40, l:1, c:1 }
```

이 토큰이 파서를 거쳐 페이지 AST에 담기고,
렌더러는 이를 `<div style="font-size:40px">Hello</div>`로 변환합니다.

> 토큰에 `l`(line), `c`(column)가 함께 저장되기 때문에
> 에러 발생 시 **정확한 위치**를 Console에 표시할 수 있습니다.

---

## 스크립트 실행의 원리

```ljm
var hp = 100
if hp > 50 {
  print("alive")
}
```

이 코드는 다음 AST로 변환됩니다:

```
Program
├── Var (name: hp)
│   └── Num (100)
└── If
    ├── Bin (>)
    │   ├── VarRef (hp)
    │   └── Num (50)
    └── Block
        └── Print
            └── Str ("alive")
```

`ScriptInterpreter`는 이 트리를 재귀적으로 순회합니다:

1. `Var` 노드 → 스코프에 `hp = 100` 저장
2. `If` 노드 → 조건식 평가 (`100 > 50` → `true`)
3. `Block` 실행 → `Print` 노드 → Console 출력

**인터프리터에 구현된 노드 타입만 실행 가능**하므로,
임의 코드 실행이 원천적으로 차단됩니다.

---

## 효과 시스템 — 플러그인 계약서

모든 효과는 이 3개의 함수만 구현하면 됩니다:

```js
EffectManager.register({
  name: "Fire",
  start(el, opts)  { /* 효과 시작 시 1회 */ },
  update(el, t)    { /* 매 프레임 호출, t = 경과 시간(ms) */ },
  stop(el)         { /* 정리: 스타일/자식 요소 원복 */ }
});
```

### 새 효과 추가 예시 — Blink

```js
APP.fx.register({
  name: "Blink",
  update(el, t) {
    el.style.opacity = Math.sin(t / 200) > 0 ? "1" : "0.1";
  },
  stop(el) { el.style.opacity = ""; }
});
```

등록 즉시 `*[글자,Blink]`로 사용 가능합니다.
`update`는 Runtime의 `requestAnimationFrame` 루프가 매 프레임 호출하므로
별도의 타이머를 만들 필요가 없습니다.

---

## 프레임 루프

```js
const loop = (t) => {
  fx.update(t);              // 모든 활성 효과의 update() 호출
  raf = requestAnimationFrame(loop);
};
```

- 애니메이션은 전부 이 **단일 루프**에서 처리됩니다 (타이머 남발 금지)
- `Stop` 버튼 → `cancelAnimationFrame` + 모든 타이머/이벤트/효과 일괄 제거
- 효과 인스턴스는 `stop()` 시 스타일을 원복하므로 DOM 오염이 없습니다

---

## 클래스 책임표

| 클래스 | 책임 | 하지 않는 일 |
|---|---|---|
| `DocTokenizer` | 문자열 → 토큰 | 의미 해석 |
| `DocParser` | 토큰 → 문서 AST | DOM 조작 |
| `ScriptTokenizer` | 스크립트 → 토큰 | 실행 |
| `ScriptParser` | 토큰 → 스크립트 AST | 실행 |
| `ScriptInterpreter` | AST 실행 | DOM 직접 조작 |
| `MathParser` | 수식 안전 평가 | JS 위임 |
| `Renderer` | AST → DOM | 파싱 |
| `EffectManager` | 효과 플러그인 등록/실행 | 렌더링 |
| `CameraManager` | 화면 변환 행렬 | 효과 |
| `TimerManager` | 타이머 생명주기 | 로직 |
| `Runtime` | 전체 오케스트레이션 | 개별 파싱 |
| `ConsoleManager` | 로그 (Line/Col 포함) | 실행 |
| `FileManager` | .ljm 입출력 | 파싱 |

---

## 새 문법 추가 가이드

**1. 문서 문법 추가** (예: `!이미지!`)

| 순서 | 수정 위치 |
|---|---|
| ① 토큰 읽기 | `DocTokenizer.readImage()` 추가 |
| ② AST 노드 | `DocParser`에서 `IMAGE` 타입 처리 |
| ③ 렌더링 | `Renderer.node()`에 `case 'IMAGE'` 추가 |

**2. 스크립트 문법 추가** (예: `sound`)

| 순서 | 수정 위치 |
|---|---|
| ① 키워드 등록 | `ScriptTokenizer`의 KW 집합 |
| ② 파싱 규칙 | `ScriptParser.pSound()` 추가 |
| ③ 실행 규칙 | `ScriptInterpreter.exec()`에 case 추가 |

각 단계가 독립되어 있어 **한 클래스만 수정하면 됩니다.**

---

## 성능 원칙

1. **DOM 생성 최소화** — 페이지 단위로 한 번만 생성
2. **단일 rAF 루프** — 효과 수와 무관하게 루프는 1개
3. **이벤트 일괄 해제** — 페이지 전환/Stop 시 누수 없음
4. **스타일 원복** — `stop()`이 시작 전 상태로 되돌림
