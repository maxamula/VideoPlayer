#pragma once
#include "common.h"
#include "d3ddescriptorheap.h"

namespace media
{
	extern ID3D12Device8* device;
	extern IDXGIFactory7* dxgiFactory;
	extern IDXGIAdapter4* dxgiAdapter;

	extern DescriptorHeap g_rtvHeap;
	extern DescriptorHeap g_dsvHeap;
	extern DescriptorHeap g_srvHeap;
	extern DescriptorHeap g_uavHeap;

	void Initialize();
	void Shutdown();
}
