#pragma once
#include "graphics.h"
#include "gpumem.h"
#include "rootsig.h"
#include "pso.h"
#include "shaders.h"

namespace VideoPanel::GFX
{
	namespace Render
	{
		void Initialize();
		void Shutdown();
		void Render(ID3D12GraphicsCommandList6* cmd, uint32_t videoframeSRVIndex, const RENDER_TARGET rtv, D3D12_VIEWPORT viewport, D3D12_RECT scissors);
	}
}

