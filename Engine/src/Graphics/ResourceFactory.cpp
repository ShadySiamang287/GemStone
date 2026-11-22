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
	
	state.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(desc.culling);
	state.RasterizerState.FrontCounterClockwise = false;
	if (desc.wireframe) {
		state.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	}


	auto& ds = state.DepthStencilState;
	ds.StencilEnable = FALSE;

	if (desc.depthTest || desc.depthWrite) {
		ds.DepthEnable = TRUE;
		ds.DepthWriteMask = desc.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL
											: D3D12_DEPTH_WRITE_MASK_ZERO;
		ds.DepthFunc = desc.depthTest ? D3D12_COMPARISON_FUNC_LESS_EQUAL
										: D3D12_COMPARISON_FUNC_ALWAYS;
		state.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	} else {
		ds.DepthEnable  = FALSE;
		state.DSVFormat = DXGI_FORMAT_D32_FLOAT; // see note below
	}

	state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	if (desc.transparency) {
		state.BlendState.AlphaToCoverageEnable = false;
		state.BlendState.IndependentBlendEnable = false;
		auto& rt = state.BlendState.RenderTarget[0];

		rt.BlendEnable = TRUE;
		rt.LogicOpEnable = FALSE;

		rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOp = D3D12_BLEND_OP_ADD;

		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		rt.LogicOp = D3D12_LOGIC_OP_NOOP;
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	}

	state.SampleDesc.Count = 1;
	state.SampleDesc.Quality = 0;
	state.SampleMask = UINT_MAX;

	state.SampleMask = UINT_MAX;

	state.PrimitiveTopologyType = desc.topologyType;

	state.NumRenderTargets = static_cast<UINT>(desc.renderTargetFormats.size());
	for (int i = 0; i < desc.renderTargetFormats.size(); i++) {
		state.RTVFormats[i] = desc.renderTargetFormats[i];
	}

	state.SampleDesc.Count = 1;
	THROW_IF_FAILED(sContextPtr->mDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&pipeline->mPipelineState)));
	THROW_IF_FAILED(pipeline->mPipelineState->SetName(desc.name.c_str()));
	pipeline->mTopology = desc.topology;
	return std::unique_ptr<GraphicsPipeline>(pipeline);
}

std::unique_ptr<ComputePipeline> ResourceFactory::CreateComputePipeline(std::shared_ptr<const RootSignature> rootSignature, ShaderBytecode shaderModule, std::wstring name) {
	CHECK_CONTEXT(nullptr);

	ComputePipeline* pipeline = new ComputePipeline();
	pipeline->mRootSignature = rootSignature->Get();

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
	desc.CS = shaderModule.GetBytecode();
	desc.pRootSignature = rootSignature->Get();
	
	THROW_IF_FAILED(sContextPtr->mDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline->mPipelineState)));
	THROW_IF_FAILED(pipeline->mPipelineState->SetName(name.c_str()));

	return std::unique_ptr<ComputePipeline>(pipeline);
}

