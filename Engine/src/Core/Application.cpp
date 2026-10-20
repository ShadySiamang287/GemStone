#include "Core/Application.h"

#include "Graphics/GraphicsCommands.h"
#include "Graphics/ResourceFactory.h"

using namespace GEM;

Application::Application(HINSTANCE instance) {
    mWindow = std::make_unique<Window>(instance, L"Gemstone Engine");
    mContext = std::make_unique<GraphicsContext>(
        mWindow->GetHandle(),
        mWindow->GetWidth(),
        mWindow->GetHeight()
    );
    GraphicsCommands::RegisterContext(mContext.get());
    ResourceFactory::RegisterContext(mContext.get());

    mRenderer = std::make_unique<Renderer>(*mContext.get(), mWindow->GetHandle());
}

void Application::Run() {
    while (mWindow->PollEvents()){
        UINT width = 0;
        UINT height = 0;
        mTimeManager.Update();
        mScene->Update(mWindow->GetInput(), mTimeManager.GetDeltaTime());

        if (mWindow->ConsumeResize(width, height)) {
            mRenderer->Resize(width, height);
        }

        if (mWindow->IsMinimised()) {
            continue;
        }

        mRenderer->Render(mScene.get());
    }
}
