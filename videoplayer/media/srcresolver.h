#pragma once
#include "common.h"

namespace player
{
	class SourceResolver
	{
	public:
		SourceResolver();

		HRESULT CreateMediaSource(const wchar_t* szwFilePath, IMFMediaSource** ppMediaSource) const;
	private:
		IMFSourceResolver* m_pSrcResolver;
	};
}
