# clce
<img width="534" height="393" alt="pixil-frame-0" src="https://github.com/user-attachments/assets/09809064-0af6-4f18-86ee-d350bcb92b0b" />


# clce는 그냥 코드 뭐시기하는데입니다.
```cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <direct.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

static const UINT WM_NET = WM_APP + 10;
static const UINT WM_VOICE = WM_APP + 11;

enum PacketType : uint32_t {
    PK_CHAT = 1,
    PK_FILE = 2,
    PK_VOICE = 3,
    PK_PING = 4
};

#pragma pack(push, 1)
struct PacketHeader {
    uint32_t type;
    uint32_t size;
};
#pragma pack(pop)

static HWND gMain = nullptr;
static HWND gFiles = nullptr;
static HWND gEditor = nullptr;
static HWND gHistory = nullptr;
static HWND gOutput = nullptr;
static HWND gChatLog = nullptr;
static HWND gChatInput = nullptr;
static HWND gHostInput = nullptr;
static HWND gPortInput = nullptr;
static HWND gStatus = nullptr;
static HFONT gFont = nullptr;
static HFONT gMono = nullptr;

static fs::path gProject;
static std::string gCurrentFile;
static std::vector<std::string> gFilesList;
static std::vector<std::string> gSnapshots;
static std::mutex gNetMutex;
static SOCKET gPeer = INVALID_SOCKET;
static SOCKET gListen = INVALID_SOCKET;
static std::thread gNetThread;
static std::atomic<bool> gRunning{false};
static std::atomic<bool> gListening{false};
static std::atomic<bool> gVoice{false};

static HWAVEIN gWaveIn = nullptr;
static HWAVEOUT gWaveOut = nullptr;
static std::vector<WAVEHDR*> gInHeaders;
static std::mutex gWaveMutex;

static std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), r.data(), n, nullptr, nullptr);
    return r;
}
static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    if (n <= 0) return L"";
    std::wstring r(n, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), r.data(), n);
    return r;
}
static std::string GetText(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring w(n + 1, L'\0');
    if (n) GetWindowTextW(h, w.data(), n + 1);
    w.resize(n);
    return WideToUtf8(w);
}
static void SetText(HWND h, const std::string& s) {
    SetWindowTextW(h, Utf8ToWide(s).c_str());
}
static void AppendText(HWND h, const std::string& s) {
    int n = GetWindowTextLengthW(h);
    SendMessageW(h, EM_SETSEL, n, n);
    SendMessageW(h, EM_REPLACESEL, FALSE, (LPARAM)Utf8ToWide(s).c_str());
}
static std::string NowStamp() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream o;
    o << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return o.str();
}
static void Status(const std::string& s) { if (gStatus) SetText(gStatus, s); }
static void EnsureDir(const fs::path& p) { std::error_code ec; fs::create_directories(p, ec); }

static void RefreshFiles() {
    if (!gFiles) return;
    SendMessageW(gFiles, LB_RESETCONTENT, 0, 0);
    gFilesList.clear();
    if (gProject.empty()) return;
    std::error_code ec;
    for (auto& e : fs::recursive_directory_iterator(gProject, ec)) {
        if (ec) break;
        if (!e.is_regular_file()) continue;
        auto rel = fs::relative(e.path(), gProject, ec).generic_string();
        if (rel.rfind(".slce/", 0) == 0 || rel == ".slce") continue;
        gFilesList.push_back(rel);
    }
    std::sort(gFilesList.begin(), gFilesList.end());
    for (auto& f : gFilesList) SendMessageW(gFiles, LB_ADDSTRING, 0, (LPARAM)Utf8ToWide(f).c_str());
}

static bool ReadFileUtf8(const fs::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return true;
}
static bool WriteFileUtf8(const fs::path& p, const std::string& s) {
    EnsureDir(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    if (!f) return false;
    f.write(s.data(), (std::streamsize)s.size());
    return !!f;
}

static std::string DiffLines(const std::string& a, const std::string& b) {
    std::vector<std::string> A, B;
    std::stringstream sa(a), sb(b); std::string x;
    while (std::getline(sa, x)) A.push_back(x);
    while (std::getline(sb, x)) B.push_back(x);
    std::vector<std::vector<int>> dp(A.size()+1, std::vector<int>(B.size()+1, 0));
    for (int i=(int)A.size()-1;i>=0;--i)
        for (int j=(int)B.size()-1;j>=0;--j)
            dp[i][j] = (A[i]==B[j]) ? dp[i+1][j+1]+1 : std::max(dp[i+1][j], dp[i][j+1]);
    std::ostringstream o;
    o << "--- SLCE DIFF ---\n";
    size_t i=0,j=0;
    while (i<A.size() || j<B.size()) {
        if (i<A.size() && j<B.size() && A[i]==B[j]) { o << "  " << A[i] << "\n"; ++i; ++j; }
        else if (j<B.size() && (i==A.size() || dp[i][j+1] >= dp[i+1][j])) { o << "+ " << B[j++] << "\n"; }
        else { o << "- " << A[i++] << "\n"; }
    }
    return o.str();
}

static std::string RenderMarkdown(const std::string& md) {
    std::stringstream in(md); std::string line; std::ostringstream out;
    bool code=false;
    while (std::getline(in,line)) {
        if (line.rfind("```",0)==0) { code=!code; out << (code?"[code]\n":"[/code]\n"); continue; }
        if (code) { out << "    " << line << "\n"; continue; }
        if (line.rfind("### ",0)==0) out << "### " << line.substr(4) << "\n";
        else if (line.rfind("## ",0)==0) out << "## " << line.substr(3) << "\n";
        else if (line.rfind("# ",0)==0) out << "# " << line.substr(2) << "\n";
        else if (line.rfind("- ",0)==0 || line.rfind("* ",0)==0) out << "• " << line.substr(2) << "\n";
        else out << line << "\n";
    }
    return out.str();
}

static void SaveCurrentFile(bool broadcast);

