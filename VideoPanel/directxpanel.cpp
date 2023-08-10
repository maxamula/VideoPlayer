#include "pch.h"
#include "directxpanel.h"
#include "d3d.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;
using namespace Concurrency;

namespace VideoPanel
{
    void DirectXPanel::StartRenderingAsnyc()
    {
        auto workItemHandler = ref new WorkItemHandler([this](Windows::Foundation::IAsyncAction^ action)
            {
                while (action->Status == Windows::Foundation::AsyncStatus::Started)
                {
                    {
                        critical_section::scoped_lock lock(m_critsec);
                        _Update();
                    }
                    d3d::Instance().dxgiOutput->WaitForVBlank();
                }
            });
        m_renderWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
    }

    void DirectXPanel::StopRenderingAsnyc()
    {
        if(m_renderWorker)
            m_renderWorker->Cancel();
    }

    DirectXPanel::DirectXPanel() : SwapChainPanel()
    {
        this->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(this, &DirectXPanel::_OnSizeChanged);
    }

    void DirectXPanel::_OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
    {
        critical_section::scoped_lock lock(m_critsec);
        _ChangeSize(e->NewSize.Width, e->NewSize.Height);
    }

    void DirectXPanel::_ChangeSize(uint32 width, uint32 height)
    {
        if (m_width != width || m_height != height)
        {
            m_d2dRenderTarget.Reset();
            m_d3dRenderTarget.Reset();
            m_width = width;
            m_height = height;
            _CreateSizeDependentResources();
        }
    }

    void DirectXPanel::_Update()
    {
        throw ref new Platform::NotImplementedException();
    }

    void DirectXPanel::_CreateSizeDependentResources()
    {
        // If the swap chain already exists, then resize it.
        if (m_swapChain != nullptr)
        {
            ThrowIfFailed(m_swapChain->ResizeBuffers(2, m_width, m_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0));
        }
        else // Otherwise, create a new one.
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
            swapChainDesc.Width = m_width;
            swapChainDesc.Height = m_height;
            swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            swapChainDesc.Flags = 0;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

            ComPtr<IDXGIDevice1> dxgiDevice;
            ThrowIfFailed(d3d::Instance().d3dDevice.As(&dxgiDevice));

            // Get adapter.
            ComPtr<IDXGIAdapter> dxgiAdapter;
            ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

            // Get factory.
            ComPtr<IDXGIFactory2> dxgiFactory;
            ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

            ComPtr<IDXGISwapChain1> swapChain;
            // Create swap chain.
            ThrowIfFailed(dxgiFactory->CreateSwapChainForComposition(d3d::Instance().d3dDevice.Get(), &swapChainDesc, nullptr, &swapChain));
            swapChain.As(&m_swapChain);

            ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));

            Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]()
            {
                ComPtr<ISwapChainPanelNative> panelNative;
                ThrowIfFailed(reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&panelNative)));
                ThrowIfFailed(panelNative->SetSwapChain(m_swapChain.Get()));
            }, Platform::CallbackContext::Any));
        }

        ComPtr<IDXGISurface> dxgiBackBuffer;
        ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)));
        D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
        ThrowIfFailed(d3d::Instance().d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiBackBuffer.Get(), &renderTargetProperties, &m_d2dRenderTarget));

        ComPtr<ID3D11Texture2D> pd3dBackBuf = nullptr;
        ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pd3dBackBuf)));
        ThrowIfFailed(d3d::Instance().d3dDevice->CreateRenderTargetView(pd3dBackBuf.Get(), nullptr, &m_d3dRenderTarget));
    }
}