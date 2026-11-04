#pragma once
#include "core/resource.hpp"
#include <memory>

namespace SE
{
    class D3D12Texture;
    class Model;

    class Image : public Resource
    {
    public:
        // for loading a texture as resource
        Image(FileIO::Directory dir, const std::string& path, bool srgb = false);
        Image(const Model& model, int index, bool srgb = false);
        virtual ~Image();

        static std::string GetPath(const Model& model, int index, bool srgb = false);
        static std::string GetPath(FileIO::Directory dir, const std::string& path, bool srgb = false);

    private:
        friend class Renderer;
        void LoadExternalFile(FileIO::Directory dir, const std::string& path);

        bool m_srgb = false;

        std::unique_ptr<D3D12Texture> m_texture;
    };
}  // namespace SE