#pragma once
#include "core/resource.hpp"

#include <memory>

namespace SE
{
    class D3D12Texture;

    class Skybox : public Resource
    {
    public:
        Skybox(FileIO::Directory dir, const std::string& path);

    private:
        void ConvertHdr(const D3D12Texture& hdrTexture);

        // void ConvertHdrGraphics(const D3D12Texture& hdrTexture, uint32_t samplerIndex);

        void ConvertHdrCompute(const D3D12Texture& hdrTexture, uint32_t samplerIndex);

        void GenerateIrradiance();

        void GenerateFilteredMaps();

        friend class Renderer;

        std::unique_ptr<D3D12Texture> m_skybox;
        std::unique_ptr<D3D12Texture> m_prefilteredMaps;
        std::unique_ptr<D3D12Texture> m_iradiance;
    };
}  // namespace SE