#include "rendering/core/buffer_dx.hpp"
#include "rendering/core/resource_dx.hpp"
#include "rendering/core/command_list_dx.hpp"
#include "rendering/core/views_dx.hpp"
#include "rendering/core/command_queue_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"

SE::Buffer::Buffer(const BufferDes& desc) : m_desc(desc)
{
    m_size = desc.elementCount * desc.elementSize;

    m_resource = new D3D12Resource(desc);

    // creating views
    switch (m_desc.type)
    {
        case BufferDes::Type::Index:
        {
            m_indexView = new IndexView(m_size, m_desc.elementSize, m_resource);
            break;
        }
        case BufferDes::Type::Vertex:
        {
            m_vertexView = new VertexView(m_size, m_desc.elementSize, m_resource);
            break;
        }
        case BufferDes::Type::Constant:
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC view{};
            view.BufferLocation = m_resource->GetGPUAdrress();
            view.SizeInBytes = m_size;

            m_cbv = new ConstantBufferView(view);
            break;
        }
        case BufferDes::Type::Structured:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC view{};
            view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            view.Buffer.NumElements = m_desc.elementCount;
            view.Buffer.StructureByteStride = m_desc.elementSize;
            m_srv = new ShaderResourceView(view, m_resource);
            break;
        }
    }

    // upload initial data if needed
    if (m_desc.data)
    {
        auto& list = Engine.Device().GetCommandList(QueueType::CopyQueue);
        auto& queue = Engine.Device().GetQueue(QueueType::CopyQueue);

        // create temporaly upload buffer
        auto uploadDesc = desc;
        uploadDesc.type = BufferDes::Type::Structured;
        uploadDesc.access = BufferDes::BufferFlags::Write;

        auto* uploadResource = new D3D12Resource(uploadDesc);

        uploadResource->Transition(list, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_resource->Transition(list, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_SUBRESOURCE_DATA subResource;
        subResource.pData = m_desc.data;
        subResource.RowPitch = m_size;
        subResource.SlicePitch = subResource.RowPitch;

        UpdateSubresources(list.List(), m_resource->GetResource(), uploadResource->GetResource(), 0, 0, 1, &subResource);

        queue.Execute(list);
        queue.Flush();  // wait for copy to finish

        delete uploadResource;
    }
}

SE::Buffer::~Buffer()
{
    delete m_uav;
    delete m_cbv;
    delete m_srv;
    delete m_vertexView;
    delete m_indexView;
    delete m_resource;

    // free cpu side memory
}

const SE::IndexView& SE::Buffer::GetIndexView() const
{
    assert(m_indexView && "Buffer Getting nonexisting view");
    return *m_indexView;
}

const SE::VertexView& SE::Buffer::GetVertexView() const
{
    assert(m_vertexView && "Buffer Getting nonexisting view");
    return *m_vertexView;
}

const SE::ShaderResourceView& SE::Buffer::GetSRV() const
{
    assert(m_srv && "Buffer Getting nonexisting view");
    return *m_srv;
}

const SE::ConstantBufferView& SE::Buffer::GetCBV() const
{
    assert(m_cbv && "Buffer Getting nonexisting view");
    return *m_cbv;
}

const SE::UnorderedAccessView& SE::Buffer::GetUAV() const
{
    assert(m_uav && "Buffer Getting nonexisting view");
    return *m_uav;
}
