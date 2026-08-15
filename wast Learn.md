# Wast Tutorial

Wast는 가볍고 읽기 쉬운 프로그래밍 언어입니다.

스크립팅에 적합하도록 설계되었으며, WastC를 사용하면 Wast 코드를 C++ 코드로 변환할 수 있습니다.

---

# 1. Hello, World!

Wast의 가장 기본적인 프로그램입니다.

```wast
print("Hello, World!")
```

`print()`는 값을 출력합니다.

여러 값을 전달할 수도 있습니다.

```wast
print("Hello", 123, true)
```

---

# 2. 주석

한 줄 주석은 `//`를 사용합니다.

```wast
// 이것은 주석입니다.
print("Hello")
```

---

# 3. 변수

변수는 `local`을 사용합니다.

```wast
local name = "Wast"
local age = 10
local score = 95.5
```

변수의 값을 변경할 수 있습니다.

```wast
local score = 10

score = 20
score += 5
```

지원되는 복합 대입 연산자:

```text
+=
-=
*=
/=
%=
```

예:

```wast
local x = 10

x += 5
x -= 2
x *= 3
x /= 2
x %= 4
```

---

# 4. 상수

`const`를 사용하면 상수를 선언할 수 있습니다.

```wast
const version = "5.0"
const max_players = 4
```

---

# 5. 기본 데이터 타입

Wast는 다음과 같은 값을 사용할 수 있습니다.

```text
nil
bool
int
float
str
array
map
func
```

## 정수

```wast
local x = 123
```

## 실수

```wast
local pi = 3.14159
```

## 문자열

```wast
local name = "Wast"
```

## Boolean

```wast
local enabled = true
local disabled = false
```

## Nil

```wast
local value = nil
```

---

# 6. 문자열

문자열은 큰따옴표를 사용합니다.

```wast
local name = "Wast"
print(name)
```

문자열에는 escape sequence를 사용할 수 있습니다.

```text
\n
\t
\r
\\
\"
\0
```

예:

```wast
print("Hello\nWast")
```

---

# 7. 문자열 보간

문자열 안에 변수 값을 넣을 수 있습니다.

```wast
local name = "Wast"

print("Hello, {name}!")
```

숫자도 사용할 수 있습니다.

```wast
print("Version {5}")
print("Pi = {3.14}")
```

예:

```wast
local hp = 100

print("HP: {hp}")
```

현재 문자열 보간은 간단한 변수 이름 또는 숫자를 지원합니다.

다음처럼 복잡한 식을 직접 넣는 것은 지원하지 않습니다.

```wast
// 지원하지 않음
print("HP: {player.hp + 10}")
```

---

# 8. 산술 연산자

다음 산술 연산자를 사용할 수 있습니다.

```text
+
-
*
/
%
```

예:

```wast
local a = 10
local b = 3

print(a + b)
print(a - b)
print(a * b)
print(a / b)
print(a % b)
```

---

# 9. 비교 연산자

```text
==
!=
<
<=
>
>=
```

예:

```wast
local hp = 50

print(hp == 50)
print(hp > 10)
print(hp <= 100)
```

---

# 10. 논리 연산자

```text
&&
||
!
```

예:

```wast
local alive = true
local enemy = false

if alive && !enemy (
    print("Safe")
)
```

---

# 11. If

Wast의 블록은 `( ... )` 형태로 작성합니다.

```wast
if score >= 90 (
    print("A")
)
```

`elseif`와 `else`도 사용할 수 있습니다.

```wast
if score >= 90 (
    print("A")
) elseif score >= 80 (
    print("B")
) elseif score >= 70 (
    print("C")
) else (
    print("F")
)
```

---

# 12. While

`while`을 사용해 조건이 참인 동안 반복할 수 있습니다.

```wast
local x = 0

while x < 5 (
    print(x)
    x += 1
)
```

---

# 13. For

숫자 범위를 반복할 수 있습니다.

