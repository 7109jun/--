#include <iostream>
#include <windows.h>
#include <Python.h>

// 리소스로부터 파일을 임시 폴더에 추출하는 함수
bool ExtractResourceToFile(int resourceId, const wchar_t* outputFileName, wchar_t* outFullPath) {
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(resourceId), L"BINARY");
    if (!hRes) return false;

    HGLOBAL hData = LoadResource(hModule, hRes);
    DWORD dataSize = SizeofResource(hModule, hRes);
    LPVOID pData = LockResource(hData);
    if (!pData) return false;

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring fullPath = std::wstring(tempPath) + outputFileName;
    wcscpy_s(outFullPath, MAX_PATH, fullPath.c_str());

    HANDLE hFile = CreateFileW(fullPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD bytesWritten;
    WriteFile(hFile, pData, dataSize, &bytesWritten, NULL);
    CloseHandle(hFile);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "사용법: pp_browser.exe <파일이름.pp>\n";
        return 1;
    }

    const char* ppFilename = argv[1];

    // 1. 임시 폴더에 내장된 파이썬 DLL과 라이브러리 압축파일 풀기
    // (리소스 ID 101: python311.dll, 102: minilib.zip)
    wchar_t tempDllPath[MAX_PATH];
    wchar_t tempZipPath[MAX_PATH];
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    ExtractResourceToFile(101, L"python311.dll", tempDllPath);
    ExtractResourceToFile(102, L"minilib.zip", tempZipPath);

    // 2. 파이썬 홈을 임시 폴더로 지정 (엔진이 자기 집을 찾도록 설정)
    Py_SetPythonHome(tempPath);

    // 3. 파이썬 인터프리터 고속 초기화 (0.3초 컷)
    Py_Initialize();

    // 4. .pp 파일 열고 실행
    FILE* fp = nullptr;
    errno_t err = fopen_s(&fp, ppFilename, "r");
    if (err != 0 || fp == nullptr) {
        std::cerr << "오류: '" << ppFilename << "'.pp 파일을 열 수 없습니다.\n";
        Py_Finalize();
        return 1;
    }

    PyRun_SimpleFile(fp, ppFilename);
    fclose(fp);

    // 5. 인터프리터 종료
    Py_Finalize();
    return 0;
}
