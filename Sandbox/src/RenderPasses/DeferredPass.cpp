#include "RenderPasses/DefferedPass.h"

#include "Graphics/GraphicsCommands.h"
#include "pix3.h"
#include "imgui.h"

#include <numbers>

using namespace GEM;

DeferredPass::DeferredPass(ConstantBuffer<FrameData>* frameDataBuffer, TLAS* tlas) {
    static_assert(sizeof(PlanetData) == 36);
    mFrameDataBuffer = frameDataBuffer;
    tlasPtr = tlas;

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(D3D12_SHADER_VISIBILITY_VERTEX & D3D12_SHADER_VISIBILITY_DOMAIN & D3D12_SHADER_VISIBILITY_HULL);

    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    D3D12_ROOT_PARAMETER tessDataParameter{};
    tessDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    tessDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
    tessDataParameter.Descriptor.RegisterSpace = 0;
    tessDataParameter.Descriptor.ShaderRegister = 1;

    D3D12_DESCRIPTOR_RANGE planetRange{};
    planetRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    planetRange.NumDescriptors = 1;
    planetRange.BaseShaderRegister = 0;
    planetRange.RegisterSpace = 0;
    planetRange.OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER planetDataParameter;
	planetDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    planetDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    planetDataParameter.DescriptorTable.NumDescriptorRanges = 1;
	planetDataParameter.DescriptorTable.pDescriptorRanges = &planetRange;

    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.parameters.push_back(frameDataParameter);
	rootDesc.parameters.push_back(planetDataParameter);
	rootDesc.parameters.push_back(tessDataParameter);

    rootDesc.name = L"Planet Root Signature";
    
    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);
    
    mPlanetDataBuffer = ResourceFactory::CreateStructuredBuffer<PlanetData, 5>(mPlanetData);

	mTesselationDataBuffer = ResourceFactory::CreateConstantBuffer<TesselationData>();
	mTesselationDataBuffer->Upload(mTesselationData);

    GraphicsPipelineDescription desc;
    desc.name = L"Planet Pipeline";
    desc.rootSignature = mRootSig;
    desc.inputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    desc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
    desc.vertexShader = {"Shaders/TriangleVS.dxil"};
    desc.pixelShader = {"Shaders/TrianglePS.dxil"};
    desc.hullShader = {"Shaders/PlanetHS.dxil"};
    desc.domainShader = {"Shaders/PlanetDS.dxil"};
    desc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    desc.topology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
    desc.wireframe = false;
    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);

    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    param.Descriptor.ShaderRegister = 0; // Binds to register(u0)
    param.Descriptor.RegisterSpace = 0;  // Binds to space0
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    RootSignatureDesc computeDesc;
    computeDesc.name = L"Planet Compute Desc";
    computeDesc.parameters.push_back(param);
    computeDesc.parameters.push_back(planetDataParameter);
    computeDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    mComputeSig = ResourceFactory::CreateRootSignature(computeDesc);
    mComputePipeline = ResourceFactory::CreateComputePipeline(mComputeSig, ShaderBytecode("Shaders/PlanetCS.dxil"), L"Planet Compute");

    mClearColour = {
        0.f, 0.f, 0.f, 1.f
    };

    GenerateBaseMesh();
}

