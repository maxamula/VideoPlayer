#include "pch.h"
#include "rendersurface.h"
//#include "gpass.h"
#include "postprocess.h"

namespace VideoPanel::GFX
{
	RenderSurface::RenderSurface(uint16 width, uint16 height)
		: m_width(width), m_height(height), m_viewport({ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f }), m_scissors({ 0, 0, (LONG)width, (LONG)height })
	{
		Release();
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = width;
		scDesc.Height = height;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.Stereo = false;
		scDesc.SampleDesc.Count = 1;
		scDesc.SampleDesc.Quality = 0;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = BACKBUFFER_COUNT;
		scDesc.Scaling = DXGI_SCALING_STRETCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		ComPtr<IDXGISwapChain1> pSwap = nullptr;
		ThrowIfFailed(dxgiFactory->CreateSwapChainForComposition(g_cmdQueue.GetCommandQueue(), &scDesc, nullptr, &pSwap));
		ThrowIfFailed(pSwap->QueryInterface(IID_PPV_ARGS(&m_pSwap)));
		m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex();
		_CreateRendertargetViews();
	}

	void RenderSurface::Release()
	{
		g_cmdQueue.Flush();
		for (int i = 0; i < BACKBUFFER_COUNT; ++i)
		{
			RELEASE(m_renderTargets[i].resource);
			g_rtvHeap.Free(m_renderTargets[i].allocation);
			m_renderTargets[i].allocation = {};
		}
		RELEASE(m_pSwap);
	}

	void RenderSurface::Resize(uint16 width, uint16 height)
	{
		g_cmdQueue.Flush();
		assert(m_pSwap != NULL);
		m_width = width;
		m_height = height;
		m_viewport.Width = (float)width;
		m_viewport.Height = (float)height;
		m_scissors.right = (LONG)width;
		m_scissors.bottom = (LONG)height;
		for (int i = 0; i < BACKBUFFER_COUNT; ++i)
		{
			RELEASE(m_renderTargets[i].resource);
		}
		ThrowIfFailed(m_pSwap->ResizeBuffers(BACKBUFFER_COUNT, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
		m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex();

		for (int i = 0; i < BACKBUFFER_COUNT; ++i)
		{
			ThrowIfFailed(m_pSwap->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&m_renderTargets[i].resource));
			wchar_t name[17];
			swprintf_s(name, L"Backbuffer (%d)", i);
			SET_NAME(m_renderTargets[i].resource, name);
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			device->CreateRenderTargetView(m_renderTargets[i].resource, &rtvDesc, m_renderTargets[i].allocation.CPU);
		}
		OutputDebugString(L"D3D12 Surface resized.\n");
	}

	void RenderSurface::_CreateRendertargetViews()
	{
		assert(m_pSwap != NULL);
		for (int i = 0; i < BACKBUFFER_COUNT; ++i)
		{
			ThrowIfFailed(m_pSwap->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&m_renderTargets[i].resource));
			INDEXED_NAME_BUFFER(17);
			SET_NAME_INDEXED(m_renderTargets[i].resource, L"Backbuffer", i);
			m_renderTargets[i].allocation = g_rtvHeap.Allocate();
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			device->CreateRenderTargetView(m_renderTargets[i].resource, &rtvDesc, m_renderTargets[i].allocation.CPU);
		}
	}
}