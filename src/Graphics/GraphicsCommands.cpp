#include "Graphics/GraphicsCommands.h"

#include "Graphics/GraphicsContext.h"

#include "Core/GemStoneAPI.h"

#include <pix3.h>

#define CHECK_CONTEXT     \
    if (!sContextPtr) {  \
        std::cout << "Graphics Context not Registered to command list" << std::endl;\
        return;\
    }\


using namespace GEM;

void GraphicsCommands::RegisterContext(GraphicsContext* context) {
    std::cout << "Graphics Context Registered" << std::endl;
    sContextPtr = context;
}

void GraphicsCommands::ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList4*)>& record){
    CHECK_CONTEXT

    auto* context = sContextPtr;

    THROW_IF_FAILED(context->mCommandAllocator->Reset());

    THROW_IF_FAILED(context->mCommandList->Reset(context->mCommandAllocator.Get(), nullptr));

    record(context->mCommandList.Get());

    THROW_IF_FAILED(context->mCommandList->Close());

    ID3D12CommandList* lists[] =
    {
        context->mCommandList.Get()
    };

    context->mCommandQueue->ExecuteCommandLists(1, lists);

    context->WaitForGpu();
}


void GraphicsCommands::BeginFrame() {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();

    THROW_IF_FAILED(sContextPtr->mCommandAllocator->Reset());
    
    THROW_IF_FAILED(commandList->Reset(sContextPtr->mCommandAllocator.Get(), nullptr));

	ID3D12DescriptorHeap* descriptorHeaps[] = {
		sContextPtr->mSRVHeap.Get()
	};
	commandList->SetDescriptorHeaps(1, descriptorHeaps);
    sContextPtr->mUploadOffset = 0;
}

ID3D12GraphicsCommandList4* GraphicsCommands::GetCommandList() {
	if (!sContextPtr) {
		return nullptr;
	}

	return sContextPtr->mCommandList.Get();
}

void GraphicsCommands::EndFrame() {
    CHECK_CONTEXT

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = sContextPtr->mRenderTargets[sContextPtr->mFrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    
    sContextPtr->mCommandList->ResourceBarrier(1, &barrier);
    
    THROW_IF_FAILED(sContextPtr->mCommandList->Close());
    
    ID3D12CommandList* commandLists[] =
    {
        sContextPtr->mCommandList.Get()
    };
    
    sContextPtr->mCommandQueue->ExecuteCommandLists(1, commandLists);
    
    auto result = sContextPtr->mSwapChain->Present(0, 0);
    if (FAILED(result)){
        if (result == DXGI_ERROR_DEVICE_REMOVED ||
            result == DXGI_ERROR_DEVICE_RESET ||
            result == DXGI_ERROR_DEVICE_HUNG)
        {
            sContextPtr->DeviceRemoved();
        }
        THROW_IF_FAILED(result);
    }
    
    sContextPtr->mFrameIndex = sContextPtr->mSwapChain->GetCurrentBackBufferIndex();
    
    sContextPtr->WaitForGpu();
}

void GraphicsCommands::Draw(const int& vertexCount, const int& instanceCount, const int& firstVertex, const int& firstInstance) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void GraphicsCommands::DrawIndexed(const int& indexCount, const int& instanceCount, const int& firstIndex, const int& firstVertex, const int& firstInstance) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
}

void GraphicsCommands::DispatchCompute(const UINT& countX, const UINT& countY, const UINT& countZ) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->Dispatch(countX, countY, countZ);
}

void GraphicsCommands::DispatchRays(ShaderTable& shaderTable, UINT width, UINT height) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    D3D12_DISPATCH_RAYS_DESC desc{};
    shaderTable.FillDispatchDesc(desc);
    desc.Width = width;
    desc.Height = height;
    desc.Depth = 1;

    commandList->DispatchRays(&desc);
}

void GraphicsCommands::BindViewport(const float& x1, const float& y1, const float& x2, const float& y2, const float& minDepth, const float& maxDepth) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    D3D12_VIEWPORT viewport = {
        x1, y1,
        x2, y2,
        minDepth, maxDepth
    };
    commandList->RSSetViewports(1, &viewport);
}

void GraphicsCommands::BindScissorRect(const int& x1, const int& y1, const int& x2, const int& y2) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    D3D12_RECT scissor = {
        x1, y1,
        x2, y2
    };
    commandList->RSSetScissorRects(1, &scissor);
}

void GraphicsCommands::BindVertexBuffer(const VertexBuffer& buffer){
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->IASetVertexBuffers(0, 1, &buffer.GetView());
}

void GraphicsCommands::BindIndexBuffer(const IndexBuffer& buffer) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->IASetIndexBuffer(&buffer.GetView());
}

void GraphicsCommands::BindGraphicsPipeline(GraphicsPipeline& pipeline) {
    CHECK_CONTEXT
    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->SetPipelineState(pipeline.mPipelineState.Get());
    commandList->SetGraphicsRootSignature(pipeline.mRootSignature.Get());
    commandList->IASetPrimitiveTopology(pipeline.mTopology);
}

