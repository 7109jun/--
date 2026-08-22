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
## ver3
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
### ver4
```cpp
// SLCE - Save Code Locally, Collaborate, Tremendous, Easy
// Single-file / no third-party libraries.
// Linux: g++ -std=c++17 -O2 -pthread slce.cpp -lX11 -o slce
// Windows: cl /std:c++17 /O2 slce.cpp user32.lib gdi32.lib ws2_32.lib winmm.lib

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <deque>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#endif

namespace fs = std::filesystem;

static constexpr uint32_t MAGIC = 0x534C4345u;
static constexpr uint32_t PROTO = 4;

static constexpr uint32_t PK_AUTH_ID = 1;
static constexpr uint32_t PK_AUTH_CHALLENGE = 2;
static constexpr uint32_t PK_AUTH_PASS = 3;
static constexpr uint32_t PK_AUTH_OK = 4;
static constexpr uint32_t PK_AUTH_FAIL = 5;
static constexpr uint32_t PK_CHAT = 10;
static constexpr uint32_t PK_CRDT_OP = 11;
static constexpr uint32_t PK_CRDT_STATE = 12;
static constexpr uint32_t PK_CURSOR = 13;
static constexpr uint32_t PK_VOICE = 14;
static constexpr uint32_t PK_BYE = 15;

static constexpr uint64_t MAX_PACKET = 32ull * 1024ull * 1024ull;

static std::string nowStamp() {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());

    std::tm tm{};

#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    char b[64];
    std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &tm);
    return b;
}

static std::string randomHex(size_t n = 12) {
    static std::mt19937_64 r(
        (uint64_t)std::chrono::high_resolution_clock::now()
            .time_since_epoch()
            .count());

    std::ostringstream o;

    while (o.tellp() < (std::streamoff)n)
        o << std::hex << std::setw(16)
          << std::setfill('0') << r();

    auto s = o.str();
    return s.substr(0, n);
}

static std::string esc(const std::string& s) {
    std::string r;

    for (unsigned char c : s) {
        if (c == '\\' || c == '\n' || c == '\r' || c == '\t')
            r.push_back('\\');

        if (c == '\n')
            r.push_back('n');
        else if (c == '\r')
            r.push_back('r');
        else if (c == '\t')
            r.push_back('t');
        else
            r.push_back((char)c);
    }

    return r;
}

static std::string unesc(const std::string& s) {
    std::string r;
    bool e = false;

    for (char c : s) {
        if (!e && c == '\\') {
            e = true;
            continue;
        }

        if (e) {
            if (c == 'n')
                r += '\n';
            else if (c == 'r')
                r += '\r';
            else if (c == 't')
                r += '\t';
            else
                r += c;

            e = false;
        } else {
            r += c;
        }
    }

    if (e)
        r += '\\';

    return r;
}

static bool readFile(const fs::path& p, std::string& o) {
    std::ifstream f(p, std::ios::binary);

    if (!f)
        return false;

    o.assign(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());

    return true;
}

static bool writeFile(const fs::path& p, const std::string& s) {
    std::error_code ec;

    if (!p.parent_path().empty())
        fs::create_directories(p.parent_path(), ec);

    std::ofstream f(
        p,
        std::ios::binary | std::ios::trunc);

    if (!f)
        return false;

    f.write(s.data(), (std::streamsize)s.size());
    return (bool)f;
}

static std::vector<std::string> splitLines(
    const std::string& s) {

    std::vector<std::string> r;
    std::stringstream q(s);
    std::string l;

    while (std::getline(q, l))
        r.push_back(l);

    if (r.empty() || (!s.empty() && s.back() == '\n'))
        r.push_back("");

    return r;
}

static bool validPassword(const std::string& p) {
    if (p.size() != 12)
        return false;

    for (char c : p)
        if (c < '0' || c > '9')
            return false;

    return true;
}

static std::string unifiedDiff(
    const std::string& a,
    const std::string& b) {

    auto A = splitLines(a);
    auto B = splitLines(b);

    size_t n = A.size();
    size_t m = B.size();

    std::vector<std::vector<uint32_t>> dp(
        n + 1,
        std::vector<uint32_t>(m + 1));

    for (size_t i = n; i-- > 0;) {
        for (size_t j = m; j-- > 0;) {
            dp[i][j] =
                (A[i] == B[j])
                    ? dp[i + 1][j + 1] + 1
                    : std::max(dp[i + 1][j],
                               dp[i][j + 1]);
        }
    }

    std::ostringstream o;

    o << "--- before\n";
    o << "+++ after\n";

    size_t i = 0;
    size_t j = 0;

    while (i < n || j < m) {
        if (i < n && j < m && A[i] == B[j]) {
            o << "  " << A[i] << "\n";
            ++i;
            ++j;
        } else if (
            j < m &&
            (i == n ||
             dp[i][j + 1] >= dp[i + 1][j])) {

            o << "+ " << B[j++] << "\n";
        } else {
            o << "- " << A[i++] << "\n";
        }
    }

    return o.str();
}

/*
    CRDT

    SLCE uses a deterministic LWW-Register CRDT per file.

    Version ordering:
        (Lamport clock, actor ID)

    This provides:
        - deterministic convergence
        - idempotent merge
        - commutative merge
        - associative merge
*/
struct CrdtVersion {
    uint64_t clock = 0;
    std::string actor;
    std::string text;
};

struct CrdtFile {
    CrdtVersion v;
};

struct CrdtOp {
    std::string path;
    uint64_t clock = 0;
    std::string actor;
    std::string text;
};

static bool newer(
    const CrdtVersion& a,
    const CrdtVersion& b) {

    return
        a.clock > b.clock ||
        (a.clock == b.clock &&
         a.actor > b.actor);
}

static std::vector<CrdtOp> makeOp(
    std::map<std::string, CrdtFile>& crdt,
    const std::string& path,
    const std::string& text,
    const std::string& actor,
    uint64_t& seq) {

    uint64_t c = ++seq;

    auto& f = crdt[path];

    c = std::max(c, f.v.clock + 1);

    f.v = {c, actor, text};

    return {
        {path, c, actor, text}
    };
}

static void applyOp(
    std::map<std::string, CrdtFile>& crdt,
    const CrdtOp& o) {

    auto& f = crdt[o.path];

    CrdtVersion v{
        o.clock,
        o.actor,
        o.text
    };

    if (newer(v, f.v) || f.v.clock == 0)
        f.v = v;
}

static std::string serializeOps(
    const std::vector<CrdtOp>& ops) {

    std::ostringstream o;

    for (auto& a : ops) {
        o
            << esc(a.path) << '\t'
            << a.clock << '\t'
            << esc(a.actor) << '\t'
            << esc(a.text) << "\n";
    }

    return o.str();
}

static std::vector<CrdtOp> parseOps(
    const std::string& s) {

    std::vector<CrdtOp> r;
    std::stringstream q(s);
    std::string l;

    while (std::getline(q, l)) {
        std::stringstream z(l);

        std::string a, b, c, d;

        if (!std::getline(z, a, '\t') ||
            !std::getline(z, b, '\t') ||
            !std::getline(z, c, '\t') ||
            !std::getline(z, d))
            continue;

        r.push_back({
            unesc(a),
            std::stoull(b),
            unesc(c),
            unesc(d)
        });
    }

    return r;
}

static std::string serializeCrdt(
    const std::map<std::string, CrdtFile>& m) {

    std::ostringstream o;

    for (auto& [p, f] : m) {
        o
            << esc(p) << '\t'
            << f.v.clock << '\t'
            << esc(f.v.actor) << '\t'
            << esc(f.v.text) << "\n";
    }

    return o.str();
}

static std::map<std::string, CrdtFile> parseCrdt(
    const std::string& s) {

    std::map<std::string, CrdtFile> r;
    std::stringstream q(s);
    std::string l;

    while (std::getline(q, l)) {
        std::stringstream z(l);

        std::string a, b, c, d;

        if (!std::getline(z, a, '\t') ||
            !std::getline(z, b, '\t') ||
            !std::getline(z, c, '\t') ||
            !std::getline(z, d))
            continue;

        r[unesc(a)].v = {
            std::stoull(b),
            unesc(c),
            unesc(d)
        };
    }

    return r;
}

/*
    Simple syntax/error analysis.

    The editor marks lines containing:
      - unmatched brackets
      - unterminated strings
*/
static std::vector<int> errorLines(
    const std::string& src) {

    std::vector<int> bad;
    std::vector<std::pair<char, int>> st;

    bool inS = false;
    bool inD = false;
    bool escp = false;

    int line = 1;

    std::set<int> bads;

    for (char c : src) {
        if (c == '\n') {
            if (inS || inD)
                bads.insert(line);

            line++;
            escp = false;
            continue;
        }

        if (escp) {
            escp = false;
            continue;
        }

        if (c == '\\' && (inS || inD)) {
            escp = true;
            continue;
        }

        if (!inD && c == '\'') {
            inS = !inS;
            continue;
        }

        if (!inS && c == '"') {
            inD = !inD;
            continue;
        }

        if (inS || inD)
            continue;

        if (c == '(' || c == '[' || c == '{')
            st.push_back({c, line});
        else if (
            c == ')' ||
            c == ']' ||
            c == '}') {

            char o =
                c == ')' ? '(' :
                c == ']' ? '[' : '{';

            if (
                st.empty() ||
                st.back().first != o) {

                bads.insert(line);

                if (!st.empty())
                    bads.insert(st.back().second);
            } else {
                st.pop_back();
            }
        }
    }

    if (inS || inD)
        bads.insert(line);

    for (auto& x : st)
        bads.insert(x.second);

    for (int x : bads)
        bad.push_back(x);

    std::sort(bad.begin(), bad.end());

    return bad;
}

struct Token {
    std::string s;
    int kind = 0;
};

/*
    Token kinds:
      0 normal
      1 keyword
      2 string
      3 comment
      4 number
*/
static std::vector<Token> highlight(
    const std::string& line) {

    static const std::set<std::string> kw = {
        "if",
        "else",
        "for",
        "while",
        "return",
        "class",
        "struct",
        "namespace",
        "using",
        "public",
        "private",
        "protected",
        "static",
        "const",
        "auto",
        "void",
        "int",
        "long",
        "short",
        "float",
        "double",
        "char",
        "bool",
        "true",
        "false",
        "nullptr",
        "include",
        "define",
        "new",
        "delete",
        "switch",
        "case",
        "break",
        "continue",
        "template",
        "typename",
        "virtual",
        "override"
    };

    std::vector<Token> r;

    for (size_t i = 0; i < line.size();) {
        if (
            i + 1 < line.size() &&
            line[i] == '/' &&
            line[i + 1] == '/') {

            r.push_back({
                line.substr(i),
                3
            });

            break;
        }

        if (
            line[i] == '"' ||
            line[i] == '\'') {

            char q = line[i];

            size_t j = i + 1;
            bool e = false;

            for (; j < line.size(); j++) {
                if (e) {
                    e = false;
                    continue;
                }

                if (line[j] == '\\') {
                    e = true;
                    continue;
                }

                if (line[j] == q) {
                    j++;
                    break;
                }
            }

            r.push_back({
                line.substr(i, j - i),
                2
            });

            i = j;
            continue;
        }

        if (std::isdigit(
                (unsigned char)line[i])) {

            size_t j = i + 1;

            while (
                j < line.size() &&
                (std::isalnum(
                     (unsigned char)line[j]) ||
                 line[j] == '.' ||
                 line[j] == '_'))
                j++;

            r.push_back({
                line.substr(i, j - i),
                4
            });

            i = j;
            continue;
        }

        if (
            std::isalpha(
                (unsigned char)line[i]) ||
            line[i] == '_') {

            size_t j = i + 1;

            while (
                j < line.size() &&
                (std::isalnum(
                     (unsigned char)line[j]) ||
                 line[j] == '_'))
                j++;

            std::string w =
                line.substr(i, j - i);

            r.push_back({
                w,
                kw.count(w) ? 1 : 0
            });

            i = j;
            continue;
        }

        r.push_back({
            std::string(1, line[i]),
            0
        });

        i++;
    }

    return r;
}

class Net {
public:
    using CB =
        std::function<
            void(uint32_t,
                 const std::string&,
                 int)>;

private:
    std::atomic<bool> run{false};

    int lf = -1;

    std::thread at;

    std::mutex mx;

    std::vector<int> peers;

    CB cb;

    static void closefd(int s) {
#if defined(_WIN32)
        closesocket(s);
#else
        shutdown(s, SHUT_RDWR);
        close(s);
#endif
    }

    static bool sendAll(
        int s,
        const void* d,
        size_t n) {

        const char* p =
            (const char*)d;

        while (n) {
#if defined(_WIN32)
            int z =
                send(
                    s,
                    p,
                    (int)n,
                    0);
#else
            ssize_t z =
                send(
                    s,
                    p,
                    n,
                    MSG_NOSIGNAL);
#endif

            if (z <= 0)
                return false;

            p += z;
            n -= z;
        }

        return true;
    }

    static bool recvAll(
        int s,
        void* d,
        size_t n) {

        char* p =
            (char*)d;

        while (n) {
#if defined(_WIN32)
            int z =
                recv(
                    s,
                    p,
                    (int)n,
                    0);
#else
            ssize_t z =
                recv(
                    s,
                    p,
                    n,
                    0);
#endif

            if (z <= 0)
                return false;

            p += z;
            n -= z;
        }

        return true;
    }

    static bool sendPkt(
        int s,
        uint32_t t,
        const std::string& p) {

        uint32_t h[3] = {
            MAGIC,
            PROTO,
            t
        };

        uint64_t n = p.size();

        return
            sendAll(s, h, sizeof(h)) &&
            sendAll(s, &n, sizeof(n)) &&
            (n
                ? sendAll(
                    s,
                    p.data(),
                    p.size())
                : true);
    }

    static bool recvPkt(
        int s,
        uint32_t& t,
        std::string& p) {

        uint32_t h[3];
        uint64_t n;

        if (!recvAll(
                s,
                h,
                sizeof(h)) ||
            !recvAll(
                s,
                &n,
                sizeof(n)))
            return false;

        if (
            h[0] != MAGIC ||
            h[1] != PROTO ||
            n > MAX_PACKET)
            return false;

        t = h[2];

        p.resize((size_t)n);

        return n
            ? recvAll(
                s,
                p.data(),
                p.size())
            : true;
    }

    void loop(int s) {
        while (run) {
            uint32_t t;
            std::string p;

            if (!recvPkt(s, t, p))
                break;

            if (cb)
                cb(t, p, s);
        }

        closefd(s);

        {
            std::lock_guard<std::mutex> g(mx);

            peers.erase(
                std::remove(
                    peers.begin(),
                    peers.end(),
                    s),
                peers.end());
        }

        if (cb)
            cb(PK_BYE, "", s);
    }

    void acceptLoop() {
        while (run) {
            sockaddr_in a{};

#if defined(_WIN32)
            int z = sizeof(a);
#else
            socklen_t z = sizeof(a);
#endif

            int s =
                (int)accept(
                    lf,
                    (sockaddr*)&a,
                    &z);

            if (s < 0) {
                if (run && cb)
                    cb(
                        PK_BYE,
                        "accept failed",
                        -1);

                continue;
            }

            {
                std::lock_guard<std::mutex> g(mx);
                peers.push_back(s);
            }

            std::thread(
                &Net::loop,
                this,
                s).detach();
        }
    }

public:
    ~Net() {
        stop();
    }

    void setCallback(CB x) {
        cb = std::move(x);
    }

    bool listenPort(uint16_t port) {
        stop();

#if defined(_WIN32)
        WSADATA w{};
        if (WSAStartup(
                MAKEWORD(2, 2),
                &w) != 0)
            return false;
#endif

        lf =
            (int)socket(
                AF_INET,
                SOCK_STREAM,
                0);

        if (lf < 0)
            return false;

        int one = 1;

        setsockopt(
            lf,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char*)&one,
            sizeof(one));

        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr =
            htonl(INADDR_ANY);
        a.sin_port =
            htons(port);

        if (
            bind(
                lf,
                (sockaddr*)&a,
                sizeof(a)) < 0 ||
            listen(lf, 16) < 0) {

            closefd(lf);
            lf = -1;
            return false;
        }

        run = true;

        at =
            std::thread(
                &Net::acceptLoop,
                this);

        return true;
    }

    bool connectTo(
        const std::string& host,
        uint16_t port) {

#if defined(_WIN32)
        WSADATA w{};
        WSAStartup(
            MAKEWORD(2, 2),
            &w);
#endif

        int s =
            (int)socket(
                AF_INET,
                SOCK_STREAM,
                0);

        if (s < 0)
            return false;

        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_port = htons(port);

        if (
            inet_pton(
                AF_INET,
                host.c_str(),
                &a.sin_addr) <= 0) {

            closefd(s);
            return false;
        }

        if (
            connect(
                s,
                (sockaddr*)&a,
                sizeof(a)) < 0) {

            closefd(s);
            return false;
        }

        run = true;

        {
            std::lock_guard<std::mutex> g(mx);
            peers.push_back(s);
        }

        std::thread(
            &Net::loop,
            this,
            s).detach();

        return true;
    }

    void broadcast(
        uint32_t type,
        const std::string& payload,
        int except = -1) {

        std::lock_guard<std::mutex> g(mx);

        for (int s : peers) {
            if (s == except)
                continue;

            sendPkt(
                s,
                type,
                payload);
        }
    }

    void sendTo(
        int s,
        uint32_t type,
        const std::string& payload) {

        sendPkt(
            s,
            type,
            payload);
    }

    void stop() {
        bool was = run.exchange(false);

        if (!was && lf < 0)
            return;

        if (lf >= 0) {
            closefd(lf);
            lf = -1;
        }

        std::vector<int> copy;

        {
            std::lock_guard<std::mutex> g(mx);
            copy = peers;
            peers.clear();
        }

        for (int s : copy)
            closefd(s);

        if (
            at.joinable() &&
            at.get_id() !=
                std::this_thread::get_id())
            at.join();

#if defined(_WIN32)
        WSACleanup();
#endif
    }
};

class Project {
    fs::path root;

    std::string myId =
        "user-" + randomHex(6);

    std::string password =
        "000000000000";

    uint64_t clock = 0;

    std::map<
        std::string,
        CrdtFile> crdt;

    std::map<
        std::string,
        std::string> current;

    std::vector<
        std::map<
            std::string,
            std::string>> history;

    Net net;

    std::function<
        void(const std::string&)> event;

    std::mutex mx;

    std::map<int, bool> authed;

    std::map<int, std::string> peerIds;

    void signal(
        const std::string& s) {

        if (event)
            event(s);
    }

    fs::path meta() const {
        return root / ".slce";
    }

    void saveIdentity() {
        fs::create_directories(meta());

        std::ofstream f(
            meta() / "identity.txt",
            std::ios::trunc);

        f << esc(myId) << "\n";
        f << password << "\n";
    }

    void loadIdentity() {
        std::ifstream f(
            meta() / "identity.txt");

        if (!f)
            return;

        std::string id;
        std::string pw;

        std::getline(f, id);
        std::getline(f, pw);

        if (!id.empty())
            myId = unesc(id);

        if (validPassword(pw))
            password = pw;
    }

    void persistCrdt() {
        fs::create_directories(meta());

        writeFile(
            meta() / "crdt.db",
            serializeCrdt(crdt));
    }

    void loadCrdt() {
        std::string s;

        if (
            readFile(
                meta() / "crdt.db",
                s))
            crdt = parseCrdt(s);
    }

    void persistHistory() {
        fs::create_directories(
            meta() / "history");

        int n = 0;

        for (auto& snap : history) {
            std::ostringstream o;

            for (auto& [p, c] : snap) {
                o
                    << esc(p)
                    << "\t"
                    << esc(c)
                    << "\n";
            }

            writeFile(
                meta() /
                    "history" /
                    (std::to_string(n++) +
                     ".snapshot"),
                o.str());
        }
    }

    void rebuildCurrent() {
        current.clear();

        for (auto& [p, f] : crdt)
            current[p] = f.v.text;
    }

    void onNet(
        uint32_t t,
        const std::string& p,
        int s) {

        if (t == PK_AUTH_ID) {
            std::string id = unesc(p);

            peerIds[s] = id;

            net.sendTo(
                s,
                PK_AUTH_CHALLENGE,
                esc(myId));

            signal(
                "[AUTH] ID received: " +
                id);

            return;
        }

        if (t == PK_AUTH_CHALLENGE) {
            net.sendTo(
                s,
                PK_AUTH_PASS,
                password);

            signal(
                "[AUTH] password requested");

            return;
        }

        if (t == PK_AUTH_PASS) {
            if (!validPassword(p) ||
                p != password) {

                net.sendTo(
                    s,
                    PK_AUTH_FAIL,
                    "wrong-password");

                signal(
                    "[AUTH] rejected");

                return;
            }

            authed[s] = true;

            net.sendTo(
                s,
                PK_AUTH_OK,
                esc(myId));

            net.sendTo(
                s,
                PK_CRDT_STATE,
                serializeCrdt(crdt));

            signal(
                "[AUTH] peer authenticated");

            return;
        }

        if (t == PK_AUTH_OK) {
            authed[s] = true;

            signal(
                "[AUTH] authenticated: " +
                unesc(p));

            net.sendTo(
                s,
                PK_CRDT_STATE,
                serializeCrdt(crdt));

            return;
        }

        if (t == PK_AUTH_FAIL) {
            signal(
                "[AUTH] failed: wrong 12-digit password");

            return;
        }

        if (!authed[s])
            return;

        if (t == PK_CRDT_OP) {
            auto ops = parseOps(p);

            for (auto& o : ops) {
                bool changed = false;
                std::string content;

                {
                    std::lock_guard<
                        std::mutex> g(mx);

                    auto before =
                        crdt[o.path].v;

                    CrdtVersion incoming{
                        o.clock,
                        o.actor,
                        o.text
                    };

                    if (
                        newer(
                            incoming,
                            before) ||
                        before.clock == 0) {

                        applyOp(
                            crdt,
                            o);

                        current[o.path] =
                            crdt[o.path].v.text;

                        content =
                            current[o.path];

                        changed = true;
                    }

                    clock =
                        std::max(
                            clock,
                            o.clock);
                }

                if (changed) {
                    writeFile(
                        root / o.path,
                        content);

                    signal(
                        "[CRDT] merged " +
                        o.path);
                }
            }

            persistCrdt();
            return;
        }

        if (t == PK_CRDT_STATE) {
            auto remote =
                parseCrdt(p);

            for (auto& [path, rf] :
                 remote) {

                bool changed = false;
                std::string content;

                {
                    std::lock_guard<
                        std::mutex> g(mx);

                    if (
                        newer(
                            rf.v,
                            crdt[path].v) ||
                        crdt[path].v.clock == 0) {

                        crdt[path] = rf;

                        current[path] =
                            rf.v.text;

                        content =
                            current[path];

                        clock =
                            std::max(
                                clock,
                                rf.v.clock);

                        changed = true;
                    }
                }

                if (changed) {
                    writeFile(
                        root / path,
                        content);
                }
            }

            persistCrdt();

            signal(
                "[CRDT] state merged");

            return;
        }

        if (t == PK_CHAT) {
            auto x =
                p.find('\t');

            std::string who =
                x == std::string::npos
                    ? "peer"
                    : unesc(
                        p.substr(
                            0,
                            x));

            std::string m =
                x == std::string::npos
                    ? p
                    : unesc(
                        p.substr(
                            x + 1));

            signal(
                "[CHAT] " +
                who +
                ": " +
                m);

            return;
        }

        if (t == PK_BYE) {
            authed.erase(s);

            signal(
                "[P2P] peer disconnected");

            return;
        }
    }

public:
    Project() {
        net.setCallback(
            [this](
                uint32_t t,
                const std::string& p,
                int s) {
                onNet(t, p, s);
            });
    }

    ~Project() {
        stop();
    }

    void setEvent(
        std::function<
            void(const std::string&)> e) {
        event = std::move(e);
    }

    void configureIdentity(
        const std::string& id,
        const std::string& pw) {

        if (id.empty())
            return;

        if (!validPassword(pw))
            return;

        myId = id;
        password = pw;

        saveIdentity();
    }

    bool open(
        const fs::path& p) {

        root = p;

        std::error_code ec;

        fs::create_directories(
            root,
            ec);

        if (ec)
            return false;

        loadIdentity();
        loadCrdt();
        rebuildCurrent();

        for (
            auto it =
                fs::recursive_directory_iterator(
                    root,
                    ec);
            it !=
                fs::recursive_directory_iterator();
            ++it) {

            if (ec)
                break;

            if (!it->is_regular_file())
                continue;

            auto rel =
                fs::relative(
                    it->path(),
                    root,
                    ec);

            if (ec)
                continue;

            if (
                rel.string().rfind(
                    ".slce",
                    0) == 0)
                continue;

            std::string s;

            if (readFile(
                    it->path(),
                    s)) {

                if (
                    current.find(
                        rel.string()) ==
                    current.end()) {

                    current[
                        rel.string()] = s;

                    makeOp(
                        crdt,
                        rel.string(),
                        s,
                        myId,
                        clock);
                }
            }
        }

        persistCrdt();

        return true;
    }

    std::string id() const {
        return myId;
    }

    bool put(
        const std::string& path,
        const std::string& text) {

        if (path.empty())
            return false;

        auto ops =
            makeOp(
                crdt,
                path,
                text,
                myId,
                clock);

        current[path] = text;

        bool ok =
            writeFile(
                root / path,
                text);

        persistCrdt();

        if (ok) {
            net.broadcast(
                PK_CRDT_OP,
                serializeOps(ops));

            signal(
                "[SAVE] " + path);
        }

        return ok;
    }

    std::string get(
        const std::string& path) {

        auto it =
            current.find(path);

        if (it == current.end())
            return "";

        return it->second;
    }

    std::vector<std::string> list() {
        std::vector<std::string> r;

        for (auto& [p, _] : current)
            r.push_back(p);

        std::sort(
            r.begin(),
            r.end());

        return r;
    }

    void snapshot(
        const std::string& message) {

        history.push_back(current);

        fs::create_directories(
            meta() / "history");

        int n =
            (int)history.size() - 1;

        std::ostringstream o;

        o
            << "SLCE SNAPSHOT\n"
            << "time=" << nowStamp()
            << "\n"
            << "message="
            << esc(message)
            << "\n\n";

        for (auto& [p, c] :
             current) {

            o
                << "FILE "
                << esc(p)
                << "\n"
                << esc(c)
                << "\nEND\n";
        }

        writeFile(
            meta() /
                "history" /
                (std::to_string(n) +
                 ".snapshot"),
            o.str());

        signal(
            "[HISTORY] snapshot #" +
            std::to_string(n));
    }

    bool restoreLatest() {
        if (history.empty())
            return false;

        current =
            history.back();

        for (auto& [p, c] :
             current)
            put(p, c);

        return true;
    }

    std::string diffLatest(
        const std::string& path) {

        if (history.empty())
            return "";

        auto it =
            history.back().find(path);

        std::string before =
            it == history.back().end()
                ? ""
                : it->second;

        return unifiedDiff(
            before,
            get(path));
    }

    std::string md(
        const std::string& path) {

        std::string s =
            get(path);

        if (
            path.size() >= 3 &&
            path.substr(
                path.size() - 3) == ".md")
            return s;

        return "```text\n" +
               s +
               "\n```";
    }

    bool exportProject(
        const fs::path& dest) {

        std::error_code ec;

        fs::create_directories(
            dest,
            ec);

        if (ec)
            return false;

        for (auto& [p, c] :
             current) {

            if (!writeFile(
                    dest / p,
                    c))
                return false;
        }

        writeFile(
            dest / ".slce-export",
            "SLCE PROJECT EXPORT\n" +
            nowStamp() +
            "\n");

        return true;
    }

    bool listen(
        uint16_t port) {

        bool ok =
            net.listenPort(port);

        if (ok) {
            signal(
                "[P2P] listening on " +
                std::to_string(port));
        }

        return ok;
    }

    bool connect(
        const std::string& host,
        uint16_t port) {

        bool ok =
            net.connectTo(
                host,
                port);

        if (ok) {
            /*
                Phase 1:
                    send user ID.

                Phase 2:
                    peer requests password.
            */
            net.broadcast(
                PK_AUTH_ID,
                esc(myId));

            signal(
                "[P2P] connected to " +
                host +
                ":" +
                std::to_string(port));
        }

        return ok;
    }

    void chat(
        const std::string& message) {

        std::string packet =
            esc(myId) +
            "\t" +
            esc(message);

        net.broadcast(
            PK_CHAT,
            packet);

        signal(
            "[CHAT] me: " +
            message);
    }

    void stop() {
        net.stop();
    }
};

