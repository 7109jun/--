# FK 프로그래밍 언어 완전 가이드

> 버전: FK 0.1
> 확장자: `.fk`
> 언어 유형: 난해 프로그래밍 언어 (Esoteric Programming Language)
> 기본 철학: **적은 문법, 강한 난해성**

---

# 1. FK란?

FK는 매우 적은 수의 문법으로 프로그램을 작성할 수 있도록 설계된 난해 프로그래밍 언어다.

FK는 Python, C, Java 같은 일반적인 범용 프로그래밍 언어와 경쟁하기 위한 언어가 아니다.

FK의 가장 중요한 목표는 다음과 같다.

> **배우는 것은 간단하지만, 복잡한 프로그램을 작성하면 매우 이상하고 난해해지는 것.**

---

# 2. FK의 철학

FK는 많은 문법을 사용하는 대신 적은 수의 문법과 연산자를 조합한다.

따라서 처음 배울 때 알아야 하는 문법은 매우 적다.

하지만 프로그램이 커질수록 표현이 압축되어 사람이 코드를 해석하기 어려워질 수 있다.

---

# 3. FK는 난해 언어다

FK는 일부러 읽기 편한 언어를 목표로 하지 않는다.

다음과 같은 코드는 FK에서 정상적인 표현이다.

```fk
current = maze = sell index(player_pos = sell index(1))
```

이러한 중첩 표현은 FK의 특징이다.

---

# 4. FK 파일 확장자

FK 프로그램은 `.fk` 확장자를 사용한다.

예:

```text
hello.fk
maze.fk
doom.fk
test.fk
```

---

# 5. 가장 기본적인 FK 프로그램

```fk
prinf("Hello FK!")
```

이 프로그램은 문자열을 출력한다.

---

# 6. 출력 명령

출력에는 `prinf()`를 사용한다.

```fk
prinf("Hello")
```

숫자도 출력할 수 있다.

```fk
prinf(123)
```

---

# 7. 문자열 출력

```fk
prinf("FK")
```

---

# 8. 숫자 출력

```fk
prinf(100)
```

---

# 9. 여러 출력

```fk
prinf("Hello")
prinf("FK")
prinf("World")
```

---

# 10. 변수의 기본 구조

FK의 기본적인 데이터 표현은 다음과 같다.

```fk
name = value
```

예:

```fk
age = 13
```

---

# 11. 일반적인 값 저장

```fk
x = 10
```

---

# 12. 문자열 값

```fk
name = "FK"
```

---

# 13. 변수 출력

```fk
name = "FK"
prinf(name)
```

---

# 14. 산술 계산

FK는 연산자를 사용해 계산한다.

```fk
x = 10 + 20
```

---

# 15. 덧셈

```fk
x = 10 + 5
```

결과:

```text
15
```

---

# 16. 뺄셈

```fk
x = 10 - 5
```

결과:

```text
5
```

---

# 17. 곱셈

```fk
x = 10 * 5
```

결과:

```text
50
```

---

# 18. 나눗셈

```fk
x = 10 / 5
```

결과:

```text
2
```

---

# 19. 나머지

```fk
x = 10 % 3
```

결과:

```text
1
```

---

# 20. 연산자 조합

```fk
x = 10 + 20 * 2
```

FK 인터프리터의 연산 규칙에 따라 계산된다.

---

# 21. Sell이란?

FK의 가장 중요한 기능 중 하나는 `sell`이다.

`sell`은 데이터를 여러 개의 index에 저장할 수 있는 공간이다.

---

# 22. Sell 생성

```fk
holy = sell make 1~10
```

이 코드는 `holy`이라는 sell을 생성한다.

---

# 23. Sell의 index

위 코드에서는 다음과 같은 index가 존재한다.

```text
holy[1]
holy[2]
holy[3]
...
holy[10]
```

---

# 24. Sell의 크기

FK의 sell은 고정된 최대 크기를 가지지 않는다.

필요에 따라 확장할 수 있다.

---

# 25. Sell은 사실상 무한 공간이다

언어 사양에서는 sell에 작은 고정 크기를 지정하지 않는다.

실제 제한은 실행 환경의 메모리와 구현 한계에 의해 결정된다.

---

# 26. Sell에 값 저장

```fk
holy = index(1) > 10
```

이 코드는 `holy`의 index 1에 `10`을 저장한다.

---

# 27. Sell에서 값 가져오기

```fk
holy = sell index(1)
```

이 코드는 `holy`의 index 1의 값을 가져온다.

---

# 28. Sell의 가장 중요한 패턴