static bool SendAll(SOCKET s, const char* data, int len) {
    while (len > 0) {
        int n = send(s, data, len, 0);
        if (n <= 0) return false;
        data += n; len -= n;
    }
    return true;
}
static bool RecvAll(SOCKET s, char* data, int len) {
    while (len > 0) {
        int n = recv(s, data, len, 0);
        if (n <= 0) return false;
        data += n; len -= n;
    }
    return true;
}
static bool SendPacket(uint32_t type, const std::string& payload) {
    std::lock_guard<std::mutex> lock(gNetMutex);
    if (gPeer == INVALID_SOCKET) return false;
    PacketHeader h{type, (uint32_t)payload.size()};
    return SendAll(gPeer, (const char*)&h, sizeof(h)) &&
           (payload.empty() || SendAll(gPeer, payload.data(), (int)payload.size()));
}
static void NetUiText(const std::string& s) {
    if (gMain) PostMessageA(gMain, WM_NET, 0, (LPARAM)new std::string(s));
}

static void PlayVoice(const std::string& pcm) {
    std::lock_guard<std::mutex> lock(gWaveMutex);
    if (!gWaveOut) {
        WAVEFORMATEX fmt{};
        fmt.wFormatTag=WAVE_FORMAT_PCM; fmt.nChannels=1; fmt.nSamplesPerSec=8000;
        fmt.wBitsPerSample=16; fmt.nBlockAlign=2; fmt.nAvgBytesPerSec=16000;
        if (waveOutOpen(&gWaveOut, WAVE_MAPPER, &fmt, (DWORD_PTR)nullptr, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
    }
    char* mem = new char[pcm.size()]; memcpy(mem, pcm.data(), pcm.size());
    auto* hdr = new WAVEHDR{};
    hdr->lpData=mem; hdr->dwBufferLength=(DWORD)pcm.size();
    if (waveOutPrepareHeader(gWaveOut, hdr, sizeof(*hdr)) == MMSYSERR_NOERROR) waveOutWrite(gWaveOut, hdr, sizeof(*hdr));
    else { delete[] mem; delete hdr; }
}

static void NetReceiver(SOCKET s) {
    gRunning=true;
    NetUiText("[P2P] connected");
    while (gRunning) {
        PacketHeader h{};
        if (!RecvAll(s,(char*)&h,sizeof(h))) break;
        if (h.size > 32*1024*1024) break;
        std::string payload(h.size,'\0');
        if (h.size && !RecvAll(s,payload.data(),(int)h.size)) break;
        if (h.type==PK_CHAT) {
            NetUiText("[CHAT] "+payload);
        } else if (h.type==PK_FILE) {
            auto p=payload.find('\0');
            if (p!=std::string::npos && !gProject.empty()) {
                std::string name=payload.substr(0,p), data=payload.substr(p+1);
                WriteFileUtf8(gProject/fs::path(Utf8ToWide(name)),data);
                RefreshFiles();
                if (name==gCurrentFile) SetText(gEditor,data);
                NetUiText("[SYNC] "+name);
            }
        } else if (h.type==PK_VOICE) {
            PlayVoice(payload);
        } else if (h.type==PK_PING) {
            SendPacket(PK_PING,"pong");
        }
    }
    {
        std::lock_guard<std::mutex> lock(gNetMutex);
        if (gPeer==s) { closesocket(gPeer); gPeer=INVALID_SOCKET; }
    }
    gRunning=false;
    NetUiText("[P2P] disconnected");
}

static bool ConnectTo(const std::string& host, unsigned short port) {
    if (gPeer!=INVALID_SOCKET) return false;
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(s==INVALID_SOCKET) return false;
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port);
    if (inet_pton(AF_INET,host.c_str(),&a.sin_addr)<=0) { closesocket(s); return false; }
    if (connect(s,(sockaddr*)&a,sizeof(a))==SOCKET_ERROR) { closesocket(s); return false; }
    gPeer=s;
    if (gNetThread.joinable()) gNetThread.detach();
    gNetThread=std::thread(NetReceiver,s);
    return true;
}
static void ListenThread(unsigned short port) {
    SOCKET ls=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(ls==INVALID_SOCKET) return;
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_addr.s_addr=INADDR_ANY; a.sin_port=htons(port);
    int yes=1; setsockopt(ls,SOL_SOCKET,SO_REUSEADDR,(char*)&yes,sizeof(yes));
    if(bind(ls,(sockaddr*)&a,sizeof(a))==SOCKET_ERROR || listen(ls,1)==SOCKET_ERROR){closesocket(ls);return;}
    gListen=ls; gListening=true; NetUiText("[P2P] listening on port "+std::to_string(port));
    SOCKET s=accept(ls,nullptr,nullptr);
    if(s!=INVALID_SOCKET){
        {std::lock_guard<std::mutex> lock(gNetMutex); gPeer=s;}
        if(gNetThread.joinable()) gNetThread.detach();
        gNetThread=std::thread(NetReceiver,s);
    }
    closesocket(ls); gListen=INVALID_SOCKET; gListening=false;
}

static void CALLBACK VoiceInputProc(HWAVEIN wi, UINT msg, DWORD_PTR instance, DWORD_PTR p1, DWORD_PTR p2) {
    (void)wi;(void)p2;
    if(msg!=WIM_DATA) return;
    WAVEHDR* hdr=(WAVEHDR*)p1;
    if(gVoice && hdr && hdr->dwBytesRecorded) SendPacket(PK_VOICE,std::string(hdr->lpData,hdr->dwBytesRecorded));
    if(gVoice && gWaveIn) { hdr->dwBytesRecorded=0; waveInAddBuffer(gWaveIn,hdr,sizeof(*hdr)); }
}

static bool StartVoice() {
    if(gVoice) return true;
    WAVEFORMATEX fmt{};
    fmt.wFormatTag=WAVE_FORMAT_PCM;fmt.nChannels=1;fmt.nSamplesPerSec=8000;fmt.wBitsPerSample=16;fmt.nBlockAlign=2;fmt.nAvgBytesPerSec=16000;
    if(waveInOpen(&gWaveIn,WAVE_MAPPER,&fmt,(DWORD_PTR)VoiceInputProc,0,CALLBACK_FUNCTION)!=MMSYSERR_NOERROR) return false;
    const int N=4, SZ=3200;
    gInHeaders.clear();
    for(int i=0;i<N;i++){
        auto* h=new WAVEHDR{}; h->lpData=new char[SZ]; h->dwBufferLength=SZ;
        if(waveInPrepareHeader(gWaveIn,h,sizeof(*h))!=MMSYSERR_NOERROR || waveInAddBuffer(gWaveIn,h,sizeof(*h))!=MMSYSERR_NOERROR){delete[] h->lpData;delete h;continue;}
        gInHeaders.push_back(h);
    }
    if(gInHeaders.empty()){waveInClose(gWaveIn);gWaveIn=nullptr;return false;}
    waveInStart(gWaveIn); gVoice=true; return true;
}
static void StopVoice() {
    gVoice=false;
    if(gWaveIn){
        waveInStop(gWaveIn); waveInReset(gWaveIn);
        for(auto* h:gInHeaders){waveInUnprepareHeader(gWaveIn,h,sizeof(*h));delete[] h->lpData;delete h;}
        gInHeaders.clear(); waveInClose(gWaveIn); gWaveIn=nullptr;
    }
}

static void CreateControls(HWND h) {
    gFiles=CreateWindowW(L"LISTBOX",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL,10,40,220,450,h,(HMENU)101,GetModuleHandle(nullptr),nullptr);
    gEditor=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL|WS_HSCROLL,240,40,620,450,h,(HMENU)102,GetModuleHandle(nullptr),nullptr);
    gHistory=CreateWindowW(L"LISTBOX",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL,870,40,250,200,h,(HMENU)103,GetModuleHandle(nullptr),nullptr);
    gOutput=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|WS_HSCROLL,870,250,250,240,h,(HMENU)104,GetModuleHandle(nullptr),nullptr);
    gChatLog=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,10,505,550,180,h,(HMENU)105,GetModuleHandle(nullptr),nullptr);
    gChatInput=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER,570,505,300,30,h,(HMENU)106,GetModuleHandle(nullptr),nullptr);
    gHostInput=CreateWindowW(L"EDIT",L"127.0.0.1",WS_CHILD|WS_VISIBLE|WS_BORDER,10,695,150,28,h,(HMENU)107,GetModuleHandle(nullptr),nullptr);
    gPortInput=CreateWindowW(L"EDIT",L"7345",WS_CHILD|WS_VISIBLE|WS_BORDER,165,695,70,28,h,(HMENU)108,GetModuleHandle(nullptr),nullptr);
    gStatus=CreateWindowW(L"STATIC",L"Ready",WS_CHILD|WS_VISIBLE,250,700,700,25,h,(HMENU)109,GetModuleHandle(nullptr),nullptr);
    struct B{int id;int x;int w;const wchar_t* t;};
    B bs[]={{200,10,70,L"Open"},{201,85,95,L"Save"},{202,185,115,L"Snapshot"},{203,305,75,L"Diff"},{204,385,100,L"Markdown"},{205,495,85,L"Export"},{206,590,100,L"Listen"},{207,695,100,L"Connect"},{208,805,100,L"Voice"},{209,915,95,L"Send Chat"}};
    for(auto& b:bs)CreateWindowW(L"BUTTON",b.t,WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,b.x,8,b.w,26,h,(HMENU)b.id,GetModuleHandle(nullptr),nullptr);
}

