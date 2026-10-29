#pragma once

#include "Renderer/RenderPasses/RenderPass.h"
#include "Graphics/RenderingStructs.h"
#include "Graphics/ResourceFactory.h"

using namespace GEM;

struct PlanetData {
    float radius = 10.f;
    float roughness = 2.5f;
    float persistence = 0.4f;
    float terrainStrength = 1.5f;
};

struct TesselationData {
    float maxTess = 14.f;
    float nearDist = 10.f;
    float farDist = 90.f;
};

class DeferredPass : public RenderPass {
public:
    DeferredPass(ConstantBuffer<FrameData>* frameDataBuffer);
    void Render(RenderTargetManager& manager, std::span<Model> models) override;
	void OnImGuiRender() override;

private:
    std::shared_ptr<RootSignature> mRootSig; 
    std::unique_ptr<GraphicsPipeline> mPipeline;
    ConstantBuffer<FrameData>* mFrameDataBuffer = nullptr;
	std::unique_ptr<ConstantBuffer<PlanetData>> mPlanetDataBuffer = nullptr;
	std::unique_ptr<ConstantBuffer<TesselationData>> mTesselationDataBuffer = nullptr;

    PlanetData mPlanetData;
    TesselationData mTesselationData;

    std::array<float, 4> mClearColour;
};