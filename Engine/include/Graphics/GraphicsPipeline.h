#pragma once

namespace GEM{
    class RootSignature;

    struct GEM_API ShaderBytecode
    {
        std::vector<std::uint8_t> data;

        D3D12_SHADER_BYTECODE GetBytecode() const noexcept
        {
            return{
                data.data(),
                data.size()
            };
        }

        ShaderBytecode(std::filesystem::path path);
    };

    enum class FaceCulling {
        NONE = 1,
        FRONT = 2,
        BACK = 3
    };

    struct GEM_API GraphicsPipelineDescription
    {
        std::wstring name;
        std::shared_ptr<const RootSignature> rootSignature = nullptr;

        std::optional<ShaderBytecode> vertexShader = std::nullopt;
        std::optional<ShaderBytecode> pixelShader = std::nullopt;
        std::optional<ShaderBytecode> domainShader = std::nullopt;
        std::optional<ShaderBytecode> hullShader = std::nullopt;
        std::optional<ShaderBytecode> geometryShader = std::nullopt;

        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;

        std::vector<DXGI_FORMAT> renderTargetFormats = {
            DXGI_FORMAT_R8G8B8A8_UNORM
        };

        D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        FaceCulling culling = FaceCulling::BACK;
        bool transparency = false;
        bool depthTest = true;
        bool depthWrite = true;
        bool wireframe = false;
    };

    class GEM_API GraphicsPipeline{
    private:
        GraphicsPipeline() {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;

        D3D12_PRIMITIVE_TOPOLOGY mTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    
        friend class GraphicsCommands;
        friend class ResourceFactory;
    };
}