std::unique_ptr<RaytracingPipeline> ResourceFactory::CreateRTPipeline(const RTPipelineDesc& desc){
	CHECK_CONTEXT(nullptr);

	RaytracingPipeline* pipeline = new RaytracingPipeline();

	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	// prevent reshuffling
	size_t totalSize = desc.libraryBytecodes.size() + desc.hitGroups.size() + (2 * desc.localRootSigs.size()) + 4;
	subobjects.reserve(totalSize);

	std::vector<D3D12_DXIL_LIBRARY_DESC> libraryDescs;
	libraryDescs.reserve(desc.libraryBytecodes.size());

	// Libraries
	for (auto& bytecode : desc.libraryBytecodes) {
		D3D12_DXIL_LIBRARY_DESC libraryDesc = {};
		libraryDesc.DXILLibrary = bytecode.GetBytecode();
		libraryDesc.NumExports = 0;
		libraryDesc.pExports = nullptr;

		libraryDescs.push_back(libraryDesc);
		subobjects.push_back(
			{D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &libraryDesc}
		);
	}

	// Hit Groups
	for (const auto& hitgroup : desc.hitGroups) {
		subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitgroup});
	}

	//Global Hit Groups
	D3D12_GLOBAL_ROOT_SIGNATURE globalRootDesc = {};
	globalRootDesc.pGlobalRootSignature = desc.globalRootSig->Get();
	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRootDesc});
	pipeline->mGlobalRootSig = desc.globalRootSig;

	//Local Root sigs
	std::vector<D3D12_LOCAL_ROOT_SIGNATURE> localRootDescs(desc.localRootSigs.size());
	std::vector<size_t> localRootIndices(desc.localRootSigs.size());

	for (int i = 0; i < desc.localRootSigs.size(); i++) {
		localRootDescs[i].pLocalRootSignature = desc.localRootSigs[i].rootSignature->Get();
		localRootIndices[i] = subobjects.size();

		subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, &localRootDescs[i]});
	}

	std::vector<std::vector<std::wstring>> persistentExports(desc.localRootSigs.size());
	std::vector<std::vector<LPCWSTR>> exportPointers(desc.localRootSigs.size());
	std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations(desc.localRootSigs.size());

	for (int i = 0; i < desc.localRootSigs.size(); i++) {
		persistentExports[i] = desc.localRootSigs[i].exports; 

		exportPointers[i].reserve(persistentExports[i].size());
		for (const auto& stringObj : persistentExports[i]) {
			exportPointers[i].push_back(stringObj.c_str());
		}

		associations[i].pSubobjectToAssociate = &subobjects[localRootIndices[i]];
		associations[i].NumExports = static_cast<UINT>(exportPointers[i].size());
		associations[i].pExports = exportPointers[i].data();

		subobjects.push_back({
			D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,
			&associations[i]
		});
	}

	// Raytracing shader config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxPayloadSizeInBytes = sizeof(HitPayload);
	shaderConfig.MaxAttributeSizeInBytes = 8; //uhh idk if this is right

	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig});

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociation{};
	shaderConfigAssociation.pSubobjectToAssociate = &subobjects[subobjects.size() - 1];
	shaderConfigAssociation.NumExports = 0;
	shaderConfigAssociation.pExports = nullptr;

	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &shaderConfigAssociation});

	// pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = desc.maxRecursionDepth;

	subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig});

	// Actually create the state object
	D3D12_STATE_OBJECT_DESC objDesc = {};
	objDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	objDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
	objDesc.pSubobjects = subobjects.data();

	THROW_IF_FAILED(sContextPtr->mDevice->CreateStateObject(&objDesc, IID_PPV_ARGS(&pipeline->mStateObject)));

	return std::unique_ptr<RaytracingPipeline>(pipeline);
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

	auto* rootSignature = new RootSignature(std::move(signature));
	rootSignature->mLocalArgsSize = ComputeLocalArgsSize(desc.parameters);

	return std::shared_ptr<RootSignature>(rootSignature);
}

