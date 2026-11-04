#include "core/engine.hpp"

#include <chrono>
#include <iostream>

#include "core/ecs.hpp"
#include "rendering/core/device_dx.hpp"
#include "core/fileio.hpp"
#include "core/input.hpp"
#include "core/resources.hpp"
#include "tools/log.hpp"

using namespace SE;

// Make the engine a global variable on free store memory.
EngineClass SE::Engine;

void EngineClass::Initialize()
{
    m_fileIO = new SE::FileIO();
    m_resources = new SE::Resources();
    m_device = new SE::Device();
    m_input = new SE::Input();
    m_ECS = new EntityComponentSystem();
}

void EngineClass::Shutdown()
{
    delete m_ECS;
    delete m_input;
    delete m_resources;
    delete m_device;
    delete m_fileIO;
}

void EngineClass::Run()
{
    auto time = std::chrono::high_resolution_clock::now();
    while (!m_device->ShouldClose())
    {
        auto ctime = std::chrono::high_resolution_clock::now();
        auto elapsed = ctime - time;
        float dt = (float)((double)std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000000.0);

        m_input->Update();
        m_ECS->UpdateSystems(dt);
        m_ECS->RemovedDeleted();
        m_device->BeginFrame();
        m_ECS->RenderSystems();
        m_device->EndFrame();
        m_device->Update();

        time = ctime;
    }
}