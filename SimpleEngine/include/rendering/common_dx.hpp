#pragma once

// dx12 core headers
#include "d3d12/d3d12.h"
#include "d3d12/d3dcommon.h"
#include "d3d12/d3dx12.h"
#include <dxgi1_6.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// for logging when checking hr result
#include "tools/log.hpp"
#include "types_dx.hpp"
#include "tools/hashing.hpp"
#include "core/fileio.hpp"

#include <filesystem>
#include <string>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

// macro for releasing dx12 objects (raw pointers only)
#define COMFREE(p)        \
    {                     \
        if (p)            \
        {                 \
            p->Release(); \
            p = nullptr;  \
        }                 \
    }

// deletes move and copy constructors and operators
#define DELETEMOVECOPY(ClassType)                    \
    ClassType(const ClassType&) = delete;            \
    ClassType& operator=(const ClassType&) = delete; \
    ClassType(ClassType&&) = delete;                 \
    ClassType& operator=(ClassType&&) = delete;

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

// add extra params when compiling shader to allow performance profiling
#define SHADER_PROFILE 1

namespace SE
{
class Shader;

// limit of uint32_t in hex value
constexpr uint32_t Uint32Max = 0xffffffff;
constexpr uint32_t InvalidDescriptor = Uint32Max;
// maximum sice for root constant is 16 = 4 * 4 (float size on gpu)
// aka float 4
constexpr uint32_t MaxConstantSize = 4 * sizeof(UINT);

constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
constexpr DXGI_FORMAT DeptBufferFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

template <typename T>
void CheckResult(T result)
{
    if (result)
    {
        assert(false);
    }
}

struct BufferDes
{
    enum Type : uint32_t
    {
        Vertex = 1 << 1,      // heap common
        Index = 1 << 2,       // heap common
        Structured = 1 << 3,  // common or upload
        Constant = 1 << 4,
    };

    enum BufferFlags : uint32_t
    {
        ReadOnly = 1 << 1,   // can only read, gpu only
        Write = 1 << 2,      // can read and write, memory not specified
        UavAccess = 1 << 3,  // allowing uav access
    };

    uint32_t type = 0;
    uint32_t access = 0;
    uint32_t elementSize = 0;
    uint32_t elementCount = 0;
    const void* data = nullptr;
    std::string debugName;

    BufferDes& InitAsIndexBuffer(uint32_t indexSize,
                                 uint32_t count,
                                 const void* indexData,
                                 const std::string& name = "index buffer");

    BufferDes& InitAsVertexBuffer(uint32_t stride,
                                  uint32_t count,
                                  const void* vertexData,
                                  const std::string& name = "vertex buffer");

    BufferDes& InitAsConstantBuffer(uint32_t byteSize, const void* indexData, const std::string& name = "constant buffer");

    BufferDes& InitAsStructuredBuffer(uint32_t stride,
                                      uint32_t count,
                                      const void* bufferData = nullptr,
                                      const std::string& name = "index buffer");

    BufferDes& AllowUAV();

    BufferDes& AllowDirectWrite();
};

struct TextureDesc
{
    enum TextureType : uint32_t
    {
        Texture2D = 1,
        CubeMap = 1 << 1,  // texturearray2D in compute
    };

    enum TextureFlags : uint32_t
    {
        RTV = 1,
        DSV = 1 << 1,
        UAV = 1 << 2,
        SRV = 1 << 3,
        Mips = 1 << 4,
    };

    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t type = 0;
    uint32_t widht = 0;
    uint32_t height = 0;
    uint32_t slice = 1;
    uint32_t mips = 0;
    uint32_t flags = 0;  // uav, render target, depth buffer
    const void* data = nullptr;
    float clearColor[4] = {0, 0, 0, 0};
    std::string debugName;

    TextureDesc& InitAsTexture2D(uint32_t textureWidht,
                                 uint32_t textureHeight,
                                 DXGI_FORMAT textureFormat,
                                 const void* initData = nullptr);

