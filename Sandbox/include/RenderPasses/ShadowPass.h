#pragma once

#include <Renderer/RenderPasses/RenderPass.h>
#include "Graphics/RenderingStructs.h"
#include "Graphics/ResourceFactory.h"

class ShadowPass : public GEM::RenderPass{
public:
    ShadowPass(GEM::ConstantBuffer<GEM::FrameData>* frameDataBuffer,  GEM::StructuredBuffer<GEM::PointLight, 1024>* lightBuffer, GEM::TLAS* tlas);
    void Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) override;

private:
    GEM::TLAS* tlasPtr;

    std::shared_ptr<GEM::RootSignature> mRTRootSig;
    std::unique_ptr<GEM::RaytracingPipeline> mRTPipeline = nullptr;
    std::unique_ptr<GEM::ShaderTable> mShaderTable = nullptr;
    GEM::ConstantBuffer<GEM::FrameData>* mFrameDataBuffer = nullptr;
    GEM::StructuredBuffer<GEM::PointLight, 1024>* mLightBuffer = nullptr;
};