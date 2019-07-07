// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//  Main header included by every file in Editor.

#include <Include/SandboxAPI.h>

//////////////////////////////////////////////////////////////////////////
// optimize away function call, in favor of inlining asm code
//////////////////////////////////////////////////////////////////////////
#pragma intrinsic( memset,memcpy,memcmp )
#pragma intrinsic( strcat,strcmp,strcpy,strlen,_strset )
//#pragma intrinsic( abs,fabs,fmod,sin,cos,tan,log,exp,atan,atan2,log10,sqrt,acos,asin )

// Warnings in STL
#pragma warning (disable : 4786) // identifier was truncated to 'number' characters in the debug information.
#pragma warning (disable : 4244) // conversion from 'long' to 'float', possible loss of data
#pragma warning (disable : 4018) // signed/unsigned mismatch
#pragma warning (disable : 4800) // BOOL bool conversion

// Disable warning when a function returns a value inside an __asm block
#pragma warning (disable : 4035)

//////////////////////////////////////////////////////////////////////////
// 64-bits related warnings.
#pragma warning (disable : 4267) // conversion from 'size_t' to 'int', possible loss of data

//////////////////////////////////////////////////////////////////////////
// Simple type definitions.
//////////////////////////////////////////////////////////////////////////
#include <CryCore/BaseTypes.h>

// If #defined, then shapes of newly created AITerritoryShapes will be limited to a single point, which is less confusing for level-designers nowadays
// as the actual shape has no practical meaning anymore (C1 and maybe C2 may have used the shape, but it was definitely no longer used for C3).
#define USE_SIMPLIFIED_AI_TERRITORY_SHAPE

/////////////////////////////////////////////////////////////////////////////
// VARIOUS MACROS AND DEFINES
/////////////////////////////////////////////////////////////////////////////
#ifdef new
	#undef new
#endif

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff.
/////////////////////////////////////////////////////////////////////////////
#include <CryCore/Platform/platform.h>
#include <CryCore/smartptr.h>
#include <CryCore/StlUtils.h>
#include <CryMath/Cry_Geo.h>
#include <CryMath/Range.h>

#define CRY_ENABLE_FBX_SDK