static void OpenProject() {
    BROWSEINFOW bi{}; bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE; bi.lpszTitle=L"Select SLCE project folder";
    PIDLIST_ABSOLUTE pid=SHBrowseForFolderW(&bi); if(!pid)return;
    wchar_t buf[MAX_PATH]; if(SHGetPathFromIDListW(pid,buf)){
        gProject=buf; CoTaskMemFree(pid); EnsureDir(gProject/L".slce/history"); RefreshFiles(); Status("Project: "+gProject.string());
    } else CoTaskMemFree(pid);
}
static void OpenSelectedFile() {
    int i=(int)SendMessageW(gFiles,LB_GETCURSEL,0,0); if(i<0 || i>=(int)gFilesList.size())return;
    gCurrentFile=gFilesList[i]; std::string s; if(ReadFileUtf8(gProject/fs::path(Utf8ToWide(gCurrentFile)),s)){SetText(gEditor,s);Status("Editing: "+gCurrentFile);}
}
static void SaveCurrentFile(bool broadcast) {
    if(gProject.empty()||gCurrentFile.empty())return;
    std::string s=GetText(gEditor); WriteFileUtf8(gProject/fs::path(Utf8ToWide(gCurrentFile)),s); RefreshFiles(); Status("Saved: "+gCurrentFile);
    if(broadcast){std::string p=gCurrentFile; p.push_back('\0'); p+=s; SendPacket(PK_FILE,p);}
}
static void SaveSnapshot() {
    if(gProject.empty()||gCurrentFile.empty())return;
    std::string stamp=NowStamp(); fs::path p=gProject/L".slce/history"/(Utf8ToWide(stamp+"__"+gCurrentFile));
    WriteFileUtf8(p,GetText(gEditor));
    gSnapshots.push_back(stamp+"__"+gCurrentFile); SendMessageW(gHistory,LB_ADDSTRING,0,(LPARAM)Utf8ToWide(gSnapshots.back()).c_str());
    Status("Snapshot: "+stamp);
}
static std::string LastSnapshotContent() {
    if(gProject.empty()||gCurrentFile.empty())return {};
    std::string best; fs::file_time_type bt{}; bool has=false;
    std::error_code ec; fs::path dir=gProject/L".slce/history";
    for(auto&e:fs::directory_iterator(dir,ec)){
        if(ec||!e.is_regular_file())continue; auto n=e.path().filename().generic_string();
        auto pos=n.find("__"); if(pos==std::string::npos||n.substr(pos+2)!=gCurrentFile)continue;
        auto t=fs::last_write_time(e,ec); if(!has||t>bt){bt=t;has=true;ReadFileUtf8(e.path(),best);}
    }
    return best;
}
static void ShowDiff() { if(gCurrentFile.empty())return; SetText(gOutput,DiffLines(LastSnapshotContent(),GetText(gEditor))); }
static void ShowMarkdown() { SetText(gOutput,RenderMarkdown(GetText(gEditor))); }
static void ExportProject() {
    if(gProject.empty())return;
    BROWSEINFOW bi{}; bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE; bi.lpszTitle=L"Select export folder";
    PIDLIST_ABSOLUTE pid=SHBrowseForFolderW(&bi);if(!pid)return; wchar_t buf[MAX_PATH];
    if(SHGetPathFromIDListW(pid,buf)){
        fs::path dst=fs::path(buf)/(gProject.filename().string()+"_export"); std::error_code ec; fs::create_directories(dst,ec);
        for(auto&e:fs::recursive_directory_iterator(gProject,ec)){
            if(ec||!e.is_regular_file())continue; auto rel=fs::relative(e.path(),gProject,ec).generic_string(); if(rel.rfind(".slce/",0)==0)continue;
            fs::path to=dst/fs::path(Utf8ToWide(rel)); fs::create_directories(to.parent_path(),ec); fs::copy_file(e.path(),to,fs::copy_options::overwrite_existing,ec);
        }
        Status("Exported: "+dst.string());
    }
    CoTaskMemFree(pid);
}
static void InitHistory() {
    gSnapshots.clear(); SendMessageW(gHistory,LB_RESETCONTENT,0,0); if(gProject.empty())return;
    std::error_code ec; auto dir=gProject/L".slce/history"; EnsureDir(dir);
    for(auto&e:fs::directory_iterator(dir,ec)) if(e.is_regular_file()) gSnapshots.push_back(e.path().filename().generic_string());
    std::sort(gSnapshots.begin(),gSnapshots.end()); for(auto&s:gSnapshots)SendMessageW(gHistory,LB_ADDSTRING,0,(LPARAM)Utf8ToWide(s).c_str());
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch(m){
    case WM_CREATE:
        gMain=h; gFont=CreateFontW(16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,L"Segoe UI");
        gMono=CreateFontW(16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_MODERN,L"Consolas");
        CreateControls(h);
        for(HWND c:{gFiles,gEditor,gHistory,gOutput,gChatLog,gChatInput,gHostInput,gPortInput,gStatus})SendMessageW(c,WM_SETFONT,(WPARAM)(c==gEditor?gMono:gFont),TRUE);
        return 0;
    case WM_COMMAND:
        if(LOWORD(w)==101 && HIWORD(w)==LBN_SELCHANGE) OpenSelectedFile();
        else if(LOWORD(w)==200) OpenProject();
        else if(LOWORD(w)==201) SaveCurrentFile(true);
        else if(LOWORD(w)==202) SaveSnapshot();
        else if(LOWORD(w)==203) ShowDiff();
        else if(LOWORD(w)==204) ShowMarkdown();
        else if(LOWORD(w)==205) ExportProject();
        else if(LOWORD(w)==206){ unsigned short p=(unsigned short)std::stoi(GetText(gPortInput)); std::thread(ListenThread,p).detach(); }
        else if(LOWORD(w)==207){ unsigned short p=(unsigned short)std::stoi(GetText(gPortInput)); if(ConnectTo(GetText(gHostInput),p))Status("Connected");else Status("Connect failed"); }
        else if(LOWORD(w)==208){ if(gVoice){StopVoice();Status("Voice stopped");}else if(StartVoice())Status("Voice started");else Status("Voice failed"); }
        else if(LOWORD(w)==209){std::string s=GetText(gChatInput);if(!s.empty()){AppendText(gChatLog,"Me: "+s+"\r\n");SendPacket(PK_CHAT,s);SetText(gChatInput,"");}}
        return 0;
    case WM_NET:{auto* s=(std::string*)l;if(s){AppendText(gChatLog,*s+"\r\n");delete s;}return 0;}
    case WM_SIZE:{int W=LOWORD(l),H=HIWORD(l); if(!gFiles)return 0; MoveWindow(gFiles,10,40,220,H-300,TRUE);MoveWindow(gEditor,240,40,W-500,H-300,TRUE);MoveWindow(gHistory,W-250,40,240,200,TRUE);MoveWindow(gOutput,W-250,250,240,H-300,TRUE);MoveWindow(gChatLog,10,H-190,W-570,150,TRUE);MoveWindow(gChatInput,W-550,H-185,310,30,TRUE);MoveWindow(gHostInput,10,H-35,150,28,TRUE);MoveWindow(gPortInput,165,H-35,70,28,TRUE);MoveWindow(gStatus,250,H-35,W-260,25,TRUE);return 0;}
    case WM_DESTROY:
        StopVoice(); gRunning=false; if(gPeer!=INVALID_SOCKET){shutdown(gPeer,SD_BOTH);closesocket(gPeer);gPeer=INVALID_SOCKET;} if(gListen!=INVALID_SOCKET){closesocket(gListen);gListen=INVALID_SOCKET;} WSACleanup(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h,m,w,l);
}

int WINAPI wWinMain(HINSTANCE hInst,HINSTANCE,LPWSTR,int nCmdShow){
    WSADATA wd{}; if(WSAStartup(MAKEWORD(2,2),&wd)!=0){MessageBoxW(nullptr,L"WSAStartup failed",L"SLCE",MB_ICONERROR);return 1;}
    INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_STANDARD_CLASSES}; InitCommonControlsEx(&ic);
    WNDCLASSW wc{};wc.lpfnWndProc=WndProc;wc.hInstance=hInst;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"SLCEWindow";wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION);RegisterClassW(&wc);
    HWND h=CreateWindowW(wc.lpszClassName,L"SLCE — Save Code Locally, Collaborate, Tremendous, Easy",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1150,760,nullptr,nullptr,hInst,nullptr);
    if(!h)return 1; ShowWindow(h,nCmdShow); UpdateWindow(h);
    MSG msg; while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);} if(gFont)DeleteObject(gFont);if(gMono)DeleteObject(gMono);return (int)msg.wParam;
}
```
## ver2
```cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <direct.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

