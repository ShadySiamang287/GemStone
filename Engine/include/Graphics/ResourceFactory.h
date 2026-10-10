#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"
#include "Graphics/RenderingStructs.h"
#include "Graphics/GPUResource.h"

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
    private:
        static GraphicsContext* sContextPtr;
    };
}