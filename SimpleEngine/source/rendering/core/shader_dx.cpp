#include "rendering/core/shader_dx.hpp"
#include <filesystem>

const std::string CompiledFolder = "ShaderCache\\shaders\\";
bool CacheInitialized = false;

SE::Shader::Shader(const ShaderDesc& desc) : Resource(ResourceType::Shader)
{
    assert(desc.type > 0 && "Invalid shader type");
    assert(!desc.path.empty() && "Empty file path for shader");

    IDxcUtils* utils = nullptr;
    IDxcCompiler3* compiler = nullptr;
    auto* includeHandler = new CustomIncludeHandler();

    CheckHR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
    CheckHR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    if (!includeHandler)
    {
        assert(false && "Failed to create include handler");
    }

    includeHandler->m_utils = utils;

    // change this to use file io from SE
    auto filePath = Engine.FileIO().GetPath(desc.directory, desc.path);
    std::wstring path = StringToWString(filePath);

    m_type = desc.type;
    m_hash = CalculateHash(desc);

    if (!ShouldRecompile(desc.path, filePath))
    {
        SE::Log::Info("Loading precompiled shader for {}", desc.path);

        auto compiledPath = StringToWString(GetCompiledPath(desc.path));

        // load file from cache
        IDxcBlobEncoding* fileSource = nullptr;
        CheckHR(utils->LoadFile(compiledPath.c_str(), nullptr, &fileSource));

        // no release needed, transfering ownership
        m_shaderBlob = fileSource;

        auto* reflection = CreateReflection(m_shaderBlob, utils);
        ParseReflection(reflection);

        COMFREE(reflection);
        COMFREE(includeHandler)
        COMFREE(utils);
        COMFREE(compiler);
        return;
    }

    Log::Info("Compiling shader from path {}", desc.path);

    m_compiled = true;  // to force pso recompilation
    IDxcBlobEncoding* fileSource = nullptr;
    CheckHR(utils->LoadFile(path.c_str(), nullptr, &fileSource));

    // creating buffer structure for compilation
    DxcBuffer buffer_source;
    buffer_source.Ptr = fileSource->GetBufferPointer();
    buffer_source.Size = fileSource->GetBufferSize();
    buffer_source.Encoding = DXC_CP_ACP;

    auto commands = GetCompileCommands(desc, path);

    IDxcResult* results = nullptr;
    CheckHR(compiler->Compile(&buffer_source,
                              commands.data(),
                              static_cast<UINT32>(commands.size()),
                              includeHandler,
                              IID_PPV_ARGS(&results)));

    COMFREE(fileSource);

    // error checking
    IDxcBlobUtf8* errors = nullptr;
    CheckHR(results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
    if (errors != nullptr && errors->GetStringLength() != 0)
    {
        SE::Log::Error("Shader compilation {}", errors->GetStringPointer());
        assert(false && "failed to compile shader");
    }

    assert(results && "Failed to compiler or missing compile result");
    COMFREE(errors);

    IDxcBlobUtf16* shaderName = nullptr;
    CheckHR(results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&m_shaderBlob), &shaderName));
    if (m_shaderBlob)
    {
        // save shader to cache
        auto savePath = GetCompiledPath(desc.path);

        std::vector<char> content;
        auto size = static_cast<size_t>(m_shaderBlob->GetBufferSize());
        content.resize(size);
        memcpy(content.data(), reinterpret_cast<const char*>(m_shaderBlob->GetBufferPointer()), size);

        Engine.FileIO().CreateDirectories(FileIO::Directory::None, CompiledFolder);

        Engine.FileIO().WriteBinaryFile(savePath, content);
    }

    auto* reflection = CreateReflection(m_shaderBlob, utils);
    ParseReflection(reflection);

    COMFREE(reflection);

    COMFREE(results);
    COMFREE(shaderName);

    COMFREE(includeHandler)
    COMFREE(utils);
    COMFREE(compiler);
}