std::unique_ptr<VertexBuffer> ResourceFactory::CreateVertexBuffer(std::span<Vertex> vertices) {
	CHECK_CONTEXT(nullptr);

	auto buffer = std::make_unique<VertexBuffer>();

	size_t bufferSize = vertices.size_bytes();
	D3D12_RESOURCE_DESC vertexDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	vertexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12MA::ALLOCATION_DESC defaultDesc{};
	defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&vertexDesc,
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
	GraphicsCommands::ExecuteImmediate([&](ID3D12GraphicsCommandList4* commandList){
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
	buffer->mVertexCount = vertices.size();

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
	GraphicsCommands::ExecuteImmediate([&](ID3D12GraphicsCommandList4* commandList){
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

std::unique_ptr<BLAS> ResourceFactory::CreateBLAS(const Primitive& primitive, std::wstring name, bool opaque, int startVertex, int vertexCount, int startIndex, int indexCount) {
	CHECK_CONTEXT(nullptr)

	BLAS* blas = new BLAS();

	blas->mPrimitives.push_back(&primitive);

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.VertexBuffer.StartAddress = primitive.vertexBuffer->GetResource()->GetGPUVirtualAddress() + startVertex * sizeof(Vertex);
	geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geomDesc.Triangles.VertexCount = vertexCount;

	geomDesc.Triangles.IndexBuffer = primitive.indexBuffer->GetResource()->GetGPUVirtualAddress() + startIndex * sizeof(UINT32);
	geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geomDesc.Triangles.IndexCount = indexCount;


	geomDesc.Triangles.Transform3x4 = 0;

	if (opaque) {
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	} else {
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	}
	blas->mGeometries.push_back(geomDesc);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	sContextPtr->mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	D3D12MA::ALLOCATION_DESC defaultDesc{};
	defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	UINT64 scratchSize = std::max(info.ScratchDataSizeInBytes, info.UpdateScratchDataSizeInBytes);
	blas->mScratchSize = scratchSize;

	D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(scratchSize);
	scratchDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes);
	resultDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&scratchDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			blas->mScratchAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);
	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&resultDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			blas->mAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = blas->mAllocation->GetResource()->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = blas->mScratchAllocation->GetResource()->GetGPUVirtualAddress();

	GraphicsCommands::ExecuteImmediate([&](ID3D12GraphicsCommandList4* commandList) {
        std::vector<D3D12_RESOURCE_BARRIER> preBarriers;
		D3D12_RESOURCE_BARRIER preVertex = CD3DX12_RESOURCE_BARRIER::Transition(
			primitive.vertexBuffer->GetResource(),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);


		preBarriers.push_back(preVertex);

		D3D12_RESOURCE_BARRIER preIndex = CD3DX12_RESOURCE_BARRIER::Transition(
			primitive.indexBuffer->GetResource(),
			D3D12_RESOURCE_STATE_INDEX_BUFFER,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);

		preBarriers.push_back(preIndex);
        commandList->ResourceBarrier(preBarriers.size(), preBarriers.data());
		commandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

        std::vector<D3D12_RESOURCE_BARRIER> postBarriers;
		D3D12_RESOURCE_BARRIER vertex = CD3DX12_RESOURCE_BARRIER::Transition(
			primitive.vertexBuffer->GetResource(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);

		postBarriers.push_back(vertex);

		D3D12_RESOURCE_BARRIER index = CD3DX12_RESOURCE_BARRIER::Transition(
			primitive.indexBuffer->GetResource(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_INDEX_BUFFER
		);

		postBarriers.push_back(index);

		D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(
            blas->GetResource()
        );
        postBarriers.push_back(uavBarrier);
		commandList->ResourceBarrier(postBarriers.size(), postBarriers.data());
	});
	blas->mAllocation->SetName(name.c_str());
	blas->mName = name.c_str();
	return std::unique_ptr<BLAS>(blas);
}

std::unique_ptr<ProceduralBLAS> ResourceFactory::CreateProceduralBLAS(const D3D12_RAYTRACING_AABB& aabb, std::wstring name, bool opaque){
	CHECK_CONTEXT(nullptr)

	ProceduralBLAS* blas = new ProceduralBLAS();

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
	geomDesc.AABBs.AABBCount = 0;
	geomDesc.AABBs.AABBs.StartAddress = blas->mAABBAllocation->GetResource()->GetGPUVirtualAddress();
	geomDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);

	if (opaque) {
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	} else {
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	}
	blas->mGeometries.push_back(geomDesc);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	sContextPtr->mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	D3D12MA::ALLOCATION_DESC defaultDesc{};
	defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	UINT64 scratchSize = std::max(info.ScratchDataSizeInBytes, info.UpdateScratchDataSizeInBytes);
	blas->mScratchSize = scratchSize;

	D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(scratchSize);
	scratchDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes);
	resultDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&scratchDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			blas->mScratchAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);
	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&resultDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			blas->mAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = blas->mAllocation->GetResource()->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = blas->mScratchAllocation->GetResource()->GetGPUVirtualAddress();

	GraphicsCommands::ExecuteImmediate([&](ID3D12GraphicsCommandList4* commandList) {
  //      std::vector<D3D12_RESOURCE_BARRIER> preBarriers;
		//D3D12_RESOURCE_BARRIER preVertex = CD3DX12_RESOURCE_BARRIER::Transition(
		//	primitive.vertexBuffer->GetResource(),
		//	D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		//	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		//);


		//preBarriers.push_back(preVertex);

		//D3D12_RESOURCE_BARRIER preIndex = CD3DX12_RESOURCE_BARRIER::Transition(
		//	primitive.indexBuffer->GetResource(),
		//	D3D12_RESOURCE_STATE_INDEX_BUFFER,
		//	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		//);

		//preBarriers.push_back(preIndex);
  //      commandList->ResourceBarrier(preBarriers.size(), preBarriers.data());
		commandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

        std::vector<D3D12_RESOURCE_BARRIER> postBarriers;
		//D3D12_RESOURCE_BARRIER vertex = CD3DX12_RESOURCE_BARRIER::Transition(
		//	primitive.vertexBuffer->GetResource(),
		//	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		//	D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		//);

		//postBarriers.push_back(vertex);

		//D3D12_RESOURCE_BARRIER index = CD3DX12_RESOURCE_BARRIER::Transition(
		//	primitive.indexBuffer->GetResource(),
		//	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		//	D3D12_RESOURCE_STATE_INDEX_BUFFER
		//);

		//postBarriers.push_back(index);

		D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(
            blas->GetResource()
        );
        postBarriers.push_back(uavBarrier);
		commandList->ResourceBarrier(postBarriers.size(), postBarriers.data());
	});
	blas->mAllocation->SetName(name.c_str());
	return std::unique_ptr<ProceduralBLAS>(blas);
}

std::unique_ptr<TLAS> ResourceFactory::CreateTLAS(UINT maxInstances) {
	TLAS* tlas = new TLAS();
	tlas->mCapacity = maxInstances;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs      = maxInstances;
	inputs.InstanceDescs = 0;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	sContextPtr->mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);


	D3D12MA::ALLOCATION_DESC defaultDesc{};
	defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	
	D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes);
	scratchDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes);
	resultDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	tlas->mScratchSize = info.ScratchDataSizeInBytes;

	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&scratchDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			tlas->mScratchAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);
	THROW_IF_FAILED(
		sContextPtr->mAllocator->CreateResource(
			&defaultDesc,
			&resultDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			tlas->mAllocation.GetAddressOf(),
			IID_NULL,
			nullptr
		)
	);

	D3D12MA::ALLOCATION_DESC uploadDesc{};
	uploadDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC instDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * std::max(maxInstances, 1u));
	THROW_IF_FAILED(sContextPtr->mAllocator->CreateResource(
		&uploadDesc, &instDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		tlas->mInstanceAllocation.GetAddressOf(), IID_NULL, nullptr));
	THROW_IF_FAILED(tlas->mInstanceAllocation->GetResource()->Map(0, nullptr, &tlas->mInstanceMapped));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.NumDescs = 0;
	asDesc.DestAccelerationStructureData = tlas->mAllocation->GetResource()->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = tlas->mScratchAllocation->GetResource()->GetGPUVirtualAddress();

	GraphicsCommands::ExecuteImmediate([&](ID3D12GraphicsCommandList4* commandList) {
		commandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = tlas->GetResource();
		commandList->ResourceBarrier(1, &uavBarrier);
	});

	UINT srvSlot = sContextPtr->AllocateSRVSlot();
	tlas->mSRVCPU = sContextPtr->GetSRVCPUHandle(srvSlot);
	tlas->mSRVGPU = sContextPtr->GetSRVGPUHandle(srvSlot);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = tlas->GetResource()->GetGPUVirtualAddress();

	sContextPtr->mDevice->CreateShaderResourceView(
		nullptr,
		&srvDesc,
		tlas->mSRVCPU
	);
	tlas->GetResource()->SetName(L"TLAS Buffer");

	return std::unique_ptr<TLAS>(tlas);
}

