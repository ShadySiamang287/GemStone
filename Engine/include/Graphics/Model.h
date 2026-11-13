#pragma once

#include "Graphics/GPUResource.h"

namespace GEM{
    struct Primitive {
        std::unique_ptr<VertexBuffer> vertexBuffer;
        std::unique_ptr<IndexBuffer> indexBuffer;
    };
    
    struct Model{
        std::vector<Primitive> primitives;
    };
}