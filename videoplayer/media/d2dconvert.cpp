#include "d2dconvert.h"
#include <wincodec.h>

namespace media
{

	HRESULT ConvertSampleToBitmap(IMFSample* pSample, ID2D1RenderTarget* pRenderTarget, ID2D1Bitmap** ppBitmap)
	{
        HRESULT hr = S_OK;

        IMFMediaBuffer* pBuffer = nullptr;
        IMF2DBuffer* p2Buffer = nullptr;
        BYTE* pBitmapBits = nullptr;
        UINT32 uWidth, uHeight, uStride;

        IMFMediaBuffer* pMediaBuffer = nullptr;
        hr = pSample->ConvertToContiguousBuffer(&pBuffer);

        hr = pBuffer->QueryInterface(IID_PPV_ARGS(&p2Buffer));

        hr = MFGetAttributeSize(pSample, MF_MT_FRAME_SIZE, &uWidth, &uHeight);

        LONG stride = 0;
        BYTE* data = 0;
        hr = p2Buffer->Lock2D(&data, &stride);



        hr = pRenderTarget->CreateBitmap(D2D1::SizeU(uWidth, uHeight), pBitmapBits, uStride, D2D1::BitmapProperties(), ppBitmap);

        pBuffer->Unlock();
        pBuffer->Release();

        return hr;
	}
}