#include "RenderPasses/AtmospherePass.h"
#include <numbers>

#include <pix3.h>
#include <imgui.h>

using namespace GEM;

AtmospherePass::AtmospherePass(ConstantBuffer<FrameData>* frameDataBuffer, TLAS* tlas){
    mFrameDataBuffer = frameDataBuffer;
    tlasPtr = tlas;
    
    GenerateBaseMesh();

    mAtmosphereDataBuffer = ResourceFactory::CreateStructuredBuffer<AtmosphereData, 5>(mAtmosphereData);

    RootSignatureDesc rtRootDesc{};
    rtRootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    rtRootDesc.name = L"RT Root Sig";

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    D3D12_DESCRIPTOR_RANGE atmosphereRange{};
    atmosphereRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    atmosphereRange.NumDescriptors = 1;
    atmosphereRange.BaseShaderRegister = 1;
    atmosphereRange.RegisterSpace = 0;
    atmosphereRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER atmosphereData {};
    atmosphereData.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    atmosphereData.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    atmosphereData.DescriptorTable.NumDescriptorRanges = 1; // b1
    atmosphereData.DescriptorTable.pDescriptorRanges = &atmosphereRange;

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
    
    rtRootDesc.parameters.push_back(accelParam);
    rtRootDesc.parameters.push_back(outputRT);
    rtRootDesc.parameters.push_back(frameDataParameter);
    rtRootDesc.parameters.push_back(atmosphereData);

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
    stDesc.hitGroups.push_back({L"DefaultHitGroup"});
    mShaderTable =  ResourceFactory::CreateShaderTable(stDesc);
}

void AtmospherePass::Render(RenderTargetManager& manager, std::span<Model> models) {
    std::array<float, 4> mClearColour = { 
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };

    auto* colourTarget = manager.GetRenderTarget(L"Scene Color");
    GraphicsCommands::BeginLabel(PIX_COLOR(0, 128, 255), "Atmosphere pass");

    for (int i = 0; i < atmosphereCount; i++) {
        auto& data = mAtmosphereData[i];
        glm::mat4x4 translation =  glm::translate({1.f}, data.position);
        glm::mat4x4 transform = glm::scale(translation, glm::vec3{data.atmosphereRadius,data.atmosphereRadius,data.atmosphereRadius});
        tlasPtr->AddInstance(*mBlas.get(), 1 + i, 0x02, glm::mat4x3(transform));
    }
    GraphicsCommands::RebuildTLAS(*tlasPtr);

    GraphicsCommands::TransitionRTV(
        *colourTarget, colourTarget->GetState(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    GraphicsCommands::BindRaytracingPipeline(*mRTPipeline);
    GraphicsCommands::BindComputeSRV(0, tlasPtr->GetSRV());
    GraphicsCommands::BindUAV(1, colourTarget->GetUAV());
    GraphicsCommands::SetComputeConstantBufferView(2, mFrameDataBuffer->GetAddress());
    GraphicsCommands::BindComputeSRV(3, mAtmosphereDataBuffer->GetSRV());
    GraphicsCommands::DispatchRays(*mShaderTable, colourTarget->GetWidth(), colourTarget->GetHeight());

    GraphicsCommands::EndLabel();
}

void AtmospherePass::OnImGuiRender() {
    ImGui::Begin("Atmosphere Data");
    ImGui::SliderInt("Atmosphere Count", &atmosphereCount, 1, 5);
    for (int i = 0; i < atmosphereCount; i++){
        auto& data = mAtmosphereData[i];
        ImGui::PushID(i);
        ImGui::Separator();
        ImGui::ColorEdit3("Planet Atmosphere", &data.atmosphereColour.r);
        ImGui::SliderFloat3("Atmosphere position", &data.position.r, 0.f, 100.f);
        ImGui::SliderFloat("Planet Radius", &data.planetRadius, -100.f, 100.f);
        ImGui::SliderFloat("Atmosphere Radius", &data.atmosphereRadius, 0.1f, 100.f);
        ImGui::SliderFloat("Density Falloff", &data.densityFalloff, 0.1f, 10.f);
        ImGui::SliderFloat("Strength Multiplier", &data.strengthMultiplier, 0.1f, 10.f);
        ImGui::SliderInt("Ray traced steps", &data.steps, 1, 10);
        ImGui::PopID();
    }
    ImGui::End();
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

    mBlas = ResourceFactory::CreateBLAS(mModel.primitives[0], L"Atmosphere BLAS", true, 0, vertices.size(), 0, indices.size());

}