SE::Shader::~Shader() { COMFREE(m_shaderBlob); }

CD3DX12_SHADER_BYTECODE SE::Shader::GetByteCode() const
{
    assert(m_shaderBlob && "Cannot create bytecode from invalid shader");
    CD3DX12_SHADER_BYTECODE bytecode;
    bytecode.BytecodeLength = m_shaderBlob->GetBufferSize();
    bytecode.pShaderBytecode = m_shaderBlob->GetBufferPointer();
    return bytecode;
}

bool SE::Shader::HasINputLayout() const
{
    if (m_type == ShaderDesc::ShaderType::Vertex)
    {
        return !m_layout.empty();
    }
    return false;
}

D3D12_INPUT_LAYOUT_DESC SE::Shader::GetInputLayout() const
{
    D3D12_INPUT_LAYOUT_DESC layout{};
    if (m_type == ShaderDesc::ShaderType::Vertex)
    {
        layout.NumElements = static_cast<UINT>(m_layout.size());
        layout.pInputElementDescs = m_layout.data();
    }

    return layout;
}

const std::vector<SE::ShaderParam>& SE::Shader::GetParams() const { return m_params; }

std::string SE::Shader::GetPath(const ShaderDesc& desc)
{
    if (desc.type == ShaderDesc::ShaderType::Vertex)
    {
        return Engine.FileIO().GetPath(desc.directory, desc.path) + " | Vertex";
    }
    else if (desc.type == ShaderDesc::ShaderType::Pixel)
    {
        return Engine.FileIO().GetPath(desc.directory, desc.path) + " | Pixel";
    }
    else if (desc.type == ShaderDesc::ShaderType::Domain)
    {
        return Engine.FileIO().GetPath(desc.directory, desc.path) + " | Domain";
    }
    else if (desc.type == ShaderDesc::ShaderType::Hull)
    {
        return Engine.FileIO().GetPath(desc.directory, desc.path) + " | Hull";
    }
    else if (desc.type == ShaderDesc::ShaderType::Compute)
    {
        return Engine.FileIO().GetPath(desc.directory, desc.path) + " | Compute";
    }
    return Engine.FileIO().GetPath(desc.directory, desc.path);
}

uint64_t SE::Shader::CalculateHash(const ShaderDesc& desc)
{
    auto hash = Hash(Engine.FileIO().GetPath(desc.directory, desc.path).c_str()).Value();
    SumHash(hash, desc.type);
    SumHash(hash, static_cast<uint64_t>(desc.directory));
    return hash;
}

uint64_t SE::Shader::GetHash() const { return m_hash; }

std::string SE::Shader::GetCompiledPath(const std::string&) const
{
    // using hash for storing precompiled paths
    const auto hashed = std::to_string(m_hash);
    return CompiledFolder + hashed + ".cso";
}

bool SE::Shader::IsPrecompiled(const std::string& path) const { return Engine.FileIO().Exists(GetCompiledPath(path)); }

bool SE::Shader::ShouldRecompile(const std::string& path, const std::string& shader) const
{
    if (!IsPrecompiled(path))
    {
        // if no precompiled version exist
        return true;
    }

    auto compiledTime = Engine.FileIO().LastModified(GetCompiledPath(path));
    auto hlslTime = Engine.FileIO().LastModified(shader);

    // compare modification time between compiled shader and hlsl file
    return compiledTime < hlslTime;
}