```fk
holy = sell make 1~10
holy = index(1) > 10
x = holy = sell index(1)
prinf(x)
```

---

# 29. Sell 여러 개 사용

```fk
holy = sell make 1~10
fire = sell make 1~10
```

---

# 30. Sell마다 독립적인 데이터가 존재한다

```fk
holy = sell make 1~10
fire = sell make 1~10

holy = index(1) > 10
fire = index(1) > 20
```

---

# 31. Index란?

`index(n)`은 sell 내부의 특정 위치를 의미한다.

예:

```fk
holy = sell index(5)
```

---

# 32. Index 1

```fk
holy = index(1) > 100
```

---

# 33. Index 2

```fk
holy = index(2) > 200
```

---

# 34. Index 100

```fk
holy = index(100) > 999
```

sell이 필요한 경우 확장될 수 있다.

---

# 35. Sell 범위

```fk
holy = sell make 1~100
```

index 1부터 100까지 사용할 수 있다.

---

# 36. 범위 연산자 `~`

`~`는 범위를 표현한다.

```fk
1~10
```

이는 1부터 10까지를 의미한다.

---

# 37. 더 큰 범위

```fk
big = sell make 1~10000
```

---

# 38. Sell의 동적 사용

초기에 작은 범위를 만들고 이후 더 큰 index를 사용할 수 있다.

예:

```fk
holy = sell make 1~2
holy = index(100) > 50
```

구현에서는 필요한 범위가 확장될 수 있다.

---

# 39. Sell을 배열처럼 생각할 수 있다

다른 언어의 배열과 비슷하게 사용할 수 있지만, FK의 목적은 배열 문법을 만드는 것이 아니다.

FK에서는 sell 자체가 핵심 개념이다.

---

# 40. 조건문

FK는 `if`를 사용한다.

기본 구조:

```fk
if 조건 {
    코드
}
```

---

# 41. 기본 조건문

```fk
if 10 > 5 {
    prinf("YES")
}
```

---

# 42. 비교 연산자

FK에서 사용할 수 있는 대표적인 비교 연산자는 다음과 같다.

```text
==
!=
>
<
>=
<=
```

---

# 43. 같다

```fk
if x == 10 {
    prinf("10")
}
```

---

# 44. 다르다

```fk
if x != 10 {
    prinf("not 10")
}
```

---

# 45. 크다

```fk
if x > 10 {
    prinf("big")
}
```

---

# 46. 작다

```fk
if x < 10 {
    prinf("small")
}
```

---

# 47. 크거나 같다

```fk
if x >= 10 {
    prinf("big or equal")
}
```

---

# 48. 작거나 같다

```fk
if x <= 10 {
    prinf("small or equal")
}
```

---

# 49. 논리 연산

FK에서는 논리 연산자를 사용할 수 있다.

대표적인 연산자는 다음과 같다.

```text
&&
||
!
```

---

# 50. AND

```fk
if x > 10 && x < 20 {
    prinf("between")
}
```

---

# 51. OR

```fk
if x == 1 || x == 2 {
    prinf("one or two")
}
```

---

# 52. NOT

```fk
if !(x == 10) {
    prinf("not ten")
}
```

---

# 53. 코드 블록

FK의 코드 블록은 `{}`로 표현한다.

```fk
if x > 10 {
    prinf("BIG")
}
```

---

# 54. 중첩 코드 블록

```fk
if x > 10 {
    if y > 20 {
        prinf("BIG")
    }
}
```

---

# 55. 반복

FK의 대표적인 반복 문법은 매우 독특하다.

```fk
[코드](횟수)
```

---

# 56. 5번 반복

```fk
[prinf("FK")](5)
```

---

# 57. 결과

위 프로그램은 다음과 같이 동작한다.

```text
FK
FK
FK
FK
FK
```

---

# 58. 여러 줄 반복

```fk
[
    prinf("A")
    prinf("B")
](3)
```

---

# 59. 반복 내부 변수

```fk
i = 1

[
    prinf(i)
    i = i + 1
](5)
```

이런 식으로 반복 횟수에 따른 상태를 직접 관리할 수 있다.

---

# 60. FK의 반복 특징

FK에는 여러 종류의 반복문을 만들 필요가 없다.

하나의 반복 구조만으로 반복 기능을 표현하는 것이 FK의 철학이다.

---

# 61. 무한 반복

매우 큰 횟수를 지정하거나 구현상 제공되는 반복 구조를 이용해 반복을 구성할 수 있다.

단, 무한 루프 전용 문법을 언어 핵심 문법으로 추가할 필요는 없다.

