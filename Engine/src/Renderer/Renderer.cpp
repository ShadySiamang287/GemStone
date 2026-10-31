#include "Renderer/Renderer.h"

#include "Renderer/RenderTargetManager.h"
#include "Graphics/GraphicsContext.h"

#include "Graphics/ResourceFactory.h"
#include "Graphics/GraphicsCommands.h"

#include "ImGui/ImGuiContext.h"

#include <imgui.h>

#include "Core/Scene.h"

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

    const float fov = glm::radians(60.0f);
    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    mFrameData.ProjectionMatrix = glm::perspectiveRH_ZO(
        fov,
        aspectRatio,
        0.1f,      // near plane
        1000.0f    // far plane
    );
    mFrameData.inverseProjMat = glm::inverse(mFrameData.ProjectionMatrix);

    mFrameDataBuffer = ResourceFactory::CreateConstantBuffer<FrameData>();
    mFrameDataBuffer->Upload(mFrameData);
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

    mFrameData.ProjectionMatrix = glm::perspectiveRH_ZO(
        glm::radians(60.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        1000.0f
    );
    mFrameData.inverseProjMat = glm::inverse(mFrameData.ProjectionMatrix);
    mFrameDataBuffer->Upload(mFrameData);
}

void Renderer::Render(Scene* scene) {
    GraphicsCommands::BeginFrame();
    mImGuiContext->BeginFrame();
    mFrameDataBuffer->Upload(mFrameData);

    auto models = scene->GetModels();

    for (auto& pass : mRenderPasses) {
        pass->Render(*mRenderTargetsPtr.get(), models);
    }

    for (auto& pass : mRenderPasses){
        pass->OnImGuiRender();
    }

    ImGui::Begin("GemStone");
    ImGui::Text("Renderer online");
    ImGui::Text("Models: %zu", models.size());
    ImGui::End();

    mImGuiContext->Render(GraphicsCommands::GetCommandList());

    GraphicsCommands::EndFrame();
}
