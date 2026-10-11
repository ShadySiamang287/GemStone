#include "Core/Application.h"

#include "Graphics/GraphicsCommands.h"
#include "Graphics/ResourceFactory.h"

using namespace GEM;

Application::Application(HINSTANCE instance) {
    mWindow = std::make_unique<Window>(instance, L"Gemstone Engine");
    mContext = std::make_unique<GraphicsContext>(mWindow->GetHandle());
    GraphicsCommands::RegisterContext(mContext.get());
    ResourceFactory::RegisterContext(mContext.get());

    mRenderer = std::make_unique<Renderer>(*mContext.get());
}

void Application::Run() {
    while (mWindow->PollEvents()){
        mRenderer->Render();
    }
}