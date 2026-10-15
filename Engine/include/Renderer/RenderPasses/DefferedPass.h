#pragma once

#include "Renderer/RenderPasses/RenderPass.h"
#include "Graphics/RenderingStructs.h"
#include "Graphics/ResourceFactory.h"

namespace GEM {
    class DeferredPass : public RenderPass {
    public:
        DeferredPass(ConstantBuffer<FrameData>* frameDataBuffer);
        void Render(RenderTargetManager& manager, std::span<Model> models) override;

    private:
        std::shared_ptr<RootSignature> mRootSig; 
        std::unique_ptr<GraphicsPipeline> mPipeline;
        ConstantBuffer<FrameData>* mFrameDataBuffer = nullptr;
    
        std::array<float, 4> mClearColour;
    };
}