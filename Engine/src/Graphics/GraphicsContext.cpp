#include "Graphics/GraphicsContext.h"

using namespace GEM;

using namespace Microsoft::WRL;

GraphicsContext::GraphicsContext(HWND window) {
	CreateFactory();
	CreateAdaptor();
	CreateDevice();
	CreateAllocator();
	CreateCommandObjects();
	CreateFence();
	CreateSwapchain(window, WIDTH, HEIGHT);
	CreateSRVHeap();
}

GraphicsContext::~GraphicsContext() {
	if (mFenceEvent){
		CloseHandle(mFenceEvent);
		mFenceEvent = nullptr;
	}

	if (mAllocator) {
		mAllocator->Release();
		mAllocator = nullptr;
	}
}

void GraphicsContext::CreateFactory() {
	UINT factoryFlags = 0;

	ComPtr<ID3D12Debug> debugController;
	#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	#endif

	THROW_IF_FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mFactory)));
	THROW_IF_FAILED(mFactory->SetPrivateData(
		WKPDID_D3DDebugObjectName,
		sizeof(L"DXGI Factory"),
		L"DXGI Factory"));
}

void GraphicsContext::CreateAdaptor() {
		for (UINT index = 0;; index++) {
		ComPtr<IDXGIAdapter4> adapter;

		HRESULT result = mFactory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));

		if (result == DXGI_ERROR_NOT_FOUND)
			break;

		if (FAILED(result))
			continue;

		DXGI_ADAPTER_DESC3 description{};
		THROW_IF_FAILED(adapter->GetDesc3(&description));

		if (description.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
		{
			continue;
		}

		// Check whether this adapter can create a D3D12 device.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			mAdapter = adapter;
			break;
		}
	}

	if (!mAdapter) {
		throw std::runtime_error("No Suitable Direct3D 12 adaptor found");
	}

	THROW_IF_FAILED(mAdapter->SetPrivateData(
		WKPDID_D3DDebugObjectName,
		sizeof(L"Graphics Adapter"),
		L"Graphics Adapter"));
}

void GraphicsContext::CreateAllocator() {
	D3D12MA::ALLOCATOR_DESC desc{};
    desc.pDevice = mDevice.Get();
    desc.pAdapter = mAdapter.Get();
	desc.Flags = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS;

    THROW_IF_FAILED(
        D3D12MA::CreateAllocator(&desc, &mAllocator)
    );
}

void GraphicsContext::CreateDevice(){
    Microsoft::WRL::ComPtr<ID3D12Device> baseDevice;

    THROW_IF_FAILED(D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&baseDevice)));

    // Query the ray-tracing-capable interface.
    THROW_IF_FAILED(baseDevice.As(&mDevice));
	THROW_IF_FAILED(mDevice->SetName(L"D3D12 Device"));

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};

    THROW_IF_FAILED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));

    if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        throw std::runtime_error("The selected adapter does not support ray tracing");
    }
}

void GraphicsContext::CreateCommandObjects(){
	D3D12_COMMAND_QUEUE_DESC queueDescription{};
    queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    queueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    THROW_IF_FAILED(mDevice->CreateCommandQueue(&queueDescription, IID_PPV_ARGS(&mCommandQueue)));
	THROW_IF_FAILED(mCommandQueue->SetName(L"Command Queue"));

	THROW_IF_FAILED(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
	THROW_IF_FAILED(mCommandAllocator->SetName(L"Direct Command Allocator"));

    THROW_IF_FAILED(mDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&mCommandList))
    );
	THROW_IF_FAILED(mCommandList->SetName(L"Direct Command List"));

    // Command lists start open.
    THROW_IF_FAILED(mCommandList->Close());
}

void GraphicsContext::CreateFence() {
	// Create the synchronization fence.
    THROW_IF_FAILED(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	THROW_IF_FAILED(mFence->SetName(L"GPU Fence"));

    mFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (!mFenceEvent)
    {
        throw std::runtime_error("Failed to create fence event");
    }
}

void GraphicsContext::CreateSwapchain(HWND window, uint32_t width, uint32_t height){
	DXGI_SWAP_CHAIN_DESC1 description{};

    description.BufferCount = sFrameCount;
    description.Width = width;
    description.Height = height;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    description.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> temporarySwapChain;

    THROW_IF_FAILED(mFactory->CreateSwapChainForHwnd(
        mCommandQueue.Get(),
        window,
        &description,
        nullptr,
        nullptr,
        &temporarySwapChain)
    );

    THROW_IF_FAILED(temporarySwapChain.As(&mSwapChain));
	THROW_IF_FAILED(mSwapChain->SetPrivateData(
		WKPDID_D3DDebugObjectName,
		sizeof(L"Swap Chain"),
		L"Swap Chain"));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};

    heapDescription.NumDescriptors = sFrameCount;
    heapDescription.Type =D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    THROW_IF_FAILED(mDevice->CreateDescriptorHeap(&heapDescription, IID_PPV_ARGS(&mRTVHeap)));

    mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE handle = mRTVHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

    for (UINT index = 0;index < sFrameCount; index++)
    {
        THROW_IF_FAILED(mSwapChain->GetBuffer(index,IID_PPV_ARGS(&mRenderTargets[index])));

        mDevice->CreateRenderTargetView(mRenderTargets[index].Get(), &rtvDesc, handle);

        handle.ptr += mRTVDescriptorSize;
    }

}

void GraphicsContext::CreateSRVHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC description{};
	description.NumDescriptors = sMaxSRVs;
	description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	description.NodeMask = 0;

	THROW_IF_FAILED(mDevice->CreateDescriptorHeap(&description, IID_PPV_ARGS(&mSRVHeap)));
	THROW_IF_FAILED(mSRVHeap->SetName(L"SRV Descriptor Heap"));
	mSRVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsContext::GetImGuiSRVCPUHandle() const {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = mSRVHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(sImGuiSRVSlot) * mSRVDescriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE GraphicsContext::GetImGuiSRVGPUHandle() const {
	D3D12_GPU_DESCRIPTOR_HANDLE handle = mSRVHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(sImGuiSRVSlot) * mSRVDescriptorSize;
	return handle;
}