static const UINT WM_NET = WM_APP + 10;
static const UINT WM_VOICE = WM_APP + 11;

enum PacketType : uint32_t {
    PK_CHAT = 1,
    PK_FILE = 2,
    PK_VOICE = 3,
    PK_PING = 4
};

#pragma pack(push, 1)
struct PacketHeader {
    uint32_t type;
    uint32_t size;
};
#pragma pack(pop)

static HWND gMain = nullptr;
static HWND gFiles = nullptr;
static HWND gEditor = nullptr;
static HWND gHistory = nullptr;
static HWND gOutput = nullptr;
static HWND gChatLog = nullptr;
static HWND gChatInput = nullptr;
static HWND gHostInput = nullptr;
static HWND gPortInput = nullptr;
static HWND gStatus = nullptr;
static HFONT gFont = nullptr;
static HFONT gMono = nullptr;

static fs::path gProject;
static std::string gCurrentFile;
static std::vector<std::string> gFilesList;
static std::vector<std::string> gSnapshots;
static std::mutex gNetMutex;
static SOCKET gPeer = INVALID_SOCKET;
static SOCKET gListen = INVALID_SOCKET;
static std::thread gNetThread;
static std::atomic<bool> gRunning{false};
static std::atomic<bool> gListening{false};
static std::atomic<bool> gVoice{false};

