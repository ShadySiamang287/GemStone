#pragma once

#include <Core/Scene.h>

class TestScene : public GEM::Scene{
public:
    void Update(GEM::Input& input, float dt) override;
};