#include "pch.h"
#include "directxpanel.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;
using namespace Concurrency;

namespace VideoPanel
{
    ComPtr<ID3D11Device> DirectXPanel::s_d3dDevice = nullptr;
    ComPtr<ID3D11DeviceContext> DirectXPanel::s_d3dContext = nullptr;

    ComPtr<ID2D1Factory> DirectXPanel::s_d2dFactory = nullptr;
    ComPtr<ID2D1Device> DirectXPanel::s_d2dDevice = nullptr;
    ComPtr<ID2D1DeviceContext> DirectXPanel::s_d2dContext = nullptr;

    ComPtr<IDXGIOutput> DirectXPanel::s_dxgiOutput = nullptr;

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
                    s_dxgiOutput->WaitForVBlank();
                }
            });
        m_renderWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
    }

    void DirectXPanel::StopRenderingAsnyc()
    {
        m_renderWorker->Cancel();
    }

    void DirectXPanel::Initialize()
    {
        s_d3dDevice.Reset();
        s_d3dContext.Reset();
        s_d2dFactory.Reset();
        s_d2dDevice.Reset();
        s_d2dContext.Reset();
        {
            // Create factory
            ComPtr<IDXGIFactory1> pFactory = nullptr;
            ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory)));
            // Enumerate adapters and find suitable
            ComPtr<IDXGIAdapter1> adapter1 = nullptr;
            for (UINT i = 0; pFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
            {
#ifdef _DEBUG
                UINT flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
                UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif
                const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
                // Check if adapter supports d3d12
                if (SUCCEEDED(D3D11CreateDevice(adapter1.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, &featureLevel, 1, D3D11_SDK_VERSION, &s_d3dDevice, NULL, &s_d3dContext)))
                {
                    adapter1->EnumOutputs(0, &s_dxgiOutput);
                    break;
                }
            }
        }

        // D2D stuff here
        ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, s_d2dFactory.GetAddressOf()));
        D2D1_CREATION_PROPERTIES d2dCreationProps = { D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_INFORMATION };
        ComPtr<IDXGIDevice> pDxgiDevice = nullptr;
        ThrowIfFailed(s_d3dDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice)));
        ThrowIfFailed(D2D1CreateDevice(pDxgiDevice.Get(), d2dCreationProps, &s_d2dDevice));
        ThrowIfFailed(s_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &s_d2dContext));
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
            HRESULT hr = m_swapChain->ResizeBuffers(2, m_width, m_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                Initialize();
                _CreateSizeDependentResources();
                return;
            }
            else
            {
                ThrowIfFailed(hr);
            }
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
            ThrowIfFailed(s_d3dDevice.As(&dxgiDevice));

            // Get adapter.
            ComPtr<IDXGIAdapter> dxgiAdapter;
            ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

            // Get factory.
            ComPtr<IDXGIFactory2> dxgiFactory;
            ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

            ComPtr<IDXGISwapChain1> swapChain;
            // Create swap chain.
            ThrowIfFailed(dxgiFactory->CreateSwapChainForComposition(s_d3dDevice.Get(), &swapChainDesc, nullptr, &swapChain));
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
        ThrowIfFailed(s_d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiBackBuffer.Get(), &renderTargetProperties, &m_d2dRenderTarget));

        ComPtr<ID3D11Texture2D> pd3dBackBuf = nullptr;
        ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pd3dBackBuf)));
        ThrowIfFailed(s_d3dDevice->CreateRenderTargetView(pd3dBackBuf.Get(), nullptr, &m_d3dRenderTarget));
    }
}