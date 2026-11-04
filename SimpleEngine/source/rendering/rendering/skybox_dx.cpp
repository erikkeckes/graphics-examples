#include "resources/skybox.hpp"
#include "rendering/common_dx.hpp"
#include "rendering/core/texture_dx.hpp"
#include "rendering/core/pipeline_dx.hpp"
#include "rendering/core/command_list_dx.hpp"
#include "rendering/core/command_queue_dx.hpp"
#include "rendering/core/buffer_dx.hpp"
#include "rendering/core/views_dx.hpp"
#include "rendering/core/upload_buffer.hpp"
#include "rendering/core/sampler_dx.hpp"
#include "rendering/rendering/shader_struct.hpp"
#include "rendering/core/resource_dx.hpp"
#include "tinygltf/stb_image.h"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"
#include "core/fileio.hpp"
#include "glm/glm.hpp"
#include <glm/gtx/quaternion.hpp>

// needs to find way how to reuse with renderer
glm::vec3 CubeMapPositions[] = {glm::vec3(1, 1, 1),   glm::vec3(1, 1, 1),    glm::vec3(1, 1, 1),    glm::vec3(1, 1, -1),
                                glm::vec3(1, 1, -1),  glm::vec3(1, 1, -1),   glm::vec3(1, -1, 1),   glm::vec3(1, -1, 1),
                                glm::vec3(1, -1, 1),  glm::vec3(1, -1, -1),  glm::vec3(1, -1, -1),  glm::vec3(1, -1, -1),
                                glm::vec3(-1, 1, 1),  glm::vec3(-1, 1, 1),   glm::vec3(-1, 1, 1),   glm::vec3(-1, 1, -1),
                                glm::vec3(-1, 1, -1), glm::vec3(-1, 1, -1),  glm::vec3(-1, -1, 1),  glm::vec3(-1, -1, 1),
                                glm::vec3(-1, -1, 1), glm::vec3(-1, -1, -1), glm::vec3(-1, -1, -1), glm::vec3(-1, -1, -1)};

uint16_t CubeMapIndicies[] = {0,  13, 20, 0,  20, 7,  9, 6, 19, 9, 19, 22, 21, 18, 12, 21, 12, 15,
                              16, 3,  10, 16, 10, 23, 5, 2, 8,  5, 8,  11, 17, 14, 1,  17, 1,  4};

constexpr uint32_t IrradianceSize = 32u;
constexpr uint32_t RadianceSize = 512u;
constexpr uint32_t SkyboxSize = 512u;

inline SE::D3D12Texture* CreateHdrTexture(SE::FileIO::Directory dir, const std::string& path)
{
    SE::D3D12Texture* temp = nullptr;

    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, channells = 0;
    auto* data = stbi_loadf(SE::Engine.FileIO().GetPath(dir, path).c_str(), &width, &height, &channells, 4);
    stbi_set_flip_vertically_on_load(false);

    if (data)
    {
        SE::TextureDesc desc{};
        // this texture is used to generate cubemap
        desc.InitAsTexture2D(width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, data).SetMips(1).SetName(path);

        temp = new SE::D3D12Texture(desc);
        stbi_image_free(data);
    }
    else
    {
        SE::Log::Error("Failed to load external texture path : {}", path);
    }

    return temp;
}

