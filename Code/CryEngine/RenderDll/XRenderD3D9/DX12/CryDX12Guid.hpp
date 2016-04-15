// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:
//  Version:     v1.00
//  Created:     03/02/2015 by Jan Pinter
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __CRYDX12GUID__
	#define __CRYDX12GUID__

	#define DX12_STRINGIFY(_STRING) # _STRING

	#define DX12_GUID_STRING(_D0, _W0, _W1, _B0, _B1, _B2, _B3, _B4, _B5, _B6, _B7) \
	  DX12_STRINGIFY(_D0 ## - ## _W0 ## - ## _W1 ## - ## _B0 ## _B1 ## - ## _B2 ## _B3 ## _B4 ## _B5 ## _B6 ## _B7)

	#define DX12_DEFINE_TYPE_GUID(_CLASS, _TYPE, _D0, _W0, _W1, _B0, _B1, _B2, _B3, _B4, _B5, _B6, _B7) \
	  _CLASS __declspec(uuid(DX12_GUID_STRING(_D0, _W0, _W1, _B0, _B1, _B2, _B3, _B4, _B5, _B6, _B7))) _TYPE;

//DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture1D, 637BD3A1, 3507, 4ECA, B0, 24, F4, 5E, 72, 1A, 93, CA);
//DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture2D, 810C3ECB, 11EA, 48C6, 92, EE, FE, 1F, 56, CC, A1, FB);
//DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture3D, AD18E34A, 1879, 4329, 8A, 38, 47, 3E, 98, 92, 11, F6);

#endif // __CRYDX12GUID__
