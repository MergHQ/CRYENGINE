/*
	image factory implementation
*/
#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include <windows.h>
#include "ImageFactory.h"
#include "HDRImageReader.h"
#include "CommonImageReader.h"
#include "TIFImageReader.h"

NImage::CImageFactory* NImage::CImageFactory::Instance()
{
	static CImageFactory inst;
	return &inst;
}

const NSH::CSmartPtr<const NImage::IImageReader, CSHAllocator<> > NImage::CImageFactory::GetImageReader(const NImage::CImageFactory::EImageType cImageType)
{
	switch(cImageType)
	{
	case IMAGE_TYPE_HDR24:
		return NSH::CSmartPtr<const CHDRImageReader, CSHAllocator<> >(new CHDRImageReader);
		break;
#if defined(USE_D3DX)
	case IMAGE_TYPE_COMMON:
		return NSH::CSmartPtr<const CCommonImageReader, CSHAllocator<> >(new CCommonImageReader);
		break;
#endif
	case IMAGE_TYPE_TIFF:
		return NSH::CSmartPtr<const CTIFImageReader, CSHAllocator<> >(new CTIFImageReader);
		break;
	default:
		assert(false);
	}
	return NSH::CSmartPtr<const NImage::IImageReader, CSHAllocator<> >(NULL);
}

#endif