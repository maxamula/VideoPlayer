#include "pch.h"
#include "shaders.h"
#include "nameof.hpp"

namespace VideoPanel
{
	extern HINSTANCE g_hInstance;
	namespace Shaders
	{
		namespace
		{
			IDxcBlob* g_engineShaders[ENGINE_SHADER::NUM_SHADERS];

			SHADER_TYPE::type GetShaderTypeFromName(const wchar_t* name)
			{
				if (wcsncmp(name, L"VS_", 3) == 0)
					return SHADER_TYPE::VERTEX;
				else if (wcsncmp(name, L"HS_", 3) == 0)
					return SHADER_TYPE::HULL;
				else if (wcsncmp(name, L"DS_", 3) == 0)
					return SHADER_TYPE::DOMAIN;
				else if (wcsncmp(name, L"GS_", 3) == 0)
					return SHADER_TYPE::GEOMETRY;
				else if (wcsncmp(name, L"PS_", 3) == 0)
					return SHADER_TYPE::PIXEL;
				else if (wcsncmp(name, L"CS_", 3) == 0)
					return SHADER_TYPE::COMPUTE;
				else if (wcsncmp(name, L"AS_", 3) == 0)
					return SHADER_TYPE::AMPLIFICATION;
				else if (wcsncmp(name, L"MS_", 3) == 0)
					return SHADER_TYPE::MESH;
				else
					return SHADER_TYPE::NUM_SHADER_TYPES;
			}
		}

		ShaderCompiler::ShaderCompiler()
		{
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)));
			ThrowIfFailed(m_utils->CreateDefaultIncludeHandler(&m_includeHandler));
		}

		ShaderCompiler::~ShaderCompiler()
		{
			RELEASE(m_compiler);
			RELEASE(m_utils);
			RELEASE(m_includeHandler);
		}

		IDxcBlob* ShaderCompiler::CompileShader(const void* pShaderSource, uint32 size, SHADER_TYPE::type shaderType)
		{
			IDxcBlobEncoding* shaderSource = nullptr;
			ThrowIfFailed(m_utils->CreateBlobFromPinned(pShaderSource, size, CP_UTF8, &shaderSource));
			assert(shaderSource);
			//create dxcbuffer
			DxcBuffer buffer;
			buffer.Encoding = CP_UTF8;
			buffer.Ptr = shaderSource->GetBufferPointer();
			buffer.Size = shaderSource->GetBufferSize();

			// compiler args
			LPCWSTR args[]
			{
				L"-E", L"main",
				L"-T", m_profiles[shaderType],
				DXC_ARG_ALL_RESOURCES_BOUND,
#if _DEBUG
				DXC_ARG_DEBUG,
				DXC_ARG_SKIP_OPTIMIZATIONS,
#else
				DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
				L"-Qstrip_reflect",
				L"-Qstrip_debug"
			};
			IDxcResult* compileResult = nullptr;
			ThrowIfFailed(m_compiler->Compile(&buffer, args, _countof(args), m_includeHandler, IID_PPV_ARGS(&compileResult)));
			assert(compileResult);
			IDxcBlob* shader = nullptr;
			IDxcBlobEncoding* errorBuffer = nullptr;
			compileResult->GetErrorBuffer(&errorBuffer);
			if (errorBuffer->GetBufferSize())
				throw std::runtime_error("Shader compilation error");
			errorBuffer->Release();

			ThrowIfFailed(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
			// Release interfaces
			RELEASE(shaderSource);
			RELEASE(compileResult);
			return shader;
		}

		D3D12_SHADER_BYTECODE GetEngineShader(ENGINE_SHADER::id id)
		{
			ThrowIfFailed(id < ENGINE_SHADER::NUM_SHADERS);
			IDxcBlob* shader = g_engineShaders[id];
			assert(shader && shader->GetBufferSize());
			return { shader->GetBufferPointer(), shader->GetBufferSize() };
		}

		void Initialize()
		{
			// loop through all dll resources
			for (uint32 shaderIndex = 0; shaderIndex < ENGINE_SHADER::NUM_SHADERS; shaderIndex++)
			{
				std::string_view view = nameof::nameof_enum<ENGINE_SHADER::id>(static_cast<ENGINE_SHADER::id>(shaderIndex));
				wchar_t resourceName[MAX_RESOURSE_NAME];
				MultiByteToWideChar(CP_UTF8, 0, view.data(), (int)view.size(), resourceName, MAX_RESOURSE_NAME);
				resourceName[view.size()] = L'\0';
				HRSRC resource = FindResource(g_hInstance, resourceName, L"ENGINE_SHADER");
				// load resource in memory
				assert(resource);
				HGLOBAL resourceData = LoadResource(g_hInstance, resource);
				assert(resourceData);
				const void* data = LockResource(resourceData);
				// get resource size
				uint32 size = SizeofResource(g_hInstance, resource);

				ShaderCompiler compiler{};
				assert(data);
				//compile shader
#ifdef _DEBUG
				char _debugMsg[20 + MAX_RESOURSE_NAME];
				sprintf(_debugMsg, "Compiling: %s", resourceName);
				OutputDebugStringA(_debugMsg);
#endif
				auto blob = compiler.CompileShader(data, size, GetShaderTypeFromName(resourceName));
				// save shader
				if (!blob || !blob->GetBufferSize())
					throw std::runtime_error("Failed to save shader");
				g_engineShaders[shaderIndex] = blob;
				// release resource memory and free resource
				UnlockResource(resourceData);
				FreeResource(resourceData);
			}
		}

		void Shutdown()
		{
			for (uint32 shaderIndex = 0; shaderIndex < ENGINE_SHADER::NUM_SHADERS; shaderIndex++)
				RELEASE(g_engineShaders[shaderIndex]);
		}
	}
}