#pragma once

namespace GEM {
class Window{
	public:
		Window(HINSTANCE instance, const std::wstring& windowName);
		

		bool PollEvents();
		bool ConsumeResize(UINT& width, UINT& height);

		bool IsMinimized() const {
			return mIsMinimized;
		}

		HWND GetHandle() const {
			return mHandle;
		}

		bool ShouldClose() const {
			return mShouldClose;
		}
	private:
		void RegisterWindowClass(HINSTANCE instance);

		HWND mHandle = nullptr;
		bool mShouldClose = false;
		bool mResizePending = false;
		bool mIsMinimized = false;
		UINT mWidth = 1280;
		UINT mHeight = 720;

		LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		static inline wchar_t sClassName[] = L"GemStoneWindowClass";
	};
}