    TextureDesc& InitAsRenderTarget(uint32_t textureWidht, uint32_t textureHeight, DXGI_FORMAT textureFormat);

    TextureDesc& InitAsDepthStencil(uint32_t textureWidht, uint32_t textureHeight, DXGI_FORMAT textureFormat);

    // not supporting multiple pictures for cubemap or hdr image, could be added later
    TextureDesc& InitAsCubeMap(uint32_t textureWidht, uint32_t textureHeight, DXGI_FORMAT textureFormat);

    // add support for init images
    // TextureDesc& InitAsTexture2DArray(uint32_t textureWidht,
    //                                  uint32_t textureHeight,
    //                                  uint32_t textureSlice,
    //                                  DXGI_FORMAT textureFormat);

    TextureDesc& AllowUAV();

    TextureDesc& AddView(TextureFlags view);

    // UINT32_MAX -> maximum ammount of mips will be generated
    TextureDesc& SetMips(uint32_t newMips = UINT32_MAX);

    TextureDesc& AddFlags(TextureFlags flag);

    TextureDesc& SetName(const std::string& name);

    TextureDesc& SetClearColor(float r, float g, float b, float a);
};

struct ShaderDesc
{
    enum ShaderType : uint32_t
    {
        Vertex = 1,
        Pixel = 2,
        Compute = 3,
        Hull = 4,
        Domain = 5,
    };

    void Check() const;

    ShaderDesc& InitAsVertexShader(FileIO::Directory dir, const std::string& filePath);

    ShaderDesc& InitAsPixelShader(FileIO::Directory dir, const std::string& filePath);

    ShaderDesc& InitAsHullShader(FileIO::Directory dir, const std::string& filePath);

    ShaderDesc& InitAsDomainShader(FileIO::Directory dir, const std::string& filePath);

    ShaderDesc& InitAsComputeShader(FileIO::Directory dir, const std::string& filePath);

    FileIO::Directory directory = FileIO::Directory::None;
    uint32_t type = 0;
    std::string path;
    // defines are not supported
    // std::vector<std::string> macros;
};

struct ShaderParam
{
    // currently unused, had an idea for descriptor ranges from reflection
    // more complex reflection would be needed
    enum ParamType : uint32_t
    {
        RootConstant = 1,  // root constants
        ConstantBuffer = 1 << 1,
        BufferSRV = 1 << 2,
        BufferUAV = 1 << 3,
        TextureSRV = 1 << 4,
        TextureUAV = 1 << 5,
        SamplerInput = 1 << 6
    };

    ShaderParam();

    void InitAsRootConstant(uint32_t slot, uint32_t space, uint32_t bufferSize);

    void InitAsConstantBuffer(uint32_t slot, uint32_t space);

    void InitAsTexture(uint32_t slot, uint32_t space);

    void InitAsTextureUAV(uint32_t slot, uint32_t space);

    void InitAsStructuredBuffer(uint32_t slot, uint32_t space);

    void InitAsStructuredBufferUAV(uint32_t slot, uint32_t space);

    void InitAsSampler(uint32_t slot, uint32_t space);

    void SetName(const std::string& resourceName);

    uint32_t GetRegister() const;

    uint32_t GetSpace() const;

    std::string GetType() const;

