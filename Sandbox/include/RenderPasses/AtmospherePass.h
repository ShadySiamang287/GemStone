#pragma once

#include <Renderer/RenderPasses/RenderPass.h>
#include "Graphics/RenderingStructs.h"
#include "Graphics/ResourceFactory.h"

struct AtmosphereData{
    glm::vec3 atmosphereColour = {0.15, 0.4, 1.0};
    float planetRadius = 10.f;
    float atmosphereRadius = 14.f;
    float densityFalloff = 7.0f;
    float strengthMultiplier = 1.25f;
    int steps = 3;
};

class AtmospherePass : public GEM::RenderPass {
public:
    AtmospherePass(GEM::ConstantBuffer<GEM::FrameData>* frameDataBuffer, GEM::TLAS* tlas);
    void Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) override;
	void OnImGuiRender() override;

private:
    std::shared_ptr<GEM::RootSignature> mRootSig; 
    std::shared_ptr<GEM::RootSignature> mRTRootSig;
    std::unique_ptr<GEM::GraphicsPipeline> mPipeline;
    GEM::ConstantBuffer<GEM::FrameData>* mFrameDataBuffer = nullptr;
    std::unique_ptr<GEM::ConstantBuffer<AtmosphereData>> mAtmosphereDataBuffer = nullptr;
    AtmosphereData mAtmosphereData;

    void GenerateBaseMesh();

    GEM::Model mModel;
    std::unique_ptr<GEM::BLAS> mBlas = nullptr;
    std::unique_ptr<GEM::RaytracingPipeline> mRTPipeline = nullptr;
    std::unique_ptr<GEM::ShaderTable> mShaderTable = nullptr;
    GEM::TLAS* tlasPtr;
};