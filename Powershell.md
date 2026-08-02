# PowerShell 밑바닥부터 시작하는 실전 완벽 가이드 (모든 핵심 명령어 총정리)

이 가이드는 파워셸을 단 한 번도 써본 적이 없는 초보자(밑바닥)의 시점에서, 현업에서 실제로 가장 많이 쓰는 모든 필수 명령어와 그 사용법을 아주 세세하게 정리한 완벽 가이드입니다. 우측 상단의 [복사 (Copy)] 버튼을 누르면 전체 내용을 한 번에 클립보드에 복사할 수 있습니다.

---

## 1. PowerShell의 탄생 배경과 만든 이

### 🛠️ 만든 이
- 파워셸(PowerShell)은 마이크로소프트(Microsoft)의 수석 건축가인 **제프리 스눅스(Jeffrey Snover)**가 주도하여 개발을 진두지휘했습니다.

### 📜 탄생 배경
- **기존 윈도우 셸의 한계**: 과거 윈도우의 전통적인 명령 프롬프트(`cmd.exe`)는 단순한 문자열 기반의 명령어 실행기였기 때문에 복잡한 시스템 관리나 자동화 작업을 처리하는 데 극심한 한계가 있었습니다.
- **리눅스와의 차이점**: 리눅스의 Bash 셸은 강력했지만 텍스트 스트림 위주로 동작하여 정형화된 데이터를 다룰 때 번거로운 파싱(Parsing) 작업이 필요했습니다.
- **객체(Object) 지향의 도입**: 마이크로소프트는 시스템 관리자가 코드를 짜듯 강력하게 윈도우 서버와 PC를 제어할 수 있도록, 텍스트가 아닌 **'.NET 개체(Object)'**를 주고받는 차세대 셸인 PowerShell을 탄생시켰습니다.

---

## 2. PowerShell 기초 조작 및 환경 설정 (가장 먼저 해야 할 일)

### 2.1 실행 정책(Execution Policy) 변경
- **밑바닥 설명**: 파워셸은 기본적으로 보안상 내가 작성한 스크립트(`.ps1`) 파일의 실행을 완전히 차단합니다. 이 설정을 풀지 않으면 아무리 좋은 코드를 짜도 실행할 수 없습니다.
- **사용법**: 윈도우 시작 버튼을 누르고 `PowerShell`을 검색한 뒤, 반드시 **[관리자 권한으로 실행]**을 누릅니다. 그 후 아래 명령어를 입력합니다.
- 명령어:
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser -Force

### 2.2 현재 실행 정책 확인
- **밑바닥 설명**: 내 파워셸에 스크립트 실행이 제대로 허용되었는지 확인하는 명령어입니다.
- 명령어:
Get-ExecutionPolicy -List

### 2.3 콘솔 화면 청소
- **밑바닥 설명**: 화면에 입력된 기록이 너무 많아 지저분할 때 화면을 깨끗하게 비웁니다.
- 명령어:
Clear-Host

### 2.4 색상이 있는 메시지 출력
- **밑바닥 설명**: 스크립트나 자동화 도중 성공, 경고, 에러 메시지를 시각적으로 구분하여 출력하고 싶을 때 사용합니다.
- 명령어:
Write-Host "작업이 성공적으로 처리되었습니다." -ForegroundColor Green

---

## 3. 파일 및 디렉터리 다루기 (완벽 실전 명령어 모음)

### 3.1 현재 폴더 위치 확인 (현재 경로 보기)
- **밑바닥 설명**: 내가 지금 파워셸 상에서 어느 폴더(디렉터리)에 들어가 있는지 확인합니다.
- 명령어:
Get-Location

### 3.2 폴더 이동하기
- **밑바닥 설명**: 다른 드라이브나 폴더로 이동할 때 사용합니다.
- 명령어:
Set-Location -Path "C:\Temp"
# 또는 줄여서
cd "C:\Temp"

