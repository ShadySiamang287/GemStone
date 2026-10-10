#pragma once

namespace GEM {
    struct RootSignatureDesc{
        std::vector<D3D12_ROOT_PARAMETER> parameters;
        std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
        D3D12_ROOT_SIGNATURE_FLAGS flags;
        std::wstring name;
    };

    class RootSignature {
    public:
    
    ID3D12RootSignature* Get() const noexcept {
        return mSignature.Get();
    }
    
    private:
        RootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature>&& signature) : mSignature(std::move(signature)) {}
        
        Microsoft::WRL::ComPtr<ID3D12RootSignature> mSignature;
        friend class ResourceFactory;
    };
}