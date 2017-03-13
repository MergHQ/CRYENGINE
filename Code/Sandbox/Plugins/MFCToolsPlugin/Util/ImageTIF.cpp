// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ImageTIF.h"

#include <tiffio.h>   // TIFF library

#include <CrySystem/File/CryFile.h>
#include "Util/FileUtil.h"


// Function prototypes
static tsize_t libtiffDummyReadProc(thandle_t fd, tdata_t buf, tsize_t size);
static tsize_t libtiffDummyWriteProc(thandle_t fd, tdata_t buf, tsize_t size);
static toff_t  libtiffDummySizeProc(thandle_t fd);
static toff_t  libtiffDummySeekProc(thandle_t fd, toff_t off, int i);
//static int libtiffDummyCloseProc (thandle_t fd);

// We need globals because of the callbacks (they don't allow us to pass state)
static CryMutex globalImageMutex;
static char* globalImageBuffer = 0;
static unsigned long globalImageBufferOffset = 0;
static unsigned long globalImageBufferSize = 0;

/////////////////// Callbacks to libtiff

static int libtiffDummyMapFileProc(thandle_t, tdata_t*, toff_t*)
{
	return 0;
}

static void libtiffDummyUnmapFileProc(thandle_t, tdata_t, toff_t)
{
}

static toff_t libtiffDummySizeProc(thandle_t fd)
{
	return globalImageBufferSize;
}

static tsize_t
libtiffDummyReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	tsize_t nBytesLeft = globalImageBufferSize - globalImageBufferOffset;

	if (size > nBytesLeft)
		size = nBytesLeft;

	memcpy(buf, &globalImageBuffer[globalImageBufferOffset], size);

	globalImageBufferOffset += size;

	// Return the amount of data read
	return size;
}

static tsize_t
libtiffDummyWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return (size);
}

static toff_t
libtiffDummySeekProc(thandle_t fd, toff_t off, int i)
{
	switch (i)
	{
	case SEEK_SET:
		globalImageBufferOffset = off;
		break;

	case SEEK_CUR:
		globalImageBufferOffset += off;
		break;

	case SEEK_END:
		globalImageBufferOffset = globalImageBufferSize - off;
		break;

	default:
		globalImageBufferOffset = off;
		break;
	}

	// This appears to return the location that it went to
	return globalImageBufferOffset;
}

static int
libtiffDummyCloseProc(thandle_t fd)
{
	// Return a zero meaning all is well
	return 0;
}

bool CImageTIF::Load(const string& fileName, CImageEx& outImage)
{
	CCryFile file;
	if (!file.Open(fileName, "rb"))
	{
		CryLog("File not found %s", (const char*)fileName);
		return false;
	}

	// We use some global variables in callbacks, so we must
	// prevent multithread access to the data
	CryAutoLock<CryMutex> tifAutoLock(globalImageMutex);

	std::vector<uint8> data;

	globalImageBufferSize = file.GetLength();

	data.resize(globalImageBufferSize);
	globalImageBuffer = (char*)&data[0];
	globalImageBufferOffset = 0;

	file.ReadRaw(globalImageBuffer, globalImageBufferSize);

	// Open the dummy document (which actually only exists in memory)
	TIFF* tif = TIFFClientOpen(fileName, "rm", (thandle_t) -1, libtiffDummyReadProc,
	                           libtiffDummyWriteProc, libtiffDummySeekProc,
	                           libtiffDummyCloseProc, libtiffDummySizeProc, libtiffDummyMapFileProc, libtiffDummyUnmapFileProc);

	//	TIFF* tif = TIFFOpen(fileName,"r");

	bool bRet = false;

	if (tif)
	{
		uint32 dwWidth, dwHeight;
		size_t npixels;
		uint32* raster;
		char* dccfilename = NULL;

		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);
		TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &dccfilename);

		npixels = dwWidth * dwHeight;
		raster = (uint32*) _TIFFmalloc((tsize_t)(npixels * sizeof(uint32)));

		if (raster)
		{
			if (TIFFReadRGBAImage(tif, dwWidth, dwHeight, raster, 0))
			{
				if (outImage.Allocate(dwWidth, dwHeight))
				{
					char* dest = (char*)outImage.GetData();
					uint32 dwPitch = dwWidth * 4;

					for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
					{
						char* src2 = (char*)&raster[(dwHeight - 1 - dwY) * dwWidth];
						char* dest2 = &dest[dwPitch * dwY];

						memcpy(dest2, src2, dwWidth * 4);
					}

					if (dccfilename)
						outImage.SetDccFilename(dccfilename);

					bRet = true;

					/*
					   {
					   string sSpecialInstructions = GetSpecialInstructionsFromTIFF(tif);

					   char *p = sSpecialInstructions.GetBuffer();

					   while(*p)
					   {
					    string sKey,sValue;

					    JumpOverWhitespace(p);

					    if(*p != '/')								// /
					    {
					      m_pCC->pLog->LogError("Special instructions in TIFF '%s' are broken (/ expected)",lpszPathName);
					      break;
					    }
					    p++;												// jump over /

					    while(IsValidNameChar(*p))	// key
					      sKey+=*p++;

					    JumpOverWhitespace(p);

					    if(*p != '=')								// =
					    {
					      m_pCC->pLog->LogError("Special instructions in TIFF '%s' are broken (= expected)",lpszPathName);
					      break;
					    }
					    p++;												// jump over =

					    JumpOverWhitespace(p);

					    while(IsValidNameChar(*p))	// value
					      sValue+=*p++;

					    JumpOverWhitespace(p);

					    sKey.Trim();sValue.Trim();

					    m_pCC->pFileSpecificConfig->UpdateOrCreateEntry("",sKey,sValue);
					   }
					   }
					 */
				}
			}

			_TIFFfree(raster);
		}
		TIFFClose(tif);
	}

	if (!bRet)
		outImage.Detach();

	return bRet;
}