#if !defined(_WIN32)

class GUI {
    Project p;

    Display* d = nullptr;
    Window w = 0;
    GC gc = 0;

    bool alive = true;

    std::vector<std::string> files;

    int sel = -1;

    std::string current;
    std::string editor;
    std::string log;

    unsigned long color(
        unsigned r,
        unsigned g,
        unsigned b) {

        return
            ((unsigned long)
                (r & 255)
                << 16) |
            ((unsigned long)
                (g & 255)
                << 8) |
            (b & 255);
    }

    void txt(
        int x,
        int y,
        const std::string& s,
        unsigned long c) {

        XSetForeground(
            d,
            gc,
            c);

        XDrawString(
            d,
            w,
            gc,
            x,
            y,
            s.c_str(),
            (int)s.size());
    }

    void logg(
        const std::string& s) {

        log += s + "\n";
        draw();
    }

    void draw() {
        if (!d)
            return;

        XClearWindow(d, w);

        txt(
            10,
            20,
            "SLCE  ID:" +
                p.id() +
                "  F2 Save  F3 Snapshot  "
                "F4 Diff  F5 Markdown  "
                "F6 Chat  F7 Host  F8 Connect",
            color(230, 230, 230));

        int y = 50;

        for (size_t i = 0;
             i < files.size();
             ++i) {

            txt(
                10,
                y,
                (int)i == sel
                    ? "> "
                    : "  ",
                color(
                    180,
                    220,
                    255));

            txt(
                28,
                y,
                files[i],
                (int)i == sel
                    ? color(
                        255,
                        255,
                        255)
                    : color(
                        180,
                        180,
                        180));

            y += 20;
        }

        int ey = 50;

        auto bad =
            errorLines(editor);

        std::set<int> badset(
            bad.begin(),
            bad.end());

        int ln = 1;

        for (
            auto& line :
            splitLines(editor)) {

            txt(
                270,
                ey,
                std::to_string(ln),
                badset.count(ln)
                    ? color(
                        255,
                        80,
                        80)
                    : color(
                        110,
                        110,
                        110));

            int x = 310;

            for (
                auto& t :
                highlight(line)) {

                unsigned long c =
                    t.kind == 1
                        ? color(
                            90,
                            180,
                            255)
                        : t.kind == 2
                            ? color(
                                120,
                                220,
                                120)
                            : t.kind == 3
                                ? color(
                                    110,
                                    110,
                                    110)
                                : t.kind == 4
                                    ? color(
                                        255,
                                        190,
                                        90)
                                    : color(
                                        225,
                                        225,
                                        225);

                txt(
                    x,
                    ey,
                    t.s,
                    c);

                x +=
                    (int)t.s.size() *
                    8;
            }

            if (badset.count(ln))
                txt(
                    1080,
                    ey,
                    "ERROR",
                    color(
                        255,
                        70,
                        70));

            ey += 16;
            ln++;

            if (ey > 610)
                break;
        }

        int ly = 640;

        for (
            auto& l :
            splitLines(log)) {

            txt(
                10,
                ly,
                l.substr(0, 180),
                color(
                    170,
                    170,
                    170));

            ly += 16;

            if (ly > 790)
                break;
        }

        XFlush(d);
    }