---

# 62. 주석

FK의 주석은 매우 특이하다.

주석 시작은:

```fk
//;;##
```

이다.

---

# 63. 주석은 한 줄 전체여야 한다

올바른 예:

```fk
//;;## FK test
```

---

# 64. 인라인 주석은 없다

다음은 올바른 FK 문법이 아니다.

```fk
x = 10 //;;## test
```

FK의 주석은 코드 뒤에 붙일 수 없다.

---

# 65. 주석은 한 줄을 통째로 사용한다

예:

```fk
//;;## player position
player = sell make 1~2
```

---

# 66. 여러 줄 설명

각 줄에 주석을 작성한다.

```fk
//;;## first line
//;;## second line
//;;## third line
```

---

# 67. FK 프로그램의 기본 구조

일반적인 FK 프로그램은 다음과 같은 구조를 가질 수 있다.

```fk
//;;## program

data = sell make 1~10

data = index(1) > 10

if data = sell index(1) > 5 {
    prinf("BIG")
}
```

---

# 68. 중첩 표현

FK의 난해성이 발생하는 가장 큰 이유 중 하나다.

```fk
x = data = sell index(a = sell index(1))
```

---

# 69. 중첩의 장점

문법을 많이 만들지 않아도 복잡한 표현을 한 줄에 넣을 수 있다.

---

# 70. 중첩의 단점

사람이 읽기 어려워진다.

하지만 이것은 FK에서는 반드시 단점만을 의미하지 않는다.

**난해성 자체가 FK의 목적 중 하나이기 때문이다.**

---

# 71. FK와 Python 비교

Python은 일반적으로 읽기 쉬운 코드를 작성하는 것을 중요하게 생각한다.

FK는 그렇지 않다.

FK는:

> 적은 문법 + 많은 조합 + 높은 난해성

을 목표로 한다.

---

# 72. FK와 C 비교

C는 저수준 프로그래밍과 시스템 프로그래밍에 매우 적합하다.

FK는 그러한 목적보다 **언어 자체의 특이성**을 중요하게 생각한다.

---

# 73. FK는 좋은 언어인가?

목적에 따라 다르다.

운영체제 개발이나 대규모 상용 소프트웨어를 작성하기 위한 언어라면 적합하지 않을 수 있다.

하지만 난해 언어라는 목적에서는 충분히 의미가 있다.

---

# 74. FK는 왜 문법이 적은가?

문법을 많이 만들지 않기 위해서다.

FK에서는 새로운 문제를 만났을 때 새로운 문법을 추가하기보다 기존 문법과 연산자를 조합하는 방향을 선호한다.

---

# 75. FK는 왜 코드가 긴가?

문법의 수는 적지만 복잡한 문제를 해결할 때 데이터를 직접 다뤄야 하기 때문이다.

---

# 76. FK는 왜 코드가 이상하게 보이는가?

변수 접근과 계산을 중첩할 수 있기 때문이다.

예:

```fk
a = b = sell index(c = d = sell index(1) + 2)
```

---

# 77. FK의 난해함은 의도적이다

FK에서 다음과 같은 현상은 정상이다.

```text
문법은 간단하다.
코드는 어렵다.
```

이것은 FK의 핵심적인 설계 목표다.

---

# 78. 프로그램 상태를 Sell로 만들 수 있다

예:

```fk
player = sell make 1~5

player = index(1) > 10
player = index(2) > 20
player = index(3) > 0
player = index(4) > 100
player = index(5) > 50
```

---

# 79. 게임 데이터에도 사용할 수 있다

예:

```fk
enemy = sell make 1~4

enemy = index(1) > 8
enemy = index(2) > 4
enemy = index(3) > 30
enemy = index(4) > 1
```

---

# 80. 맵 데이터도 Sell로 만들 수 있다

예:

```fk
map = sell make 1~25

map = index(1) > 1
map = index(2) > 0
map = index(3) > 0
```

---

# 81. 미로 구현

FK의 sell은 미로처럼 1차원 데이터로 저장된 2차원 구조를 표현하는 데 사용할 수 있다.

예:

```text
1  2  3  4  5
6  7  8  9  10
11 12 13 14 15
16 17 18 19 20
21 22 23 24 25
```

---

# 82. 위치 계산

예:

```fk
pos = (y - 1) * 20 + x
```

---

# 83. 맵 값 접근

```fk
tile = map = sell index(pos)
```

---

# 84. 게임 상태 구현

FK에서는 여러 개의 sell을 사용해 상태를 관리할 수 있다.

예:

