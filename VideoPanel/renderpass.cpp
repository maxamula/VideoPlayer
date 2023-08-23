#include "pch.h"
#include "renderpass.h"
#include <random>

#define NUM_PIXEL_SHADER_CONSTANTS 2

#if _DEBUG
constexpr float CLEAR_COLOR[4]{ 0.3f, 0.0f, 0.0f, 0.0f };
#else
constexpr float CLEAR_COLOR[4]{ };
#endif

namespace VideoPanel::GFX::Render
{
	namespace
	{
		ID3D12RootSignature* g_gpassRootsig = nullptr;
		ID3D12PipelineState* g_gpassPso = nullptr;
		DESCRIPTOR_HANDLE g_samplerHandle{};

		const DXGI_FORMAT g_mainBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		const DXGI_FORMAT g_depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

		enum ROOT_PARAM : uint32
		{
			ROOT_PARAM_PIXEL_SHADER_CONSTANTS = 0,
			ROOT_PARAM_SAMPLER = 1,
		};

		struct PIXEL_SHADER_CONSTANTS
		{
			uint32 srvTextureIndex = 0;
			uint32 brightness = 100;
		};

		static_assert(sizeof(PIXEL_SHADER_CONSTANTS)/4 == NUM_PIXEL_SHADER_CONSTANTS);

		void CreatePsoAndRootsig()
		{
			if(g_gpassPso || g_gpassRootsig)
				throw std::runtime_error("GPass PSO or root signature already created");
			RootParameter params[2] = {};
			params[ROOT_PARAM_PIXEL_SHADER_CONSTANTS].AsConstants(NUM_PIXEL_SHADER_CONSTANTS, D3D12_SHADER_VISIBILITY_ALL, 0);

			GFX::DescriptorRange samplerRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
			params[ROOT_PARAM_SAMPLER].AsDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, &samplerRange, 1);

			RootSignature rootSig(&params[0], (uint32_t)std::size(params));
			g_gpassRootsig = rootSig.Create();
			assert(g_gpassRootsig);
			SET_NAME(g_gpassRootsig, L"GPass Root signature");

			// PSO
			struct
			{
				SubRootSignature rootSig{ g_gpassRootsig };
				SubVertexShader vs{ Shaders::GetEngineShader(Shaders::ENGINE_SHADER::VS_PLAYER) };
				SubPixelShader ps{ Shaders::GetEngineShader(Shaders::ENGINE_SHADER::PS_PLAYER) };
				SubPrimitiveTopology topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
				SubRenderTargetFormats rtvFormats;
				SubDepthStenciFormat dsvFormat{ g_depthBufferFormat };
				SubRasterizer rasterizer{ GFX::RASTERIZER_STATE.NO_CULL };
				SubDepthStencil1 depthStencil{ GFX::DEPTH_STATE.DISABLED };
			} stream;
			D3D12_RT_FORMAT_ARRAY rtvFormats = {};
			rtvFormats.NumRenderTargets = 1;
			rtvFormats.RTFormats[0] = g_mainBufferFormat;

			stream.rtvFormats = rtvFormats;

			g_gpassPso = CreatePSO(&stream, sizeof(stream));
			SET_NAME(g_gpassPso, L"GPass PSO");
			if(!g_gpassPso || !g_gpassRootsig)
				throw std::runtime_error("Failed to create GPass Rootsig or PSO");
		}
	}



	void Initialize()
	{
		CreatePsoAndRootsig();
		// Create videoframe sampler
		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.BorderColor[0] = 1.0f;
		samplerDesc.BorderColor[1] = 1.0f;
		samplerDesc.BorderColor[2] = 1.0f;
		samplerDesc.BorderColor[3] = 1.0f;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

		// Allocate descriptor
		g_samplerHandle = g_samplerHeap.Allocate();

		device->CreateSampler(&samplerDesc, g_samplerHandle.CPU);
	}

	void Shutdown()
	{
		RELEASE(g_gpassPso);
		RELEASE(g_gpassRootsig);
		g_samplerHeap.Free(g_samplerHandle);
	}

	void Render(ID3D12GraphicsCommandList6* cmd, uint32_t videoframeSRVIndex, const RENDER_TARGET rtv, D3D12_VIEWPORT viewport, D3D12_RECT scissors, uint32 brightness)
	{
		// Params
		PIXEL_SHADER_CONSTANTS pixelShaderConstants = { videoframeSRVIndex, brightness };
		g_cmdQueue.BeginFrame();

		TransitionResource(cmd, rtv.resource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		ID3D12DescriptorHeap* heaps[] = { g_srvHeap.GetDescriptorHeap(), g_samplerHeap.GetDescriptorHeap() };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);

		cmd->ClearRenderTargetView(rtv.allocation.CPU, CLEAR_COLOR, 0, nullptr);
		cmd->RSSetViewports(1, &viewport);
		cmd->RSSetScissorRects(1, &scissors);
		cmd->OMSetRenderTargets(1, &rtv.allocation.CPU, FALSE, nullptr);

		cmd->SetGraphicsRootSignature(g_gpassRootsig);
		cmd->SetPipelineState(g_gpassPso);

		cmd->SetGraphicsRoot32BitConstants(ROOT_PARAM_PIXEL_SHADER_CONSTANTS, NUM_PIXEL_SHADER_CONSTANTS, &pixelShaderConstants, 0);
		cmd->SetGraphicsRootDescriptorTable(ROOT_PARAM_SAMPLER, g_samplerHandle.GPU);

		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawInstanced(6, 1, 0, 0);
		TransitionResource(cmd, rtv.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		g_cmdQueue.EndFrame();
	}

	void Clear(ID3D12GraphicsCommandList6* cmd, const RENDER_TARGET rtv)
	{
		g_cmdQueue.BeginFrame();
		TransitionResource(cmd, rtv.resource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd->ClearRenderTargetView(rtv.allocation.CPU, CLEAR_COLOR, 0, nullptr);
		TransitionResource(cmd, rtv.resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		g_cmdQueue.EndFrame();
	}
}