#pragma once
#include "Graphics/GPUResource.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"
#include "Renderer/RenderTargetManager.h"

namespace GEM{
    class GraphicsContext;

    class Renderer{
    public:
        Renderer(GraphicsContext& context);
        void Render();

    private:
        std::unique_ptr<RenderTargetManager> mRenderTargetsPtr;
    
        std::unique_ptr<GraphicsPipeline> mPipeline;
        std::shared_ptr<RootSignature> mRootSig;
        std::unique_ptr<VertexBuffer> mVertexBuffer;
        std::unique_ptr<IndexBuffer> mIndexBuffer;
    };
}