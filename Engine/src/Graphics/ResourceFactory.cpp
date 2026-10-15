#include "Graphics/ResourceFactory.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsCommands.h"

using namespace GEM;


void ResourceFactory::RegisterContext(GraphicsContext* context) {
	std::cout << "Graphics Context Registered to Resource Factory" << std::endl;
	sContextPtr = context;
}

std::unique_ptr<GraphicsPipeline> ResourceFactory::CreateGraphicsPipeline(const GraphicsPipelineDescription& desc) {
	CHECK_CONTEXT(nullptr);

	GraphicsPipeline* pipeline  = new GraphicsPipeline();

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
	state.RasterizerState.FrontCounterClockwise = false;

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

std::shared_ptr<RootSignature> ResourceFactory::CreateRootSignature(const RootSignatureDesc& desc){
	CHECK_CONTEXT(nullptr);

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

	return std::shared_ptr<RootSignature>(new RootSignature(std::move(signature)));
}

std::unique_ptr<VertexBuffer> ResourceFactory::CreateVertexBuffer(std::span<Vertex> vertices) {
	CHECK_CONTEXT(nullptr);

	auto buffer = std::make_unique<VertexBuffer>();

	size_t bufferSize = vertices.size_bytes();
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12MA::ALLOCATION_DESC defaultDesc{};
	defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			buffer->mAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);

	Microsoft::WRL::ComPtr<D3D12MA::Allocation> uploadAllocation;
    D3D12MA::ALLOCATION_DESC uploadDesc{};
    uploadDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    THROW_IF_FAILED(
        sContextPtr->mAllocator->CreateResource(
            &uploadDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            uploadAllocation.GetAddressOf(),
            IID_NULL,
            nullptr
        )
    );


    ID3D12Resource* destination = buffer->GetResource();
    ID3D12Resource* upload = uploadAllocation->GetResource();

	//Copy the data to the transfer buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange{ 0, 0 };

    THROW_IF_FAILED(upload->Map(0,&readRange, &mappedData));

    std::memcpy(mappedData, vertices.data(),static_cast<size_t>(bufferSize));

    upload->Unmap(0, nullptr);

	// copy the transfer buffer to the actual buffer
	GraphicsCommands::ExecuteImmediate([&](ID3D12GraphicsCommandList* commandList){
        auto toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
			destination,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		);

		commandList->ResourceBarrier(1, &toCopyDest);
		
		commandList->CopyBufferRegion(
            destination,
            0,
            upload,
            0,
            bufferSize
        );

        auto barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(
                destination,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
            );

        commandList->ResourceBarrier(1, &barrier);
	});
	uploadAllocation.Reset();

	buffer->mView.BufferLocation = destination->GetGPUVirtualAddress();
	buffer->mView.SizeInBytes = bufferSize;
	buffer->mView.StrideInBytes = sizeof(Vertex);

	buffer->mAllocation->SetName(L"Vertex Buffer");

	return buffer;
}

std::unique_ptr<IndexBuffer> ResourceFactory::CreateIndexBuffer(std::span<uint32_t> indicies) {
	CHECK_CONTEXT(nullptr)

	auto buffer = std::make_unique<IndexBuffer>();

	size_t bufferSize = indicies.size_bytes();
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12MA::ALLOCATION_DESC defaultDesc{};
	defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			buffer->mAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);

	Microsoft::WRL::ComPtr<D3D12MA::Allocation> uploadAllocation;
    D3D12MA::ALLOCATION_DESC uploadDesc{};
    uploadDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    THROW_IF_FAILED(
        sContextPtr->mAllocator->CreateResource(
            &uploadDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            uploadAllocation.GetAddressOf(),
            IID_NULL,
            nullptr
        )
    );

    ID3D12Resource* destination = buffer->GetResource();
    ID3D12Resource* upload = uploadAllocation->GetResource();

	//Copy the data to the transfer buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange{ 0, 0 };

    THROW_IF_FAILED(upload->Map(0,&readRange, &mappedData));

    std::memcpy(mappedData, indicies.data(),static_cast<size_t>(bufferSize));

    upload->Unmap(0, nullptr);

	// copy the transfer buffer to the actual buffer
	GraphicsCommands::ExecuteImmediate([&](ID3D12GraphicsCommandList* commandList){
        auto toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
			destination,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		);

		commandList->ResourceBarrier(1, &toCopyDest);
		
		commandList->CopyBufferRegion(
            destination,
            0,
            upload,
            0,
            bufferSize
        );

        auto barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(
                destination,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_INDEX_BUFFER
            );

        commandList->ResourceBarrier(1, &barrier);
	});
	uploadAllocation.Reset();

	buffer->mView.BufferLocation = destination->GetGPUVirtualAddress();
	buffer->mView.SizeInBytes = bufferSize;
	buffer->mView.Format = DXGI_FORMAT_R32_UINT;

	buffer->mAllocation->SetName(L"Index Buffer");
	buffer->mIndexCount = indicies.size();

	return buffer;
}


GraphicsContext* ResourceFactory::sContextPtr = nullptr;