### 3.3 현재 폴더 안의 파일 및 폴더 목록 보기
- **밑바닥 설명**: 리눅스의 `ls`와 같은 기능으로, 폴더 안에 어떤 파일과 폴더가 있는지 조회합니다.
- 명령어:
Get-ChildItem
# 또는 줄여서
ls
dir

### 3.4 폴더 및 파일 생성하기
- **밑바닥 설명**: 백업 폴더 구조를 코드로 자동 생성하거나 빈 파일을 만들 때 사용합니다. `-Force`를 붙이면 중간에 상위 폴더가 없어도 한 번에 만들어줍니다.
- 명령어:
New-Item -Path "C:\Temp\Logs" -ItemType Directory -Force
New-Item -Path "C:\Temp\test.txt" -ItemType File -Value "초기 내용" -Force

### 3.5 파일 복사하기
- **밑바닥 설명**: 원본 파일을 유지한 채 지정한 위치에 복사본을 만듭니다.
- 명령어:
Copy-Item -Path "C:\Temp\test.txt" -Destination "C:\Backup\test_backup.txt" -Force

### 3.6 파일 이동 및 이름 변경하기
- **밑바닥 설명**: 지정한 위치로 파일을 이동시키면서 파일명도 동시에 변경합니다.
- 명령어:
Move-Item -Path "C:\Temp\test.txt" -Destination "C:\Temp\Archive\old_test.txt" -Force

### 3.7 파일 및 폴더 삭제하기
- **밑바닥 설명**: 하위 파일이나 폴더가 가득 차 있어도 경고 없이 통째로 삭제합니다.
- 명령어:
Remove-Item -Path "C:\Temp\Logs" -Recurse -Force

### 3.8 대량 파일 확장자 일괄 변경하기
- **밑바닥 설명**: 특정 폴더 안의 모든 텍스트 파일(.txt)을 로그 파일(.log)로 한 번에 바꿀 때 쓰는 실무 자동화 명령어입니다.
- 명령어:
Get-ChildItem -Path "C:\Temp" -Filter "*.txt" | ForEach-Object { Rename-Item $_.FullName ($_.Name -replace '\.txt$', '.log') }

---

## 4. 프로세스 및 서비스 완벽 제어 명령어 모음

### 4.1 실행 중인 모든 프로세스 조회하기
- **밑바닥 설명**: 현재 컴퓨터에서 돌아가는 모든 프로그램의 목록과 PID(프로세스 번호)를 확인합니다.
- 명령어:
Get-Process

### 4.2 CPU 사용량이 높은 프로세스 상위 5개 확인하기
- **밑바닥 설명**: 서버나 PC가 갑자기 느려졌을 때 리소스를 과도하게 쓰는 범인을 파워셸 창에서 바로 찾아냅니다.
- 명령어:
Get-Process | Sort-Object CPU -Descending | Select-Object -First 5 -Property Name, CPU, WorkingSet

### 4.3 특정 프로세스 강제 종료하기
- **밑바닥 설명**: 응답하지 않는 프로그램을 이름이나 PID로 강제 종료합니다.
- 명령어:
Stop-Process -Name "notepad" -Force

### 4.4 특정 프로그램 실행하기
- **밑바닥 설명**: 파워셸을 통해 특정 응용 프로그램을 강제로 띄웁니다.
- 명령어:
Start-Process "notepad.exe"

### 4.5 윈도우 서비스 상태 확인하기
- **밑바닥 설명**: 특정 백그라운드 서비스의 현재 실행 상태(Running, Stopped 등)를 확인합니다.
- 명령어:
Get-Service -Name "W3SVC"

### 4.6 윈도우 서비스 시작, 중지, 재시작하기
- **밑바닥 설명**: 웹 서버나 데이터베이스 서비스를 코드로 제어합니다.
- 명령어:
Start-Service -Name "W3SVC"
Stop-Service -Name "W3SVC"
Restart-Service -Name "W3SVC"