//////////////////////////////////////////////////////////////////////////
bool CImageTIF::SaveRAW(const string& fileName, const void* pData, int width, int height, int bytesPerChannel, int numChannels, bool bFloat, const char* preset)
{
	if (bFloat && (bytesPerChannel != 2 && bytesPerChannel != 4))
	{
		bFloat = false;
	}

	bool bRet = false;

	CFileUtil::OverwriteFile(fileName);
	TIFF* tif = TIFFOpen(fileName, "wb");
	if (tif)
	{
		TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, numChannels);
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bytesPerChannel * 8);
		TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		if (bFloat)
		{
			TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
		}

		if (preset && preset[0])
		{
			string tiffphotoshopdata, valueheader;
			string presetkeyvalue = string("/preset=") + string(preset);

			valueheader.push_back('\x1C');
			valueheader.push_back('\x02');
			valueheader.push_back('\x28');
			valueheader.push_back((presetkeyvalue.size() >> 8) & 0xFF);
			valueheader.push_back((presetkeyvalue.size()) & 0xFF);
			valueheader.append(presetkeyvalue);

			tiffphotoshopdata.push_back('8');
			tiffphotoshopdata.push_back('B');
			tiffphotoshopdata.push_back('I');
			tiffphotoshopdata.push_back('M');
			tiffphotoshopdata.push_back('\x04');
			tiffphotoshopdata.push_back('\x04');
			tiffphotoshopdata.push_back('\x00');
			tiffphotoshopdata.push_back('\x00');

			tiffphotoshopdata.push_back((valueheader.size() >> 24) & 0xFF);
			tiffphotoshopdata.push_back((valueheader.size() >> 16) & 0xFF);
			tiffphotoshopdata.push_back((valueheader.size() >> 8) & 0xFF);
			tiffphotoshopdata.push_back((valueheader.size()) & 0xFF);
			tiffphotoshopdata.append(valueheader);

			TIFFSetField(tif, TIFFTAG_PHOTOSHOP, tiffphotoshopdata.size(), tiffphotoshopdata.c_str());
		}

		size_t pitch = width * bytesPerChannel * numChannels;
		char* raster = (char*) _TIFFmalloc((tsize_t)(pitch * height));
		memcpy(raster, pData, pitch * height);

		bRet = true;
		for (int h = 0; h < height; ++h)
		{
			size_t offset = h * pitch;
			int err = TIFFWriteScanline(tif, raster + offset, h, 0);
			assert(IsHeapValid());
			if (err < 0)
			{
				bRet = false;
				break;
			}
		}
		_TIFFfree(raster);
		TIFFClose(tif);
	}
	return bRet;
}

const char* CImageTIF::GetPreset(const string& fileName)
{
	std::vector<uint8> data;
	CCryFile file;
	if (!file.Open(fileName, "rb"))
	{
		CryLog("File not found %s", (const char*)fileName);
		return NULL;
	}
	globalImageBufferSize = file.GetLength();

	data.resize(globalImageBufferSize);
	globalImageBuffer = (char*)&data[0];
	globalImageBufferOffset = 0;

	file.ReadRaw(globalImageBuffer, globalImageBufferSize);

	TIFF* tif = TIFFClientOpen(fileName, "rm", (thandle_t) -1, libtiffDummyReadProc,
	                           libtiffDummyWriteProc, libtiffDummySeekProc,
	                           libtiffDummyCloseProc, libtiffDummySizeProc, libtiffDummyMapFileProc, libtiffDummyUnmapFileProc);

	string strReturn;
	char* preset = NULL;
	int size;
	if (tif)
	{
		TIFFGetField(tif, TIFFTAG_PHOTOSHOP, &size, &preset);
		for (int i = 0; i < size; ++i)
		{
			if (!strncmp((preset + i), "preset", 6))
			{
				char* presetoffset = preset + i;
				strReturn = presetoffset;
				if (strReturn.find('/') != -1)
					strReturn = strReturn.substr(0, strReturn.find('/'));

				break;
			}

		}
		TIFFClose(tif);
	}
	return strReturn.c_str();
}
