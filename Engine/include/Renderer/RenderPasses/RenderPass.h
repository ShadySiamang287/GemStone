#pragma once

#include "Graphics/Model.h"
#include "Renderer/RenderTargetManager.h"

namespace GEM{
    class RenderPass{
    public:

        virtual ~RenderPass() = default;

        virtual void Render(RenderTargetManager& manager, std::span<Model> models) = 0;
    };
}