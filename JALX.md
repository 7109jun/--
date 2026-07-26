# JALX 가이드
### JALX: Jun’s Articulated Layered eXpression

이 문서는 **1. 배우기 → 2. 사용해보기 → 3. 다른 포맷과 비교 → 4. 예제** 순서로 구성되어 있다.
문법 세부 규칙은 `JALX_SPEC.md`를 참고. 여기서는 실전 위주로 다룬다.

---

## 1. 배우기

JALX를 배우는 데 필요한 건 딱 6개 기호뿐이다.

| 기호 | 의미 |
|---|---|
| `[ ]` | 객체 (key: value 묶음) |
| `( )` | 리스트 |
| `" "` / `""" """` | 문자열 / 여러 줄 문자열 |
| `*( )` | 속성 (attribute) |
| `(type)` | 값 앞에 붙는 타입 힌트 |
| `@type`, `@root` | 스키마 선언 |

### 1.1 가장 작은 문서
```
name: "홍길동"
age: 27
```
콤마도, 들여쓰기도 필요 없다. 줄바꿈은 그냥 가독성용이고 파싱에 아무 영향 없다. 한 줄로 써도 결과는 같다:
```
name: "홍길동" age: 27
```

### 1.2 객체 안에 객체
```
address: [city: "서울", zip: "04524"]
```

### 1.3 리스트
```
tags: ("dev", "game", "engine")
items: ([id: 1, name: "a"], [id: 2, name: "b"])
```

### 1.4 타입 힌트 — 스키마 없이 바로 검증
```
(int)age: 27
```
`jalx.validate()`를 부르면 `age`가 진짜 정수인지 그 자리에서 확인한다. `@type`도 `@root`도 필요 없다.

### 1.5 속성 (XML의 attribute 대체)
```
profile*(lang="ko", verified=true): "자기소개"
```
`*( )` 안에 `key=value`를 콤마로 나열. 값 자체(`"자기소개"`)와 속성은 별개다.

### 1.6 텍스트 + 자식 (혼합 컨텐츠)
```
profile*(lang="ko"): "자기소개", [tags: ("dev", "gamer")]
```
텍스트 값 뒤에 **콤마 + `[...]`**가 오면 그게 자식이다. 콤마 없이 `[...]`만 오면 자식으로 안 쳐준다 — 헷갈리지 말라고 일부러 그렇게 정함.

### 1.7 스키마 선언
```
@type Person: [name: str, age: int, bio?: str]
@root: Person
```
`?`가 붙은 필드는 선택 사항. `jalx.validate()`가 최상위 데이터를 `Person` 스키마 기준으로 검사.

여기까지가 전부다. 이제 실제로 써보자.

---

## 2. 사용해보기

### 2.1 설치 없이 바로 쓰기
`jalx.py` 하나만 있으면 된다. 외부 의존성 없음 (표준 라이브러리 `re`만 사용).

```python
import jalx

doc = jalx.loads('''
@type Person: [name: str, age: int]
@root: Person

name: "홍길동"
age: 27
''')

print(doc["name"])      # 홍길동
print(doc["age"])       # 27
print(jalx.validate(doc))  # True
```

### 2.2 파이썬 객체 -> JALX 문자열
```python
data = {"name": "김철수", "scores": [90, 85, 100]}
text = jalx.dumps(data)
print(text)
```

### 2.3 파일 읽기/쓰기
```python
doc = jalx.load("config.jalx")
jalx.dump(doc, "config.jalx")
```

### 2.4 잘못된 데이터 잡아내기
```python
bad = jalx.loads('(int)age: "스물일곱"')
try:
    jalx.validate(bad)
except jalx.JALXError as e:
    print(e)   # $.age: expected int, got '스물일곱'
```

### 2.5 XML 스타일 혼합 컨텐츠 다루기
```python
doc = jalx.loads('note*(author="kim"): "회의록 내용", [tags: ("긴급", "검토필요")]')
node = doc["note"]
print(node.attrs)       # {'author': 'kim'}
print(node.text)        # 회의록 내용
print(node["tags"])     # ['긴급', '검토필요']
```

---

## 3. 다른 데이터 포맷과 비교

같은 데이터를 4가지 포맷으로 표현:

**JSON**
```json
{
  "name": "홍길동",
  "age": 27,
  "tags": ["dev", "gamer"]
}
```

**YAML**
```yaml
name: 홍길동
age: 27
tags:
  - dev
  - gamer
```

**XML**
```xml
<person lang="ko">
  <name>홍길동</name>
  <age>27</age>
  <tags><tag>dev</tag><tag>gamer</tag></tags>
</person>
```

**JALX**
```
person*(lang="ko"): [name: "홍길동", age: 27, tags: ("dev", "gamer")]
```

