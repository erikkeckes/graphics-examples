#include "rendering/core/texture_dx.hpp"
#include "rendering/core/resource_dx.hpp"
#include "rendering/core/views_dx.hpp"
#include "rendering/core/command_list_dx.hpp"
#include "rendering/core/command_queue_dx.hpp"
#include "rendering/core/upload_buffer.hpp"
#include "rendering/core/pipeline_dx.hpp"
#include "rendering/core/sampler_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"

struct MipDataGeneration
{
    uint32_t srvView;
    uint32_t samplerIndex;
    uint32_t mipCount;
    uint32_t srcMip;
    glm::uvec4 mips;
    glm::vec2 TexelSize;
    uint32_t srgb;
};

uint32_t SE::D3D12Texture::GetMaxMips(uint32_t width, uint32_t height)
{
    // lowest 3 mips are ignored
    return std::max(static_cast<uint32_t>(log2(std::min(width, height)) + 1), 0u);
}

SE::D3D12Texture::D3D12Texture(TextureDesc desc)
{
    m_desc = std::move(desc);
    assert(m_desc.type > 0 && "Texture unknown texture type");
    assert(m_desc.widht > 0 && "Texture width must be greater that 0");
    assert(m_desc.height > 0 && "Texture height must be greater that 0");
    assert(m_desc.flags > 0 && "Texture no views were specified");

    const bool dsv = m_desc.flags & TextureDesc::TextureFlags::DSV;
    const bool rtv = m_desc.flags & TextureDesc::TextureFlags::RTV;

    if (dsv && rtv)
    {
        assert(false && "Textures are not allowed to be depth buffer and render target at same time");
    }

    if ((dsv || rtv) && m_desc.data)
    {
        m_desc.data = nullptr;
        Log::Warn("Depth buffer and render target ignore initial data");
    }

    if (m_desc.mips == Uint32Max)
    {
        // requested max mips
        m_desc.mips = GetMaxMips(m_desc.widht, m_desc.height);
    }
    else if (m_desc.mips > 0)
    {
        m_desc.mips = std::min(m_desc.mips, GetMaxMips(m_desc.widht, m_desc.height));
    }

    if (m_desc.mips > 1)
    {
        // if mip maps are requested uav will be needed
        m_desc.flags |= TextureDesc::TextureFlags::UAV;
    }

    const bool srv = m_desc.flags & TextureDesc::TextureFlags::SRV;
    const bool uav = m_desc.flags & TextureDesc::TextureFlags::UAV;

    m_clearColor = glm::vec4(m_desc.clearColor[0], m_desc.clearColor[1], m_desc.clearColor[2], m_desc.clearColor[3]);

    auto tempDesc = m_desc;

    if (IsSrgbFormat(m_desc.format))
    {
        tempDesc.format = GetTypelessFormat(m_desc.format);
    }
    m_resource = new D3D12Resource(tempDesc);

    // create views
    if (rtv)
    {
        // for now only texture2d as render targets
        D3D12_RENDER_TARGET_VIEW_DESC view{};
        view.Format = m_desc.format;

        if (m_desc.type == TextureDesc::Texture2D)
        {
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            m_rtvs.push_back(new RenderTargetView(view, m_resource));
        }
        else
        {
            // create a render target view for each face in array
            m_rtvs.resize(m_desc.slice);
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            view.Texture2DArray.MipSlice = 0;

            for (uint32_t i = 0; i < m_desc.slice; i++)
            {
                view.Texture2DArray.FirstArraySlice = i;
                view.Texture2DArray.ArraySize = 1;
                m_rtvs[i] = new RenderTargetView(view, m_resource);
            }
        }
    }
    if (dsv)
    {
        assert(m_desc.type == TextureDesc::Texture2D && "Depth buffer can only be a texture 2D");
        D3D12_DEPTH_STENCIL_VIEW_DESC view{};
        view.Format = m_desc.format;
        view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        m_dsv = new DepthStencilView(view, m_resource);
    }

    if (srv)
    {
        m_srvs.resize(m_desc.mips);
        for (uint32_t i = 0; i < m_desc.mips; i++)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC view{};
            view.Format = m_desc.format;
            view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            if (m_desc.type == TextureDesc::Texture2D)
            {
                view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                view.Texture2D.MipLevels = UINT_MAX;
                view.Texture2D.MostDetailedMip = i;

                if (dsv)
                {
                    view.Format = GetRegularForDsv(m_desc.format);
                }
            }
            else
            {
                view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                view.TextureCube.MostDetailedMip = 0;
                view.TextureCube.MipLevels = UINT_MAX;
            }

            m_srvs[i] = new ShaderResourceView(view, m_resource);
        }
    }

    if (uav)
    {
        m_uavs.resize(m_desc.mips);
        for (uint32_t i = 0; i < m_desc.mips; i++)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
            view.Format = GetRegularForSrgb(m_desc.format);
            if (m_desc.type == TextureDesc::Texture2D)
            {
                view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                view.Texture2D.MipSlice = i;
                m_uavs[i] = new UnorderedAccessView(view, m_resource);
            }
            else
            {
                // could support each per mip level
                view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                view.Texture2DArray.ArraySize = m_desc.slice;
                view.Texture2DArray.FirstArraySlice = 0;
                view.Texture2DArray.MipSlice = i;
                m_uavs[i] = new UnorderedAccessView(view, m_resource);
            }
        }
    }

    if (m_desc.data)
    {
        // upload data
        UploadData();
    }
}

