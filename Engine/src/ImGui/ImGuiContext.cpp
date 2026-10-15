#include "ImGui/ImGuiContext.h"

#include "Graphics/GraphicsContext.h"

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>

namespace GEM {

ImGuiContext::ImGuiContext(GraphicsContext& graphicsContext, HWND window)
    : mGraphicsContext(graphicsContext) {
    IMGUI_CHECKVERSION();
    ::ImGui::CreateContext();
    ::ImGui::StyleColorsDark();

    ::ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplWin32_Init(window)) {
        ::ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialise ImGui Win32 backend");
    }

    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = mGraphicsContext.GetDevice();
    initInfo.CommandQueue = mGraphicsContext.GetCommandQueue();
    initInfo.NumFramesInFlight = 2;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    initInfo.SrvDescriptorHeap = mGraphicsContext.GetSRVHeap();
    initInfo.UserData = this;
    initInfo.SrvDescriptorAllocFn = &ImGuiContext::AllocateDescriptor;
    initInfo.SrvDescriptorFreeFn = &ImGuiContext::FreeDescriptor;

    if (!ImGui_ImplDX12_Init(&initInfo)) {
        ImGui_ImplWin32_Shutdown();
        ::ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialise ImGui DirectX 12 backend");
    }

    mInitialized = true;
}

ImGuiContext::~ImGuiContext() {
    if (!mInitialized) {
        return;
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ::ImGui::DestroyContext();
}

void ImGuiContext::BeginFrame() {
    if (!mInitialized) {
        return;
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ::ImGui::NewFrame();
}

void ImGuiContext::Render(ID3D12GraphicsCommandList* commandList) {
    if (!mInitialized || !commandList) {
        return;
    }

    ::ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(::ImGui::GetDrawData(), commandList);
}

void ImGuiContext::AllocateDescriptor(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle
) {
    auto* context = static_cast<ImGuiContext*>(info->UserData);
    *cpuHandle = context->mGraphicsContext.GetImGuiSRVCPUHandle();
    *gpuHandle = context->mGraphicsContext.GetImGuiSRVGPUHandle();
}

void ImGuiContext::FreeDescriptor(
    ImGui_ImplDX12_InitInfo*,
    D3D12_CPU_DESCRIPTOR_HANDLE,
    D3D12_GPU_DESCRIPTOR_HANDLE
) {
    // The descriptor is a permanently reserved slot in GraphicsContext's heap.
}

}
