#pragma once
#include "graphics.h"
// directx compiling lib
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
#undef DOMAIN

namespace VideoPanel::Shaders
{
	struct SHADER_TYPE
	{
		enum type : uint32
		{
			VERTEX,
			HULL,
			DOMAIN,
			GEOMETRY,
			PIXEL,
			COMPUTE,
			AMPLIFICATION,
			MESH,
			NUM_SHADER_TYPES
		};
	};

	struct ENGINE_SHADER
	{
		enum id : uint32
		{
			VS_PLAYER,
			PS_PLAYER,
			NUM_SHADERS
		};
	};

	class ShaderCompiler
	{
	public:
		ShaderCompiler();
		~ShaderCompiler();
		DISABLE_MOVE_COPY(ShaderCompiler);
		[[nodiscard]] IDxcBlob* CompileShader(const void* pShaderSource, uint32 size, SHADER_TYPE::type shaderType);
	private:
		const wchar_t* m_profiles[SHADER_TYPE::NUM_SHADER_TYPES]{ L"vs_6_6", L"hs_6_6", L"ds_6_6", L"gs_6_6", L"ps_6_6", L"cs_6_6", L"as_6_6", L"ms_6_6" };

		IDxcCompiler3* m_compiler;
		IDxcUtils* m_utils;
		IDxcIncludeHandler* m_includeHandler;
	};

	[[nodiscard]] D3D12_SHADER_BYTECODE GetEngineShader(ENGINE_SHADER::id id);
	void Initialize();
	void Shutdown();
}

