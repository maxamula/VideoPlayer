#pragma once
#include <pch.h>
#include "graphics.h"

namespace VideoPanel::GFX
{
	void TransitionResource(ID3D12GraphicsCommandList6* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after
		, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	class Texture
	{
	public:
		// From pixel data
		Texture(const void* pData, uint64 cbData, uint64 width, uint64 height);
		~Texture();

		[[nodiscard]] inline ID3D12Resource* Resource() const { return m_res; }
		[[nodiscard]] inline DESCRIPTOR_HANDLE SRVAllocation() const { return m_srv; }
	protected:
		static constexpr uint16 maxMips{ 10 };
		ID3D12Resource* m_res = nullptr;
		DESCRIPTOR_HANDLE m_srv;
	};
}
