/*
	image factory declaration
*/

#if defined(OFFLINE_COMPUTATION)

#pragma once

#include <exception>
#include "IImageReader.h"

namespace NImage
{
	class CHDRImageReader;
#if defined(USE_D3DX)
	class CCommonImageReader;
#endif
	//!< singleton to abstract image factory
	class CImageFactory
	{
	public:
		typedef enum EImageType
		{
			IMAGE_TYPE_HDR24,			//!< HDR map with 3xfloat 32
#if defined(USE_D3DX)
			IMAGE_TYPE_COMMON,		//!< normal format supported by DirectX
#endif
			IMAGE_TYPE_TIFF,			//!< tiff format(not supported by directx)
		}EImageType;

		//!< singleton stuff
		static CImageFactory* Instance();

		const NSH::CSmartPtr<const IImageReader, CSHAllocator<> >  GetImageReader(const EImageType cImageType);

	private:
		//!< singleton stuff
		CImageFactory(){}
		CImageFactory(const CImageFactory&);
		CImageFactory& operator= (const CImageFactory&);
	};
}

#endif
