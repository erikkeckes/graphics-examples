#pragma once
#include <string>

namespace SE
{
    class EntityComponentSystem;
    class FileIO;
    class Resources;
    class Device;
    class Input;

    class EngineClass
    {
    public:
        void Initialize();
        void Shutdown();
        void Run();

        FileIO& FileIO() { return *m_fileIO; }
        Resources& Resources() { return *m_resources; }
        Device& Device() { return *m_device; }
        Input& Input() { return *m_input; }
        EntityComponentSystem& ECS() { return *m_ECS; }
    private:
        SE::FileIO* m_fileIO = nullptr;
        SE::Resources* m_resources = nullptr;
        SE::Device* m_device = nullptr;
        SE::Input* m_input = nullptr;
        EntityComponentSystem* m_ECS = nullptr;
    };

    extern EngineClass Engine;

}  // namespace SE