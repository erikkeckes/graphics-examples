#include "rendering/core/command_list_dx.hpp"
#include "rendering/core/device_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/texture_dx.hpp"
#include "rendering/core/buffer_dx.hpp"
#include "rendering/core/resource_dx.hpp"
#include "rendering/core/views_dx.hpp"
#include "rendering/core/pipeline_dx.hpp"
#include "rendering/core/shader_dx.hpp"
#include "rendering/core/pipeline_state_dx.hpp"
#include "rendering/core/root_signature_dx.hpp"
#include "rendering/core/upload_buffer.hpp"

SE::CommandList::CommandList(Device* device, D3D12_COMMAND_LIST_TYPE type) : m_type(type)
{
    CheckHR(device->GetDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator)));
    CheckHR(device->GetDevice()->CreateCommandList(0, type, m_allocator, nullptr, IID_PPV_ARGS(&m_list)));

    switch (type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            m_list->SetName(L"Graphics");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            m_list->SetName(L"Compute");
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            m_list->SetName(L"Copy");
            break;
        default:
            break;
    }
}

SE::CommandList::~CommandList()
{
    COMFREE(m_allocator);
    COMFREE(m_list);
}

ID3D12GraphicsCommandList* SE::CommandList::List() const { return m_list; }

void SE::CommandList::Reset()
{
    if (!m_cmdListClosed)
    {
        Close();
    }
    // reseting param ptr
    m_params = nullptr;

    // reset command allocator and list so it can be reused for new commands
    m_allocator->Reset();
    m_list->Reset(m_allocator, nullptr);

    m_cmdListClosed = false;
}

void SE::CommandList::Close()
{
    if (m_cmdListClosed)
    {
        return;
    }
    m_list->Close();
    m_cmdListClosed = true;
}

void SE::CommandList::StartMarker(const std::string& name)
{
    m_list->BeginEvent(1u, name.c_str(), static_cast<UINT>(name.size() + 1));
}

void SE::CommandList::EndMarker() { m_list->EndEvent(); }

void SE::CommandList::SetIndexBuffer(const Buffer& buffer)
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    buffer.m_resource->Transition(*this, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    const auto view = buffer.GetIndexView().GetView();
    m_list->IASetIndexBuffer(&view);
}

void SE::CommandList::SetVertexBuffer(const Buffer& buffer)
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    buffer.m_resource->Transition(*this, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    const auto view = buffer.GetVertexView().GetView();
    m_list->IASetVertexBuffers(0, 1, &view);
}

void SE::CommandList::Present(const D3D12Texture& renderTarget)
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    renderTarget.m_resource->Transition(*this, D3D12_RESOURCE_STATE_PRESENT);
}

void SE::CommandList::ClearScreen(const D3D12Texture& renderTarget, int right, int bottom, int left, int top)
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    const auto rect = CD3DX12_RECT(top, left, right, bottom);
    renderTarget.m_resource->Transition(*this, D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto handle = renderTarget.GetRTV().GetCpuHandle();
    auto color = renderTarget.GetClearColor();
    m_list->ClearRenderTargetView(handle, &color[0], 1, &rect);
}

void SE::CommandList::ClearScreen(const D3D12Texture& renderTarget)
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    const auto& rect = Engine.Device().GetRect();
    renderTarget.m_resource->Transition(*this, D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto handle = renderTarget.GetRTV().GetCpuHandle();
    auto color = renderTarget.GetClearColor();
    m_list->ClearRenderTargetView(handle, &color[0], 1, &rect);
}

void SE::CommandList::ClearDepthStnecil(const D3D12Texture& depthStenil, int right, int bottom, int left, int top)
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    const auto rect = CD3DX12_RECT(top, left, right, bottom);
    auto handle = depthStenil.GetDSV().GetCpuHandle();
    auto flags = D3D12_CLEAR_FLAG_STENCIL | D3D12_CLEAR_FLAG_DEPTH;
    depthStenil.m_resource->Transition(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_list->ClearDepthStencilView(handle, flags, 1, 0, 1, &rect);
}

void SE::CommandList::ClearDepthStnecil(const D3D12Texture& depthStenil)
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    const auto& rect = Engine.Device().GetRect();
    auto handle = depthStenil.GetDSV().GetCpuHandle();
    auto flags = D3D12_CLEAR_FLAG_STENCIL | D3D12_CLEAR_FLAG_DEPTH;
    depthStenil.m_resource->Transition(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_list->ClearDepthStencilView(handle, flags, 1, 0, 1, &rect);
}

