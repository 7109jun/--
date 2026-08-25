#define _CRT_SECURE_NO_WARNINGS
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define HADA_VERSION L"0.1.0"
#define HADA_MAX_LINE 8192
#define HADA_MAX_ARGS 256
#define HADA_X_LIMIT (40ULL * 1024ULL * 1024ULL * 1024ULL)

static wchar_t g_root[MAX_PATH * 4];
static wchar_t g_sandbox[MAX_PATH * 4];
static wchar_t g_u[MAX_PATH * 4];
static wchar_t g_x[MAX_PATH * 4];

static void die_msg(const wchar_t *msg) {
    fwprintf(stderr, L"HADA: %ls\n", msg);
}

static void winerr(const wchar_t *where) {
    DWORD e = GetLastError();
    wchar_t *buf = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, e, 0, (LPWSTR)&buf, 0, NULL);
    if (buf) {
        fwprintf(stderr, L"HADA: %ls: %ls (code %lu)\n", where, buf, (unsigned long)e);
        LocalFree(buf);
    } else {
        fwprintf(stderr, L"HADA: %ls: Windows error %lu\n", where, (unsigned long)e);
    }
}

static void trim_ws(wchar_t *s) {
    if (!s) return;
    size_t n = wcslen(s);
    size_t a = 0;
    while (a < n && iswspace(s[a])) a++;
    size_t b = n;
    while (b > a && iswspace(s[b - 1])) b--;
    if (a > 0) memmove(s, s + a, (b - a) * sizeof(wchar_t));
    s[b - a] = L'\0';
}

static int ieq(const wchar_t *a, const wchar_t *b) {
    return _wcsicmp(a, b) == 0;
}

static int starts_with_i(const wchar_t *s, const wchar_t *p) {
    while (*p) {
        if (towlower(*s++) != towlower(*p++)) return 0;
    }
    return 1;
}

static void normalize_slashes(wchar_t *s) {
    for (; *s; ++s) if (*s == L'/') *s = L'\\';
}

static int ensure_dir_recursive(const wchar_t *path) {
    wchar_t buf[MAX_PATH * 8];
    if (wcslen(path) >= sizeof(buf)/sizeof(buf[0])) return 0;
    wcscpy(buf, path);
    normalize_slashes(buf);
    for (wchar_t *p = buf; *p; ++p) {
        if (*p == L'\\' && p != buf + 2) {
            wchar_t save = *p;
            *p = L'\0';
            if (wcslen(buf) > 0) CreateDirectoryW(buf, NULL);
            *p = save;
        }
    }
    if (CreateDirectoryW(buf, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) return 1;
    return 0;
}

static int path_exists(const wchar_t *p) {
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES;
}

static int is_dir(const wchar_t *p) {
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static void dirname_of(const wchar_t *path, wchar_t *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = L'\0';
    wcsncpy(out, path, cap - 1);
    out[cap - 1] = L'\0';
    wchar_t *p = wcsrchr(out, L'\\');
    if (!p) p = wcsrchr(out, L'/');
    if (p) *p = L'\0';
}

static void basename_of(const wchar_t *path, wchar_t *out, size_t cap) {
    if (!out || cap == 0) return;
    const wchar_t *p = wcsrchr(path, L'\\');
    const wchar_t *q = wcsrchr(path, L'/');
    if (q && (!p || q > p)) p = q;
    p = p ? p + 1 : path;
    wcsncpy(out, p, cap - 1);
    out[cap - 1] = L'\0';
}

static int copy_tree(const wchar_t *src, const wchar_t *dst) {
    DWORD attr = GetFileAttributesW(src);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        if (!CreateDirectoryW(dst, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            winerr(L"create directory");
            return 0;
        }
        wchar_t pat[MAX_PATH * 8];
        _snwprintf(pat, sizeof(pat)/sizeof(pat[0]), L"%ls\\*", src);
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(pat, &fd);
        if (h == INVALID_HANDLE_VALUE) return 1;
        int ok = 1;
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            wchar_t s[MAX_PATH * 8], d[MAX_PATH * 8];
            _snwprintf(s, sizeof(s)/sizeof(s[0]), L"%ls\\%ls", src, fd.cFileName);
            _snwprintf(d, sizeof(d)/sizeof(d[0]), L"%ls\\%ls", dst, fd.cFileName);
            if (!copy_tree(s, d)) ok = 0;
        } while (FindNextFileW(h, &fd));
        FindClose(h);
        return ok;
    }
    wchar_t parent[MAX_PATH * 8];
    dirname_of(dst, parent, sizeof(parent)/sizeof(parent[0]));
    if (parent[0]) ensure_dir_recursive(parent);
    if (!CopyFileW(src, dst, FALSE)) {
        winerr(L"copy");
        return 0;
    }
    return 1;
}

static int remove_tree(const wchar_t *path) {
    DWORD attr = GetFileAttributesW(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL);
        return DeleteFileW(path) != 0;
    }
    wchar_t pat[MAX_PATH * 8];
    _snwprintf(pat, sizeof(pat)/sizeof(pat[0]), L"%ls\\*", path);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            wchar_t child[MAX_PATH * 8];
            _snwprintf(child, sizeof(child)/sizeof(child[0]), L"%ls\\%ls", path, fd.cFileName);
            remove_tree(child);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL);
    return RemoveDirectoryW(path) != 0;
}

