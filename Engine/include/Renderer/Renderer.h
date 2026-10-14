#pragma once
#include "Graphics/GPUResource.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"
#include "Renderer/RenderTargetManager.h"
#include "Graphics/RenderingStructs.h"

namespace GEM{
    class GraphicsContext;

    class Renderer{
    public:
        Renderer(GraphicsContext& context);
        void Render();

    private:
        std::unique_ptr<RenderTargetManager> mRenderTargetsPtr;
        RenderTarget* mSceneTarget = nullptr;
        DepthTarget* mDepthTarget = nullptr;
    
        std::unique_ptr<GraphicsPipeline> mPipeline;
        std::unique_ptr<GraphicsPipeline> mToneMappingPipeline;
        std::shared_ptr<RootSignature> mRootSig;
        std::shared_ptr<RootSignature> mFullscreenRootSig;
        std::unique_ptr<VertexBuffer> mVertexBuffer;
        std::unique_ptr<IndexBuffer> mIndexBuffer;
        FrameData mFrameData;
        std::unique_ptr<ConstantBuffer<FrameData>> mFrameDataBuffer;
    };
}