---

## 5. 네트워크 및 방화벽 관리 명령어 모음

### 5.1 IP 주소 확인하기
- **밑바닥 설명**: 현재 내 컴퓨터의 사설 또는 공인 IPv4 주소와 네트워크 어댑터 이름을 확인합니다.
- 명령어:
Get-NetIPAddress -AddressFamily IPv4 | Select-Object InterfaceAlias, IPAddress

### 5.2 핑(Ping) 테스트하기
- **밑바닥 설명**: 외부 서버나 게이트웨이와 네트워크 통신이 정상적으로 연결되어 있는지 테스트합니다.
- 명령어:
Test-Connection -ComputerName "8.8.8.8" -Count 4

### 5.3 포트 열림 여부 확인하기 (TCP 포트 진단)
- **밑바닥 설명**: 원격 서버의 특정 포트(예: 원격 데스크톱 3389포트)가 방화벽에 막혀있는지 정밀 진단합니다.
- 명령어:
Test-NetConnection -ComputerName "192.168.1.100" -Port 3389

### 5.4 방화벽 인바운드 규칙 추가하기 (포트 열기)
- **밑바닥 설명**: 새로운 프로그램을 설치했는데 외부에서 접속이 안 될 때, 방화벽 포트를 코드로 즉시 열어줍니다.
- 명령어:
New-NetFirewallRule -DisplayName "Web Port 8080" -Direction Inbound -Protocol TCP -LocalPort 8080 -Action Allow

---

## 6. 레지스트리 및 환경 변수 조작 명령어 모음

### 6.1 레지스트리 값 읽어오기
- **밑바닥 설명**: 윈도우 시스템 레지스트리에 저장된 특정 키의 값을 조회합니다.
- 명령어:
Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion" -Name "ProgramFilesDir"

### 6.2 레지스트리 값 수정 및 생성하기
- **밑바닥 설명**: 프로그램 설정이나 윈도우 시스템 레지스트리를 코드로 직접 변경해야 할 때 사용합니다.
- 명령어:
Set-ItemProperty -Path "HKCU:\Software\MyCompany" -Name "SettingKey" -Value "SampleData" -Force

### 6.3 시스템 환경 변수 Path 확인하기
- **밑바닥 설명**: 현재 등록된 시스템 환경 변수 Path 경로들을 불러옵니다.
- 명령어:
[Environment]::GetEnvironmentVariable("Path", "Machine")

### 6.4 환경 변수 Path에 새 경로 추가하기
- **밑바닥 설명**: 파이썬이나 개발 도구 경로(`C:\MyTools`)를 시스템 환경 변수에 영구적으로 등록하여 어디서든 명령어를 쓸 수 있게 만듭니다.
- 명령어:
[Environment]::SetEnvironmentVariable("Path", "$([Environment]::GetEnvironmentVariable('Path', 'Machine'));C:\MyTools", "Machine")

---

## 7. 텍스트 파싱, CSV 및 JSON 데이터 가공 명령어 모음

### 7.1 텍스트 파일 읽기
- **밑바닥 설명**: 메모장 등으로 만들어진 텍스트 파일의 내용을 파워셸 창에 출력합니다.
- 명령어:
Get-Content -Path "C:\Temp\test.txt"

### 7.2 텍스트 파일에 내용 쓰기 및 이어쓰기
- **밑바닥 설명**: 파일에 새로운 내용을 덮어쓰거나 기존 내용 밑에 이어 붙입니다.
- 명령어:
Set-Content -Path "C:\Temp\test.txt" -Value "새로운 내용 덮어쓰기"
Add-Content -Path "C:\Temp\test.txt" -Value "밑에 이어쓰기"

