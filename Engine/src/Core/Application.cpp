#include "Core/Application.h"

#include "Graphics/GraphicsCommands.h"
#include "Graphics/ResourceFactory.h"

using namespace GEM;

Application::Application(HINSTANCE instance) {
    mWindow = std::make_unique<Window>(instance, L"Gemstone Engine");
    mContext = std::make_unique<GraphicsContext>(mWindow->GetHandle());
    GraphicsCommands::RegisterContext(mContext.get());
    ResourceFactory::RegisterContext(mContext.get());

    GraphicsRootSignatureDesc rootDesc;
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.name = L"Root Signature";
    mRootSig = ResourceFactory::CreateGraphicsRootSignature(rootDesc);

    GraphicsPipelineDescription desc;
    desc.name = L"Triangle Test Pipeline";
    desc.rootSignature = mRootSig;
    desc.renderTargetFormats = {DXGI_FORMAT_R8G8B8A8_UNORM};
    desc.vertexShader = {"Shaders/TriangleVS.dxil"};
    desc.pixelShader = {"Shaders/TrianglePS.dxil"};
    desc.enableDepth = false;
    mPipeline = ResourceFactory::CreateGraphicsPipeline(desc);
}

void Application::Run() {
    while (mWindow->PollEvents()){
        GraphicsCommands::BeginFrame();
        GraphicsCommands::BindGraphicsPipeline(*mPipeline.get());
        GraphicsCommands::BindViewport(0.f, 0.f, 1280.f, 720.f, 0.f, 1.f);
        GraphicsCommands::BindScissorRect(0, 0, 1280, 720);
        GraphicsCommands::Draw(3, 1, 0, 0);
        GraphicsCommands::EndFrame();
    }
}