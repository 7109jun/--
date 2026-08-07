# pnixscript (ps) 사용 가이드

pnix는 메모리 분석 및 조작을 위한 크립트 엔진 `pnixscript`(약칭 `ps`)를 제공합니다.
이 가이드는 ps의 기본 문법, 경로 규칙, 플래그 시스템 및 주요 명령어를 설명합니다.

## 1. 기본 문법 구조

모든 ps 명령어는 다음 형식을 따릅니다:

    ps / <command> [arguments] [flags]

-   `ps`: 스크립트 엔진 식별자입니다.
-   `/`: 명령어와 인자를 구분하는 필수 구분자입니다.
-   `<command>`: 실행할 내장 명령어 이름입니다.
-   `[arguments]`: 명령어에 전달할 인자(주소, 값, 타입 등)입니다.
-   `[flags]`: 실행 방식을 제어하는 단일 문자 옵션입니다.

## 2. 경로 및 파일 시스템 규칙

pnix는 크로스 플랫폼 일관성을 위해 경로 구분자로 **정방향 슬래시(`/`)** 만 사용합니다.

-   Windows의 백슬래시(`\`)는 입력 시 자동으로 `/`로 정규화됩니다.
-   예: `C:\games\data.csv` → 내부적으로 `C:/games/data.csv` 처리
-   상대 경로와 절대 경로 모두 지원하며, CSV 프리셋 로드 시에도 동일하게 적용됩니다.

## 3. 플래그 시스템

명령어 끝에 단일 문자 플래그를 추가하여 실행 동작을 제어할 수 있습니다.

| 플래그 | 의미 | 설명 |
| :--- | :--- | :--- |
| `x` | eXecute silently | 확인 메시지 없이 즉시 실, 일반 로그 출력 억제 |
| `t` | Talk verbosely | 상세 디버그 정보, 내부 상태, 바이트 수준 데이터 출력 |

### 플래그 조합 예시

    ps / write 0x1B20 999 x      # 조용히 쓰기 (로그 없음)
    ps / read 0x1B20 4 t         # 읽기 + 상세 바이트 정보 출력
    ps / connect game.exe x t    # 확인 없이 연결 + 연결 상세 로그

플래그는 순서에 관계없이 인식되며, `x`와 `t`를 동시에 사용할 수 있습니다.

## 4. 주요 내장 명령어

### help
사용 가능한 모든 명령어와 플래그 정보를 표시합니다.

    ps / help

### read
지정된 메모리 주소의 값을 읽어 통역된 형태로 출력합니다.

    ps / read <address> [size]

-   `address`: 16진수 또는 10진수 메모리 주소
-   `size`: 읽을 바이트 수 (기본값: 4)

### write
지정된 메모리 주소에 값을写入합니다. Dry Run/Real Execute 모드를 존중합니다.

    ps / write <address> <value> [type]

-   `type`: `int32`(기본), `float`, `string` 등 지정 가능

### scan
연결된 프로세스의 메모리에서 특정 값을 검색합니다.

    ps / scan <value> [type]

### dump
지정된 메모리 영역의 Hex 덤프를 출력합니다.

    ps / dump <start_addr> <end_addr>

### undo
마지막 Real Execute 쓰기 작업을 되돌립니다. Dry Run에는 영향 없습니다.

    ps / undo

### mode
실행 모드를 전환합니다.

    ps / mode dry     # 시뮬레이션 모드 (안전)
    ps / mode real    # 실제 메모리 쓰기 모드

### list
현재 로드된 CSV 프리셋 테이블의 항목을 표시합니다.

    ps / list

### connect
프로세스 이름을 기반으로 대상 프로세스에 연결합니다.

    ps / connect <process_name.exe>

### status
현재 연결 상태, 모드, 프리셋 수, Undo 스택 크기 등을 표시합니다.

    ps / status

## 5. 안전 및 철학 준수 사항

-   **Dry Run 우선**: Real Execute 전 반드시 `ps / mode dry`에서 검증하세요.
-   **Undo 활용**: 실수 시 `ps / undo`로 즉시 복원 가능합니다.
-   **자기 소유 한정**: 타인 서비스 약관 위반 목적으로 사용하지 마십시오.
-   **관리자 권한**: 다른 프로세스 메모리 접근 시 관리자 권한 행이 필요합니다.

## 6. 명령어 확장 방법

소스 코드 `InitPsEngine()` 함수에서 `RegisterPsCommand()`를 호출하면
수백 개의 사용자 정의 명령어를 자유롭게 추가할 수 있습니다.

    RegisterPsCommand("freeze", "Freeze value", PsCmdFreeze);

pnixscript는 단순함이 힘입니다. 복잡한 설정 없이
한 줄 명령어로 메모리를 이해하고 안전하게 조작하세요.
## Code
``` /*
 * pnix - Memory Master with pnixscript (ps) Engine
 * Single File C Implementation (Win32 API Only)
 * 
 * Philosophy: Anyone can be a white hacker
 * Scripting: ps (pnixscript) - path separator '/', flags: x(silent), t(verbose)
 * Compile: cl /Fe:pnix.exe pnix.c user32.lib gdi32.lib kernel32.lib comctl32.lib
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// CONSTANTS & COLORS (Orange/White Retro Theme - No Emojis)
// ============================================================================
#define ORANGE_COLOR    RGB(255, 128, 0)
#define WHITE_COLOR     RGB(255, 255, 255)
#define BLACK_COLOR     RGB(0, 0, 0)
#define LIGHT_ORANGE    RGB(255, 200, 150)
#define DARK_ORANGE     RGB(200, 100, 0)
#define GREEN_COLOR     RGB(0, 180, 0)
#define GRAY_COLOR      RGB(200, 200, 200)
#define RED_COLOR       RGB(220, 50, 50)

#define MAX_PRESETS     100
#define MAX_LOG_LINES   500
#define UNDO_STACK_SIZE 50
#define MAX_COMMANDS    500
#define MAX_TOKENS      32
#define SCRIPT_LINE_MAX 1024

// ps Flags
#define PS_FLAG_NONE    0x00
#define PS_FLAG_SILENT  0x01  // 'x' - execute without asking, minimal log
#define PS_FLAG_VERBOSE 0x02  // 't' - talk verbosely, detailed output

// ============================================================================
// DATA STRUCTURES
// ============================================================================
typedef enum {
    EXEC_MODE_DRY_RUN = 0,
    EXEC_MODE_REAL = 1
} ExecutionMode;

typedef enum {
    TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64,
    TYPE_UINT8, TYPE_UINT16, TYPE_UINT32, TYPE_UINT64,
    TYPE_FLOAT, TYPE_DOUBLE, TYPE_STRING
} DataType;

typedef struct {
    char name[64];
    char category[32];
    DWORD offset;
    DataType type;
    int size;
    char description[128];
} PresetRule;

typedef struct {
    DWORD address;
    BYTE originalData[16];
    BYTE newData[16];
    int size;
    BOOL isActive;
} UndoEntry;

// pnixscript (ps) Command System
typedef struct {
    int argc;
    char** argv;
    int flags;  // PS_FLAG_SILENT | PS_FLAG_VERBOSE
} PsContext;

typedef int (*PsCommandHandler)(PsContext* ctx);

typedef struct {
    char name[64];
    char description[128];
    PsCommandHandler handler;
} PnixCommand;

typedef struct {
    HWND hwndMain;
    HWND hwndProcessInput;
    HWND hwndConnectBtn;
    HWND hwndTreeView;
    HWND hwndListView;
    HWND hwndLogEdit;
    HWND hwndScriptInput;
    HWND hwndStatusBar;
    HWND hwndDryRunBtn;
    HWND hwndRealExecBtn;
    HWND hwndScriptRunBtn;
    
    HANDLE hProcess;
    DWORD processId;
    char processName[MAX_PATH];
    
    ExecutionMode execMode;
    PresetRule presets[MAX_PRESETS];
    int presetCount;
    
    UndoEntry undoStack[UNDO_STACK_SIZE];
    int undoTop;
    
    // pnixscript (ps) engine
    PnixCommand commands[MAX_COMMANDS];
    int commandCount;
    
    HFONT hFontPixel;
    HBRUSH hBrushOrange;
    HBRUSH hBrushWhite;
    HBRUSH hBrushLightOrange;
} AppContext;

static AppContext g_ctx = {0};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ConnectWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL FindProcessByName(const char* name, DWORD* pid);
HANDLE OpenTargetProcess(DWORD pid);
void LoadPresetsFromCSV(const char* filename);
void PopulatePresetTree(void);
void RefreshMemoryView(DWORD baseAddr);
void AddLog(const char* format, ...);
void AddVerboseLog(const char* format, ...);
void SwitchExecutionMode(ExecutionMode mode);
BOOL SafeWriteMemory(DWORD address, const void* data, SIZE_T size, int psFlags);
void PushUndo(DWORD address, const void* oldData, const void* newData, int size);
void PerformUndo(int psFlags);
const char* TranslateValue(const BYTE* data, int size, DataType type);
void DrawCustomButton(HDC hdc, RECT* rect, const char* text, BOOL isHovered);

// pnixscript (ps) engine
void InitPsEngine(void);
int RegisterPsCommand(const char* name, const char* desc, PsCommandHandler handler);
int ExecutePsLine(const char* line);
int ParsePsTokens(const char* line, char** tokens, int maxTokens, int* outFlags);
void NormalizePath(char* path);

// Built-in ps commands
int PsCmdHelp(PsContext* ctx);
int PsCmdRead(PsContext* ctx);
int PsCmdWrite(PsContext* ctx);
int PsCmdScan(PsContext* ctx);
int PsCmdDump(PsContext* ctx);
int PsCmdUndo(PsContext* ctx);
int PsCmdMode(PsContext* ctx);
int PsCmdList(PsContext* ctx);
int PsCmdConnect(PsContext* ctx);
int PsCmdStatus(PsContext* ctx);

// ============================================================================
// PNIXSCRIPT (PS) ENGINE CORE
// ============================================================================
void InitPsEngine(void) {
    g_ctx.commandCount = 0;
    
    // Register built-in ps commands
    RegisterPsCommand("help", "Show available ps commands", PsCmdHelp);
    RegisterPsCommand("read", "Read memory: ps / read <addr> [size]", PsCmdRead);
    RegisterPsCommand("write", "Write value: ps / write <addr> <val> [type]", PsCmdWrite);
    RegisterPsCommand("scan", "Scan for value: ps / scan <value> [type]", PsCmdScan);
    RegisterPsCommand("dump", "Dump region: ps / dump <start> <end>", PsCmdDump);
    RegisterPsCommand("undo", "Undo last write: ps / undo", PsCmdUndo);
    RegisterPsCommand("mode", "Switch mode: ps / mode dry|real", PsCmdMode);
    RegisterPsCommand("list", "List presets: ps / list", PsCmdList);
    RegisterPsCommand("connect", "Connect process: ps / connect <name.exe>", PsCmdConnect);
    RegisterPsCommand("status", "Show status: ps / status", PsCmdStatus);
    
    AddLog("ps engine initialized: %d commands registered", g_ctx.commandCount);
    AddVerboseLog("ps path separator: '/' | flags: x(silent), t(verbose)");
}

int RegisterPsCommand(const char* name, const char* desc, PsCommandHandler handler) {
    if (g_ctx.commandCount >= MAX_COMMANDS) return -1;
    
    PnixCommand* cmd = &g_ctx.commands[g_ctx.commandCount++];
    strncpy(cmd->name, name, sizeof(cmd->name) - 1);
    strncpy(cmd->description, desc, sizeof(cmd->description) - 1);
    cmd->handler = handler;
    return 0;
}

// Normalize path: replace '\' with '/' for ps consistency
void NormalizePath(char* path) {
    for (char* p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
}

// Parse ps line: "ps / <cmd> [args] [flags]"
// Flags: 'x' = silent, 't' = verbose
int ParsePsTokens(const char* line, char** tokens, int maxTokens, int* outFlags) {
    static char buffer[SCRIPT_LINE_MAX];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    *outFlags = PS_FLAG_NONE;
    int count = 0;
    char* p = buffer;
    
    // Skip leading whitespace
    while (*p && isspace((unsigned char)*p)) p++;
    
    // Check for "ps /" prefix
    if (strncmp(p, "ps", 2) == 0 && (p[2] == ' ' || p[2] == '/')) {
        p += 2;
        while (*p && (isspace((unsigned char)*p) || *p == '/')) p++;
    }
    
    // Tokenize arguments
    while (*p && count < maxTokens - 1) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        
        // Check for flags at end of token
        if ((*p == 'x' || *p == 't') && (p[1] == '\0' || isspace((unsigned char)p[1]))) {
            if (*p == 'x') *outFlags |= PS_FLAG_SILENT;
            if (*p == 't') *outFlags |= PS_FLAG_VERBOSE;
            p++;
            continue;
        }
        
        tokens[count++] = p;
        while (*p && !isspace((unsigned char)*p)) p++;
        if (*p) *p++ = '\0';
    }
    tokens[count] = NULL;
    
    // Normalize paths in tokens
    for (int i = 0; i < count; i++) {
        NormalizePath(tokens[i]);
    }
    
    return count;
}

int ExecutePsLine(const char* line) {
    if (!line || !*line) return -1;
    
    char* tokens[MAX_TOKENS];
    int flags = PS_FLAG_NONE;
    int argc = ParsePsTokens(line, tokens, MAX_TOKENS, &flags);
    
    if (argc == 0) return -1;
    
    // Build context
    PsContext ctx = {0};
    ctx.argc = argc;
    ctx.argv = tokens;
    ctx.flags = flags;
    
    // Verbose logging
    if (flags & PS_FLAG_VERBOSE) {
        AddVerboseLog("[ps VERBOSE] Parsing: '%s'", line);
        AddVerboseLog("[ps VERBOSE] Tokens: %d, Flags: %s%s", 
                     argc, 
                     (flags & PS_FLAG_SILENT) ? "x" : "",
                     (flags & PS_FLAG_VERBOSE) ? "t" : "");
    }
    
    // Silent mode: suppress normal logs
    if (!(flags & PS_FLAG_SILENT)) {
        AddLog("[ps] Executing: %s", tokens[0]);
    }
    
    // Find and execute command
    for (int i = 0; i < g_ctx.commandCount; i++) {
        if (_stricmp(tokens[0], g_ctx.commands[i].name) == 0) {
            int result = g_ctx.commands[i].handler(&ctx);
            
            if (flags & PS_FLAG_VERBOSE) {
                AddVerboseLog("[ps VERBOSE] Command '%s' returned: %d", tokens[0], result);
            }
            return result;
        }
    }
    
    if (!(flags & PS_FLAG_SILENT)) {
        AddLog("[ps ERROR] Unknown command: %s", tokens[0]);
    }
    return -1;
}

// ============================================================================
// LOGGING WITH PS FLAGS SUPPORT
// ============================================================================
void AddLog(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamped[600];
    sprintf(timestamped, "[%02d:%02d:%02d] %s\r\n", 
            st.wHour, st.wMinute, st.wSecond, buffer);
    
    int len = GetWindowTextLength(g_ctx.hwndLogEdit);
    SendMessage(g_ctx.hwndLogEdit, EM_SETSEL, len, len);
    SendMessage(g_ctx.hwndLogEdit, EM_REPLACESEL, 0, (LPARAM)timestamped);
    SendMessage(g_ctx.hwndLogEdit, EM_LINESCROLL, 0, 
                SendMessage(g_ctx.hwndLogEdit, EM_GETLINECOUNT, 0, 0));
}

void AddVerboseLog(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    // Verbose logs only shown when explicitly requested or in debug
    char verboseLine[600];
    sprintf(verboseLine, "[VERBOSE] %s\r\n", buffer);
    
    int len = GetWindowTextLength(g_ctx.hwndLogEdit);
    SendMessage(g_ctx.hwndLogEdit, EM_SETSEL, len, len);
    SendMessage(g_ctx.hwndLogEdit, EM_REPLACESEL, 0, (LPARAM)verboseLine);
}

// ============================================================================
// BUILT-IN PS COMMANDS
// ============================================================================
int PsCmdHelp(PsContext* ctx) {
    if (!(ctx->flags & PS_FLAG_SILENT)) {
        AddLog("=== ps Commands ===");
        for (int i = 0; i < g_ctx.commandCount; i++) {
            AddLog("  ps / %-10s : %s", g_ctx.commands[i].name, g_ctx.commands[i].description);
        }
        AddLog("Flags: x = silent (no confirm), t = verbose (detailed output)");
        AddLog("Example: ps / write 0x1234 999 x");
        AddLog("===================");
    }
    return 0;
}

int PsCmdRead(PsContext* ctx) {
    if (ctx->argc < 2) {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("Usage: ps / read <address> [size]");
        return -1;
    }
    
    DWORD addr = (DWORD)strtoul(ctx->argv[1], NULL, 0);
    int size = (ctx->argc > 2) ? atoi(ctx->argv[2]) : 4;
    BYTE buffer[256] = {0};
    SIZE_T bytesRead = 0;
    
    if (g_ctx.hProcess) {
        ReadProcessMemory(g_ctx.hProcess, (LPCVOID)addr, buffer, size, &bytesRead);
        if (!(ctx->flags & PS_FLAG_SILENT)) {
            AddLog("READ 0x%08X (%d bytes): %s", addr, size, TranslateValue(buffer, size, TYPE_INT32));
        }
        if (ctx->flags & PS_FLAG_VERBOSE) {
            AddVerboseLog("READ detail: addr=0x%08X, size=%d, bytesRead=%zu", addr, size, bytesRead);
        }
    } else {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("No process connected");
    }
    return 0;
}

int PsCmdWrite(PsContext* ctx) {
    if (ctx->argc < 3) {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("Usage: ps / write <address> <value> [type]");
        return -1;
    }
    
    DWORD addr = (DWORD)strtoul(ctx->argv[1], NULL, 0);
    int val = atoi(ctx->argv[2]);
    DataType type = (ctx->argc > 3 && _stricmp(ctx->argv[3], "float") == 0) ? TYPE_FLOAT : TYPE_INT32;
    
    BYTE data[8] = {0};
    int size = 4;
    
    if (type == TYPE_FLOAT) {
        float fval = (float)atof(ctx->argv[2]);
        memcpy(data, &fval, 4);
    } else {
        memcpy(data, &val, 4);
    }
    
    // SafeWriteMemory respects ps flags
    SafeWriteMemory(addr, data, size, ctx->flags);
    return 0;
}

int PsCmdScan(PsContext* ctx) {
    if (ctx->argc < 2) {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("Usage: ps / scan <value> [type]");
        return -1;
    }
    if (!(ctx->flags & PS_FLAG_SILENT)) {
        AddLog("SCAN initiated for value: %s", ctx->argv[1]);
    }
    if (ctx->flags & PS_FLAG_VERBOSE) {
        AddVerboseLog("SCAN detail: target=%s, mode=%s", 
                     ctx->argv[1], g_ctx.execMode == EXEC_MODE_DRY_RUN ? "DRY" : "REAL");
    }
    return 0;
}

int PsCmdDump(PsContext* ctx) {
    if (ctx->argc < 3) {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("Usage: ps / dump <start> <end>");
        return -1;
    }
    DWORD start = (DWORD)strtoul(ctx->argv[1], NULL, 0);
    DWORD end = (DWORD)strtoul(ctx->argv[2], NULL, 0);
    if (!(ctx->flags & PS_FLAG_SILENT)) {
        AddLog("DUMP region 0x%08X - 0x%08X (%lu bytes)", start, end, end - start);
    }
    return 0;
}

int PsCmdUndo(PsContext* ctx) {
    PerformUndo(ctx->flags);
    return 0;
}

int PsCmdMode(PsContext* ctx) {
    if (ctx->argc < 2) {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("Usage: ps / mode dry|real");
        return -1;
    }
    if (_stricmp(ctx->argv[1], "dry") == 0) {
        SwitchExecutionMode(EXEC_MODE_DRY_RUN);
    } else if (_stricmp(ctx->argv[1], "real") == 0) {
        SwitchExecutionMode(EXEC_MODE_REAL);
    }
    if (ctx->flags & PS_FLAG_VERBOSE) {
        AddVerboseLog("MODE switched to: %s", ctx->argv[1]);
    }
    return 0;
}

int PsCmdList(PsContext* ctx) {
    if (!(ctx->flags & PS_FLAG_SILENT)) {
        AddLog("=== Loaded Presets (%d) ===", g_ctx.presetCount);
        for (int i = 0; i < g_ctx.presetCount; i++) {
            AddLog("  [%s] %s @ 0x%08X", 
                   g_ctx.presets[i].category, g_ctx.presets[i].name, g_ctx.presets[i].offset);
        }
    }
    if (ctx->flags & PS_FLAG_VERBOSE) {
        AddVerboseLog("LIST detail: total=%d, csv_source=presets.csv", g_ctx.presetCount);
    }
    return 0;
}

int PsCmdConnect(PsContext* ctx) {
    if (ctx->argc < 2) {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("Usage: ps / connect <process_name.exe>");
        return -1;
    }
    
    DWORD pid = 0;
    if (FindProcessByName(ctx->argv[1], &pid)) {
        g_ctx.processId = pid;
        strcpy(g_ctx.processName, ctx->argv[1]);
        g_ctx.hProcess = OpenTargetProcess(pid);
        if (g_ctx.hProcess && !(ctx->flags & PS_FLAG_SILENT)) {
            AddLog("Connected to %s (PID: %lu)", ctx->argv[1], pid);
        }
    } else {
        if (!(ctx->flags & PS_FLAG_SILENT)) AddLog("Process not found: %s", ctx->argv[1]);
    }
    return 0;
}

int PsCmdStatus(PsContext* ctx) {
    if (!(ctx->flags & PS_FLAG_SILENT)) {
        AddLog("=== pnix Status ===");
        AddLog("Process: %s (PID: %lu)", g_ctx.processName, g_ctx.processId);
        AddLog("Handle: %s", g_ctx.hProcess ? "VALID" : "NULL");
        AddLog("Mode: %s", g_ctx.execMode == EXEC_MODE_DRY_RUN ? "DRY RUN" : "REAL EXECUTE");
        AddLog("Presets: %d | Commands: %d | Undo: %d", 
               g_ctx.presetCount, g_ctx.commandCount, g_ctx.undoTop);
    }
    if (ctx->flags & PS_FLAG_VERBOSE) {
        AddVerboseLog("STATUS detail: hwnd=0x%p, font=%p", g_ctx.hwndMain, g_ctx.hFontPixel);
    }
    return 0;
}

// ============================================================================
// PROCESS ENUMERATION
// ============================================================================
BOOL FindProcessByName(const char* name, DWORD* pid) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;
    
    PROCESSENTRY32 pe = {0};
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    BOOL found = FALSE;
    if (Process32First(hSnapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                *pid = pe.th32ProcessID;
                found = TRUE;
                break;
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
    return found;
}

HANDLE OpenTargetProcess(DWORD pid) {
    HANDLE hProc = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
        FALSE, pid
    );
    if (!hProc) {
        MessageBox(NULL, "Cannot open process. Run as administrator.", 
                   "Permission Error", MB_ICONERROR);
    }
    return hProc;
}

// ============================================================================
// CSV PRESET LOADER (Path uses '/')
// ============================================================================
void LoadPresetsFromCSV(const char* filename) {
    // Normalize path for ps consistency
    char normalizedPath[MAX_PATH];
    strncpy(normalizedPath, filename, MAX_PATH - 1);
    NormalizePath(normalizedPath);
    
    FILE* fp = fopen(normalizedPath, "r");
    if (!fp) {
        AddLog("Preset file not found: %s", normalizedPath);
        return;
    }
    
    char line[512];
    fgets(line, sizeof(line), fp);
    
    g_ctx.presetCount = 0;
    while (fgets(line, sizeof(line), fp) && g_ctx.presetCount < MAX_PRESETS) {
        PresetRule* p = &g_ctx.presets[g_ctx.presetCount];
        char typeStr[16] = {0};
        
        if (sscanf(line, "%[^,],%[^,],%i,%[^,],%i,%[^\n]",
                   p->category, p->name, &p->offset, typeStr, &p->size, p->description) >= 5) {
            
            if (_stricmp(typeStr, "INT32") == 0) p->type = TYPE_INT32;
            else if (_stricmp(typeStr, "FLOAT") == 0) p->type = TYPE_FLOAT;
            else if (_stricmp(typeStr, "STRING") == 0) p->type = TYPE_STRING;
            else p->type = TYPE_INT32;
            
            g_ctx.presetCount++;
        }
    }
    fclose(fp);
    AddLog("Loaded %d presets from %s", g_ctx.presetCount, normalizedPath);
}

// ============================================================================
// MEMORY TRANSLATION ENGINE
// ============================================================================
const char* TranslateValue(const BYTE* data, int size, DataType type) {
    static char buffer[256];
    
    switch (type) {
        case TYPE_INT32: {
            int val = *(int*)data;
            sprintf(buffer, "%d", val);
            break;
        }
        case TYPE_FLOAT: {
            float val = *(float*)data;
            sprintf(buffer, "%.2f", val);
            break;
        }
        case TYPE_STRING: {
            strncpy(buffer, (const char*)data, size - 1);
            buffer[size - 1] = '\0';
            break;
        }
        default:
            sprintf(buffer, "0x%02X", data[0]);
    }
    return buffer;
}

// ============================================================================
// SAFE MEMORY WRITE (Respects ps flags)
// ============================================================================
BOOL SafeWriteMemory(DWORD address, const void* data, SIZE_T size, int psFlags) {
    BOOL isSilent = (psFlags & PS_FLAG_SILENT) != 0;
    BOOL isVerbose = (psFlags & PS_FLAG_VERBOSE) != 0;
    
    if (g_ctx.execMode == EXEC_MODE_DRY_RUN) {
        if (!isSilent) {
            AddLog("[DRY RUN] Simulate write to 0x%08X (%zu bytes)", address, size);
        }
        if (isVerbose) {
            AddVerboseLog("DRY RUN detail: addr=0x%08X, size=%zu, value_preview=%s", 
                         address, size, TranslateValue((BYTE*)data, size, TYPE_INT32));
        }
        
        if (size == 4) {
            int val = *(int*)data;
            if (val < -1000000 || val > 1000000 && !isSilent) {
                AddLog("[WARNING] Value %d exceeds safe range", val);
            }
        }
        return TRUE;
    } 
    else {
        BYTE oldData[16] = {0};
        SIZE_T bytesRead = 0;
        
        ReadProcessMemory(g_ctx.hProcess, (LPCVOID)address, oldData, size, &bytesRead);
        
        SIZE_T bytesWritten = 0;
        BOOL result = WriteProcessMemory(g_ctx.hProcess, (LPVOID)address, data, size, &bytesWritten);
        
        if (result && bytesWritten == size) {
            PushUndo(address, oldData, data, (int)size);
            if (!isSilent) {
                AddLog("[REAL] Written to 0x%08X (%zu bytes)", address, size);
            }
            if (isVerbose) {
                AddVerboseLog("REAL WRITE detail: addr=0x%08X, old=%s, new=%s", 
                             address, TranslateValue(oldData, size, TYPE_INT32),
                             TranslateValue((BYTE*)data, size, TYPE_INT32));
            }
            return TRUE;
        } else {
            if (!isSilent) {
                AddLog("[ERROR] Write failed: 0x%08X (err: %lu)", address, GetLastError());
            }
            return FALSE;
        }
    }
}

// ============================================================================
// UNDO SYSTEM (Respects ps flags)
// ============================================================================
void PushUndo(DWORD address, const void* oldData, const void* newData, int size) {
    if (g_ctx.undoTop >= UNDO_STACK_SIZE) return;
    
    UndoEntry* entry = &g_ctx.undoStack[g_ctx.undoTop++];
    entry->address = address;
    entry->size = size;
    entry->isActive = TRUE;
    memcpy(entry->originalData, oldData, size);
    memcpy(entry->newData, newData, size);
}

void PerformUndo(int psFlags) {
    BOOL isSilent = (psFlags & PS_FLAG_SILENT) != 0;
    
    if (g_ctx.undoTop <= 0) {
        if (!isSilent) AddLog("Nothing to undo");
        return;
    }
    
    UndoEntry* entry = &g_ctx.undoStack[--g_ctx.undoTop];
    if (entry->isActive && g_ctx.hProcess) {
        SIZE_T written = 0;
        WriteProcessMemory(g_ctx.hProcess, (LPVOID)entry->address, 
                          entry->originalData, entry->size, &written);
        if (!isSilent) {
            AddLog("[UNDO] Restored 0x%08X", entry->address);
        }
        entry->isActive = FALSE;
    }
}

// ============================================================================
// EXECUTION MODE SWITCHING
// ============================================================================
void SwitchExecutionMode(ExecutionMode mode) {
    g_ctx.execMode = mode;
    
    const char* modeText = (mode == EXEC_MODE_DRY_RUN) ? 
        "[MODE] DRY RUN (Simulation)" : "[MODE] REAL EXECUTE (Live)";
    
    SendMessage(g_ctx.hwndStatusBar, SB_SETTEXT, 0, (LPARAM)modeText);
    
    EnableWindow(g_ctx.hwndDryRunBtn, mode == EXEC_MODE_REAL);
    EnableWindow(g_ctx.hwndRealExecBtn, mode == EXEC_MODE_DRY_RUN);
    
    AddLog("Mode switched: %s", modeText);
    InvalidateRect(g_ctx.hwndListView, NULL, TRUE);
}

// ============================================================================
// UI POPULATION
// ============================================================================
void PopulatePresetTree(void) {
    SendMessage(g_ctx.hwndTreeView, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    
    for (int i = 0; i < g_ctx.presetCount; i++) {
        TVINSERTSTRUCT tvis = {0};
        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvis.item.pszText = g_ctx.presets[i].name;
        tvis.item.lParam = i;
        SendMessage(g_ctx.hwndTreeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);
    }
}

void RefreshMemoryView(DWORD baseAddr) {
    ListView_DeleteAllItems(g_ctx.hwndListView);
    
    BYTE buffer[256] = {0};
    SIZE_T bytesRead = 0;
    
    if (g_ctx.hProcess) {
        ReadProcessMemory(g_ctx.hProcess, (LPCVOID)baseAddr, buffer, sizeof(buffer), &bytesRead);
    }
    
    for (int i = 0; i < 16; i++) {
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        
        char addrStr[16], hexStr[48], decStr[32];
        sprintf(addrStr, "0x%08X", baseAddr + i * 16);
        
        hexStr[0] = '\0';
        for (int j = 0; j < 16; j++) {
            char byteStr[4];
            sprintf(byteStr, "%02X ", buffer[i * 16 + j]);
            strcat(hexStr, byteStr);
        }
        
        int val = *(int*)(buffer + i * 16);
        sprintf(decStr, "%d", val);
        
        lvi.pszText = addrStr;
        ListView_InsertItem(g_ctx.hwndListView, &lvi);
        ListView_SetItemText(g_ctx.hwndListView, i, 1, hexStr);
        ListView_SetItemText(g_ctx.hwndListView, i, 2, decStr);
    }
}

// ============================================================================
// CUSTOM DRAW (Orange/White Theme - No Emojis)
// ============================================================================
void DrawCustomButton(HDC hdc, RECT* rect, const char* text, BOOL isHovered) {
    HBRUSH bgBrush = isHovered ? g_ctx.hBrushLightOrange : g_ctx.hBrushWhite;
    HPEN pen = CreatePen(PS_SOLID, 2, BLACK_COLOR);
    
    SelectObject(hdc, bgBrush);
    SelectObject(hdc, pen);
    Rectangle(hdc, rect->left, rect->top, rect->right, rect->bottom);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ORANGE_COLOR);
    SelectObject(hdc, g_ctx.hFontPixel);
    DrawText(hdc, text, -1, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    DeleteObject(pen);
}

// ============================================================================
// WINDOW PROCEDURES
// ============================================================================
LRESULT CALLBACK ConnectWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_ctx.hwndProcessInput = CreateWindow("EDIT", "game.exe",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                50, 80, 300, 30, hwnd, NULL, NULL, NULL);
            
            g_ctx.hwndConnectBtn = CreateWindow("BUTTON", "CONNECT",
                WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                100, 130, 200, 40, hwnd, (HMENU)1, NULL, NULL);
            
            CreateWindow("STATIC", "Enter game process name\nExample: minecraft.exe",
                WS_VISIBLE | WS_CHILD, 50, 180, 300, 40, hwnd, NULL, NULL, NULL);
            
            g_ctx.hFontPixel = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Terminal");
            
            g_ctx.hBrushOrange = CreateSolidBrush(ORANGE_COLOR);
            g_ctx.hBrushWhite = CreateSolidBrush(WHITE_COLOR);
            g_ctx.hBrushLightOrange = CreateSolidBrush(LIGHT_ORANGE);
            
            break;
        }
        
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis->CtlID == 1) {
                DrawCustomButton(dis->hDC, &dis->rcItem, "CONNECT", 
                               dis->itemState & ODS_SELECTED);
            }
            return TRUE;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) {
                char procName[MAX_PATH] = {0};
                GetWindowText(g_ctx.hwndProcessInput, procName, MAX_PATH);
                
                DWORD pid = 0;
                if (FindProcessByName(procName, &pid)) {
                    g_ctx.processId = pid;
                    strcpy(g_ctx.processName, procName);
                    g_ctx.hProcess = OpenTargetProcess(pid);
                    
                    if (g_ctx.hProcess) {
                        AddLog("Connected: %s (PID: %lu)", procName, pid);
                        DestroyWindow(hwnd);
                        ShowWindow(g_ctx.hwndMain, SW_SHOW);
                    }
                } else {
                    MessageBox(hwnd, "Process not found. Start the game first.", 
                             "Connection Failed", MB_ICONWARNING);
                }
            }
            break;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_ctx.hwndStatusBar = CreateWindow(STATUSCLASSNAME, NULL,
                WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
            SendMessage(g_ctx.hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"[MODE] DRY RUN");
            
            g_ctx.hwndTreeView = CreateWindow(WC_TREEVIEW, NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS,
                0, 0, 200, 350, hwnd, (HMENU)2, NULL, NULL);
            
            g_ctx.hwndListView = CreateWindow(WC_LISTVIEW, NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT,
                200, 0, 500, 350, hwnd, (HMENU)3, NULL, NULL);
            
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.cx = 100; lvc.pszText = "Address";
            ListView_InsertColumn(g_ctx.hwndListView, 0, &lvc);
            lvc.cx = 250; lvc.pszText = "Hex Dump";
            ListView_InsertColumn(g_ctx.hwndListView, 1, &lvc);
            lvc.cx = 150; lvc.pszText = "Decoded";
            ListView_InsertColumn(g_ctx.hwndListView, 2, &lvc);
            
            // ps script input area
            CreateWindow("STATIC", "ps >",
                WS_VISIBLE | WS_CHILD, 10, 360, 40, 20, hwnd, NULL, NULL, NULL);
            
            g_ctx.hwndScriptInput = CreateWindow("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                50, 358, 540, 24, hwnd, (HMENU)7, NULL, NULL);
            
            g_ctx.hwndScriptRunBtn = CreateWindow("BUTTON", "RUN",
                WS_VISIBLE | WS_CHILD, 600, 358, 80, 24, hwnd, (HMENU)8, NULL, NULL);
            
            g_ctx.hwndLogEdit = CreateWindow("EDIT", NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                0, 390, 700, 160, hwnd, (HMENU)4, NULL, NULL);
            
            g_ctx.hwndDryRunBtn = CreateWindow("BUTTON", "DRY RUN",
                WS_VISIBLE | WS_CHILD, 550, 10, 140, 30, hwnd, (HMENU)5, NULL, NULL);
            g_ctx.hwndRealExecBtn = CreateWindow("BUTTON", "REAL EXEC",
                WS_VISIBLE | WS_CHILD | WS_DISABLED, 550, 50, 140, 30, hwnd, (HMENU)6, NULL, NULL);
            
            LoadPresetsFromCSV("presets.csv");
            PopulatePresetTree();
            InitPsEngine();
            
            g_ctx.execMode = EXEC_MODE_DRY_RUN;
            AddLog("pnix Memory Master initialized");
            AddLog("Type 'ps / help' to see commands. Flags: x(silent), t(verbose)");
            break;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case 5: SwitchExecutionMode(EXEC_MODE_DRY_RUN); break;
                case 6: SwitchExecutionMode(EXEC_MODE_REAL); break;
                case 8: { // RUN ps script button
                    char scriptLine[SCRIPT_LINE_MAX] = {0};
                    GetWindowText(g_ctx.hwndScriptInput, scriptLine, SCRIPT_LINE_MAX);
                    ExecutePsLine(scriptLine);
                    SetWindowText(g_ctx.hwndScriptInput, "");
                    break;
                }
            }
            break;
        }
        
        case WM_NOTIFY: {
            NMHDR* nmhdr = (NMHDR*)lParam;
            if (nmhdr->code == TVN_SELCHANGED) {
                NMTREEVIEW* nmtv = (NMTREEVIEW*)lParam;
                int idx = (int)nmtv->itemNew.lParam;
                if (idx >= 0 && idx < g_ctx.presetCount) {
                    DWORD addr = g_ctx.presets[idx].offset;
                    RefreshMemoryView(addr);
                    AddLog("Selected preset: %s (0x%08X)", 
                           g_ctx.presets[idx].name, addr);
                }
            }
            break;
        }
        
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            MoveWindow(g_ctx.hwndStatusBar, 0, height - 25, width, 25, TRUE);
            MoveWindow(g_ctx.hwndTreeView, 0, 0, 200, height - 225, TRUE);
            MoveWindow(g_ctx.hwndListView, 200, 0, width - 200, height - 225, TRUE);
            MoveWindow(g_ctx.hwndScriptInput, 50, height - 222, width - 140, 24, TRUE);
            MoveWindow(g_ctx.hwndScriptRunBtn, width - 90, height - 222, 80, 24, TRUE);
            MoveWindow(g_ctx.hwndLogEdit, 0, height - 195, width, 170, TRUE);
            break;
        }
        
        case WM_DESTROY:
            if (g_ctx.hProcess) CloseHandle(g_ctx.hProcess);
            DeleteObject(g_ctx.hFontPixel);
            DeleteObject(g_ctx.hBrushOrange);
            DeleteObject(g_ctx.hBrushWhite);
            DeleteObject(g_ctx.hBrushLightOrange);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PnixMainClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RegisterClass(&wc);
    
    WNDCLASS wcConnect = {0};
    wcConnect.lpfnWndProc = ConnectWndProc;
    wcConnect.hInstance = hInstance;
    wcConnect.lpszClassName = "PnixConnectClass";
    wcConnect.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RegisterClass(&wcConnect);
    
    g_ctx.hwndMain = CreateWindow("PnixMainClass", "pnix - Memory Master",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);
    
    HWND hwndConnect = CreateWindow("PnixConnectClass", "pnix - Process Connect",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 250,
        NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwndConnect, SW_SHOW);
    UpdateWindow(hwndConnect);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