static HWAVEIN gWaveIn = nullptr;
static HWAVEOUT gWaveOut = nullptr;
static std::vector<WAVEHDR*> gInHeaders;
static std::mutex gWaveMutex;

static std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), r.data(), n, nullptr, nullptr);
    return r;
}
static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    if (n <= 0) return L"";
    std::wstring r(n, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), r.data(), n);
    return r;
}
static std::string GetText(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring w(n + 1, L'\0');
    if (n) GetWindowTextW(h, w.data(), n + 1);
    w.resize(n);
    return WideToUtf8(w);
}
static void SetText(HWND h, const std::string& s) {
    SetWindowTextW(h, Utf8ToWide(s).c_str());
}
static void AppendText(HWND h, const std::string& s) {
    int n = GetWindowTextLengthW(h);
    SendMessageW(h, EM_SETSEL, n, n);
    SendMessageW(h, EM_REPLACESEL, FALSE, (LPARAM)Utf8ToWide(s).c_str());
}
static std::string NowStamp() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream o;
    o << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return o.str();
}
static void Status(const std::string& s) { if (gStatus) SetText(gStatus, s); }
static void EnsureDir(const fs::path& p) { std::error_code ec; fs::create_directories(p, ec); }

static void RefreshFiles() {
    if (!gFiles) return;
    SendMessageW(gFiles, LB_RESETCONTENT, 0, 0);
    gFilesList.clear();
    if (gProject.empty()) return;
    std::error_code ec;
    for (auto& e : fs::recursive_directory_iterator(gProject, ec)) {
        if (ec) break;
        if (!e.is_regular_file()) continue;
        auto rel = fs::relative(e.path(), gProject, ec).generic_string();
        if (rel.rfind(".slce/", 0) == 0 || rel == ".slce") continue;
        gFilesList.push_back(rel);
    }
    std::sort(gFilesList.begin(), gFilesList.end());
    for (auto& f : gFilesList) SendMessageW(gFiles, LB_ADDSTRING, 0, (LPARAM)Utf8ToWide(f).c_str());
}

