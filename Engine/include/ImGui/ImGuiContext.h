#pragma once

struct ImGui_ImplDX12_InitInfo;

namespace GEM {
    class GraphicsContext;

    /// @brief Owns the Dear ImGui context and its Win32/D3D12 backends.
    class ImGuiContext {
    public:
        ImGuiContext(GraphicsContext& graphicsContext, HWND window);
        ~ImGuiContext();

        ImGuiContext(const ImGuiContext&) = delete;
        ImGuiContext& operator=(const ImGuiContext&) = delete;

        /// @brief Starts a new ImGui frame.
        void BeginFrame();

        /// @brief Records the current ImGui draw data into an open command list.
        void Render(ID3D12GraphicsCommandList* commandList);

    private:
        static void AllocateDescriptor(
            ::ImGui_ImplDX12_InitInfo* info,
            D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle
        );

        static void FreeDescriptor(
            ::ImGui_ImplDX12_InitInfo* info,
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle
        );

        GraphicsContext& mGraphicsContext;
        bool mInitialized = false;
    };
}
