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
    glm::vec3 ambientLight = { 1.f, 1.f, 1.f };
    float ambientStrength = { 0.5f };
    Camera mCamera;
};