static unsigned long long dir_size(const wchar_t *path) {
    DWORD attr = GetFileAttributesW(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        WIN32_FILE_ATTRIBUTE_DATA d;
        if (GetFileAttributesExW(path, GetFileExInfoStandard, &d)) {
            ULARGE_INTEGER u;
            u.HighPart = d.nFileSizeHigh; u.LowPart = d.nFileSizeLow;
            return u.QuadPart;
        }
        return 0;
    }
    unsigned long long total = 0;
    wchar_t pat[MAX_PATH * 8];
    _snwprintf(pat, sizeof(pat)/sizeof(pat[0]), L"%ls\\*", path);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        wchar_t child[MAX_PATH * 8];
        _snwprintf(child, sizeof(child)/sizeof(child[0]), L"%ls\\%ls", path, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) total += dir_size(child);
        else {
            ULARGE_INTEGER u;
            u.HighPart = fd.nFileSizeHigh; u.LowPart = fd.nFileSizeLow;
            total += u.QuadPart;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return total;
}

static void join2(const wchar_t *a, const wchar_t *b, wchar_t *out, size_t cap) {
    if (!a || !b || !out || cap == 0) return;
    if (a[0] == 0) _snwprintf(out, cap, L"%ls", b);
    else if (b[0] == 0) _snwprintf(out, cap, L"%ls", a);
    else if (a[wcslen(a)-1] == L'\\') _snwprintf(out, cap, L"%ls%ls", a, b);
    else _snwprintf(out, cap, L"%ls\\%ls", a, b);
    out[cap - 1] = L'\0';
}

static int env_expand(const wchar_t *in, wchar_t *out, size_t cap) {
    DWORD n = ExpandEnvironmentStringsW(in, out, (DWORD)cap);
    return n != 0 && n <= cap;
}

static int resolve_path(const wchar_t *in, wchar_t *out, size_t cap) {
    if (!in || !out || cap == 0) return 0;
    wchar_t s[MAX_PATH * 8];
    wcsncpy(s, in, sizeof(s)/sizeof(s[0]) - 1);
    s[sizeof(s)/sizeof(s[0]) - 1] = L'\0';
    trim_ws(s);
    if (s[0] == L'"') {
        size_t n = wcslen(s);
        if (n >= 2 && s[n - 1] == L'"') { memmove(s, s + 1, (n - 2) * sizeof(wchar_t)); s[n-2] = L'\0'; }
    }
    wchar_t exp[MAX_PATH * 8];
    if (!env_expand(s, exp, sizeof(exp)/sizeof(exp[0]))) wcscpy(exp, s);
    if (starts_with_i(exp, L"U:\\")) {
        wchar_t rest[MAX_PATH * 8];
        wcscpy(rest, exp + 3);
        join2(g_u, rest, out, cap);
        return 1;
    }
    if (starts_with_i(exp, L"X:\\")) {
        wchar_t rest[MAX_PATH * 8];
        wcscpy(rest, exp + 3);
        join2(g_x, rest, out, cap);
        return 1;
    }
    if (starts_with_i(exp, L"sandbox/")) {
        wchar_t rest[MAX_PATH * 8];
        wcscpy(rest, exp + 9);
        join2(g_sandbox, rest, out, cap);
        return 1;
    }
    if (starts_with_i(exp, L"sandbox\\")) {
        wchar_t rest[MAX_PATH * 8];
        wcscpy(rest, exp + 9);
        join2(g_sandbox, rest, out, cap);
        return 1;
    }
    if ((iswalpha(exp[0]) && exp[1] == L':') || (exp[0] == L'\\' && exp[1] == L'\\')) {
        wcsncpy(out, exp, cap - 1); out[cap-1] = L'\0'; normalize_slashes(out); return 1;
    }
    wchar_t cwd[MAX_PATH * 8];
    DWORD n = GetCurrentDirectoryW((DWORD)(sizeof(cwd)/sizeof(cwd[0])), cwd);
    if (!n || n >= sizeof(cwd)/sizeof(cwd[0])) return 0;
    join2(cwd, exp, out, cap);
    normalize_slashes(out);
    return 1;
}

static int is_inside(const wchar_t *child, const wchar_t *root) {
    wchar_t a[MAX_PATH * 8], b[MAX_PATH * 8];
    if (!GetFullPathNameW(child, sizeof(a)/sizeof(a[0]), a, NULL)) return 0;
    if (!GetFullPathNameW(root, sizeof(b)/sizeof(b[0]), b, NULL)) return 0;
    _wcslwr(a); _wcslwr(b);
    size_t n = wcslen(b);
    if (n && b[n-1] == L'\\') return wcsncmp(a, b, n) == 0;
    return wcsncmp(a, b, n) == 0 && (a[n] == L'\0' || a[n] == L'\\');
}

static int safe_delete(const wchar_t *path) {
    if (is_inside(path, g_sandbox)) return remove_tree(path);
    die_msg(L"delete is restricted to the active sandbox");
    return 0;
}

static int x_quota_ok_for(const wchar_t *src, const wchar_t *dst) {
    if (!is_inside(dst, g_x)) return 1;
    unsigned long long add = dir_size(src);
    unsigned long long used = dir_size(g_x);
    if (used > HADA_X_LIMIT || add > HADA_X_LIMIT - used) {
        fwprintf(stderr, L"HADA: X: quota exceeded (limit 40GB).\n");
        return 0;
    }
    return 1;
}

static void print_size(unsigned long long n) {
    if (n >= 1024ULL*1024ULL*1024ULL) fwprintf(stdout, L"%llu bytes (%.2f GB)\n", n, (double)n/(1024.0*1024.0*1024.0));
    else if (n >= 1024ULL*1024ULL) fwprintf(stdout, L"%llu bytes (%.2f MB)\n", n, (double)n/(1024.0*1024.0));
    else if (n >= 1024ULL) fwprintf(stdout, L"%llu bytes (%.2f KB)\n", n, (double)n/1024.0);
    else fwprintf(stdout, L"%llu bytes\n", n);
}

static int copy_resolved(const wchar_t *src, const wchar_t *dst) {
    if (!path_exists(src)) { die_msg(L"source does not exist"); return 0; }
    if (!x_quota_ok_for(src, dst)) return 0;
    return copy_tree(src, dst);
}

static int parse_words(const wchar_t *line, wchar_t argv[][1024], int maxargs) {
    int argc = 0; size_t i = 0, n = wcslen(line);
    while (i < n && argc < maxargs) {
        while (i < n && iswspace(line[i])) i++;
        if (i >= n) break;
        size_t j = 0;
        if (line[i] == L'"') {
            i++;
            while (i < n) {
                if (line[i] == L'"') { i++; break; }
                if (line[i] == L'\\' && i + 1 < n && line[i+1] == L'"') { argv[argc][j++] = L'"'; i += 2; }
                else argv[argc][j++] = line[i++];
                if (j + 1 >= 1024) break;
            }
        } else {
            while (i < n && !iswspace(line[i])) {
                if (j + 1 < 1024) argv[argc][j++] = line[i];
                i++;
            }
        }
        argv[argc][j] = L'\0';
        argc++;
    }
    return argc;
}

static int split_top_level(const wchar_t *line, wchar_t *left, size_t lcap, wchar_t *right, size_t rcap, const wchar_t *op) {
    int quote = 0, depth = 0;
    size_t n = wcslen(line), oplen = wcslen(op);
    for (size_t i=0;i+oplen<=n;i++) {
        wchar_t c = line[i];
        if (c == L'"') quote = !quote;
        else if (!quote && c == L'(') depth++;
        else if (!quote && c == L')' && depth>0) depth--;
        if (!quote && depth==0 && wcsncmp(line+i, op, oplen)==0) {
            size_t a=i, b=i+oplen;
            while (a>0 && iswspace(line[a-1])) a--;
            while (b<n && iswspace(line[b])) b++;
            if (i >= lcap || n-b+1 >= rcap) return 0;
            wcsncpy(left, line, i); left[i] = L'\0';
            wcsncpy(right, line+b, rcap-1); right[rcap-1]=L'\0';
            trim_ws(left); trim_ws(right);
            return 1;
        }
    }
    return 0;
}

static int execute_line(wchar_t *line);

static int cmd_help(int argc, wchar_t argv[][1024]) {
    (void)argc; (void)argv;
    wprintf(L"HADA %ls\n", HADA_VERSION);
    wprintf(L"Sandbox-first Windows shell.\n\n");
    wprintf(L"Core: log, help, about, version, exit, clear, pwd, cd, dir, scan, copy, move, delete, mkdir, rmdir, open, run, process, sandbox, server, service, system.\n");
    wprintf(L"Syntax: && AND, -> selection, > output, < input, & chain, $ values, - options.\n");
    wprintf(L"Sandbox storage: U:\\ (unlimited file storage), X:\\ (40GB important storage).\n");
    return 1;
}

static int cmd_log(const wchar_t *rest) {
    wchar_t x[4096];
    wcsncpy(x, rest, sizeof(x)/sizeof(x[0])-1); x[sizeof(x)/sizeof(x[0])-1]=L'\0'; trim_ws(x);
    if (x[0] == L'(' && x[wcslen(x)-1] == L')') { x[wcslen(x)-1]=L'\0'; memmove(x,x+1,wcslen(x)*sizeof(wchar_t)); }
    trim_ws(x);
    if (x[0] == L'"' && x[wcslen(x)-1] == L'"') { x[wcslen(x)-1]=L'\0'; wprintf(L"%ls\n", x+1); }
    else {
        wchar_t exp[4096]; if (env_expand(x, exp, sizeof(exp)/sizeof(exp[0]))) wprintf(L"%ls\n", exp); else wprintf(L"%ls\n", x);
    }
    return 1;
}

static int cmd_pwd(void) { wchar_t b[MAX_PATH*8]; DWORD n=GetCurrentDirectoryW((DWORD)(sizeof(b)/sizeof(b[0])),b); if(n) wprintf(L"%ls\n",b); else winerr(L"pwd"); return n!=0; }

static int cmd_cd(const wchar_t *arg) {
    wchar_t p[MAX_PATH*8]; if (!resolve_path(arg,p,sizeof(p)/sizeof(p[0]))) { die_msg(L"invalid path"); return 0; }
    if (!SetCurrentDirectoryW(p)) { winerr(L"cd"); return 0; }
    return 1;
}

static void print_entry(const WIN32_FIND_DATAW *fd) {
    if (fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) wprintf(L"<DIR> %ls\n", fd->cFileName);
    else {
        ULARGE_INTEGER u; u.HighPart=fd->nFileSizeHigh; u.LowPart=fd->nFileSizeLow;
        wprintf(L"%12llu %ls\n", u.QuadPart, fd->cFileName);
    }
}

static int cmd_dir(const wchar_t *arg) {
    wchar_t p[MAX_PATH*8]; if (!resolve_path(arg && *arg ? arg : L".",p,sizeof(p)/sizeof(p[0]))) return 0;
    wchar_t pat[MAX_PATH*8]; _snwprintf(pat,sizeof(pat)/sizeof(pat[0]),L"%ls\\*",p);
    WIN32_FIND_DATAW fd; HANDLE h=FindFirstFileW(pat,&fd); if(h==INVALID_HANDLE_VALUE){winerr(L"dir");return 0;}
    do { if(wcscmp(fd.cFileName,L".")&&wcscmp(fd.cFileName,L"..")) print_entry(&fd); } while(FindNextFileW(h,&fd));
    FindClose(h); return 1;
}

static int wildcard_match_i(const wchar_t *pat, const wchar_t *str) {
    if (!*pat) return !*str;
    if (*pat == L'*') return wildcard_match_i(pat+1,str) || (*str && wildcard_match_i(pat,str+1));
    if (*pat == L'?') return *str && wildcard_match_i(pat+1,str+1);
    return towlower(*pat)==towlower(*str) && wildcard_match_i(pat+1,str+1);
}

static void scan_recursive(const wchar_t *base, const wchar_t *pattern, int recursive, unsigned long long *count) {
    wchar_t pat[MAX_PATH*8]; _snwprintf(pat,sizeof(pat)/sizeof(pat[0]),L"%ls\\*",base);
    WIN32_FIND_DATAW fd; HANDLE h=FindFirstFileW(pat,&fd); if(h==INVALID_HANDLE_VALUE)return;
    do {
        if(wcscmp(fd.cFileName,L".")==0||wcscmp(fd.cFileName,L"..")==0) continue;
        wchar_t child[MAX_PATH*8]; _snwprintf(child,sizeof(child)/sizeof(child[0]),L"%ls\\%ls",base,fd.cFileName);
        if (wildcard_match_i(pattern,fd.cFileName)) { wprintf(L"%ls\n",child); (*count)++; }
        if ((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&recursive) scan_recursive(child,pattern,recursive,count);
    } while(FindNextFileW(h,&fd));
    FindClose(h);
}

static int cmd_scan(int argc, wchar_t argv[][1024]) {
    if (argc < 2) { die_msg(L"usage: scan <path> [-> <pattern>] [-recursive]"); return 0; }
    wchar_t p[MAX_PATH*8]; if(!resolve_path(argv[1],p,sizeof(p)/sizeof(p[0]))) return 0;
    const wchar_t *pattern=L"*"; int recursive=0;
    for(int i=2;i<argc;i++){ if(ieq(argv[i],L"->") && i+1<argc) pattern=argv[++i]; else if(ieq(argv[i],L"-recursive")) recursive=1; }
    unsigned long long c=0; scan_recursive(p,pattern,recursive,&c); wprintf(L"-- %llu match(es)\n",c); return 1;
}

static int cmd_mkdir(const wchar_t *arg) { wchar_t p[MAX_PATH*8]; if(!resolve_path(arg,p,sizeof(p)/sizeof(p[0])))return 0; if(ensure_dir_recursive(p))return 1; winerr(L"mkdir"); return 0; }
static int cmd_rmdir(const wchar_t *arg) { wchar_t p[MAX_PATH*8]; if(!resolve_path(arg,p,sizeof(p)/sizeof(p[0])))return 0; if(!is_inside(p,g_sandbox)){die_msg(L"rmdir is restricted to the active sandbox");return 0;} return remove_tree(p); }

static int cmd_copy(const wchar_t *a, const wchar_t *b) {
    wchar_t src[MAX_PATH*8], dst[MAX_PATH*8]; if(!resolve_path(a,src,sizeof(src)/sizeof(src[0]))||!resolve_path(b,dst,sizeof(dst)/sizeof(dst[0]))) return 0;
    if(!path_exists(src)){die_msg(L"source does not exist");return 0;}
    if(is_dir(dst) || (path_exists(dst)&&is_dir(dst))) { wchar_t base[1024], nd[MAX_PATH*8]; basename_of(src,base,sizeof(base)/sizeof(base[0])); join2(dst,base,nd,sizeof(nd)/sizeof(nd[0])); wcscpy(dst,nd); }
    if(is_inside(src,g_sandbox)==0 && is_inside(dst,g_sandbox)==0){die_msg(L"at least one side of copy must be inside the sandbox");return 0;}
    return copy_resolved(src,dst);
}

static int cmd_move(const wchar_t *a, const wchar_t *b) {
    wchar_t src[MAX_PATH*8], dst[MAX_PATH*8]; if(!resolve_path(a,src,sizeof(src)/sizeof(src[0]))||!resolve_path(b,dst,sizeof(dst)/sizeof(dst[0]))) return 0;
    if(!path_exists(src)){die_msg(L"source does not exist");return 0;}
    if(is_dir(dst)){wchar_t base[1024],nd[MAX_PATH*8];basename_of(src,base,sizeof(base)/sizeof(base[0]));join2(dst,base,nd,sizeof(nd)/sizeof(nd[0]));wcscpy(dst,nd);}
    if(is_inside(src,g_sandbox)==0 && is_inside(dst,g_sandbox)==0){die_msg(L"at least one side of move must be inside the sandbox");return 0;}
    if(is_inside(dst,g_x) && !x_quota_ok_for(src,dst)) return 0;
    wchar_t parent[MAX_PATH*8];dirname_of(dst,parent,sizeof(parent)/sizeof(parent[0]));if(parent[0])ensure_dir_recursive(parent);
    if(MoveFileExW(src,dst,MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED))return 1;
    winerr(L"move");return 0;
}

static int cmd_delete(const wchar_t *arg) { wchar_t p[MAX_PATH*8]; if(!resolve_path(arg,p,sizeof(p)/sizeof(p[0])))return 0; if(!safe_delete(p)){if(GetLastError())winerr(L"delete");return 0;} return 1; }
static int cmd_file_info(const wchar_t *arg) { wchar_t p[MAX_PATH*8]; if(!resolve_path(arg,p,sizeof(p)/sizeof(p[0])))return 0; if(!path_exists(p)){die_msg(L"not found");return 0;} wprintf(L"Path: %ls\nType: %ls\nSize: ",p,is_dir(p)?L"directory":L"file"); print_size(dir_size(p)); return 1; }

static int build_commandline(wchar_t *out,size_t cap,int argc,wchar_t argv[][1024],int start) {
    out[0]=L'\0'; size_t used=0;
    for(int i=start;i<argc;i++){
        if(i>start){if(used+1>=cap)return 0;out[used++]=L' ';}
        const wchar_t *a=argv[i]; int quote=(wcspbrk(a,L" \t")!=NULL); if(quote){if(used+1>=cap)return 0;out[used++]=L'"';}
        size_t n=wcslen(a); if(used+n+(quote?1:0)+1>=cap)return 0; wcscpy(out+used,a);used+=n;if(quote)out[used++]=L'"';
    }
    out[used]=L'\0';return 1;
}

static int cmd_run(int argc,wchar_t argv[][1024]) {
    if(argc<2){die_msg(L"usage: run <program> [args]");return 0;}
    wchar_t program[MAX_PATH*8]; if(!resolve_path(argv[1],program,sizeof(program)/sizeof(program[0])))return 0;
    if(!is_inside(program,g_sandbox)){
        wchar_t candidate[MAX_PATH*8]; wchar_t sandboxprog[MAX_PATH*8];
        if(starts_with_i(argv[1],L"C:\\")||starts_with_i(argv[1],L"D:\\")||starts_with_i(argv[1],L"E:\\")) {
            /* Explicit host paths are copied into U:\imports before execution. */
            wchar_t name[1024]; basename_of(program,name,sizeof(name)/sizeof(name[0])); join2(g_u,L"imports",candidate,sizeof(candidate)/sizeof(candidate[0])); ensure_dir_recursive(candidate); join2(candidate,name,sandboxprog,sizeof(sandboxprog)/sizeof(sandboxprog[0]));
            if(!copy_resolved(program,sandboxprog)) return 0; wcscpy(program,sandboxprog);
        } else { die_msg(L"run is sandbox-first; use a sandbox path or a host executable that can be imported"); return 0; }
    }
    wchar_t cmdline[HADA_MAX_LINE]; if(!build_commandline(cmdline,sizeof(cmdline)/sizeof(cmdline[0]),argc,argv,1)) {die_msg(L"command line too long");return 0;}
    /* Replace first token with fully resolved program. */
    wchar_t rest[HADA_MAX_LINE]=L""; size_t pos=0; wcsncpy(rest,cmdline,sizeof(rest)/sizeof(rest[0])-1); rest[sizeof(rest)/sizeof(rest[0])-1]=L'\0';
    wchar_t *sp=wcschr(rest,L' '); wchar_t newcmd[HADA_MAX_LINE]; if(sp){_snwprintf(newcmd,sizeof(newcmd)/sizeof(newcmd[0]),L"\"%ls\"%ls",program,sp);}else{_snwprintf(newcmd,sizeof(newcmd)/sizeof(newcmd[0]),L"\"%ls\"",program);} (void)pos;
    STARTUPINFOW si; PROCESS_INFORMATION pi; ZeroMemory(&si,sizeof(si)); ZeroMemory(&pi,sizeof(pi)); si.cb=sizeof(si);
    wchar_t cwd[MAX_PATH*8]; wcscpy(cwd,g_sandbox);
    if(!CreateProcessW(program,newcmd,NULL,NULL,FALSE,CREATE_NEW_PROCESS_GROUP,NULL,cwd,&si,&pi)){winerr(L"run");return 0;}
    wprintf(L"PID %lu\n",(unsigned long)pi.dwProcessId);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return 1;
}

static void list_processes(void) {
    HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); if(h==INVALID_HANDLE_VALUE){winerr(L"process list");return;}
    PROCESSENTRY32W pe; pe.dwSize=sizeof(pe); if(Process32FirstW(h,&pe)){ do{wprintf(L"%6lu %ls\n",(unsigned long)pe.th32ProcessID,pe.szExeFile);}while(Process32NextW(h,&pe));}
    CloseHandle(h);
}

static int cmd_process(int argc,wchar_t argv[][1024]) {
    if(argc<2){die_msg(L"usage: process list|kill <pid>");return 0;}
    if(ieq(argv[1],L"list")){list_processes();return 1;}
    if(ieq(argv[1],L"kill")&&argc>=3){DWORD pid=(DWORD)wcstoul(argv[2],NULL,10);HANDLE h=OpenProcess(PROCESS_TERMINATE,FALSE,pid);if(!h){winerr(L"process kill");return 0;}BOOL ok=TerminateProcess(h,1);CloseHandle(h);if(!ok){winerr(L"process kill");return 0;}return 1;}
    if(ieq(argv[1],L"status")&&argc>=3){DWORD pid=(DWORD)wcstoul(argv[2],NULL,10);HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!h){winerr(L"process status");return 0;}DWORD code=0;BOOL ok=GetExitCodeProcess(h,&code);CloseHandle(h);if(ok)wprintf(L"PID %lu status %lu\n",(unsigned long)pid,(unsigned long)code);return ok;}
    die_msg(L"unknown process command");return 0;
}

