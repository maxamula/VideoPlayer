#include "media.h"
#include "d3dmanager.h"
#include "videosurface.h"
#include <thread>
#include <vector>
#include <shared_mutex>

namespace media
{
	D3DManager g_d3d{};
	std::atomic<bool> g_bRunning{false};
	std::unique_ptr<std::thread> g_rendererThread{};
	std::vector<VideoSurface*> g_activeRenderers{};
	std::shared_mutex g_activeRenderersMutex;

	HRESULT Initialize()
	{
		HRESULT hr = S_OK;
		hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (FAILED(hr)) return hr;
		hr = MFStartup(MF_VERSION);
		if (FAILED(hr)) return hr;
		// Launch renderer thread

		g_bRunning = true;
		g_rendererThread = std::make_unique<std::thread>([&]()
			{
				while (g_bRunning)
				{
					g_activeRenderersMutex.lock_shared();
					for (VideoSurface* surface : g_activeRenderers)
					{
						// Add ref in case surface is deleted mid-frameprocessing
						surface->AddRef();
						surface->Update();
						surface->Release();
					}
					g_activeRenderersMutex.unlock_shared();
				}
			});
		return hr;
	}

	void Shutdown()
	{
		g_bRunning = false;
		if (g_rendererThread->joinable())
			g_rendererThread->join();
		g_rendererThread.reset();
		g_activeRenderers.clear();
		MFShutdown();
	}
}