#include "rendering/core/pipeline_state_dx.hpp"
#include "rendering/core/shader_dx.hpp"
#include "rendering/core/root_signature_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"

const std::string PsoCache = "ShaderCache\\pso\\";

std::unordered_map<uint64_t, std::shared_ptr<SE::PipelineStateObject>> SE::PipelineStateObject::m_psoCache;

std::shared_ptr<SE::PipelineStateObject> SE::PipelineStateObject::GetPSO(const PipelineSettings& settings,
                                                                           const std::shared_ptr<RootSignature>& signature,
                                                                           const GraphicsShaderState& shaders)
{
    auto hash = GetHash(settings, signature, shaders);

    auto map = m_psoCache.find(hash);
    if (map != m_psoCache.end())
    {
        return m_psoCache.at(hash);
    }

    auto ptr = std::shared_ptr<PipelineStateObject>(new PipelineStateObject(settings, signature, shaders));
    m_psoCache.insert({hash, ptr});

    return ptr;
}

std::shared_ptr<SE::PipelineStateObject> SE::PipelineStateObject::GetPSO(const std::shared_ptr<RootSignature>& signature,
                                                                           const std::shared_ptr<Shader>& compute)
{
    auto hash = GetHash(signature, compute);

  /*  auto map = m_psoCache.find(hash);
    if (map != m_psoCache.end())
    {
        return m_psoCache.at(hash);
    }*/

    auto ptr = std::shared_ptr<PipelineStateObject>(new PipelineStateObject(signature, compute));
    m_psoCache.insert({hash, ptr});
    return ptr;
}

uint64_t SE::PipelineStateObject::GetHash(const PipelineSettings& settings,
                                           const std::shared_ptr<RootSignature>& signature,
                                           const GraphicsShaderState& shaders)
{
    auto hash = PipelineSettings::HashSettings(settings);

    SumHash(hash, shaders.vertex->GetHash());
    SumHash(hash, shaders.pixel->GetHash());

    if (shaders.hull && shaders.domain)
    {
        SumHash(hash, shaders.hull->GetHash());
        SumHash(hash, shaders.domain->GetHash());
    }

    SumHash(hash, signature->m_hash);
    return hash;
}

uint64_t SE::PipelineStateObject::GetHash(const std::shared_ptr<RootSignature>& signature,
                                           const std::shared_ptr<Shader>& compute)
{
    auto hash = compute->GetHash() << 8;
    SumHash(hash, signature->m_hash);
    return hash;
}

