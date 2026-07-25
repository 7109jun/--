# Junscript Official Documentation (v1.0)

## 1. 개요 (Overview)
Junscript는 C++ 기반 브라우저 Jwan에서 실행되기 위해 설계된 프로그래밍 언어입니다. .juns 확장자를 사용하며, JIT(Just-In-Time) 컴파일 방식을 통해 소스 코드를 실시간으로 최적화된 WebAssembly(Wasm) 기계어로 변환하여 실행합니다.

- 목표: 웹 환경에서의 파이썬(Python) 대체 및 데이터 분석/AI 지원
- 특징: 
  - 백엔드 없이 브라우저(Jwan) 단독 실행
  - 자체 내장 라이브러리(Math, AI, UI) 지원
  - 가독성을 최우선으로 한 _(언더스코어) 연결 문법
  - JIT 컴파일을 통한 높은 실행 성능과 동적 최적화

---

## 2. 기본 문법 (Basic Syntax)

### 2.1 주석 (Comments)
코드에 설명을 남길 때 사용합니다. # 기호를 사용합니다.

예시:
# 이것은 한 줄 주석입니다
print("Hello") # 인라인 주석도 가능합니다

### 2.2 변수 및 리스트 정의 (Definitions)
def 키워드를 사용하여 변수와 리스트를 선언합니다. 값은 괄호 () 또는 대괄호 []로 감쌉니다.

예시:
def_Name = ("Jun")          // 문자열 변수
def_Age = (25)              // 정수 변수
def_Scores = [90, 85, 100]  // 리스트(배열)

### 2.3 변수 변경 (Variable Change)
기존 변수의 값을 변경하거나 산술 연산을 수행할 때 사용합니다.

예시:
Variable_Change_Name = ("Qwen")   // 값 덮어쓰기
Variable_Change_Age + (1)         // 1 더하기
Variable_Change_Age x (2)         // 2배 곱하기

### 2.4 리스트 조작 (List Change)
리스트의 특정 인덱스 값을 변경하거나 연산합니다.

예시:
List_Change_Scores_index(0) = (95)    // 0번 인덱스를 95로 변경
List_Change_Scores_index(1) + (5)     // 1번 인덱스 값에 5 더하기

---

## 3. 제어문 및 함수 (Control Flow & Functions)

### 3.1 조건문 (Conditionals)
if, else_if, else를 사용하여 논리적 분기를 처리합니다.

예시:
if_Age >= (20) {
    print("성인입니다")
} else_if_Age >= (13) {
    print("청소년입니다")
} else {
    print("어린이입니다")
}

### 3.2 반복문 (Loops)
for와 while을 지원합니다.

예시:
// 0부터 4까지 반복
for_i_in_range(0, 5) {
    console(i)
}

// 조건이 참일 동안 반복
while_IsActive == (true) {
    func_DoWork()
}

### 3.3 함수 정의 (Functions)
func 키워드를 사용하여 재사용 가능한 코드 블록을 정의합니다.

예시:
func_Greet(name) {
    print("안녕하세요_" + name)
}

// 호출
func_Greet("Jun")

---

## 4. 고급 기능 (Advanced Features)

### 4.1 클래스 (Classes)
객체지향 프로그래밍을 위한 클래스를 정의합니다.

예시:
class_Player {
    def_name = ("")
    
    func_Init(name) {
        this.name = name
    }
    
    func_Introduce() {
        print("저는_" + this.name + "입니다")
    }
}

def_Hero = new_Player("Jun")
func_Introduce(Hero)

### 4.2 데이터 분석 및 AI (Data & AI)
내장된 수학 및 AI 모듈을 사용하여 복잡한 계산을 수행합니다.

예시:
// 행렬 연산
def_Matrix = tensor([[1, 2], [3, 4]])
def_Result = func_matmul(Matrix, Matrix)

// AI 모델 학습
model_AI = sequential(layer_dense(10))
func_fit(model_AI, train_data, epochs=10)

### 4.3 예외 처리 (Error Handling)
예상치 못한 에러로부터 프로그램을 보호합니다.

예시:
try {
    def_Val = 10 / 0
} except_Err {
    console("에러_발생:" + Err.message)
}

---

## 5. 컴파일 및 실행 (JIT Compilation)
Junscript 코드는 Jwan JIT Engine을 통해 실시간으로 WebAssembly(Wasm) 바이너리로 변환됩니다.

1. Parsing: .juns 소스 코드를 읽어 AST(추상 구문 트리)로 변환
2. Optimization: 불필요한 연산 제거 및 메모리 최적화
3. Code Generation: 최적화된 AST를 Wasm 기계어로 생성
4. Execution: Jwan 브라우저 내에서 즉시 실행

참고: JIT 방식은 처음 실행 시 약간의 지연이 있을 수 있으나, 반복 실행 시 캐싱을 통해 매우 빠른 속도를 제공합니다.

---

## 6. 내장 라이브러리 (Built-in Libraries)
백엔드 없이 다음과 같은 기능을 내장 라이브러리로 제공합니다.
- Math: func_sin(), func_cos(), func_matmul() 등
- AI: model_sequential(), func_fit(), func_predict() 등
- UI: element_create(), viz_bar_chart() 등
- IO: file_load(), storage_set() 등
