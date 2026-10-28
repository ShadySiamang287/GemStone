#pragma once

#include <Core/Scene.h>

#include "Camera.h"

class TestScene : public GEM::Scene{
public:
    TestScene();
    void Update(GEM::Input& input, GEM::Renderer& renderer, float dt) override;

private:

    std::vector<GEM::Vertex> GeneratePlanetVertices();
    std::vector<uint32_t> GeneratePlanetIndices();

    Camera mCamera;
};