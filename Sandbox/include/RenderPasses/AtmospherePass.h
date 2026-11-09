#pragma once

#include <Renderer/RenderPasses/RenderPass.h>
#include "Graphics/RenderingStructs.h"
#include "Graphics/ResourceFactory.h"

struct AtmosphereData{
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::vec3 atmosphereColour = {0.15, 0.4, 1.0};
    float planetRadius = 10.f;
    float atmosphereRadius = 14.f;
    float densityFalloff = 5.5f;
    float strengthMultiplier = 0.55f;
    int steps = 3;
};

class AtmospherePass : public GEM::RenderPass {
public:
    AtmospherePass(GEM::ConstantBuffer<GEM::FrameData>* frameDataBuffer, GEM::TLAS* tlas);
    void Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) override;
	void OnImGuiRender() override;

private:
    GEM::TLAS* tlasPtr;
    std::unique_ptr<GEM::BLAS> mBlas = nullptr;
    GEM::Model mModel;
    void GenerateBaseMesh();
    
    std::shared_ptr<GEM::RootSignature> mRTRootSig;
    std::unique_ptr<GEM::RaytracingPipeline> mRTPipeline = nullptr;
    std::unique_ptr<GEM::ShaderTable> mShaderTable = nullptr;
    GEM::ConstantBuffer<GEM::FrameData>* mFrameDataBuffer = nullptr;
    std::unique_ptr<GEM::StructuredBuffer<AtmosphereData, 5>> mAtmosphereDataBuffer = nullptr;

    std::array<AtmosphereData, 5> mAtmosphereData;
    int atmosphereCount = 1;


};