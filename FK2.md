# FK 프로그래밍 언어 v2.0 문법 가이드

> **FK v2.0**
>
> 확장자: `.fk`
>
> 종류: **난해 프로그래밍 언어 (Esoteric Programming Language)**
>
> 설계 철학: **적은 문법, 많은 조합, 높은 난해성**

---

# 1. FK란?

FK는 문법을 최대한 적게 유지하면서 프로그램을 작성할 수 있도록 설계된 난해 프로그래밍 언어다.

FK는 Python이나 C처럼 "읽기 편하고 생산적인 언어"를 목표로 하지 않는다.

FK의 핵심 목표는 다음과 같다.

```text
배우기는 쉽다.
문법은 적다.
기능은 조합해서 사용한다.
프로그램이 커지면 이상해진다.
```

---

# 2. FK의 핵심 철학

FK는 새로운 기능을 만들 때마다 새로운 문법을 계속 추가하는 방식보다 기존 문법을 조합하는 방식을 선호한다.

예를 들어:

```fk
current = maze = sell index(player_pos = sell index(1))
```

이처럼 하나의 표현 안에 여러 작업을 넣을 수 있다.

이것이 FK의 난해성을 만든다.

---

# 3. FK는 난해 언어다

FK는 일부러 코드가 이상하게 보일 수 있도록 설계할 수 있다.

다음과 같은 코드는 정상적인 FK 코드다.

```fk
value = data = sell index(position = player = sell index(1))
```

가독성이 떨어져도 FK에서는 문제가 되지 않는다.

**난해함 자체가 언어의 특징이기 때문이다.**

---

# 4. FK 파일

FK 소스 파일의 확장자는 `.fk`다.

예:

```text
hello.fk
maze.fk
game.fk
doom.fk
test.fk
```

---

# 5. 가장 기본적인 프로그램

```fk
prinf("Hello FK")
```

---

# 6. 출력

출력 함수는 `prinf()`다.

```fk
prinf("Hello")
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

# 9. 표현식 출력

```fk
prinf(10 + 20)
```

---

# 10. 여러 값 출력

```fk
prinf("HP")
prinf(100)
prinf("Ammo")
prinf(50)
```

---

# 11. 기본 변수 구조

FK의 기본적인 값 표현은 다음과 같다.

```fk
name = value
```

예:

```fk
age = 13
```

---

# 12. 숫자 변수

```fk
x = 100
```

---

# 13. 문자열 변수

```fk
name = "FK"
```

---

# 14. 변수 사용

```fk
name = "FK"
prinf(name)
```

---

# 15. 변수 재할당

```fk
x = 10
x = 20
```

---

# 16. 덧셈

```fk
x = 10 + 20
```

---

# 17. 뺄셈

```fk
x = 20 - 10
```

---

# 18. 곱셈

```fk
x = 10 * 20
```

---

# 19. 나눗셈

```fk
x = 20 / 10
```

---

# 20. 나머지

```fk
x = 20 % 3
```

---

# 21. 산술 연산자

기본 산술 연산자는 다음과 같다.

```text
+
-
*
/
%
```

---

# 22. 비교 연산자

비교에는 다음과 같은 연산자를 사용할 수 있다.

```text
==
!=
>
<
>=
<=
```

---

# 23. 같다

```fk
if x == 10 {
    prinf("TEN")
}
```

---

# 24. 다르다

```fk
if x != 10 {
    prinf("NOT TEN")
}
```

---

# 25. 크다

```fk
if x > 10 {
    prinf("BIG")
}
```

---

# 26. 작다

```fk
if x < 10 {
    prinf("SMALL")
}
```

---

# 27. 크거나 같다

```fk
if x >= 10 {
    prinf("BIG")
}
```

---

# 28. 작거나 같다

```fk
if x <= 10 {
    prinf("SMALL")
}
```

---

# 29. 논리 AND

```fk
if x > 10 && x < 20 {
    prinf("OK")
}
```

`&&`는 AND다.

---

# 30. 논리 OR

```fk
if x == 1 || x == 2 {
    prinf("ONE OR TWO")
}
```

`||`는 OR다.

---

# 31. 논리 NOT

```fk
if !(x == 10) {
    prinf("NOT TEN")
}
```

`!`는 NOT이다.

---

# 32. 조건문

기본 조건문:

```fk
if 조건 {
    코드
}
```

---

# 33. 조건문 예제

```fk
x = 10

