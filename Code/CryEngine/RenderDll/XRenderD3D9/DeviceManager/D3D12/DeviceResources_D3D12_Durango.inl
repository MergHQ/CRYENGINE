// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

uint32 CDeviceTexture::TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF, ETEX_TileMode eTM)
{
	FUNCTION_PROFILER_RENDERER;

	if (nMips && eTF != eTF_Unknown)
	{
		const SPixFormat* pPF;
		ETEX_Format eTFDst = gcpRendD3D->m_hwTexFormatSupport.GetClosestFormatSupported(eTF, pPF);
		if (eTFDst != eTF_Unknown && eTM != eTM_None)
		{
			XG_FORMAT xgFmt = (XG_FORMAT)pPF->DeviceFormat;
			XG_TILE_MODE xgTileMode;
			XG_USAGE xgUsage = XG_USAGE_DEFAULT;
			switch (eTM)
			{
			case eTM_LinearPadded:
				xgTileMode = XG_TILE_MODE_LINEAR;
				xgUsage = XG_USAGE_STAGING;
				break;
			case eTM_Optimal:
				xgTileMode = XGComputeOptimalTileMode(
					nDepth == 1 ? XG_RESOURCE_DIMENSION_TEXTURE2D : XG_RESOURCE_DIMENSION_TEXTURE3D,
					xgFmt,
					nWidth,
					nHeight,
					nSlices,
					1,
					XG_BIND_SHADER_RESOURCE);
				break;
			}

			if (nDepth == 1)
			{
				XG_TEXTURE2D_DESC desc;
				desc.Width = nWidth;
				desc.Height = nHeight;
				desc.MipLevels = nMips;
				desc.ArraySize = nSlices;
				desc.Format = xgFmt;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Usage = xgUsage;
				desc.BindFlags = XG_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;
				desc.TileMode = xgTileMode;
				desc.Pitch = 0;

				XG_RESOURCE_LAYOUT layout;
				if (!FAILED(XGComputeTexture2DLayout(&desc, &layout)))
				{
					return (int)layout.SizeBytes;
				}
			}
			else
			{
				XG_TEXTURE3D_DESC desc;
				desc.Width = nWidth;
				desc.Height = nHeight;
				desc.Depth = nDepth;
				desc.MipLevels = nMips;
				desc.Format = xgFmt;
				desc.Usage = xgUsage;
				desc.BindFlags = XG_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;
				desc.TileMode = xgTileMode;
				desc.Pitch = 0;

				XG_RESOURCE_LAYOUT layout;
				if (!FAILED(XGComputeTexture3DLayout(&desc, &layout)))
				{
					return (int)layout.SizeBytes;
				}
			}
		}

		// Fallback to the default texture size function
		return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, nSlices, eTF, eTM_None);
	}

	return 0;
}