void DeferredPass::Render(RenderTargetManager& manager, std::span<Model> models) {

    auto* colourTarget = manager.GetRenderTarget(L"Colour GBuffer");
    auto* normalTarget = manager.GetRenderTarget(L"Normal GBuffer");
    auto* depthTarget = manager.GetDepthTarget(L"Default Depth");

    GraphicsCommands::BeginLabel(PIX_COLOR(255, 128, 0), "Planet pass");

    GraphicsCommands::BeginLabel(PIX_COLOR(255, 0, 0), "Planet Compute");
    GraphicsCommands::TransitionBuffer(*mPlanetMesh.primitives[0].vertexBuffer.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsCommands::BindComputePipeline(*mComputePipeline.get());
    GraphicsCommands::BindUAV(0, mPlanetMesh.primitives[0].vertexBuffer->GetResource()->GetGPUVirtualAddress());
    GraphicsCommands::BindComputeSRV(1, mPlanetDataBuffer->GetSRV());
    UINT threadXCount = (planetVertexCount + 256 - 1) / 256;
    GraphicsCommands::DispatchCompute(threadXCount, totalPlanetCount, 1);
    GraphicsCommands::TransitionBuffer(*mPlanetMesh.primitives[0].vertexBuffer.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    
    for (int i = 0; i < totalPlanetCount; i++) {
        GraphicsCommands::RebuildBLAS(*mBLASes[i].get());
        glm::mat4x4 translation =  glm::translate({1.f}, mPlanetData[i].position);
        tlasPtr->AddInstance(*mBLASes[i].get(), i, 0, 0x01, glm::mat4x3(translation));
    }

    GraphicsCommands::RebuildTLAS(*tlasPtr);
    GraphicsCommands::EndLabel();

    GraphicsCommands::TransitionRTV(
        *colourTarget,
        colourTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    GraphicsCommands::TransitionRTV(
        *normalTarget,
        normalTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    GraphicsCommands::TransitionDSV(
        *depthTarget, depthTarget->GetState(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );

    std::array<RenderTarget*, 2> sceneTargets = {
        colourTarget, normalTarget
    };

    GraphicsCommands::BindRenderTargets(sceneTargets, depthTarget);

    GraphicsCommands::ClearRenderTarget(*colourTarget, mClearColour.data());
    GraphicsCommands::ClearRenderTarget(*normalTarget, mClearColour.data());
    GraphicsCommands::ClearDepthStencil(*depthTarget);

    GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    GraphicsCommands::SetGraphicsConstantBufferView(0, mFrameDataBuffer->GetAddress());
	GraphicsCommands::BindSRV(1, mPlanetDataBuffer->GetSRV());
	GraphicsCommands::SetGraphicsConstantBufferView(2, mTesselationDataBuffer->GetAddress());
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
    GraphicsCommands::BindVertexBuffer(*mPlanetMesh.primitives[0].vertexBuffer.get());
    GraphicsCommands::BindIndexBuffer(*mPlanetMesh.primitives[0].indexBuffer.get());
    GraphicsCommands::DrawIndexed(planetIndexCount, totalPlanetCount, 0, 0, 0);

    GraphicsCommands::EndLabel();
}

void DeferredPass::OnImGuiRender() {
    ImGui::Begin("Planet Pass");
    ImGui::SliderInt("Planet Count", &totalPlanetCount, 1, 5);
    ImGui::SeparatorText("Tesselation Settings");
	ImGui::SliderFloat("Max Tesselation", &mTesselationData.maxTess, 1.f, 64.f);
	ImGui::SliderFloat("Near Distance", &mTesselationData.nearDist, 1.f, 100.f);
	ImGui::SliderFloat("Far Distance", &mTesselationData.farDist, 1.f, 100.f);

    ImGui::SeparatorText("Planet Generation Settings");
    for (int i = 0; i < totalPlanetCount; i++){
        ImGui::PushID(i);
        ImGui::Separator();
        auto& data = mPlanetData[i];
        ImGui::SliderFloat("Radius", &data.radius, 0.1f, 100.f);
        ImGui::SliderFloat3("Position", &data.position.x, -100.f, 100.f);
        ImGui::SliderFloat("Roughness", &data.roughness, 0.0f, 10.f);
        ImGui::SliderFloat("Riged Roughness", &data.rigedRoughness, 0.f, 10.f);
        ImGui::SliderFloat("Persistence", &data.persistence, 0.0f, 10.f);
        ImGui::SliderFloat("Terrain Strength", &data.terrainStrength, 0.0f, 10.f);
        ImGui::PopID();
    }

    GraphicsCommands::CopyDataToBuffer(mPlanetDataBuffer->GetResource(), mPlanetData.data(), mPlanetData.size() * sizeof(PlanetData));

	mTesselationDataBuffer->Upload(mTesselationData);
    ImGui::End();
}

void DeferredPass::GenerateBaseMesh() {
    constexpr float radius = 1.f;
    constexpr float inverseRad = 1.f / radius;
    constexpr int stackCount = 60;
    constexpr int sectorCount = 60;

    constexpr float sectorStep = (2 * std::numbers::pi_v<float>) / static_cast<float>(sectorCount);
    constexpr float stackStep = std::numbers::pi_v<float> / static_cast<float>(stackCount);

    std::vector<GEM::Vertex> vertices;
    vertices.reserve(stackCount * sectorCount * 5);
    std::vector<uint32_t> indices;

    int indexCount, vertexCount;

    for (int planetCount = 0; planetCount < sMaxPlanets; planetCount++) {
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
        if (planetCount == 0) {
            planetIndexCount = indices.size();
            planetVertexCount = vertices.size();
        }
    }

    std::reverse(indices.begin(), indices.end());

    mPlanetMesh.primitives.push_back({
        ResourceFactory::CreateVertexBuffer(vertices),
        ResourceFactory::CreateIndexBuffer(indices)
    });
    mBLASes.resize(sMaxPlanets);
    
    for (int i = 0; i < sMaxPlanets; i++) {
        mPlanetData[i].vertexCount = planetVertexCount;
        mBLASes[i] = ResourceFactory::CreateBLAS(mPlanetMesh.primitives[0], L"Planet BLAS", true, i * planetVertexCount, planetVertexCount, i * planetIndexCount, planetIndexCount);
    }
}