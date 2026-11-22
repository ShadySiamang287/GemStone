#include "RenderPasses/ShadowPass.h"

#include <pix3.h>

using namespace GEM;

ShadowPass::ShadowPass(GEM::ConstantBuffer<GEM::FrameData>* frameDataBuffer,  GEM::StructuredBuffer<GEM::PointLight, 1024>* lightBuffer, GEM::TLAS* tlas) : 
    mFrameDataBuffer(frameDataBuffer), mLightBuffer(lightBuffer), tlasPtr(tlas)
{
    
    RootSignatureDesc rootDesc{};
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    rootDesc.name = L"Shadow Root Sig";

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    D3D12_DESCRIPTOR_RANGE accelRange{};
    accelRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    accelRange.NumDescriptors = 1;
    accelRange.BaseShaderRegister = 0;
    accelRange.RegisterSpace = 0;
    accelRange.OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER accelParam{};
    accelParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    accelParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    accelParam.DescriptorTable.NumDescriptorRanges = 1;
    accelParam.DescriptorTable.pDescriptorRanges = &accelRange;

    D3D12_DESCRIPTOR_RANGE lightRange = {};
    lightRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    lightRange.NumDescriptors = 1;
    lightRange.BaseShaderRegister = 1;   // t0
    lightRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER lightingParam;
    lightingParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    lightingParam.DescriptorTable.NumDescriptorRanges = 1;
    lightingParam.DescriptorTable.pDescriptorRanges = &lightRange;
    lightingParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;    

    D3D12_DESCRIPTOR_RANGE depthRange{};
    depthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    depthRange.NumDescriptors = 1;
    depthRange.BaseShaderRegister = 2;
    depthRange.RegisterSpace = 0;
    depthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER depthParam{};
    depthParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    depthParam.DescriptorTable.NumDescriptorRanges = 1;
    depthParam.DescriptorTable.pDescriptorRanges = &depthRange;
    depthParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE normalRange{};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 3;
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER normalParam{};
    normalParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    normalParam.DescriptorTable.NumDescriptorRanges = 1;
    normalParam.DescriptorTable.pDescriptorRanges = &normalRange;
    normalParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE uavRange{};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0; // u0
    uavRange.RegisterSpace = 0;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    
    D3D12_ROOT_PARAMETER outputRT{};
    outputRT.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    outputRT.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    outputRT.DescriptorTable.NumDescriptorRanges = 1;
    outputRT.DescriptorTable.pDescriptorRanges = &uavRange;

    rootDesc.parameters.push_back(frameDataParameter);
    rootDesc.parameters.push_back(accelParam);
    rootDesc.parameters.push_back(lightingParam);
    rootDesc.parameters.push_back(depthParam);
    rootDesc.parameters.push_back(normalParam);
    rootDesc.parameters.push_back(outputRT);

    mRTRootSig = ResourceFactory::CreateRootSignature(rootDesc);

    D3D12_HIT_GROUP_DESC defaultGroup{};
    defaultGroup.HitGroupExport = L"DefaultGroup";
    defaultGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    
    RTPipelineDesc pipelineDesc{};
    pipelineDesc.hitGroups.push_back(defaultGroup);
    pipelineDesc.libraryBytecodes.push_back({"Shaders/ShadowRT.dxil"});
    pipelineDesc.globalRootSig = mRTRootSig;

    mRTPipeline = ResourceFactory::CreateRTPipeline(pipelineDesc);

    ShaderTableDesc stDesc{};
    stDesc.pipeline = mRTPipeline.get();
    stDesc.rayGen = {L"shadowRayGen"};
    stDesc.missShaders.push_back({L"miss"});
    stDesc.hitGroups.push_back({L"DefaultGroup"});
    mShaderTable =  ResourceFactory::CreateShaderTable(stDesc);

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
    fullscreenRootDesc.name = L"Bilateral Filter Root Signature";

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

    GraphicsPipelineDescription pipDesc;
    pipDesc.name = L"Bilateral Filter Pipeline";
    pipDesc.rootSignature = mRootSig;
    pipDesc.vertexShader = {"Shaders/FullscreenVS.dxil"};
    pipDesc.pixelShader = {"Shaders/BileteralFilterPS.dxil"};
    pipDesc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM};
    pipDesc.depthTest = false;
    pipDesc.depthWrite = false;

    mPipeline = ResourceFactory::CreateGraphicsPipeline(pipDesc);

}

void ShadowPass::Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) {
    std::array<float, 4> mClearColour = { 
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };
    auto* depthTarget = manager.GetDepthTarget(L"Default Depth");
    auto* outputImage = manager.GetRenderTarget(L"Shadow Mask");
    auto* deNoiseImage = manager.GetRenderTarget(L"Shadow Ping");
    auto* normalTarget = manager.GetRenderTarget(L"Normal GBuffer");

    
    GraphicsCommands::BeginLabel(PIX_COLOR(255, 128, 52), "Shadow pass");

    GraphicsCommands::TransitionRTV(
        *outputImage,
        outputImage->GetState(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    GraphicsCommands::TransitionDSV(
        *depthTarget,
        depthTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    GraphicsCommands::BindRaytracingPipeline(*mRTPipeline);
    GraphicsCommands::SetComputeConstantBufferView(0, mFrameDataBuffer->GetAddress());
    GraphicsCommands::BindComputeSRV(1, tlasPtr->GetSRV());
    GraphicsCommands::BindComputeSRV(2, mLightBuffer->GetSRV());
    GraphicsCommands::BindComputeSRV(3, depthTarget->GetSRV());
    GraphicsCommands::BindComputeSRV(4, normalTarget->GetSRV());
    GraphicsCommands::BindUAV(5, outputImage->GetUAV());
    GraphicsCommands::DispatchRays(*mShaderTable, outputImage->GetWidth(), outputImage->GetHeight());

    GraphicsCommands::TransitionRTV(
        *outputImage,
        outputImage->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    GraphicsCommands::TransitionRTV(
        *deNoiseImage,
        deNoiseImage->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    std::array<RenderTarget*, 1> targets = {
        deNoiseImage
    };

    GraphicsCommands::ClearRenderTarget(*deNoiseImage, mClearColour.data());
    GraphicsCommands::BindRenderTargets(targets);
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

    GraphicsCommands::BindGraphicsPipeline(*mPipeline);
    GraphicsCommands::BindSRV(0, outputImage->GetSRV());
    GraphicsCommands::Draw(3, 1, 0, 0);
    GraphicsCommands::EndLabel();
}