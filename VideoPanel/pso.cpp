#include "pch.h"
#include "pso.h"

namespace VideoPanel::GFX
{
	ID3D12PipelineState* CreatePSO(void* stream, uint64 size)
	{
		if (!stream || !size)
			throw std::runtime_error("Invalid PSO stream.");
		D3D12_PIPELINE_STATE_STREAM_DESC desc = {};
		desc.pPipelineStateSubobjectStream = stream;
		desc.SizeInBytes = size;
		return CreatePSO(desc);
	}

	ID3D12PipelineState* CreatePSO(D3D12_PIPELINE_STATE_STREAM_DESC desc)
	{
		ID3D12PipelineState* pso = nullptr;
		ThrowIfFailed(device->CreatePipelineState(&desc, IID_PPV_ARGS(&pso)));
		return pso;
	}
}