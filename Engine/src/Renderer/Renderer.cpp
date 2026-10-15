#include "Renderer/Renderer.h"

#include "Renderer/RenderTargetManager.h"
#include "Graphics/GraphicsContext.h"

#include "Graphics/ResourceFactory.h"
#include "Graphics/GraphicsCommands.h"

#include "Renderer/RenderPasses/DefferedPass.h"
#include "Renderer/RenderPasses/LightingPass.h"
#include "Renderer/RenderPasses/ToneMappingPass.h"

#include "pix3.h"

using namespace GEM;

namespace {
    constexpr std::array<float, 4> sceneClearColour = {
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };

    constexpr std::array<float, 4> gbufferClearColour = {
        0.f, 0.f, 0.f, 1.f
    };
}

Renderer::Renderer(GraphicsContext& context) {
    mRenderTargetsPtr = std::make_unique<RenderTargetManager>(context);
    mColourTarget = &mRenderTargetsPtr->CreateRenderTarget(
        L"Colour GBuffer",
        DXGI_FORMAT_R8G8B8A8_UNORM,
        1280,
        720,
        gbufferClearColour
        );
    mNormalTarget = &mRenderTargetsPtr->CreateRenderTarget(
        L"Normal GBuffer",
        DXGI_FORMAT_R8G8B8A8_UNORM,
        1280,
        720,
        gbufferClearColour
    );
    mLightingTarget = &mRenderTargetsPtr->CreateRenderTarget(
        L"Scene Color",
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        1280,
        720,
        sceneClearColour
    );
    
    mDepthTarget = &mRenderTargetsPtr->CreateDepthTarget(L"Default Depth", 1280, 720);

    DirectX::XMVECTOR position = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
    DirectX::XMVECTOR target   = DirectX::XMVectorSet(0.0f, 0.0f,  0.0f, 1.0f);
    DirectX::XMVECTOR up       = DirectX::XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f);

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(
        position,       
        target,
        up
    );
    DirectX::XMStoreFloat4x4(&mFrameData.ViewMatrix, view);

    float fov = DirectX::XMConvertToRadians(60.0f);
    float aspectRatio = static_cast<float>(1280) / static_cast<float>(720);

    DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
        fov,
        aspectRatio,
        0.1f,      // near plane
        1000.0f    // far plane
    );

    DirectX::XMStoreFloat4x4(&mFrameData.ProjectionMatrix, projection);

    mFrameDataBuffer = ResourceFactory::CreateConstantBuffer<FrameData>();
    mFrameDataBuffer->Upload(mFrameData);

    mRenderPasses.push_back(std::make_unique<DeferredPass>(mFrameDataBuffer.get()));
    mRenderPasses.push_back(std::make_unique<LightingPass>());
    mRenderPasses.push_back(std::make_unique<ToneMappingPass>());


    std::array<Vertex, 4> vertices = {
        Vertex{{-0.5f, 0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        Vertex{{0.5f, 0.5f, 0.f}, {0.f, 0.f, 1.f},{1.f, 1.f, 1.f, 1.f}},
        Vertex{{-0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        Vertex{{0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
    };
    
    std::array<uint32_t, 6> indices = {
        0, 1, 2,
        1, 3, 2
    };

    Model model;
    model.primitives.push_back({
        ResourceFactory::CreateVertexBuffer(vertices),
        ResourceFactory::CreateIndexBuffer(indices)
    });
    mModels.push_back(std::move(model));
}

void Renderer::Render() {
    GraphicsCommands::BeginFrame();

    for (auto& pass : mRenderPasses) {
        pass->Render(*mRenderTargetsPtr.get(), mModels);
    }

    GraphicsCommands::EndFrame();
}
