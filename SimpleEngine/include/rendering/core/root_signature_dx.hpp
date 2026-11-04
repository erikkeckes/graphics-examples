#pragma once
#include "rendering/common_dx.hpp"
#include "rendering/core/shader_dx.hpp"
#include <unordered_map>

namespace SE
{

class RootSignature
{
public:
    static std::shared_ptr<RootSignature> GetSignature(const ShaderParamContainer& params, bool inputLayout = false);
    RootSignature() = delete;
    ~RootSignature();

private:
    friend class Device;               // for clearing the cache
    friend class PipelineStateObject;  // needs access to signature during creation
    friend class CommandList;          // for binding to command list
    RootSignature(const ShaderParamContainer& params, bool inputLayout = false);

    static uint64_t GetSignatureHash(const ShaderParamContainer& params, bool inputLayout = false);

    static ID3DBlob* SerializeSignature(const ShaderParamContainer& params, bool inputLayout);

    std::string GetBinaryPath() const;

    uint64_t m_hash;
    ID3D12RootSignature* m_signature = nullptr;
    ShaderParamContainer m_params;

    static std::unordered_map<uint64_t, std::shared_ptr<RootSignature>> m_signatureCache;
};
}  // namespace SE