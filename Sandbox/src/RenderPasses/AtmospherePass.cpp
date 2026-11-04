#include "RenderPasses/AtmospherePass.h"
#include <numbers>

#include <pix3.h>
#include <imgui.h>

using namespace GEM;

AtmospherePass::AtmospherePass(ConstantBuffer<FrameData>* frameDataBuffer, TLAS* tlas){
    mFrameDataBuffer = frameDataBuffer;
    tlasPtr = tlas;

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

    GenerateBaseMesh();

    RootSignatureDesc rtRootDesc{};
    rtRootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    rtRootDesc.name = L"RT Root Sig";

    D3D12_ROOT_PARAMETER accelParam{};
    accelParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    accelParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    accelParam.Descriptor.ShaderRegister = 0;
    accelParam.Descriptor.RegisterSpace = 0;

    rtRootDesc.parameters.push_back(accelParam);

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

    rtRootDesc.parameters.push_back(outputRT);

    mRTRootSig = ResourceFactory::CreateRootSignature(rtRootDesc);

    D3D12_HIT_GROUP_DESC defGroup{};
    defGroup.HitGroupExport = L"DefaultHitGroup";
    defGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    defGroup.ClosestHitShaderImport = L"closestHit";

    RTPipelineDesc rtPipeDesc{};
    rtPipeDesc.hitGroups.push_back(defGroup);
    rtPipeDesc.libraryBytecodes.push_back({"Shaders/AtmosphereRT.dxil"});
    rtPipeDesc.globalRootSig = mRTRootSig;

    mRTPipeline = ResourceFactory::CreateRTPipeline(rtPipeDesc);

    ShaderTableDesc stDesc{};
    stDesc.pipeline = mRTPipeline.get();
    stDesc.rayGen = {L"rayGen"};
    stDesc.missShaders.push_back({L"miss"});
    mShaderTable =  ResourceFactory::CreateShaderTable(stDesc);
}

void AtmospherePass::Render(RenderTargetManager& manager, std::span<Model> models) {
    // std::array<float, 4> mClearColour = { 
    //     0.05f,
    //     0.10f,
    //     0.20f,
    //     1.0f
    // };

    auto* colourTarget = manager.GetRenderTarget(L"Scene Color");
    // auto* depthTarget = manager.GetDepthTarget(L"Default Depth");
    GraphicsCommands::BeginLabel(PIX_COLOR(0, 128, 255), "Atmosphere pass");

    glm::mat4x4 transform = glm::scale({1.f}, glm::vec3{mAtmosphereData.atmosphereRadius,mAtmosphereData.atmosphereRadius,mAtmosphereData.atmosphereRadius});
    tlasPtr->AddInstance(*mBlas.get(), 1, 0xFF, glm::mat4x3(transform));
    GraphicsCommands::RebuildTLAS(*tlasPtr);

    GraphicsCommands::TransitionRTV(
        *colourTarget, colourTarget->GetState(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    GraphicsCommands::BindRaytracingPipeline(*mRTPipeline);
    GraphicsCommands::BindComputeSRV(0, tlasPtr->GetSRV());
    GraphicsCommands::BindUAV(1, colourTarget->GetResource()->GetGPUVirtualAddress());
    GraphicsCommands::DispatchRays(*mShaderTable, colourTarget->GetWidth(), colourTarget->GetHeight());
    // GraphicsCommands::TransitionDSV(
    //     *depthTarget, depthTarget->GetState(),
    //     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    // );


    // std::array<RenderTarget*, 1> targets = {
    //     colourTarget
    // };
    // GraphicsCommands::BindRenderTargets(targets);
    // GraphicsCommands::BindViewport(
    //     0.f,
    //     0.f,
    //     static_cast<float>(manager.GetWidth()),
    //     static_cast<float>(manager.GetHeight()),
    //     0.f,
    //     1.f
    // );
    // GraphicsCommands::BindScissorRect(
    //     0,
    //     0,
    //     static_cast<int>(manager.GetWidth()),
    //     static_cast<int>(manager.GetHeight())
    // );
    // GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    // GraphicsCommands::SetGraphicsConstantBufferView(0, mFrameDataBuffer->GetAddress());
    // GraphicsCommands::SetGraphicsConstantBufferView(1, mAtmosphereDataBuffer->GetAddress());
    // GraphicsCommands::BindSRV(2, depthTarget->GetSRV());
    // GraphicsCommands::BindVertexBuffer(*mModel.primitives[0].vertexBuffer.get());
    // GraphicsCommands::BindIndexBuffer(*mModel.primitives[0].indexBuffer.get());
    // GraphicsCommands::DrawIndexed(mModel.primitives[0].indexBuffer->GetIndexCount(), 1, 0, 0, 0);
    GraphicsCommands::EndLabel();
}

void AtmospherePass::OnImGuiRender() {
    ImGui::Begin("Atmosphere Data");
    ImGui::ColorEdit3("Planet Atmosphere", &mAtmosphereData.atmosphereColour.r);
    ImGui::SliderFloat("Planet Radius", &mAtmosphereData.planetRadius, 0.1f, 100.f);
    ImGui::SliderFloat("Atmosphere Radius", &mAtmosphereData.atmosphereRadius, 0.1f, 100.f);
    ImGui::SliderFloat("Density Falloff", &mAtmosphereData.densityFalloff, 0.1f, 10.f);
    ImGui::SliderFloat("Strength Multiplier", &mAtmosphereData.strengthMultiplier, 0.1f, 10.f);
    ImGui::SliderInt("Ray traced steps", &mAtmosphereData.steps, 1, 10);
    ImGui::End();

    mAtmosphereDataBuffer->Upload(mAtmosphereData);
}

void AtmospherePass::GenerateBaseMesh() {
    constexpr float radius = 1.f;
    constexpr float inverseRad = 1.f / radius;
    constexpr int stackCount = 20;
    constexpr int sectorCount = 20;

    constexpr float sectorStep = (2 * std::numbers::pi_v<float>) / static_cast<float>(sectorCount);
    constexpr float stackStep = std::numbers::pi_v<float> / static_cast<float>(stackCount);

    std::vector<GEM::Vertex> vertices;
    vertices.reserve(stackCount * sectorCount);

    for (int i = 0; i <= stackCount; i++) {
        float stackAngle = (std::numbers::pi_v<float> / 2.f) - i * stackStep;
        float xy = radius * std::cosf(stackAngle);
        float z = radius * std::sinf(stackAngle);

        for (int j = 0; j <= sectorCount; j++) {
            GEM::Vertex vert;
            float sectorAngle = j * sectorStep;
            vert.position = {
                xy * std::cosf(sectorAngle),
                xy * std::sinf(sectorAngle),
                z
            };

            vert.normal = {
                vert.position.x * inverseRad,
                vert.position.y * inverseRad,
                vert.position.z * inverseRad,
            };

            vertices.push_back(vert);
        }
    }

    std::vector<uint32_t> indices;

    for (int i = 0; i < stackCount; i++) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; j++, k1++, k2++) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }

        }
    }
    std::reverse(indices.begin(), indices.end());

    mModel.primitives.push_back({
        ResourceFactory::CreateVertexBuffer(vertices),
        ResourceFactory::CreateIndexBuffer(indices)
    });

    mBlas = ResourceFactory::CreateBLAS(mModel.primitives[0], L"Atmosphere BLAS", false);

}