    void refresh() {
        files = p.list();

        if (
            sel < 0 &&
            !files.empty())
            sel = 0;

        if (
            sel >= 0 &&
            sel < (int)files.size()) {

            current =
                files[sel];

            editor =
                p.get(current);
        }

        draw();
    }

    void save() {
        if (!current.empty())
            p.put(
                current,
                editor);
    }

    void key(
        XKeyEvent& e) {

        char b[128]{};
        KeySym ks{};

        int n =
            XLookupString(
                &e,
                b,
                sizeof(b),
                &ks,
                nullptr);

        if (
            (e.state & ControlMask) &&
            ks == XK_s) {

            save();
            return;
        }

        if (ks == XK_F2) {
            save();
            return;
        }

        if (ks == XK_F3) {
            p.snapshot("manual");
            return;
        }

        if (ks == XK_F4) {
            logg(
                p.diffLatest(current));
            return;
        }

        if (ks == XK_F5) {
            logg(
                p.md(current));
            return;
        }

        if (ks == XK_F6) {
            p.chat(editor);
            return;
        }

        if (ks == XK_F7) {
            logg(
                p.listen(43117)
                    ? "[P2P] host ready"
                    : "[P2P] host failed");

            return;
        }

        if (ks == XK_F8) {
            logg(
                p.connect(
                    "127.0.0.1",
                    43117)
                    ? "[P2P] connect requested"
                    : "[P2P] connect failed");

            return;
        }

        if (ks == XK_Return)
            editor.push_back('\n');
        else if (
            ks == XK_BackSpace &&
            !editor.empty())
            editor.pop_back();
        else if (n > 0) {
            for (int i = 0; i < n; i++) {
                if (
                    (unsigned char)b[i] >=
                    32)
                    editor.push_back(b[i]);
            }
        }

        draw();
    }

public:
    bool run() {
        p.setEvent(
            [this](
                const std::string& s) {
                logg(s);
            });

        const char* h =
            getenv("HOME");

        fs::path root =
            h
                ? fs::path(h) /
                    "SLCEProject"
                : fs::path(
                    "SLCEProject");

        fs::create_directories(root);

        p.open(root);

        d =
            XOpenDisplay(nullptr);

        if (!d) {
            std::cerr
                << "DISPLAY unavailable\n";
            return false;
        }

        int sc =
            DefaultScreen(d);

        w =
            XCreateSimpleWindow(
                d,
                RootWindow(d, sc),
                30,
                30,
                1200,
                820,
                1,
                BlackPixel(d, sc),
                0x15181D);

        XStoreName(
            d,
            w,
            "SLCE");

        XSelectInput(
            d,
            w,
            ExposureMask |
                KeyPressMask |
                ButtonPressMask |
                StructureNotifyMask);

        gc =
            XCreateGC(
                d,
                w,
                0,
                nullptr);

        XMapWindow(
            d,
            w);

        refresh();

        while (alive) {
            XEvent e;

            XNextEvent(
                d,
                &e);

            if (e.type == Expose)
                draw();
            else if (
                e.type == KeyPress)
                key(e.xkey);
            else if (
                e.type == ButtonPress &&
                e.xbutton.x < 250 &&
                e.xbutton.y > 25) {

                int i =
                    (e.xbutton.y - 30) /
                    20;

                if (
                    i >= 0 &&
                    i < (int)files.size()) {

                    sel = i;
                    current =
                        files[i];

                    editor =
                        p.get(current);

                    draw();
                }
            } else if (
                e.type == DestroyNotify) {

                alive = false;
            }
        }

        XFreeGC(
            d,
            gc);

        XDestroyWindow(
            d,
            w);

        XCloseDisplay(d);

        p.stop();

        return true;
    }
};

