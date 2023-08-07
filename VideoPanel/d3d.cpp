#include "pch.h"
#include "d3d.h"

namespace VideoPanel
{
	d3d::d3d()
	{
        d3dDevice.Reset();
        d3dContext.Reset();
        d2dFactory.Reset();
        d2dDevice.Reset();
        d2dContext.Reset();

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
                if (SUCCEEDED(D3D11CreateDevice(adapter1.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, &featureLevel, 1, D3D11_SDK_VERSION, &d3dDevice, NULL, &d3dContext)))
                {
                    adapter1->EnumOutputs(0, &dxgiOutput);
                    break;
                }
            }
        }

        // D2D stuff here
        ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.GetAddressOf()));
        D2D1_CREATION_PROPERTIES d2dCreationProps = { D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_INFORMATION };
        ComPtr<IDXGIDevice> pDxgiDevice = nullptr;
        ThrowIfFailed(d3dDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice)));
        ThrowIfFailed(D2D1CreateDevice(pDxgiDevice.Get(), d2dCreationProps, &d2dDevice));
        ThrowIfFailed(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext));

        ThrowIfFailed(MFStartup(MF_VERSION));
        ThrowIfFailed(XAudio2Create(&audio, 0, XAUDIO2_DEFAULT_PROCESSOR));
#ifdef _DEBUG
        XAUDIO2_DEBUG_CONFIGURATION debugConfig = { 0 };
        debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
        debugConfig.BreakMask = 0;
        debugConfig.LogThreadID = TRUE;
        debugConfig.LogFileline = TRUE;
        debugConfig.LogFunctionName = TRUE;
        debugConfig.LogTiming = TRUE;
        audio->SetDebugConfiguration(&debugConfig);
#endif
        ThrowIfFailed(audio->CreateMasteringVoice(&masteringVoice));
	}

	d3d::~d3d()
	{
        ThrowIfFailed(MFShutdown());
		masteringVoice->DestroyVoice();
	}
}


