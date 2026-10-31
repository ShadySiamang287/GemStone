#pragma once

#include <Renderer/RenderPasses/RenderPass.h>
#include "Graphics/RenderingStructs.h"
#include "Graphics/ResourceFactory.h"

class AtmospherePass : public GEM::RenderPass {
public:
    AtmospherePass(GEM::ConstantBuffer<GEM::FrameData>* frameDataBuffer);
    void Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) override;
	void OnImGuiRender() override;

private:
    std::shared_ptr<GEM::RootSignature> mRootSig; 
    std::unique_ptr<GEM::GraphicsPipeline> mPipeline;
    GEM::ConstantBuffer<GEM::FrameData>* mFrameDataBuffer = nullptr;
};