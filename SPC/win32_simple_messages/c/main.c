#include <windows.h>
#include <stdio.h>
#include <time.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {

        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) {
                DestroyWindow(hwnd);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    char name[256];
    {
        DWORD size = sizeof(name);
        if (GetUserNameA(name, &size)) {
            printf("Hello, %s\n", name);
        } else {
            printf("GetUserName failed: %lu\n", GetLastError());
        }
    }

    char computer_name[256];
    {
        DWORD size = sizeof(computer_name);
        if (GetComputerNameA(computer_name, &size)) {
            printf("On computer %s\n", computer_name);
        } else {
            printf("GetComputerNameA failed: %lu\n", GetLastError());
        }
    }

    char time_buf[256];
    {
        // current time
        time_t now = time(NULL);
        struct tm t;
        if (localtime_s(&t, &now)) {
            perror("Get time");
        }
        snprintf(time_buf, sizeof(time_buf), "%04d-%02d-%02d|%02d:%02d:%2d",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min, t.tm_sec);
    }

    char greeting_msg[256];
    snprintf(greeting_msg, sizeof(greeting_msg), "Time: %s\nHello Mr %s from %s", time_buf, name, computer_name);
    MessageBeep(MB_OK);
    MessageBoxA(NULL, greeting_msg, "MyFirst App", MB_OK);

    // Win32 Window

    WNDCLASS wc = {
        .lpfnWndProc = WindowProc,
        .hInstance = hInstance,
        .lpszClassName = "TinyWindowClass",
    };

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        "TinyWindowClass",
        "Tiny Win32 Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        NULL, NULL,
        hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

