/*
	image reader interface definitions
*/

#if defined(OFFLINE_COMPUTATION)

#pragma once

#include <PRT/PRTTypes.h>
#include <exception>

namespace NImage
{
	//!< image float file format
	enum EImageFileFormat
	{
		A32B32G32R32F,
		B32G32R32F
	};

	//!< image reader interface
	interface IImageReader
	{
		//!< to have this method at least in common, a void pointer needs to be returned
		virtual const void* ReadImage(const char * cpImageName, uint32& rImageWidth, uint32& rImageHeight, EImageFileFormat& rFormat, const uint32 cBPP = 4) const = 0;
		//!< need virtual destructor here since inherited classes are using allocator
		virtual ~IImageReader(){}
	};	
}

#endif