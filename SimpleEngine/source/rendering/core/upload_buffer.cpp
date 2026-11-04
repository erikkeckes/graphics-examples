#include "rendering/core/upload_buffer.hpp"
#include "rendering/core/resource_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"

SE::UploadBuffer::UploadBuffer(size_t pageSize) : m_PageSize(pageSize) {}

SE::UploadBuffer::~UploadBuffer() {}

SE::UploadBuffer::Allocation SE::UploadBuffer::AllocateSpecific(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > m_PageSize)
    {
        throw std::bad_alloc();
    }

    // If there is no current page, or the requested allocation exceeds the
    // remaining space in the current page, request a new page.
    if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment))
    {
        m_CurrentPage = RequestPage();
    }

    return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

std::shared_ptr<SE::UploadBuffer::Page> SE::UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;

    if (!m_AvailablePages.empty())
    {
        page = m_AvailablePages.front();
        m_AvailablePages.pop_front();
    }
    else
    {
        page = std::make_shared<Page>(m_PageSize);
        m_PagePool.push_back(page);
    }

    return page;
}

void SE::UploadBuffer::Reset()
{
    m_CurrentPage = nullptr;
    // Reset all available pages.
    m_AvailablePages = m_PagePool;

    for (auto page : m_AvailablePages)
    {
        // Reset the page for new allocations.
        page->Reset();
    }
}

SE::UploadBuffer::Page::Page(size_t sizeInBytes)
    : m_CPUPtr(nullptr), m_GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0)), m_PageSize(sizeInBytes), m_Offset(0)
{
    BufferDes desc{};
    desc.InitAsStructuredBuffer(static_cast<uint32_t>(sizeInBytes), 1, nullptr, "upload buffer").AllowDirectWrite();

    m_resource = new D3D12Resource(desc);

    m_GPUPtr = m_resource->GetGPUAdrress();
    m_resource->GetResource()->Map(0, nullptr, &m_CPUPtr);
}

SE::UploadBuffer::Page::~Page()
{
    m_resource->GetResource()->Unmap(0, nullptr);
    m_CPUPtr = nullptr;
    m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
    delete m_resource;
}

bool SE::UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize = AlignUp(sizeInBytes, alignment);
    size_t alignedOffset = AlignUp(m_Offset, alignment);

    return alignedOffset + alignedSize <= m_PageSize;
}

SE::UploadBuffer::Allocation SE::UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment))
    {
        // Can't allocate space from page.
        throw std::bad_alloc();
    }

    size_t alignedSize = AlignUp(sizeInBytes, alignment);
    m_Offset = AlignUp(m_Offset, alignment);

    Allocation allocation;
    allocation.CPU = static_cast<uint8_t*>(m_CPUPtr) + m_Offset;
    allocation.GPU = m_GPUPtr + m_Offset;

    m_Offset += alignedSize;

    return allocation;
}

void SE::UploadBuffer::Page::Reset() { m_Offset = 0; }