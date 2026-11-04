#include "resources/image.hpp"
#include "rendering/common_dx.hpp"
#include "resources/model.hpp"
#include "tinygltf/stb_image.h"
#include "rendering/core/texture_dx.hpp"

inline DXGI_FORMAT GetTextureFormat(bool srgb)
{
    if (srgb)
    {
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    }

    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

SE::Image::Image(FileIO::Directory dir, const std::string& path, bool srgb) : Resource(ResourceType::Image), m_srgb(srgb)
{
    LoadExternalFile(dir, path);
}

SE::Image::Image(const Model& model, int index, bool srgb) : Resource(ResourceType::Image), m_srgb(srgb)
{
    const auto& doc = model.GetModel();
    assert(static_cast<int>(doc.images.size()) > index && "loading image outside off bounds");

    const auto& image = doc.images[index];
    if (!image.uri.empty())
    {
        auto dir = Engine.FileIO().GetDirectory(model.GetPath());
        LoadExternalFile(dir, image.uri);
    }
    else
    {
        TextureDesc desc{};

        if (!image.image.empty())
        {
            desc.InitAsTexture2D(image.width, image.height, GetTextureFormat(m_srgb), image.image.data()).SetMips();
            m_texture = std::make_unique<D3D12Texture>(desc);
        }
        else if (image.bufferView >= 0)
        {
            const auto& view = doc.bufferViews[image.bufferView];
            const auto& buffer = doc.buffers[view.buffer];
            const auto* ptr = &buffer.data.at(view.byteOffset);
            int width = 0, height = 0, channels = 0;
            auto* data = stbi_load_from_memory(ptr, (int)buffer.data.size(), &width, &height, &channels, 4);
            if (data)
            {
                desc.InitAsTexture2D(image.width, image.height, GetTextureFormat(m_srgb), data).SetMips();
                m_texture = std::make_unique<D3D12Texture>(desc);
                stbi_image_free(data);
            }
        }

        assert(m_texture);
    }
}

SE::Image::~Image() = default;

std::string SE::Image::GetPath(const Model& model, int index, bool srgb)
{
    if (srgb)
    {
        return model.GetPath() + " | Image " + std::to_string(index) + " | srgb";
    }
    return model.GetPath() + " | Image " + std::to_string(index);
}

std::string SE::Image::GetPath(FileIO::Directory dir, const std::string& path, bool srgb)
{
    if (srgb)
    {
        return Engine.FileIO().GetPath(dir, path) + " | srgb";
    }
    return Engine.FileIO().GetPath(dir, path);
}

void SE::Image::LoadExternalFile(FileIO::Directory dir, const std::string& path)
{
    // load from external file
    std::vector<char> texture = Engine.FileIO().ReadBinaryFile(dir, path);

    if (!texture.empty())
    {
        int width = 0, height = 0, channells = 0;
        auto* data = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(texture.data()),
                                           static_cast<int>(texture.size()),
                                           &width,
                                           &height,
                                           &channells,
                                           4);
        if (data)
        {
            // only expecting 4 channels for now
            // set name base on m_path from resources
            TextureDesc desc{};
            desc.InitAsTexture2D(width, height, GetTextureFormat(m_srgb), data).SetName(path).SetMips();

            m_texture = std::make_unique<D3D12Texture>(desc);
            stbi_image_free(data);
        }
    }
    else
    {
        Log::Error("Failed to load external texture path : {}", path);
    }
}