static int sandbox_make(const wchar_t *name) {
    wchar_t p[MAX_PATH*8]; join2(g_root,name,p,sizeof(p)/sizeof(p[0]));
    wchar_t u[MAX_PATH*8],x[MAX_PATH*8];join2(p,L"U",u,sizeof(u)/sizeof(u[0]));join2(p,L"X",x,sizeof(x)/sizeof(x[0]));
    if(!ensure_dir_recursive(u)||!ensure_dir_recursive(x)){winerr(L"sandbox create");return 0;}
    return 1;
}

static int sandbox_open(const wchar_t *name) {
    wchar_t p[MAX_PATH*8]; join2(g_root,name,p,sizeof(p)/sizeof(p[0])); if(!is_dir(p)){die_msg(L"sandbox not found");return 0;}
    wcscpy(g_sandbox,p); join2(p,L"U",g_u,sizeof(g_u)/sizeof(g_u[0])); join2(p,L"X",g_x,sizeof(g_x)/sizeof(g_x[0])); ensure_dir_recursive(g_u); ensure_dir_recursive(g_x); return 1;
}

static void sandbox_status(void) {
    wprintf(L"Sandbox: %ls\nU: %ls\nX: %ls\nU size: ",g_sandbox,g_u,g_x);print_size(dir_size(g_u));wprintf(L"X size: ");print_size(dir_size(g_x));wprintf(L"X limit: 40GB\n");
}

