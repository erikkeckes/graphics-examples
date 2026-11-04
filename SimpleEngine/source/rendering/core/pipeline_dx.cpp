#include "rendering/core/pipeline_dx.hpp"
#include "rendering/core/shader_dx.hpp"
#include "rendering/core/root_signature_dx.hpp"
#include "rendering/core/pipeline_state_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"
#include "core/resources.hpp"

std::unordered_map<uint64_t, std::shared_ptr<SE::Pipeline>> SE::Pipeline::m_pipelineCache;

std::shared_ptr<SE::Pipeline> SE::Pipeline::GetPipeline(const GraphicsPipelineDesc& desc)
{
    assert(desc.m_shaderMask > 0 && "Creating pipeline for unitialized description");
    auto hash = PipelineSettings::HashSettings(desc.m_settings);

    SumHash(hash, Engine.Resources().Load<Shader>(desc.m_vertex)->GetHash());
    SumHash(hash, Engine.Resources().Load<Shader>(desc.m_pixel)->GetHash());

    if ((desc.m_shaderMask & GraphicsPipelineDesc::ShaderMask::DS) &&
        (desc.m_shaderMask & GraphicsPipelineDesc::ShaderMask::HS))
    {
        SumHash(hash, Engine.Resources().Load<Shader>(desc.m_hull)->GetHash());
        SumHash(hash, Engine.Resources().Load<Shader>(desc.m_domain)->GetHash());
    }

   /* if (CheckCache(hash))
    {
        return m_pipelineCache.at(hash);
    }*/

    auto pipeline = std::shared_ptr<Pipeline>(new Pipeline(desc));
    m_pipelineCache.insert({hash, pipeline});
    return pipeline;
}

std::shared_ptr<SE::Pipeline> SE::Pipeline::GetPipeline(const ComputePipelineDesc& desc)
{
    assert(desc.m_initialized && "Creating pipeline for unitialized description");
    auto hash = Engine.Resources().Load<Shader>(desc.m_compute)->GetHash();

   if (CheckCache(hash))
   {
        return m_pipelineCache.at(hash);
   }

    auto pipeline = std::shared_ptr<Pipeline>(new Pipeline(desc));
    m_pipelineCache.insert({hash, pipeline});
    return pipeline;
}

SE::Pipeline::Pipeline(const GraphicsPipelineDesc& desc)
{
    assert((desc.m_shaderMask & GraphicsPipelineDesc::ShaderMask::VS) && "Missing vertex shader");
    assert((desc.m_shaderMask & GraphicsPipelineDesc::ShaderMask::PS) && "Missing pixel shader");

    m_settings = desc.m_settings;
    m_shaderBinds = new ShaderParamContainer();

    // load shaders
    m_graphicShaders.vertex = Engine.Resources().Load<Shader>(desc.m_vertex);
    m_graphicShaders.pixel = Engine.Resources().Load<Shader>(desc.m_pixel);

    AddParams(*m_graphicShaders.vertex);
    AddParams(*m_graphicShaders.pixel);
    m_shaderBinds->AddShaderHash(m_graphicShaders.vertex->GetHash());
    m_shaderBinds->AddShaderHash(m_graphicShaders.pixel->GetHash());

    const bool hs = (desc.m_shaderMask & GraphicsPipelineDesc::ShaderMask::HS);
    const bool ds = (desc.m_shaderMask & GraphicsPipelineDesc::ShaderMask::DS);

    if (hs && !ds)
    {
        Log::Warn("Description only contains hull shader, domain is missing");
        Log::Warn("Skipping hull shader compilation");
    }
    else if (!hs && ds)
    {
        Log::Warn("Description only contains domain shader, hull is missing");
        Log::Warn("Skipping domain shader compilation");
    }
    else if (hs && ds)
    {
        m_graphicShaders.domain = Engine.Resources().Load<Shader>(desc.m_domain);
        m_graphicShaders.hull = Engine.Resources().Load<Shader>(desc.m_hull);

        AddParams(*m_graphicShaders.domain);
        AddParams(*m_graphicShaders.hull);
        m_shaderBinds->AddShaderHash(m_graphicShaders.domain->GetHash());
        m_shaderBinds->AddShaderHash(m_graphicShaders.hull->GetHash());
    }

    m_signature = RootSignature::GetSignature(*m_shaderBinds, m_graphicShaders.vertex->HasINputLayout());
    m_pso = PipelineStateObject::GetPSO(desc.m_settings, m_signature, m_graphicShaders);
}

SE::Pipeline::Pipeline(const ComputePipelineDesc& desc)
{
    assert(desc.m_initialized && "Missing compute shader shader");

    m_shaderBinds = new ShaderParamContainer();

    m_computeShaders.compute = Engine.Resources().Load<Shader>(desc.m_compute);
    m_shaderBinds->AddShaderHash(m_computeShaders.compute->GetHash());
    AddParams(*m_computeShaders.compute);

    // compute doesn't have a input layout
    m_signature = RootSignature::GetSignature(*m_shaderBinds, false);
    // chage to refference or use shared ptr
    m_pso = PipelineStateObject::GetPSO(m_signature, m_computeShaders.compute);

    m_compute = true;
}

SE::Pipeline::~Pipeline() { delete m_shaderBinds; }

bool SE::Pipeline::CheckCache(uint64_t hash)
{
    auto cache = m_pipelineCache.find(hash);
    return cache != m_pipelineCache.end();
}

void SE::Pipeline::AddParams(const Shader& shader)
{
    const auto& params = shader.GetParams();
    for (const auto& param : params)
    {
        m_shaderBinds->AddParam(param);
    }
}