    D3D12_ROOT_PARAMETER param;
    D3D12_DESCRIPTOR_RANGE range;
    HasString nameHash;
    uint64_t paramHash = 0;
};

struct RasterState
{
    FillMode fill = FillMode::Solid;
    CullMode culling = CullMode::None;
    bool counterClockWise = true;
};

struct StencilDesc
{
    StencilOperation stencilFailOperation = StencilOperation::Keep;
    StencilOperation stencilDepthFailOperation = StencilOperation::Keep;
    StencilOperation stencilPassOperation = StencilOperation::Keep;
    ComparisonFunction stencilFunction = ComparisonFunction::NoComparison;
};

struct DepthStencilState
{
    bool depthEnabled = false;
    DepthMask depthMask = DepthMask::All;
    ComparisonFunction depthComparison = ComparisonFunction::Less;
    bool stencilEnabled = false;
    uint8_t stencilReadMask = 0u;
    uint8_t stencilWriteMask = 0u;
    StencilDesc frontFace;
    StencilDesc backFace;
};

struct Blend
{
    bool blendEnable = false;
    bool logicalOpEnable = false;
    BlendMode srcBlend = BlendMode::Zero;
    BlendMode dstBlend = BlendMode::Zero;
    BlendOperation blendOperation = BlendOperation::Add;
    BlendMode srcAlphaBlend = BlendMode::Zero;
    BlendMode dstAlphaBlend = BlendMode::Zero;
    BlendOperation AlphaOperation = BlendOperation::Add;
    LogicOperation logicalOperation = LogicOperation::Set;
    uint8_t renderTargetWriteMask = AllChannels;
};

class PipelineSettings
{
public:
    RasterState raster;
    DepthStencilState depthStencil;
    Blend blend[8];
    Topology topology = Topology::Undefined;

    uint32_t rtvCount = 0;
    DXGI_FORMAT rtvFormats[8] = {DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT depthFormat = DeptBufferFormat;

    static uint64_t HashSettings(const PipelineSettings& settings);

private:
    static uint64_t HashRaster(const RasterState& settings);
    static uint64_t HashStencil(const StencilDesc& settings);
    static uint64_t HashDepthStencil(const DepthStencilState& settings);
    static uint64_t HashBlend(const Blend& settings);
};

class GraphicsPipelineDesc
{
public:
    enum ShaderMask
    {
        VS = 1,
        PS = 1 << 1,
        HS = 1 << 2,
        DS = 1 << 3,
    };

    GraphicsPipelineDesc& SetVertexShader(const ShaderDesc& vs);

    GraphicsPipelineDesc& SetPixelShader(const ShaderDesc& ps);

    GraphicsPipelineDesc& SetHullShader(const ShaderDesc& hs);

    GraphicsPipelineDesc& SetDomainShader(const ShaderDesc& ds);

    GraphicsPipelineDesc& SetSettigns(const PipelineSettings& setting);

private:
    friend class Pipeline;
    uint8_t m_shaderMask = 0;

    ShaderDesc m_vertex{};
    ShaderDesc m_pixel{};
    ShaderDesc m_hull{};
    ShaderDesc m_domain{};