| 항목 | JSON | YAML | XML | JALX |
|---|---|---|---|---|
| 사람이 타이핑하기 | 보통 (콤마/괄호 많음) | 쉬움 (기호 적음) | 번거로움 (닫는 태그) | JSON과 비슷하지만 `{}`가 없어 살짝 더 간결 |
| 들여쓰기 의존 | 없음 | **있음** (버그 유발 원인 1위) | 없음 | 없음 (JSON처럼 괄호가 구조를 결정) |
| 속성(attribute) | 없음 (필드로 흉내) | 없음 | **있음** | **있음** (`*( )`) |
| 텍스트+자식 혼합 | 불가능 | 불가능 | **가능** | **가능** |
| 주석 | 불가능 | 가능 (`#`) | 가능 (`<!-- -->`) | 가능 (`#`) |
| 스키마 내장 | 없음 (별도 JSON Schema) | 없음 | 있음 (XSD, 별도 파일) | **있음** (`@type`, 같은 파일 안에) |
| 여러 줄 문자열 | 불가능 (`\n` 이스케이프만) | 가능 (`|`, `>`) | 가능 (CDATA) | 가능 (`"""`) |

**요약**: JALX는 "JSON처럼 파싱이 명확하면서, XML처럼 속성/혼합 컨텐츠를 표현하고, 스키마까지 같은 파일에 넣고 싶을 때" 쓰기 좋다. 반대로 사람이 손으로 대량의 설정 파일을 편하게 쳐야 한다면(예: 서버 설정) YAML이 여전히 타이핑량은 더 적다.

---

## 4. 예제

### 4.1 서버 설정 파일
```
@type ServerConfig: [
  host: str
  port: int
  debug?: bool
  allowed_ips: list<str>
]
@root: ServerConfig

host: "0.0.0.0"
port: 8080
debug: true
allowed_ips: ("127.0.0.1", "192.168.0.1")
```

### 4.2 블로그 글 (혼합 컨텐츠 활용)
```
@type Post: [title: str, author: str, body: str]
@root: Post

title: "JALX 소개"
author: "룰루랄라"
body*(published="2026-07-25", lang="ko"): """
    JALX는 JSON, YAML, XML의 장점을 합친 포맷입니다.
    """, [
  comments: (
    [user: "a", text: "좋은 글이네요"]
    [user: "b", text: "감사합니다"]
  )
]
```

### 4.3 게임 저장 데이터 (중첩 구조 + 타입 힌트)
```
@type Player: [name: str, level: int, hp: int, inventory: list<str>]
@root: Player

name: "용사"
(int)level: 12
(int)hp: 87
inventory: ("검", "방패", "회복 물약")
position: [x: 120.5, y: 340.2]
```

### 4.4 다국어 리소스 (속성 + 리스트)
```
greetings: (
  greeting*(lang="ko"): "안녕하세요"
  greeting*(lang="en"): "Hello"
  greeting*(lang="ja"): "こんにちは"
)
```
같은 키(`greeting`)가 여러 개 필요하면 객체(`[]`)가 아니라 리스트(`()`) 안에 넣는다 — 객체는 키가 유일해야 하기 때문.
# JALX Specification v2.0

**JALX** (JSON + YAML + XML) — JSON의 명확한 타입, YAML에서 가져온 담백한 텍스트 흐름, XML의 속성/태그 표현력을 하나로 합친 데이터 포맷.

> v2.0에서 들여쓰기 의존형 문법을 폐기하고, 괄호 기반의 델리미터 문법으로 전면 개정했다.
> 들여쓰기/공백은 순수히 가독성을 위한 장식일 뿐 파싱 규칙에 영향을 주지 않는다 (문서 구조의 ~15%만 담당).
> 구조는 전적으로 `[]`, `()`, `:`, `,` 같은 명시적 기호가 결정한다.

파일 확장자: `.jalx`  ·  인코딩: UTF-8

---

## 1. 기본 원칙

| 기호 | 역할 |
|---|---|
| `[ ]` | **객체** (key: value 묶음) — JSON의 `{}` 대체 |
| `( )` | **리스트/배열** |
| `" "` | 문자열. 여러 줄은 `""" ... """` |
| `* attr=value` | **속성(attribute)** 표시자 — XML의 attribute 역할 |
| `(type) key` | 키 앞에 붙는 **타입 접두** — 값의 타입을 짧게 명시 |
| `#` | 라인 주석 |
| `@type`, `@root` | 스키마 선언 |

들여쓰기와 줄바꿈은 전부 공백(whitespace)으로 취급되어 무시된다. 한 줄에 다 써도 되고, 예쁘게 들여써도 되고 결과는 동일하다.

---

## 2. 스칼라

| 타입 | 예시 |
|---|---|
| 문자열 | `"hello"` |
| 멀티라인 문자열 | `"""여러줄\n텍스트"""` |
| 정수/실수 | `42`, `-3.14`, `1.5e10` |
| 불리언 | `true`, `false` |
| null | `null` |

이스케이프: `\"`, `\\`, `\n`, `\t`, `\uXXXX`

---

## 3. 키(key) 문법

```
key      := ( '(' TYPE ')' )? IDENT ( '?' )? ( '*' '(' IDENT '=' value (',' IDENT '=' value)* ')' )?
```

