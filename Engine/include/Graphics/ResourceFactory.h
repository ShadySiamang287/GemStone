#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/GraphicsRootSignature.h"

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
        static std::shared_ptr<GraphicsRootSignature> CreateGraphicsRootSignature(const GraphicsRootSignatureDesc& desc);
    private:
        static GraphicsContext* sContextPtr;
    };
}