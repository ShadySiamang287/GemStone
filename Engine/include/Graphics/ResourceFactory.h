#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/ComputePipeline.h"
#include "Graphics/RootSignature.h"
#include "Graphics/RenderingStructs.h"
#include "Graphics/GPUResource.h"
#include "Graphics/GraphicsCommands.h"
#include "Graphics/DXR/AccelerationStructs.h"
#include "Graphics/Model.h"

#define CHECK_CONTEXT(OUTPUT)     \
    if (!sContextPtr) {  \
        std::cerr << "Graphics Context not Registered to command list" << std::endl;\
        return (OUTPUT);\
    }\

namespace GEM{
    class GraphicsContext;

    /// @brief This is the resource factory used to create graphics objects
    /// @note This all use the graphics context under the hood
    class GEM_API ResourceFactory{
    public:
        /// @brief This is used to register the instance of the graphic context
        /// @param context The context to register
        static void RegisterContext(GraphicsContext* context);

        /// @brief this creates a graphics pipeline
        /// @param desc The specifics of the pipeline created
        /// @return A unique ptr to the the graphics pipeline
        static std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDescription& desc);
       
        static std::unique_ptr<ComputePipeline> CreateComputePipeline(std::shared_ptr<const RootSignature> rootSignature, ShaderBytecode shaderModule, std::wstring name);

        /// @brief This creates a root signature
        /// @param desc the description of the root signature
        /// @return a newly created root signature
        static std::shared_ptr<RootSignature> CreateRootSignature(const RootSignatureDesc& desc);

        /// @brief Creates a vertex buffer
        /// @param verticies A span of verticies so any structure can be used
        /// @return The newly created vertex buffer
        static std::unique_ptr<VertexBuffer> CreateVertexBuffer(std::span<Vertex> verticies);
    
        /// @brief Creates an index buffer
        /// @param indicies a span uint32_t indicies so any structure can be used
        /// @return the newly created index buffer
        static std::unique_ptr<IndexBuffer> CreateIndexBuffer(std::span<uint32_t> indicies);
    
    
        template <typename T>
        static std::unique_ptr<ConstantBuffer<T>> CreateConstantBuffer() {
            CHECK_CONTEXT(nullptr)

            auto buffer = std::unique_ptr<ConstantBuffer<T>>(new ConstantBuffer<T>());
            
            size_t bufferSize = (sizeof(T) + 255) & ~255;
            D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
            
            D3D12MA::ALLOCATION_DESC defaultDesc {};
            defaultDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

            THROW_IF_FAILED(
                sContextPtr->mAllocator->CreateResource(
                    &defaultDesc,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    buffer->mAllocation.ReleaseAndGetAddressOf(),
                    IID_NULL,
                    nullptr
                )
            );

            D3D12_RANGE noReadRange{0, 0};

            THROW_IF_FAILED(
                buffer->GetResource()->Map(
                    0, &noReadRange, &buffer->mMappedData
                )
            );

            return std::move(buffer);
        }

        template <typename T, size_t Capacity>
        static std::unique_ptr<StructuredBuffer<T, Capacity>>
        CreateStructuredBuffer(std::span<const T> elements) {
            CHECK_CONTEXT(nullptr)

            if (elements.size() > Capacity) {
                throw std::invalid_argument("Structured buffer data exceeds its capacity");
            }

            constexpr UINT stride = static_cast<UINT>(sizeof(T));
            constexpr UINT64 bufferSize = sizeof(T) * Capacity;

            auto buffer = std::unique_ptr<StructuredBuffer<T, Capacity>>(
                new StructuredBuffer<T, Capacity>()
            );

            D3D12_RESOURCE_DESC resourceDesc =
                CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

            D3D12MA::ALLOCATION_DESC defaultDesc{};
            defaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

            THROW_IF_FAILED(
                sContextPtr->mAllocator->CreateResource(
                    &defaultDesc,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_COMMON,
                    nullptr,
                    buffer->mAllocation.GetAddressOf(),
                    IID_NULL,
                    nullptr
                )
            );

            Microsoft::WRL::ComPtr<D3D12MA::Allocation> uploadAllocation;
            D3D12MA::ALLOCATION_DESC uploadDesc{};
            uploadDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

            THROW_IF_FAILED(
                sContextPtr->mAllocator->CreateResource(
                    &uploadDesc,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    uploadAllocation.GetAddressOf(),
                    IID_NULL,
                    nullptr
                )
            );

            ID3D12Resource* destination = buffer->GetResource();
            ID3D12Resource* upload = uploadAllocation->GetResource();

            void* mappedData = nullptr;
            D3D12_RANGE readRange{ 0, 0 };

            THROW_IF_FAILED(upload->Map(0, &readRange, &mappedData));

            if (!elements.empty()) {
                std::memcpy(
                    mappedData,
                    elements.data(),
                    elements.size_bytes()
                );
            }

            upload->Unmap(0, nullptr);

            GraphicsCommands::ExecuteImmediate(
                [&](ID3D12GraphicsCommandList4* commandList) {
                    auto toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
                        destination,
                        D3D12_RESOURCE_STATE_COMMON,
                        D3D12_RESOURCE_STATE_COPY_DEST
                    );
                    commandList->ResourceBarrier(1, &toCopyDest);

                    commandList->CopyBufferRegion(
                        destination,
                        0,
                        upload,
                        0,
                        bufferSize
                    );

                    auto toShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
                        destination,
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
                    );
                    commandList->ResourceBarrier(1, &toShaderResource);
                }
            );

            buffer->mSize = bufferSize;
            buffer->mElementCount = static_cast<UINT>(Capacity);

            const UINT srvSlot = sContextPtr->AllocateSRVSlot();
            buffer->mSRVCPU = sContextPtr->GetSRVCPUHandle(srvSlot);
            buffer->mSRVGPU = sContextPtr->GetSRVGPUHandle(srvSlot);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping =
                D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = buffer->mElementCount;
            srvDesc.Buffer.StructureByteStride = stride;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

            sContextPtr->mDevice->CreateShaderResourceView(
                destination,
                &srvDesc,
                buffer->mSRVCPU
            );

            buffer->mAllocation->SetName(L"Structured Buffer");
            return buffer;
        }
    
        static std::unique_ptr<BLAS> CreateBLAS(const Primitive& primitive, std::wstring name, bool opaque);
    
        static std::unique_ptr<TLAS> CreateTLAS(UINT maxInstances); 
    private:
        static GraphicsContext* sContextPtr;
    };
}
