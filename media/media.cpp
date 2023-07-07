#include "media.h"
#include "videosurface.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace media
{
	namespace
	{
		ID3D11Device* g_pDevice = nullptr;
		ID3D11DeviceContext* g_pContext = nullptr;

		ID2D1Factory* g_pD2DFactory = nullptr;
		ID2D1Device* g_pD2DDevice = nullptr;
		ID2D1DeviceContext* g_pD2DContext = nullptr;

		IDXGIFactory7* dxgiFactory = nullptr;
		IDXGIAdapter4* dxgiAdapter = nullptr;

		media::SurfaceManager g_surfManager;

		std::unique_ptr<std::thread> g_rendererThread{};
		std::atomic<bool> g_bLaunched{false};

		HRESULT CreateMediaSource(const wchar_t* szwFilePath, IMFMediaSource** ppMediaSource)
		{
			HRESULT hr = S_OK;
			ComPtr<IMFSourceResolver> pResolver = nullptr;
			hr = MFCreateSourceResolver(&pResolver);
			if (FAILED(hr))
				return hr;
			if (!ppMediaSource)
				return E_POINTER;
			MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
			IUnknown* pSource = nullptr;
			if (pResolver->CreateObjectFromURL(szwFilePath, MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, &pSource) != S_OK)
				return E_INVALIDARG;
			assert(ObjectType != MF_OBJECT_INVALID);
			hr = pSource->QueryInterface(IID_PPV_ARGS(ppMediaSource));
			return hr;
		}
	}


	HRESULT Initialize()
	{
		HRESULT hr = S_OK;
		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(hr)) return hr;
		hr = MFStartup(MF_VERSION);
		if (FAILED(hr)) return hr;

		if (g_pDevice)
			Shutdown();
		{
			// Create factory
			hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
			if (FAILED(hr)) return hr;
			// Enumerate adapters and find suitable
			IDXGIAdapter1* adapter1 = nullptr;
			for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
#ifdef _DEBUG
				UINT flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
				UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif
				const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
				// Check if adapter supports d3d12
				if (SUCCEEDED(D3D11CreateDevice(adapter1, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, &featureLevel, 1, D3D11_SDK_VERSION, &g_pDevice, NULL, &g_pContext)))
				{
					hr = adapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter)); // Get adapter
					if (FAILED(hr)) return hr;
					break;
				}
			}
		}

		// D2D stuff here
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
		if (FAILED(hr)) return hr;
		D2D1_CREATION_PROPERTIES d2dCreationProps = { D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_INFORMATION };
		ComPtr<IDXGIDevice> pDxgiDevice = nullptr;
		hr = g_pDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
		if (FAILED(hr)) return hr;
		hr = D2D1CreateDevice(pDxgiDevice.Get(), d2dCreationProps, &g_pD2DDevice);
		if (FAILED(hr)) return hr;
		hr = g_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &g_pD2DContext);

		// Init rendering thread
		g_bLaunched = true;
		g_rendererThread = std::make_unique<std::thread>([&]()
			{
				while (g_bLaunched)
				{
					for (const auto& it : g_surfManager)
					{
						VIDEO_SURFACE& surface = g_surfManager.Get(it.first);
						if (surface.state == PLAYER_STATE_INVALID)
							continue;
						if (surface.state == PLAYER_STATE_RESIZING)
						{
							SetEvent(surface.hResizeEvent);
							surface.state = PLAYER_STATE_INVALID;
							continue;
						}
						surface.p2dTarget->BeginDraw();
						if (surface.state == PLAYER_STATE_IDLE)
						{
							surface.p2dTarget->Clear(D2D1::ColorF(D2D1::ColorF::Blue));
							surface.p2dTarget->EndDraw();
							surface.pSwap->Present(0, 0);
							continue;
						}

						if (surface.state == PLAYER_STATE_PAUSED)
						{

						}

						if (surface.state == PLAYER_STATE_PLAYING)
						{
							// Get and convert the frame
							DWORD dwStreamIdx = 0, dwStreamFlags = 0;
							ComPtr<IMFSample> pSample;
							surface.pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &dwStreamIdx, &dwStreamFlags, &surface.currentPos, &pSample);


							ComPtr<IMFMediaType> pMediaType = nullptr;
							surface.pReader->GetCurrentMediaType(dwStreamIdx, &pMediaType);
							UINT32 width = 0, height = 0;
							MFGetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);

							ComPtr<IMFMediaBuffer> buffer;
							pSample->ConvertToContiguousBuffer(&buffer);
							ComPtr<IMFDXGIBuffer> dxgiBuffer;
							buffer.As(&dxgiBuffer);
							BYTE* pBuffer = nullptr;
							DWORD bufferSize = 0;
							buffer->Lock(&pBuffer, nullptr, &bufferSize);

							D2D1_BITMAP_PROPERTIES bitmapProperties;
							bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
							bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
							bitmapProperties.dpiX = 96.0f;
							bitmapProperties.dpiY = 96.0f;

							surface.p2dTarget->CreateBitmap(D2D1::SizeU(width, height), pBuffer, width * 4, bitmapProperties, &surface.pLastFrame);
							buffer->Unlock();	
						}

						if (surface.pLastFrame)
						{
							D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, surface.width, surface.height);
							surface.p2dTarget->DrawBitmap(surface.pLastFrame, &destRect);
						}
						surface.p2dTarget->EndDraw();

						if (surface.overlayDuration)
						{
							g_pContext->OMSetRenderTargets(1, &surface.pTarget, nullptr);
							ImGui::SetCurrentContext(surface.overlayContext);
							ImGui_ImplDX11_NewFrame();
							ImGui_ImplWin32_NewFrame();
							ImGui::NewFrame();
							ImGui::SetNextWindowSize(ImVec2(surface.width, 40));
							ImGui::SetNextWindowPos(ImVec2(0, surface.height - 40));
							ImGui::Begin("Playback control", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
							ImGui::SetNextItemWidth(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2);
							if (ImGui::SliderScalar("##time", ImGuiDataType_S64, &surface.currentPos, &VIDEO_SURFACE::PLAYBACK_START, &surface.duration))
							{

							}
							ImGui::End();
							ImGui::Render();
							ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
						}
						surface.pSwap->Present(0, 0);
					}
				}
			});
		return hr;
	}

	HRESULT CreateRenderTarget(HWND hWnd, uint16_t width, uint16_t height, uint32_t* pSurfaceId)
	{
		assert(hWnd != NULL && hWnd != INVALID_HANDLE_VALUE);
		HRESULT hr = S_OK;

		VIDEO_SURFACE videoSurface{};

		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = width;
		scDesc.Height = height;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.Stereo = false;
		scDesc.SampleDesc.Count = 1;
		scDesc.SampleDesc.Quality = 0;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = 2;
		scDesc.Scaling = DXGI_SCALING_STRETCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		ComPtr<IDXGISwapChain1> pSwap = nullptr;
		hr = dxgiFactory->CreateSwapChainForHwnd(g_pDevice, hWnd, &scDesc, nullptr, nullptr, &pSwap);
		if (FAILED(hr)) return hr;
		hr = pSwap->QueryInterface(IID_PPV_ARGS(&videoSurface.pSwap)) == S_OK;
		if (FAILED(hr)) return hr;

		videoSurface.width = width;
		videoSurface.height = height;

		ComPtr<IDXGISurface> pDxgiSurface = nullptr;
		hr = videoSurface.pSwap->GetBuffer(0, IID_PPV_ARGS(&pDxgiSurface));
		if (FAILED(hr)) return hr;
		D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
		hr = g_pD2DFactory->CreateDxgiSurfaceRenderTarget(pDxgiSurface.Get(), &renderTargetProperties, &videoSurface.p2dTarget);
		if (FAILED(hr)) return hr;

		ComPtr<ID3D11Texture2D> pd3dBackBuf = nullptr;
		hr = videoSurface.pSwap->GetBuffer(0, IID_PPV_ARGS(&pd3dBackBuf));
		if (FAILED(hr)) return hr;
		hr = g_pDevice->CreateRenderTargetView(pd3dBackBuf.Get(), nullptr, &videoSurface.pTarget);
		if (SUCCEEDED(hr))
		{
			videoSurface.overlayContext = ImGui::CreateContext();
			ImGui::SetCurrentContext(videoSurface.overlayContext);
			ImGui::StyleColorsDark();
			ImGui_ImplWin32_Init(hWnd);
			ImGui_ImplDX11_Init(g_pDevice, g_pContext);
			videoSurface.hResizeEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
			*pSurfaceId = g_surfManager.Add(videoSurface);
		}		
		else
			*pSurfaceId = 0xffffffff;
		


		return hr;
	}

	HRESULT ResizeRenderTarget(uint32_t rendererId, uint16_t width, uint16_t height)
	{
		try
		{
			VIDEO_SURFACE& surface = g_surfManager.Get(rendererId);
			PLAYER_STATE prevState = surface.state;
			surface.state = PLAYER_STATE_RESIZING;
			if (WaitForSingleObject(surface.hResizeEvent, 1000) == WAIT_TIMEOUT)
			{
				return E_FAIL;
			}
			surface.width = width;
			surface.height = height;
			SAFE_RELEASE(surface.p2dTarget);
			SAFE_RELEASE(surface.pTarget);
			HRESULT hr = surface.pSwap->ResizeBuffers(NUM_BACKBUFFERS, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
			if (FAILED(hr)) return hr;
			ComPtr<IDXGISurface> pDxgiSurface = nullptr;
			hr = surface.pSwap->GetBuffer(0, IID_PPV_ARGS(&pDxgiSurface));
			if (FAILED(hr)) return hr;
			D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
			hr = g_pD2DFactory->CreateDxgiSurfaceRenderTarget(pDxgiSurface.Get(), &renderTargetProperties, &surface.p2dTarget);
			if (FAILED(hr)) return hr;

			ComPtr<ID3D11Texture2D> pd3dBackBuf = nullptr;
			hr = surface.pSwap->GetBuffer(0, IID_PPV_ARGS(&pd3dBackBuf));
			if (FAILED(hr)) return hr;
			hr = g_pDevice->CreateRenderTargetView(pd3dBackBuf.Get(), nullptr, &surface.pTarget);
			if (SUCCEEDED(hr))
				surface.state = prevState;
			else
				surface.state = PLAYER_STATE_INVALID;
			return hr;
		}
		catch (...)
		{
			return E_INVALIDARG;
		}
	}

	HRESULT OpenSource(uint32_t rendererId, const wchar_t* szPath)
	{
		try
		{
			HRESULT hr = S_OK;
			VIDEO_SURFACE& videoSurface = g_surfManager.Get(rendererId);
			SAFE_RELEASE(videoSurface.pReader);
			SAFE_RELEASE(videoSurface.pLastFrame);
			ComPtr<IMFMediaSource> pSource;
			hr = CreateMediaSource(szPath, &pSource);
			if (FAILED(hr)) return hr;
			ComPtr<IMFAttributes> pAttributes = nullptr;
			hr = MFCreateAttributes(&pAttributes, 1);
			if (FAILED(hr)) return hr;
			hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
			if (FAILED(hr)) return hr;
			hr = MFCreateSourceReaderFromMediaSource(pSource.Get(), pAttributes.Get(), &videoSurface.pReader);
			if (FAILED(hr)) return hr;
			ComPtr<IMFMediaType> pNewMediaType = nullptr;
			hr = MFCreateMediaType(&pNewMediaType);
			if (FAILED(hr)) return hr;
			hr = pNewMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			if (FAILED(hr)) return hr;
			hr = pNewMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			if (FAILED(hr)) return hr;
			hr = videoSurface.pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pNewMediaType.Get());
			if (FAILED(hr)) return hr;
			hr = videoSurface.pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
			if (SUCCEEDED(hr))
			{
				PROPVARIANT var;
				PropVariantInit(&var);
				hr = videoSurface.pReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
				if (SUCCEEDED(hr) && (var.vt == VT_UI8))
				{
					videoSurface.duration = var.uhVal.QuadPart;
				}
				PropVariantClear(&var);
				videoSurface.state = PLAYER_STATE_PLAYING;
			}
			return hr;
		}
		catch (...)
		{
			return E_INVALIDARG;
		}
	}

	LRESULT HandleWin32Msg(uint32_t rendererId, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		VIDEO_SURFACE& surface = g_surfManager.Get(rendererId);
		if (msg == WM_MOUSEMOVE)
			surface.overlayDuration = 2000;
		return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	}

	void Shutdown()
	{
		// Shutdown async worker
		g_bLaunched = false;
		if (g_rendererThread->joinable())
			g_rendererThread->join();
		g_rendererThread.reset();
		// Destroy all render targets


		SAFE_RELEASE(g_pDevice);
		SAFE_RELEASE(g_pContext);
		SAFE_RELEASE(g_pD2DFactory);
		SAFE_RELEASE(g_pD2DDevice);
		SAFE_RELEASE(g_pD2DContext);
		SAFE_RELEASE(dxgiFactory);
		SAFE_RELEASE(dxgiAdapter);
	}
}