### 7.3 CSV 파일 읽기
- **밑바닥 설명**: 대량의 사원 명부나 리포트 데이터를 불러와 반복적인 자동화 작업을 처리할 때 사용합니다.
- 명령어:
Import-Csv -Path "C:\Temp\users.csv" -Encoding UTF8

### 7.4 데이터를 CSV 파일로 내보내기
- **밑바닥 설명**: 현재 실행 중인 프로세스나 객체 리스트를 엑셀 호환 CSV 파일로 저장합니다.
- 명령어:
Get-Process | Export-Csv -Path "C:\Temp\processes.csv" -NoTypeInformation -Encoding UTF8

### 7.5 JSON 파일 읽기 및 객체 변환
- **밑바닥 설명**: JSON 포맷의 설정 파일을 파워셸 객체로 변환하여 내부 속성값을 읽습니다.
- 명령어:
Get-Content -Path "C:\Temp\config.json" -Raw | ConvertFrom-Json

### 7.6 객체를 JSON으로 변환하여 파일로 저장하기
- **밑바닥 설명**: 파워셸 내부의 데이터를 설정 파일 표준 포맷인 JSON으로 변환하여 파일로 저장합니다.
- 명령어:
@{ ServerIP = "192.168.1.50"; Port = 443 } | ConvertTo-Json | Set-Content -Path "C:\Temp\config.json" -Encoding UTF8

---

## 8. 실무형 에러 처리 및 로그 기록 스크립트 템플릿

### 8.1 스크립트 통짜 템플릿 (현업 필수)
- **밑바닥 설명**: 스크립트 실행 중 에러가 발생해도 프로그램이 튕기거나 멈추지 않고, 어떤 시간에 무슨 에러가 났는지 로그 파일에 시간별로 기록되게 만드는 현업 필수 코드입니다.
- 명령어 및 내용:
$LogFile = "C:\Temp\Execution_Log.log"
function Write-Log ($Msg) { 
    $Time = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    "[$Time] $Msg" | Add-Content $LogFile 
}
try {
    Write-Log "작업을 시작합니다."
    if (!(Test-Path "C:\Data")) { 
        throw "필수 데이터 경로를 찾을 수 없습니다." 
    }
    Write-Log "모든 작업이 정상 완료되었습니다."
} catch {
    Write-Log "치명적 에러 발생: $_"
}

---

## 9. 원격 관리 및 압축 명령어 모음

### 9.1 원격 서버 세션 연결하기
- **밑바닥 설명**: 내 자리에 앉아서 다른 원격 서버에 파워셸 세션을 맺은 뒤, 서버 안의 프로세스를 조회하거나 원격으로 명령어를 실행합니다.
- 명령어:
$Session = New-PSSession -ComputerName "192.168.1.50" -Credential (Get-Credential)

### 9.2 원격 서버에 명령어 실행하기
- **밑바닥 설명**: 맺어둔 원격 세션을 통해 대상 서버에서 특정 파워셸 명령어를 수행합니다.
- 명령어:
Invoke-Command -Session $Session -ScriptBlock { Get-Process }

### 9.3 원격 세션 종료하기
- **밑바닥 설명**: 원격 작업이 모두 끝난 후 연결된 세션을 안전하게 끊고 자원을 반환합니다.
- 명령어:
Remove-PSSession -Session $Session

### 9.4 폴더 ZIP 압축하기
- **밑바닥 설명**: 백업 폴더 전체를 하나의 압축 파일로 깔끔하게 묶을 때 별도의 외부 프로그램 설치 없이 수행합니다.
- 명령어:
Compress-Archive -Path "C:\Temp\Logs" -DestinationPath "C:\Temp\Logs_Backup.zip" -Force

### 9.5 ZIP 압축 해제하기
- **밑바닥 설명**: 압축 파일을 지정한 폴더 경로에 자동으로 풀어줍니다.
- 명령어:
Expand-Archive -Path "C:\Temp\Logs_Backup.zip" -DestinationPath "C:\Temp\UnzippedFolder" -Force