if x == 10 {
    prinf("YES")
}
```

---

# 34. 코드 블록

FK의 코드 블록은 `{}`다.

```fk
if x > 5 {
    prinf("A")
    prinf("B")
}
```

---

# 35. 중첩 블록

```fk
if x > 5 {
    if y > 10 {
        prinf("YES")
    }
}
```

---

# 36. Else

조건이 거짓일 때 다른 블록을 실행할 수 있다.

```fk
if x > 10 {
    prinf("BIG")
} else {
    prinf("SMALL")
}
```

---

# 37. Sell

FK에서 가장 중요한 자료 구조 중 하나가 `sell`이다.

```fk
holy = sell make 1~10
```

---

# 38. Sell 생성

위 문장은 `holy`라는 Sell을 만든다.

```fk
holy = sell make 1~10
```

---

# 39. Sell의 Index

위 Sell에는 다음과 같은 index가 있다.

```text
1
2
3
4
...
10
```

---

# 40. Sell의 크기

Sell은 고정된 작은 최대 크기를 목표로 하지 않는다.

필요한 데이터를 더 사용할 수 있도록 확장할 수 있다.

---

# 41. Sell의 기본 구조

개념적으로:

```text
holy
 ├─ index 1
 ├─ index 2
 ├─ index 3
 ├─ ...
 └─ index 10
```

---

# 42. Index 값 저장

```fk
holy = index(1) > 10
```

---

# 43. Index 값 가져오기

```fk
holy = sell index(1)
```

---

# 44. Index 값을 변수로 가져오기

```fk
x = holy = sell index(1)
```

---

# 45. Index 여러 개 사용

```fk
holy = index(1) > 10
holy = index(2) > 20
holy = index(3) > 30
```

---

# 46. Sell 범위

`~`는 범위를 나타낸다.

```fk
holy = sell make 1~100
```

---

# 47. 범위의 의미

```text
1~10
```

은 1부터 10까지를 의미한다.

---

# 48. Sell의 동적 사용

예:

```fk
holy = sell make 1~2
holy = index(100) > 500
```

Sell은 필요한 범위에 맞게 확장될 수 있다.

---

# 49. Sell 여러 개

```fk
player = sell make 1~5
enemy = sell make 1~10
map = sell make 1~200
```

---

# 50. Sell을 이용한 상태 저장

```fk
player = sell make 1~5

player = index(1) > 10
player = index(2) > 20
player = index(3) > 100
```

---

# 51. 반복

FK의 반복 문법은 다음과 같다.

```fk
[코드](횟수)
```

---

# 52. 5회 반복

```fk
[prinf("FK")](5)
```

---

# 53. 반복 결과

```text
FK
FK
FK
FK
FK
```

---

# 54. 여러 줄 반복

```fk
[
    prinf("A")
    prinf("B")
](3)
```

---

# 55. 반복 안에서 변수 사용

```fk
i = 1