static int cmd_sandbox(int argc,wchar_t argv[][1024]) {
    if(argc<2){sandbox_status();return 1;}
    if(ieq(argv[1],L"create")&&argc>=3){if(!sandbox_make(argv[2]))return 0;return sandbox_open(argv[2]);}
    if(ieq(argv[1],L"open")&&argc>=3)return sandbox_open(argv[2]);
    if(ieq(argv[1],L"list")){wchar_t pat[MAX_PATH*8];_snwprintf(pat,sizeof(pat)/sizeof(pat[0]),L"%ls\\*",g_root);WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE){return 1;}do{if((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&wcscmp(fd.cFileName,L".")&&wcscmp(fd.cFileName,L".."))wprintf(L"%ls\n",fd.cFileName);}while(FindNextFileW(h,&fd));FindClose(h);return 1;}
    if(ieq(argv[1],L"status")){sandbox_status();return 1;}
    if(ieq(argv[1],L"reset")){if(!remove_tree(g_sandbox)){die_msg(L"sandbox reset failed");return 0;} if(!sandbox_make(L"default"))return 0;return sandbox_open(L"default");}
    if(ieq(argv[1],L"save")){wprintf(L"Sandbox is persistent; current state is already saved at %ls\n",g_sandbox);return 1;}
    if(ieq(argv[1],L"load")&&argc>=3)return sandbox_open(argv[2]);
    if(ieq(argv[1],L"delete")&&argc>=3){wchar_t p[MAX_PATH*8];join2(g_root,argv[2],p,sizeof(p)/sizeof(p[0]));if(ieq(argv[2],L"default")){die_msg(L"cannot delete default sandbox");return 0;}if(!remove_tree(p)){winerr(L"sandbox delete");return 0;}return 1;}
    if(ieq(argv[1],L"run")&&argc>=3)return cmd_run(argc-1,argv+1);
    if(ieq(argv[1],L"open")&&argc>=3)return sandbox_open(argv[2]);
    die_msg(L"unknown sandbox command");return 0;
}

