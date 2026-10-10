#include "Core/Application.h"

#include "Graphics/GraphicsCommands.h"

using namespace GEM;

Application::Application(HINSTANCE instance) {
    mWindow = std::make_unique<Window>(instance, L"Gemstone Engine");
    mContext = std::make_unique<GraphicsContext>(mWindow->GetHandle());
    GraphicsCommands::RegisterContext(mContext.get());
}

void Application::Run() {
    while (mWindow->PollEvents()){
        GraphicsCommands::BeginFrame();
        GraphicsCommands::EndFrame();
    }
}