- `(TYPE)` — 접두 타입 힌트. 예: `(int)age: 27`. **`@type`/`@root` 스키마 선언이 전혀 없어도 그 자체로 검증 대상이 된다** — `jalx.validate()`는 문서 전체를 순회하며 모든 `(type)` 힌트를 항상 검사한다.
- `?` — 스키마 필드 선언에서 optional 필드 표시. 예: `bio?: str`
- `*( ... )` — 속성을 하나의 그룹으로 묶어서 표기. 여러 속성은 콤마로 구분. 예: `profile*(lang="ko", verified=true): ...`
  (v1 초안의 `*attr=value*attr2=value2` 체이닝 방식은 가독성 문제로 폐기됨)

---

## 4. 값(value) 문법

### 4.1 객체 `[ ]`
```
[name: "홍길동", age: 27]
```

### 4.2 리스트 `( )`
```
(1, 2, 3)
("dev", "gamer")
(
  [id: 1, name: "first"]
  [id: 2, name: "second"]
)
```

### 4.3 속성 + 텍스트 + 자식 (혼합 컨텐츠, XML 대체 문법)

```
profile*(lang="ko", verified=true): "자기소개 텍스트", [tags: ("dev", "gamer")]
```

- `key*(attrs): value` — `value`는 텍스트/스칼라 컨텐츠
- 텍스트 값 바로 뒤에 **콤마 + `[...]`** 가 오면 그것이 **자식 노드**로 인식된다.
  콤마 없이 `[...]`가 오면 자식으로 인식되지 않는다 — 텍스트와 자식의 경계를 명확히 하기 위한 규칙.
- 자식은 `*(...)` 속성이나 `(type)` 접두가 있는 "태그형" 키에서만 의미가 있다. 일반 키에는 적용되지 않는다.
- 텍스트와 자식을 **동시에** 가질 수 있다 (mixed content)

### 4.4 타입 접두 (짧은 인라인 타입 힌트 = 인라인 스키마)
```
(int)age: 27
(str)name: "홍길동"
```
스키마 선언 없이도 값 자체에 타입을 명시하면 그 자체로 검증 대상이 된다 (별도 `@type`/`@root` 불필요).

### 4.5 멀티라인 문자열의 공통 들여쓰기 제거 (dedent)
```
bio: """
    여러 줄
    텍스트
    """
```
`"""` 여는/닫는 줄 바로 다음·이전의 개행 하나씩은 구분자로 간주되어 제거되고, 그 사이 모든 줄에 공통으로 걸린 들여쓰기는 자동으로 제거된다. 즉 위 예시의 실제 문자열 값은 `"여러 줄\n텍스트"` 이지 앞에 공백이 붙지 않는다.

### 4.6 구분자(콤마) 규칙
객체 `[]`, 리스트 `()`, 최상위 문서, `@type` 필드 선언 — **모든 곳에서 콤마는 선택 사항**이다 (가독성을 위해 써도 되고, 줄바꿈만으로 항목을 구분해도 된다). 어떤 곳은 필수고 어떤 곳은 선택인 v1의 불일치가 v2에서 완전히 통일됨.



---

## 5. 스키마 선언

### 5.1 타입 정의 — `@type Name: [ field: type, ... ]`
```
@type Address: [city: str, zip: str]

@type Person: [
  name: str,
  age: int,
  bio?: str,
  address?: Address
]
```
지원 타입: `str`, `int`, `float`, `bool`, `null`, `any`, `list<T>`, `map<T>`, 다른 `@type` 이름.

### 5.2 루트 타입 — `@root: Name`
```
@root: Person
```

---

## 6. 전체 예시

```
@type Address: [city: str, zip: str]
@type Person: [name: str, age: int, bio?: str, address?: Address]
@root: Person

name: "홍길동"
age: 27
profile*(lang="ko", verified=true): """
    자기소개 텍스트입니다.
    여러 줄 가능.
    """, [tags: ("dev", "gamer")]
address: [city: "서울", zip: "04524"]
items: (
  [id: 1, name: "first"]
  [id: 2, name: "second"]
)
```

---

## 7. Python API (jalx 모듈)

```python
import jalx

data = jalx.loads(text)        # 문자열 -> 파이썬 객체 (JALXDocument)
text = jalx.dumps(data)        # 파이썬 객체 -> JALX 문자열
data = jalx.load("config.jalx")
jalx.dump(data, "config.jalx")
jalx.validate(data)            # @type / @root 기준 검증
```

