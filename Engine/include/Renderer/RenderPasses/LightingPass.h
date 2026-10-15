#pragma once

#include "Renderer/RenderPasses/RenderPass.h"
#include "Graphics/ResourceFactory.h"

namespace GEM {
    class LightingPass : public RenderPass {
    public:
        LightingPass();
        void Render(RenderTargetManager& manager, std::span<Model> models) override;

    private:
        std::shared_ptr<RootSignature> mRootSig; 
        std::unique_ptr<GraphicsPipeline> mPipeline;
    
        std::array<float, 4> mClearColour;
    };
}