#include <EntryPoint.h>

#include "TestScene.h"

#include "RenderPasses/DefferedPass.h"
#include "RenderPasses/LightingPass.h"
#include "RenderPasses/ToneMappingPass.h"
#include "RenderPasses/AtmospherePass.h"

class SandboxApp : public GEM::Application {
public:
	SandboxApp(HINSTANCE instance) : Application(instance) {
		mScene = std::make_unique<TestScene>();
		mRenderer->AddRenderPass(std::make_unique<DeferredPass>(mRenderer->GetFrameDataBuffer()));
		mRenderer->AddRenderPass(std::make_unique<LightingPass>());
		mRenderer->AddRenderPass(std::make_unique<AtmospherePass>(mRenderer->GetFrameDataBuffer()));
		mRenderer->AddRenderPass(std::make_unique<ToneMappingPass>());
	}
};

GEM::Application* GEM::CreateApplication(HINSTANCE instance) {
	return new SandboxApp(instance);
}