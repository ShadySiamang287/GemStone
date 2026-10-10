#pragma once

#include "Core/GemStoneAPI.h"

#include "Core/Window.h"

namespace GEM{
    class GEM_API Application {
    public:
        Application(HINSTANCE instance);
		virtual ~Application() = default;

		void Run();

    private:
        std::unique_ptr<Window> mWindow;
    };

    extern Application* CreateApplication(HINSTANCE instance);
}