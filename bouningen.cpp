#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <process.h>
#include "pngres.h"
#include "resource.h"

const LPCTSTR s_pszClassName = TEXT("MZ Bouningen");

HINSTANCE g_hInstance;
HWND g_hMainWnd;

PngRes g_bmpLeft1;
PngRes g_bmpLeft2;
PngRes g_bmpRight1;
PngRes g_bmpRight2;

BOOL g_bQuit = FALSE;

extern "C"
LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        MessageBeep(MB_ICONERROR);
        g_bQuit = TRUE;
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL InitInstance(HINSTANCE hInstance)
{
    g_hInstance = hInstance;
    if (g_bmpLeft1.Load(hInstance, MAKEINTRESOURCE(ID_LEFT1)) &&
        g_bmpLeft2.Load(hInstance, MAKEINTRESOURCE(ID_LEFT2)) &&
        g_bmpRight1.Load(hInstance, MAKEINTRESOURCE(ID_RIGHT1)) &&
        g_bmpRight2.Load(hInstance, MAKEINTRESOURCE(ID_RIGHT2)))
    {
        g_bmpLeft1.Premultiply();
        g_bmpLeft2.Premultiply();
        g_bmpRight1.Premultiply();
        g_bmpRight2.Premultiply();
        return TRUE;
    }
    return FALSE;
}

unsigned __stdcall PlayThread(void *)
{
    if (g_hMainWnd == NULL)
        return 0;

    HDC hdc = GetDC(g_hMainWnd);
    HDC hdcMem = CreateCompatibleDC(NULL);
    POINT ptCur, pt, ptSrc = {0, 0};
    srand(GetTickCount());

    BITMAP bm;
    g_bmpRight1.GetBitmap(&bm);
    SIZE sizSrc = {bm.bmWidth, bm.bmHeight};

    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 0xFF;
    bf.AlphaFormat = AC_SRC_ALPHA;
    HGDIOBJ hbmOld;

    pt.x = rand() % (GetSystemMetrics(SM_CXSCREEN) - sizSrc.cx);
    pt.y = rand() % (GetSystemMetrics(SM_CYSCREEN) - sizSrc.cy);

    while (!g_bQuit)
    {
        static BOOL b = FALSE;
        GetCursorPos(&ptCur);
        if (b)
        {
            if (ptCur.x < pt.x + sizSrc.cx / 2)
                hbmOld = SelectObject(hdcMem, g_bmpLeft2.GetHandle());
            else
                hbmOld = SelectObject(hdcMem, g_bmpRight2.GetHandle());
        }
        else
        {
            if (ptCur.x < pt.x + sizSrc.cx / 2)
                hbmOld = SelectObject(hdcMem, g_bmpLeft1.GetHandle());
            else
                hbmOld = SelectObject(hdcMem, g_bmpRight1.GetHandle());
        }
        UpdateLayeredWindow(g_hMainWnd, hdc, &pt, &sizSrc,
            hdcMem, &ptSrc, 0, &bf, ULW_ALPHA);
        SelectObject(hdcMem, hbmOld);

        ShowWindow(g_hMainWnd, SW_SHOWNORMAL);
        UpdateWindow(g_hMainWnd);

        INT dx, dy;
        if (abs(ptCur.x - (pt.x + sizSrc.cx / 2)) > 50)
            dx = 30;
        else
            dx = 10;
        if (abs(ptCur.y - (pt.y + sizSrc.cy / 2)) > 100)
            dy = 30;
        else
            dy = 10;

        if (ptCur.x < pt.x + sizSrc.cx / 2)
            pt.x -= dx;
        else
            pt.x += dx;
        if (ptCur.y < pt.y + sizSrc.cy / 2)
            pt.y -= dy;
        else
            pt.y += dy;
        Sleep(300);
        b = !b;
    }
    DeleteDC(hdcMem);
    ReleaseDC(g_hMainWnd, hdc);
    return 0;
}

extern "C"
INT WINAPI _tWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      pszCmdLine,
    INT         nCmdShow)
{
    if (!InitInstance(hInstance))
    {
        MessageBox(NULL, TEXT("Failed to initialize instance!"), NULL, MB_ICONERROR);
        return 3;
    }

    WNDCLASS wc;
    wc.style = 0; 
    wc.lpfnWndProc = WindowProc; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
    wc.hInstance = hInstance; 
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wc.hbrBackground = HBRUSH(GetStockObject(NULL_BRUSH));
    wc.lpszMenuName = NULL; 
    wc.lpszClassName = s_pszClassName; 
    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, TEXT("Failed to register window!"), NULL, MB_ICONERROR);
        return 1;
    }

    g_hMainWnd = CreateWindowEx(
        WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED,
        s_pszClassName, TEXT("BOUNINGEN"),
        WS_POPUPWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance,
        NULL);
    if (g_hMainWnd == NULL)
    {
        MessageBox(NULL, TEXT("Failed to create main window!"), NULL, MB_ICONERROR);
        return 2;
    }

    unsigned threadid;
    HANDLE hThread;
    hThread = HANDLE(_beginthreadex(NULL, 0, PlayThread, NULL, 0, &threadid));
    if (hThread == NULL)
    {
        DestroyWindow(g_hMainWnd);
        MessageBox(NULL, TEXT("Failed to create main thread!"), NULL, MB_ICONERROR);
        return 4;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return INT(msg.wParam);
}
