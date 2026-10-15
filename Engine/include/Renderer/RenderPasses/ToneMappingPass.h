#pragma once

#include "Renderer/RenderPasses/RenderPass.h"
#include "Graphics/ResourceFactory.h"

namespace GEM {
    struct ToneMappingData{
        float exposure = 0.1;
    };

    class ToneMappingPass : public RenderPass{
    public:
        ToneMappingPass();
        void Render(RenderTargetManager& manager, std::span<Model> models) override;
    
        void OnImGuiRender() override;
    private:
        std::shared_ptr<RootSignature> mRootSig; 
        std::unique_ptr<GraphicsPipeline> mPipeline;
        ToneMappingData mData;
        std::unique_ptr<ConstantBuffer<ToneMappingData>> mConstantBuffer;

        std::array<float, 4> mClearColour;
    };
}