```wast
for i = 0, 5 (
    print(i)
)
```

시작값, 끝값, 증가값을 지정할 수도 있습니다.

```wast
for i = 0, 10, 2 (
    print(i)
)
```

감소도 가능합니다.

```wast
for i = 10, 0, -1 (
    print(i)
)
```

---

# 14. For-In

배열 등의 값을 순회할 수 있습니다.

```wast
local nums = [10, 20, 30, 40]

for x in nums (
    print(x)
)
```

---

# 15. Break

반복문을 즉시 종료합니다.

```wast
local i = 0

while true (
    print(i)
    i += 1

    if i >= 5 (
        break
    )
)
```

---

# 16. Continue

현재 반복을 건너뛰고 다음 반복으로 넘어갑니다.

```wast
for i = 0, 10 (
    if i == 5 (
        continue
    )

    print(i)
)
```

---

# 17. 함수

함수는 `fn`으로 선언합니다.

```wast
fn hello() (
    print("Hello!")
)
```

함수 호출:

```wast
hello()
```

---

# 18. 함수 인자

함수에 인자를 전달할 수 있습니다.

```wast
fn greet(name) (
    print("Hello, {name}!")
)

greet("Wast")
```

여러 인자도 사용할 수 있습니다.

```wast
fn add(a, b) (
    return a + b
)

local result = add(10, 20)

print(result)
```

---

# 19. 반환값

`return`을 사용합니다.

```wast
fn square(x) (
    return x * x
)

local result = square(5)

print(result)
```

값을 반환하지 않을 수도 있습니다.

```wast
fn hello() (
    print("Hello")
    return
)
```

마지막 표현식을 암시적으로 반환할 수도 있습니다.

```wast
fn add(a, b) (
    a + b
)
```

---

# 20. 타입 표기

함수 매개변수와 반환값에는 타입 표기를 작성할 수 있습니다.

```wast
fn add(a: int, b: int) -> int (
    return a + b
)
```

변수에도 타입 표기를 사용할 수 있습니다.

```wast
local x: int = 10
```

---

# 21. 배열

배열은 `[]`를 사용합니다.

```wast
local numbers = [1, 2, 3, 4, 5]
```

인덱스로 접근할 수 있습니다.

```wast
print(numbers[0])
print(numbers[2])
```

음수 인덱스도 사용할 수 있습니다.

```wast
print(numbers[-1])
```

---

# 22. 배열 수정

인덱스에 값을 대입할 수 있습니다.

```wast
local numbers = [1, 2, 3]

numbers[0] = 100
```

`push()`로 값을 추가할 수 있습니다.

```wast
local numbers = [1, 2, 3]

push(numbers, 4)
```

`pop()`으로 마지막 값을 제거하고 가져올 수 있습니다.

```wast
local numbers = [1, 2, 3]

local last = pop(numbers)

print(last)
```

---

# 23. 배열 길이

`len()`을 사용합니다.

```wast
local numbers = [10, 20, 30]

print(len(numbers))
```

---

# 24. Map

Map은 `{}`를 사용합니다.

```wast
local player = {
    name: "Player",
    hp: 100,
    level: 5
}
```

필드에 접근할 수 있습니다.

```wast
print(player.name)
print(player.hp)
```

값을 변경할 수도 있습니다.

```wast
player.hp = 80
```

인덱스로도 접근할 수 있습니다.

```wast
print(player["name"])
```

---

# 25. Map의 Keys와 Values

`keys()`를 사용하면 Map의 키를 가져올 수 있습니다.

```wast
local player = {
    name: "Player",
    hp: 100
}

print(keys(player))
```

`values()`를 사용하면 값을 가져올 수 있습니다.

```wast
print(values(player))
```

---

# 26. Struct

`struct`를 사용하면 구조를 정의할 수 있습니다.

```wast
struct Player(
    name: str,
    hp: int,
    level: int
)
```

생성:

```wast
local player = Player("Knight", 100, 5)
```

필드 접근:

