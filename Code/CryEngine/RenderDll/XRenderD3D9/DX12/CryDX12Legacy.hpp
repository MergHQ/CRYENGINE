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
#ifndef __CRYDX12LEGACY__
	#define __CRYDX12LEGACY__

	#if defined(WIN32)
		#include <windows.h>
		#include <objbase.h>
		#include <mmsyscom.h>
		#include <d3d11.h>
		#include <dxgi.h>
		#include <dxgi1_2.h>
	#endif

	#if !defined(D3D_OK)
		#define D3D_OK 0
	#endif

	#if !defined(S_OK)
		#define S_OK 0
	#endif

typedef enum _D3DPOOL
{
	D3DPOOL_DEFAULT     = 0,
	D3DPOOL_MANAGED     = 1,
	D3DPOOL_SYSTEMMEM   = 2,
	D3DPOOL_SCRATCH     = 3,
	D3DPOOL_FORCE_DWORD = 0x7fffffff
} D3DPOOL;

#endif // __CRYDX12LEGACY__
