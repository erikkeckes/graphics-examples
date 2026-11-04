#include "rendering/core/sampler_dx.hpp"
#include "rendering/core/views_dx.hpp"
#include "components/rendering_components.hpp"

std::unordered_map<uint64_t, std::shared_ptr<SE::D3D12Sampler>> SE::D3D12Sampler::m_samplerMap;

D3D12_TEXTURE_ADDRESS_MODE GetAddressMode(SE::Sampler::Wrap wrap)
{
    switch (wrap)
    {
        case SE::Sampler::Wrap::ClampEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case SE::Sampler::Wrap::MiroredRepeat:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case SE::Sampler::Wrap::Repeat:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

// created with help of chatGPT tp match all combinations of types
D3D12_FILTER GetFilterMode(const SE::Sampler& sampler)
{
    if (sampler.magFiler == SE::Sampler::Filter::Linear)
    {
        switch (sampler.minFiler)
        {
            case SE::Sampler::Filter::Linear:
                return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            case SE::Sampler::Filter::Nearest:
                return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            case SE::Sampler::Filter::LinearMipLinear:
                return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            case SE::Sampler::Filter::LinearMipNearest:
                return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            case SE::Sampler::Filter::NearestMipLinear:
                return D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_POINT,
                                                 D3D12_FILTER_TYPE_LINEAR,
                                                 D3D12_FILTER_TYPE_LINEAR,
                                                 D3D12_FILTER_REDUCTION_TYPE_STANDARD);
            case SE::Sampler::Filter::NearestMipNearest:
                return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
    }
    else if (sampler.magFiler == SE::Sampler::Filter::Nearest)
    {
        switch (sampler.minFiler)
        {
            case SE::Sampler::Filter::Linear:
                return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            case SE::Sampler::Filter::Nearest:
                return D3D12_FILTER_MIN_MAG_MIP_POINT;
            case SE::Sampler::Filter::LinearMipLinear:
                return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            case SE::Sampler::Filter::LinearMipNearest:
                return D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_LINEAR,
                                                 D3D12_FILTER_TYPE_POINT,
                                                 D3D12_FILTER_TYPE_POINT,
                                                 D3D12_FILTER_REDUCTION_TYPE_STANDARD);
            case SE::Sampler::Filter::NearestMipLinear:
                return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            case SE::Sampler::Filter::NearestMipNearest:
                return D3D12_FILTER_MIN_MAG_MIP_POINT;
        }
    }
    else
    {
        // mag fileter can be only linear or nearest based on specification
        assert(false && "Creating sampler with invalid parameters");
    }

    return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

D3D12_SAMPLER_DESC SE::D3D12Sampler::CreateDescription(const SE::Sampler& sampler)
{
    D3D12_SAMPLER_DESC desc{};
    desc.Filter = GetFilterMode(sampler);

    desc.AddressU = GetAddressMode(sampler.wrapS);
    desc.AddressV = GetAddressMode(sampler.wrapT);
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;  // GLTF inlcudes only 2D texture samplers

    desc.MinLOD = 0;
    desc.MaxLOD = FLT_MAX;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 1;

    desc.BorderColor[0] = 0.0f;
    desc.BorderColor[1] = 0.0f;
    desc.BorderColor[2] = 0.0f;
    desc.BorderColor[3] = 0.0f;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    return desc;
}

uint64_t SE::D3D12Sampler::CreateSampler(const Sampler& sampler) { return CreateSampler(CreateDescription(sampler)); }

uint64_t SE::D3D12Sampler::CreateSampler(const D3D12_SAMPLER_DESC& desc)
{
    uint64_t hash = Hash(desc);

    auto mapEntry = m_samplerMap.find(hash);
    if (mapEntry != m_samplerMap.end())
    {
        return hash;
    }

    // would use std::make_shared, don't know how to use it with private constructor
    auto newSampler = std::shared_ptr<D3D12Sampler>(new D3D12Sampler(desc, hash));
    m_samplerMap.insert({hash, newSampler});
    return hash;
}

std::shared_ptr<SE::D3D12Sampler> SE::D3D12Sampler::GetSampler(const std::shared_ptr<Sampler>& sampler)
{
    // could generate shorter hash directly from sampler
    auto hash = Hash(CreateDescription(*sampler));
    return m_samplerMap.at(hash);
}

std::shared_ptr<SE::D3D12Sampler> SE::D3D12Sampler::GetSampler(uint64_t hash) { return m_samplerMap.at(hash); }

SE::D3D12Sampler::D3D12Sampler(const D3D12_SAMPLER_DESC& desc, uint64_t hash) : m_hash(hash), m_desc(desc)
{
    m_view = new SamplerView(m_desc);
}

SE::D3D12Sampler::~D3D12Sampler()
{
    // remove sampler from static map
    delete m_view;
}

const SE::SamplerView& SE::D3D12Sampler::GetView() const
{
    assert(m_view && "Sampler Accessing non existing view");
    return *m_view;
}

uint64_t SE::D3D12Sampler::Hash(const D3D12_SAMPLER_DESC& desc)
{
    uint64_t hash = 0;
    // 4
    SumHash(hash, static_cast<uint64_t>(desc.Filter));  // max value 469

    // 4
    // use last 4 bytes, with 1 bit offset
    // max value of this enum is 5
    SumHash(hash, static_cast<uint64_t>(desc.AddressU) << 32);
    SumHash(hash, static_cast<uint64_t>(desc.AddressV) << 33);
    SumHash(hash, static_cast<uint64_t>(desc.AddressW) << 34);

    // 4
    SumHash(hash, static_cast<uint64_t>(desc.ComparisonFunc));  // max value 8

    // 4
    // shift each float by 1 bite
    SumHash(hash, static_cast<uint64_t>(desc.BorderColor[0]));  // float 4 bites
    SumHash(hash, static_cast<uint64_t>(desc.BorderColor[1]) << 1);
    SumHash(hash, static_cast<uint64_t>(desc.BorderColor[2]) << 2);
    SumHash(hash, static_cast<uint64_t>(desc.BorderColor[3]) << 3);

    // 4
    // using last 4 bytes
    SumHash(hash, static_cast<uint64_t>(desc.MaxAnisotropy) << 32);  // float 4 bites

    // 4
    SumHash(hash, static_cast<uint64_t>(desc.MinLOD));           // float 4 bites
    SumHash(hash, static_cast<uint64_t>(desc.MaxLOD) << 1);      // float 4 bites
    SumHash(hash, static_cast<uint64_t>(desc.MipLODBias) << 2);  // float 4 bites

    return hash;
}
