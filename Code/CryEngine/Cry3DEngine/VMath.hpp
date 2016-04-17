// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Version:     v1.00
//  Created:     Michael Kopietz
//  Description: unified vector math lib
// -------------------------------------------------------------------------
//  History:		- created 1999  for Katmai and K3
//							-	...
//							-	integrated into cryengine
//
////////////////////////////////////////////////////////////////////////////
#ifndef __VMATH__
#define __VMATH__

#define VEC4_SSE
#if CRY_PLATFORM_DURANGO
	#define VEC4_SSE4
#endif

//#include <math.h>
#include <CryMath/Cry_Math.h>

namespace NVMath
{
#if CRY_PLATFORM_NEON
	#include "VMath_NEON.hpp"
#elif (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO || CRY_PLATFORM_ORBIS || CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_MAC || defined(IOS_SIMULATOR)) && (defined(VEC4_SSE) || defined(VEC4_SSE4))
	#include "VMath_SSE.hpp"
#else
	#include "VMath_C.hpp"
#endif
}

#endif
