#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"
#include "Graphics/RenderingStructs.h"
#include "Graphics/GPUResource.h"

#define CHECK_CONTEXT(OUTPUT)     \
    if (!sContextPtr) {  \
        std::cerr << "Graphics Context not Registered to command list" << std::endl;\
        return (OUTPUT);\
    }\

namespace GEM{
    class GraphicsContext;

    /// @brief This is the resource factory used to create graphics objects
    /// @note This all use the graphics context under the hood
    class ResourceFactory{
    public:
        /// @brief This is used to register the instance of the graphic context
        /// @param context The context to register
        static void RegisterContext(GraphicsContext* context);

        /// @brief this creates a graphics pipeline
        /// @param desc The specifics of the pipeline created
        /// @return A unique ptr to the the graphics pipeline
        static std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDescription& desc);
       
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
    private:
        static GraphicsContext* sContextPtr;
    };
}