#else

class GUI {
    Project p;

    HWND wnd{};
    HWND files{};
    HWND edit{};
    HWND log{};

    std::string current;

    static std::wstring W(
        const std::string& s) {

        int n =
            MultiByteToWideChar(
                CP_UTF8,
                0,
                s.data(),
                (int)s.size(),
                nullptr,
                0);

        std::wstring r(
            n,
            L' ');

        MultiByteToWideChar(
            CP_UTF8,
            0,
            s.data(),
            (int)s.size(),
            r.data(),
            n);

        return r;
    }

    static std::string A(
        HWND h) {

        int n =
            GetWindowTextLengthW(h);

        std::wstring w(
            n,
            L' ');

        GetWindowTextW(
            h,
            w.data(),
            n + 1);

        int z =
            WideCharToMultiByte(
                CP_UTF8,
                0,
                w.data(),
                n,
                nullptr,
                0,
                0,
                0);

        std::string s(
            z,
            ' ');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            w.data(),
            n,
            s.data(),
            z,
            0,
            0);

        return s;
    }

    void add(
        const std::string& s) {

        int n =
            GetWindowTextLengthW(log);

        SendMessageW(
            log,
            EM_SETSEL,
            n,
            n);

        auto w =
            W(s + "\r\n");

        SendMessageW(
            log,
            EM_REPLACESEL,
            0,
            (LPARAM)w.c_str());
    }

    static LRESULT CALLBACK proc(
        HWND h,
        UINT m,
        WPARAM wp,
        LPARAM lp) {

        GUI* g =
            (GUI*)GetWindowLongPtrW(
                h,
                GWLP_USERDATA);

        if (m == WM_NCCREATE) {
            g =
                (GUI*)
                ((CREATESTRUCTW*)lp)
                    ->lpCreateParams;

            SetWindowLongPtrW(
                h,
                GWLP_USERDATA,
                (LONG_PTR)g);
        }

        if (!g)
            return DefWindowProcW(
                h,
                m,
                wp,
                lp);

        if (m == WM_COMMAND) {
            int id =
                LOWORD(wp);

            if (
                id == 20 &&
                HIWORD(wp) ==
                    LBN_SELCHANGE) {

                int i =
                    (int)SendMessageW(
                        g->files,
                        LB_GETCURSEL,
                        0,
                        0);

                auto v =
                    g->p.list();

                if (
                    i >= 0 &&
                    i < (int)v.size()) {

                    g->current =
                        v[i];

                    SetWindowTextW(
                        g->edit,
                        g->W(
                            g->p.get(
                                g->current))
                            .c_str());
                }
            }
            else if (id == 10) {
                g->p.put(
                    g->current,
                    g->A(g->edit));
            }
            else if (id == 11) {
                g->p.snapshot("manual");
            }
            else if (id == 12) {
                g->add(
                    g->p.diffLatest(
                        g->current));
            }
            else if (id == 13) {
                g->add(
                    g->p.md(
                        g->current));
            }
            else if (id == 14) {
                g->p.chat(
                    g->A(g->edit));
            }
            else if (id == 15) {
                g->add(
                    g->p.listen(43117)
                        ? "[P2P] host ready"
                        : "[P2P] host failed");
            }
            else if (id == 16) {
                g->add(
                    g->p.connect(
                        "127.0.0.1",
                        43117)
                        ? "[P2P] connect requested"
                        : "[P2P] connect failed");
            }
        }

        if (m == WM_DESTROY) {
            g->p.stop();
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(
            h,
            m,
            wp,
            lp);
    }

    void refresh() {
        SendMessageW(
            files,
            LB_RESETCONTENT,
            0,
            0);

        for (auto& s :
             p.list()) {

            auto w =
                W(s);

            SendMessageW(
                files,
                LB_ADDSTRING,
                0,
                (LPARAM)w.c_str());
        }
    }

public:
    bool run() {
        p.setEvent(
            [this](
                const std::string& s) {
                add(s);
            });

        const wchar_t* name =
            L"SLCE_WIN";

        WNDCLASSW c{};

        c.hInstance =
            GetModuleHandleW(nullptr);

        c.lpfnWndProc =
            proc;

        c.lpszClassName =
            name;

        c.hCursor =
            LoadCursor(
                nullptr,
                IDC_ARROW);

        RegisterClassW(&c);

        wnd =
            CreateWindowW(
                name,
                L"SLCE",
                WS_OVERLAPPEDWINDOW |
                    WS_VISIBLE,
                80,
                80,
                1250,
                820,
                nullptr,
                nullptr,
                c.hInstance,
                this);

        files =
            CreateWindowW(
                L"LISTBOX",
                nullptr,
                WS_CHILD |
                    WS_VISIBLE |
                    WS_BORDER |
                    LBS_NOTIFY,
                10,
                40,
                260,
                650,
                wnd,
                (HMENU)20,
                c.hInstance,
                nullptr);

        edit =
            CreateWindowW(
                L"EDIT",
                nullptr,
                WS_CHILD |
                    WS_VISIBLE |
                    WS_BORDER |
                    ES_MULTILINE |
                    WS_VSCROLL |
                    WS_HSCROLL,
                280,
                40,
                700,
                650,
                wnd,
                nullptr,
                c.hInstance,
                nullptr);

        log =
            CreateWindowW(
                L"EDIT",
                nullptr,
                WS_CHILD |
                    WS_VISIBLE |
                    WS_BORDER |
                    ES_MULTILINE |
                    ES_READONLY |
                    WS_VSCROLL,
                990,
                40,
                240,
                650,
                wnd,
                nullptr,
                c.hInstance,
                nullptr);

        const wchar_t* b[] = {
            L"Save",
            L"Snapshot",
            L"Diff",
            L"Markdown",
            L"Chat",
            L"Host",
            L"Connect"
        };

        for (int i = 0; i < 7; i++) {
            CreateWindowW(
                L"BUTTON",
                b[i],
                WS_CHILD |
                    WS_VISIBLE,
                280 + i * 100,
                710,
                90,
                32,
                wnd,
                (HMENU)(10 + i),
                c.hInstance,
                nullptr);
        }

        std::wstring title =
            L"SLCE - ID " +
            W(p.id());

        SetWindowTextW(
            wnd,
            title.c_str());

        refresh();

        MSG m;

        while (
            GetMessageW(
                &m,
                nullptr,
                0,
                0) > 0) {

            TranslateMessage(&m);
            DispatchMessageW(&m);
        }

        return true;
    }
};