void GraphicsCommands::BindComputePipeline(ComputePipeline& pipeline){
    CHECK_CONTEXT
    auto* commandList = sContextPtr->mCommandList.Get();

    commandList->SetPipelineState(pipeline.mPipelineState.Get());
    commandList->SetComputeRootSignature(pipeline.mRootSignature.Get());
}

void GraphicsCommands::BindRaytracingPipeline(RaytracingPipeline& pipeline) {
    CHECK_CONTEXT
    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->SetPipelineState1(pipeline.mStateObject.Get());
    commandList->SetComputeRootSignature(pipeline.mGlobalRootSig->Get());
}

void GraphicsCommands::SetGraphicsConstantBufferView(const UINT& index, D3D12_GPU_VIRTUAL_ADDRESS address) {
    CHECK_CONTEXT
    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->SetGraphicsRootConstantBufferView(index, address);
}

void GraphicsCommands::SetComputeConstantBufferView(const UINT& index, D3D12_GPU_VIRTUAL_ADDRESS address) {
    CHECK_CONTEXT
    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->SetComputeRootConstantBufferView(index, address);
}

void GraphicsCommands::BindSRV(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptor) {
    CHECK_CONTEXT

    sContextPtr->mCommandList->SetGraphicsRootDescriptorTable(
        rootParameterIndex,
        descriptor
    );
}

void GraphicsCommands::BindComputeSRV(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptor) {
    CHECK_CONTEXT

    sContextPtr->mCommandList->SetComputeRootDescriptorTable(
        rootParameterIndex,
        descriptor
    );
}

void GraphicsCommands::BindUAV(UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address){
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->SetComputeRootUnorderedAccessView(rootParameterIndex, address);
}

void GraphicsCommands::BindUAV(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptor) {
    CHECK_CONTEXT

    sContextPtr->mCommandList->SetComputeRootDescriptorTable(
        rootParameterIndex,
        descriptor
    );
}

void GraphicsCommands::TransitionRTV(RenderTarget& renderTarget, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    CHECK_CONTEXT

    if (before == after) {
        return;
    }

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget.GetResource(),
        before,
        after
    );

    sContextPtr->mCommandList->ResourceBarrier(1, &barrier);
    renderTarget.mState = after;
}

void GraphicsCommands::TransitionDSV(DepthTarget& depthTarget, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    CHECK_CONTEXT

    if (before == after) {
        return;
    }

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        depthTarget.GetResource(),
        before,
        after
    );

    sContextPtr->mCommandList->ResourceBarrier(1, &barrier);
    depthTarget.mState = after;
}

void GraphicsCommands::TransitionBuffer(GPUBuffer& bufferTarget, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    CHECK_CONTEXT

    if (before == after) {
        return;
    }

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferTarget.GetResource(),
        before,
        after
    );
    sContextPtr->mCommandList->ResourceBarrier(1, &barrier);
}

void GraphicsCommands::ClearRenderTarget(RenderTarget& renderTarget, const float clearColour[4]) {
    CHECK_CONTEXT

    sContextPtr->mCommandList->ClearRenderTargetView(
        renderTarget.GetRTV(),
        clearColour,
        0,
        nullptr
    );
}

void GraphicsCommands::ClearSwapchainImage(const float clearColour[4]) {
    CHECK_CONTEXT

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sContextPtr->mRTVHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += sContextPtr->mFrameIndex * sContextPtr->mRTVDescriptorSize;

    sContextPtr->mCommandList->ClearRenderTargetView(
        rtvHandle,
        clearColour,
        0,
        nullptr
    );
}

void GraphicsCommands::ClearDepthStencil(DepthTarget& depthTarget, D3D12_CLEAR_FLAGS clearFlags, float depth, UINT8 stencil) {
    CHECK_CONTEXT

    sContextPtr->mCommandList->ClearDepthStencilView(
        depthTarget.GetDSV(),
        clearFlags,
        depth,
        stencil,
        0,
        nullptr
    );
}


void GraphicsCommands::BindRenderTargets(std::span<RenderTarget*> renderTargets, DepthTarget* depthTarget) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles(renderTargets.size());

    if (renderTargets.empty()) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = sContextPtr->mRenderTargets[sContextPtr->mFrameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sContextPtr->mRTVHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += sContextPtr->mFrameIndex * sContextPtr->mRTVDescriptorSize;
        rtvHandles.push_back(rtvHandle);
    }
    else {
        std::transform(renderTargets.begin(), renderTargets.end(), rtvHandles.begin(), [](RenderTarget* target) {
            return target->GetRTV();
        });
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
    if (depthTarget) {
        dsvHandle = depthTarget->GetDSV();
    }

    commandList->OMSetRenderTargets(
        static_cast<UINT>(rtvHandles.size()),
        rtvHandles.data(),
        FALSE,
        depthTarget ? &dsvHandle : nullptr
    );

}

