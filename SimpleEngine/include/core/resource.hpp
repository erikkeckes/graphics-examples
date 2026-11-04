#pragma once

#include <string>
#include "core/engine.hpp"
#include "core/fileio.hpp"

namespace SE
{

    enum class ResourceType
    {
        Image,
        Shader,
        Model,
        Mesh,
        Vibration,
        Font
    };

    class Resource
    {
        friend class Resources;

    public:
        size_t ResourceID() const { return m_ID; }

        ResourceType GetType() const { return m_type; }

        const std::string& GetPath() const { return m_path; }

        bool Generated() const { return m_generated; }

        Resource(const Resource&) = delete;
    protected:
        Resource(ResourceType type) : m_ID(0), m_type(type) {}

        static std::string GetPath(FileIO::Directory directory, const std::string& path)
        {
            return Engine.FileIO().GetPath(directory, path);
        }

        virtual ~Resource() = default;

        virtual void Reload() {}

        static size_t GetNextGeneratedID() { return m_nextGeneratedID++; }

        size_t m_ID;

        std::string m_path;

        ResourceType m_type;

        FileIO::Directory m_directory = FileIO::Directory::None;

        bool m_generated = false;

    private:
        static size_t m_nextGeneratedID;
    };

}  // namespace SE