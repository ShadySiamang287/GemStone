#include "Core/Application.h"

#ifndef GEM_PLATFORM_WINDOWS
#error GEM_PLATFORM_WINDOWS is missing
#endif

#ifndef GEM_BUILD_DLL
#error GEM_BUILD_DLL is missing
#endif

using namespace GEM;

Application::Application(HINSTANCE instance) {
    mWindow = std::make_unique<Window>(instance, L"Gemstone Engine");
}

void Application::Run() {
    while (mWindow->PollEvents()){

    }
}