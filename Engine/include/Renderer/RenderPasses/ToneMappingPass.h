#pragma once

#include "Renderer/RenderPasses/RenderPass.h"
#include "Graphics/ResourceFactory.h"

namespace GEM {
    enum class ToneMappingType : uint16_t {
        ReinhardLuminance,
        AECS,
        UNCHARTED,
        AGX
    };

    struct ToneMappingData{
        float exposure = 0.1;
        float hableLuminance = 11.2;
        ToneMappingType type = ToneMappingType::ReinhardLuminance;
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