#pragma once

#include <Core/Scene.h>

#include "Camera.h"

class TestScene : public GEM::Scene{
public:
    void Update(GEM::Input& input, GEM::Renderer& renderer, float dt) override;

private:
    Camera mCamera;
};