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
}