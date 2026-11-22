#include <EntryPoint.h>

#include "TestScene.h"

#include "RenderPasses/DefferedPass.h"
#include "RenderPasses/LightingPass.h"
#include "RenderPasses/ToneMappingPass.h"
#include "RenderPasses/AtmospherePass.h"
#include "RenderPasses/BloomPass.h"
#include "RenderPasses/ShadowPass.h"

class SandboxApp : public GEM::Application {
public:
	SandboxApp(HINSTANCE instance) : Application(instance) {
		mScene = std::make_unique<TestScene>();
		mRenderer->AddRenderPass(std::make_unique<DeferredPass>(mRenderer->GetFrameDataBuffer(), mRenderer->GetTLAS()));
		mRenderer->AddRenderPass(std::make_unique<ShadowPass>(mRenderer->GetFrameDataBuffer(),  mRenderer->GetPointLightBuffer(), mRenderer->GetTLAS()));
		mRenderer->AddRenderPass(std::make_unique<LightingPass>(mRenderer->GetFrameDataBuffer(), mRenderer->GetPointLightBuffer()));
		mRenderer->AddRenderPass(std::make_unique<AtmospherePass>(mRenderer->GetFrameDataBuffer(), mRenderer->GetTLAS()));
		mRenderer->AddRenderPass(std::make_unique<BloomPass>());
		mRenderer->AddRenderPass(std::make_unique<ToneMappingPass>());
	}
};

GEM::Application* GEM::CreateApplication(HINSTANCE instance) {
	return new SandboxApp(instance);
}