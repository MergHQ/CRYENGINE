/*
	common image reader declaration using directx
*/

#if defined(OFFLINE_COMPUTATION)

#pragma once


#include "IImageReader.h"


namespace NImage
{
	class CTIFImageReader : public IImageReader
	{
	public:
		INSTALL_CLASS_NEW(CTIFImageReader)

		virtual const void* ReadImage(const char * cpImageName, uint32& rImageWidth, uint32& rImageheight, EImageFileFormat& rFormat, const uint32) const;
		virtual ~CTIFImageReader(){}
	private:
	};
}

#endif