std::unique_ptr<ShaderTable> ResourceFactory::CreateShaderTable(const ShaderTableDesc& desc) {
	ShaderTable* table = new ShaderTable();

	auto Align = [](UINT v, UINT a) {return (v + a - 1) & ~(a - 1);};

	if (!desc.pipeline){
		throw std::runtime_error("ShaderTaleDesc: pipeline is null!");
	}

	const RaytracingPipeline& pipeline = *desc.pipeline;
    constexpr UINT idSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;


	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> props;
	pipeline.mStateObject.As(&props);

	auto identifier = [&](const std::wstring& name) -> const void* {
		return props->GetShaderIdentifier(name.c_str());   // nullptr if the export doesn't exist
	};

    auto validate = [&](const ShaderRecordDesc& r) {
        if (!identifier(r.exportName)) {
            throw std::runtime_error("Shader table: unknown export in pipeline");
		}

		UINT expected = ComputeLocalArgsSize(pipeline, r.exportName);
		if (r.localArgs.size() != expected) {
			throw std::runtime_error("Invalid local root signature");
		}
    };

    // 2. Stride = largest record in the table, aligned to 32
    auto strideOf = [&](const std::vector<ShaderRecordDesc>& recs) {
        UINT stride = 0;
        for (const auto& r : recs) {
            validate(r);
            stride = std::max(stride, Align(idSize + (UINT)r.localArgs.size(), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT));
        }
        return stride;
    };

	if (desc.rayGen.exportName.empty()){
        throw std::runtime_error("Shader table: no ray generation shader");
	}
    if (desc.missShaders.empty()) {
        throw std::runtime_error("Shader table: at least one miss shader required");
	}

	TableLayout rayGen{}, miss{}, hit{};
	validate(desc.rayGen);
	rayGen.offset = 0;
    rayGen.stride = rayGen.size = Align(idSize + (UINT)desc.rayGen.localArgs.size(), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	rayGen.count = 1;

	UINT cursor = rayGen.offset + rayGen.size;

    auto place = [&](TableLayout& t, const std::vector<ShaderRecordDesc>& recs) {
        if (recs.empty()) return;
        t.stride = Align(strideOf(recs), 64);
        t.count  = (UINT)recs.size();
        t.size   = t.stride * t.count;
        t.offset = Align(cursor, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        cursor   = t.offset + t.size;
    };

	place(miss, desc.missShaders);
    place(hit, desc.hitGroups);

	const UINT totalSize = Align(cursor, 64);

	D3D12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(totalSize);
    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	THROW_IF_FAILED(sContextPtr->mAllocator->CreateResource(
		&allocDesc, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 
		nullptr, &table->mAllocation, IID_NULL, nullptr
	))

	uint8_t* data = nullptr;
    D3D12_RANGE noRead = { 0, 0 };
	auto hr = table->mAllocation->GetResource()->Map(0, &noRead, reinterpret_cast<void**>(&data));
	if (FAILED(hr)) {
        table->mAllocation->Release();
        throw std::runtime_error("Shader table: map failed");
	}
	memset(data, 0, totalSize); 

	auto writeRecord = [&](uint8_t* dst, const ShaderRecordDesc& r) {
        memcpy(dst, props->GetShaderIdentifier(r.exportName.c_str()), idSize);
        if (!r.localArgs.empty())
            memcpy(dst + idSize, r.localArgs.data(), r.localArgs.size());
    };
    auto writeTable = [&](const TableLayout& t, const std::vector<ShaderRecordDesc>& recs) {
        for (UINT i = 0; i < t.count; ++i)
            writeRecord(data + t.offset + i * t.stride, recs[i]);
    };

    writeRecord(data + rayGen.offset, desc.rayGen);
    writeTable(miss, desc.missShaders);
    writeTable(hit, desc.hitGroups);

    table->mAllocation->GetResource()->Unmap(0, nullptr);

    // 6. Hand the result to the ShaderTable (private ctor, so no make_unique)
    table->mRayGen   = { rayGen.offset,   rayGen.size,   rayGen.stride,   rayGen.count };
    table->mMiss     = { miss.offset,     miss.size,     miss.stride,     miss.count };
    table->mHit      = { hit.offset,      hit.size,      hit.stride,      hit.count };
    return std::unique_ptr<ShaderTable>(table);
}

UINT ResourceFactory::ComputeLocalArgsSize(const std::vector<D3D12_ROOT_PARAMETER>& params)
{

	auto Align = [](UINT v, UINT a) {return (v + a) - 1 & ~(a - 1);};

    UINT offset = 0;
    for (const auto& p : params) {
        switch (p.ParameterType) {
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
            offset += p.Constants.Num32BitValues * 4;
            break;
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
        case D3D12_ROOT_PARAMETER_TYPE_SRV:
        case D3D12_ROOT_PARAMETER_TYPE_UAV:
            offset = Align(offset, 8) + 8;
            break;
        }
    }
    return offset;
}

UINT ResourceFactory::ComputeLocalArgsSize(const RaytracingPipeline& pipeline, const std::wstring& exportName) {
    for (const auto& assoc : pipeline.mLocalRootSigs) {
        if (std::find(assoc.exports.begin(), assoc.exports.end(), exportName) != assoc.exports.end())
            return assoc.rootSignature->GetLocalArgsSize();
    }
    return 0; // no local root signature
}

GraphicsContext* ResourceFactory::sContextPtr = nullptr;