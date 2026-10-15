#include "Renderer/RenderPasses/LightingPass.h"

#include "Graphics/GraphicsCommands.h"
#include "pix3.h"

using namespace GEM;

LightingPass::LightingPass() {
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

    mRootSig = ResourceFactory::CreateRootSignature(lightingRootDesc);

    GraphicsPipelineDescription lightingDesc;
    lightingDesc.name = L"Lighting Pipeline";
    lightingDesc.rootSignature = mRootSig;
    lightingDesc.vertexShader = { "Shaders/FullscreenVS.dxil" };
    lightingDesc.pixelShader = { "Shaders/LightingPS.dxil" };
    lightingDesc.renderTargetFormats = { DXGI_FORMAT_R16G16B16A16_FLOAT };
    lightingDesc.enableDepth = false;

    mPipeline = ResourceFactory::CreateGraphicsPipeline(lightingDesc);

    mClearColour = { 
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };
}

void LightingPass::Render(RenderTargetManager& manager, std::span<Model> models) {
    auto* colourTarget = manager.GetRenderTarget(L"Colour GBuffer");
    auto* normalTarget = manager.GetRenderTarget(L"Normal GBuffer");
    auto* depthTarget = manager.GetDepthTarget(L"Default Depth");
    auto* lightingTarget = manager.GetRenderTarget(L"Scene Color");


    GraphicsCommands::BeginLabel(PIX_COLOR(255, 82, 124), "Lighting Pass");
    GraphicsCommands::TransitionRTV(
        *colourTarget,
        colourTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::TransitionRTV(
        *normalTarget,
        normalTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::TransitionDSV(
        *depthTarget, depthTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::TransitionRTV(
        *lightingTarget,
        lightingTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );


    std::array<RenderTarget*, 1> lightingTargets = {
        lightingTarget
    };

    GraphicsCommands::BindRenderTargets(lightingTargets);
    GraphicsCommands::ClearRenderTarget(*lightingTarget, mClearColour.data());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    GraphicsCommands::BindSRV(0, colourTarget->GetSRV());
    GraphicsCommands::BindSRV(1, normalTarget->GetSRV());
    GraphicsCommands::BindSRV(2, depthTarget->GetSRV());
    GraphicsCommands::Draw(3, 1, 0, 0);
    GraphicsCommands::EndLabel();
}