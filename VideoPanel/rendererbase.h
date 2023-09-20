#pragma once
#include "pch.h"
#include "rendersurface.h"
#include <concrt.h>

namespace VideoPanel
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class RendererBase : public Windows::UI::Xaml::Controls::SwapChainPanel
	{
	public:
		void StartRenderingAsnyc();
		void StopRenderingAsnyc();
	private protected:
		RendererBase();
		virtual void _OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void _ChangeSize(uint16 width, uint16 height);
		virtual void _Update();

		GFX::RenderSurface m_surface;

		Concurrency::critical_section m_critsec;
		// Worker
		Windows::Foundation::IAsyncAction^ m_renderWorker;
	};
}