[
    prinf(i)
    i = i + 1
](5)
```

---

# 56. 반복 + 조건

```fk
[
    if x > 10 {
        prinf("BIG")
    }
](10)
```

---

# 57. 반복 중첩

```fk
[
    [
        prinf("FK")
    ](5)
](5)
```

---

# 58. 반복의 의미

`[](N)`은 내부 코드를 N번 실행한다.

따라서 별도의 여러 반복문을 만들 필요가 없다.

---

# 59. 주석

FK 주석의 시작은 다음과 같다.

```text
//;;##
```

---

# 60. 주석 예제

```fk
//;;## FK program
```

---

# 61. 주석은 한 줄 전체다

FK에서는 인라인 주석을 사용하지 않는다.

올바른 예:

```fk
//;;## Player data
player = sell make 1~5
```

---

# 62. 잘못된 주석

다음처럼 코드 뒤에 주석을 붙이는 방식은 FK 문법이 아니다.

```fk
x = 10 //;;## test
```

---

# 63. 여러 줄 설명

여러 줄 설명을 작성하려면 줄마다 주석을 사용한다.

```fk
//;;## first
//;;## second
//;;## third
```

---

# 64. 파일 저장

FK v2.0에서는 Sell 데이터를 실제 컴퓨터 파일로 저장할 수 있다.

기본 형태:

```fk
Sell (name) index (1) computer (경로) Make
```

---

# 65. 파일 저장 예제

```fk
data = sell make 1~10
data = index(1) > "Hello FK"

Sell (data) index (1) computer (C:\User\Drive\FK\file) Make
```

---

# 66. `computer`

`computer (...)`는 실제 컴퓨터의 파일 경로를 지정한다.

예:

```fk
computer (C:\User\Drive\FK\file)
```

---

# 67. `Make`

`Make`는 Sell의 값을 지정된 파일에 저장하는 동작이다.

```fk
Sell (data) index (1) computer (C:\User\Drive\FK\file) Make
```

---

# 68. 파일 덮어쓰기

`to cover up`은 기존 파일에 값을 덮어쓴다.

```fk
Sell (data) index (1) computer (C:\User\Drive\FK\file) to cover up
```

---

# 69. `to cover up`의 의미

개념:

```text
기존 파일
   ↓
새 Sell 값
   ↓
파일 내용 교체
```

---

# 70. 파일 삭제

`Kill`은 지정된 컴퓨터 파일을 삭제하는 동작이다.

```fk
Sell (data) index (1) computer (C:\User\Drive\FK\file) Kill
```

---

# 71. 파일 I/O 전체 형태

```fk
Sell (name) index (1) computer (path) Make
Sell (name) index (1) computer (path) to cover up
Sell (name) index (1) computer (path) Kill
```

---

# 72. API 통신

FK v2.0은 `https://` 형태의 외부 API 통신 표현을 사용할 수 있다.

예:

```fk
https://api.example.com
```

---

# 73. API URL

API 주소는 URL 형태로 표현한다.

```fk
https://api.example.com/data
```

---

# 74. API 결과를 변수와 연결

API 기능은 FK의 다른 데이터 처리와 연결해서 사용할 수 있다.

예:

```fk
data = https://api.example.com/data
```

---

# 75. API와 Sell

API로 얻은 데이터를 Sell과 결합하는 형태도 사용할 수 있다.

```fk
data = sell make 1~10
data = https://api.example.com/data
```

---

# 76. GUI

FK v2.0에서는 GUI 관련 명령을 사용할 수 있다.

기본 창:

```fk
gui window (800,600) Make
```

---

# 77. GUI 창 크기

```fk
gui window (800,600) Make
```

여기서:

```text
800 = 너비
600 = 높이
```

---

# 78. GUI 창 제목

```fk
gui window (800,600) title ("FK") Make
```

---

# 79. GUI 표시

```fk
gui window Show
```

---

# 80. GUI 종료

```fk
gui window Kill
```

---

# 81. GUI 텍스트

```fk
gui text ("Hello FK") Make
```

---

# 82. GUI 텍스트 위치

```fk
gui text ("Hello FK") position (100,100) Make
```

---

# 83. GUI 버튼

```fk
gui button ("CLICK") position (100,100) Make
```

---

# 84. GUI 사각형

```fk
gui rect (100,100,200,200) Make
```

---

# 85. GUI 이미지

```fk
gui image (C:\User\Drive\FK\image.png) Make
```

---

# 86. 이미지 위치

```fk
gui image (C:\User\Drive\FK\image.png) position (100,100) Make
```

---

# 87. 마우스

