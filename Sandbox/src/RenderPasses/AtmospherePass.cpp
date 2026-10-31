#include "RenderPasses/AtmospherePass.h"

#include <pix3.h>

using namespace GEM;

AtmospherePass::AtmospherePass(ConstantBuffer<FrameData>* frameDataBuffer){
    mFrameDataBuffer = frameDataBuffer;


    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.name = L"Atmosphere Root Signature";

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(D3D12_SHADER_VISIBILITY_VERTEX & D3D12_SHADER_VISIBILITY_DOMAIN & D3D12_SHADER_VISIBILITY_HULL);

    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0
    
    rootDesc.parameters.push_back(frameDataParameter);

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
    desc.depthTest = true;
    desc.depthWrite = false;
    desc.transparency = true;

    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);
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
    GraphicsCommands::TransitionRTV(
        *colourTarget,
        colourTarget->GetState(),
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    GraphicsCommands::TransitionDSV(
        *depthTarget, depthTarget->GetState(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );


    std::array<RenderTarget*, 1> targets = {
        colourTarget
    };
    GraphicsCommands::BindRenderTargets(targets, depthTarget);
    //GraphicsCommands::ClearRenderTarget(*colourTarget, mClearColour.data());
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

}