```wast
print(player.name)
print(player.hp)
print(player.level)
```

---

# 27. Enum

Enum은 `enum`으로 선언합니다.

```wast
enum State = Idle | Walk | Attack | Dead
```

Enum 값은 다음과 같이 사용할 수 있습니다.

```wast
print(State.Idle)
print(State.Attack)
```

---

# 28. Match

`match`는 값에 따라 분기할 때 사용합니다.

```wast
local state = "Idle"

match state (
    Idle -> print("대기")
    Walk -> print("이동")
    Attack -> print("공격")
    _ -> print("알 수 없음")
)
```

여러 패턴을 하나의 분기에 사용할 수도 있습니다.

```wast
match state (
    Idle | Walk -> print("일반 상태")
    Attack -> print("전투")
    _ -> print("기타")
)
```

`_`는 wildcard입니다.

---

# 29. Lambda

Lambda는 이름이 없는 함수입니다.

```wast
local double = fn(x) (
    return x * 2
)

print(double(10))
```

마지막 표현식을 반환할 수도 있습니다.

```wast
local double = fn(x) (
    x * 2
)
```

---

# 30. Pipe

Wast에는 `|>` pipe 연산자가 있습니다.

```wast
local nums = [1, 2, 3]

nums |> len |> print
```

함수에 값을 전달할 수도 있습니다.

```wast
fn double(x) (
    x * 2
)

10 |> double |> print
```

Lambda도 pipe의 대상이 될 수 있습니다.

```wast
10 |> fn(x) (
    x * 2
) |> print
```

---

# 31. Range

`..`와 `..=`를 사용할 수 있습니다.

```wast
local a = 1 .. 5
local b = 1 ..= 5
```

`range()` 함수도 사용할 수 있습니다.

```wast
local a = range(1, 5)
local b = range(1, 5, true)
```

---

# 32. Map

`map()`은 배열의 각 요소에 함수를 적용합니다.

```wast
local nums = [1, 2, 3, 4]

local doubled = map(nums, fn(x) (
    x * 2
))

print(doubled)
```

---

# 33. Filter

`filter()`는 조건을 만족하는 요소만 남깁니다.

```wast
local nums = [1, 2, 3, 4, 5]

local even = filter(nums, fn(x) (
    x % 2 == 0
))

print(even)
```

---

# 34. Reduce

`reduce()`는 여러 값을 하나의 값으로 축약합니다.

```wast
local nums = [1, 2, 3, 4]

local total = reduce(nums, 0, fn(acc, x) (
    acc + x
))

print(total)
```

---

# 35. Type

`type()`으로 값의 타입을 확인할 수 있습니다.

```wast
print(type(10))
print(type(3.14))
print(type("Hello"))
print(type(true))
print(type([1, 2, 3]))
```

Wast의 주요 타입:

```text
nil
bool
int
float
str
array
map
func
```

---

# 36. 타입 변환

## int

```wast
print(int("123"))
print(int(3.14))
print(int(true))
```

## float

```wast
print(float("3.14"))
print(float(10))
```

## str

```wast
print(str(123))
print(str(true))
```

---

# 37. Input

`input()`으로 입력을 받을 수 있습니다.

```wast
print("이름을 입력하세요:")

local name = input()

print("Hello, {name}!")
```

---

# 38. Sleep

`sleep()`은 밀리초 단위로 대기합니다.

```wast
print("Start")

sleep(1000)

print("1 second later")
```

---

# 39. Shell

`shell()`을 사용하면 시스템 명령을 실행할 수 있습니다.

```wast
shell("echo Hello")
```

반환값을 받을 수도 있습니다.

```wast
local result = shell("echo Hello")

print(result)
```

`Shell`은 운영체제의 명령 실행 기능을 사용하므로 실행 환경에 따라 동작이 달라질 수 있습니다.

---

# 40. 함수와 배열

Wast에서는 함수와 배열을 함께 사용할 수 있습니다.