`JALXNode`가 태그/속성/타입힌트/텍스트/자식을 가진 노드를 표현하며, dict처럼 `[]`, `.items()`, `.keys()`를 사용할 수 있다.
"""
JALX (JSON + YAML + XML) reference implementation -- v2
==========================================================
Bracket-delimited grammar (indentation/whitespace carry no structural
meaning; `[]`/`()`/`:`/`,`/`*`/`(type)` decide structure entirely).

Step-by-step procedural design:
  1. Tokenizer     (strings incl. multiline, numbers, punctuation, comments)
  2. Value parser   ([...] objects, (...) lists, scalars)
  3. Key parser      ((type) prefix, '?' optional marker, '*attr=value' attrs)
  4. Schema handling  (@type / @root directives, TYPEEXPR incl. list<T>/map<T>)
  5. Validator         (jalx.validate)
  6. Serializer         (jalx.dumps)
  7. Public API           (loads/dumps/load/dump/validate)
"""

import re


# ---------------------------------------------------------------------------
# 0. Errors & core node type
# ---------------------------------------------------------------------------

class JALXError(Exception):
    def __init__(self, message, line=None):
        self.line = line
        if line is not None:
            message = f"line {line}: {message}"
        super().__init__(message)


class JALXNode:
    """
    Represents a node with an optional attribute set (*attr=value), an
    optional declared type ((type)key), an optional text/scalar value,
    and optional children (an object dict, produced by a trailing [...]).
    """

    __slots__ = ("tag", "decl_type", "attrs", "text", "children")

    def __init__(self, tag=None, decl_type=None, attrs=None, text=None, children=None):
        self.tag = tag
        self.decl_type = decl_type
        self.attrs = attrs if attrs is not None else {}
        self.text = text
        self.children = children if children is not None else {}

    def __getitem__(self, key):
        return self.children[key]

    def __setitem__(self, key, value):
        self.children[key] = value

    def __contains__(self, key):
        return key in self.children

    def __iter__(self):
        return iter(self.children)

    def keys(self):
        return self.children.keys()

    def items(self):
        return self.children.items()

    def values(self):
        return self.children.values()

    def get(self, key, default=None):
        return self.children.get(key, default)

    def __repr__(self):
        ch = list(self.children.keys()) if isinstance(self.children, dict) else self.children
        return (f"JALXNode(tag={self.tag!r}, decl_type={self.decl_type!r}, "
                f"attrs={self.attrs!r}, text={self.text!r}, children={ch!r})")

    def __eq__(self, other):
        if not isinstance(other, JALXNode):
            return NotImplemented
        return (self.tag == other.tag and self.decl_type == other.decl_type and
                self.attrs == other.attrs and self.text == other.text and
                self.children == other.children)


# ---------------------------------------------------------------------------
# 1. Tokenizer
# ---------------------------------------------------------------------------
# Token kinds: LBRACK/RBRACK '[' ']', LPAREN/RPAREN '(' ')', LT/GT '<' '>',
#              COLON ':', COMMA ',', STAR '*', QMARK '?', AT '@',
#              STRING, NUMBER, BOOL, NULL, IDENT, EOF

_PUNCT = {
    "[": "LBRACK", "]": "RBRACK",
    "(": "LPAREN", ")": "RPAREN",
    "<": "LT", ">": "GT",
    ":": "COLON", ",": "COMMA",
    "*": "STAR", "?": "QMARK", "@": "AT", "=": "EQ",
}

_IDENT_RE = re.compile(r"[A-Za-z0-9_\-\uac00-\ud7a3]+")
_NUMBER_RE = re.compile(r"-?\d+(\.\d+)?([eE][+-]?\d+)?")


def _dedent_multiline(body):
    """Strips a leading/trailing newline (from the triple-quote + newline
    convention) and removes the common leading whitespace shared by every non-blank
    line, so indenting a triple-quoted block for readability doesn't leak into the
    string's actual content."""
    s = body
    if s.startswith("\n"):
        s = s[1:]
    if s.endswith("\n"):
        s = s[:-1]
    lines = s.split("\n")
    non_empty = [ln for ln in lines if ln.strip() != ""]
    if not non_empty:
        return s
    min_indent = min(len(ln) - len(ln.lstrip(" ")) for ln in non_empty)
    if min_indent == 0:
        return s
    dedented = [ln[min_indent:] if len(ln) >= min_indent else ln.lstrip(" ") for ln in lines]
    return "\n".join(dedented)


class _Token:
    __slots__ = ("kind", "value", "line")

    def __init__(self, kind, value, line):
        self.kind = kind
        self.value = value
        self.line = line

    def __repr__(self):
        return f"_Token({self.kind}, {self.value!r}, L{self.line})"


