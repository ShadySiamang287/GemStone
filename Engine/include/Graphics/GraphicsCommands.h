#pragma once

#include "Graphics/GraphicsPipeline.h"
#include "Graphics/GPUResource.h"

namespace GEM {
    class GraphicsContext;

    /// @brief These are the commands used for issueing commands to the graphics card
    /// @note This all use the graphics context under the hood
    class GraphicsCommands{
    public:
        /// @brief This is used to register the instance of the graphic context
        /// @param context The context to register
        static void RegisterContext(GraphicsContext* context);

        /// @brief Executes the lambda and then stalls the GPU uuntil its finished
        /// @param record The commands to execute
        static void ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList*)>& record);

        /// @brief Prepares the frame to be renderered
        static void BeginFrame();

        /// @brief Ends the frame and presents
        static void EndFrame();
        
        /// @brief Draws a mesh without indicies
        /// @param vertexCount the number of verticies
        /// @param instanceCount the number of instances to draw
        /// @param firstVertex where the first vertex is
        /// @param firstInstance where the first instance is
        static void Draw(const int& vertexCount, const int& instanceCount, const int& firstVertex, const int& firstInstance);

        /// @brief Draws a mesh with indicies
        /// @param indexCount the number of indicies
        /// @param instanceCount the number of instances to draw
        /// @param firstIndex where the first index is
        /// @param firstVertex where the first vertex is
        /// @param firstInstance where the first instance is
        static void DrawIndexed(const int& indexCount, const int& instanceCount, const int& firstIndex, const int& firstVertex, const int& firstInstance);


        /// @brief Binds the viewport
        /// @param x1 intial X
        /// @param y1 inital Y
        /// @param x2 Extent X
        /// @param y2 Extent Y
        /// @param minDepth the minimum depth
        /// @param maxDepth the max depth
        static void BindViewport(const float& x1, const float& y1, const float& x2, const float& y2, const float& minDepth, const float& maxDepth);
        
        /// @brief Binds the scissor rect
        /// @param x1 intial X
        /// @param y1 inital Y
        /// @param x2 Extent X
        /// @param y2 Extent Y
        static void BindScissorRect(const int& x1, const int& y1, const int& x2, const int& y2);
        
        /// @brief Binds the vertex buffer to be drawn from
        /// @param buffer The buffer to bind
        static void BindVertexBuffer(const VertexBuffer& buffer);
        
        /// @brief Binds the index buffer to be drawn from
        /// @param buffer The buffer to bind
        static void BindIndexBuffer(const IndexBuffer& buffer);

        /// @brief This binds pipeline to the current render pass
        /// @param pipeline The pipeline to bind
        static void BindGraphicsPipeline(GraphicsPipeline& pipeline);

        static void BindSRV(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptor);

        static void TransitionRTV(
            RenderTarget& renderTarget,
            D3D12_RESOURCE_STATES before,
            D3D12_RESOURCE_STATES after
        );

        static void TransitionDSV(
            DepthTarget& depthTarget,
            D3D12_RESOURCE_STATES before,
            D3D12_RESOURCE_STATES after
        );

        static void ClearRenderTarget(
            RenderTarget& renderTarget,
            const float clearColour[4]
        );

        static void ClearSwapchainImage(const float clearColour[4]);

        static void ClearDepthStencil(
            DepthTarget& depthTarget,
            D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH,
            float depth = 1.0f,
            UINT8 stencil = 0
        );

        static void BindRenderTargets(std::span<RenderTarget*> renderTargets = {}, DepthTarget* depthTarget = nullptr);

    private:
        static GraphicsContext* sContextPtr;
    };
}
