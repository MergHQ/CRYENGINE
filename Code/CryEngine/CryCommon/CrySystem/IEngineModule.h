// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   IEngineModule.h
//  Created:     11/8/2009 by Timur.
//  Description: Defines the extension interface for the CryEngine modules.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __IEngineModule_h__
#define __IEngineModule_h__
#pragma once

#include <CryExtension/ICryUnknown.h>

struct SSystemInitParams;

//! Base Interface for all engine module extensions.
struct IEngineModule : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IEngineModule, 0xf899cf661df04f61, 0xa341a8a7ffdf9de4);

	// <interfuscator:shuffle>
	//! Retrieve name of the extension module.
	virtual const char* GetName() = 0;

	//! Retrieve category for the extension module (CryEngine for standard modules).
	virtual const char* GetCategory() = 0;

	//! This is called to initialize the new module.
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) = 0;
	// </interfuscator:shuffle>
};

#endif //__IEngineModule_h__
