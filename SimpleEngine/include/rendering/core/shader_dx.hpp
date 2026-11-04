#pragma once

#include "rendering/common_dx.hpp"
#include "core/resource.hpp"

#include "dxc/d3d12shader.h"
#include "dxc/dxcapi.h"
#include <atlbase.h>

#include <unordered_set>
#include <unordered_map>
#include <string>

namespace SE
{
class ShaderParamContainer
{
public:
    const std::vector<ShaderParam>& GetShaderParams() const;

    ShaderParamContainer() = default;
    ~ShaderParamContainer() = default;

    void AddParam(const ShaderParam& param);

    void AddShaderHash(uint64_t hash);

    uint64_t GetShaderHash() const { return m_shaderHash; }

    const std::unordered_map<uint64_t, uint32_t>& GetNameMap() const { return m_paramNameMap; }
    const std::unordered_map<uint64_t, uint32_t>& GetParamMap() const { return m_paramNameMap; }

private:
    uint64_t m_shaderHash = 0;
    std::vector<ShaderParam> m_params;
    // hash to index
    std::unordered_map<uint64_t, uint32_t> m_paramHashMap;
    std::unordered_map<uint64_t, uint32_t> m_paramNameMap;
};

// heleper class for includes inside hlsl files
class CustomIncludeHandler : public IDxcIncludeHandler
{
public:
    CustomIncludeHandler() = default;
    virtual ~CustomIncludeHandler() = default;

    // loads include file
    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename,
                                         _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override;

    // needed overides
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR * __RPC_FAR * ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

private:
    friend class Shader;

    // included files to prevent double includes
    std::unordered_set<std::string> m_include_files;

    // shader compiler utilities
    IDxcUtils* m_utils = nullptr;

    // reference count
    ULONG m_refCount = 1;
};

class Shader : public Resource
{
public:
    DELETEMOVECOPY(Shader);

    Shader(const ShaderDesc& desc);
    ~Shader();

    CD3DX12_SHADER_BYTECODE GetByteCode() const;

    bool HasINputLayout() const;

    D3D12_INPUT_LAYOUT_DESC GetInputLayout() const;

    const std::vector<ShaderParam>& GetParams() const;

    static std::string GetPath(const ShaderDesc& desc);

    static uint64_t CalculateHash(const ShaderDesc& desc);

    uint64_t GetHash() const;

    bool IsCompiled() const { return m_compiled; }

private:
    std::string GetCompiledPath(const std::string& path) const;

    bool IsPrecompiled(const std::string& path) const;

    bool ShouldRecompile(const std::string& path, const std::string& shader) const;

    static std::vector<LPCWSTR> GetCompileCommands(const ShaderDesc& desc, const std::wstring& path);

    static ID3D12ShaderReflection* CreateReflection(IDxcBlob* shaderBlob, IDxcUtils* utils);

    void ParseReflection(ID3D12ShaderReflection* reflection);

    bool m_compiled = false;

    uint32_t m_type = 0;

    uint64_t m_hash = 0;

    IDxcBlob* m_shaderBlob = nullptr;

    std::vector<ShaderParam> m_params;

    std::vector<std::string> m_semantics;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_layout;
};
}  // namespace SE