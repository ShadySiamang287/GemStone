#pragma once

#include "Graphics/GPUResource.h"

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

namespace GEM{
    class GraphicsContext;

    class GEM_API RenderTargetManager {
    public:
        explicit RenderTargetManager(GraphicsContext& context);

        void Resize(UINT width, UINT height);

        UINT GetWidth() const {
            return mWidth;
        }

        UINT GetHeight() const {
            return mHeight;
        }

        RenderTarget& CreateRenderTarget(
            std::wstring name,
            DXGI_FORMAT format,
            UINT width,
            UINT height,
            const std::array<float, 4>& clearColour = {0.0f, 0.0f, 0.0f, 1.0f}
        );
        DepthTarget& CreateDepthTarget(std::wstring name, UINT width, UINT height);

        RenderTarget* GetRenderTarget(std::wstring_view name);
        DepthTarget* GetDepthTarget(std::wstring_view name);

        const RenderTarget* GetRenderTarget(std::wstring_view name) const;
        const DepthTarget* GetDepthTarget(std::wstring_view name) const;

    private:
        static constexpr UINT sMaxRenderTargets = 16;
        static constexpr UINT sMaxDepthTargets = 8;

        UINT AcquireRTVSlot();
        UINT AcquireDSVSlot();
        UINT AcquireSRVSlot();

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(UINT slot) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(UINT slot) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(UINT slot) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(UINT slot) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPUHandle(UINT slot) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGPUHandle(UINT slot) const;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDSVHeap;

        UINT mRTVDescriptorSize = 0;
        UINT mDSVDescriptorSize = 0;
        UINT mNextRTVSlot = 0;
        UINT mNextDSVSlot = 0;
        UINT mWidth = 1280;
        UINT mHeight = 720;

        std::unordered_map<std::wstring, RenderTarget> mRenderTargets;
        std::unordered_map<std::wstring, DepthTarget> mDepthTargets;

        GraphicsContext& mContext;
    };
}
