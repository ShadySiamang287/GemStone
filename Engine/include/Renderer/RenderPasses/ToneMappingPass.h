#pragma once

#include "Renderer/RenderPasses/RenderPass.h"
#include "Graphics/ResourceFactory.h"

namespace GEM {
    class ToneMappingPass : public RenderPass{
    public:
        ToneMappingPass();
        void Render(RenderTargetManager& manager, std::span<Model> models) override;
    private:
        std::shared_ptr<RootSignature> mRootSig; 
        std::unique_ptr<GraphicsPipeline> mPipeline;   

        std::array<float, 4> mClearColour;
    };
}