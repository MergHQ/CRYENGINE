/*
	common image reader declaration using directx
*/

#if defined(OFFLINE_COMPUTATION) && defined(USE_D3DX)

#pragma once


#include "IImageReader.h"


namespace NImage
{
	class CCommonImageReader : public IImageReader
	{
	public:
		INSTALL_CLASS_NEW(CCommonImageReader)

		virtual const void* ReadImage(const char * cpImageName, uint32& rImageWidth, uint32& rImageheight, EImageFileFormat& rFormat, const uint32 cBPP = 4) const;
		virtual ~CCommonImageReader(){}
	private:
	};
}

#endif