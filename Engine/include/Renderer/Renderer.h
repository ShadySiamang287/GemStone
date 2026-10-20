#pragma once
#include "Graphics/GPUResource.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"
#include "Renderer/RenderTargetManager.h"
#include "Graphics/RenderingStructs.h"

#include "Renderer/RenderPasses/RenderPass.h"

namespace GEM{
    class GraphicsContext;
    class ImGuiContext;

    class GEM_API Renderer{
    public:
        Renderer(GraphicsContext& context, HWND window);
        ~Renderer();
        void Render();
        void Resize(UINT width, UINT height);

        void AddRenderPass(std::unique_ptr<RenderPass> pass) {
            mRenderPasses.push_back(std::move(pass));
        }

        ConstantBuffer<FrameData>* GetFrameDataBuffer() const {
            return mFrameDataBuffer.get();
        }

    private:
        GraphicsContext& mContext;
        std::unique_ptr<RenderTargetManager> mRenderTargetsPtr;
        RenderTarget* mColourTarget = nullptr;
        RenderTarget* mNormalTarget = nullptr;
        RenderTarget* mLightingTarget = nullptr;
        DepthTarget* mDepthTarget = nullptr;
    

        std::vector<std::unique_ptr<RenderPass>> mRenderPasses;
        FrameData mFrameData;
        std::unique_ptr<ConstantBuffer<FrameData>> mFrameDataBuffer;
        std::vector<Model> mModels;
        std::unique_ptr<ImGuiContext> mImGuiContext;
    
    };
}