static int cmd_open(const wchar_t *arg) {
    wchar_t p[MAX_PATH*8]; if(!resolve_path(arg,p,sizeof(p)/sizeof(p[0])))return 0;
    HINSTANCE r=ShellExecuteW(NULL,L"open",p,NULL,NULL,SW_SHOWNORMAL); if((INT_PTR)r<=32){winerr(L"open");return 0;} return 1;
}

static int cmd_about(void) {
    wprintf(L"HADA %ls\n",HADA_VERSION);
    wprintf(L"Windows-only sandbox-first shell.\n");
    wprintf(L"Active sandbox: %ls\n",g_sandbox);
    wprintf(L"U: unlimited general storage\nX: 40GB important storage\n");
    return 1;
}

static int cmd_system(int argc,wchar_t argv[][1024]) {
    if(argc>=2&&ieq(argv[1],L"info")){SYSTEM_INFO si;GetSystemInfo(&si);wprintf(L"CPU architecture: %u\nProcessors: %lu\nPage size: %lu\n",si.wProcessorArchitecture,(unsigned long)si.dwNumberOfProcessors,(unsigned long)si.dwPageSize);return 1;}
    if(argc>=2&&ieq(argv[1],L"status")){wprintf(L"Windows uptime: %llu seconds\n",(unsigned long long)(GetTickCount64()/1000ULL));return 1;}
    die_msg(L"usage: system info|status");return 0;
}

