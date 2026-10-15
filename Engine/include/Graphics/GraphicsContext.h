#pragma once

namespace GEM{
    /// @brief This class encapsulates the DirectX12 objects 
    class GraphicsContext{
    public:
        /// @brief Initialises the objects
        /// @param window The window being renderered too 
        GraphicsContext(HWND window);
        /// @brief This is needed since the fence needs to empty before the objects are destroyed.
        ~GraphicsContext();

        ID3D12Device* GetDevice() const {
            return mDevice.Get();
        }

        ID3D12CommandQueue* GetCommandQueue() const {
            return mCommandQueue.Get();
        }

        ID3D12GraphicsCommandList* GetCommandList() const {
            return mCommandList.Get();
        }

        ID3D12DescriptorHeap* GetSRVHeap() const {
            return mSRVHeap.Get();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiSRVCPUHandle() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiSRVGPUHandle() const;

    private:
        void CreateFactory();
        void CreateAdaptor();
        void CreateDevice();
        void CreateAllocator();
        void CreateCommandObjects();
        void CreateFence();
        void CreateSwapchain(HWND window, uint32_t width, uint32_t height);
        void CreateSRVHeap();

		Microsoft::WRL::ComPtr<IDXGIFactory6> mFactory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> mAdapter;
		Microsoft::WRL::ComPtr<ID3D12Device5> mDevice;

        D3D12MA::Allocator* mAllocator = nullptr;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>mCommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;

        static constexpr UINT sFrameCount = 2;
        static constexpr UINT sMaxSRVs = 24;
        static constexpr UINT sImGuiSRVSlot = sMaxSRVs - 1;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSRVHeap;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, sFrameCount> mRenderTargets;

		HANDLE mFenceEvent = nullptr;
		UINT64 mFenceValue = 0;
        UINT mRTVDescriptorSize = 0;
        UINT mSRVDescriptorSize = 0;

        UINT mFrameIndex = 0;

        UINT WIDTH = 1280;
        UINT HEIGHT = 720;

        friend class GraphicsCommands;
        friend class ResourceFactory;
        friend class RenderTargetManager;
   };
}
