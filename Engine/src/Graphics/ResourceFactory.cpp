#include "Graphics/ResourceFactory.h"

#include "Graphics/GraphicsContext.h"


using namespace GEM;

void ResourceFactory::RegisterContext(GraphicsContext* context) {
    std::cout << "Graphics Context Registered to Resource Factory" << std::endl;
    sContextPtr = context;
}

std::unique_ptr<GraphicsPipeline> ResourceFactory::CreateGraphicsPipeline(const GraphicsPipelineDescription& desc) {
    GraphicsPipeline* pipeline  = new GraphicsPipeline();
    if (!sContextPtr) {
        std::cerr << "Graphics context not registered to resource factory" << std::endl;
        return nullptr;
    }

    if (!desc.rootSignature)
    {
        throw std::runtime_error("Graphics pipeline requires a root signature");
    }
    pipeline->mRootSignature = desc.rootSignature->Get();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state{};

    state.pRootSignature = pipeline->mRootSignature.Get();
    if (desc.vertexShader.has_value()) {
        state.VS = desc.vertexShader->GetBytecode();
    }
    if (desc.pixelShader.has_value()) {
        state.PS = desc.pixelShader->GetBytecode();
    }
    if (desc.domainShader.has_value()) {
        state.DS = desc.domainShader->GetBytecode();
    }
    if (desc.hullShader.has_value()) {
        state.HS = desc.hullShader->GetBytecode();
    }
    if (desc.geometryShader.has_value()) {
        state.GS = desc.geometryShader->GetBytecode();
    }


    state.InputLayout = {
        desc.inputElements.data(),
        static_cast<UINT>(desc.inputElements.size())
    };

    state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    if (desc.enableDepth) {
        state.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

        state.DepthStencilState.DepthEnable = TRUE;
        state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        state.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    }
    else {
        state.DepthStencilState.DepthEnable = FALSE;
        state.DepthStencilState.StencilEnable = FALSE;
    }

    state.SampleDesc.Count = 1;
    state.SampleDesc.Quality = 0;
    state.SampleMask = UINT_MAX;

    state.SampleMask = UINT_MAX;

    state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    state.NumRenderTargets = static_cast<UINT>(desc.renderTargetFormats.size());
    for (int i = 0; i < desc.renderTargetFormats.size(); i++) {
        state.RTVFormats[i] = desc.renderTargetFormats[i];
    }

    state.SampleDesc.Count = 1;
    THROW_IF_FAILED(sContextPtr->mDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&pipeline->mPipelineState)));
	THROW_IF_FAILED(pipeline->mPipelineState->SetName(desc.name.c_str()));

    return std::unique_ptr<GraphicsPipeline>(pipeline);
}

std::shared_ptr<GraphicsRootSignature> ResourceFactory::CreateGraphicsRootSignature(const GraphicsRootSignatureDesc& desc){
    if (!sContextPtr) {
        std::cerr << "Graphics context not registered to resource factory" << std::endl;
        return nullptr;
    }

    D3D12_ROOT_SIGNATURE_DESC sigDesc{};

    sigDesc.NumParameters = static_cast<UINT>(desc.parameters.size());
    sigDesc.pParameters = nullptr;
    if (!desc.parameters.empty()) {
        sigDesc.pParameters = desc.parameters.data();
    }

    sigDesc.NumStaticSamplers = static_cast<UINT>(desc.samplers.size());
    sigDesc.pStaticSamplers = nullptr;
    if (!desc.samplers.empty()) {
        sigDesc.pStaticSamplers = desc.samplers.data();
    }
    sigDesc.Flags = desc.flags;

    Microsoft::WRL::ComPtr<ID3DBlob> serialized;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;

    HRESULT result = D3D12SerializeRootSignature(
        &sigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serialized,
        &errors);

    if (FAILED(result)){
        if (errors){
            OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
        }

        THROW_IF_FAILED(result);
    }
    Microsoft::WRL::ComPtr<ID3D12RootSignature> signature;
    THROW_IF_FAILED(sContextPtr->mDevice->CreateRootSignature(
        0,
        serialized->GetBufferPointer(),
        serialized->GetBufferSize(),
        IID_PPV_ARGS(&signature)));

    THROW_IF_FAILED(signature->SetName(desc.name.c_str()));

    return std::shared_ptr<GraphicsRootSignature>(new GraphicsRootSignature(std::move(signature)));
}


GraphicsContext* ResourceFactory::sContextPtr = nullptr;