def tokenize(text):
    tokens = []
    i = 0
    n = len(text)
    line = 1

    while i < n:
        c = text[i]

        if c == "\n":
            line += 1
            i += 1
            continue
        if c in " \t\r":
            i += 1
            continue
        if c == "#":
            while i < n and text[i] != "\n":
                i += 1
            continue

        if text.startswith('"""', i):
            start_line = line
            body_start = i + 3
            close = text.find('"""', body_start)
            if close == -1:
                raise JALXError("unterminated triple-quoted string", start_line)
            body = text[body_start:close]
            line += body.count("\n")
            tokens.append(_Token("STRING", _dedent_multiline(body), start_line))
            i = close + 3
            continue

        if c == '"':
            start_line = line
            j = i + 1
            out = []
            while j < n and text[j] != '"':
                if text[j] == "\\":
                    j += 1
                    if j >= n:
                        raise JALXError("bad escape in string", start_line)
                    e = text[j]
                    mapping = {"n": "\n", "t": "\t", '"': '"', "\\": "\\", "r": "\r"}
                    if e in mapping:
                        out.append(mapping[e])
                        j += 1
                    elif e == "u":
                        hex_digits = text[j + 1:j + 5]
                        out.append(chr(int(hex_digits, 16)))
                        j += 5
                    else:
                        out.append(e)
                        j += 1
                else:
                    if text[j] == "\n":
                        line += 1
                    out.append(text[j])
                    j += 1
            if j >= n:
                raise JALXError("unterminated string", start_line)
            tokens.append(_Token("STRING", "".join(out), start_line))
            i = j + 1
            continue

        if c in _PUNCT:
            tokens.append(_Token(_PUNCT[c], c, line))
            i += 1
            continue

        m = _NUMBER_RE.match(text, i)
        if m and m.group() not in ("", "-"):
            tokens.append(_Token("NUMBER", m.group(), line))
            i = m.end()
            continue

        m = _IDENT_RE.match(text, i)
        if m:
            word = m.group()
            if word == "true":
                tokens.append(_Token("BOOL", True, line))
            elif word == "false":
                tokens.append(_Token("BOOL", False, line))
            elif word == "null":
                tokens.append(_Token("NULL", None, line))
            else:
                tokens.append(_Token("IDENT", word, line))
            i = m.end()
            continue

        raise JALXError(f"unexpected character {c!r}", line)

    tokens.append(_Token("EOF", None, line))
    return tokens


# ---------------------------------------------------------------------------
# 2-4. Parser (values, keys, objects, lists, schema directives)
# ---------------------------------------------------------------------------

class _Parser:
    def __init__(self, tokens):
        self.toks = tokens
        self.pos = 0

    def peek(self):
        return self.toks[self.pos]

    def advance(self):
        t = self.toks[self.pos]
        self.pos += 1
        return t

    def expect(self, kind):
        t = self.peek()
        if t.kind != kind:
            raise JALXError(f"expected {kind}, got {t.kind} ({t.value!r})", t.line)
        return self.advance()

    def at(self, kind):
        return self.peek().kind == kind

    # -- top level ------------------------------------------------------

    def parse_document(self):
        data = {}
        schemas = {}
        root_type = None

        while not self.at("EOF"):
            if self.at("AT"):
                self.advance()
                ident = self.expect("IDENT")
                if ident.value == "type":
                    name = self.expect("IDENT").value
                    self.expect("COLON")
                    fields = self.parse_type_object()
                    schemas[name] = fields
                elif ident.value == "root":
                    self.expect("COLON")
                    root_type = self.expect("IDENT").value
                else:
                    raise JALXError(f"unknown directive @{ident.value}", ident.line)
                continue

            key, value = self.parse_pair()
            data[key] = value

        return data, schemas, root_type

    # -- key: (type)? IDENT '?'? ('*' IDENT '=' value)* ------------------

    def parse_key(self):
        decl_type = None
        if self.at("LPAREN"):
            self.advance()
            decl_type = self.parse_typeexpr()
            self.expect("RPAREN")

        name_tok = self.expect("IDENT")
        name = name_tok.value

        optional = False
        if self.at("QMARK"):
            self.advance()
            optional = True

        attrs = {}
        if self.at("STAR"):
            self.advance()
            self.expect("LPAREN")
            if not self.at("RPAREN"):
                while True:
                    attr_name = self.expect("IDENT").value
                    self.expect("EQ")
                    attrs[attr_name] = self.parse_value()
                    if self.at("COMMA"):
                        self.advance()
                        continue
                    break
            self.expect("RPAREN")

        return name, decl_type, optional, attrs

    def parse_pair(self):
        name, decl_type, optional, attrs = self.parse_key()
        self.expect("COLON")
        value = self.parse_value()

        # a trailing children-object is only recognised for tagged nodes
        # (decl_type or attrs present), and only when explicitly introduced
        # by a comma -- this removes the ambiguity of a bare value being
        # immediately followed by an unrelated bracket.
        children = None
        if (decl_type is not None or attrs) and self.at("COMMA") and \
                self.pos + 1 < len(self.toks) and self.toks[self.pos + 1].kind == "LBRACK":
            self.advance()
            children = self.parse_object()

        if decl_type is not None or attrs or children is not None:
            node = JALXNode(tag=name, decl_type=decl_type, attrs=attrs,
                             text=value, children=children if children is not None else {})
            return name, node

        return name, value

    # -- values -----------------------------------------------------------


    def parse_value(self):
        t = self.peek()
        if t.kind == "STRING":
            self.advance()
            return t.value
        if t.kind == "NUMBER":
            self.advance()
            s = t.value
            if "." in s or "e" in s or "E" in s:
                return float(s)
            return int(s)
        if t.kind == "BOOL":
            self.advance()
            return t.value
        if t.kind == "NULL":
            self.advance()
            return None
        if t.kind == "LBRACK":
            return self.parse_object()
        if t.kind == "LPAREN":
            return self.parse_list()
        raise JALXError(f"expected a value, got {t.kind} ({t.value!r})", t.line)

    def parse_object(self):
        self.expect("LBRACK")
        obj = {}
        while not self.at("RBRACK"):
            key, value = self.parse_pair()
            obj[key] = value
            if self.at("COMMA"):
                self.advance()
        self.expect("RBRACK")
        return obj

    def parse_list(self):
        self.expect("LPAREN")
        items = []
        while not self.at("RPAREN"):
            if self.at("IDENT"):
                # a bare 'name: value' (optionally *(attrs) / (type)-prefixed)
                # appearing directly as a list item -- lets a list hold
                # tagged/attributed nodes without needing to wrap them in [].
                name, val = self.parse_pair()
                items.append(val if isinstance(val, JALXNode) else {name: val})
            else:
                items.append(self.parse_value())
            if self.at("COMMA"):
                self.advance()
        self.expect("RPAREN")
        return items

    # -- type expressions: IDENT ('<' typeexpr '>')? -----------------------

    def parse_typeexpr(self):
        name = self.expect("IDENT").value
        if self.at("LT"):
            self.advance()
            inner = self.parse_typeexpr()
            self.expect("GT")
            return f"{name}<{inner}>"
        return name

    def parse_type_object(self):
        self.expect("LBRACK")
        fields = {}
        while not self.at("RBRACK"):
            fname = self.expect("IDENT").value
            optional = False
            if self.at("QMARK"):
                self.advance()
                optional = True
            self.expect("COLON")
            ftype = self.parse_typeexpr()
            fields[fname] = {"type": ftype, "optional": optional}
            if self.at("COMMA"):
                self.advance()
        self.expect("RBRACK")
        return fields


