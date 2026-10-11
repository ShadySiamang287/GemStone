#include "Renderer/Renderer.h"

#include "Renderer/RenderTargetManager.h"
#include "Graphics/GraphicsContext.h"

#include "Graphics/ResourceFactory.h"
#include "Graphics/GraphicsCommands.h"

using namespace GEM;

Renderer::Renderer(GraphicsContext& context) {
    mRenderTargetsPtr = std::make_unique<RenderTargetManager>(context);

    RootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.name = L"Root Signature";
    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);

    GraphicsPipelineDescription desc;
    desc.name = L"Triangle Test Pipeline";
    desc.rootSignature = mRootSig;
    desc.inputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    desc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM};
    desc.vertexShader = {"Shaders/TriangleVS.dxil"};
    desc.pixelShader = {"Shaders/TrianglePS.dxil"};
    desc.enableDepth = false;
    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);

    std::array<Vertex, 4> vertices = {
        Vertex{{-0.5f, 0.5f, 0.f}, {1.f, 0.f, 0.f, 1.f}},
        Vertex{{0.5f, 0.5f, 0.f}, {0.f, 0.f, 1.f, 1.f}},
        Vertex{{-0.5f, -0.5f, 0.f}, {0.f, 1.f, 0.f, 1.f}},
        Vertex{{0.5f, -0.5f, 0.f}, {0.f, 1.f, 1.f, 1.f}},
    };
    mVertexBuffer = ResourceFactory::CreateVertexBuffer(vertices);
    
    std::array<uint32_t, 6> indices = {
        0, 1, 2,
        1, 3, 2
    };
    mIndexBuffer = ResourceFactory::CreateIndexBuffer(indices);

}

void Renderer::Render() {
    GraphicsCommands::BeginFrame();
    GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
    GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
    GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
    GraphicsCommands::BindVertexBuffer(*mVertexBuffer.get());
    GraphicsCommands::BindIndexBuffer(*mIndexBuffer.get());
    GraphicsCommands::DrawIndexed(6, 1, 0, 0, 0);
    GraphicsCommands::EndFrame();
}