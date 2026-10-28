#pragma once

#include "Core/Input.h"
#include "Renderer/Renderer.h"

namespace GEM{
    class GEM_API Scene{
    public:
        virtual ~Scene() = default;

        Scene() = default;
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        virtual void Update(Input& input, GEM::Renderer& renderer, float dt) {}
    

        std::span<Model> GetModels() {
            return mModels;
        }
    protected:
        std::vector<Model> mModels;
    };
}