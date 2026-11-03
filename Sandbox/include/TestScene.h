#pragma once

#include <Core/Scene.h>

#include "Camera.h"

class TestScene : public GEM::Scene{
public:
    TestScene();
    void Update(GEM::Input& input, GEM::Renderer& renderer, float dt) override;

    void OnImGuiRenderer() override;
private:

    std::vector<GEM::Vertex> GeneratePlanetVertices();
    std::vector<uint32_t> GeneratePlanetIndices();

    std::vector<GEM::PointLight> mLights;

    std::unique_ptr<GEM::ComputePipeline> mComputePipeline = nullptr;
    std::shared_ptr<GEM::RootSignature> mRootSig = nullptr; 
    Camera mCamera;
};