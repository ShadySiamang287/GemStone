#include "Renderer/Renderer.h"

#include "Renderer/RenderTargetManager.h"
#include "Graphics/GraphicsContext.h"

#include "Graphics/ResourceFactory.h"
#include "Graphics/GraphicsCommands.h"

#include "ImGui/ImGuiContext.h"

#include <imgui.h>

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

Renderer::Renderer(GraphicsContext& context, HWND window)
    : mContext(context) {
    mImGuiContext = std::make_unique<ImGuiContext>(context, window);

    mRenderTargetsPtr = std::make_unique<RenderTargetManager>(context);
    const UINT width = context.GetWidth();
    const UINT height = context.GetHeight();
    mColourTarget = &mRenderTargetsPtr->CreateRenderTarget(
        L"Colour GBuffer",
        DXGI_FORMAT_R8G8B8A8_UNORM,
        width,
        height,
        gbufferClearColour
        );
    mNormalTarget = &mRenderTargetsPtr->CreateRenderTarget(
        L"Normal GBuffer",
        DXGI_FORMAT_R8G8B8A8_UNORM,
        width,
        height,
        gbufferClearColour
    );
    mLightingTarget = &mRenderTargetsPtr->CreateRenderTarget(
        L"Scene Color",
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        width,
        height,
        sceneClearColour
    );
    auto mBloomTarget1 = &mRenderTargetsPtr->CreateRenderTarget(
        L"Bloom Ping",
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        width,
        height,
        sceneClearColour
    );
    auto mBloomTarget2 = &mRenderTargetsPtr->CreateRenderTarget(
        L"Bloom Pong",
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        width,
        height,
        sceneClearColour
    );
    
    mDepthTarget = &mRenderTargetsPtr->CreateDepthTarget(L"Default Depth", width, height);

    const glm::vec3 position{0.0f, 0.0f, -5.0f};
    const glm::vec3 target{0.0f, 0.0f, 0.0f};
    const glm::vec3 up{0.0f, 1.0f, 0.0f};

    mFrameData.ViewMatrix = glm::lookAtLH(position, target, up);

    const float fov = glm::radians(60.0f);
    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    mFrameData.ProjectionMatrix = glm::perspectiveLH_ZO(
        fov,
        aspectRatio,
        0.1f,      // near plane
        1000.0f    // far plane
    );

    mFrameDataBuffer = ResourceFactory::CreateConstantBuffer<FrameData>();
    mFrameDataBuffer->Upload(mFrameData);

    // mRenderPasses.push_back(std::make_unique<DeferredPass>(mFrameDataBuffer.get()));
    // mRenderPasses.push_back(std::make_unique<LightingPass>());
    // mRenderPasses.push_back(std::make_unique<ToneMappingPass>());


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

Renderer::~Renderer() = default;

void Renderer::Resize(UINT width, UINT height) {
    if (width == 0 || height == 0) {
        return;
    }

    if (width == mContext.GetWidth() && height == mContext.GetHeight()) {
        return;
    }

    mContext.Resize(width, height);
    mRenderTargetsPtr->Resize(width, height);

    mFrameData.ProjectionMatrix = glm::perspectiveLH_ZO(
        glm::radians(60.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        1000.0f
    );
    mFrameDataBuffer->Upload(mFrameData);
}

void Renderer::Render(Scene* scene) {
    GraphicsCommands::BeginFrame();
    mImGuiContext->BeginFrame();

    for (auto& pass : mRenderPasses) {
        pass->Render(*mRenderTargetsPtr.get(), mModels);
    }

    for (auto& pass : mRenderPasses){
        pass->OnImGuiRender();
    }

    ImGui::Begin("GemStone");
    ImGui::Text("Renderer online");
    ImGui::Text("Models: %zu", mModels.size());
    ImGui::End();

    mImGuiContext->Render(GraphicsCommands::GetCommandList());

    GraphicsCommands::EndFrame();
}
