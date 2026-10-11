#include "Renderer/Renderer.h"

#include "Renderer/RenderTargetManager.h"
#include "Graphics/GraphicsContext.h"

#include "Graphics/ResourceFactory.h"
#include "Graphics/GraphicsCommands.h"

using namespace GEM;

namespace {
    constexpr std::array<float, 4> sceneClearColour = {
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };
}

Renderer::Renderer(GraphicsContext& context) {
    mRenderTargetsPtr = std::make_unique<RenderTargetManager>(context);
    mSceneTarget = &mRenderTargetsPtr->CreateRenderTarget(
        L"Scene Color",
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        1280,
        720,
        sceneClearColour
    );
    mDepthTarget = &mRenderTargetsPtr->CreateDepthTarget(L"Default Depth", 1280, 720);

    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.name = L"Root Signature";

    D3D12_STATIC_SAMPLER_DESC linearWrap{};
    linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.MipLODBias = 0.0f;
    linearWrap.MaxAnisotropy = 1;
    linearWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    linearWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    linearWrap.MinLOD = 0.0f;
    linearWrap.MaxLOD = D3D12_FLOAT32_MAX;
    linearWrap.ShaderRegister = 0;
    linearWrap.RegisterSpace = 0;
    linearWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);

    RootSignatureDesc fullscreenRootDesc;
    fullscreenRootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    fullscreenRootDesc.name = L"Fullscreen Root Signature";

    D3D12_DESCRIPTOR_RANGE fullscreenSRVRange{};
    fullscreenSRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    fullscreenSRVRange.NumDescriptors = 1;
    fullscreenSRVRange.BaseShaderRegister = 0;
    fullscreenSRVRange.RegisterSpace = 0;
    fullscreenSRVRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER fullscreenSRVParameter{};
    fullscreenSRVParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    fullscreenSRVParameter.DescriptorTable.NumDescriptorRanges = 1;
    fullscreenSRVParameter.DescriptorTable.pDescriptorRanges = &fullscreenSRVRange;
    fullscreenSRVParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    fullscreenRootDesc.parameters.push_back(fullscreenSRVParameter);
    fullscreenRootDesc.samplers.push_back(linearWrap);

    mFullscreenRootSig = ResourceFactory::CreateRootSignature(fullscreenRootDesc);

    GraphicsPipelineDescription toneMappingDesc;
    toneMappingDesc.name = L"Tone Mapping Pipeline";
    toneMappingDesc.rootSignature = mFullscreenRootSig;
    toneMappingDesc.vertexShader = {"Shaders/FullscreenVS.dxil"};
    toneMappingDesc.pixelShader = {"Shaders/ToneMappingPS.dxil"};
    toneMappingDesc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};
    toneMappingDesc.enableDepth = false;

    mToneMappingPipeline = ResourceFactory::CreateGraphicsPipeline(toneMappingDesc);

    GraphicsPipelineDescription desc;
    desc.name = L"Triangle Test Pipeline";
    desc.rootSignature = mRootSig;
    desc.inputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    desc.renderTargetFormats = {DXGI_FORMAT_R16G16B16A16_FLOAT};
    desc.vertexShader = {"Shaders/TriangleVS.dxil"};
    desc.pixelShader = {"Shaders/TrianglePS.dxil"};
    desc.enableDepth = true;
    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);

    std::array<Vertex, 4> vertices = {
        Vertex{{-0.5f, 0.5f, 0.f}, {1.f, 0.f, 0.f, 1.f}},
        Vertex{{0.5f, 0.5f, 0.f}, {0.f, 0.f, 1.f, 1.f}},
        Vertex{{-0.5f, -0.5f, 0.f}, {0.f, 1.f, 0.f, 1.f}},
        Vertex{{0.5f, -0.5f, 0.f}, {0.f, 1.f, 1.f, 1.f}},
    };
    mVertexBuffer = ResourceFactory::CreateVertexBuffer(vertices);
    
    std::array<uint32_t, 6> indices = {
        0, 1, 2,
        1, 3, 2
    };
    mIndexBuffer = ResourceFactory::CreateIndexBuffer(indices);

}

void Renderer::Render() {
    GraphicsCommands::BeginFrame();

    GraphicsCommands::TransitionRTV(
        *mSceneTarget,
        mSceneTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    std::array<RenderTarget*, 1> sceneTargets = {
        mSceneTarget
    };

    GraphicsCommands::BindRenderTargets(sceneTargets, mDepthTarget);

    GraphicsCommands::ClearRenderTarget(*mSceneTarget, sceneClearColour.data());
    GraphicsCommands::ClearDepthStencil(*mDepthTarget);

    GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindVertexBuffer(*mVertexBuffer.get());
    GraphicsCommands::BindIndexBuffer(*mIndexBuffer.get());
    GraphicsCommands::DrawIndexed(6, 1, 0, 0, 0);

    GraphicsCommands::TransitionRTV(
        *mSceneTarget,
        mSceneTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::BindRenderTargets();
    GraphicsCommands::ClearSwapchainImage(sceneClearColour.data());

    GraphicsCommands::BindGraphicsPipeline(*mToneMappingPipeline.get());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindSRV(0, mSceneTarget->GetSRV());
    GraphicsCommands::Draw(3, 1, 0, 0);

    GraphicsCommands::EndFrame();
}