SE::PipelineStateObject::PipelineStateObject(const PipelineSettings& settings,
                                              const std::shared_ptr<RootSignature>& signature,
                                              const GraphicsShaderState& shaders)
{
    assert(shaders.vertex);
    assert(shaders.pixel);
    assert(signature);

    m_hash = GetHash(settings, signature, shaders);

    bool shoulRecompile = false;

    // set shader code
    m_graphicsDesc.VS = shaders.vertex->GetByteCode();
    m_graphicsDesc.PS = shaders.pixel->GetByteCode();

    shoulRecompile |= shaders.vertex->IsCompiled() || shaders.pixel->IsCompiled();

    if (shaders.hull && shaders.domain)
    {
        m_graphicsDesc.HS = shaders.hull->GetByteCode();
        m_graphicsDesc.DS = shaders.domain->GetByteCode();

        shoulRecompile |= shaders.hull->IsCompiled() || shaders.domain->IsCompiled();
    }

    m_graphicsDesc.pRootSignature = signature->m_signature;

    // add input layout if it exists
    m_graphicsDesc.InputLayout = shaders.vertex->GetInputLayout();

    // right now only 1 render target support
    m_graphicsDesc.NumRenderTargets = settings.rtvCount;

    for (int i = 0; i < 8; i++)
    {
        m_graphicsDesc.RTVFormats[i] = settings.rtvFormats[i];
    }

    m_graphicsDesc.DSVFormat = settings.depthFormat;
    m_graphicsDesc.SampleMask = 0xFFFFFFFF;
    m_graphicsDesc.SampleDesc = {1, 0};

    m_graphicsDesc.PrimitiveTopologyType = GetTopology(settings.topology);

    // blend state
    m_graphicsDesc.BlendState.AlphaToCoverageEnable = false;
    m_graphicsDesc.BlendState.IndependentBlendEnable = false;

    const auto& blend = settings.blend;

    // description contains 8 setting for render targets
    for (auto i = 0; i < 8; i++)
    {
        m_graphicsDesc.BlendState.RenderTarget[i].BlendEnable = blend[i].blendEnable;
        m_graphicsDesc.BlendState.RenderTarget[i].LogicOpEnable = blend[i].logicalOpEnable;
        m_graphicsDesc.BlendState.RenderTarget[i].SrcBlend = GetBlendMode(blend[i].srcBlend);
        m_graphicsDesc.BlendState.RenderTarget[i].DestBlend = GetBlendMode(blend[i].dstBlend);
        m_graphicsDesc.BlendState.RenderTarget[i].BlendOp = GetBlendOperation(blend[i].blendOperation);
        m_graphicsDesc.BlendState.RenderTarget[i].SrcBlendAlpha = GetBlendMode(blend[i].srcAlphaBlend);
        m_graphicsDesc.BlendState.RenderTarget[i].DestBlendAlpha = GetBlendMode(blend[i].dstAlphaBlend);
        m_graphicsDesc.BlendState.RenderTarget[i].BlendOpAlpha = GetBlendOperation(blend[i].AlphaOperation);
        m_graphicsDesc.BlendState.RenderTarget[i].LogicOp = GetLogicalOperation(blend[i].logicalOperation);
        m_graphicsDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = blend[i].renderTargetWriteMask;
    }

    // raster state
    m_graphicsDesc.RasterizerState.FillMode = GetFillMode(settings.raster.fill);
    m_graphicsDesc.RasterizerState.CullMode = GetCullingMode(settings.raster.culling);
    m_graphicsDesc.RasterizerState.FrontCounterClockwise = settings.raster.counterClockWise;
    m_graphicsDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // depth stencil state
    const auto& depth = settings.depthStencil;
    m_graphicsDesc.DepthStencilState.DepthEnable = depth.depthEnabled;
    m_graphicsDesc.DepthStencilState.DepthWriteMask = GetDepthMask(depth.depthMask);
    m_graphicsDesc.DepthStencilState.DepthFunc = GetComparisionFunction(depth.depthComparison);
    m_graphicsDesc.DepthStencilState.StencilEnable = depth.stencilEnabled;
    m_graphicsDesc.DepthStencilState.StencilWriteMask = depth.stencilWriteMask;
    m_graphicsDesc.DepthStencilState.StencilReadMask = depth.stencilReadMask;

    m_graphicsDesc.DepthStencilState.FrontFace.StencilFailOp = GetStencilOperation(depth.frontFace.stencilFailOperation);
    m_graphicsDesc.DepthStencilState.FrontFace.StencilDepthFailOp =
        GetStencilOperation(depth.frontFace.stencilDepthFailOperation);
    m_graphicsDesc.DepthStencilState.FrontFace.StencilPassOp = GetStencilOperation(depth.frontFace.stencilPassOperation);
    m_graphicsDesc.DepthStencilState.FrontFace.StencilFunc = GetComparisionFunction(depth.frontFace.stencilFunction);

    m_graphicsDesc.DepthStencilState.BackFace.StencilFailOp = GetStencilOperation(depth.backFace.stencilFailOperation);
    m_graphicsDesc.DepthStencilState.BackFace.StencilDepthFailOp =
        GetStencilOperation(depth.backFace.stencilDepthFailOperation);
    m_graphicsDesc.DepthStencilState.BackFace.StencilPassOp = GetStencilOperation(depth.backFace.stencilPassOperation);
    m_graphicsDesc.DepthStencilState.BackFace.StencilFunc = GetComparisionFunction(depth.backFace.stencilFunction);

    m_graphicsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    auto cached = false;
    std::vector<char> binary;
    if (shoulRecompile)
    {
        Log::Info("Detected shaders changed not checking for a binary");
    }
    else
    {
        if (LoadBinary(binary))
        {
            Log::Info("Loading cached graphics pso");
            cached = true;
            m_graphicsDesc.CachedPSO.CachedBlobSizeInBytes = binary.size();
            m_graphicsDesc.CachedPSO.pCachedBlob = binary.data();
        }
        else
        {
            Log::Info("No binary detected, creating new graphics PSO");
        }
    }

    CheckHR(Engine.Device().GetDevice()->CreateGraphicsPipelineState(&m_graphicsDesc, IID_PPV_ARGS(&m_pso)));

    if (!cached)
    {
        SaveBinary();
    }
}

SE::PipelineStateObject::PipelineStateObject(const std::shared_ptr<RootSignature>& signature,
                                              const std::shared_ptr<Shader>& compute)
{
    assert(compute);
    assert(signature);

    m_hash = GetHash(signature, compute);

    m_computeDesc.CS = compute->GetByteCode();
    m_computeDesc.NodeMask = 0;

    m_computeDesc.pRootSignature = signature->m_signature;

    m_computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    auto cached = false;
    std::vector<char> binary;
    if (compute->IsCompiled())
    {
        Log::Info("Detected shaders changed, not checking for a binary");
    }
    else
    {
        if (LoadBinary(binary))
        {
            Log::Info("Loading cached compute pso");
            cached = true;
            m_computeDesc.CachedPSO.CachedBlobSizeInBytes = binary.size();
            m_computeDesc.CachedPSO.pCachedBlob = binary.data();
        }
        else
        {
            Log::Info("No binary detected, creating new compute PSO");
        }
    }

    CheckHR(Engine.Device().GetDevice()->CreateComputePipelineState(&m_computeDesc, IID_PPV_ARGS(&m_pso)));

    if (!cached)
    {
        SaveBinary();
    }
}

SE::PipelineStateObject::~PipelineStateObject() { COMFREE(m_pso); }

void SE::PipelineStateObject::SaveBinary() const
{
    Engine.FileIO().CreateDirectories(FileIO::Directory::None, PsoCache);

    ID3DBlob* blob = nullptr;
    CheckHR(m_pso->GetCachedBlob(&blob));

    std::vector<char> binary;
    binary.assign(reinterpret_cast<const char*>(blob->GetBufferPointer()),
                  reinterpret_cast<const char*>(blob->GetBufferPointer()) + blob->GetBufferSize());

    Engine.FileIO().WriteBinaryFile(PsoCache + std::to_string(m_hash), binary);
}

bool SE::PipelineStateObject::LoadBinary(std::vector<char>& binary) const
{
    auto exists = Engine.FileIO().Exists(PsoCache + std::to_string(m_hash));
    if (exists)
    {
        binary = Engine.FileIO().ReadBinaryFile(FileIO::Directory::None, PsoCache + std::to_string(m_hash));
    }
    return exists;
}
