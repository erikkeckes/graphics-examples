#pragma once
#include "rendering/common_dx.hpp"
#include <deque>

// this class is taken from 3dgep directx tutorial

namespace SE
{
class D3D12Resource;

class UploadBuffer
{
public:
    // Use to upload data to the GPU
    struct Allocation
    {
        void* CPU;
        D3D12_GPU_VIRTUAL_ADDRESS GPU;
    };

    /**
     * @param pageSize The size to use to allocate new pages in GPU memory.
     */
    explicit UploadBuffer(size_t pageSize = _2MB);

    virtual ~UploadBuffer();

    /**
     * The maximum size of an allocation is the size of a single page.
     */
    size_t GetPageSize() const { return m_PageSize; }

    // 256 is default aligment for constant buffer
    template <typename T>
    D3D12_GPU_VIRTUAL_ADDRESS Allocate(const T& element, size_t alignment = 256);

    /**
     * Allocate memory in an Upload heap.
     * An allocation must not exceed the size of a page.
     * Use a memcpy or similar method to copy the
     * buffer data to CPU pointer in the Allocation structure returned from
     * this function.
     */
    Allocation AllocateSpecific(size_t sizeInBytes, size_t alignment = 256);

    /**
     * Release all allocated pages. This should only be done when the command list
     * is finished executing on the CommandQueue.
     */
    void Reset();

private:
    // A single page for the allocator.
    struct Page
    {
        Page(size_t sizeInBytes);
        ~Page();

        // Check to see if the page has room to satisfy the requested
        // allocation.
        bool HasSpace(size_t sizeInBytes, size_t alignment) const;

        // Allocate memory from the page.
        // Throws std::bad_alloc if the the allocation size is larger
        // that the page size or the size of the allocation exceeds the
        // remaining space in the page.
        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        // Reset the page for reuse.
        void Reset();

    private:
        D3D12Resource* m_resource = nullptr;

        // Base pointer.
        void* m_CPUPtr;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

        // Allocated page size.
        size_t m_PageSize;
        // Current allocation offset in bytes.
        size_t m_Offset;
    };

    // A pool of memory pages.
    using PagePool = std::deque<std::shared_ptr<Page>>;

    // Request a page from the pool of available pages
    // or create a new page if there are no available pages.
    std::shared_ptr<Page> RequestPage();

    PagePool m_PagePool;
    PagePool m_AvailablePages;

    std::shared_ptr<Page> m_CurrentPage;

    // The size of each page of memory.
    size_t m_PageSize;
};
template <typename T>
inline D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::Allocate(const T& element, size_t alignment)
{
    const auto size = sizeof(T);
    const auto allocation = AllocateSpecific(size, alignment);
    memcpy(allocation.CPU, &element, size);
    return allocation.GPU;
}
}  // namespace SE