SE::Skybox::Skybox(FileIO::Directory dir, const std::string& path) : Resource(ResourceType::Image)
{
    const auto* hdrTexture = CreateHdrTexture(dir, path);

    assert(hdrTexture);

    TextureDesc desc{};
    desc.InitAsCubeMap(SkyboxSize, SkyboxSize, DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetName(path + " enviromnt")
        .AllowUAV()
        // do need all the mips
        .SetMips();

    m_skybox = std::make_unique<D3D12Texture>(desc);

    TextureDesc filterDesc{};
    filterDesc.InitAsCubeMap(RadianceSize, RadianceSize, DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetName(path + " prefiltered")
        .AllowUAV()
        .SetMips();

    m_prefilteredMaps = std::make_unique<D3D12Texture>(filterDesc);

    TextureDesc irDesc{};
    irDesc.InitAsCubeMap(IrradianceSize, IrradianceSize, DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetName(path + " iradiance")
        .SetMips(1)
        .AllowUAV();

    m_iradiance = std::make_unique<D3D12Texture>(irDesc);

    auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);
    auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);

    m_iradiance->m_resource->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_prefilteredMaps->m_resource->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_skybox->m_resource->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    queue.Execute(list);
    queue.Flush();

    ConvertHdr(*hdrTexture);
    delete hdrTexture;

    GenerateIrradiance();
    GenerateFilteredMaps();
    // add transitions to pixel resource states for booth resources

    auto& lastlist = Engine.Device().GetCommandList(QueueType::DirectQueue);

    m_iradiance->m_resource->Transition(lastlist, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_prefilteredMaps->m_resource->Transition(lastlist, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    queue.Execute(lastlist);
}

void SE::Skybox::ConvertHdr(const D3D12Texture& hdrTexture)
{
    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_ANISOTROPIC;

    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    sampler.MinLOD = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 16;

    sampler.BorderColor[0] = 0.0f;
    sampler.BorderColor[1] = 0.0f;
    sampler.BorderColor[2] = 0.0f;
    sampler.BorderColor[3] = 0.0f;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    const auto samplerHash = D3D12Sampler::CreateSampler(sampler);
    const auto samplerPtr = D3D12Sampler::GetSampler(samplerHash);
    const auto samplerIndex = samplerPtr->GetView().GetHandle();

    ConvertHdrCompute(hdrTexture, samplerIndex);

    m_skybox->GenerateMipsTexture2DArray();
}

// void SE::Skybox::ConvertHdrGraphics(const D3D12Texture& hdrTexture, uint32_t samplerIndex)
//{
//     BufferDes vertex{};
//     BufferDes index{};
//
//     vertex.InitAsVertexBuffer(sizeof(glm::vec3), 24, CubeMapPositions);
//     index.InitAsIndexBuffer(sizeof(uint16_t), 36, CubeMapIndicies);
//
//     const auto vertexBuff = Buffer(vertex);
//     const auto indexBuff = Buffer(index);
//
//     auto upload = UploadBuffer(_KB(1));
//
//     const auto hdrDesc = hdrTexture.GetDesc();
//
//     SE::ShaderDesc vs{};
//     vs.InitAsVertexShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\skybox_vs.hlsl");
//
//     SE::ShaderDesc ps{};
//     ps.InitAsPixelShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\equirectangular_to_cubemap_ps.hlsl");
//
//     PipelineSettings settings{};
//     settings.topology = Topology::Triangle;
//     settings.depthStencil.depthEnabled = false;
//     settings.depthStencil.depthComparison = ComparisonFunction::Never;
//     settings.depthStencil.stencilWriteMask = 0xFF;
//
//     settings.raster.culling = CullMode::None;
//
//     settings.rtvCount = 1;
//     settings.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
//
//     SE::GraphicsPipelineDesc pipelineDesc{};
//     pipelineDesc.SetVertexShader(vs).SetPixelShader(ps).SetSettigns(settings);
//
//     const auto pipeline = Pipeline::GetPipeline(pipelineDesc);
//
//     auto& queue = Engine.Device().GetQueue(QueueType::DirectQueue);
//     auto& list = Engine.Device().GetCommandList(QueueType::DirectQueue);
//
//     list.StartMarker("Converting HDR Graphics");
//     m_skybox->m_resource->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);
//
//
//     list.BeginRender(pipeline);
//
//     const auto hdr = hdrTexture.GetSRV().GetHandle();
//     list.SetConstants("texture", &hdr, sizeof(uint32_t));
//     list.SetConstants("sampelr", &samplerIndex, sizeof(uint32_t));
//
//     list.SetRect(hdrDesc.height, hdrDesc.height);
//     list.SetViewport(hdrDesc.height, hdrDesc.height);
//
//     list.SetVertexBuffer(vertexBuff);
//     list.SetIndexBuffer(indexBuff);
//
//     const auto perspective = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 10.0f);
//
//     glm::mat4 views[] = {glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
//                          glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,
//                          0.0f)),
//
//                          glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f,
//                          -1.0f)), glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f,
//                          0.0f, 1.0f)), glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f,
//                          -1.0f, 0.0f)), glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
//                          glm::vec3(0.0f, -1.0f, 0.0f))};
//
//     for (int i = 0; i < 6; i++)
//     {
//         auto cam = perspective * views[i];
//         list.SetCBV("Camera", upload.Allocate(cam));
//
//         auto view = m_skybox->GetRTV(i).GetCpuHandle();
//         list.List()->OMSetRenderTargets(1, &view, FALSE, nullptr);
//
//         list.DrawCall(36);
//     }
//
//     m_skybox->m_resource->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
//     list.EndMarker();
//     queue.Execute(list);
//     queue.Flush();  // need to wait due to upload buffer being linked
// }

void SE::Skybox::ConvertHdrCompute(const D3D12Texture& hdrTexture, uint32_t samplerIndex)
{
    ShaderDesc cs{};
    cs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\hdr_tocubemap_cs.hlsl");

    ComputePipelineDesc pipelineDesc{};
    pipelineDesc.InitComputePipeline(cs);

    const auto pipeline = Pipeline::GetPipeline(pipelineDesc);

    const auto hdr = hdrTexture.GetSRV().GetHandle();
    const auto skybox = m_skybox->GetUAV(0).GetHandle();

    auto& queue = Engine.Device().GetQueue(QueueType::ComputeQueue);
    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);

    list.StartMarker("Converting HDR Compute");
    list.BeginRender(pipeline, {});

    glm::uvec3 data = glm::uvec3(hdr, skybox, samplerIndex);

    list.SetConstants("MapData", &data[0], sizeof(glm::uvec3));
    list.Dispatch(SkyboxSize / 32, SkyboxSize / 32, 6);
    m_skybox->m_resource->UavBarier(list);
    list.EndMarker();
    queue.Execute(list);
}

void SE::Skybox::GenerateIrradiance()
{
    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    sampler.MinLOD = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 1;

    sampler.BorderColor[0] = 0.0f;
    sampler.BorderColor[1] = 0.0f;
    sampler.BorderColor[2] = 0.0f;
    sampler.BorderColor[3] = 0.0f;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    const auto hash = D3D12Sampler::CreateSampler(sampler);
    const auto samplerHandle = D3D12Sampler::GetSampler(hash)->GetView().GetHandle();

    SE::ShaderDesc vs{};
    vs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\ibl_iradiance_cs.hlsl");

    SE::ComputePipelineDesc pipelineDesc{};
    pipelineDesc.InitComputePipeline(vs);

    const auto pipeline = Pipeline::GetPipeline(pipelineDesc);

    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    auto& compute = Engine.Device().GetQueue(QueueType::ComputeQueue);

    auto& direct = Engine.Device().GetQueue(QueueType::DirectQueue);

    compute.GPUSynchronize(direct);

    list.StartMarker("Irradiance");
    list.BeginRender(pipeline, {});

    glm::uvec2 env = {m_skybox->GetSRV().GetHandle(), samplerHandle};
    auto handle = m_iradiance->GetUAV().GetHandle();

    list.SetConstants("Enviroment", &env[0], sizeof(glm::uvec2));
    list.SetConstants("IradianceMap", &handle, sizeof(uint32_t));
    const auto sourceWidth = m_skybox->GetDesc().widht;
    list.SetConstants("SamplingData", &sourceWidth, sizeof(uint32_t));

    list.Dispatch(IrradianceSize / 32, IrradianceSize / 32, 6);

    list.EndMarker();
    compute.Execute(list);
    compute.Flush();
}

void SE::Skybox::GenerateFilteredMaps()
{
    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    sampler.MinLOD = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 1;

    sampler.BorderColor[0] = 0.0f;
    sampler.BorderColor[1] = 0.0f;
    sampler.BorderColor[2] = 0.0f;
    sampler.BorderColor[3] = 0.0f;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    const auto hash = D3D12Sampler::CreateSampler(sampler);
    const auto samplerHandle = D3D12Sampler::GetSampler(hash)->GetView().GetHandle();

    SE::ShaderDesc vs{};
    vs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\ibl_prefiltered_cs.hlsl");

    SE::ComputePipelineDesc pipelineDesc{};
    pipelineDesc.InitComputePipeline(vs);

    const auto pipeline = Pipeline::GetPipeline(pipelineDesc);

    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    auto& compute = Engine.Device().GetQueue(QueueType::ComputeQueue);

    const auto desc = m_prefilteredMaps->GetDesc();
    const auto mips = desc.mips;
    auto width = RadianceSize;
    auto height = RadianceSize;

    list.StartMarker("Enviroment filtering");
    list.BeginRender(pipeline, {});

    // can just take width, using same size for width and height
    const auto sourceWidth = m_skybox->GetDesc().widht;
    const auto env = m_skybox->GetSRV().GetHandle();
    const auto roughnessStep = 1.f / static_cast<float>(mips - 1);
    float r = 0;

    list.SetConstants("SamplingData", &sourceWidth, sizeof(uint32_t));
    for (uint32_t i = 0; i < mips; i++)
    {
        const auto handle = m_prefilteredMaps->GetUAV(i).GetHandle();

        auto settings = glm::uvec3(env, handle, samplerHandle);
        list.SetConstants("PrefilterSettings", &settings[0], sizeof(glm::uvec3));
        list.SetConstants("RoughnessData", &r, sizeof(float));

        list.Dispatch(width / 32, height / 32, 6);
        m_prefilteredMaps->m_resource->UavBarier(list);
        r += roughnessStep;
        width /= 2;
        height /= 2;
    }
    list.EndMarker();
    compute.Execute(list);
    Engine.Device().GetQueue(QueueType::DirectQueue).GPUSynchronize(compute);
}
