#include "RenderPasses/BloomPass.h"

#include "pix3.h"

using namespace GEM;

BloomPass::BloomPass() {
    D3D12_STATIC_SAMPLER_DESC linearWrap{};
    linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
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
    fullscreenRootDesc.name = L"Bloom Root Signature";

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

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    fullscreenRootDesc.parameters.push_back(fullscreenSRVParameter);
    fullscreenRootDesc.parameters.push_back(frameDataParameter);
    fullscreenRootDesc.samplers.push_back(linearWrap);

    mRootSig = ResourceFactory::CreateRootSignature(fullscreenRootDesc);
    
    GraphicsPipelineDescription bloomDesc;
    bloomDesc.name = L"Bloom Pipeline";
    bloomDesc.rootSignature = mRootSig;
    bloomDesc.vertexShader = {"Shaders/FullscreenVS.dxil"};
    bloomDesc.pixelShader = {"Shaders/BloomPS.dxil"};
    bloomDesc.renderTargetFormats = {DXGI_FORMAT_R16G16B16A16_FLOAT};
    bloomDesc.depthTest = false;
    bloomDesc.depthWrite = false;

    mPipeline = ResourceFactory::CreateGraphicsPipeline(bloomDesc);

    mConstantBuffer = ResourceFactory::CreateConstantBuffer<BloomData>();
}

void BloomPass::Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) {
    auto* bloomPing = manager.GetRenderTarget(L"Bloom Ping");
    auto* bloomPong = manager.GetRenderTarget(L"Bloom Pong");
    mData.horizontal = true;
    mConstantBuffer->Upload(mData);

    GraphicsCommands::BeginLabel(PIX_COLOR(172, 123, 48), "Bloom Pass");
    GraphicsCommands::TransitionRTV(
        *bloomPing, bloomPing->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    GraphicsCommands::TransitionRTV(
        *bloomPong, bloomPong->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    std::array<RenderTarget*, 1> targets = {
        bloomPong
    };

    GraphicsCommands::BindRenderTargets(targets);
    GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    GraphicsCommands::BindViewport(
        0.f,
        0.f,
        static_cast<float>(manager.GetWidth()),
        static_cast<float>(manager.GetHeight()),
        0.f,
        1.f
    );
    GraphicsCommands::BindScissorRect(
        0,
        0,
        static_cast<int>(manager.GetWidth()),
        static_cast<int>(manager.GetHeight())
    );
    GraphicsCommands::BindSRV(0, bloomPing->GetSRV());
    GraphicsCommands::SetGraphicsConstantBufferView(1, mConstantBuffer->GetAddress());
        std::array<float, 4> clearColor = {
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };
    GraphicsCommands::ClearRenderTarget(*bloomPong, clearColor.data());
    GraphicsCommands::Draw(3, 1, 0, 0);
    GraphicsCommands::TransitionRTV(
        *bloomPing, bloomPing->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    GraphicsCommands::TransitionRTV(
        *bloomPong, bloomPong->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    GraphicsCommands::BindSRV(0, bloomPong->GetSRV());
    targets = {
        bloomPing
    };
    mData.horizontal = false;
    mConstantBuffer->Upload(mData);
    GraphicsCommands::SetGraphicsConstantBufferView(1, mConstantBuffer->GetAddress());

    GraphicsCommands::BindRenderTargets(targets);
    GraphicsCommands::Draw(3, 1, 0, 0);

    GraphicsCommands::EndLabel();
}

void BloomPass::OnImGuiRender(){

}