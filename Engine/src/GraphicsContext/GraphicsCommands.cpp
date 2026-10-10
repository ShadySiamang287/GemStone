#include "Graphics/GraphicsCommands.h"

#include "Graphics/GraphicsContext.h"

#include <pix3.h>

using namespace GEM;

void GraphicsCommands::RegisterContext(GraphicsContext* context) {
    std::cout << "Graphics Context Registered" << std::endl;
    sContextPtr = context;
}

void GraphicsCommands::BeginFrame() {
    THROW_IF_FAILED(sContextPtr->mCommandAllocator->Reset());
    
    THROW_IF_FAILED(sContextPtr->mCommandList->Reset(sContextPtr->mCommandAllocator.Get(), nullptr));
    PIXBeginEvent(sContextPtr->mCommandList.Get(), PIX_COLOR(255, 128, 0), "Test");

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = sContextPtr->mRenderTargets[sContextPtr->mFrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    sContextPtr->mCommandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sContextPtr->mRTVHeap->GetCPUDescriptorHandleForHeapStart();

    rtvHandle.ptr += sContextPtr->mFrameIndex * sContextPtr->mRTVDescriptorSize;

    const float clearColour[] =
    {
        0.05f,
        0.10f,
        0.20f,
        1.0f
    };

    sContextPtr->mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    
    sContextPtr->mCommandList->ClearRenderTargetView(rtvHandle, clearColour, 0, nullptr);
}

void GraphicsCommands::EndFrame() {
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
    
    THROW_IF_FAILED(sContextPtr->mSwapChain->Present(0, 0));
    
    sContextPtr->mFrameIndex = sContextPtr->mSwapChain->GetCurrentBackBufferIndex();
    
    sContextPtr->mFenceValue++;
    
    THROW_IF_FAILED(sContextPtr->mCommandQueue->Signal(sContextPtr->mFence.Get(), sContextPtr->mFenceValue));
    
    if (sContextPtr->mFence->GetCompletedValue() < sContextPtr->mFenceValue)
    {
        THROW_IF_FAILED(sContextPtr->mFence->SetEventOnCompletion(sContextPtr->mFenceValue, sContextPtr->mFenceEvent));
        
        WaitForSingleObject(sContextPtr->mFenceEvent, INFINITE);
    }
    PIXEndEvent(sContextPtr->mCommandList.Get());
}

GraphicsContext* GraphicsCommands::sContextPtr = nullptr;