void GraphicsCommands::BeginLabel(uint32_t colour, std::string string) {
    CHECK_CONTEXT   
    auto* commandList = sContextPtr->mCommandList.Get();
    PIXBeginEvent(commandList, colour, string.c_str());
}

void GraphicsCommands::EndLabel() {
    CHECK_CONTEXT
    auto* commandList = sContextPtr->mCommandList.Get();
    PIXEndEvent(commandList);
}

void GraphicsCommands::CopyDataToBuffer(ID3D12Resource* buffer, void* data, size_t size) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    ID3D12Resource* upload = sContextPtr->mUploadBuffer->GetResource();

    size_t offset = (sContextPtr->mUploadOffset + 255) & ~255;
    assert(offset + size <= 1024 * 512);
    sContextPtr->mUploadOffset = offset + size;

    uint8_t* mappedData = nullptr;
    D3D12_RANGE readRange{ 0, 0 };

    THROW_IF_FAILED(upload->Map(0,&readRange, reinterpret_cast<void**>(&mappedData)));
    std::memcpy(mappedData + offset, data, size);

    upload->Unmap(0, nullptr);

    auto toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
        buffer,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    commandList->ResourceBarrier(1, &toCopyDest);
    
    commandList->CopyBufferRegion(
        buffer,
        0,
        upload,
        offset,
        size
    );

    auto barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            buffer,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

    commandList->ResourceBarrier(1, &barrier);
}

void GraphicsCommands::RebuildBLAS(BLAS& blas) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	inputs.NumDescs = blas.mGeometries.size();
	inputs.pGeometryDescs = blas.mGeometries.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	sContextPtr->mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	D3D12MA::ALLOCATION_DESC defaultDesc{};
	defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
    asDesc.SourceAccelerationStructureData= blas.mAllocation->GetResource()->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = blas.mAllocation->GetResource()->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = blas.mScratchAllocation->GetResource()->GetGPUVirtualAddress();

    std::vector<D3D12_RESOURCE_BARRIER> preBarriers;
    for (const auto prim : blas.mPrimitives) {
        D3D12_RESOURCE_BARRIER vertex = CD3DX12_RESOURCE_BARRIER::Transition(
            prim->vertexBuffer->GetResource(),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        );


        preBarriers.push_back(vertex);

        D3D12_RESOURCE_BARRIER index = CD3DX12_RESOURCE_BARRIER::Transition(
            prim->indexBuffer->GetResource(),
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        );

        preBarriers.push_back(index);
    }
    commandList->ResourceBarrier(preBarriers.size(), preBarriers.data());
    commandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

    std::vector<D3D12_RESOURCE_BARRIER> postBarriers;
    for (const auto prim : blas.mPrimitives) {
        D3D12_RESOURCE_BARRIER vertex = CD3DX12_RESOURCE_BARRIER::Transition(
            prim->vertexBuffer->GetResource(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        postBarriers.push_back(vertex);

        D3D12_RESOURCE_BARRIER index = CD3DX12_RESOURCE_BARRIER::Transition(
            prim->indexBuffer->GetResource(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_INDEX_BUFFER
        );

        postBarriers.push_back(index);
    }

    D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(
        blas.GetResource()
    );
    postBarriers.push_back(uavBarrier);
    commandList->ResourceBarrier(postBarriers.size(), postBarriers.data());
    blas.mAllocation->GetResource()->SetName(blas.mName.c_str());
}

void GraphicsCommands::RebuildTLAS(TLAS& tlas){
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();

    const UINT count = static_cast<UINT>(tlas.mInstanceDescs.size());

    // Upload this frame's instances
    if (count > 0) {
        memcpy(tlas.mInstanceMapped, tlas.mInstanceDescs.data(),
               sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * count);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE; // must match CreateTLAS
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = count;
    inputs.InstanceDescs = count > 0
        ? tlas.mInstanceAllocation->GetResource()->GetGPUVirtualAddress()
        : 0;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
    asDesc.Inputs = inputs;
    asDesc.DestAccelerationStructureData = tlas.GetResource()->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData = tlas.mScratchAllocation->GetResource()->GetGPUVirtualAddress();
    asDesc.SourceAccelerationStructureData = 0; // full rebuild


    // Make sure any BLAS builds/refits recorded earlier in this list have finished
    D3D12_RESOURCE_BARRIER blasBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    commandList->ResourceBarrier(1, &blasBarrier);

    commandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

    // Make the TLAS visible to DispatchRays
    D3D12_RESOURCE_BARRIER tlasBarrier = CD3DX12_RESOURCE_BARRIER::UAV(tlas.GetResource());
    commandList->ResourceBarrier(1, &tlasBarrier);
}

GraphicsContext* GraphicsCommands::sContextPtr = nullptr;
