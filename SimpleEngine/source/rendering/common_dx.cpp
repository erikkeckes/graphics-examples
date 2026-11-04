#include "rendering/common_dx.hpp"

namespace SE
{
BufferDes& BufferDes::InitAsIndexBuffer(uint32_t indexSize, uint32_t count, const void* indexData, const std::string& name)
{
    assert((indexSize == sizeof(uint32_t) || indexSize == sizeof(uint16_t)) &&
           "Buffer description :: index buffer unknown stride\nUse uint32 or uint16");
    assert(indexData && "Buffer description :: index buffer needs initial data");
    assert(count > 0 && "Buffer description :: index buffer zero or negative element count");
    type = Type::Index;
    access = BufferFlags::ReadOnly;
    elementSize = indexSize;
    elementCount = count;
    data = indexData;
    debugName = name;
    return *this;
}

BufferDes& BufferDes::InitAsVertexBuffer(uint32_t stride, uint32_t count, const void* vertexData, const std::string& name)
{
    assert(stride > 0 && "Buffer description :: vertex buffer zero or negative stride");
    assert(vertexData && "Buffer description :: vertex buffer needs initial data");
    assert(count > 0 && "Buffer description :: vertex buffer zero or negative element count");
    type = Type::Vertex;
    access = BufferFlags::ReadOnly;
    elementSize = stride;
    elementCount = count;
    data = vertexData;
    debugName = name;
    return *this;
}

BufferDes& BufferDes::InitAsConstantBuffer(uint32_t byteSize, const void* indexData, const std::string& name)
{
    assert(byteSize > 0 && "Buffer description :: constant buffer zero or negative size");
    type = Type::Constant;
    access = BufferFlags::ReadOnly;
    // CB needs size multiple of 256
    elementSize = AlignUp(byteSize, 256);
    elementCount = 1;
    data = indexData;
    debugName = name;
    return *this;
}

BufferDes& BufferDes::InitAsStructuredBuffer(uint32_t stride, uint32_t count, const void* bufferData, const std::string& name)
{
    assert(stride > 0 && "Buffer description :: structured buffer zero or negative stride");
    assert(count > 0 && "Buffer description :: structured buffer zero or negative element count");
    type = Type::Structured;
    access = BufferFlags::ReadOnly;
    elementSize = stride;
    elementCount = count;
    data = bufferData;
    debugName = name;
    return *this;
}

BufferDes& BufferDes::AllowUAV()
{
    access |= BufferFlags::UavAccess;
    return *this;
}

BufferDes& BufferDes::AllowDirectWrite()
{
    // for performance reasons only structured buffer can have direct write
    assert(type == Type::Structured && "Only structured and constant buffers are allowed direct write access");
    access |= BufferFlags::Write;
    return *this;
}

TextureDesc& TextureDesc::InitAsTexture2D(uint32_t textureWidht,
                                          uint32_t textureHeight,
                                          DXGI_FORMAT textureFormat,
                                          const void* initData)
{
    assert(textureWidht > 0 && "TextureDesc texture2D must greater that 0");
    assert(textureHeight > 0 && "TextureDesc texture2D heigh must greater that 0");
    assert(textureFormat != DXGI_FORMAT_UNKNOWN && "TextureDesc texture2D texture format is not unknow");
    widht = textureWidht;
    height = textureHeight;
    format = textureFormat;
    data = initData;
    flags |= TextureFlags::SRV;
    type = TextureType::Texture2D;
    return *this;
}

TextureDesc& TextureDesc::InitAsRenderTarget(uint32_t textureWidht, uint32_t textureHeight, DXGI_FORMAT textureFormat)
{
    assert(textureWidht > 0 && "TextureDesc render target must greater that 0");
    assert(textureHeight > 0 && "TextureDesc render target heigh must greater that 0");
    assert(textureFormat != DXGI_FORMAT_UNKNOWN && "TextureDesc render target texture format is not unknow");
    widht = textureWidht;
    height = textureHeight;
    format = textureFormat;
    flags |= TextureFlags::RTV;
    type = TextureType::Texture2D;
    return *this;
}

TextureDesc& TextureDesc::InitAsDepthStencil(uint32_t textureWidht, uint32_t textureHeight, DXGI_FORMAT textureFormat)
{
    assert(textureWidht > 0 && "TextureDesc depth stencil must greater that 0");
    assert(textureHeight > 0 && "TextureDesc depth stencil heigh must greater that 0");
    assert(textureFormat != DXGI_FORMAT_UNKNOWN && "TextureDesc depth stencil texture format is not unknow");
    widht = textureWidht;
    height = textureHeight;
    format = textureFormat;
    flags |= TextureFlags::DSV;
    type = TextureType::Texture2D;
    return *this;
}

// not supporting multiple pictures for cubemap or hdr image, could be added later
TextureDesc& TextureDesc::InitAsCubeMap(uint32_t textureWidht, uint32_t textureHeight, DXGI_FORMAT textureFormat)
{
    assert(textureWidht > 0 && "TextureDesc cubemap must greater that 0");
    assert(textureHeight > 0 && "TextureDesc cubemap heigh must greater that 0");
    assert(textureFormat != DXGI_FORMAT_UNKNOWN && "TextureDesc cubemap format is not unknow");
    widht = textureWidht;
    height = textureHeight;
    format = textureFormat;
    slice = 6;  // cubemap has 6 faces
    flags |= TextureFlags::SRV;
    type = TextureType::CubeMap;
    return *this;
}

// add support for init images
// TextureDesc& TextureDesc::InitAsTexture2DArray(uint32_t textureWidht,
//                                               uint32_t textureHeight,
//                                               uint32_t textureSlice,
//                                               DXGI_FORMAT textureFormat)
//{
//    assert(textureWidht > 0 && "TextureDesc texture2D array width must greater that 0");
//    assert(textureHeight > 0 && "TextureDesc texture2D array heigh must greater that 0");
//    assert(textureSlice > 0 && "TextureDesc texture2D array slices must greater that 0");
//    assert(textureFormat != DXGI_FORMAT_UNKNOWN && "TextureDesc texture2D array texture format is not unknow");
//
//    if (slice == 0)
//    {
//        // if slice is just one no reason to use texture array
//        bee::Log::Warn("TextureDesc texture2D array, slice == 1 use texture 2D instead");
//    }
//
//    widht = textureWidht;
//    height = textureHeight;
//    format = textureFormat;
//    slice = textureSlice;
//    flags |= TextureFlags::SRV;
//    type = TextureType::TextureArray;
//    return *this;
//}

TextureDesc& TextureDesc::AllowUAV()
{
    flags |= TextureFlags::UAV;
    return *this;
}

TextureDesc& TextureDesc::AddView(TextureFlags view)
{
    flags |= view;
    return *this;
}

TextureDesc& TextureDesc::SetMips(uint32_t newMips)
{
    // will need uav access to be able to generate mipmpas on gpu
    flags |= TextureFlags::Mips;
    mips = newMips;
    return *this;
}

TextureDesc& TextureDesc::AddFlags(TextureFlags flag)
{
    flags |= flag;
    return *this;
}

TextureDesc& TextureDesc::SetName(const std::string& name)
{
    debugName = name;
    return *this;
}

TextureDesc& TextureDesc::SetClearColor(float r, float g, float b, float a)
{
    clearColor[0] = r;
    clearColor[1] = g;
    clearColor[2] = b;
    clearColor[3] = a;
    return *this;
}

void ShaderDesc::Check() const
{
    assert(directory != FileIO::Directory::None && "ShaderDescription invalid directory");
    assert(type > 0 && "ShaderDescription invalid type");
    assert(!path.empty() && "ShaderDescription path cannot be empty");
}

ShaderDesc& ShaderDesc::InitAsVertexShader(FileIO::Directory dir, const std::string& filePath)
{
    directory = dir;
    path = filePath;
    type = ShaderType::Vertex;
    return *this;
}

ShaderDesc& ShaderDesc::InitAsPixelShader(FileIO::Directory dir, const std::string& filePath)
{
    directory = dir;
    path = filePath;
    type = ShaderType::Pixel;
    return *this;
}

ShaderDesc& ShaderDesc::InitAsHullShader(FileIO::Directory dir, const std::string& filePath)
{
    directory = dir;
    path = filePath;
    type = ShaderType::Hull;
    return *this;
}

ShaderDesc& ShaderDesc::InitAsDomainShader(FileIO::Directory dir, const std::string& filePath)
{
    directory = dir;
    path = filePath;
    type = ShaderType::Domain;
    return *this;
}

ShaderDesc& ShaderDesc::InitAsComputeShader(FileIO::Directory dir, const std::string& filePath)
{
    directory = dir;
    path = filePath;
    type = ShaderType::Compute;
    return *this;
}

ShaderParam::ShaderParam()
{
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    param.DescriptorTable.pDescriptorRanges = &range;
}

void ShaderParam::InitAsRootConstant(uint32_t slot, uint32_t space, uint32_t bufferSize)
{
    param.Constants.ShaderRegister = slot;
    param.Constants.RegisterSpace = space;
    param.Constants.Num32BitValues = bufferSize / sizeof(UINT);  // each value on GPU has 4 bites
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

    SumHash(paramHash, static_cast<uint64_t>(slot) << 32);
    SumHash(paramHash, static_cast<uint64_t>(space) << 32);
    SumHash(paramHash, static_cast<uint64_t>(bufferSize) << 32);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) << 60);
};
void ShaderParam::InitAsConstantBuffer(uint32_t slot, uint32_t space)
{
    param.Descriptor.ShaderRegister = slot;
    param.Descriptor.RegisterSpace = space;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;

    SumHash(paramHash, static_cast<uint64_t>(slot) << 32);
    SumHash(paramHash, static_cast<uint64_t>(space) << 32);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_ROOT_PARAMETER_TYPE_CBV) << 60);
};
void ShaderParam::InitAsTexture(uint32_t slot, uint32_t space)
{
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &range;

    range.NumDescriptors = 1;
    range.RegisterSpace = space;
    range.BaseShaderRegister = slot;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    SumHash(paramHash, static_cast<uint64_t>(slot) << 32);
    SumHash(paramHash, static_cast<uint64_t>(space) << 32);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) << 60);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_DESCRIPTOR_RANGE_TYPE_SRV) << 56);
};
void ShaderParam::InitAsTextureUAV(uint32_t slot, uint32_t space)
{
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &range;

    range.NumDescriptors = 1;
    range.RegisterSpace = space;
    range.BaseShaderRegister = slot;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    SumHash(paramHash, static_cast<uint64_t>(slot) << 32);
    SumHash(paramHash, static_cast<uint64_t>(space) << 32);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) << 60);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_DESCRIPTOR_RANGE_TYPE_UAV) << 56);
};
void ShaderParam::InitAsStructuredBuffer(uint32_t slot, uint32_t space)
{
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &range;

    range.NumDescriptors = 1;
    range.RegisterSpace = space;
    range.BaseShaderRegister = slot;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    SumHash(paramHash, static_cast<uint64_t>(slot) << 32);
    SumHash(paramHash, static_cast<uint64_t>(space) << 32);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) << 60);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_DESCRIPTOR_RANGE_TYPE_SRV) << 56);
};
void ShaderParam::InitAsStructuredBufferUAV(uint32_t slot, uint32_t space)
{
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &range;

    range.NumDescriptors = 1;
    range.RegisterSpace = space;
    range.BaseShaderRegister = slot;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    SumHash(paramHash, static_cast<uint64_t>(slot) << 32);
    SumHash(paramHash, static_cast<uint64_t>(space) << 32);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) << 60);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_DESCRIPTOR_RANGE_TYPE_UAV) << 56);
};
void ShaderParam::InitAsSampler(uint32_t slot, uint32_t space)
{
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &range;

    range.NumDescriptors = 1;
    range.RegisterSpace = space;
    range.BaseShaderRegister = slot;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

    SumHash(paramHash, static_cast<uint64_t>(slot) << 32);
    SumHash(paramHash, static_cast<uint64_t>(space) << 32);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) << 60);
    SumHash(paramHash, static_cast<uint64_t>(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER));
};
void ShaderParam::SetName(const std::string& resourceName) { nameHash = HasString(resourceName); }

