#include "Renderer/Renderer.h"

#include "Renderer/RenderTargetManager.h"
#include "Graphics/GraphicsContext.h"

#include "Graphics/ResourceFactory.h"
#include "Graphics/GraphicsCommands.h"

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

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.parameters.push_back(frameDataParameter);
    rootDesc.name = L"Deferred Root Signature";
    
    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);

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

    RootSignatureDesc lightingRootDesc;
    lightingRootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    lightingRootDesc.name = L"Lighting Root Signature";

    D3D12_DESCRIPTOR_RANGE albedoRange{};
    albedoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    albedoRange.NumDescriptors = 1;
    albedoRange.BaseShaderRegister = 0;
    albedoRange.RegisterSpace = 0;
    albedoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE normalRange{};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE depthRange{};
    depthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    depthRange.NumDescriptors = 1;
    depthRange.BaseShaderRegister = 2;
    depthRange.RegisterSpace = 0;
    depthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER albedoParam{};
    albedoParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    albedoParam.DescriptorTable.NumDescriptorRanges = 1;
    albedoParam.DescriptorTable.pDescriptorRanges = &albedoRange;
    albedoParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_PARAMETER normalParam{};
    normalParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    normalParam.DescriptorTable.NumDescriptorRanges = 1;
    normalParam.DescriptorTable.pDescriptorRanges = &normalRange;
    normalParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_PARAMETER depthParam{};
    depthParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    depthParam.DescriptorTable.NumDescriptorRanges = 1;
    depthParam.DescriptorTable.pDescriptorRanges = &depthRange;
    depthParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    lightingRootDesc.parameters.push_back(albedoParam);
    lightingRootDesc.parameters.push_back(normalParam);
    lightingRootDesc.parameters.push_back(depthParam);
    lightingRootDesc.samplers.push_back(linearWrap);

    mLightingSig = ResourceFactory::CreateRootSignature(lightingRootDesc);

    GraphicsPipelineDescription lightingDesc;
    lightingDesc.name = L"Lighting Pipeline";
    lightingDesc.rootSignature = mLightingSig;
    lightingDesc.vertexShader = { "Shaders/FullscreenVS.dxil" };
    lightingDesc.pixelShader = { "Shaders/LightingPS.dxil" };
    lightingDesc.renderTargetFormats = { DXGI_FORMAT_R16G16B16A16_FLOAT };
    lightingDesc.enableDepth = false;

    mLightingPipeline = ResourceFactory::CreateGraphicsPipeline(lightingDesc);

    GraphicsPipelineDescription toneMappingDesc;
    toneMappingDesc.name = L"Tone Mapping Pipeline";
    toneMappingDesc.rootSignature = mFullscreenRootSig;
    toneMappingDesc.vertexShader = {"Shaders/FullscreenVS.dxil"};
    toneMappingDesc.pixelShader = {"Shaders/ToneMappingPS.dxil"};
    toneMappingDesc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};
    toneMappingDesc.enableDepth = false;

    mToneMappingPipeline = ResourceFactory::CreateGraphicsPipeline(toneMappingDesc);

    GraphicsPipelineDescription desc;
    desc.name = L"Deferred Pipeline";
    desc.rootSignature = mRootSig;
    desc.inputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    desc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
    desc.vertexShader = {"Shaders/TriangleVS.dxil"};
    desc.pixelShader = {"Shaders/TrianglePS.dxil"};
    desc.enableDepth = true;
    mDefferedPipeline = ResourceFactory::CreateGraphicsPipeline(desc);





    std::array<Vertex, 4> vertices = {
        Vertex{{-0.5f, 0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        Vertex{{0.5f, 0.5f, 0.f}, {0.f, 0.f, 1.f},{1.f, 1.f, 1.f, 1.f}},
        Vertex{{-0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        Vertex{{0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
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
    GraphicsCommands::BeginLabel(PIX_COLOR(255, 128, 0), "forward pass");

    GraphicsCommands::TransitionRTV(
        *mColourTarget,
        mColourTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    GraphicsCommands::TransitionRTV(
        *mNormalTarget,
        mNormalTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    GraphicsCommands::TransitionDSV(
        *mDepthTarget, mDepthTarget->GetState(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );

    std::array<RenderTarget*, 2> sceneTargets = {
        mColourTarget, mNormalTarget
    };

    GraphicsCommands::BindRenderTargets(sceneTargets, mDepthTarget);

    GraphicsCommands::ClearRenderTarget(*mColourTarget, gbufferClearColour.data());
    GraphicsCommands::ClearRenderTarget(*mNormalTarget, gbufferClearColour.data());
    GraphicsCommands::ClearDepthStencil(*mDepthTarget);

    GraphicsCommands::BindGraphicsPipeline(*mDefferedPipeline.get());
    GraphicsCommands::SetConstantBufferView(0, mFrameDataBuffer->GetAddress());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindVertexBuffer(*mVertexBuffer.get());
    GraphicsCommands::BindIndexBuffer(*mIndexBuffer.get());
    GraphicsCommands::DrawIndexed(6, 1, 0, 0, 0);
    GraphicsCommands::EndLabel();


    GraphicsCommands::BeginLabel(PIX_COLOR(255, 82, 124), "Lighting Pass");
    GraphicsCommands::TransitionRTV(
        *mColourTarget,
        mColourTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::TransitionRTV(
        *mNormalTarget,
        mNormalTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::TransitionDSV(
        *mDepthTarget, mDepthTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::TransitionRTV(
        *mLightingTarget,
        mLightingTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );


    std::array<RenderTarget*, 1> lightingTargets = {
        mLightingTarget
    };

    GraphicsCommands::BindRenderTargets(lightingTargets);
    GraphicsCommands::ClearRenderTarget(*mLightingTarget, sceneClearColour.data());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindGraphicsPipeline(*mLightingPipeline.get());
    GraphicsCommands::BindSRV(0, mColourTarget->GetSRV());
    GraphicsCommands::BindSRV(1, mNormalTarget->GetSRV());
    GraphicsCommands::BindSRV(2, mDepthTarget->GetSRV());
    GraphicsCommands::Draw(3, 1, 0, 0);
    GraphicsCommands::EndLabel();

    GraphicsCommands::BeginLabel(PIX_COLOR(72, 123, 255), "Tone Mapping");
    GraphicsCommands::TransitionRTV(
        *mLightingTarget, mLightingTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    GraphicsCommands::BindRenderTargets();
    GraphicsCommands::ClearSwapchainImage(sceneClearColour.data());

    GraphicsCommands::BindGraphicsPipeline(*mToneMappingPipeline.get());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindSRV(0, mLightingTarget->GetSRV());
    GraphicsCommands::Draw(3, 1, 0, 0);
    GraphicsCommands::EndLabel();

    GraphicsCommands::EndFrame();
}
