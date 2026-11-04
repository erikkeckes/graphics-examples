#include "rendering/core/resource_dx.hpp"
#include "rendering/core/device_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/command_list_dx.hpp"

SE::D3D12Resource::~D3D12Resource() { COMFREE(m_resource); }

void SE::D3D12Resource::Transition(CommandList& list, D3D12_RESOURCE_STATES state, uint32_t subresource)
{
    if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        auto count = 0;
        bool diffTransitions = false;
        const auto stateCount = static_cast<int>(m_states.size());
        for (auto i = 0; i < stateCount; i++)
        {
            if (m_states[i] != state)
            {
                count++;
                if (diffTransitions)
                {
                    // check for different transition sonly if needed
                    continue;
                }
                for (auto j = 0; j < stateCount; j++)
                {
                    if (j == i)
                    {
                        continue;
                    }
                    diffTransitions |= m_states[i] != m_states[j];
                    if (diffTransitions)
                    {
                        break;
                    }
                }
            }
        }

        if (count == 0)
        {
            return;
        }

        if (!diffTransitions && count == stateCount)
        {
            D3D12_RESOURCE_BARRIER transition =
                CD3DX12_RESOURCE_BARRIER::Transition(m_resource, m_states[0], state, subresource);
            list.SubmitResourceTransitions(1, &transition);
            SetState(state);
        }
        else
        {
            std::vector<D3D12_RESOURCE_BARRIER> bariers;
            bariers.resize(count);
            auto index = 0;
            for (auto i = 0; i < stateCount; i++)
            {
                if (m_states[i] == state)
                {
                    continue;
                }

                bariers[index] = CD3DX12_RESOURCE_BARRIER::Transition(m_resource, m_states[i], state, i);
                m_states[i] = state;
                index++;
            }
            list.SubmitResourceTransitions(count, bariers.data());
        }
    }
    else if (subresource != D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && m_states[subresource] != state)
    {
        D3D12_RESOURCE_BARRIER transition =
            CD3DX12_RESOURCE_BARRIER::Transition(m_resource, m_states[subresource], state, subresource);
        list.SubmitResourceTransitions(1, &transition);
        m_states[subresource] = state;
    }
}

void SE::D3D12Resource::UavBarier(CommandList& list)
{
    D3D12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::UAV(m_resource);
    list.SubmitResourceTransitions(1, &transition);
}

ID3D12Resource* SE::D3D12Resource::GetResource() const { return m_resource; }

D3D12_GPU_VIRTUAL_ADDRESS SE::D3D12Resource::GetGPUAdrress() const
{
    assert(m_resource && "Accessing gpu adrress of invlaid resource");
    return m_resource->GetGPUVirtualAddress();
}

SE::D3D12Resource* SE::D3D12Resource::CreateUploadResource() const
{
    UINT64 totalSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    Engine.Device().GetDevice()->GetCopyableFootprints(&m_desc, 0, 1, 0, &footprint, nullptr, nullptr, &totalSize);
    D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(totalSize);

    auto* uploadResource = new D3D12Resource();

    auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CheckHR(Engine.Device().GetDevice()->CreateCommittedResource(&props,
                                                                 D3D12_HEAP_FLAG_NONE,
                                                                 &uploadDesc,
                                                                 D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                                 nullptr,
                                                                 IID_PPV_ARGS(&uploadResource->m_resource)));

    uploadResource->SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    uploadResource->m_resource->SetName(StringToWString("Upload resource").c_str());

    return uploadResource;
}

void SE::D3D12Resource::Write(const void* data, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint)
{
    assert(data);

    // for buffer this will crash application
    auto formatSize = GetFormatSize(footprint.Footprint.Format);

    uint8_t* mappedData = nullptr;
    m_resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));

    for (uint32_t row = 0; row < footprint.Footprint.Height; ++row)
    {
        memcpy(mappedData + row * footprint.Footprint.RowPitch,
               static_cast<const uint8_t*>(data) + row * (footprint.Footprint.Width * formatSize),
               footprint.Footprint.Width * formatSize);
    }

    m_resource->Unmap(0, nullptr);
}

