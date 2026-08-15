# wastc (wast2 / wastc_v6) — 가이드

이 문서는 `wastc_v6` (파일: `wast2.cpp`) — 단일 파일 WAST → C++23 트랜스파일러(및 런타임 포함)에 대한 사용법, 구현 요약, 주의사항 및 기여 안내를 담고 있습니다.

원본 파일(참고)
- Repository: 7109jun/--
- Commit: `5c198e673a6d300f157281ead79551109506b181`
- 파일: https://github.com/7109jun/--/blob/5c198e673a6d300f157281ead79551109506b181/wast2.cpp

---

## 요약

- `wastc_v6`는 WAST(간단한 스크립트 언어)를 C++23 코드로 트랜스파일하고, 자체 런타임을 포함한 단일 C++ 파일을 생성합니다.
- 런타임은 값 타입(`wast::Value`)을 구현해 동적 타입 처리(숫자, 문자열, 배열, 맵, 함수 등)를 지원합니다.
- 코드베이스는 렉서 → 파서 → AST → 옵티마이저(상수 폴딩) → 코드 생성 순으로 구성됩니다.
- 언어는 함수, 지역 변수, 제어문(if/while/for), 구조체/열거, 패턴 매칭, 라이브러리(트리플 쿼트로 임베디드 C++ 코드) 등을 지원합니다.

---

## 빌드 (트랜스파일러 자체)

권장: 시스템에 C++23을 지원하는 컴파일러 필요.

예:
```bash
g++ -std=c++23 -O3 -march=native -flto -funroll-loops wast2.cpp -o wastc
```

성공적으로 빌드되면 `./wastc`로 실행 가능.

---

## 사용법 (트랜스파일러)

기본:
```bash
./wastc input.wast output.cpp
```
- `input.wast`: 트랜스파일할 WAST 소스
- `output.cpp`: 생성할 C++23 파일(생략하면 `input.wast.cpp`)

트랜스파일러는 생성된 C++ 파일에 런타임 코드(단일 파일)와 변환된 함수들을 포함합니다. 생성된 C++을 다시 컴파일해야 실행 가능한 바이너리가 됩니다.

---

## WAST 문법 요약 / 신규 문법

- 함수 정의:
  ```
  fn add(a: i64, b: i64) -> i64 ( 
      return a + b;
  )
  ```

- 라이브러리(트리플 쿼트 C++ 임베딩):
  ```
  library add(a: i64, b: i64) -> i64 """
      return a + b;
  """
  ```
  - 삼중 따옴표(`""" ... """`)로 감싼 C++ 코드를 그대로 생성된 C++ 안에 주입합니다.
  - 반환 타입으로 `i64, f64, bool, str, value, nil` 등을 지원합니다.

- 변수:
  - 선언: `local x = 1` 또는 `const x = 2`
- 제어문:
  - `if <cond> ( ... )`, `elseif`는 단일 토큰 `elseif`로 인식됩니다 (주의: `else if`(두 개 토큰)는 현재 허용되지 않음).
  - `while <cond> ( ... )`
  - `for i = start, end, step ( ... )` — 숫자형 for
  - `for x in collection ( ... )` — 반복자 for
- 배열/맵 리터럴: `[1,2,3]`, `{a: 1, "b": 2}`
- 패턴 매칭: `match expr ( "x" -> ( ... ), _ -> ... )`
- 문자열 내 보간(interpolation): `"Hello {name}"` — 보간은 제한적이며 단순 식별자 또는 숫자만 지원

---

## 런타임 API (생성된 C++ 안의 `wast::` 네임스페이스)

주요 함수/유틸:
- 타입/값 관련:
  - `wast::Value` : 동적 값 타입
  - `wast::truthy(Value)`, `wast::to_int`, `wast::to_float`, `wast::to_str`
- 컬렉션:
  - `wast::make_array`, `wast::make_map`, `wast::index`, `wast::set_index`, `wast::field`, `wast::set_field`
  - `wast::len`, `wast::push`, `wast::pop`, `wast::keys`, `wast::values`
- 연산:
  - `wast::add`, `wast::sub`, `wast::mul`, `wast::divide`, `wast::mod`
  - 비교: `wast::eq`, `wast::ne`, `wast::lt`, `wast::le`, `wast::gt`, `wast::ge`
- 유틸:
  - `wast::print(std::vector<Value>)` — 표준 출력
  - `wast::call(Value fn, std::vector<Value>)` — 클로저/함수 호출
  - `wast::range(start, end, inclusive)` — range 생성
  - `wast::to_iterable(Value)` — 반복 가능한 값 반환

생성된 함수는 모두 시그니처 `Value func(std::vector<Value> args)` 형태입니다.

---

## 예시

간단한 WAST:
```
fn main() (
    local a = 1
    local b = 2
    print(a + b)
)
```

트랜스파일 과정:
1. wastc가 `wast2.cpp`를 이용해 `output.cpp` 생성
2. `g++ -std=c++23 output.cpp -o out` 로 컴파일
3. `./out` 실행하면 `3` 출력

라이브러리 예:
```
library add(a: i64, b: i64) -> i64 """
    return a + b;
"""
fn main() (
    print(add(3, 4))
)
```
- `library` 블록은 생성된 C++ 함수 본문에 그대로 주입됩니다. 반환 타입 변환(예: `i64` → `Value`)은 트랜스파일러가 처리합니다.

---

## 구현 개요 (내부 구조)

