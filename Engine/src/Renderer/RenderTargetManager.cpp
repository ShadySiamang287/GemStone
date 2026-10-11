#include "Renderer/RenderTargetManager.h"

#include "Graphics/GraphicsContext.h"

using namespace GEM;

RenderTargetManager::RenderTargetManager(GraphicsContext& context) : mContext(context) {
	ID3D12Device* device = mContext.mDevice.Get();

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
	rtvDesc.NumDescriptors = sMaxRenderTargets;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NodeMask = 0;

	THROW_IF_FAILED(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&mRTVHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
	dsvDesc.NumDescriptors = sMaxDepthTargets;
	dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDesc.NodeMask = 0;

	THROW_IF_FAILED(device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&mDSVHeap)));

	mRTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDSVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mSRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

RenderTarget& RenderTargetManager::CreateRenderTarget(
	std::wstring name,
	DXGI_FORMAT format,
	UINT width,
	UINT height,
	const std::array<float, 4>& clearColour
) {
	if (name.empty()) {
		throw std::invalid_argument("Render target name cannot be empty");
	}

	if (width == 0 || height == 0) {
		throw std::invalid_argument("Render target dimensions must be non-zero");
	}

	if (mRenderTargets.contains(name)) {
		throw std::runtime_error("A render target with this name already exists");
	}

	const UINT rtvSlot = AcquireRTVSlot();
	const UINT srvSlot = AcquireSRVSlot();

	RenderTarget target;
	target.mDescription = CD3DX12_RESOURCE_DESC::Tex2D(
		format,
		width,
		height,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	);

	target.mClearValue.Format = format;
	target.mClearValue.Color[0] = clearColour[0];
	target.mClearValue.Color[1] = clearColour[1];
	target.mClearValue.Color[2] = clearColour[2];
	target.mClearValue.Color[3] = clearColour[3];
	target.mState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	D3D12MA::ALLOCATION_DESC allocationDesc{};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	THROW_IF_FAILED(mContext.mAllocator->CreateResource(
		&allocationDesc,
		&target.mDescription,
		target.mState,
		&target.mClearValue,
		target.mAllocation.GetAddressOf(),
		IID_NULL,
		nullptr
	));

	target.mAllocation->SetName(name.c_str());
	target.mRTV = GetRTVHandle(rtvSlot);

	mContext.mDevice->CreateRenderTargetView(
		target.GetResource(),
		nullptr,
		target.mRTV
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	target.mSRVCPU = GetSRVCPUHandle(srvSlot);
	target.mSRVGPU = GetSRVGPUHandle(srvSlot);

	mContext.mDevice->CreateShaderResourceView(
		target.GetResource(),
		&srvDesc,
		target.mSRVCPU
	);

	auto [iterator, inserted] = mRenderTargets.emplace(std::move(name), std::move(target));

	if (!inserted) {
		throw std::runtime_error("Failed to register render target");
	}

	return iterator->second;
}

DepthTarget& RenderTargetManager::CreateDepthTarget(std::wstring name, UINT width, UINT height) {
	if (name.empty()) {
		throw std::invalid_argument("Depth target name cannot be empty");
	}

	if (width == 0 || height == 0) {
		throw std::invalid_argument("Depth target dimensions must be non-zero");
	}

	if (mDepthTargets.contains(name)) {
		throw std::runtime_error("A depth target with this name already exists");
	}

	const UINT dsvSlot = AcquireDSVSlot();
	const UINT srvSlot = AcquireSRVSlot();

	DepthTarget target;
	target.mDescription = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32_TYPELESS,
		width,
		height,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	target.mClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	target.mClearValue.DepthStencil.Depth = 1.0f;
	target.mClearValue.DepthStencil.Stencil = 0;
	target.mState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	D3D12MA::ALLOCATION_DESC allocationDesc{};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	THROW_IF_FAILED(mContext.mAllocator->CreateResource(
		&allocationDesc,
		&target.mDescription,
		target.mState,
		&target.mClearValue,
		target.mAllocation.GetAddressOf(),
		IID_NULL,
		nullptr
	));

	target.mAllocation->SetName(name.c_str());
	target.mDSV = GetDSVHandle(dsvSlot);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	mContext.mDevice->CreateDepthStencilView(
		target.GetResource(),
		&dsvDesc,
		target.mDSV
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	target.mSRVCPU = GetSRVCPUHandle(srvSlot);
	target.mSRVGPU = GetSRVGPUHandle(srvSlot);

	mContext.mDevice->CreateShaderResourceView(
		target.GetResource(),
		&srvDesc,
		target.mSRVCPU
	);

	auto [iterator, inserted] = mDepthTargets.emplace(std::move(name), std::move(target));

	if (!inserted) {
		throw std::runtime_error("Failed to register depth target");
	}

	return iterator->second;
}

RenderTarget* RenderTargetManager::GetRenderTarget(std::wstring_view name) noexcept {
	auto iterator = mRenderTargets.find(std::wstring(name));
	return iterator == mRenderTargets.end() ? nullptr : &iterator->second;
}

DepthTarget* RenderTargetManager::GetDepthTarget(std::wstring_view name) noexcept {
	auto iterator = mDepthTargets.find(std::wstring(name));
	return iterator == mDepthTargets.end() ? nullptr : &iterator->second;
}

const RenderTarget* RenderTargetManager::GetRenderTarget(std::wstring_view name) const noexcept {
	auto iterator = mRenderTargets.find(std::wstring(name));
	return iterator == mRenderTargets.end() ? nullptr : &iterator->second;
}

const DepthTarget* RenderTargetManager::GetDepthTarget(std::wstring_view name) const noexcept {
	auto iterator = mDepthTargets.find(std::wstring(name));
	return iterator == mDepthTargets.end() ? nullptr : &iterator->second;
}

UINT RenderTargetManager::AcquireRTVSlot() {
	if (mNextRTVSlot >= sMaxRenderTargets) {
		throw std::runtime_error("Render target descriptor heap is full");
	}

	return mNextRTVSlot++;
}

UINT RenderTargetManager::AcquireDSVSlot() {
	if (mNextDSVSlot >= sMaxDepthTargets) {
		throw std::runtime_error("Depth target descriptor heap is full");
	}

	return mNextDSVSlot++;
}

UINT RenderTargetManager::AcquireSRVSlot() {
	if (mNextSRVSlot >= mContext.sMaxSRVs) {
		throw std::runtime_error("SRV descriptor heap is full");
	}

	return mNextSRVSlot++;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetManager::GetRTVHandle(UINT slot) const noexcept {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(slot) * mRTVDescriptorSize;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetManager::GetDSVHandle(UINT slot) const noexcept {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = mDSVHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(slot) * mDSVDescriptorSize;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetManager::GetSRVCPUHandle(UINT slot) const noexcept {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = mContext.mSRVHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(slot) * mSRVDescriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderTargetManager::GetSRVGPUHandle(UINT slot) const noexcept {
	D3D12_GPU_DESCRIPTOR_HANDLE handle = mContext.mSRVHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(slot) * mSRVDescriptorSize;
	return handle;
}
