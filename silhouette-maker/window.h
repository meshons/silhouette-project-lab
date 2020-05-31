#ifndef SILHOUETTE_WINDOW_H
#define SILHOUETTE_WINDOW_H

#include <tchar.h>
#include <windows.h>
#include <exception>
#include <functional>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 1024
#endif

class window {
    HWND hwnd;
public:
    window(HINSTANCE & instance, int nCmdShow) {
        const char CLASS_NAME[]  = "silhouette window";

        WNDCLASS wc = { };

        wc.lpfnWndProc   = WindowProc;
        wc.hInstance     = instance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        RECT wr = { 0,0, SCREEN_WIDTH, SCREEN_HEIGHT };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

        hwnd = CreateWindowEx(
                0,
                CLASS_NAME,
                "silhouette maker",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
                nullptr,
                nullptr,
                instance,
                nullptr
        );

        if (hwnd == nullptr)
        {
            throw std::runtime_error("Could not create hwnd");
        }

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    void run(std::function<bool(void)> render, std::function<void(void)> clean) {
        MSG msg = { };
        try {
            while (true) {
                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);

                    if (msg.message == WM_QUIT)
                        break;
                }


                if (!render()) {
                    break;
                }
            }

            clean();
        } catch (const std::exception & e) {
            clean();
            throw e;
        }
    }

    LRESULT static CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_CLOSE:
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

    HWND & getHWND() {
        return hwnd;
    }
};



#endif //SILHOUETTE_WINDOW_H
