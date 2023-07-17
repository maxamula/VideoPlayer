#pragma once
#include "d3dmanager.h"
#include "common.h"

namespace media
{
#ifdef _DETAIL
	D3DManager& D3D();

#endif
	HRESULT Initialize();
	void Shutdown();
}