static bool ReadFileUtf8(const fs::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return true;
}
static bool WriteFileUtf8(const fs::path& p, const std::string& s) {
    EnsureDir(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    if (!f) return false;
    f.write(s.data(), (std::streamsize)s.size());
    return !!f;
}

static std::string DiffLines(const std::string& a, const std::string& b) {
    std::vector<std::string> A, B;
    std::stringstream sa(a), sb(b); std::string x;
    while (std::getline(sa, x)) A.push_back(x);
    while (std::getline(sb, x)) B.push_back(x);
    std::vector<std::vector<int>> dp(A.size()+1, std::vector<int>(B.size()+1, 0));
    for (int i=(int)A.size()-1;i>=0;--i)
        for (int j=(int)B.size()-1;j>=0;--j)
            dp[i][j] = (A[i]==B[j]) ? dp[i+1][j+1]+1 : std::max(dp[i+1][j], dp[i][j+1]);
    std::ostringstream o;
    o << "--- SLCE DIFF ---\n";
    size_t i=0,j=0;
    while (i<A.size() || j<B.size()) {
        if (i<A.size() && j<B.size() && A[i]==B[j]) { o << "  " << A[i] << "\n"; ++i; ++j; }
        else if (j<B.size() && (i==A.size() || dp[i][j+1] >= dp[i+1][j])) { o << "+ " << B[j++] << "\n"; }
        else { o << "- " << A[i++] << "\n"; }
    }
    return o.str();
}

static std::string RenderMarkdown(const std::string& md) {
    std::stringstream in(md); std::string line; std::ostringstream out;
    bool code=false;
    while (std::getline(in,line)) {
        if (line.rfind("```",0)==0) { code=!code; out << (code?"[code]\n":"[/code]\n"); continue; }
        if (code) { out << "    " << line << "\n"; continue; }
        if (line.rfind("### ",0)==0) out << "### " << line.substr(4) << "\n";
        else if (line.rfind("## ",0)==0) out << "## " << line.substr(3) << "\n";
        else if (line.rfind("# ",0)==0) out << "# " << line.substr(2) << "\n";
        else if (line.rfind("- ",0)==0 || line.rfind("* ",0)==0) out << "• " << line.substr(2) << "\n";
        else out << line << "\n";
    }
    return out.str();
}

static void SaveCurrentFile(bool broadcast);

static bool SendAll(SOCKET s, const char* data, int len) {
    while (len > 0) {
        int n = send(s, data, len, 0);
        if (n <= 0) return false;
        data += n; len -= n;
    }
    return true;
}
static bool RecvAll(SOCKET s, char* data, int len) {
    while (len > 0) {
        int n = recv(s, data, len, 0);
        if (n <= 0) return false;
        data += n; len -= n;
    }
    return true;
}
static bool SendPacket(uint32_t type, const std::string& payload) {
    std::lock_guard<std::mutex> lock(gNetMutex);
    if (gPeer == INVALID_SOCKET) return false;
    PacketHeader h{type, (uint32_t)payload.size()};
    return SendAll(gPeer, (const char*)&h, sizeof(h)) &&
           (payload.empty() || SendAll(gPeer, payload.data(), (int)payload.size()));
}
static void NetUiText(const std::string& s) {
    if (gMain) PostMessageA(gMain, WM_NET, 0, (LPARAM)new std::string(s));
}

static void PlayVoice(const std::string& pcm) {
    std::lock_guard<std::mutex> lock(gWaveMutex);
    if (!gWaveOut) {
        WAVEFORMATEX fmt{};
        fmt.wFormatTag=WAVE_FORMAT_PCM; fmt.nChannels=1; fmt.nSamplesPerSec=8000;
        fmt.wBitsPerSample=16; fmt.nBlockAlign=2; fmt.nAvgBytesPerSec=16000;
        if (waveOutOpen(&gWaveOut, WAVE_MAPPER, &fmt, (DWORD_PTR)nullptr, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
    }
    char* mem = new char[pcm.size()]; memcpy(mem, pcm.data(), pcm.size());
    auto* hdr = new WAVEHDR{};
    hdr->lpData=mem; hdr->dwBufferLength=(DWORD)pcm.size();
    if (waveOutPrepareHeader(gWaveOut, hdr, sizeof(*hdr)) == MMSYSERR_NOERROR) waveOutWrite(gWaveOut, hdr, sizeof(*hdr));
    else { delete[] mem; delete hdr; }
}

static void NetReceiver(SOCKET s) {
    gRunning=true;
    NetUiText("[P2P] connected");
    while (gRunning) {
        PacketHeader h{};
        if (!RecvAll(s,(char*)&h,sizeof(h))) break;
        if (h.size > 32*1024*1024) break;
        std::string payload(h.size,'\0');
        if (h.size && !RecvAll(s,payload.data(),(int)h.size)) break;
        if (h.type==PK_CHAT) {
            NetUiText("[CHAT] "+payload);
        } else if (h.type==PK_FILE) {
            auto p=payload.find('\0');
            if (p!=std::string::npos && !gProject.empty()) {
                std::string name=payload.substr(0,p), data=payload.substr(p+1);
                WriteFileUtf8(gProject/fs::path(Utf8ToWide(name)),data);
                RefreshFiles();
                if (name==gCurrentFile) SetText(gEditor,data);
                NetUiText("[SYNC] "+name);
            }
        } else if (h.type==PK_VOICE) {
            PlayVoice(payload);
        } else if (h.type==PK_PING) {
            SendPacket(PK_PING,"pong");
        }
    }
    {
        std::lock_guard<std::mutex> lock(gNetMutex);
        if (gPeer==s) { closesocket(gPeer); gPeer=INVALID_SOCKET; }
    }
    gRunning=false;
    NetUiText("[P2P] disconnected");
}

static bool ConnectTo(const std::string& host, unsigned short port) {
    if (gPeer!=INVALID_SOCKET) return false;
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(s==INVALID_SOCKET) return false;
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port);
    if (inet_pton(AF_INET,host.c_str(),&a.sin_addr)<=0) { closesocket(s); return false; }
    if (connect(s,(sockaddr*)&a,sizeof(a))==SOCKET_ERROR) { closesocket(s); return false; }
    gPeer=s;
    if (gNetThread.joinable()) gNetThread.detach();
    gNetThread=std::thread(NetReceiver,s);
    return true;
}
static void ListenThread(unsigned short port) {
    SOCKET ls=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(ls==INVALID_SOCKET) return;
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_addr.s_addr=INADDR_ANY; a.sin_port=htons(port);
    int yes=1; setsockopt(ls,SOL_SOCKET,SO_REUSEADDR,(char*)&yes,sizeof(yes));
    if(bind(ls,(sockaddr*)&a,sizeof(a))==SOCKET_ERROR || listen(ls,1)==SOCKET_ERROR){closesocket(ls);return;}
    gListen=ls; gListening=true; NetUiText("[P2P] listening on port "+std::to_string(port));
    SOCKET s=accept(ls,nullptr,nullptr);
    if(s!=INVALID_SOCKET){
        {std::lock_guard<std::mutex> lock(gNetMutex); gPeer=s;}
        if(gNetThread.joinable()) gNetThread.detach();
        gNetThread=std::thread(NetReceiver,s);
    }
    closesocket(ls); gListen=INVALID_SOCKET; gListening=false;
}

static void CALLBACK VoiceInputProc(HWAVEIN wi, UINT msg, DWORD_PTR instance, DWORD_PTR p1, DWORD_PTR p2) {
    (void)wi;(void)p2;
    if(msg!=WIM_DATA) return;
    WAVEHDR* hdr=(WAVEHDR*)p1;
    if(gVoice && hdr && hdr->dwBytesRecorded) SendPacket(PK_VOICE,std::string(hdr->lpData,hdr->dwBytesRecorded));
    if(gVoice && gWaveIn) { hdr->dwBytesRecorded=0; waveInAddBuffer(gWaveIn,hdr,sizeof(*hdr)); }
}

static bool StartVoice() {
    if(gVoice) return true;
    WAVEFORMATEX fmt{};
    fmt.wFormatTag=WAVE_FORMAT_PCM;fmt.nChannels=1;fmt.nSamplesPerSec=8000;fmt.wBitsPerSample=16;fmt.nBlockAlign=2;fmt.nAvgBytesPerSec=16000;
    if(waveInOpen(&gWaveIn,WAVE_MAPPER,&fmt,(DWORD_PTR)VoiceInputProc,0,CALLBACK_FUNCTION)!=MMSYSERR_NOERROR) return false;
    const int N=4, SZ=3200;
    gInHeaders.clear();
    for(int i=0;i<N;i++){
        auto* h=new WAVEHDR{}; h->lpData=new char[SZ]; h->dwBufferLength=SZ;
        if(waveInPrepareHeader(gWaveIn,h,sizeof(*h))!=MMSYSERR_NOERROR || waveInAddBuffer(gWaveIn,h,sizeof(*h))!=MMSYSERR_NOERROR){delete[] h->lpData;delete h;continue;}
        gInHeaders.push_back(h);
    }
    if(gInHeaders.empty()){waveInClose(gWaveIn);gWaveIn=nullptr;return false;}
    waveInStart(gWaveIn); gVoice=true; return true;
}
static void StopVoice() {
    gVoice=false;
    if(gWaveIn){
        waveInStop(gWaveIn); waveInReset(gWaveIn);
        for(auto* h:gInHeaders){waveInUnprepareHeader(gWaveIn,h,sizeof(*h));delete[] h->lpData;delete h;}
        gInHeaders.clear(); waveInClose(gWaveIn); gWaveIn=nullptr;
    }
}

static void CreateControls(HWND h) {
    gFiles=CreateWindowW(L"LISTBOX",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL,10,40,220,450,h,(HMENU)101,GetModuleHandle(nullptr),nullptr);
    gEditor=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL|WS_HSCROLL,240,40,620,450,h,(HMENU)102,GetModuleHandle(nullptr),nullptr);
    gHistory=CreateWindowW(L"LISTBOX",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL,870,40,250,200,h,(HMENU)103,GetModuleHandle(nullptr),nullptr);
    gOutput=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|WS_HSCROLL,870,250,250,240,h,(HMENU)104,GetModuleHandle(nullptr),nullptr);
    gChatLog=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,10,505,550,180,h,(HMENU)105,GetModuleHandle(nullptr),nullptr);
    gChatInput=CreateWindowW(L"EDIT",nullptr,WS_CHILD|WS_VISIBLE|WS_BORDER,570,505,300,30,h,(HMENU)106,GetModuleHandle(nullptr),nullptr);
    gHostInput=CreateWindowW(L"EDIT",L"127.0.0.1",WS_CHILD|WS_VISIBLE|WS_BORDER,10,695,150,28,h,(HMENU)107,GetModuleHandle(nullptr),nullptr);
    gPortInput=CreateWindowW(L"EDIT",L"7345",WS_CHILD|WS_VISIBLE|WS_BORDER,165,695,70,28,h,(HMENU)108,GetModuleHandle(nullptr),nullptr);
    gStatus=CreateWindowW(L"STATIC",L"Ready",WS_CHILD|WS_VISIBLE,250,700,700,25,h,(HMENU)109,GetModuleHandle(nullptr),nullptr);
    struct B{int id;int x;int w;const wchar_t* t;};
    B bs[]={{200,10,70,L"Open"},{201,85,95,L"Save"},{202,185,115,L"Snapshot"},{203,305,75,L"Diff"},{204,385,100,L"Markdown"},{205,495,85,L"Export"},{206,590,100,L"Listen"},{207,695,100,L"Connect"},{208,805,100,L"Voice"},{209,915,95,L"Send Chat"}};
    for(auto& b:bs)CreateWindowW(L"BUTTON",b.t,WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,b.x,8,b.w,26,h,(HMENU)b.id,GetModuleHandle(nullptr),nullptr);
}

