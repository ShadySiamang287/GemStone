#include "Renderer/RenderPasses/ToneMappingPass.h"

#include "Graphics/GraphicsCommands.h"
#include "pix3.h"

using namespace GEM;

ToneMappingPass::ToneMappingPass() {
    mClearColour = {
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };
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

    mRootSig = ResourceFactory::CreateRootSignature(fullscreenRootDesc);

    GraphicsPipelineDescription toneMappingDesc;
    toneMappingDesc.name = L"Tone Mapping Pipeline";
    toneMappingDesc.rootSignature = mRootSig;
    toneMappingDesc.vertexShader = {"Shaders/FullscreenVS.dxil"};
    toneMappingDesc.pixelShader = {"Shaders/ToneMappingPS.dxil"};
    toneMappingDesc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};
    toneMappingDesc.enableDepth = false;

    mPipeline = ResourceFactory::CreateGraphicsPipeline(toneMappingDesc);
}

void ToneMappingPass::Render(RenderTargetManager& manager, std::span<Model> models) {
    
    auto* lightingTarget = manager.GetRenderTarget(L"Scene Color");

    GraphicsCommands::BeginLabel(PIX_COLOR(72, 123, 255), "Tone Mapping");
    GraphicsCommands::TransitionRTV(
        *lightingTarget, lightingTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    GraphicsCommands::BindRenderTargets();
    GraphicsCommands::ClearSwapchainImage(mClearColour.data());

    GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindSRV(0, lightingTarget->GetSRV());
    GraphicsCommands::Draw(3, 1, 0, 0);
    GraphicsCommands::EndLabel();
}