void SE::CommandList::SetMainRenderTarget()
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    SetRectViewportFullScreen();

    const auto& dsv = Engine.Device().GetDepthBuffer().GetDSV().GetCpuHandle();
    const auto& rtv = Engine.Device().GetBackBuffer().GetRTV().GetCpuHandle();
    Engine.Device().GetDepthBuffer().m_resource->Transition(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void SE::CommandList::SubmitResourceTransitions(uint32_t count, const D3D12_RESOURCE_BARRIER* bariers)
{
    m_list->ResourceBarrier(count, bariers);
}

void SE::CommandList::SetImGuiRendering()
{
    if (m_type != D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        return;
    }

    SetRectViewportFullScreen();

    const auto& rtv = Engine.Device().GetBackBuffer().GetRTV().GetCpuHandle();
    m_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void SE::CommandList::BeginRender(const std::shared_ptr<Pipeline>& pipeline, const RenderingData& data)
{
    SetPipeline(pipeline);

    if (!pipeline->m_compute)
    {
        auto depth = pipeline->m_settings.depthStencil.depthEnabled;
        auto rendering = pipeline->m_settings.rtvCount > 0;
#ifdef DEBUG
        if (depth && !data.defaultDepth && !data.depthBuffer)
        {
            depth = false;
            Log::Warn("Pipeline uses depth buffer but it's missing");
            Log::Warn("No depth buffer will be set");
        }

        bool sameSize = pipeline->m_settings.rtvCount == static_cast<uint32_t>(data.renderTargets.size());
        if (rendering && !data.defaultTarget && (data.renderTargets.empty() || !sameSize))
        {
            if (data.renderTargets.empty())
            {
                Log::Warn("No custom render targets provided");
                Log::Warn("No render target will be set");
                rendering = false;
            }
            else if (!sameSize)
            {
                Log::Warn("Mismatch of render target count");
                Log::Warn("No render target will be set");
                rendering = false;
            }
        }
#endif
        if (data.fullscreen)
        {
            SetRectViewportFullScreen();
        }
        else
        {
            SetRectViewport(data.right, data.bottom, data.left, data.top);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsv;
        if (depth)
        {
            if (data.defaultDepth)
            {
                dsv = Engine.Device().GetDepthBuffer().GetDSV().GetCpuHandle();
            }
            else
            {
                dsv = data.depthBuffer->GetDSV().GetCpuHandle();
            }
        }

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
        if (rendering)
        {
            rtvs.resize(pipeline->m_settings.rtvCount);
            if (data.defaultTarget)
            {
                // only one render target
                rtvs[0] = Engine.Device().GetBackBuffer().GetRTV().GetCpuHandle();
            }
            else
            {
                for (auto i = 0u; i < pipeline->m_settings.rtvCount; i++)
                {
                    rtvs[i] = data.renderTargets[i]->GetRTV().GetCpuHandle();
                }
            }
        }

        if (rendering || depth)
        {
            if (depth)
            {
                m_list->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.data(), FALSE, &dsv);
            }
            else
            {
                m_list->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.data(), FALSE, nullptr);
            }
        }
        else
        {
            Log::Warn("No render target or depth buffer was set for graphics pipeline");
            Log::Warn("Check RenderingData struct for pipeline");
        }
    }
}

void SE::CommandList::SetPipeline(const std::shared_ptr<Pipeline>& pipeline)
{
    if (m_type == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        m_list->SetGraphicsRootSignature(pipeline->m_signature->m_signature);
        m_list->IASetPrimitiveTopology(GetTopo(pipeline->m_settings.topology));
    }
    else if (m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        m_list->SetComputeRootSignature(pipeline->m_signature->m_signature);
    }

    m_list->SetPipelineState(pipeline->m_pso->m_pso);

    // take shader params
    m_params = pipeline->m_shaderBinds;
}

void SE::CommandList::UavBarrier(const D3D12Texture& texture) { texture.m_resource->UavBarier(*this); }

void SE::CommandList::CopyTexture(const D3D12Texture& src, const D3D12Texture& dst)
{
    const auto& desc2 = dst.GetDesc();
#ifdef DEBUG
    const auto& desc1 = src.GetDesc();

    if (desc1.widht != desc2.widht || desc1.height != desc2.height || desc1.format != desc2.format)
    {
        Log::Error("Copy of different textures");
        Log::Error("Incombatible width {} height {} format {}",
                   desc1.widht != desc2.widht,
                   desc1.height != desc2.height,
                   desc1.format != desc2.format);
        assert(false);
    }

#endif

    dst.Transition(*this, D3D12_RESOURCE_STATE_COPY_DEST);
    src.Transition(*this, D3D12_RESOURCE_STATE_COPY_SOURCE);

    D3D12_BOX srcBox = {};
    srcBox.left = 0;
    srcBox.top = 0;
    srcBox.front = 0;
    srcBox.right = desc2.widht;
    srcBox.bottom = desc2.height;
    srcBox.back = 1;

    D3D12_TEXTURE_COPY_LOCATION destLocation = {};
    destLocation.pResource = dst.m_resource->GetResource();
    destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = src.m_resource->GetResource();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    m_list->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, &srcBox);
}