void SE::D3D12Resource::SetState(D3D12_RESOURCE_STATES state)
{
    for (auto i = 0; i < static_cast<int>(m_states.size()); i++)
    {
        m_states[i] = state;
    }
}

SE::D3D12Resource::D3D12Resource(const BufferDes& desc)
{
    auto size = desc.elementCount * desc.elementSize;
    m_desc = CD3DX12_RESOURCE_DESC::Buffer(size);

    auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    if (desc.access & BufferDes::BufferFlags::Write)
    {
        props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    }

    if (desc.access & BufferDes::BufferFlags::UavAccess)
    {
        // allow uav
        m_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    CheckHR(Engine.Device().GetDevice()->CreateCommittedResource(&props,
                                                                 D3D12_HEAP_FLAG_NONE,
                                                                 &m_desc,
                                                                 D3D12_RESOURCE_STATE_COMMON,
                                                                 nullptr,
                                                                 IID_PPV_ARGS(&m_resource)));
    m_states.resize(1);
    SetState(D3D12_RESOURCE_STATE_COMMON);

    m_resource->SetName(StringToWString(desc.debugName).c_str());
}

SE::D3D12Resource::D3D12Resource(const TextureDesc& desc)
{
    switch (desc.type)
    {
        case TextureDesc::TextureType::Texture2D:
        {
            m_desc = CD3DX12_RESOURCE_DESC::Tex2D(desc.format,
                                                  static_cast<UINT64>(desc.widht),
                                                  static_cast<UINT>(desc.height),
                                                  1u,
                                                  static_cast<UINT16>(desc.mips));
        }
        break;
        case TextureDesc::TextureType::CubeMap:
        {
            // cubemap has slice of 6
            m_desc = CD3DX12_RESOURCE_DESC::Tex2D(desc.format,
                                                  static_cast<UINT64>(desc.widht),
                                                  static_cast<UINT>(desc.height),
                                                  6u,
                                                  static_cast<UINT16>(desc.mips));
        }
        break;
            // case TextureDesc::TextureType::TextureArray:
            //{
            //     m_desc = CD3DX12_RESOURCE_DESC::Tex2D(desc.format,
            //                                           static_cast<UINT64>(desc.widht),
            //                                           static_cast<UINT>(desc.height),
            //                                           static_cast<UINT16>(desc.slice),
            //                                           static_cast<UINT16>(desc.mips));
            // }
            // break;
    }

    auto state = D3D12_RESOURCE_STATE_COMMON;

    D3D12_CLEAR_VALUE* color = nullptr;
    if (desc.flags & TextureDesc::TextureFlags::UAV)
    {
        m_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    if (desc.flags & TextureDesc::TextureFlags::RTV)
    {
        m_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        m_optimizedColor.Format = desc.format;
        m_optimizedColor.Color[0] = desc.clearColor[0];
        m_optimizedColor.Color[1] = desc.clearColor[1];
        m_optimizedColor.Color[2] = desc.clearColor[2];
        m_optimizedColor.Color[3] = desc.clearColor[3];
        color = &m_optimizedColor;
    }
    if (desc.flags & TextureDesc::TextureFlags::DSV)
    {
        state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        m_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        m_optimizedColor.Format = m_desc.Format;
        m_optimizedColor.Color[0] = desc.clearColor[0];
        m_optimizedColor.Color[1] = desc.clearColor[1];
        m_optimizedColor.Color[2] = desc.clearColor[2];
        m_optimizedColor.Color[3] = desc.clearColor[3];
        m_optimizedColor.DepthStencil.Depth = 1;
        m_optimizedColor.DepthStencil.Stencil = 0;
        color = &m_optimizedColor;
    }

    auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    CheckHR(Engine.Device()
                .GetDevice()
                ->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &m_desc, state, color, IID_PPV_ARGS(&m_resource)));

    m_states.resize(desc.mips);
    SetState(state);
    m_resource->SetName(StringToWString(desc.debugName).c_str());
}

SE::D3D12Resource::D3D12Resource(const TextureDesc&, ID3D12Resource* resource)
{
    // get description
    m_desc = resource->GetDesc();
    // take ownership of this resource
    m_resource = resource;
    m_resource->SetName(L"Render Target");
    m_states.resize(1);
    SetState(D3D12_RESOURCE_STATE_PRESENT);
}
