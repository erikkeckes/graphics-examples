
#pragma once
#include "rendering/common_dx.hpp"

namespace SE
{
class CommandQueue;

class Fence
{
public:
    DELETEMOVECOPY(Fence);

    Fence();
    Fence(ID3D12Device2* device);
    ~Fence();

    void Signal(const CommandQueue& queue);

    void Wait();

    void WaitFor(uint64_t value);

    uint64_t GetWaitValue() const;

    uint64_t GetCurrentValue() const;

private:
    friend class CommandQueue;

    ID3D12Fence* m_fence = nullptr;
    uint64_t m_fenceValue = 0;
    HANDLE m_event;
};
}  // namespace SE