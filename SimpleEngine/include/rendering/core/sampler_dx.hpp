#pragma once
#include "rendering/common_dx.hpp"
#include <unordered_map>

namespace SE
{
class SamplerView;
class Sampler;

class D3D12Sampler
{
public:
    // create sampler based on sampler either from gltf or defined gltf
    // limited set of these can be created
    static uint64_t CreateSampler(const Sampler& sampler);
    static uint64_t CreateSampler(const D3D12_SAMPLER_DESC& desc);

    static std::shared_ptr<D3D12Sampler> GetSampler(const std::shared_ptr<Sampler>& sampler);
    static std::shared_ptr<D3D12Sampler> GetSampler(uint64_t hash);

    ~D3D12Sampler();

    const SamplerView& GetView() const;

    static uint64_t Hash(const D3D12_SAMPLER_DESC& desc);

private:
    friend class Device;
    D3D12Sampler(const D3D12_SAMPLER_DESC& desc, uint64_t hash);

    static D3D12_SAMPLER_DESC CreateDescription(const SE::Sampler& sampler);

    uint64_t m_hash;
    SamplerView* m_view = nullptr;

    D3D12_SAMPLER_DESC m_desc;

    static std::unordered_map<uint64_t, std::shared_ptr<D3D12Sampler>> m_samplerMap;
};
}  // namespace SE