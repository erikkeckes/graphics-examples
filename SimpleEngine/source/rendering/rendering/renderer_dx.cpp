#include "rendering/rendering/renderer_dx.hpp"
#include "rendering/core/pipeline_dx.hpp"
#include "rendering/core/buffer_dx.hpp"
#include "rendering/core/command_list_dx.hpp"
#include "rendering/core/command_queue_dx.hpp"
#include "rendering/core/upload_buffer.hpp"
#include "rendering/core/views_dx.hpp"
#include "rendering/core/texture_dx.hpp"
#include "rendering/core/sampler_dx.hpp"
#include "rendering/core/resource_dx.hpp"

#include "core/engine.hpp"

#include "rendering/core/device_dx.hpp"
#include "core/resources.hpp"
#include "components/rendering_components.hpp"
#include "resources/model.hpp"
#include "resources/mesh.hpp"
#include "resources/image.hpp"
#include "components/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "rendering/rendering/shader_struct.hpp"
#include "core/fileio.hpp"

#include "imgui/imgui.h"
#include "resources/skybox.hpp"
#include <glm/gtc/type_ptr.hpp>

glm::vec2 QuadVerticies[] = {
    glm::vec2{-1.0f, -1.0f},  // bottom-left
    glm::vec2{1.0f, -1.0f},   // bottom-right
    glm::vec2{1.0f, 1.0f},    // top-right
    glm::vec2{-1.0f, 1.0f}    // top-left
};
// Indices (two triangles)
uint16_t QuadIndicies[] = {0, 1, 2, 0, 2, 3};

constexpr uint32_t LUTSize = 2048u;

void SE::Renderer::Render()
{
    m_upload->Reset();

    CheckGbuffersResize();

    auto view = SE::Engine.ECS().Registry.view<SE::Camera, SE::Transform>();

    if (view.size_hint() <= 0)
    {
        Log::Warn("Scene doens't contain a camera");
        Log::Warn("Rendering won't work without camera");
    }
    else
    {
        CameraData camera{};
        CameraData invCamera{};
        for (const auto& [ent, cam, trans] : view.each())
        {
            camera.projection = cam.projection;
            camera.view = glm::inverse(trans.GetWorld());
            camera.eyePosition = trans.GetTranslation();

            invCamera.projection = glm::inverse(cam.projection);
            invCamera.view = trans.GetWorld();
            invCamera.eyePosition = trans.GetTranslation();
        }

        const auto cameraBuff = m_upload->Allocate(camera);
        const auto invCameraBuff = m_upload->Allocate(invCamera);

        ClearGbuffers();

        GPass(cameraBuff);

        // rest of the m_colorModes are debug modes
        if (m_colorMode == 0 || m_colorMode == 7 || m_colorMode == 10)
        {
            if (m_colorMode != 10)  // ibl only
            {
                PointLightPreepass(cameraBuff);

                DirectLight(invCameraBuff);

                PointLight(invCameraBuff, cameraBuff);
            }

            if (m_colorMode != 7)  // no ibl
            {
                IBL(invCameraBuff);
            }

            if (m_colorMode == 0)
            {
                Bloom();

                ToneMapingPass();
            }
        }
        else
        {
            RenderDebug(invCameraBuff);
        }
    }

    {
        auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);
        auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);

        queue.GPUSynchronize(Engine.Device().GetQueue(QueueType::ComputeQueue));

        list.CopyTexture(*m_defferedTarget, Engine.Device().GetBackBuffer());

        Engine.Device().GetBackBuffer().Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);

        list.Close();
        queue.Execute(list);
    }
}

SE::Renderer::Renderer()
{
    m_upload = new UploadBuffer();

    auto model = Engine.Resources().Load<Model>(FileIO::Directory::SharedAssets, "models/Sphere.gltf");
    // specific sphere model for optimization purposes
    m_sphere = model->GetMeshes()[0][0];

    CreateDefferedPasses();

    CreateBuffers();

    CreateSamplers();

    GenerateLut();
}

SE::Renderer::~Renderer()
{
    // to make sure all work is finished
    Engine.Device().Flush();

    for (auto i = 0; i < 3; i++)
    {
        delete m_gbuffers[i];
    }

    delete m_defferedTarget;
    delete m_bloomTarget;
    delete m_lutTexture;
    delete m_upload;
    delete m_quadIndecies;
    delete m_quadVerticies;
}