#endif

static bool selfTest() {
    fs::path base =
        fs::temp_directory_path() /
        ("slce-" + randomHex(6));

    fs::path a =
        base / "A";

    fs::path b =
        base / "B";

    fs::create_directories(a);
    fs::create_directories(b);

    Project A;
    Project B;

    bool ok = true;

    A.setEvent(
        [](const std::string& s) {
            std::cerr
                << "A "
                << s
                << "\n";
        });

    B.setEvent(
        [](const std::string& s) {
            std::cerr
                << "B "
                << s
                << "\n";
        });

    auto T =
        [&](const char* n,
            bool v) {

            std::cerr
                << n
                << ": "
                << (v
                    ? "PASS"
                    : "FAIL")
                << "\n";

            ok &= v;
        };

    T(
        "A.open",
        A.open(a));

    T(
        "B.open",
        B.open(b));

    A.configureIdentity(
        "alice",
        "123456789012");

    B.configureIdentity(
        "bob",
        "123456789012");

    T(
        "put1",
        A.put(
            "main.cpp",
            "int main(){\n"
            "return 0;\n"
            "}\n"));

    A.snapshot("one");

    T(
        "put2",
        A.put(
            "main.cpp",
            "int main(){\n"
            "return 1;\n"
            "}\n"));

    std::cerr
        << "DIFF=\n"
        << A.diffLatest(
            "main.cpp")
        << "\n";

    T(
        "diff",
        A.diffLatest(
            "main.cpp")
            .find(
                "+ return 1") !=
            std::string::npos);

    T(
        "markdown",
        A.md(
            "main.cpp")
            .find(
                "int main") !=
            std::string::npos);

    auto errs =
        errorLines(
            "int x=(1+2;\n");

    T(
        "error-line",
        !errs.empty());

    T(
        "listen",
        A.listen(43217));

    T(
        "connect",
        B.connect(
            "127.0.0.1",
            43217));

    std::this_thread::sleep_for(
        std::chrono::milliseconds(500));

    T(
        "shared-put",
        B.put(
            "shared.cpp",
            "int x=42;\n"));

    for (
        int i = 0;
        i < 100 &&
        A.get("shared.cpp") !=
            "int x=42;\n";
        i++) {

        std::this_thread::sleep_for(
            std::chrono::milliseconds(20));
    }

    T(
        "crdt-sync",
        A.get(
            "shared.cpp") ==
        "int x=42;\n");

    A.stop();
    B.stop();

    std::error_code ec;

    fs::remove_all(
        base,
        ec);

    return ok;
}

