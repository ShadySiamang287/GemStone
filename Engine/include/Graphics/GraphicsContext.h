#pragma once

namespace GEM{
    class GraphicsContext{
    public:
        GraphicsContext(HWND window);
        ~GraphicsContext();

    private:
        void CreateFactory();
        void CreateAdaptor();
        void CreateDevice();
        void CreateCommandObjects();
        void CreateFence();
        void CreateSwapchain(HWND window, uint32_t width, uint32_t height);

		Microsoft::WRL::ComPtr<IDXGIFactory6> mFactory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> mAdapter;
		Microsoft::WRL::ComPtr<ID3D12Device5> mDevice;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>mCommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;

        static constexpr UINT sFrameCount = 2;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVHeap;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, sFrameCount> mRenderTargets;

		HANDLE mFenceEvent = nullptr;
		UINT64 mFenceValue = 0;
        UINT mRTVDescriptorSize = 0;

        UINT mFrameIndex = 0;

        UINT WIDTH = 1280;
        UINT HEIGHT = 720;

        friend class GraphicsCommands;
   };
}