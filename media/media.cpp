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
					std::lock_guard lock(VideoSurface::s_activeSurfacesMutex);
					for (auto surface : VideoSurface::s_activeSurfaces)
					{
						surface->Update();
					}
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
		VideoSurface::s_activeSurfaces.clear();
		MFShutdown();
	}
}