FK는 실시간 마우스 입력을 다룰 수 있다.

```fk
mouse = position
```

---

# 88. 마우스 X

```fk
mouse = x
```

---

# 89. 마우스 Y

```fk
mouse = y
```

---

# 90. 마우스 버튼

대표적인 마우스 버튼:

```text
left
right
middle
```

예:

```fk
if mouse = left {
    prinf("CLICK")
}
```

---

# 91. 키보드

FK v2.0은 실시간 키보드 입력을 지원할 수 있다.

```fk
keyboard = "A"
```

---

# 92. 키보드 Down

```fk
keyboard = down ("A")
```

키가 눌린 상태를 검사한다.

---

# 93. 키보드 Up

```fk
keyboard = up ("A")
```

키가 올라온 상태를 검사한다.

---

# 94. 키보드 Hold

```fk
keyboard = hold ("A")
```

키를 누르고 있는 동안의 입력을 검사한다.

---

# 95. 실시간 입력 루프

```fk
[
    if keyboard = hold ("W") {
        prinf("UP")
    }

    if keyboard = hold ("S") {
        prinf("DOWN")
    }
](100000)
```

---

# 96. Audio

FK v2.0에서는 오디오 파일을 다룰 수 있다.

파일:

```fk
audio (C:\User\Drive\FK\music.wav) Make
```

---

# 97. Audio 재생

```fk
audio play
```

---

# 98. Audio 제어

정지:

```fk
audio stop
```

일시정지:

```fk
audio pause
```

볼륨:

```fk
audio volume > 50
```

효과음:

```fk
audio (C:\User\Drive\FK\click.wav) effect
```

---

# 99. FK 전체 핵심 문법

```text
=                  값 연결
sell               Sell
make               Sell 생성
index()            Index 접근
>                  Index 값 저장
if                 조건
else               조건의 반대 블록
{}                 코드 블록
[]()               반복
prinf()            출력
~                  범위
//;;##             주석

computer()         컴퓨터 파일 경로
Make               파일 저장/생성
to cover up        파일 덮어쓰기
Kill               파일 삭제

https://           API 통신

gui                GUI
window             GUI 창
text               텍스트
button             버튼
rect               사각형
image              이미지
Show               표시

mouse              마우스
position           위치
x                  X 좌표
y                  Y 좌표
left               왼쪽 버튼
right              오른쪽 버튼
middle             가운데 버튼

keyboard            키보드
down               키 눌림
up                 키 해제
hold               누르는 동안

audio              오디오
play               재생
stop               정지
pause              일시정지
effect             효과음
volume             음량
```

---

# 100. FK v2.0을 한 문장으로 설명하기

> **FK는 적은 문법으로 프로그램을 만들 수 있지만, 그 적은 문법을 극단적으로 조합하여 복잡하고 이상하며 난해한 프로그램을 만드는 것을 목표로 하는 프로그래밍 언어다.**

FK의 핵심은 다음과 같다.

```text
문법은 적다.
기본 개념도 적다.

하지만 조합할 수 있다.

Sell을 만든다.
Index를 사용한다.
조건을 만든다.
반복한다.
값을 중첩한다.

파일과 통신한다.
GUI를 만든다.
마우스를 읽는다.
키보드를 읽는다.
오디오를 다룬다.

그리고 프로그램이 커진다.

코드가 이상해진다.

그것이 FK다.
```

## FK의 대표적인 난해 코드

```fk
//;;## FK example

maze = sell make 1~200

player = sell make 1~5
player = index(1) > 2
player = index(2) > 3

[
    current = maze = sell index(player = sell index(1))

    if current == 0 {
        player = index(1) > player = sell index(1) + 1
    }

    prinf(current)
](100)
```

이 코드가 읽기 어렵게 느껴진다면 **FK가 의도한 방향으로 제대로 작동하고 있는 것**이다.

FK는 반드시 가장 좋은 언어일 필요가 없다.

FK는 **가장 이상한 언어 중 하나가 되는 것**을 목표로 한다.
