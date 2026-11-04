#include "rendering/core/root_signature_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"

const std::string SignatureCache = "ShaderCache\\signatures\\";
std::unordered_map<uint64_t, std::shared_ptr<SE::RootSignature>> SE::RootSignature::m_signatureCache;

std::shared_ptr<SE::RootSignature> SE::RootSignature::GetSignature(const ShaderParamContainer& params, bool inputLayout)
{
    auto hash = GetSignatureHash(params, inputLayout);

    auto cache = m_signatureCache.find(hash);
    if (cache != m_signatureCache.end())
    {
        return m_signatureCache.at(hash);
    }

    auto ptr = std::shared_ptr<RootSignature>(new RootSignature(params, inputLayout));
    m_signatureCache.insert({hash, ptr});
    return ptr;
}

uint64_t SE::RootSignature::GetSignatureHash(const ShaderParamContainer& params, bool inputLayout)
{
    auto hash = static_cast<uint64_t>(D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED);
    if (inputLayout)
    {
        SumHash(hash, static_cast<uint64_t>(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT));
    }

    for (auto param : params.GetShaderParams())
    {
        SumHash(hash, static_cast<uint64_t>(param.paramHash));
        SumHash(hash, static_cast<uint64_t>(param.nameHash.GetHash()));
    }

    SumHash(hash, static_cast<uint64_t>(params.GetShaderParams().size()));

    return hash;
}

ID3DBlob* SE::RootSignature::SerializeSignature(const ShaderParamContainer& params, bool inputLayout)
{
    D3D12_ROOT_SIGNATURE_DESC desc{};
    // using bindless textures
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
    if (inputLayout)
    {
        desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }

    const auto& inputParams = params.GetShaderParams();
    desc.NumParameters = static_cast<UINT>(inputParams.size());

    std::vector<D3D12_ROOT_PARAMETER> signatureParams;
    signatureParams.resize(inputParams.size());
    int index = 0;
    for (const auto& param : inputParams)
    {
        signatureParams[index] = param.param;
        index++;
    }

    desc.pParameters = signatureParams.data();
    desc.NumStaticSamplers = 0;
    desc.pStaticSamplers = nullptr;

    ID3DBlob* serializedSignature = nullptr;
    ID3DBlob* errorBlob = nullptr;
    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorBlob);
    if (errorBlob != nullptr && errorBlob->GetBufferSize() != 0)
    {
        SE::Log::Error("Signature serialization failed {}", reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    COMFREE(errorBlob);
    return serializedSignature;
}

std::string SE::RootSignature::GetBinaryPath() const { return SignatureCache + std::to_string(m_hash); }

SE::RootSignature::RootSignature(const ShaderParamContainer& params, bool inputLayout)
{
    m_hash = GetSignatureHash(params, inputLayout);
    m_params = params;

    if (Engine.FileIO().Exists(GetBinaryPath()))
    {
        // loading precompiled binary
        Log::Info("Loading precompiled Root signature");
        auto contains = Engine.FileIO().ReadBinaryFile(FileIO::Directory::None, GetBinaryPath());
        CheckHR(
            Engine.Device().GetDevice()->CreateRootSignature(0, contains.data(), contains.size(), IID_PPV_ARGS(&m_signature)));
    }
    else
    {
        // creating new binary
        Log::Info("Creating new Root signature");
        ID3DBlob* serializedBlob = SerializeSignature(params, inputLayout);
        CheckHR(Engine.Device().GetDevice()->CreateRootSignature(0,
                                                                 serializedBlob->GetBufferPointer(),
                                                                 serializedBlob->GetBufferSize(),
                                                                 IID_PPV_ARGS(&m_signature)));

        std::vector<char> binary;
        binary.assign(reinterpret_cast<const char*>(serializedBlob->GetBufferPointer()),
                      reinterpret_cast<const char*>(serializedBlob->GetBufferPointer()) + serializedBlob->GetBufferSize());

        Engine.FileIO().CreateDirectories(FileIO::Directory::None, SignatureCache);
        Engine.FileIO().WriteBinaryFile(GetBinaryPath(), binary);

        COMFREE(serializedBlob);
    }

    assert(m_signature);
}

SE::RootSignature::~RootSignature() { COMFREE(m_signature); }