static int execute_one(wchar_t *line) {
    trim_ws(line); if(!*line)return 1;
    if (starts_with_i(line, L"log(")) return cmd_log(line + 3);
    wchar_t argv[HADA_MAX_ARGS][1024]; int argc=parse_words(line,argv,HADA_MAX_ARGS); if(argc==0)return 1;
    if(ieq(argv[0],L"help"))return cmd_help(argc,argv);
    if(ieq(argv[0],L"about"))return cmd_about();
    if(ieq(argv[0],L"version")){wprintf(L"HADA %ls\n",HADA_VERSION);return 1;}
    if(ieq(argv[0],L"clear")){system("cls");return 1;}
    if(ieq(argv[0],L"exit")){exit(0);}
    if(ieq(argv[0],L"log")){const wchar_t *p=wcsstr(line,L"log");p+=3;return cmd_log(p);}
    if(ieq(argv[0],L"pwd"))return cmd_pwd();
    if(ieq(argv[0],L"cd")){return argc>=2?cmd_cd(argv[1]):cmd_cd(g_sandbox);}
    if(ieq(argv[0],L"dir")||ieq(argv[0],L"files"))return cmd_dir(argc>=2?argv[1]:L".");
    if(ieq(argv[0],L"scan"))return cmd_scan(argc,argv);
    if(ieq(argv[0],L"mkdir"))return argc>=2?cmd_mkdir(argv[1]):0;
    if(ieq(argv[0],L"rmdir"))return argc>=2?cmd_rmdir(argv[1]):0;
    if(ieq(argv[0],L"copy")){ if(argc<4 || !ieq(argv[2],L"->")){die_msg(L"usage: copy <source> -> <destination>");return 0;} return cmd_copy(argv[1],argv[3]); }
    if(ieq(argv[0],L"move")){ if(argc<4 || !ieq(argv[2],L"->")){die_msg(L"usage: move <source> -> <destination>");return 0;} return cmd_move(argv[1],argv[3]); }
    if(ieq(argv[0],L"delete"))return argc>=2?cmd_delete(argv[1]):0;
    if(ieq(argv[0],L"file"))return argc>=2?cmd_file_info(argv[1]):0;
    if(ieq(argv[0],L"open"))return argc>=2?cmd_open(argv[1]):0;
    if(ieq(argv[0],L"run"))return cmd_run(argc,argv);
    if(ieq(argv[0],L"process"))return cmd_process(argc,argv);
    if(ieq(argv[0],L"sandbox"))return cmd_sandbox(argc,argv);
    if(ieq(argv[0],L"system"))return cmd_system(argc,argv);
    if(ieq(argv[0],L"xspace")){wprintf(L"X: used: ");print_size(dir_size(g_x));wprintf(L"X: free quota: ");unsigned long long used=dir_size(g_x);print_size(used>=HADA_X_LIMIT?0:HADA_X_LIMIT-used);return 1;}
    if(ieq(argv[0],L"xinfo")){wprintf(L"X:\\\nLimit: 40GB\nUsed: ");print_size(dir_size(g_x));return 1;}
    if(ieq(argv[0],L"server")){if(argc>=2&&ieq(argv[1],L"status")){wprintf(L"HADA server command family is available; use Windows services or your server executable inside the sandbox.\n");return 1;} if(argc>=2&&ieq(argv[1],L"list")){wprintf(L"Server discovery is not implemented in v0.1.\n");return 1;}}
    if(ieq(argv[0],L"service")){wprintf(L"Service management is reserved for the Windows-specific extended command family.\n");return 1;}
    /* External Windows command: still launched inside the sandbox working directory. */
    wchar_t cmdline[HADA_MAX_LINE]; if(!build_commandline(cmdline,sizeof(cmdline)/sizeof(cmdline[0]),argc,argv,0)) return 0;
    STARTUPINFOW si; PROCESS_INFORMATION pi; ZeroMemory(&si,sizeof(si));ZeroMemory(&pi,sizeof(pi));si.cb=sizeof(si);
    wchar_t mutable_cmd[HADA_MAX_LINE];wcscpy(mutable_cmd,cmdline);
    if(CreateProcessW(NULL,mutable_cmd,NULL,NULL,FALSE,CREATE_NEW_PROCESS_GROUP,NULL,g_sandbox,&si,&pi)){
        wprintf(L"PID %lu\n",(unsigned long)pi.dwProcessId);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return 1;
    }
    fwprintf(stderr,L"HADA: unknown command '%ls'\n",argv[0]);return 0;
}

