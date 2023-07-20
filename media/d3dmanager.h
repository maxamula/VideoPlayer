#pragma once
#include "common.h"

namespace media
{
	class D3DManager
	{
	public:
		D3DManager();
		~D3DManager();
		DISABLE_COPY(D3DManager);

		void SetDX11RenderTargetView(ID3D11RenderTargetView** ppRtv);
		HRESULT CreateRenderTarget(HWND hWnd, uint16_t width, uint16_t height, IDXGISwapChain3** ppSwapchain, ID2D1RenderTarget** pp2dRenderTraget, ID3D11RenderTargetView** ppRenderTarget);
		HRESULT ResizeSwapchain(IDXGISwapChain3* pSwapchain, uint16_t width, uint16_t height, ID2D1RenderTarget** pp2dRenderTraget, ID3D11RenderTargetView** ppRenderTarget);

		inline ID3D11Device* Device() { return m_device.Get(); }
		inline ID3D11DeviceContext* Context() { return m_context.Get(); }
		inline IXAudio2* Audio() { return m_audio.Get(); }
		inline IXAudio2MasteringVoice* MasteringVoice() { return m_masteringVoice; }
	private:
		HRESULT _Initialize() noexcept;

		ComPtr<ID3D11Device> m_device = nullptr;
		ComPtr<ID3D11DeviceContext> m_context = nullptr;
		ComPtr<ID2D1Factory> m_2dFactory = nullptr;
		ComPtr<ID2D1Device> m_2dDevice = nullptr;
		ComPtr<ID2D1DeviceContext> m_2dContext = nullptr;
		ComPtr<IDXGIFactory7> m_dxgiFactory = nullptr;
		ComPtr<IDXGIAdapter4> m_dxgiAdapter = nullptr;

		ComPtr<IXAudio2> m_audio = nullptr;
		IXAudio2MasteringVoice* m_masteringVoice = nullptr;
	};
}

