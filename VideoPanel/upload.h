#pragma once
#include "graphics.h"

namespace VideoPanel::GFX::Upload
{
	void Initialize();
	void Shutdown();

	class CopyContext
	{
	public:
		CopyContext(uint64 alignedSize);
		~CopyContext()
		{}
		DISABLE_MOVE_COPY(CopyContext);
		void Flush();
		// Accessors
		[[nodiscard]] inline ID3D12Resource* GetUploadBuffer() const { return m_uploadBuffer; }
		[[nodiscard]] inline void* Mapped() const { return m_mapped; }
		[[nodiscard]] inline ID3D12GraphicsCommandList6* GetCommandList() const { return m_cmdList; }
	private:
		ID3D12GraphicsCommandList6* m_cmdList = nullptr;
		ID3D12Resource* m_uploadBuffer = nullptr;
		void* m_mapped = nullptr;
		uint32 m_frameIndex = 0;
	};
}