std::vector<LPCWSTR> SE::Shader::GetCompileCommands(const ShaderDesc& desc, const std::wstring& path)
{
    auto commands = std::vector<LPCWSTR>();

    // add support for defines
    // defines need -D + define

#if SHADER_PROFILE
    commands.reserve(10);
#else
    commands.reserve(8);
#endif

    commands.push_back(L"-E");
    commands.push_back(L"main");
    commands.push_back(L"-HV");
    commands.push_back(L"2021");
    commands.push_back(L"-T");

    switch (desc.type)
    {
        case ShaderDesc::ShaderType::Vertex:
            commands.push_back(L"vs_6_6");
            break;
        case ShaderDesc::ShaderType::Pixel:
            commands.push_back(L"ps_6_6");
            break;
        case ShaderDesc::ShaderType::Compute:
            commands.push_back(L"cs_6_6");
            break;
        case ShaderDesc::ShaderType::Hull:
            commands.push_back(L"hs_6_6");
            break;
        case ShaderDesc::ShaderType::Domain:
            commands.push_back(L"ds_6_6");
            break;
    }

#if SHADER_PROFILE
    // enables shader debugging using nvidia insigths or amd tools
    commands.push_back(L"-Zi");
    commands.push_back(L"-Qstrip_debug");
#endif
    commands.push_back(path.c_str());
    commands.push_back(L"-all-resources-bound");

    return commands;
}

ID3D12ShaderReflection* SE::Shader::CreateReflection(IDxcBlob* shaderBlob, IDxcUtils* utils)
{
    assert(shaderBlob && "Missing shader blob for reflection");
    assert(utils && "Missing utils for shader reflection creation");

    DxcBuffer reflectionBuffer{};
    reflectionBuffer.Ptr = shaderBlob->GetBufferPointer();
    reflectionBuffer.Size = shaderBlob->GetBufferSize();
    reflectionBuffer.Encoding = 0;

    ID3D12ShaderReflection* reflection = nullptr;
    CheckHR(utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection)));

    return reflection;
}

void SE::Shader::ParseReflection(ID3D12ShaderReflection* reflection)
{
    assert(reflection && "Missing reflection for parsing");

    D3D12_SHADER_DESC shaderDesc{};
    CheckHR(reflection->GetDesc(&shaderDesc));

    m_params.reserve(static_cast<size_t>(shaderDesc.BoundResources));

    uint32_t constantBuffer = 0;
    for (uint32_t i = 0; i < shaderDesc.BoundResources; i++)
    {
        // parse input layout
        D3D12_SHADER_INPUT_BIND_DESC resource{};
        CheckHR(reflection->GetResourceBindingDesc(i, &resource));

        ShaderParam param;
        param.SetName(resource.Name);

        switch (resource.Type)
        {
            case D3D_SIT_CBUFFER:  // constant buffers
            {
                auto* buffer = reflection->GetConstantBufferByIndex(constantBuffer);

                D3D12_SHADER_BUFFER_DESC bufferDesc{};
                CheckHR(buffer->GetDesc(&bufferDesc));

                if (bufferDesc.Size > MaxConstantSize)
                {
                    param.InitAsConstantBuffer(resource.BindPoint, resource.Space);
                }
                else
                {
                    param.InitAsRootConstant(resource.BindPoint, resource.Space, bufferDesc.Size);
                }
                constantBuffer++;
            }
            break;
            case D3D_SIT_TEXTURE:  // srv texture
            {
                param.InitAsTexture(resource.BindPoint, resource.Space);
            }
            break;
            case D3D_SIT_UAV_RWTYPED:  // uav texture
            {
                param.InitAsTextureUAV(resource.BindPoint, resource.Space);
            }
            break;
            case D3D_SIT_SAMPLER:  // sampler
            {
                param.InitAsSampler(resource.BindPoint, resource.Space);
            }
            break;
            case D3D_SIT_STRUCTURED:  // structured buffer
            {
                param.InitAsStructuredBuffer(resource.BindPoint, resource.Space);
            }
            break;
            case D3D_SIT_UAV_RWSTRUCTURED:  // uav buffer
            {
                param.InitAsStructuredBufferUAV(resource.BindPoint, resource.Space);
            }
            break;
            default:
                break;
        }

        m_params.push_back(param);
    }

    // generate input layout if vertex shader
    if (m_type == ShaderDesc::ShaderType::Vertex)
    {
        m_layout.reserve(shaderDesc.InputParameters);
        m_semantics.reserve(shaderDesc.InputParameters);
        for (uint32_t i = 0; i < shaderDesc.InputParameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
            reflection->GetInputParameterDesc(i, &paramDesc);

            // storing semantics names in local shader storage
            // after reflection com ptr is released all semantics names are released too in usage element.SemanticName =
            // paramDesc.SemanticName;
            m_semantics.emplace_back(paramDesc.SemanticName);

            D3D12_INPUT_ELEMENT_DESC element = {};
            element.SemanticName = m_semantics[i].c_str();
            element.SemanticIndex = paramDesc.SemanticIndex;
            element.InputSlot = 0;
            element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            element.InstanceDataStepRate = 0;
            element.Format = GetInputFormat(paramDesc.ComponentType, GetChannelSize(paramDesc.Mask));
            m_layout.push_back(element);
        }
    }
}