# ---------------------------------------------------------------------------
# 5. Document wrapper + schema validator
# ---------------------------------------------------------------------------

class JALXDocument(dict):
    """A parsed JALX document: dict of top-level data, plus `.schemas`
    (dict of @type field-defs) and `.root` (the @root type name, or None)."""

    def __init__(self, data, schemas=None, root=None):
        super().__init__(data)
        self.schemas = schemas or {}
        self.root = root


_LIST_TYPE_RE = re.compile(r'^list<(.+)>$')
_MAP_TYPE_RE = re.compile(r'^map<(.+)>$')
_PRIMITIVES = {"str", "int", "float", "bool", "null", "any"}


def _type_ok_primitive(value, type_name):
    if type_name == "any":
        return True
    if type_name == "str":
        return isinstance(value, str)
    if type_name == "int":
        return isinstance(value, int) and not isinstance(value, bool)
    if type_name == "float":
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    if type_name == "bool":
        return isinstance(value, bool)
    if type_name == "null":
        return value is None
    return False


def _unwrap(value):
    """Node -> the dict used for field lookups; everything else passes through."""
    return value.children if isinstance(value, JALXNode) else value


def _check_type(value, type_str, schemas, path):
    type_str = type_str.strip()

    m = _LIST_TYPE_RE.match(type_str)
    if m:
        inner = m.group(1)
        if not isinstance(value, list):
            raise JALXError(f"{path}: expected list<{inner}>, got {type(value).__name__}")
        for i, item in enumerate(value):
            _check_type(item, inner, schemas, f"{path}({i})")
        return

    m = _MAP_TYPE_RE.match(type_str)
    if m:
        inner = m.group(1)
        mapping = _unwrap(value)
        if not isinstance(mapping, dict):
            raise JALXError(f"{path}: expected map<{inner}>, got {type(value).__name__}")
        for k, v in mapping.items():
            _check_type(v, inner, schemas, f"{path}.{k}")
        return

    if type_str in _PRIMITIVES:
        scalar = value.text if isinstance(value, JALXNode) else value
        if not _type_ok_primitive(scalar, type_str):
            raise JALXError(f"{path}: expected {type_str}, got {scalar!r}")
        return

    if type_str in schemas:
        fields = schemas[type_str]
        mapping = _unwrap(value)
        if not isinstance(mapping, dict):
            raise JALXError(f"{path}: expected object of type {type_str}, got {type(value).__name__}")
        for fname, finfo in fields.items():
            if fname not in mapping:
                if finfo["optional"]:
                    continue
                raise JALXError(f"{path}.{fname}: missing required field of type {finfo['type']}")
            _check_type(mapping[fname], finfo["type"], schemas, f"{path}.{fname}")
        return

    raise JALXError(f"{path}: unknown type {type_str!r}")


