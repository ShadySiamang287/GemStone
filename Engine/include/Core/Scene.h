#pragma once

#include "Core/Input.h"
#include "Renderer/Renderer.h"

namespace GEM{
    class GEM_API Scene{
    public:
        virtual ~Scene() = default;

        virtual void Update(Input& input, GEM::Renderer& renderer, float dt) {}
    };
}