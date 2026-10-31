#include "RenderPasses/AtmospherePass.h"

#include <pix3.h>
#include <imgui.h>

using namespace GEM;

AtmospherePass::AtmospherePass(ConstantBuffer<FrameData>* frameDataBuffer){
    mFrameDataBuffer = frameDataBuffer;

    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.name = L"Atmosphere Root Signature";

    D3D12_DESCRIPTOR_RANGE depthRange{};
    depthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    depthRange.NumDescriptors = 1;
    depthRange.BaseShaderRegister = 0;
    depthRange.RegisterSpace = 0;
    depthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER depthParam{};
    depthParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    depthParam.DescriptorTable.NumDescriptorRanges = 1;
    depthParam.DescriptorTable.pDescriptorRanges = &depthRange;
    depthParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    D3D12_ROOT_PARAMETER atmosphereData {};
    atmosphereData.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    atmosphereData.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    atmosphereData.Descriptor.ShaderRegister = 1; // b1
    atmosphereData.Descriptor.RegisterSpace = 0;  // space0
    
    rootDesc.parameters.push_back(frameDataParameter);
    rootDesc.parameters.push_back(atmosphereData);
    rootDesc.parameters.push_back(depthParam);

    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);

    GraphicsPipelineDescription desc;
    desc.name = L"Atmosphere Pass";
    desc.rootSignature = mRootSig;
    desc.inputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    desc.vertexShader = { "Shaders/AtmosphereVS.dxil" };
    desc.pixelShader = { "Shaders/AtmospherePS.dxil" };
    desc.renderTargetFormats = {DXGI_FORMAT_R16G16B16A16_FLOAT};
    desc.culling = FaceCulling::BACK;
    desc.depthTest = false;
    desc.depthWrite = false;
    desc.transparency = true;

    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);

    mAtmosphereDataBuffer = ResourceFactory::CreateConstantBuffer<AtmosphereData>();
    mAtmosphereDataBuffer->Upload(mAtmosphereData);
}

void AtmospherePass::Render(RenderTargetManager& manager, std::span<Model> models) {
    std::array<float, 4> mClearColour = { 
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };

    auto* colourTarget = manager.GetRenderTarget(L"Scene Color");
    auto* depthTarget = manager.GetDepthTarget(L"Default Depth");
    GraphicsCommands::BeginLabel(PIX_COLOR(0, 128, 255), "Atmosphere pass");
    GraphicsCommands::TransitionDSV(
        *depthTarget, depthTarget->GetState(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );


    std::array<RenderTarget*, 1> targets = {
        colourTarget
    };
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
    GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    GraphicsCommands::SetConstantBufferView(0, mFrameDataBuffer->GetAddress());
    GraphicsCommands::SetConstantBufferView(1, mAtmosphereDataBuffer->GetAddress());
    GraphicsCommands::BindSRV(2, depthTarget->GetSRV());
    for (auto& model : models){
        for (auto& primitive : model.primitives) {
            GraphicsCommands::BindVertexBuffer(*primitive.vertexBuffer.get());
            GraphicsCommands::BindIndexBuffer(*primitive.indexBuffer.get());
            GraphicsCommands::DrawIndexed(primitive.indexBuffer->GetIndexCount(), 1, 0, 0, 0);
        }
    }
    GraphicsCommands::EndLabel();
}

void AtmospherePass::OnImGuiRender() {
    ImGui::Begin("Atmosphere Data");
    ImGui::SliderFloat("Planet Radius", &mAtmosphereData.planetRadius, 0.1f, 100.f);
    ImGui::SliderFloat("Atmosphere Radius", &mAtmosphereData.atmosphereRadius, 0.1f, 100.f);
    ImGui::SliderFloat("Density Falloff", &mAtmosphereData.densityFalloff, 0.1f, 10.f);
    ImGui::SliderFloat("Strength Multiplier", &mAtmosphereData.strengthMultiplier, 0.1f, 10.f);
    ImGui::SliderInt("Ray traced steps", &mAtmosphereData.steps, 1, 10);
    ImGui::End();

    mAtmosphereDataBuffer->Upload(mAtmosphereData);
}