uint32_t ShaderParam::GetRegister() const
{
    if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
    {
        return range.BaseShaderRegister;
    }

    return param.Constants.RegisterSpace;
}

uint32_t ShaderParam::GetSpace() const
{
    if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
    {
        return range.RegisterSpace;
    }

    return param.Constants.RegisterSpace;
}

std::string ShaderParam::GetType() const
{
    if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
    {
        if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
        {
            return "Sampler";
        }
        if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
        {
            return "SRV";
        }
        if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
        {
            return "UAV";
        }
    }
    else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
    {
        return "Root Constant";
    }
    else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
    {
        return "CBV";
    }

    return "Unknown";
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetVertexShader(const ShaderDesc& vs)
{
    vs.Check();
    m_vertex = vs;
    m_shaderMask |= ShaderMask::VS;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetPixelShader(const ShaderDesc& ps)
{
    ps.Check();
    m_pixel = ps;
    m_shaderMask |= ShaderMask::PS;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetHullShader(const ShaderDesc& hs)
{
    hs.Check();
    m_hull = hs;
    m_shaderMask |= ShaderMask::HS;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetDomainShader(const ShaderDesc& ds)
{
    ds.Check();
    m_domain = ds;
    m_shaderMask |= ShaderMask::DS;
    return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::SetSettigns(const PipelineSettings& setting)
{
    m_settings = setting;
    return *this;
}

ComputePipelineDesc& ComputePipelineDesc::InitComputePipeline(const ShaderDesc& cs)
{
    cs.Check();
    m_compute = cs;
    m_initialized = true;
    return *this;
}

uint64_t PipelineSettings::HashSettings(const PipelineSettings& settings)
{
    uint64_t hash = 0;
    SumHash(hash, HashRaster(settings.raster));              // using last 4 bits
    SumHash(hash, HashDepthStencil(settings.depthStencil));  // using 16 first bits
    for (auto i = 0; i < 8; i++)
    {
        SumHash(hash, HashBlend(settings.blend[i]));  // using bits between 16 and 60
    }

    // using last 4 bits
    SumHash(hash, static_cast<uint64_t>(settings.topology) << 60);

    for (int i = 0; i < 8; i++)
    {
        SumHash(hash, static_cast<uint64_t>(settings.rtvFormats[i]));
    }

    SumHash(hash, settings.rtvCount);
    SumHash(hash, static_cast<uint64_t>(settings.depthFormat));
    return hash;
}

uint64_t PipelineSettings::HashRaster(const RasterState& settings)
{
    uint64_t hash = 0;
    SumHash(hash, static_cast<uint64_t>(settings.fill) << 63);
    SumHash(hash, static_cast<uint64_t>(settings.culling) << 60);
    SumHash(hash, static_cast<uint64_t>(settings.counterClockWise) << 62);
    return hash;
}

uint64_t PipelineSettings::HashStencil(const StencilDesc& settings)
{
    uint64_t hash = 0;
    SumHash(hash, static_cast<uint64_t>(settings.stencilFailOperation) << 8);  // max 5
    SumHash(hash, static_cast<uint64_t>(settings.stencilDepthFailOperation) << 8);
    SumHash(hash, static_cast<uint64_t>(settings.stencilPassOperation) << 8);
    SumHash(hash, static_cast<uint64_t>(settings.stencilFunction) << 8);  // max 8
    return hash;
}

uint64_t PipelineSettings::HashDepthStencil(const DepthStencilState& settings)
{
    uint64_t hash = 0;
    // depth using first 8 bits
    SumHash(hash, static_cast<uint64_t>(settings.depthEnabled));
    SumHash(hash, static_cast<uint64_t>(settings.depthMask) << 1);
    SumHash(hash, static_cast<uint64_t>(settings.depthComparison) << 4);

    // stencill using bits after 8
    SumHash(hash, static_cast<uint64_t>(settings.stencilEnabled) << 8);
    SumHash(hash, static_cast<uint64_t>(settings.stencilReadMask) << 8);
    SumHash(hash, static_cast<uint64_t>(settings.stencilWriteMask) << 8);

    SumHash(hash, HashStencil(settings.frontFace));
    SumHash(hash, HashStencil(settings.backFace) << 8);

    return hash;
}

uint64_t PipelineSettings::HashBlend(const Blend& settings)
{
    uint64_t hash = 0;
    SumHash(hash, static_cast<uint64_t>(settings.blendEnable));
    SumHash(hash, static_cast<uint64_t>(settings.logicalOpEnable) << 1);

    SumHash(hash, static_cast<uint64_t>(settings.srcBlend) << 2);
    SumHash(hash, static_cast<uint64_t>(settings.dstBlend) << 2);
    SumHash(hash, static_cast<uint64_t>(settings.blendOperation) << 6);

    SumHash(hash, static_cast<uint64_t>(settings.srcAlphaBlend) << 10);
    SumHash(hash, static_cast<uint64_t>(settings.dstAlphaBlend) << 10);
    SumHash(hash, static_cast<uint64_t>(settings.AlphaOperation) << 14);
    SumHash(hash, static_cast<uint64_t>(settings.logicalOperation) << 14);

    SumHash(hash, static_cast<uint64_t>(settings.renderTargetWriteMask) << 16);

    return hash << 16;
}
}  // namespace bee