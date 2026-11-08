#pragma once

#include "Graphics/GPUResource.h"
#include "Graphics/Model.h"

namespace GEM{
    // this feels dumb right?
    class GEM_API BLAS : public GPUResource{
    protected:
        BLAS() {}
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> mGeometries;
        std::vector<const Primitive*> mPrimitives;

        Microsoft::WRL::ComPtr<D3D12MA::Allocation> mScratchAllocation;
        UINT64 mScratchSize = 0;
        
        std::wstring mName;

        friend class ResourceFactory;
        friend class GraphicsCommands;
        friend class TLAS;
    };

    class GEM_API ProceduralBLAS : public BLAS{
        ProceduralBLAS();

        Microsoft::WRL::ComPtr<D3D12MA::Allocation> mAABBAllocation;

        friend class ResourceFactory;
        friend class GraphicsCommands;
        friend class TLAS;
    };

    class GEM_API TLAS : public GPUResource{
    public:
        ~TLAS();

        void AddInstance(BLAS& targetBlas, UINT instanceID, UINT mask, glm::mat4x3 transform);
        void Clear();

        D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPU() const noexcept {
            return mSRVCPU;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const noexcept {
            return mSRVGPU;
        }

    private:
        TLAS() {}

        D3D12_CPU_DESCRIPTOR_HANDLE mSRVCPU{};
        D3D12_GPU_DESCRIPTOR_HANDLE mSRVGPU{};

        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> mInstanceDescs;
        std::vector<BLAS*> mBLASes;
        UINT mNextHitGroupOffset = 0;
        UINT mCapacity = 0;

        UINT64 mScratchSize = 0;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> mScratchAllocation;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> mInstanceAllocation; // upload heap
        void* mInstanceMapped = nullptr;

        friend class ResourceFactory;
        friend class GraphicsCommands;
    };
}