int main(
    int argc,
    char** argv) {

    if (
        argc > 1 &&
        std::string(argv[1]) ==
            "--self-test") {

        bool ok =
            selfTest();

        std::cout
            << (
                ok
                    ? "SLCE SELF-TEST: PASS\n"
                    : "SLCE SELF-TEST: FAIL\n");

        return ok ? 0 : 1;
    }

    if (
        argc > 1 &&
        std::string(argv[1]) ==
            "--identity" &&
        argc >= 4) {

        std::string id =
            argv[2];

        std::string pw =
            argv[3];

        if (
            id.empty() ||
            !validPassword(pw)) {

            std::cerr
                << "SLCE: ID/password invalid. "
                   "Password must be exactly "
                   "12 digits.\n";

            return 2;
        }

        fs::path r;

        const char* h =
            std::getenv("HOME");

        r =
            h
                ? fs::path(h) /
                    "SLCEProject"
                : fs::path(
                    "SLCEProject");

        fs::create_directories(
            r / ".slce");

        std::ofstream f(
            r / ".slce" /
                "identity.txt",
            std::ios::trunc);

        f
            << esc(id)
            << "\n"
            << pw
            << "\n";

        std::cout
            << "SLCE identity saved: "
            << id
            << "\n";

        return 0;
    }

#if defined(_WIN32)
    GUI g;
    return g.run() ? 0 : 1;
#else
    GUI g;
    return g.run() ? 0 : 1;
#endif
}
```