static int execute_line(wchar_t *line) {
    trim_ws(line); if(!*line)return 1;
    wchar_t left[HADA_MAX_LINE], right[HADA_MAX_LINE];
    if(split_top_level(line,left,sizeof(left)/sizeof(left[0]),right,sizeof(right)/sizeof(right[0]),L"&&")){
        int ok=execute_line(left); if(!ok)return 0; return execute_line(right);
    }
    if(split_top_level(line,left,sizeof(left)/sizeof(left[0]),right,sizeof(right)/sizeof(right[0]),L"&")){
        int a=execute_line(left); int b=execute_line(right); return a&&b;
    }
    return execute_one(line);
}

static void set_console_utf8(void){ SetConsoleCP(CP_UTF8); SetConsoleOutputCP(CP_UTF8); }

static int init_hada(void) {
    set_console_utf8();
    wchar_t app[MAX_PATH*8]; DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",app,sizeof(app)/sizeof(app[0]));
    if(!n || n>=sizeof(app)/sizeof(app[0])) { GetCurrentDirectoryW(sizeof(app)/sizeof(app[0]),app); join2(app,L".hada",g_root,sizeof(g_root)/sizeof(g_root[0])); }
    else join2(app,L"HADA\\sandboxes",g_root,sizeof(g_root)/sizeof(g_root[0]));
    ensure_dir_recursive(g_root);
    if(!path_exists(g_root)){die_msg(L"cannot initialize HADA storage");return 0;}
    if(!sandbox_make(L"default"))return 0;
    if(!sandbox_open(L"default"))return 0;
    SetCurrentDirectoryW(g_sandbox);
    return 1;
}

int wmain(void) {
    if(!init_hada()) return 1;
    wprintf(L"HADA %ls - Sandbox-first Windows Shell\n",HADA_VERSION);
    wprintf(L"Sandbox: %ls\nU: %ls\nX: %ls (40GB)\n",g_sandbox,g_u,g_x);
    wprintf(L"Type help for commands.\n\n");
    wchar_t line[HADA_MAX_LINE];
    for(;;){
        wprintf(L"HADA[%ls]> ",g_sandbox);
        fflush(stdout);
        if(!fgetws(line,(int)(sizeof(line)/sizeof(line[0])),stdin)) break;
        line[wcscspn(line,L"\r\n")]=L'\0';
        if(!execute_line(line)) wprintf(L"HADA: command failed.\n");
    }
    return 0;
}
#else
#include <stdio.h>
int main(void){fprintf(stderr,"HADA is a Windows-only shell. Build with MinGW-w64 or MSVC on Windows.\n");return 1;}
#endif
