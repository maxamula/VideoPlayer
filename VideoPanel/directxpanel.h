#pragma once
#include "pch.h"
#include <concrt.h>

namespace VideoPanel
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class DirectXPanel : public Windows::UI::Xaml::Controls::SwapChainPanel
	{
	public:
		void StartRenderingAsnyc();
		void StopRenderingAsnyc();
	internal:
		static void Initialize();
	private protected:


		static ComPtr<ID3D11Device> s_d3dDevice;
		static ComPtr<ID3D11DeviceContext> s_d3dContext;

		static ComPtr<ID2D1Factory> s_d2dFactory;
		static ComPtr<ID2D1Device> s_d2dDevice;
		static ComPtr<ID2D1DeviceContext> s_d2dContext;

		static ComPtr<IDXGIOutput> s_dxgiOutput;

		DirectXPanel();
		virtual void _OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void _ChangeSize(uint32 width, uint32 height);
		virtual void _Update();
		void _CreateSizeDependentResources();

		ComPtr<IDXGISwapChain2> m_swapChain;

		ComPtr<ID2D1RenderTarget> m_d2dRenderTarget = nullptr;
		ComPtr<ID3D11RenderTargetView> m_d3dRenderTarget = nullptr;

		Concurrency::critical_section m_critsec;
		// Worker
		Windows::Foundation::IAsyncAction^ m_renderWorker;

		uint32 m_height = 0;
		uint32 m_width = 0;
	};
}
