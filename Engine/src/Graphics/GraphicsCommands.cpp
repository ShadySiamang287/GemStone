#include "Graphics/GraphicsCommands.h"

#include "Graphics/GraphicsContext.h"

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

void GraphicsCommands::ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList*)>& record){
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

    ++context->mFenceValue;

    THROW_IF_FAILED(context->mCommandQueue->Signal(context->mFence.Get(), context->mFenceValue));

    if (context->mFence->GetCompletedValue() < context->mFenceValue){
        THROW_IF_FAILED(context->mFence->SetEventOnCompletion(context->mFenceValue,context->mFenceEvent));

        WaitForSingleObject(context->mFenceEvent, INFINITE);
    }
}


void GraphicsCommands::BeginFrame() {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();

    THROW_IF_FAILED(sContextPtr->mCommandAllocator->Reset());
    
    THROW_IF_FAILED(commandList->Reset(sContextPtr->mCommandAllocator.Get(), nullptr));
    PIXBeginEvent(commandList, PIX_COLOR(255, 128, 0), "Test");

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = sContextPtr->mRenderTargets[sContextPtr->mFrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sContextPtr->mRTVHeap->GetCPUDescriptorHandleForHeapStart();

    rtvHandle.ptr += sContextPtr->mFrameIndex * sContextPtr->mRTVDescriptorSize;

    const float clearColour[] =
    {
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    
    commandList->ClearRenderTargetView(rtvHandle, clearColour, 0, nullptr);
}

void GraphicsCommands::EndFrame() {
    CHECK_CONTEXT

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = sContextPtr->mRenderTargets[sContextPtr->mFrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    
    sContextPtr->mCommandList->ResourceBarrier(1, &barrier);
    
    PIXEndEvent(sContextPtr->mCommandList.Get());

    THROW_IF_FAILED(sContextPtr->mCommandList->Close());
    
    ID3D12CommandList* commandLists[] =
    {
        sContextPtr->mCommandList.Get()
    };
    
    sContextPtr->mCommandQueue->ExecuteCommandLists(1, commandLists);
    
    THROW_IF_FAILED(sContextPtr->mSwapChain->Present(0, 0));
    
    sContextPtr->mFrameIndex = sContextPtr->mSwapChain->GetCurrentBackBufferIndex();
    
    sContextPtr->mFenceValue++;
    
    THROW_IF_FAILED(sContextPtr->mCommandQueue->Signal(sContextPtr->mFence.Get(), sContextPtr->mFenceValue));
    
    if (sContextPtr->mFence->GetCompletedValue() < sContextPtr->mFenceValue)
    {
        THROW_IF_FAILED(sContextPtr->mFence->SetEventOnCompletion(sContextPtr->mFenceValue, sContextPtr->mFenceEvent));
        
        WaitForSingleObject(sContextPtr->mFenceEvent, INFINITE);
    }
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

void GraphicsCommands::BindGraphicsPipeline(GraphicsPipeline& pipeline) {
    CHECK_CONTEXT

    auto* commandList = sContextPtr->mCommandList.Get();
    commandList->SetGraphicsRootSignature(pipeline.mRootSignature.Get());
    commandList->SetPipelineState(pipeline.mPipelineState.Get());
    commandList->IASetPrimitiveTopology(pipeline.mTopology);
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

GraphicsContext* GraphicsCommands::sContextPtr = nullptr;