#pragma once

namespace GEM{
    struct Vertex{
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT4 colour;
    };

    struct FrameData {
        DirectX::XMFLOAT4X4 ViewMatrix;
        DirectX::XMFLOAT4X4 ProjectionMatrix;
    };
}