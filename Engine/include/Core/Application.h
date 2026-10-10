#pragma once

#include "Core/GemStoneAPI.h"

#include "Core/Window.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"

#include "Graphics/GPUResource.h"

namespace GEM{
    /// @brief The base version of the application that gets inherited from
    class GEM_API Application {
    public:
        /// @brief The constructor that is used to initialise the app
        /// @param instance The instance handle passed into the constructor
        Application(HINSTANCE instance);
		virtual ~Application() = default;

		/// @brief  This function runs the game loop
		void Run();

    private:
        std::unique_ptr<Window> mWindow;
        std::unique_ptr<GraphicsContext> mContext;
        std::unique_ptr<GraphicsPipeline> mPipeline;
        std::shared_ptr<RootSignature> mRootSig;
        std::unique_ptr<VertexBuffer> mVertexBuffer;
        std::unique_ptr<IndexBuffer> mIndexBuffer;
    };

    /// @brief This function is used to the instance of the application the program uses
    /// @param instance The instance handle provided by windows
    /// @return The application that the program runs
    /// @note the you dont need to call it just define it.
    extern Application* CreateApplication(HINSTANCE instance);
}