#pragma once

namespace GEM {
/// @brief this class encapsulates the win32 window
class Window{
	public:
		/// @brief This is used to initialise the window
		/// @param instance The instance handle provided by windows
		/// @param windowName The name of the window
		Window(HINSTANCE instance, const std::wstring& windowName);
		

		/// @brief This pools the windows event
		/// @return If the program should continue executing
		bool PollEvents();
		/// @brief Used to resize the window
		/// @param width the new width
		/// @param height the new height
		/// @return If the window actually got resized
		bool ConsumeResize(UINT& width, UINT& height);

		/// @brief Checks if the window is minimised
		/// @return if the window is minimised 
		bool IsMinimised() const {
			return mIsMinimised;
		}

		/// @brief gets the handle to the window that was created
		/// @return the handle
		HWND GetHandle() const {
			return mHandle;
		}

		/// @brief checks if the window should close
		/// @return if the window needs to close
		bool ShouldClose() const {
			return mShouldClose;
		}
	private:
		void RegisterWindowClass(HINSTANCE instance);

		HWND mHandle = nullptr;
		bool mShouldClose = false;
		bool mResizePending = false;
		bool mIsMinimised = false;
		UINT mWidth = 1280;
		UINT mHeight = 720;

		LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		static inline wchar_t sClassName[] = L"GemStoneWindowClass";
	};
}