#include "Renderer/RenderPasses/DefferedPass.h"

#include "Graphics/GraphicsCommands.h"
#include "pix3.h"

using namespace GEM;

DeferredPass::DeferredPass(ConstantBuffer<FrameData>* frameDataBuffer) {
    mFrameDataBuffer = frameDataBuffer;

    D3D12_ROOT_PARAMETER frameDataParameter {};
    frameDataParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    frameDataParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    frameDataParameter.Descriptor.ShaderRegister = 0; // b0
    frameDataParameter.Descriptor.RegisterSpace = 0;  // space0

    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.parameters.push_back(frameDataParameter);
    rootDesc.name = L"Deferred Root Signature";
    
    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);

    GraphicsPipelineDescription desc;
    desc.name = L"Deferred Pipeline";
    desc.rootSignature = mRootSig;
    desc.inputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    desc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
    desc.vertexShader = {"Shaders/TriangleVS.dxil"};
    desc.pixelShader = {"Shaders/TrianglePS.dxil"};
    desc.enableDepth = true;
    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);

    mClearColour = {
        0.f, 0.f, 0.f, 1.f
    };
}

void DeferredPass::Render(RenderTargetManager& manager, std::span<Model> models) {

    auto* colourTarget = manager.GetRenderTarget(L"Colour GBuffer");
    auto* normalTarget = manager.GetRenderTarget(L"Normal GBuffer");
    auto* depthTarget = manager.GetDepthTarget(L"Default Depth");

    GraphicsCommands::BeginLabel(PIX_COLOR(255, 128, 0), "forward pass");

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
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    for (auto& model : models){
        for (auto& primitive : model.primitives) {
            GraphicsCommands::BindVertexBuffer(*primitive.vertexBuffer.get());
            GraphicsCommands::BindIndexBuffer(*primitive.indexBuffer.get());
            GraphicsCommands::DrawIndexed(primitive.indexBuffer->GetIndexCount(), 1, 0, 0, 0);
        }
    }

    GraphicsCommands::EndLabel();
}