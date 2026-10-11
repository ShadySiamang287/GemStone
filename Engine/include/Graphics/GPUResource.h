#pragma once
namespace GEM{
    class GPUResource{
    public:
        ID3D12Resource* GetResource() const {
            if (mAllocation) {
                return mAllocation->GetResource();
            }
            return nullptr;
        }
        void Reset() {
            mAllocation.Reset();
        }

    protected:
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> mAllocation;

        friend class ResourceFactory;
        friend class RenderTargetManager;
    };

    class GPUBuffer : public GPUResource{
        uint64_t GetSize() const {
            return mSize;
        }
    protected:
        uint64_t mSize = 0;
    };

    class VertexBuffer : public GPUBuffer{
    public:
        const D3D12_VERTEX_BUFFER_VIEW& GetView() const {
            return mView;
        }

        uint32_t GetVertexCount() const {
            return mVertexCount;
        }
    private:
        D3D12_VERTEX_BUFFER_VIEW mView{};
        uint32_t mVertexCount = 0;
        uint32_t mStride = 0;

        friend class ResourceFactory;
    };

    class IndexBuffer : public GPUBuffer {
    public:
        const D3D12_INDEX_BUFFER_VIEW& GetView() const {
            return mView;
        }

        uint32_t GetIndexCount() const {
            return mIndexCount;
        }
    private:
        D3D12_INDEX_BUFFER_VIEW mView{};
        uint32_t mIndexCount = 0;
        uint32_t mStride = 0;

        friend class ResourceFactory;
    };

    class GPUTexture : public GPUResource {
    public:
        const D3D12_RESOURCE_DESC& GetDescription() const {
            return mDescription;
        }

        DXGI_FORMAT GetFormat() const {
            return mDescription.Format;
        }

        UINT GetWidth() const {
            return static_cast<UINT>(mDescription.Width);
        }

        UINT GetHeight() const {
            return mDescription.Height;
        }

        D3D12_RESOURCE_STATES GetState() const {
            return mState;
        }

    protected:
        D3D12_RESOURCE_DESC mDescription{};
        D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;

        friend class ResourceFactory;
        friend class RenderTargetManager;
    };

    class RenderTarget : public GPUTexture {
    public:
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const {
            return mRTV;
        }

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE mRTV{};

        D3D12_CLEAR_VALUE mClearValue{};

        friend class ResourceFactory;
        friend class RenderTargetManager;
    };

    class DepthTarget : public GPUTexture {
    public:
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const {
            return mDSV;
        }

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE mDSV{};

        D3D12_CLEAR_VALUE mClearValue{};

        friend class ResourceFactory;
        friend class RenderTargetManager;
    };
}
