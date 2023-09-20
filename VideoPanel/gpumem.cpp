#include "pch.h"
#include "gpumem.h"
#include "upload.h"
#include "d3dx12.h"

namespace VideoPanel::GFX
{
	void TransitionResource(ID3D12GraphicsCommandList6* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after
		, D3D12_RESOURCE_BARRIER_FLAGS flags, uint32_t subresource)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = flags;
		barrier.Transition.pResource = resource;
		barrier.Transition.Subresource = subresource;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		cmd->ResourceBarrier(1, &barrier);
	}

	Texture::Texture(const void* pData, uint64 cbData, uint64 width, uint64 height)
	{
		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		ThrowIfFailed(device->CreateCommittedResource(&HEAP.DEFAULT, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_res)));

		const UINT subresourceCount = textureDesc.DepthOrArraySize * textureDesc.MipLevels;
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_res, 0, subresourceCount);

		GFX::Upload::CopyContext copy(uploadBufferSize);

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = pData;
		textureData.RowPitch = static_cast<LONG_PTR>((4 * width));;
		textureData.SlicePitch = textureData.RowPitch * height;

		UpdateSubresources(copy.GetCommandList(), m_res, copy.GetUploadBuffer(), 0, 0, subresourceCount, &textureData);
		copy.Flush();

		D3D12_SHADER_RESOURCE_VIEW_DESC diffuseSrvDesc{};
		diffuseSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		diffuseSrvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		diffuseSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		diffuseSrvDesc.Texture2D.MipLevels = 1;

		m_srv = g_srvHeap.Allocate();
		device->CreateShaderResourceView(m_res, &diffuseSrvDesc, m_srv.CPU);
	}

	Texture::~Texture()
	{
		RELEASE(m_res);
		//g_srvHeap.Free(m_srv);
	}
}