```wast
fn square(x) (
    x * x
)

local numbers = [1, 2, 3, 4, 5]

local result = map(numbers, square)

print(result)
```

---

# 41. 완전한 예제

다음은 Wast의 여러 기능을 사용하는 예제입니다.

```wast
struct Player(
    name: str,
    hp: int,
    level: int
)

enum State = Idle | Attack | Dead

fn damage(player, amount) (
    player.hp -= amount

    if player.hp <= 0 (
        player.hp = 0
        return State.Dead
    )

    State.Attack
)

fn main() (
    local player = Player("Knight", 100, 5)

    print("Player: {player.name}")
    print("HP: {player.hp}")
    print("Level: {player.level}")

    local state = damage(player, 30)

    print("HP: {player.hp}")

    match state (
        Attack -> print("Player attacked")
        Dead -> print("Player is dead")
        _ -> print("Idle")
    )
)
```

---

# 42. WastC 사용하기

WastC는 Wast 소스 파일을 C++ 코드로 변환합니다.

기본 사용법:

```text
wastc input.wast output.cpp
```

출력 파일을 생략할 수도 있습니다.

```text
wastc input.wast
```

이 경우 기본적으로 다음과 같은 출력 파일이 생성됩니다.

```text
input.wast.cpp
```

WastC의 처리 과정:

```text
Wast
  ↓
Lexer
  ↓
Parser
  ↓
AST
  ↓
Optimizer
  ↓
C++ Code Generator
  ↓
C++
```

WastC는 C++17과 표준 라이브러리를 사용합니다.

---

# 43. 최적화

WastC에는 기본적인 상수식 최적화가 포함되어 있습니다.

예:

```wast
local x = 10 + 20
```

상수로만 이루어진 연산은 컴파일 과정에서 계산될 수 있습니다.

문자열 상수 결합도 최적화 대상입니다.

```wast
local text = "Hello " + "Wast"
```

Boolean 상수식도 최적화할 수 있습니다.

```wast
local value = true && false
```

---

# 44. Wast의 특징

Wast는 복잡한 문법보다 읽기 쉬운 코드를 목표로 합니다.

함수:

```wast
fn add(a, b) (
    a + b
)
```

조건:

```wast
if hp <= 0 (
    die()
)
```

반복:

```wast
for i = 0, 10 (
    print(i)
)
```

데이터:

```wast
local player = {
    name: "Player",
    hp: 100
}
```

함수형 처리:

```wast
numbers |> map
```

---

# 45. 빠른 문법 요약

```text
// comment

local x = 10
const y = 20

x = 30
x += 1

if condition (
    ...
) elseif condition (
    ...
) else (
    ...
)

while condition (
    ...
)

for i = 0, 10 (
    ...
)

for x in items (
    ...
)

fn hello() (
    ...
)

fn add(a, b) (
    return a + b
)

struct Player(
    name: str,
    hp: int
)

enum State = Idle | Walk | Attack

match state (
    Idle -> ...
    Walk -> ...
    _ -> ...
)

local f = fn(x) (
    x * 2
)

numbers |> len |> print

[1, 2, 3]

{
    name: "Player",
    hp: 100
}

value[0]
value.field

push(array, value)
pop(array)
len(value)

map(array, fn(x) (...))
filter(array, fn(x) (...))
reduce(array, initial, fn(a, b) (...))

int(value)
float(value)
str(value)
type(value)

input()
sleep(1000)
shell("command")
```

---

# 46. 다음 단계

Wast의 기본 문법을 익혔다면 다음 순서로 연습하는 것을 추천합니다.

1. 변수와 연산자
2. 조건문
3. 반복문
4. 함수
5. 배열과 Map
6. Struct와 Enum
7. Match
8. Lambda
9. Pipe
10. Map / Filter / Reduce
11. WastC로 C++ 코드 생성

간단한 프로그램부터 시작해 점점 여러 기능을 조합하면 Wast에 익숙해질 수 있습니다.
