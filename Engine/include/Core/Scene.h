#pragma once

#include "Core/Input.h"

namespace GEM{
    class GEM_API Scene{
    public:
        virtual ~Scene() = default;

        virtual void Update(Input& input, float dt) {}
    };
}