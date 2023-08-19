#include "pch.h"
#include "rendererbase.h"
#include "d3d.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;
using namespace Concurrency;

namespace VideoPanel
{
    void RendererBase::StartRenderingAsnyc()
    {
        auto workItemHandler = ref new WorkItemHandler([this](Windows::Foundation::IAsyncAction^ action)
            {
                while (action->Status == Windows::Foundation::AsyncStatus::Started)
                {
                    {
                        critical_section::scoped_lock lock(m_critsec);
                        _Update();
                    }
                    GFX::dxgiOutput->WaitForVBlank();
                }
            });
        m_renderWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::Low, WorkItemOptions::TimeSliced);
    }

    void RendererBase::StopRenderingAsnyc()
    {
        if(m_renderWorker)
            m_renderWorker->Cancel();
    }

    RendererBase::RendererBase() : SwapChainPanel(), m_surface(100, 100)
    {
        this->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(this, &RendererBase::_OnSizeChanged);
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]()
            {
                ComPtr<ISwapChainPanelNative> panelNative;
                ThrowIfFailed(reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&panelNative)));
                ThrowIfFailed(panelNative->SetSwapChain(m_surface.GetSwapChain()));
            }, Platform::CallbackContext::Any));
    }

    void RendererBase::_OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
    {
        critical_section::scoped_lock lock(m_critsec);
        _ChangeSize(e->NewSize.Width, e->NewSize.Height);
    }

    void RendererBase::_ChangeSize(uint16 width, uint16 height)
    {
        if (m_surface.GetWidth() != width || m_surface.GetHeight() != height)
        {
            m_surface.Resize(width, height);
        }
    }

    void RendererBase::_Update()
    {
        throw ref new Platform::NotImplementedException();
    }
}