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
}

void ShadowPass::Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) {
    std::array<float, 4> mClearColour = { 
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };
    auto* depthTarget = manager.GetDepthTarget(L"Default Depth");
    auto* outputImage = manager.GetRenderTarget(L"Shadow Mask");

    
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
    GraphicsCommands::BindUAV(4, outputImage->GetUAV());
    GraphicsCommands::DispatchRays(*mShaderTable, outputImage->GetWidth(), outputImage->GetHeight());

    GraphicsCommands::EndLabel();
}