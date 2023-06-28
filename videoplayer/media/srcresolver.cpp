#include "srcresolver.h"

namespace media
{
	SourceResolver::SourceResolver()
	{
		assert(MFCreateSourceResolver(&m_pSrcResolver) == S_OK);
	}

	HRESULT SourceResolver::CreateMediaSource(const wchar_t* szwFilePath, IMFMediaSource** ppMediaSource) const
	{
		if (!ppMediaSource)
			return E_POINTER;
		MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
		IUnknown* pSource = nullptr;
		if (m_pSrcResolver->CreateObjectFromURL(szwFilePath, MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, &pSource) != S_OK)
			return E_INVALIDARG;
		assert(ObjectType != MF_OBJECT_INVALID);
		assert(pSource->QueryInterface(IID_PPV_ARGS(ppMediaSource)) == S_OK);
		return S_OK;
	}
}