HRESULT __stdcall SE::CustomIncludeHandler::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
{
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> pEncoding = nullptr;
    std::string path = (WStringToString(pFilename));
    if (m_include_files.find(path) != m_include_files.end())
    {
        // Return empty string blob if this file has SEn included before
        static const char nullStr[] = " ";
        m_utils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, pEncoding.GetAddressOf());
        *ppIncludeSource = pEncoding.Detach();
        return S_OK;
    }

    HRESULT hr = m_utils->LoadFile(pFilename, nullptr, pEncoding.GetAddressOf());
    if (SUCCEEDED(hr))
    {
        m_include_files.insert(path);
        *ppIncludeSource = pEncoding.Detach();
    }
    return hr;
}

HRESULT __stdcall SE::CustomIncludeHandler::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == __uuidof(IDxcIncludeHandler) || riid == __uuidof(IUnknown))
    {
        *ppvObject = static_cast<IDxcIncludeHandler*>(this);
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG __stdcall SE::CustomIncludeHandler::AddRef() { return ++m_refCount; }

ULONG __stdcall SE::CustomIncludeHandler::Release()
{
    ULONG count = --m_refCount;
    if (count == 0) delete this;
    return count;
}

const std::vector<SE::ShaderParam>& SE::ShaderParamContainer::GetShaderParams() const { return m_params; }

void SE::ShaderParamContainer::AddParam(const ShaderParam& param)
{
    auto paramHash = m_paramHashMap.find(param.paramHash);
    auto nameHash = m_paramNameMap.find(param.nameHash.GetHash());

    if (paramHash != m_paramHashMap.end())
    {
        // found same param check name hash for better consistency in shaders
        const auto& storedParam = m_params[paramHash->second];
        if (storedParam.nameHash != param.nameHash)
        {
            Log::Warn("Same shader parameter in different shaders doesn't have same name {} name {}",
                      storedParam.nameHash.GetName(),
                      param.nameHash.GetName());
        }
        return;
    }

    if (nameHash != m_paramNameMap.end())
    {
        // param hash doesn't match but name was found
        const auto& storedParam = m_params[nameHash->second];
        Log::Error("Shader parameter shares name but not register, space or type");
        Log::Error("Change name or match register, space and type");
        Log::Error("Stored parameter register {}, space {}, type {}",
                   storedParam.GetRegister(),
                   storedParam.GetSpace(),
                   storedParam.GetType());
        Log::Error("Stored parameter register {}, space {}, type {}", param.GetRegister(), param.GetSpace(), param.GetType());
        assert(false && "shader parameter mismatch");
        return;
    }

    // no match add paramameter
    m_params.push_back(param);
    uint32_t index = static_cast<uint32_t>(m_params.size()) - 1;
    m_paramNameMap.insert({param.nameHash.GetHash(), index});
    m_paramHashMap.insert({param.paramHash, index});
}

void SE::ShaderParamContainer::AddShaderHash(uint64_t hash) { SumHash(m_shaderHash, hash); }