SE::D3D12Texture::D3D12Texture(TextureDesc desc, IDXGISwapChain2* swapchain, uint32_t index)
{
    m_desc = std::move(desc);
    assert(m_desc.type == TextureDesc::TextureType::Texture2D && "Render target must be texture2D");
    assert(m_desc.widht > 0 && "Texture width must be greater that 0");
    assert(m_desc.height > 0 && "Texture height must be greater that 0");
    assert(m_desc.flags > 0 && "Texture no views were specified");

    m_clearColor = glm::vec4(m_desc.clearColor[0], m_desc.clearColor[1], m_desc.clearColor[2], m_desc.clearColor[3]);

    // render target from swapchain is allowed to have a rtv, no mip maps, no initial data
    m_desc.data = nullptr;
    m_desc.mips = 1u;
    m_desc.flags = TextureDesc::TextureFlags::RTV;

    ID3D12Resource* resource = nullptr;
    CheckHR(swapchain->GetBuffer(index, IID_PPV_ARGS(&resource)));

    // no need to check for typelles conversions, render targets from swapchain doesn't have allowed uav
    // only rtv is allowed for these render targets
    m_resource = new D3D12Resource(m_desc, resource);

    // create views
    D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};
    viewDesc.Format = m_desc.format;
    viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // for now only allowing an rtv, could allow srv if needed
    m_rtvs.push_back(new RenderTargetView(viewDesc, m_resource));
}

SE::D3D12Texture::~D3D12Texture()
{
    for (auto i = 0; i < static_cast<int>(m_rtvs.size()); i++)
    {
        delete m_rtvs[i];
    }

    for (auto i = 0; i < static_cast<int>(m_uavs.size()); i++)
    {
        delete m_uavs[i];
    }

    for (auto i = 0; i < static_cast<int>(m_srvs.size()); i++)
    {
        delete m_srvs[i];
    }

    delete m_dsv;
    delete m_resource;
}

const SE::ShaderResourceView& SE::D3D12Texture::GetSRV() const
{
    assert(!m_srvs.empty() && "Nonexisting view");
    return *m_srvs[0];
}

const SE::ShaderResourceView& SE::D3D12Texture::GetSRV(int index) const
{
    assert(!m_srvs.empty() && "Nonexisting view");
    assert(static_cast<int>(m_srvs.size()) > index && "Requesting view outside of allowed views");
    return *m_srvs[index];
}

const SE::UnorderedAccessView& SE::D3D12Texture::GetUAV() const
{
    assert(!m_uavs.empty() && "Nonexisting view");
    return *m_uavs[0];
}

const SE::UnorderedAccessView& SE::D3D12Texture::GetUAV(int index) const
{
    assert(!m_uavs.empty() && "Nonexisting view");
    assert(static_cast<int>(m_uavs.size()) > index && "Requesting view outside of allowed views");
    return *m_uavs[index];
}

const SE::RenderTargetView& SE::D3D12Texture::GetRTV() const
{
    assert(!m_rtvs.empty() && "Nonexisting view");
    return *m_rtvs[0];
}

const SE::RenderTargetView& SE::D3D12Texture::GetRTV(int index) const
{
    assert(!m_rtvs.empty() && "Nonexisting view");
    assert(static_cast<int>(m_rtvs.size()) > index && "Requesting view outside of allowed views");
    return *m_rtvs[index];
}

const SE::DepthStencilView& SE::D3D12Texture::GetDSV() const
{
    assert(m_dsv && "Nonexisting view");
    return *m_dsv;
}

void SE::D3D12Texture::Transition(CommandList& list, D3D12_RESOURCE_STATES state, uint32_t subresource) const
{
    m_resource->Transition(list, state, subresource);
}

