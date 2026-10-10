#include "Core/Window.h"

using namespace GEM;

Window::Window(HINSTANCE instance, const std::wstring& windowName) {
    RegisterWindowClass(instance);

    mHandle = CreateWindowExW(
        0,
        sClassName,
        windowName.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        nullptr,
        nullptr,
        instance,
        this
    );

    ShowWindow(mHandle, SW_SHOW);
    UpdateWindow(mHandle);
}

bool Window::ConsumeResize(UINT& width, UINT& height) {
	if (!mResizePending || mIsMinimized) {
		return false;
	}

	width = mWidth;
	height = mHeight;
	mResizePending = false;
	return true;
}

bool Window::PollEvents() {
    MSG message{};

    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            mShouldClose = true;
            break;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return !mShouldClose;
}

void Window::RegisterWindowClass(HINSTANCE instance)
{
	WNDCLASSEXW windowClass{};
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Window::WindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(nullptr, IDC_ARROW);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = sClassName;
    windowClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    if (RegisterClassExW(&windowClass) == 0)
    {
        const DWORD error = GetLastError();

        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            throw std::runtime_error("Failed to register window class");
        }
    }
}

LRESULT Window::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        mShouldClose = true;
        PostQuitMessage(0);
        return 0;

    case WM_RBUTTONDOWN:
        SetCapture(hwnd);
        return 0;

    case WM_RBUTTONUP:
        ReleaseCapture();
        return 0;

    case WM_SIZE:
        mIsMinimized = wParam == SIZE_MINIMIZED;

        if (!mIsMinimized) {
            const UINT width = LOWORD(lParam);
            const UINT height = HIWORD(lParam);

            if (width > 0 && height > 0 && (width != mWidth || height != mHeight)) {
                mWidth = width;
                mHeight = height;
                mResizePending = true;
            }
        }
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        auto* createInfo = reinterpret_cast<CREATESTRUCTW*>(lParam);

        window = static_cast<Window*>(createInfo->lpCreateParams);

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

        window->mHandle = hwnd;
    }


    if (window) {
        LRESULT result = window->HandleMessage(hwnd, message, wParam, lParam);
        if (message == WM_NCDESTROY) {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }

        return result;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
