#pragma once
#include "common.h"

namespace media
{
	class SourceResolver
	{
	public:
		SourceResolver();

		HRESULT CreateMediaSource(const wchar_t* szwFilePath, IMFMediaSource** ppMediaSource) const;
	private:
		ComPtr<IMFSourceResolver> m_pSrcResolver;
	};
}
