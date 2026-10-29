#include "RenderPasses/DefferedPass.h"

#include "Graphics/GraphicsCommands.h"
#include "pix3.h"
#include "imgui.h"

using namespace GEM;

DeferredPass::DeferredPass(ConstantBuffer<FrameData>* frameDataBuffer) {
    mFrameDataBuffer = frameDataBuffer;

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(D3D12_SHADER_VISIBILITY_VERTEX & D3D12_SHADER_VISIBILITY_DOMAIN & D3D12_SHADER_VISIBILITY_HULL);

    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    D3D12_ROOT_PARAMETER planetDataParameter{};
	planetDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    planetDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
    planetDataParameter.Descriptor.RegisterSpace = 0;
	planetDataParameter.Descriptor.ShaderRegister = 1;

    D3D12_ROOT_PARAMETER tessDataParameter{};
    tessDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    tessDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
    tessDataParameter.Descriptor.RegisterSpace = 0;
    tessDataParameter.Descriptor.ShaderRegister = 2;

    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.parameters.push_back(frameDataParameter);
	rootDesc.parameters.push_back(planetDataParameter);
	rootDesc.parameters.push_back(tessDataParameter);

    rootDesc.name = L"Planet Root Signature";
    
    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);
    
    mPlanetDataBuffer = ResourceFactory::CreateConstantBuffer<PlanetData>();
    mPlanetDataBuffer->Upload(mPlanetData);

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
    desc.enableDepth = true;
    desc.wireframe = false;
    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);

    mClearColour = {
        0.f, 0.f, 0.f, 1.f
    };
}

void DeferredPass::Render(RenderTargetManager& manager, std::span<Model> models) {

    auto* colourTarget = manager.GetRenderTarget(L"Colour GBuffer");
    auto* normalTarget = manager.GetRenderTarget(L"Normal GBuffer");
    auto* depthTarget = manager.GetDepthTarget(L"Default Depth");

    GraphicsCommands::BeginLabel(PIX_COLOR(255, 128, 0), "Planet pass");

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
    GraphicsCommands::SetConstantBufferView(0, mFrameDataBuffer->GetAddress());
	GraphicsCommands::SetConstantBufferView(1, mPlanetDataBuffer->GetAddress());
	GraphicsCommands::SetConstantBufferView(2, mTesselationDataBuffer->GetAddress());
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
    for (auto& model : models){
        for (auto& primitive : model.primitives) {
            GraphicsCommands::BindVertexBuffer(*primitive.vertexBuffer.get());
            GraphicsCommands::BindIndexBuffer(*primitive.indexBuffer.get());
            GraphicsCommands::DrawIndexed(primitive.indexBuffer->GetIndexCount(), 1, 0, 0, 0);
        }
    }

    GraphicsCommands::EndLabel();
}

void DeferredPass::OnImGuiRender() {
    ImGui::Begin("Planet Pass");
    ImGui::SeparatorText("Planet Generation Settings");
    ImGui::SliderFloat("Radius", &mPlanetData.radius, 0.1f, 100.f);
    ImGui::SliderFloat("Roughness", &mPlanetData.roughness, 0.0f, 10.f);
    ImGui::SliderFloat("Riged Roughness", &mPlanetData.rigedRoughness, 0.f, 10.f);
    ImGui::SliderFloat("Persistence", &mPlanetData.persistence, 0.0f, 10.f);
    ImGui::SliderFloat("Terrain Strength", &mPlanetData.terrainStrength, 0.0f, 10.f);
    mPlanetDataBuffer->Upload(mPlanetData);

    ImGui::SeparatorText("Tesselation Settings");
	ImGui::SliderFloat("Max Tesselation", &mTesselationData.maxTess, 1.f, 64.f);
	ImGui::SliderFloat("Near Distance", &mTesselationData.nearDist, 1.f, 100.f);
	ImGui::SliderFloat("Far Distance", &mTesselationData.farDist, 1.f, 100.f);

	mTesselationDataBuffer->Upload(mTesselationData);
    ImGui::End();
}