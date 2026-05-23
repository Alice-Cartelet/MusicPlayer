#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <fstream>
#pragma comment(lib,"Comdlg32.lib")
#pragma comment(lib,"Shell32.lib")
#define BTN_ADD 1001
#define BTN_CLEAR 1002
#define LIST_FILES 1003
HWND g_list;
std::vector<std::wstring> g_files;
int SyncSafeToInt( unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4 )
{
    return ((b1 & 0x7F) << 21) | ((b2 & 0x7F) << 14) | ((b3 & 0x7F) << 7 ) | (b4 & 0x7F);
}
void IntToSyncSafe( int value, unsigned char out[4] )
{
    out[0] = (value >> 21) & 0x7F;
    out[1] = (value >> 14) & 0x7F;
    out[2] = (value >> 7 ) & 0x7F;
    out[3] = value & 0x7F;
}
bool RemoveCover( const std::wstring& path )
{
    std::ifstream in( path.c_str(), std::ios::binary );
    if (!in) return false;
    std::vector<char> data( (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>() );
    in.close();
    if (data.size() < 10) return false;
    if (!(data[0] == 'I' && data[1] == 'D' && data[2] == '3'))
    {
        return false;
    }
    int tagSize = SyncSafeToInt( (unsigned char)data[6], (unsigned char)data[7], (unsigned char)data[8], (unsigned char)data[9] );
    int pos = 10;
    int end = 10 + tagSize;
    std::vector<char> newFrames;
    while (pos + 10 <= end)
    {
        char id[5] =
        {
        }
        ;
        memcpy(id, &data[pos], 4);
        if (id[0] == 0) break;
        int frameSize = ((unsigned char)data[pos + 4] << 24) | ((unsigned char)data[pos + 5] << 16) | ((unsigned char)data[pos + 6] << 8 ) | ((unsigned char)data[pos + 7]);
        if (frameSize <= 0) break;
        int totalSize = frameSize + 10;
        if (pos + totalSize > (int)data.size()) break;
        if (strncmp(id, "APIC", 4) != 0)
        {
            newFrames.insert( newFrames.end(), data.begin() + pos, data.begin() + pos + totalSize );
        }
        pos += totalSize;
    }
    int newTagSize = (int)newFrames.size();
    std::vector<char> out;
    out.insert( out.end(), data.begin(), data.begin() + 10 );
    unsigned char sizeBytes[4];
    IntToSyncSafe( newTagSize, sizeBytes );
    out[6] = sizeBytes[0];
    out[7] = sizeBytes[1];
    out[8] = sizeBytes[2];
    out[9] = sizeBytes[3];
    out.insert( out.end(), newFrames.begin(), newFrames.end() );
    out.insert( out.end(), data.begin() + end, data.end() );
    std::ofstream file( path.c_str(), std::ios::binary | std::ios::trunc );
    if (!file) return false;
    file.write( out.data(), out.size() );
    file.close();
    return true;
}
void AddFile( const std::wstring& path )
{
    if (path.size() < 4) return;
    if (_wcsicmp( path.substr(path.size() - 4).c_str(), L".mp3" ) != 0)
    {
        return;
    }
    g_files.push_back(path);
    SendMessageW( g_list, LB_ADDSTRING, 0, (LPARAM)path.c_str() );
}
void OpenFiles(HWND hwnd)
{
    wchar_t files[32768] =
    {
    }
    ;
    OPENFILENAMEW ofn =
    {
    }
    ;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"MP3 文件 (*.mp3)\0*.mp3\0";
    ofn.lpstrFile = files;
    ofn.nMaxFile = 32768;
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    wchar_t* p = files;
    std::wstring dir = p;
    p += dir.size() + 1;
    if (*p == 0)
    {
        AddFile(dir);
        return;
    }
    while (*p)
    {
        std::wstring file = dir + L"\\" + p;
        AddFile(file);
        p += wcslen(p) + 1;
    }
}
void ProcessAll(HWND hwnd)
{
    if (g_files.empty())
    {
        MessageBoxW( hwnd, L"没有文件", L"提示", MB_OK );
        return;
    }
    int okCount = 0;
    for (const auto& file : g_files)
    {
        if (RemoveCover(file)) okCount++;
    }
    std::wstring msg = L"处理完成\r\n\r\n成功：" + std::to_wstring(okCount) + L"\r\n失败：" + std::to_wstring( g_files.size() - okCount );
    MessageBoxW( hwnd, msg.c_str(), L"完成", MB_OK );
}
LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch (msg)
    {
        case WM_CREATE:
        {
            DragAcceptFiles(hwnd, TRUE);
            CreateWindowW( L"BUTTON", L"添加MP3", WS_VISIBLE | WS_CHILD, 20, 20, 120, 35, hwnd, (HMENU)BTN_ADD, nullptr, nullptr );
            CreateWindowW( L"BUTTON", L"批量清除", WS_VISIBLE | WS_CHILD, 160, 20, 120, 35, hwnd, (HMENU)BTN_CLEAR, nullptr, nullptr );
            g_list = CreateWindowW( L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 20, 70, 560, 240, hwnd, (HMENU)LIST_FILES, nullptr, nullptr );
            break;
        }
        case WM_DROPFILES:
        {
            HDROP hDrop = (HDROP)wParam;
            UINT count = DragQueryFileW( hDrop, 0xFFFFFFFF, nullptr, 0 );
            for (UINT i = 0; i < count; i++)
            {
                wchar_t path[MAX_PATH];
                DragQueryFileW( hDrop, i, path, MAX_PATH );
                AddFile(path);
            }
            DragFinish(hDrop);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case BTN_ADD:
                {
                    OpenFiles(hwnd);
                    break;
                }
                case BTN_CLEAR:
                {
                    ProcessAll(hwnd);
                    break;
                }
            }
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        default:
        {
            return DefWindowProcW( hwnd, msg, wParam, lParam );
        }
    }
    return 0;
}
int WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, int nShow )
{
    WNDCLASSW wc =
    {
    }
    ;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"Mp3Cleaner";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW( 0, wc.lpszClassName, L"音频封面批量清除工具", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 620, 420, nullptr, nullptr, hInst, nullptr );
    ShowWindow(hwnd, nShow);
    MSG msg;
    while (GetMessageW( &msg, nullptr, 0, 0 ))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}