- Lexer
  - 토큰화: 식별자, 숫자(정수/부동소수), 문자열, 삼중 따옴표(raw C++ 코드), 연산자 등
  - 주석: `//` 라인 주석 지원
- Parser
  - 재귀 하강 방식으로 AST를 구성
  - 함수/라이브러리/구조체/열거/매치/표현식/문 등 처리
- AST (노드 타입)
  - Program, FuncDef, LibraryFunc, StructDecl, EnumDecl 등과 표현식 노드(IntLit, StrLit, BinOp 등)
- Optimizer
  - 상수 폴딩: 문자열 결합, 숫자 연산, 불리언 연산 등 기본적인 폴딩 수행
- CodeGen
  - 트리에서 C++ 코드 텍스트 생성: 런타임 포함, struct/enum/library/function 코드 생성
  - Value 기반 런타임 호출로 모든 연산을 래핑

---

## 알려진 문제 및 주의사항

1. C++23 의존성(호환성)
   - 코드(런타임과 main에서) `#include <print>`와 `std::print`/`std::println`을 사용합니다.
   - 많은 컴파일러/표준 라이브러리(특히 시스템의 g++/libstdc++/clang/stdlibc++)에서 아직 완전히 지원되지 않을 수 있어 빌드 실패가 발생할 수 있습니다.
   - 권장: `std::cout`로 대체하거나 `fmtlib` 또는 자체 포맷 함수로 폴백을 추가하세요.

2. `elseif` 토큰화
   - 현재 `elseif`는 하나의 토큰으로만 처리됩니다. `else if`(두 단어) 형태는 허용되지 않습니다.

3. 할당 대상(assign target)과 임시
   - `a[b] = x` 또는 `obj.field = x` 처리 시, 코드 생성기는 임시 객체를 만들어 `wast::set_index`/`wast::set_field`를 호출합니다.
   - 만약 좌변이 임시값(예: 함수 호출 결과)라면 이 변경은 원본에 반영되지 않거나 의미가 없어질 수 있습니다.
   - 권장: 허용되는 lvalue의 범위를 좁혀 컴파일 단계에서 에러로 처리하거나, 더 정교한 대상 평가 전략을 구현하세요.

4. 문자열 보간 제약
   - 보간은 `{...}` 형태이며 현재는 "단순 변수(식별자)"나 "숫자 리터럴"만 허용됩니다. 복잡한 표현식은 지원되지 않습니다.

5. 숫자 리터럴 문법
   - 언더스코어(`_`)는 숫자 구분기호로 허용(무시)됩니다. 지수 표기(exp) 및 16진수 등은 제한적입니다.

6. 에러 메시지
   - 현재 에러 메시지는 파일 위치(line:col)와 근처 토큰 텍스트를 출력하지만, 소스 라인 스니펫과 캐럿 표시가 없어 디버깅이 다소 불편할 수 있습니다.

---

## 개발/기여자 안내

- 로컬 개발:
  1. 트랜스파일러 빌드: `g++ -std=c++23 -O3 wast2.cpp -o wastc`
  2. 예제 트랜스파일: `./wastc examples/test.wast out.cpp`
  3. 결과 컴파일: `g++ -std=c++23 -O2 out.cpp -o out`
  4. 실행: `./out`

- 디버깅 팁:
  - `wastc`는 런타임 예외를 통해 대부분의 실행 오류를 보고합니다. 트랜스파일된 C++에서 예외 메시지에 런타임 에러 원인이 포함되어 있습니다.
  - 트랜스파일러의 에러(구문/타입 관련)는 `CompileError`로 포맷된 위치 정보와 함께 발생합니다.

- 기여:
  - 포팅성(printf/print 대체), lvalue 규칙 강화, `else if` 처리 등부터 시작하는 것이 좋습니다.
  - 단위 테스트 스위트를 추가해 코드 생성 후 C++ 컴파일과 간단한 실행을 자동화하면 안정성이 크게 향상됩니다.

---

## 예제 테스트 케이스 (권장)

1. 기본 산술 및 출력
```wast
fn main() (
    print(1 + 2)
)
```

2. 배열/인덱스/반복
```wast
fn main() (
    local a = [1,2,3]
    for x in a ( print(x) )
)
```

3. 라이브러리 임베딩
```wast
library inc(a: i64) -> i64 """
    return a + 1;
"""
fn main() (
    print(inc(5))
)
```

---

## 자주 묻는 질문일거 같은거 (FAQ)

Q. 생성된 C++는 어느 범용성으로 빌드되나요?
- 생성된 C++는 C++23 문법을 사용합니다(예: `std::print` 등). 따라서 표준 라이브러리/컴파일러 버전에 따라 빌드가 달라질 수 있습니다. 호환성을 위해 `std::cout` 폴백을 적용할 것을 권장합니다.

Q. 라이브러리 블록 안에서 안전하지 않은 C++ 코드를 삽입하면?
- 트랜스파일러는 라이브러리 코드 블록을 그대로 생성된 C++에 삽입합니다. 따라서 삽입한 C++의 안정성, 메모리 안전성, 예외 처리는 전적으로 삽입자 책임입니다.

---

## 연락 / 추가 도움

필요하시면 다음 작업을 바로 도와드릴 수 있습니다:
- `std::print` → `std::cout` 대체 패치 제공
- 할당 대상(lvalue) 검증 패치
- `else if`(두 토큰) 허용 확장
- 에러 메시지 개선(라인 스니펫 추가)

원하시는 개선 항목을 알려주시면, 수정된 코드(패치 또는 전체 파일)를 제공하겠습니다.