void SE::D3D12Texture::UploadData()
{
    assert(m_resource);
    if (!m_desc.data)
    {
        return;
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    SE::Engine.Device()
        .GetDevice()
        ->GetCopyableFootprints(&m_resource->GetDesc(), 0, 1, 0, &footprint, nullptr, nullptr, nullptr);

    auto* uploadResource = m_resource->CreateUploadResource();
    uploadResource->Write(m_desc.data, footprint);

    auto& list = Engine.Device().GetCommandList(QueueType::CopyQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::CopyQueue);

    list.CopyTexture(*uploadResource, *this, footprint);

    queue.Execute(list);

    auto& directQueue = Engine.Device().GetQueue(QueueType::DirectQueue);
    if (m_desc.flags & TextureDesc::TextureFlags::Mips && m_desc.mips > 1)
    {
        GenerateMipsTexture2D();
    }
    else
    {
        // mip generation flushes compute queue at end no need to synchronize
        directQueue.GPUSynchronize(queue);
    }

    auto& directList = Engine.Device().GetCommandList(QueueType::DirectQueue);

    m_resource->Transition(directList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    directQueue.Execute(directList);

    // wait for work to finish
    directQueue.Flush();

    delete uploadResource;
}

void SE::D3D12Texture::GenerateMipsTexture2D()
{
    if (m_desc.mips <= 1)
    {
        // no need for mip generation
        return;
    }

    // create pipeline for mipmaps
    ShaderDesc shaderDesc{};
    shaderDesc.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\mipmap_cs.hlsl");

    ComputePipelineDesc pipelineDesc{};
    pipelineDesc.InitComputePipeline(shaderDesc);

    auto mipMapsPipeline = Pipeline::GetPipeline(pipelineDesc);

    // create uavs for mip levels
    // these should be deleted after mip maps are generated
    std::vector<UnorderedAccessView> mipViews;
    mipViews.reserve(m_desc.mips - 1);

    bool isSrgb = IsSrgbFormat(m_desc.format);
    auto uavFormat = GetRegularForSrgb(m_desc.format);  // cannot use srgb formats for uav;

    for (uint32_t i = 1; i < m_desc.mips; i++)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
        desc.Format = uavFormat;
        // may need adjustments for cube maps or have different branch for this
        desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = i;

        mipViews.emplace_back(desc, m_resource);
    }

    // get sampler for mipmap filtering
    D3D12_SAMPLER_DESC desc{};
    desc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;

    desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    desc.MinLOD = 0;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.MipLODBias = 0;
    desc.MaxAnisotropy = 1;

    desc.BorderColor[0] = 0.0f;
    desc.BorderColor[1] = 0.0f;
    desc.BorderColor[2] = 0.0f;
    desc.BorderColor[3] = 0.0f;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;

    auto hash = D3D12Sampler::CreateSampler(desc);
    auto samplerHandle = D3D12Sampler::GetSampler(hash)->GetView().GetHandle();

    // small local upload buffer for this pipeline
    auto upload = UploadBuffer(_64KB);

    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    auto& queue = Engine.Device().GetQueue(QueueType::ComputeQueue);
    queue.GPUSynchronize(Engine.Device().GetQueue(QueueType::CopyQueue));

    list.StartMarker("MipMaps");
    list.BeginRender(mipMapsPipeline, {});

    m_resource->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    for (uint32_t i = 1; i <= static_cast<uint32_t>(mipViews.size()); i += 4)
    {
        MipDataGeneration data{};
        data.srvView = m_srvs[0]->GetHandle();

        // source mip for sampling
        // generated mip are resused too
        data.srcMip = i - 1;
        data.samplerIndex = samplerHandle;

        m_resource->Transition(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, data.srcMip);

        if (isSrgb)
        {
            data.srgb = 1;
        }
        else
        {
            data.srgb = 0;
        }

        // how many mip maps should be calculated
        auto generatedMips = std::min((m_desc.mips - i), 4u);

        int index = 0;
        data.mipCount = generatedMips;
        // set proper uav handles per each mip map
        for (uint32_t j = i; j < i + generatedMips; j++)
        {
            data.mips[index] = mipViews[j - 1].GetHandle();
            index++;
        }

        auto dstWidth = m_desc.widht >> i;
        auto dstHeight = m_desc.height >> i;
        // texel size, needed for getting proper UV for mip map sampling
        data.TexelSize.x = 1.f / static_cast<float>(m_desc.widht >> i);
        data.TexelSize.y = 1.f / static_cast<float>(m_desc.height >> i);

        list.SetConstants("MipData", &data, sizeof(MipDataGeneration), &upload);
        list.Dispatch(dstWidth / 8, dstHeight / 8, 1);
        m_resource->UavBarier(list);
    }

    list.EndMarker();
    queue.Execute(list);
    queue.Flush();
}

// should remae to cubemap, or just leave the shader name to be texture array
void SE::D3D12Texture::GenerateMipsTexture2DArray()
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
    vs.InitAsComputeShader(FileIO::Directory::SharedAssets, "shaders\\dx12\\mipmap_cubemap_cs.hlsl");

    SE::ComputePipelineDesc pipelineDesc{};
    pipelineDesc.InitComputePipeline(vs);

    const auto pipeline = Pipeline::GetPipeline(pipelineDesc);

    auto& list = Engine.Device().GetCommandList(QueueType::ComputeQueue);
    auto& compute = Engine.Device().GetQueue(QueueType::ComputeQueue);

    const auto mips = m_desc.mips;
    auto width = m_desc.widht;
    auto height = m_desc.height;

    list.StartMarker("Texture Array mips");
    list.BeginRender(pipeline, {});

    glm::uvec2 env = {GetSRV().GetHandle(), samplerHandle};
    list.SetConstants("Source", &env[0], sizeof(glm::uvec2));

    for (uint32_t i = 1; i < mips; i++)
    {
        auto handle = GetUAV(i).GetHandle();
        list.SetConstants("Target", &handle, sizeof(uint32_t));

        width /= 2;
        height /= 2;

        list.Dispatch(width / 32, height / 32, 6);
        m_resource->UavBarier(list);
    }
    list.EndMarker();
    compute.Execute(list);
    compute.Flush();
}