#pragma once
#include "common.h"

namespace media
{
	extern ID3D11Device* g_pDevice;
	extern ID3D11DeviceContext* g_pContext;

	extern ID2D1Factory* g_pD2DFactory;
	extern ID2D1Device* g_pD2DDevice;
	extern ID2D1DeviceContext* g_pD2DContext;

	extern IDXGIFactory7* dxgiFactory;
	extern IDXGIAdapter4* dxgiAdapter;

	void Initialize();
	void Shutdown();
}
