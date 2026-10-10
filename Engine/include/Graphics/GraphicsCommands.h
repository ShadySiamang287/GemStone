#pragma once

#include "Graphics/GraphicsPipeline.h"

namespace GEM {
    class GraphicsContext;

    /// @brief These are the commands used for issueing commands to the graphics card
    /// @note This all use the graphics context under the hood
    class GraphicsCommands{
    public:
        /// @brief This is used to register the instance of the graphic context
        /// @param context The context to register
        static void RegisterContext(GraphicsContext* context);

        /// @brief Prepares the frame to be renderered
        static void BeginFrame();
        /// @brief Ends the frame and presents
        static void EndFrame();
        static void Draw(const int& vertexCount, const int& instanceCount, const int& firstVertex, const int& firstInstance);

        static void BindViewport(const float& x1, const float& y1, const float& x2, const float& y2, const float& minDepth, const float& maxDepth);
        static void BindScissorRect(const int& x1, const int& y1, const int& x2, const int& y2);



        /// @brief This binds pipeline to the current render pass
        /// @param pipeline The pipeline to bind
        static void BindGraphicsPipeline(GraphicsPipeline& pipeline);

    private:
        static GraphicsContext* sContextPtr;
    };
}