    // pso settings
    PipelineSettings m_settings{};
};

class ComputePipelineDesc
{
public:
    ComputePipelineDesc& InitComputePipeline(const ShaderDesc& cs);

private:
    friend class Pipeline;
    bool m_initialized = false;
    ShaderDesc m_compute{};
};

inline ChannelSize GetChannelSize(BYTE byte)
{
    if (byte == 1)
    {
        return SingleChannel;
    }
    else if (byte <= 3)
    {
        return DoubleChannel;
    }
    else if (byte <= 7)
    {
        return TripleChannel;
    }
    else if (byte <= 15)
    {
        return FourChannel;
    }
    return Unknown;
}

inline DXGI_FORMAT GetInputFormat(D3D_REGISTER_COMPONENT_TYPE type, ChannelSize size)
{
    switch (size)
    {
        case ChannelSize::Unknown:
        {
            return DXGI_FORMAT_UNKNOWN;
        }
        break;
        case ChannelSize::SingleChannel:
        {
            if (type == D3D_REGISTER_COMPONENT_UINT32)
            {
                return DXGI_FORMAT_R32_UINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_SINT32)
            {
                return DXGI_FORMAT_R32_SINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                return DXGI_FORMAT_R32_FLOAT;
            }
        }
        break;
        case ChannelSize::DoubleChannel:
        {
            if (type == D3D_REGISTER_COMPONENT_UINT32)
            {
                return DXGI_FORMAT_R32G32_UINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_SINT32)
            {
                return DXGI_FORMAT_R32G32_SINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                return DXGI_FORMAT_R32G32_FLOAT;
            }
        }
        break;
        case ChannelSize::TripleChannel:
        {
            if (type == D3D_REGISTER_COMPONENT_UINT32)
            {
                return DXGI_FORMAT_R32G32B32_UINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_SINT32)
            {
                return DXGI_FORMAT_R32G32B32_SINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                return DXGI_FORMAT_R32G32B32_FLOAT;
            }
        }
        break;
        case ChannelSize::FourChannel:
        {
            if (type == D3D_REGISTER_COMPONENT_UINT32)
            {
                return DXGI_FORMAT_R32G32B32A32_UINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_SINT32)
            {
                return DXGI_FORMAT_R32G32B32A32_SINT;
            }
            else if (type == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }
    }

    return DXGI_FORMAT_UNKNOWN;
}

struct GraphicsShaderState
{
    std::shared_ptr<Shader> vertex = nullptr;
    std::shared_ptr<Shader> pixel = nullptr;
    std::shared_ptr<Shader> hull = nullptr;
    std::shared_ptr<Shader> domain = nullptr;
};

struct ComputeShaderState
{
    std::shared_ptr<Shader> compute = nullptr;
};

inline D3D12_FILL_MODE GetFillMode(FillMode mode)
{
    switch (mode)
    {
        case FillMode::WireFrame:
            return D3D12_FILL_MODE_WIREFRAME;
        case FillMode::Solid:
            return D3D12_FILL_MODE_SOLID;
    }
    return D3D12_FILL_MODE_SOLID;
}

inline D3D12_CULL_MODE GetCullingMode(CullMode mode)
{
    switch (mode)
    {
        case CullMode::None:
            return D3D12_CULL_MODE_NONE;
            break;
        case CullMode::Front:
            return D3D12_CULL_MODE_FRONT;
            break;
        case CullMode::Back:
            return D3D12_CULL_MODE_BACK;
            break;
    }
    return D3D12_CULL_MODE_NONE;
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopology(Topology topology)
{
    switch (topology)
    {
        case Topology::Undefined:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
            break;
        case Topology::Point:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            break;
        case Topology::Line:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            break;
        case Topology::Triangle:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        case Topology::Patch:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
            break;
    }
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

inline D3D12_PRIMITIVE_TOPOLOGY GetTopo(Topology topology)
{
    switch (topology)
    {
        case Topology::Undefined:
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
            break;
        case Topology::Point:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case Topology::Line:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case Topology::Triangle:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case Topology::Patch:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
    }
    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

inline D3D12_COMPARISON_FUNC GetComparisionFunction(ComparisonFunction function)
{
    switch (function)
    {
        case ComparisonFunction::NoComparison:
            return D3D12_COMPARISON_FUNC_NONE;
            break;
        case ComparisonFunction::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
            break;
        case ComparisonFunction::Less:
            return D3D12_COMPARISON_FUNC_LESS;
            break;
        case ComparisonFunction::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
            break;
        case ComparisonFunction::LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            break;
        case ComparisonFunction::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
            break;
        case ComparisonFunction::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            break;
        case ComparisonFunction::GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            break;
        case ComparisonFunction::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
            break;
    }
    return D3D12_COMPARISON_FUNC_NONE;
}

inline D3D12_DEPTH_WRITE_MASK GetDepthMask(DepthMask mask)
{
    switch (mask)
    {
        case DepthMask::All:
            return D3D12_DEPTH_WRITE_MASK_ALL;
            break;
        case DepthMask::NoMask:
            return D3D12_DEPTH_WRITE_MASK_ZERO;
            break;
    }

    return D3D12_DEPTH_WRITE_MASK_ZERO;
}

inline D3D12_STENCIL_OP GetStencilOperation(StencilOperation operation)
{
    switch (operation)
    {
        case StencilOperation::Keep:
            return D3D12_STENCIL_OP_KEEP;
            break;
        case StencilOperation::ToZero:
            return D3D12_STENCIL_OP_ZERO;
            break;
        case StencilOperation::Replace:
            return D3D12_STENCIL_OP_REPLACE;
            break;
        case StencilOperation::Invert:
            return D3D12_STENCIL_OP_INVERT;
            break;
        case StencilOperation::Increment:
            return D3D12_STENCIL_OP_INCR;
            break;
        case StencilOperation::Decrement:
            return D3D12_STENCIL_OP_DECR;
    }
    return D3D12_STENCIL_OP_KEEP;
}

inline uint8_t GetBlendMask(bool red = false, bool green = false, bool blue = false, bool alpha = false)
{
    uint8_t mask = 0;
    if (red)
    {
        mask |= BlendMask::Red;
    }
    if (green)
    {
        mask |= BlendMask::Green;
    }
    if (blue)
    {
        mask |= BlendMask::Blue;
    }
    if (alpha)
    {
        mask |= BlendMask::Alpha;
    }
    return mask;
}

inline D3D12_BLEND GetBlendMode(BlendMode mode)
{
    switch (mode)
    {
        case BlendMode::Zero:
            return D3D12_BLEND_ZERO;
            break;
        case BlendMode::One:
            return D3D12_BLEND_ONE;
            break;
        case BlendMode::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
            break;
        case BlendMode::InverseSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
            break;
        case BlendMode::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
            break;
        case BlendMode::InverseSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
            break;
        case BlendMode::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
            break;
        case BlendMode::InverseDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
            break;
        case BlendMode::SrcAlphaSat:
            return D3D12_BLEND_SRC_ALPHA_SAT;
            break;
    }
    return D3D12_BLEND_ZERO;
}

inline D3D12_BLEND_OP GetBlendOperation(BlendOperation function)
{
    switch (function)
    {
        case BlendOperation::Add:
            return D3D12_BLEND_OP_ADD;
            break;
        case BlendOperation::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
            break;
        case BlendOperation::ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
            break;
        case BlendOperation::Min:
            return D3D12_BLEND_OP_MIN;
            break;
        case BlendOperation::Max:
            return D3D12_BLEND_OP_MAX;
            break;
    }
    return D3D12_BLEND_OP_ADD;
}

inline D3D12_LOGIC_OP GetLogicalOperation(LogicOperation operation)
{
    switch (operation)
    {
        case LogicOperation::Clear:
            return D3D12_LOGIC_OP_CLEAR;
            break;
        case LogicOperation::Set:
            return D3D12_LOGIC_OP_SET;
            break;
        case LogicOperation::Copy:
            return D3D12_LOGIC_OP_COPY;
            break;
        case LogicOperation::CopyInv:
            return D3D12_LOGIC_OP_COPY_INVERTED;
            break;
        case LogicOperation::Inverting:
            return D3D12_LOGIC_OP_INVERT;
            break;
        case LogicOperation::And:
            return D3D12_LOGIC_OP_AND;
            break;
        case LogicOperation::Nand:
            return D3D12_LOGIC_OP_NAND;
            break;
        case LogicOperation::Or:
            return D3D12_LOGIC_OP_OR;
            break;
        case LogicOperation::Nor:
            return D3D12_LOGIC_OP_NOR;
            break;
        case LogicOperation::Xor:
            return D3D12_LOGIC_OP_XOR;
            break;
        case LogicOperation::AndReverse:
            return D3D12_LOGIC_OP_AND_REVERSE;
            break;
        case LogicOperation::AndInverted:
            return D3D12_LOGIC_OP_AND_INVERTED;
            break;
        case LogicOperation::OrReversed:
            return D3D12_LOGIC_OP_OR_REVERSE;
            break;
        case LogicOperation::OrInverted:
            return D3D12_LOGIC_OP_OR_INVERTED;
            break;
    }
    return D3D12_LOGIC_OP_CLEAR;
}

inline DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format)
{
    assert(format != DXGI_FORMAT_UNKNOWN && "Cannot parse Unknown format");

    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        {
            return DXGI_FORMAT_R32G32B32_TYPELESS;
        }

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        {
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        {
            return DXGI_FORMAT_R32G32_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        {
            return DXGI_FORMAT_R10G10B10A2_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        {
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        {
            return DXGI_FORMAT_R16G16_TYPELESS;
        }
        break;

        case DXGI_FORMAT_D32_FLOAT:
        {
            return DXGI_FORMAT_D32_FLOAT;
        }

        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            return DXGI_FORMAT_R32_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        {
            return DXGI_FORMAT_R8G8_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        {
            return DXGI_FORMAT_R16_TYPELESS;
        }
        break;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        {
            return DXGI_FORMAT_R8_TYPELESS;
        }
        break;
        default:
            break;
    }

    assert(false && "Converting to typeless unsupported format");

    return format;
}

inline uint32_t GetFormatSize(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            return 16;
        }
        break;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        {
            return 12;
        }

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        {
            return 8;
        }
        break;

        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        {
            return 8;
        }
        break;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        {
            return 4;
        }
        break;

        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        {
            return 4;
        }
        break;

        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            return 4;
        }
        break;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        {
            return 2;
        }
        break;

        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        {
            return 2;
        }
        break;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        {
            return 1;
        }
        break;
        default:
            Log::Warn("Not supported texture format for format size calculation");
            return UINT32_MAX;
    }
}

inline bool IsSrgbFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;
        default:
            return false;
    }
}

inline DXGI_FORMAT GetRegularForSrgb(DXGI_FORMAT format)
{
    auto finalFormat = format;
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        {
            finalFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        break;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        {
            finalFormat = DXGI_FORMAT_BC1_UNORM;
        }
        break;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        {
            finalFormat = DXGI_FORMAT_BC2_UNORM;
        }
        break;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        {
            finalFormat = DXGI_FORMAT_BC3_UNORM;
        }
        break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        {
            finalFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        }
        break;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        {
            finalFormat = DXGI_FORMAT_B8G8R8X8_UNORM;
        }
        break;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        {
            finalFormat = DXGI_FORMAT_BC7_UNORM;
        }
        break;
        default:
            break;
    }
    return finalFormat;
}

inline DXGI_FORMAT GetRegularForDsv(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
            break;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            break;
        default:
            return format;
            break;
    }
}

inline void CheckHR(HRESULT hr)
{
    if (FAILED(hr))
    {
        // getting message from hr result
        char* hrCstr = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       hr,
                       0,
                       (LPSTR)&hrCstr,
                       0,
                       nullptr);

        SE::Log::Critical("HRESULT Error: {0}", hrCstr);
    }
}

inline DXGI_FORMAT GetIndexFormat(uint32_t stride)
{
    if (stride == sizeof(uint32_t))
    {
        return DXGI_FORMAT_R32_UINT;
    }
    else if (stride == sizeof(uint16_t))
    {
        return DXGI_FORMAT_R16_UINT;
    }

    assert(false && "Uknown index format");
    return DXGI_FORMAT_UNKNOWN;
}

inline std::string WStringToString(const std::wstring& wstr) { return std::filesystem::path(wstr).string(); }

inline std::wstring StringToWString(const std::string& str) { return std::filesystem::path(str).wstring(); }

// These functions were taken from the MiniEngine.
// Source code available here:
// https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Math/Common.h
template <typename T>
inline T AlignUpWithMask(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T>
inline T AlignDownWithMask(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

template <typename T>
inline T AlignUp(T value, size_t alignment)
{
    return AlignUpWithMask(value, alignment - 1);
}

template <typename T>
inline T AlignDown(T value, size_t alignment)
{
    return AlignDownWithMask(value, alignment - 1);
}

template <typename T>
inline bool IsAligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T>
inline T DivideByMultiple(T value, size_t alignment)
{
    return (T)((value + alignment - 1) / alignment);
}
/***************************************************************************/

}  // namespace SE