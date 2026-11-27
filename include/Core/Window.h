#pragma once

namespace GEM {
/// @brief this class encapsulates the win32 window
class Window{
	public:
		Window() = default;
		
		/// @brief gets the handle to the window that was created
		/// @return the handle
		#ifdef GEM_PLATFORM_WINDOWS
		virtual HWND GetHandle() = 0;
		#endif

		uint16_t GetWidth() const {
			return mWidth;
		}

		uint16_t GetHeight() const {
			return mHeight;
		}

		/// @brief checks if the window should close
		/// @return if the window needs to close
		bool ShouldClose() const {
			return mShouldClose;
		}
	protected:

		bool mShouldClose = false;
		bool mResizePending = false;
		bool mIsMinimised = false;
		uint16_t mWidth = 1280;
		uint16_t mHeight = 720;
	};
}
