#include "imgui.h"
#include "imgui_impl.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_dx12.h"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"
#include "rendering/core/command_list_dx.hpp"
#include "rendering/core/command_queue_dx.hpp"
#include "rendering/core/descriptor_allocators_dx.hpp"

bool ImGui_Impl_Init()
{
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOther(SE::Engine.Device().GetWindow(), true);

    ImGui_ImplDX12_InitInfo initInfo;
    initInfo.Device = SE::Engine.Device().GetDevice();
    initInfo.NumFramesInFlight = SE::BufferCount;
    initInfo.RTVFormat = SE::BackBufferFormat;
    initInfo.SrvDescriptorHeap = SE::Engine.Device().GetCBVSRVUAVAllocator().Heap();
    initInfo.SrvDescriptorAllocFn =
        [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
    { SE::Engine.Device().GetCBVSRVUAVAllocator().AllocateImgui(out_cpu_handle, out_gpu_handle); };
    initInfo.SrvDescriptorFreeFn =
        [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
    { SE::Engine.Device().GetCBVSRVUAVAllocator().FreeImgui(cpu_handle, gpu_handle); };
    return ImGui_ImplDX12_Init(&initInfo);
}

void ImGui_Impl_Shutdown()
{
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplDX12_Shutdown();
}

void ImGui_Impl_NewFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData*)
{
    auto& list = SE::Engine.Device().GetCommandList(SE::QueueType::DirectQueue);
    auto& queue = SE::Engine.Device().GetQueue(SE::QueueType::DirectQueue);

    list.StartMarker("ImGui");
    list.SetImGuiRendering();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list.List());
    list.EndMarker();
    queue.Execute(list);
}