#pragma once

#include <glm/glm.hpp>

namespace GEM{
    struct Vertex{
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 colour;
    };

    struct FrameData {
        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
    };
}
