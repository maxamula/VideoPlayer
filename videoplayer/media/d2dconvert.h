#pragma once
#include "common.h"

namespace media
{
	HRESULT ConvertSampleToBitmap(IMFSample* pSample, ID2D1RenderTarget* pRenderTarget, ID2D1Bitmap** ppBitmap);
}