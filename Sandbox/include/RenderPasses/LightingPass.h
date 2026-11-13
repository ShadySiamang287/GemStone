#pragma once

#include <Renderer/RenderPasses/RenderPass.h>
#include <Graphics/ResourceFactory.h>
#include <Graphics/RenderingStructs.h>

using namespace GEM;

class LightingPass : public RenderPass {
public:
    LightingPass(ConstantBuffer<FrameData>* frameDataBuffer, StructuredBuffer<PointLight, 1024>* lightBuffer);
    void Render(RenderTargetManager& manager, std::span<Model> models) override;

private:
    std::shared_ptr<RootSignature> mRootSig; 
    std::unique_ptr<GraphicsPipeline> mPipeline;

    std::array<float, 4> mClearColour;

    ConstantBuffer<FrameData>* mFrameDataBuffer = nullptr;
    StructuredBuffer<PointLight, 1024>* mLightBuffer = nullptr;
};