```fk
player = sell make 1~5
map = sell make 1~200
enemies = sell make 1~50
config = sell make 1~10
```

---

# 85. 콘솔 게임

FK에서는 `prinf()`를 이용해 콘솔 기반 게임을 구현할 수 있다.

예:

```fk
prinf("####################")
prinf("#..................#")
prinf("#.......@..........#")
prinf("#..................#")
prinf("####################")
```

---

# 86. 반복을 이용한 렌더링

```fk
[
    prinf("##########")
](10)
```

---

# 87. 조건과 Sell 조합

```fk
player = sell make 1~2

player = index(1) > 5

if player = sell index(1) > 0 {
    prinf("alive")
}
```

---

# 88. AI 구현

FK는 적은 문법만으로도 상태 기반 AI를 만들 수 있다.

예:

```fk
enemy = sell make 1~4

if enemy = sell index(1) < player = sell index(1) {
    enemy = index(1) > enemy = sell index(1) + 1
}
```

---

# 89. 반복 + 조건 + Sell

이 세 기능을 조합하면 FK의 실제 프로그램이 상당히 복잡해진다.

```fk
[
    if player = sell index(1) < 10 {
        player = index(1) > player = sell index(1) + 1
    }
](50)
```

---

# 90. FK의 유명한 코드 스타일

FK에서는 다음과 같은 코드가 충분히 정상적인 스타일이다.

```fk
current = maze = sell index(player_pos = sell index(1))
```

또는:

```fk
next_pos = player = sell index(2) * 20 + player = sell index(1)
```

---

# 91. 복잡성을 줄이는 방법

FK 자체의 문법을 늘리기보다는 중간 변수를 사용한다.

예:

```fk
player_x = player = sell index(1)
player_y = player = sell index(2)

pos = (player_y - 1) * 20 + player_x

tile = maze = sell index(pos)
```

---

# 92. 복잡성을 높이는 방법

FK의 난해성을 즐기고 싶다면 중첩 표현을 사용할 수 있다.

예:

```fk
tile = maze = sell index((player = sell index(2) - 1) * 20 + player = sell index(1))
```

---

# 93. FK의 코드 스타일은 자유롭다

읽기 쉬운 FK를 만들 수도 있고, 매우 난해한 FK를 만들 수도 있다.

둘 다 유효한 FK 코드다.

---

# 94. 난해성 레벨

FK 프로그램은 다음과 같이 난해성을 구분할 수 있다.

```text
Level 1   매우 쉬움
Level 2   쉬움
Level 3   보통
Level 4   난해
Level 5   매우 난해
Level 6   인간이 싫어함
```

---

# 95. Level 1 예제

```fk
prinf("Hello")
```

---

# 96. Level 3 예제

```fk
x = sell make 1~5
x = index(1) > 10

if x = sell index(1) > 5 {
    prinf("BIG")
}
```

---

# 97. Level 5 예제

```fk
x = sell make 1~10
x = index(1) > 10
x = index(2) > x = sell index(1) + 20

if x = sell index(2) > x = sell index(1) {
    prinf("BIG")
}
```

---

# 98. Level 6 예제

```fk
[
    current = maze = sell index(player = sell index(1))
    next = current + maze = sell index(player = sell index(2))
    if next > maze = sell index(current) {
        player = index(1) > player = sell index(1) + 1
    }
](100)
```

이 정도가 되면 FK의 난해함이 본격적으로 드러난다.

---

# 99. FK의 핵심 문법 요약

```text
=              값과 표현 연결
sell           Sell 사용
make           Sell 생성
index()        Sell index 접근
>              Sell 값 저장에 사용
if             조건문
{}             코드 블록
[]()           반복
prinf()        출력
//;;##         주석
~              범위
```

대표 연산자:

```text
+
-
*
/
%
==
!=
>
<
>=
<=
&&
||
!
```

---

# 100. FK를 한 문장으로 설명하면

> **FK는 배우기 위한 문법은 작지만, 프로그램을 작성하면 점점 이상해지는 것을 목표로 하는 난해 프로그래밍 언어다.**

FK의 핵심은 **“좋은 프로그래밍 언어”가 되는 것이 아니다.**

FK는 다음을 목표로 한다.

```text
문법은 적다.
개념도 적다.
배우기는 쉽다.

하지만...

코드를 작성하면 이상하다.
중첩하면 더 이상하다.
프로그램이 커지면 매우 난해하다.

그리고 그것이 정상이다.
```

FK는 **간단한 문법으로 복잡하고 기묘한 프로그램을 만드는 것 자체가 재미있는 언어**다.