def _walk_decl_types(value, schemas, path):
    """Recursively verifies any inline (type)key hints against their actual value."""
    if isinstance(value, JALXNode):
        if value.decl_type is not None:
            _check_type(value.text, value.decl_type, schemas, path)
        for k, v in (value.children.items() if isinstance(value.children, dict) else []):
            _walk_decl_types(v, schemas, f"{path}.{k}")
    elif isinstance(value, dict):
        for k, v in value.items():
            _walk_decl_types(v, schemas, f"{path}.{k}")
    elif isinstance(value, list):
        for i, v in enumerate(value):
            _walk_decl_types(v, schemas, f"{path}({i})")


def validate(data, schemas=None, root_type=None):
    """
    Validates `data` against `schemas` using `root_type` as the expected
    top-level type, PLUS checks every inline (type)key hint found anywhere
    in the tree. Raises JALXError on failure, returns True on success.
    """
    if isinstance(data, JALXDocument):
        schemas = schemas if schemas is not None else data.schemas
        root_type = root_type if root_type is not None else data.root
    schemas = schemas or {}

    if root_type:
        if root_type not in schemas:
            raise JALXError(f"unknown root type {root_type!r}: no matching @type declaration")
        _check_type(data, root_type, schemas, "$")

    _walk_decl_types(data, schemas, "$")
    return True


# ---------------------------------------------------------------------------
# 6. Serializer (Python object -> JALX text)
# ---------------------------------------------------------------------------

def _dump_string(s):
    if "\n" in s:
        body = s.replace("\\", "\\\\")
        return f'"""\n{body}\n"""'
    escaped = s.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def _dump_value(value):
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return str(value)
    if isinstance(value, str):
        return _dump_string(value)
    if isinstance(value, dict):
        return _dump_object(value)
    if isinstance(value, list):
        return _dump_list(value)
    if isinstance(value, JALXNode):
        return _dump_pair(value.tag or "item", value)
    raise JALXError(f"cannot serialize value of type {type(value).__name__}")


def _dump_pair(key, value):
    if isinstance(value, JALXNode):
        keystr = f"({value.decl_type}){key}" if value.decl_type else key
        if value.attrs:
            attrs_body = ", ".join(f"{k}={_dump_value(v)}" for k, v in value.attrs.items())
            keystr += f"*({attrs_body})"
        text_str = _dump_value(value.text) if value.text is not None else "null"
        s = f"{keystr}: {text_str}"
        if isinstance(value.children, dict) and value.children:
            s += ", " + _dump_object(value.children)
        return s
    return f"{key}: {_dump_value(value)}"


def _dump_object(obj):
    parts = [_dump_pair(k, v) for k, v in obj.items()]
    return "[" + ", ".join(parts) + "]"


def _dump_list(items):
    parts = []
    for item in items:
        if isinstance(item, dict):
            parts.append(_dump_object(item))
        elif isinstance(item, JALXNode):
            parts.append(_dump_pair(item.tag or "item", item))
        else:
            parts.append(_dump_value(item))
    return "(" + ", ".join(parts) + ")"


def dumps(data, schemas=None, root_type=None):
    """Serializes a Python dict / JALXDocument (optionally with @type / @root
    declarations) into JALX v2 text."""
    if isinstance(data, JALXDocument):
        schemas = schemas if schemas is not None else data.schemas
        root_type = root_type if root_type is not None else data.root

    lines = []
    if schemas:
        for name, fields in schemas.items():
            parts = []
            for fname, finfo in fields.items():
                q = "?" if finfo["optional"] else ""
                parts.append(f"{fname}{q}: {finfo['type']}")
            lines.append(f"@type {name}: [" + ", ".join(parts) + "]")
        lines.append("")

    if root_type:
        lines.append(f"@root: {root_type}")
        lines.append("")

    if not isinstance(data, dict):
        raise JALXError("top-level data to dumps() must be a dict / JALXDocument")

    for k, v in data.items():
        lines.append(_dump_pair(k, v))

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# 7. Public API
# ---------------------------------------------------------------------------

def loads(text):
    """Parses a JALX string, returning a JALXDocument (dict subclass with
    `.schemas` and `.root` attributes)."""
    tokens = tokenize(text)
    parser = _Parser(tokens)
    data, schemas, root = parser.parse_document()
    return JALXDocument(data, schemas, root)


def load(path):
    with open(path, "r", encoding="utf-8") as f:
        return loads(f.read())


def dump(data, path, schemas=None, root_type=None):
    text = dumps(data, schemas=schemas, root_type=root_type)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    return text
    #!/usr/bin/env python3
"""
jalx_convert.py -- JSON <-> JALX bidirectional converter, and a JALX syntax checker.

Usage:
  python3 jalx_convert.py json2jalx  <input.json> [output.jalx]
  python3 jalx_convert.py jalx2json  <input.jalx> [output.json]
  python3 jalx_convert.py check      <input.jalx> [--validate]

Examples:
  python3 jalx_convert.py json2jalx config.json config.jalx
  python3 jalx_convert.py jalx2json config.jalx config.json
  python3 jalx_convert.py check config.jalx --validate
  echo '{"a": 1}' | python3 jalx_convert.py json2jalx -
"""

import sys
import json
import argparse

import jalx


