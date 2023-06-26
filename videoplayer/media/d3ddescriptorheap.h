#pragma once
#include "common.h"
#include <DescriptorHeap.h>
#include <boost/dynamic_bitset.hpp>

namespace media
{
	struct DESCRIPTOR_HANDLE
	{
		friend class DescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE CPU = {};
		D3D12_GPU_DESCRIPTOR_HANDLE GPU = {};
		inline uint32_t GetIndex() const { return index; }
	private:
		uint32_t index = -1;
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap() = default;
		DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, size_t capacity);
		DISABLE_MOVE_COPY(DescriptorHeap);
		~DescriptorHeap();
		void Release();
		DESCRIPTOR_HANDLE Allocate();
		void Free(DESCRIPTOR_HANDLE handle);
		inline ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_heap->Heap(); }
		inline size_t GetNumDescriptors() const { return m_size; }
	private:
		uint32_t _GetFreeIndex() const;
		std::unique_ptr<DirectX::DescriptorHeap> m_heap = {};
		boost::dynamic_bitset<unsigned int> m_available;
		std::mutex m_mutex;

		size_t m_size = 0;
		size_t m_cap = 0;
		bool m_shaderVisible = false;
	};
}
