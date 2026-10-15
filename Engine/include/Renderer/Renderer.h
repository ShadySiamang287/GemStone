#pragma once
#include "Graphics/GPUResource.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"
#include "Renderer/RenderTargetManager.h"
#include "Graphics/RenderingStructs.h"

#include "Renderer/RenderPasses/RenderPass.h"

namespace GEM{
    class GraphicsContext;

    class Renderer{
    public:
        Renderer(GraphicsContext& context);
        void Render();

    private:
        std::unique_ptr<RenderTargetManager> mRenderTargetsPtr;
        RenderTarget* mColourTarget = nullptr;
        RenderTarget* mNormalTarget = nullptr;
        RenderTarget* mLightingTarget = nullptr;
        DepthTarget* mDepthTarget = nullptr;
    

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;
        //std::unique_ptr<GraphicsPipeline> mDefferedPipeline;
        // std::unique_ptr<GraphicsPipeline> mLightingPipeline;
        std::unique_ptr<GraphicsPipeline> mToneMappingPipeline;
        //std::shared_ptr<RootSignature> mRootSig;
        //std::shared_ptr<RootSignature> mLightingSig;
        std::shared_ptr<RootSignature> mFullscreenRootSig;
        // std::unique_ptr<VertexBuffer> mVertexBuffer;
        // std::unique_ptr<IndexBuffer> mIndexBuffer;
        FrameData mFrameData;
        std::unique_ptr<ConstantBuffer<FrameData>> mFrameDataBuffer;
        std::vector<Model> mModels;
    
    };
}