static void OpenProject() {
    BROWSEINFOW bi{}; bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE; bi.lpszTitle=L"Select SLCE project folder";
    PIDLIST_ABSOLUTE pid=SHBrowseForFolderW(&bi); if(!pid)return;
    wchar_t buf[MAX_PATH]; if(SHGetPathFromIDListW(pid,buf)){
        gProject=buf; CoTaskMemFree(pid); EnsureDir(gProject/L".slce/history"); RefreshFiles(); Status("Project: "+gProject.string());
    } else CoTaskMemFree(pid);
}
static void OpenSelectedFile() {
    int i=(int)SendMessageW(gFiles,LB_GETCURSEL,0,0); if(i<0 || i>=(int)gFilesList.size())return;
    gCurrentFile=gFilesList[i]; std::string s; if(ReadFileUtf8(gProject/fs::path(Utf8ToWide(gCurrentFile)),s)){SetText(gEditor,s);Status("Editing: "+gCurrentFile);}
}
static void SaveCurrentFile(bool broadcast) {
    if(gProject.empty()||gCurrentFile.empty())return;
    std::string s=GetText(gEditor); WriteFileUtf8(gProject/fs::path(Utf8ToWide(gCurrentFile)),s); RefreshFiles(); Status("Saved: "+gCurrentFile);
    if(broadcast){std::string p=gCurrentFile; p.push_back('\0'); p+=s; SendPacket(PK_FILE,p);}
}
static void SaveSnapshot() {
    if(gProject.empty()||gCurrentFile.empty())return;
    std::string stamp=NowStamp(); fs::path p=gProject/L".slce/history"/(Utf8ToWide(stamp+"__"+gCurrentFile));
    WriteFileUtf8(p,GetText(gEditor));
    gSnapshots.push_back(stamp+"__"+gCurrentFile); SendMessageW(gHistory,LB_ADDSTRING,0,(LPARAM)Utf8ToWide(gSnapshots.back()).c_str());
    Status("Snapshot: "+stamp);
}
static std::string LastSnapshotContent() {
    if(gProject.empty()||gCurrentFile.empty())return {};
    std::string best; fs::file_time_type bt{}; bool has=false;
    std::error_code ec; fs::path dir=gProject/L".slce/history";
    for(auto&e:fs::directory_iterator(dir,ec)){
        if(ec||!e.is_regular_file())continue; auto n=e.path().filename().generic_string();
        auto pos=n.find("__"); if(pos==std::string::npos||n.substr(pos+2)!=gCurrentFile)continue;
        auto t=fs::last_write_time(e,ec); if(!has||t>bt){bt=t;has=true;ReadFileUtf8(e.path(),best);}
    }
    return best;
}
static void ShowDiff() { if(gCurrentFile.empty())return; SetText(gOutput,DiffLines(LastSnapshotContent(),GetText(gEditor))); }
static void ShowMarkdown() { SetText(gOutput,RenderMarkdown(GetText(gEditor))); }
static void ExportProject() {
    if(gProject.empty())return;
    BROWSEINFOW bi{}; bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE; bi.lpszTitle=L"Select export folder";
    PIDLIST_ABSOLUTE pid=SHBrowseForFolderW(&bi);if(!pid)return; wchar_t buf[MAX_PATH];
    if(SHGetPathFromIDListW(pid,buf)){
        fs::path dst=fs::path(buf)/(gProject.filename().string()+"_export"); std::error_code ec; fs::create_directories(dst,ec);
        for(auto&e:fs::recursive_directory_iterator(gProject,ec)){
            if(ec||!e.is_regular_file())continue; auto rel=fs::relative(e.path(),gProject,ec).generic_string(); if(rel.rfind(".slce/",0)==0)continue;
            fs::path to=dst/fs::path(Utf8ToWide(rel)); fs::create_directories(to.parent_path(),ec); fs::copy_file(e.path(),to,fs::copy_options::overwrite_existing,ec);
        }
        Status("Exported: "+dst.string());
    }
    CoTaskMemFree(pid);
}
static void InitHistory() {
    gSnapshots.clear(); SendMessageW(gHistory,LB_RESETCONTENT,0,0); if(gProject.empty())return;
    std::error_code ec; auto dir=gProject/L".slce/history"; EnsureDir(dir);
    for(auto&e:fs::directory_iterator(dir,ec)) if(e.is_regular_file()) gSnapshots.push_back(e.path().filename().generic_string());
    std::sort(gSnapshots.begin(),gSnapshots.end()); for(auto&s:gSnapshots)SendMessageW(gHistory,LB_ADDSTRING,0,(LPARAM)Utf8ToWide(s).c_str());
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch(m){
    case WM_CREATE:
        gMain=h; gFont=CreateFontW(16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,L"Segoe UI");
        gMono=CreateFontW(16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_MODERN,L"Consolas");
        CreateControls(h);
        for(HWND c:{gFiles,gEditor,gHistory,gOutput,gChatLog,gChatInput,gHostInput,gPortInput,gStatus})SendMessageW(c,WM_SETFONT,(WPARAM)(c==gEditor?gMono:gFont),TRUE);
        return 0;
    case WM_COMMAND:
        if(LOWORD(w)==101 && HIWORD(w)==LBN_SELCHANGE) OpenSelectedFile();
        else if(LOWORD(w)==200) OpenProject();
        else if(LOWORD(w)==201) SaveCurrentFile(true);
        else if(LOWORD(w)==202) SaveSnapshot();
        else if(LOWORD(w)==203) ShowDiff();
        else if(LOWORD(w)==204) ShowMarkdown();
        else if(LOWORD(w)==205) ExportProject();
        else if(LOWORD(w)==206){ unsigned short p=(unsigned short)std::stoi(GetText(gPortInput)); std::thread(ListenThread,p).detach(); }
        else if(LOWORD(w)==207){ unsigned short p=(unsigned short)std::stoi(GetText(gPortInput)); if(ConnectTo(GetText(gHostInput),p))Status("Connected");else Status("Connect failed"); }
        else if(LOWORD(w)==208){ if(gVoice){StopVoice();Status("Voice stopped");}else if(StartVoice())Status("Voice started");else Status("Voice failed"); }
        else if(LOWORD(w)==209){std::string s=GetText(gChatInput);if(!s.empty()){AppendText(gChatLog,"Me: "+s+"\r\n");SendPacket(PK_CHAT,s);SetText(gChatInput,"");}}
        return 0;
    case WM_NET:{auto* s=(std::string*)l;if(s){AppendText(gChatLog,*s+"\r\n");delete s;}return 0;}
    case WM_SIZE:{int W=LOWORD(l),H=HIWORD(l); if(!gFiles)return 0; MoveWindow(gFiles,10,40,220,H-300,TRUE);MoveWindow(gEditor,240,40,W-500,H-300,TRUE);MoveWindow(gHistory,W-250,40,240,200,TRUE);MoveWindow(gOutput,W-250,250,240,H-300,TRUE);MoveWindow(gChatLog,10,H-190,W-570,150,TRUE);MoveWindow(gChatInput,W-550,H-185,310,30,TRUE);MoveWindow(gHostInput,10,H-35,150,28,TRUE);MoveWindow(gPortInput,165,H-35,70,28,TRUE);MoveWindow(gStatus,250,H-35,W-260,25,TRUE);return 0;}
    case WM_DESTROY:
        StopVoice(); gRunning=false; if(gPeer!=INVALID_SOCKET){shutdown(gPeer,SD_BOTH);closesocket(gPeer);gPeer=INVALID_SOCKET;} if(gListen!=INVALID_SOCKET){closesocket(gListen);gListen=INVALID_SOCKET;} WSACleanup(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h,m,w,l);
}

int WINAPI wWinMain(HINSTANCE hInst,HINSTANCE,LPWSTR,int nCmdShow){
    WSADATA wd{}; if(WSAStartup(MAKEWORD(2,2),&wd)!=0){MessageBoxW(nullptr,L"WSAStartup failed",L"SLCE",MB_ICONERROR);return 1;}
    INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_STANDARD_CLASSES}; InitCommonControlsEx(&ic);
    WNDCLASSW wc{};wc.lpfnWndProc=WndProc;wc.hInstance=hInst;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"SLCEWindow";wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION);RegisterClassW(&wc);
    HWND h=CreateWindowW(wc.lpszClassName,L"SLCE — Save Code Locally, Collaborate, Tremendous, Easy",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1150,760,nullptr,nullptr,hInst,nullptr);
    if(!h)return 1; ShowWindow(h,nCmdShow); UpdateWindow(h);
    MSG msg; while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);} if(gFont)DeleteObject(gFont);if(gMono)DeleteObject(gMono);return (int)msg.wParam;
}
```