# ---------------------------------------------------------------------------
# JSON -> JALX
# ---------------------------------------------------------------------------
# A plain json.loads() result is already exactly the shape jalx.dumps()
# expects (nested dict / list / str / int / float / bool / None) -- JALX's
# object/list model is a strict superset of JSON's, so no lossy mapping is
# needed in this direction.

def json_to_jalx(json_text):
    data = json.loads(json_text)
    if not isinstance(data, dict):
        # JALX documents are key: value at the top level; wrap non-dict
        # JSON roots (arrays, bare scalars) under a single synthetic key
        # so they still round-trip losslessly.
        data = {"value": data}
    return jalx.dumps(data)


# ---------------------------------------------------------------------------
# JALX -> JSON
# ---------------------------------------------------------------------------
# JALXNode (attrs / decl_type / text / children) has no direct JSON
# equivalent, so it is flattened into a plain JSON object using reserved
# keys prefixed with '@' (a common convention borrowed from JSON<->XML
# converters): "@attrs", "@type", "@text", plus the node's own children
# merged in directly.

def _jalx_value_to_json(value):
    if isinstance(value, jalx.JALXNode):
        obj = {}
        if value.decl_type is not None:
            obj["@type"] = value.decl_type
        if value.attrs:
            obj["@attrs"] = {k: _jalx_value_to_json(v) for k, v in value.attrs.items()}
        if value.text is not None:
            obj["@text"] = _jalx_value_to_json(value.text)
        if isinstance(value.children, dict):
            for k, v in value.children.items():
                obj[k] = _jalx_value_to_json(v)
        elif isinstance(value.children, list):
            obj["@children"] = [_jalx_value_to_json(v) for v in value.children]
        return obj
    if isinstance(value, dict):
        return {k: _jalx_value_to_json(v) for k, v in value.items()}
    if isinstance(value, list):
        return [_jalx_value_to_json(v) for v in value]
    return value  # str / int / float / bool / None pass through unchanged


def jalx_to_json(jalx_text, indent=2):
    doc = jalx.loads(jalx_text)
    plain = {k: _jalx_value_to_json(v) for k, v in doc.items()}
    return json.dumps(plain, ensure_ascii=False, indent=indent)


# ---------------------------------------------------------------------------
# Syntax checker
# ---------------------------------------------------------------------------

def check_syntax(jalx_text, also_validate=False):
    """Returns (ok: bool, message: str)."""
    try:
        doc = jalx.loads(jalx_text)
    except jalx.JALXError as e:
        return False, f"SYNTAX ERROR: {e}"
    except Exception as e:  # noqa: BLE001 -- surface *any* parse failure to the user
        return False, f"SYNTAX ERROR (unexpected): {e}"

    if also_validate:
        try:
            jalx.validate(doc)
        except jalx.JALXError as e:
            return False, f"VALIDATION ERROR: {e}"

    n_keys = len(doc)
    n_types = len(doc.schemas)
    root = doc.root or "(none)"
    msg = (f"OK -- syntax valid. top-level keys: {n_keys}, "
           f"@type declarations: {n_types}, @root: {root}")
    if also_validate:
        msg += " -- schema validation passed"
    return True, msg


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _read_input(path):
    if path == "-":
        return sys.stdin.read()
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _write_output(path, text):
    if path is None or path == "-":
        sys.stdout.write(text)
        if not text.endswith("\n"):
            sys.stdout.write("\n")
    else:
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
        print(f"wrote {path}")


def main():
    parser = argparse.ArgumentParser(description="JSON <-> JALX converter / syntax checker")
    sub = parser.add_subparsers(dest="command", required=True)

    p1 = sub.add_parser("json2jalx", help="Convert a JSON file to JALX")
    p1.add_argument("input")
    p1.add_argument("output", nargs="?")

    p2 = sub.add_parser("jalx2json", help="Convert a JALX file to JSON")
    p2.add_argument("input")
    p2.add_argument("output", nargs="?")
    p2.add_argument("--indent", type=int, default=2)

    p3 = sub.add_parser("check", help="Check a JALX file for syntax (and optionally schema) errors")
    p3.add_argument("input")
    p3.add_argument("--validate", action="store_true",
                     help="also run @type/@root schema validation, not just syntax")

    args = parser.parse_args()

    if args.command == "json2jalx":
        text = _read_input(args.input)
        try:
            out = json_to_jalx(text)
        except json.JSONDecodeError as e:
            print(f"JSON ERROR: {e}", file=sys.stderr)
            sys.exit(1)
        _write_output(args.output, out)

    elif args.command == "jalx2json":
        text = _read_input(args.input)
        try:
            out = jalx_to_json(text, indent=args.indent)
        except jalx.JALXError as e:
            print(f"JALX ERROR: {e}", file=sys.stderr)
            sys.exit(1)
        _write_output(args.output, out)

    elif args.command == "check":
        text = _read_input(args.input)
        ok, msg = check_syntax(text, also_validate=args.validate)
        print(msg)
        sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