void SE::Renderer::SetTexture(TextureData& data, const std::shared_ptr<Texture>& texture) const
{
    data.textureIndex = texture->image->m_texture->GetSRV().GetHandle();

    if (data.samplerIndex != UINT32_MAX)
    {
        data.samplerIndex = D3D12Sampler::GetSampler(texture->sampler->m_hash)->GetView().GetHandle();
    }
    else
    {
        data.samplerIndex = m_defaultSampler;
    }
}

std::string SE::Renderer::GetTexturePath(const std::shared_ptr<Texture>& texture)
{
    if (!texture)
    {
        return std::string();
    }

    if (!texture->image)
    {
        return std::string();
    }

    return texture->image->m_texture->GetName();
}

void SE::Renderer::CreateDefferedPasses()
{
    // gpass pipeline
    {
        SE::ShaderDesc vs{};
        vs.InitAsVertexShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\g_pass_vs.hlsl");

        SE::ShaderDesc ps{};
        ps.InitAsPixelShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\g_pass_ps.hlsl");

        PipelineSettings settings{};
        settings.topology = Topology::Triangle;
        settings.depthStencil.depthEnabled = true;
        settings.depthStencil.depthComparison = ComparisonFunction::Less;

        settings.raster.culling = CullMode::Back;

        settings.rtvCount = 3;
        settings.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        settings.rtvFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
        settings.rtvFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;

        SE::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.SetVertexShader(vs).SetPixelShader(ps).SetSettigns(settings);

        m_gpassPipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // direct light pass
    {
        ShaderDesc vs{};
        vs.InitAsVertexShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\quad_vs.hlsl");

        ShaderDesc ps{};
        ps.InitAsPixelShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\direct_light_ps.hlsl");

        PipelineSettings settings{};
        settings.rtvCount = 1;
        settings.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        settings.topology = Topology::Triangle;

        GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.SetVertexShader(vs).SetPixelShader(ps).SetSettigns(settings);

        m_directPipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // point light stencil prepass
    {
        ShaderDesc vs{};
        vs.InitAsVertexShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\stencil_write_vs.hlsl");

        ShaderDesc ps{};
        ps.InitAsPixelShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\stencil_write_ps.hlsl");

        PipelineSettings settings{};
        settings.topology = Topology::Triangle;

        // depth testing without write
        settings.depthStencil.depthEnabled = true;
        settings.depthStencil.depthMask = DepthMask::NoMask;
        settings.depthStencil.depthComparison = ComparisonFunction::Less;

        settings.depthStencil.stencilEnabled = true;
        settings.depthStencil.stencilWriteMask = 0xff;

        settings.depthStencil.frontFace.stencilDepthFailOperation = StencilOperation::Increment;
        settings.depthStencil.frontFace.stencilFailOperation = StencilOperation::Keep;
        settings.depthStencil.frontFace.stencilPassOperation = StencilOperation::Keep;
        settings.depthStencil.frontFace.stencilFunction = ComparisonFunction::Always;

        settings.depthStencil.backFace.stencilDepthFailOperation = StencilOperation::Decrement;
        settings.depthStencil.backFace.stencilFailOperation = StencilOperation::Keep;
        settings.depthStencil.backFace.stencilPassOperation = StencilOperation::Keep;
        settings.depthStencil.backFace.stencilFunction = ComparisonFunction::Always;

        GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.SetVertexShader(vs).SetPixelShader(ps).SetSettigns(settings);

        m_pointStencilPipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    PipelineSettings settings = CreateDefferdSettings();

    // point light pass
    {
        ShaderDesc vs{};
        vs.InitAsVertexShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\point_light_vs.hlsl");

        ShaderDesc ps{};
        ps.InitAsPixelShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\point_light_ps.hlsl");

        auto pSettings = settings;
        pSettings.raster.culling = CullMode::Front;
        pSettings.depthStencil.depthEnabled = true;
        pSettings.depthStencil.depthComparison = ComparisonFunction::Always;
        pSettings.depthStencil.depthMask = DepthMask::NoMask;

        pSettings.depthStencil.stencilReadMask = 0xff;
        pSettings.depthStencil.stencilEnabled = true;
        pSettings.depthStencil.frontFace.stencilFunction = ComparisonFunction::NotEqual;
        pSettings.depthStencil.backFace.stencilFunction = ComparisonFunction::NotEqual;

        GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.SetVertexShader(vs).SetPixelShader(ps).SetSettigns(pSettings);

        m_pointPipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // ibl light pass
    {
        ShaderDesc vs{};
        vs.InitAsVertexShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\quad_vs.hlsl");

        ShaderDesc ps{};
        ps.InitAsPixelShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\ibl_ps.hlsl");

        GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.SetVertexShader(vs).SetPixelShader(ps).SetSettigns(settings);

        m_iblPipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // tone mapping
    {
        ShaderDesc cs{};
        cs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\tone_mapping_cs.hlsl");

        ComputePipelineDesc pipelineDesc{};
        pipelineDesc.InitComputePipeline(cs);

        m_toneMappingPipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // blooom pre pass pass
    {
        ShaderDesc cs{};
        cs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\bloom_extract_cs.hlsl");

        ComputePipelineDesc pipelineDesc{};
        pipelineDesc.InitComputePipeline(cs);

        m_bloomPrepassPipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // blooom down sample
    {
        ShaderDesc cs{};
        cs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\bloom_downsample_cs.hlsl");

        ComputePipelineDesc pipelineDesc{};
        pipelineDesc.InitComputePipeline(cs);

        m_bloomDownsamplePipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // bloom upsample
    {
        ShaderDesc cs{};
        cs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\bloom_upsample_cs.hlsl");

        ComputePipelineDesc pipelineDesc{};
        pipelineDesc.InitComputePipeline(cs);

        m_bloomUpsamplePipeline = Pipeline::GetPipeline(pipelineDesc);
    }

    // debug
    {
        ShaderDesc vs{};
        vs.InitAsVertexShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\quad_vs.hlsl");

        ShaderDesc ps{};
        ps.InitAsPixelShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\debug_ps.hlsl");

        PipelineSettings dsettings{};
        dsettings.rtvCount = 1;
        dsettings.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        dsettings.topology = Topology::Triangle;

        GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.SetVertexShader(vs).SetPixelShader(ps).SetSettigns(dsettings);

        m_debugPipeline = Pipeline::GetPipeline(pipelineDesc);
    }
}

void SE::Renderer::CreateBuffers()
{
    // rtvs
    {
        TextureDesc rtvDesc{};
        rtvDesc.InitAsRenderTarget(1200, 720, DXGI_FORMAT_R8G8B8A8_UNORM)
            .SetMips(1)
            .AddFlags(TextureDesc::SRV)
            .SetName("GAlbido");

        rtvDesc.clearColor[0] = 0;
        rtvDesc.clearColor[1] = 0;
        rtvDesc.clearColor[2] = 0;
        rtvDesc.clearColor[3] = 0;

        m_gbuffers[0] = new D3D12Texture(rtvDesc);

        rtvDesc.SetName("GEmisiveAo");
        m_gbuffers[1] = new D3D12Texture(rtvDesc);

        rtvDesc.SetName("NormalRougnessMetalic");
        rtvDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;  // need higher precision
        m_gbuffers[2] = new D3D12Texture(rtvDesc);

        rtvDesc.AddFlags(TextureDesc::UAV);
        rtvDesc.format = BackBufferFormat;

        rtvDesc.SetName("DefferedCombined");
        m_defferedTarget = new D3D12Texture(rtvDesc);

        rtvDesc.SetName("Bloom generation").SetMips(5);
        m_bloomTarget = new D3D12Texture(rtvDesc);
    }

    // vertex bufffer for full screen quad
    {
        BufferDes desc{};
        desc.InitAsVertexBuffer(sizeof(glm::vec2), 4, QuadVerticies, "Gbuffer quad vertex");

        m_quadVerticies = new Buffer(desc);
    }

    // index bufffer for full screen quad
    {
        BufferDes desc{};
        desc.InitAsIndexBuffer(sizeof(uint16_t), 6, QuadIndicies, "Gbuffer quad index");

        m_quadIndecies = new Buffer(desc);
    }
}

void SE::Renderer::CreateSamplers()
{
    D3D12_SAMPLER_DESC desc{};
    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

    desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    desc.MinLOD = 0;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 1;

    desc.BorderColor[0] = 0.0f;
    desc.BorderColor[1] = 0.0f;
    desc.BorderColor[2] = 0.0f;
    desc.BorderColor[3] = 0.0f;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;

    // point sampler
    {
        auto hash = D3D12Sampler::CreateSampler(desc);
        m_bufferSampler = D3D12Sampler::GetSampler(hash)->GetView().GetHandle();
    }

    {
        desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

        desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        auto hash = D3D12Sampler::CreateSampler(desc);
        m_defaultSampler = D3D12Sampler::GetSampler(hash)->GetView().GetHandle();
    }

    {
        desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;

        desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;

        auto hash = D3D12Sampler::CreateSampler(desc);
        m_bloomSampler = D3D12Sampler::GetSampler(hash)->GetView().GetHandle();
    }
}

void SE::Renderer::GenerateLut()
{
    TextureDesc desc{};
    desc.InitAsTexture2D(LUTSize, LUTSize, DXGI_FORMAT_R16G16_FLOAT).AllowUAV().SetMips(1).SetName("LUT texture");

    m_lutTexture = new D3D12Texture(desc);

    ShaderDesc cs{};
    cs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\ibl_lut_cs.hlsl");

    ComputePipelineDesc pipelineDesc{};
    pipelineDesc.InitComputePipeline(cs);

    const auto pipeline = Pipeline::GetPipeline(pipelineDesc);

    auto& queue = Engine.Device().GetQueue(QueueType::ComputeQueue);
    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);

    list.StartMarker("LUT Generation");
    list.BeginRender(pipeline, {});

    const auto lutHandle = m_lutTexture->GetUAV().GetHandle();
    list.SetConstants("LutData", &lutHandle, sizeof(uint32_t));

    list.Dispatch(LUTSize / 32, LUTSize / 32, 1);
    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::CheckGbuffersResize()
{
    const auto& desc = m_gbuffers[0]->GetDesc();

    if (desc.widht != static_cast<uint32_t>(Engine.Device().GetWidth()) ||
        desc.height != static_cast<uint32_t>(Engine.Device().GetHeight()))
    {
        for (int i = 0; i < 3; i++)
        {
            auto gdesc = m_gbuffers[i]->GetDesc();
            gdesc.widht = Engine.Device().GetWidth();
            gdesc.height = Engine.Device().GetHeight();

            delete m_gbuffers[i];
            m_gbuffers[i] = new D3D12Texture(gdesc);
        }

        {
            auto ddesc = m_defferedTarget->GetDesc();
            ddesc.widht = Engine.Device().GetWidth();
            ddesc.height = Engine.Device().GetHeight();

            delete m_defferedTarget;
            m_defferedTarget = new D3D12Texture(ddesc);
        }

        {
            auto ddesc = m_bloomTarget->GetDesc();
            ddesc.widht = Engine.Device().GetWidth();
            ddesc.height = Engine.Device().GetHeight();
            ddesc.SetMips(5);

            delete m_bloomTarget;
            m_bloomTarget = new D3D12Texture(ddesc);
        }
    }
}

void SE::Renderer::ClearGbuffers() const
{
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);

    list.StartMarker("Gbuffer clearing");

    for (int i = 0; i < 3; i++)
    {
        list.ClearScreen(*m_gbuffers[i], Engine.Device().GetWidth(), Engine.Device().GetHeight());
    }

    list.ClearScreen(*m_defferedTarget, Engine.Device().GetWidth(), Engine.Device().GetHeight());

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::GPass(const D3D12_GPU_VIRTUAL_ADDRESS& cameraBuff) const
{
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);

    list.StartMarker("Gpass");

    RenderingData gData{};
    gData.defaultTarget = false;
    gData.renderTargets.resize(3);
    for (int i = 0; i < 3; i++)
    {
        gData.renderTargets[i] = m_gbuffers[i];
    }

    list.BeginRender(m_gpassPipeline, gData);

    Engine.Device().GetDepthBuffer().Transition(list, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    list.SetCBV("Camera", cameraBuff);

    for (const auto& [ent, instance, transform] : Engine.ECS().Registry.view<MeshInstance, Transform>().each())
    {
        list.SetConstants("ModelMatrix", glm::value_ptr(transform.GetWorld()), sizeof(glm::mat4), m_upload);

        const auto instanceMat = instance.materail;

        MaterialData matData{};
        matData.normalScale = instanceMat->normalTextureScale;
        matData.occlusionStrenght = instanceMat->occlusionTextureStrength;
        matData.rougnessFactor = instanceMat->roughnessFactor;
        matData.baseColorFactor = instanceMat->baseColorFactor;
        matData.emisiveFactor = instanceMat->emissiveFactor;
        matData.textureMask = instanceMat->textureMask;

        if (instanceMat->textureMask & Material::TextureMask::Albedo)
        {
            SetTexture(matData.albedaTexture, instance.materail->baseColorTexture);
        }
        if (instanceMat->textureMask & Material::TextureMask::Normal)
        {
            SetTexture(matData.normalTexture, instance.materail->normalTexture);
        }
        if (instanceMat->textureMask & Material::TextureMask::Emisive)
        {
            SetTexture(matData.emisiveTexture, instance.materail->emissiveTexture);
        }
        if (instanceMat->textureMask & Material::TextureMask::MetalRougness)
        {
            SetTexture(matData.metalRougnessTexture, instance.materail->metallicRoughnessTexture);
        }
        if (instanceMat->textureMask & Material::TextureMask::Occlusion)
        {
            SetTexture(matData.occlusionTexture, instance.materail->occlusionTexture);
        }

        list.SetConstants("material", &matData, sizeof(MaterialData), m_upload);

        list.SetIndexBuffer(*instance.mesh->m_index);
        list.SetVertexBuffer(*instance.mesh->m_vertex);
        list.DrawCall(instance.mesh->m_index->GetElementCount());
    }

    for (int i = 0; i < 3; i++)
    {
        m_gbuffers[i]->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::DirectLight(const D3D12_GPU_VIRTUAL_ADDRESS& invCameraBuff)
{
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);

    list.StartMarker("direct Light pass");
    RenderingData directData{};
    directData.defaultTarget = false;
    directData.renderTargets.resize(1);
    directData.renderTargets[0] = m_defferedTarget;

    list.BeginRender(m_directPipeline, directData);
    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);

    Engine.Device().GetDepthBuffer().Transition(list, D3D12_RESOURCE_STATE_DEPTH_READ);
    SetupCommandListForDefferedLights(list, m_quadVerticies, m_quadIndecies, false);

    list.SetCBV("Camera", invCameraBuff);

    glm::vec2 dim;
    dim.x = static_cast<float>(Engine.Device().GetWidth());
    dim.y = static_cast<float>(Engine.Device().GetHeight());
    list.SetConstants("Screen", &dim[0], sizeof(glm::vec2));

    // add spot light later
    for (const auto& [ent, light, trans] : SE::Engine.ECS().Registry.view<SE::Light, SE::Transform>().each())
    {
        if (light.type & Light::Direct)
        {
            DirectLightData lightData;
            lightData.color = light.color;
            lightData.intensity = light.intensity;
            lightData.direction = trans.GetWorld() * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
            list.SetConstants("Light", &lightData, sizeof(DirectLightData), m_upload);

            list.DrawCall(m_quadIndecies->GetElementCount());
        }
    }

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::PointLightPreepass(const D3D12_GPU_VIRTUAL_ADDRESS& cameraBuff) const
{
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);

    list.StartMarker("point Light pree pass");
    RenderingData directData{};
    directData.defaultTarget = false;
    list.BeginRender(m_pointStencilPipeline, directData);

    list.SetVertexBuffer(*m_sphere->m_vertex);
    list.SetIndexBuffer(*m_sphere->m_index);

    list.SetCBV("Camera", cameraBuff);

    for (const auto& [ent, light, trans] : SE::Engine.ECS().Registry.view<SE::Light, SE::Transform>().each())
    {
        if (light.type & Light::Point)
        {
            // render to stencil
            auto world = trans.GetWorld();
            list.SetConstants("MVP", &world[0][0], sizeof(glm::mat4), m_upload);

            list.DrawCall(m_sphere->m_index->GetElementCount());
        }
    }

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::PointLight(const D3D12_GPU_VIRTUAL_ADDRESS& invCameraBuff, const D3D12_GPU_VIRTUAL_ADDRESS& cameraBuff)
{
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);

    list.StartMarker("point Light pass");
    RenderingData directData{};
    directData.defaultTarget = false;
    directData.renderTargets.resize(1);
    directData.renderTargets[0] = m_defferedTarget;
    list.BeginRender(m_pointPipeline, directData);

    Engine.Device().GetDepthBuffer().Transition(list, D3D12_RESOURCE_STATE_DEPTH_READ);
    SetupCommandListForDefferedLights(list, m_sphere->m_vertex, m_sphere->m_index, true);

    list.SetCBV("Camera", cameraBuff);
    list.SetCBV("InvCamera", invCameraBuff);
    glm::vec2 dim;
    dim.x = static_cast<float>(Engine.Device().GetWidth());
    dim.y = static_cast<float>(Engine.Device().GetHeight());
    list.SetConstants("Screen", &dim[0], sizeof(glm::vec2));

    for (const auto& [ent, light, trans] : SE::Engine.ECS().Registry.view<SE::Light, SE::Transform>().each())
    {
        if (light.type & Light::Point)
        {
            PointLightData lightData;
            lightData.color = light.color;
            lightData.intensity = light.intensity;
            lightData.range = light.range;
            // extracting translation from world matrix
            lightData.position = trans.GetWorld() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            list.SetConstants("Light", &lightData, sizeof(PointLightData), m_upload);

            if (trans.GetScale() != glm::vec3(light.range, light.range, light.range))
            {
                trans.SetScale(glm::vec3(light.range, light.range, light.range));
            }
            const auto world = trans.GetWorld();
            list.SetConstants("ModelMatrix", &world, sizeof(glm::mat4), m_upload);

            list.DrawCall(m_sphere->m_index->GetElementCount());
        }
    }

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::IBL(const D3D12_GPU_VIRTUAL_ADDRESS& cameraBuff)
{
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);

    list.StartMarker("image based Light pass");
    RenderingData directData{};
    directData.defaultTarget = false;
    directData.renderTargets.resize(1);
    directData.renderTargets[0] = m_defferedTarget;
    list.BeginRender(m_iblPipeline, directData);
    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& depth = Engine.Device().GetDepthBuffer();
    depth.Transition(list, D3D12_RESOURCE_STATE_DEPTH_READ);
    SetupCommandListForDefferedLights(list, m_quadVerticies, m_quadIndecies, false);

    // ibl only mode, direct light pass handles overdraw of combined buffer
    // no direct light pass need to clear screen
    if (m_colorMode == 10)
    {
        list.ClearScreen(*m_defferedTarget, Engine.Device().GetWidth(), Engine.Device().GetHeight());
    }

    list.SetCBV("Camera", cameraBuff);

    glm::vec2 dim;
    dim.x = static_cast<float>(Engine.Device().GetWidth());
    dim.y = static_cast<float>(Engine.Device().GetHeight());
    list.SetConstants("Screen", &dim[0], sizeof(glm::vec2));

    IBLEnviroment ibl{};
    int envCount = 0;
    for (const auto& [ent, env] : Engine.ECS().Registry.view<Enviroment>().each())
    {
        const auto& sk = env.m_skyox;
        const auto& filterDesc = sk->m_prefilteredMaps->GetDesc();
        ibl.envirometMask = 1;
        ibl.irradianceIndex = sk->m_iradiance->GetSRV().GetHandle();
        ibl.prefilterdIndex = sk->m_prefilteredMaps->GetSRV().GetHandle();
        ibl.envSamplerIndex = m_defaultSampler;
        ibl.envSamplerIndex = m_bufferSampler;
        ibl.filterMips = filterDesc.mips - 1;
        ibl.lutIndex = m_lutTexture->GetSRV().GetHandle();

        envCount++;
        if (envCount > 1)
        {
            Log::Warn("Detecting multiple skyboxes for ibl, last will be eused");
        }
    }

    list.SetConstants("enviroment", &ibl, sizeof(IBLEnviroment), m_upload);

    list.DrawCall(m_quadIndecies->GetElementCount());

    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_COMMON);

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::ToneMapingPass() const
{
    if (m_mappingStyle == 0)
    {
        return;
    }

    Engine.Device().GetQueue(QueueType::ComputeQueue).GPUSynchronize(Engine.Device().GetQueue(QueueType::DirectQueue));

    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::ComputeQueue);

    list.StartMarker("tone mapping");
    list.BeginRender(m_toneMappingPipeline, {});

    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    DefferedLigth textureInfo{};
    textureInfo.writeBuffer = m_defferedTarget->GetSRV().GetHandle();

    list.SetConstants("Textures", &textureInfo, sizeof(DefferedLigth), m_upload);

    list.SetConstants("ToneMapping", &m_mappingStyle, sizeof(uint32_t));

    list.Dispatch(static_cast<uint32_t>(Engine.Device().GetWidth()) / 32u,
                  static_cast<uint32_t>(Engine.Device().GetHeight()) / 32u);

    list.UavBarrier(*m_defferedTarget);

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::Bloom() const
{
    if (m_bloomStrength <= 0)
    {
        return;
    }

    {
        auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);
        auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);

        m_bloomTarget->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);
        list.ClearScreen(*m_bloomTarget, Engine.Device().GetWidth(), Engine.Device().GetHeight());

        m_bloomTarget->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        queue.Execute(list);
    }

    Engine.Device().GetQueue(QueueType::ComputeQueue).GPUSynchronize(Engine.Device().GetQueue(QueueType::DirectQueue));

    BloomExtract();
    BloomDownSample();
    BloomUpsample();
}

void SE::Renderer::BloomExtract() const
{
    auto& queue = Engine.Device().GetQueue(QueueType::ComputeQueue);
    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    list.StartMarker("bloom preepass mapping");
    list.BeginRender(m_bloomPrepassPipeline, {});

    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_bloomTarget->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    DefferedLigth textureInfo{};
    textureInfo.readBuffer = m_defferedTarget->GetSRV().GetHandle();
    textureInfo.writeBuffer = m_bloomTarget->GetUAV().GetHandle();

    list.SetConstants("Textures", &textureInfo, sizeof(DefferedLigth), m_upload);

    list.Dispatch(static_cast<uint32_t>(Engine.Device().GetWidth()) / 32u,
                  static_cast<uint32_t>(Engine.Device().GetHeight()) / 32u,
                  1);

    list.UavBarrier(*m_bloomTarget);

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::BloomDownSample() const
{
    auto& queue = Engine.Device().GetQueue(QueueType::ComputeQueue);
    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    list.StartMarker("bloom downsample");
    list.BeginRender(m_bloomDownsamplePipeline, {});

    const auto& desc = m_bloomTarget->GetDesc();

    auto width = desc.widht;
    auto height = desc.height;

    for (auto i = 1u; i < desc.mips; i++)
    {
        m_bloomTarget->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i);
        m_bloomTarget->Transition(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, i - 1);

        BloomData downSample;
        downSample.srv = m_bloomTarget->GetSRV(i - 1).GetHandle();
        downSample.uav = m_bloomTarget->GetUAV(i).GetHandle();
        downSample.sampler = m_bloomSampler;
        downSample.strength = m_bloomThreshold;

        list.SetConstants("DownSample", &downSample, sizeof(BloomData));

        width = width >> 1;
        height = height >> 1;

        list.Dispatch(width / 8u, height / 8u, 1);

        list.UavBarrier(*m_bloomTarget);
    }

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::BloomUpsample() const
{
    auto& queue = Engine.Device().GetQueue(QueueType::ComputeQueue);
    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    list.StartMarker("bloom upsample");
    list.BeginRender(m_bloomUpsamplePipeline, {});

    const auto& desc = m_bloomTarget->GetDesc();

    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto composite = m_defferedTarget->GetUAV().GetHandle();

    for (auto i = desc.mips - 1; i > 0; i--)
    {
        m_bloomTarget->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i - 1);
        m_bloomTarget->Transition(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, i);

        BloomData downSample;
        downSample.srv = m_bloomTarget->GetSRV(i).GetHandle();
        downSample.uav = m_bloomTarget->GetUAV(i - 1).GetHandle();
        downSample.sampler = m_bloomSampler;
        downSample.strength = i == 1 ? m_bloomStrength : 0;

        list.SetConstants("UpSample", &downSample, sizeof(BloomData));
        list.SetConstants("Composite", &composite, sizeof(uint32_t));

        auto width = desc.widht >> (i - 1);
        auto height = desc.height >> (i - 1);

        list.Dispatch(width / 8u, height / 8u, 1);

        list.UavBarrier(*m_bloomTarget);
    }

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::RenderDebug(const D3D12_GPU_VIRTUAL_ADDRESS& invCameraBuff) const
{
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);

    list.StartMarker("debug pass");
    RenderingData directData{};
    directData.defaultTarget = false;
    directData.renderTargets.resize(1);
    directData.renderTargets[0] = m_defferedTarget;
    list.BeginRender(m_debugPipeline, directData);

    const auto& depth = Engine.Device().GetDepthBuffer();
    depth.Transition(list, D3D12_RESOURCE_STATE_DEPTH_READ);
    SetupCommandListForDefferedLights(list, m_quadVerticies, m_quadIndecies, false);
    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);

    CameraData data{};
    for (const auto& [ent, cam, trans] : SE::Engine.ECS().Registry.view<SE::Camera, SE::Transform>().each())
    {
        data.projection = glm::inverse(cam.projection);
        data.view = trans.GetWorld();
        data.eyePosition = trans.GetTranslation();
    }

    list.SetCBV("Camera", invCameraBuff);

    glm::vec2 dim;
    dim.x = static_cast<float>(Engine.Device().GetWidth());
    dim.y = static_cast<float>(Engine.Device().GetHeight());
    list.SetConstants("Screen", &dim[0], sizeof(glm::vec2));

    IBLEnviroment ibl{};
    int envCount = 0;
    for (const auto& [ent, env] : Engine.ECS().Registry.view<Enviroment>().each())
    {
        const auto& sk = env.m_skyox;
        const auto& filterDesc = sk->m_prefilteredMaps->GetDesc();
        ibl.envirometMask = 1;
        ibl.irradianceIndex = sk->m_iradiance->GetSRV().GetHandle();
        ibl.prefilterdIndex = sk->m_prefilteredMaps->GetSRV().GetHandle();
        ibl.envSamplerIndex = m_defaultSampler;
        ibl.envSamplerIndex = m_bufferSampler;
        ibl.filterMips = filterDesc.mips - 1;
        ibl.lutIndex = m_lutTexture->GetSRV().GetHandle();

        envCount++;
        if (envCount > 1)
        {
            Log::Warn("Detecting multiple skyboxes for ibl, last will be eused");
        }
    }

    list.SetConstants("enviroment", &ibl, sizeof(IBLEnviroment), m_upload);
    list.SetConstants("DebugMode", &m_colorMode, sizeof(uint32_t));

    list.DrawCall(m_quadIndecies->GetElementCount());

    m_defferedTarget->Transition(list, D3D12_RESOURCE_STATE_COMMON);

    list.EndMarker();
    queue.Execute(list);
}

void SE::Renderer::SetupCommandListForDefferedLights(CommandList& list, Buffer* vertex, Buffer* index, bool) const
{
    list.SetIndexBuffer(*index);
    list.SetVertexBuffer(*vertex);

    DefferedLigth textureInfo{};
    textureInfo.albodoRougness = m_gbuffers[0]->GetSRV().GetHandle();
    textureInfo.emisiveMetal = m_gbuffers[1]->GetSRV().GetHandle();
    textureInfo.normalAo = m_gbuffers[2]->GetSRV().GetHandle();
    textureInfo.depthBuffer = Engine.Device().GetDepthBuffer().GetSRV().GetHandle();
    textureInfo.writeBuffer = m_bufferSampler;

    list.SetConstants("Textures", &textureInfo, sizeof(DefferedLigth), m_upload);
}

SE::PipelineSettings SE::Renderer::CreateDefferdSettings()
{
    PipelineSettings settings{};
    settings.rtvCount = 1;
    settings.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    settings.topology = Topology::Triangle;

    settings.blend[0].blendEnable = true;
    settings.blend[0].srcBlend = BlendMode::One;
    settings.blend[0].dstBlend = BlendMode::One;
    settings.blend[0].blendOperation = BlendOperation::Add;
    settings.blend[0].srcAlphaBlend = BlendMode::One;
    settings.blend[0].dstAlphaBlend = BlendMode::One;
    settings.blend[0].blendOperation = BlendOperation::Max;
    return settings;
}