void SE::CommandList::CopyTexture(const D3D12Resource& src,
                                   const D3D12Texture& dst,
                                   const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint)
{
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = src.GetResource();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = dst.m_resource->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    dst.m_resource->Transition(*this, D3D12_RESOURCE_STATE_COPY_DEST);
    m_list->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void SE::CommandList::SetCBV(const Hash& name, const D3D12_GPU_VIRTUAL_ADDRESS& address)
{
#ifdef DEBUG
    const auto map = m_params->GetNameMap();
    if (map.find(name.Value()) == map.end())
    {
        Log::Error("Current pipeline doesn't include this param");
        Log::Error("Could be a typo, no usage in shader or wrong pipeline");
        Log::Error("Current pipeline params:");
        for (const auto& param : m_params->GetShaderParams())
        {
            Log::Error("    param name {}", param.nameHash.GetName());
        }
        assert(false);
    }
#endif
    auto slot = m_params->GetNameMap().at(name.Value());
    if (m_type == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        m_list->SetGraphicsRootConstantBufferView(slot, address);
    }
    else if (m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        m_list->SetComputeRootConstantBufferView(slot, address);
    }
}

void SE::CommandList::SetConstants(const Hash& name, const void* data, size_t size, UploadBuffer* upload)
{
    assert(data && "Passing invalid data for constants values");
    assert(size > 0 && "Constant values size is zero, specific correct size");

    if (size > MaxConstantSize)
    {
        assert(upload && "Cannot upload CBV without proper upload buffer");
        auto alloc = upload->AllocateSpecific(size);
        memcpy(alloc.CPU, data, size);
        SetCBV(name, alloc.GPU);
    }
    else
    {
        SetRootConstants(name, reinterpret_cast<const uint32_t*>(data), size);
    }
}

void SE::CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    if (m_type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        m_list->Dispatch(std::max(static_cast<UINT>(x), 1u),
                         std::max(static_cast<UINT>(y), 1u),
                         std::max(static_cast<UINT>(z), 1u));
    }
}

void SE::CommandList::DrawCall(uint32_t indexCount,
                                uint32_t instances,
                                uint32_t indexOffset,
                                uint32_t vertexOffset,
                                uint32_t instanceOffset)
{
    m_list->DrawIndexedInstanced(indexCount, instances, indexOffset, vertexOffset, instanceOffset);
}

void SE::CommandList::SetRectViewportFullScreen()
{
    const auto& rect = Engine.Device().GetRect();
    const auto& viewport = Engine.Device().GetViewPort();
    m_list->RSSetScissorRects(1, &rect);
    m_list->RSSetViewports(1, &viewport);
}

void SE::CommandList::SetRectViewport(int right, int bottom, int left, int top)
{
    SetRect(right, bottom, left, top);
    SetViewport(right, bottom, left, top);
}

void SE::CommandList::SetRect(int right, int bottom, int left, int top)
{
    const auto rect = CD3DX12_RECT(top, left, right, bottom);
    m_list->RSSetScissorRects(1, &rect);
}

void SE::CommandList::SetViewport(int right, int bottom, int left, int top)
{
    const auto viewport = CD3DX12_VIEWPORT(static_cast<FLOAT>(left),
                                           static_cast<FLOAT>(top),
                                           static_cast<FLOAT>(right),
                                           static_cast<FLOAT>(bottom));
    m_list->RSSetViewports(1, &viewport);
}

void SE::CommandList::SetRootConstants(const Hash& name, const uint32_t* values, size_t size)
{
    const UINT paramSize = static_cast<UINT>(size) / sizeof(UINT);
    const auto& map = m_params->GetNameMap();
    auto slot = map.at(name.Value());
#ifdef DEBUG
    if (map.find(name.Value()) == map.end())
    {
        Log::Error("Current pipeline doesn't include this param");
        Log::Error("Could be a typo, no usage in shader or wrong pipeline");
        Log::Error("Current pipeline params:");
        for (const auto& param : m_params->GetShaderParams())
        {
            Log::Error("    param name {}", param.nameHash.GetName());
        }
        assert(false);
    }

    auto paramIndex = m_params->GetParamMap().at(name.Value());
    auto param = m_params->GetShaderParams().at(paramIndex);
    // Num32BitValues can be bigger, 1 uint in shader CBV will end up with a size of 16
    if (paramSize > param.param.Constants.Num32BitValues)
    {
        Log::Error("Setting root constants with wrong size");
        Log::Error("Param name {}", param.nameHash.GetName());
        Log::Error("Root constants 32 bit value count {}, setting {}", param.param.Constants.Num32BitValues, paramSize);
        assert(false);
    }
#endif

    if (m_type == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        m_list->SetGraphicsRoot32BitConstants(slot, paramSize, values, 0);
    }
    else if (m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        m_list->SetComputeRoot32BitConstants(slot, paramSize, values, 0);
    }
}
