#pragma once

#include "Graphics/GPUResource.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace GEM{
    class GraphicsContext;

    class RenderTargetManager {
    public:
        explicit RenderTargetManager(GraphicsContext& context);

        RenderTarget& CreateRenderTarget(std::wstring name, DXGI_FORMAT format, UINT width, UINT height);
        DepthTarget& CreateDepthTarget(std::wstring name, UINT width, UINT height);

        RenderTarget* GetRenderTarget(std::wstring_view name) noexcept;
        DepthTarget* GetDepthTarget(std::wstring_view name) noexcept;

        const RenderTarget* GetRenderTarget(std::wstring_view name) const noexcept;
        const DepthTarget* GetDepthTarget(std::wstring_view name) const noexcept;

    private:
        static constexpr UINT sMaxRenderTargets = 16;
        static constexpr UINT sMaxDepthTargets = 8;

        UINT AcquireRTVSlot();
        UINT AcquireDSVSlot();

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(UINT slot) const noexcept;
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(UINT slot) const noexcept;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDSVHeap;

        UINT mRTVDescriptorSize = 0;
        UINT mDSVDescriptorSize = 0;
        UINT mNextRTVSlot = 0;
        UINT mNextDSVSlot = 0;

        std::unordered_map<std::wstring, RenderTarget> mRenderTargets;
        std::unordered_map<std::wstring, DepthTarget> mDepthTargets;

        GraphicsContext& mContext;
    };
}
