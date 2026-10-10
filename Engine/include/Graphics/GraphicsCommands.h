#pragma once

namespace GEM {
    class GraphicsContext;

    class GraphicsCommands{
    public:
        static void RegisterContext(GraphicsContext* context);

        static void BeginFrame();
        static void EndFrame();

    private:
        static GraphicsContext* sContextPtr;
    };
}