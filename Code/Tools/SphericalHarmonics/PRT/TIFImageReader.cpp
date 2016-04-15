/*
	common image reader implementation using directx
*/

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include "TIFImageReader.h"
#include <windows.h>
#define _TIFF_DATA_TYPEDEFS_					// because we defined uint32,... already
#include "tiffio.h"		// TIFF library

const void* NImage::CTIFImageReader::ReadImage
(
	const char * cpImageName, 
	uint32& rImageWidth, 
	uint32& rImageHeight, 
	EImageFileFormat& rFormat, 
	const uint32
) const
{
	const uint32 cBPP = 3;
	static CSHAllocator<float> sAllocator;
	TIFF* pTiff = TIFFOpen(cpImageName,"r");

	float *pOutput = NULL;
	if(pTiff) 
	{
		uint32 width, height;
		size_t pixelNum;
		uint32 *pRGBA;

		TIFFGetField(pTiff, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(pTiff, TIFFTAG_IMAGELENGTH, &height);
		rImageWidth		= width;
		rImageHeight	= height;
		rFormat	= (cBPP == 4)?A32B32G32R32F : B32G32R32F;

		pixelNum = width * height;
		pRGBA = (uint32*) new char[pixelNum * sizeof (uint32)];

		if(pRGBA) 
		{
			if(TIFFReadRGBAImage(pTiff, width, height, pRGBA, 0)) 
			{
				//convert data into floats
				// scan and copy surface, everything is float RGB
				pOutput = (float*)(sAllocator.new_mem_array(sizeof(float) * rImageWidth * rImageHeight * cBPP));
				assert(pOutput);	

				float *pCurrentOutput = pOutput;
				uint32 *pCurrentInput = pRGBA;

				for(unsigned int y=0; y < height; ++y)
				{
					for(unsigned int x=0; x < width; ++x)
					{
						for(int i=0; i<cBPP; ++i)
							pCurrentOutput[i] = (float)pCurrentInput[i];
						pCurrentOutput	+= cBPP;
						pCurrentInput		+= cBPP;
					}
				}
			}
			SAFE_DELETE_ARRAY(pRGBA);
		}
//		else RCLogError("ReadImage out of memory");
	}
	else
	{
		char message[200];
		sprintf(message, "Could not load Texture: %s\n", cpImageName);
		GetSHLog().LogError(message);
		return NULL;
	}

	if(pTiff)
		TIFFClose(pTiff);

	return (void*)pOutput;
}

#endif
