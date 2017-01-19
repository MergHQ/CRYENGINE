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

#include <CrySystem/ICryPlugin.h>

struct SSystemInitParams;

//! Base Interface for all engine module extensions.
struct IEngineModule : public ICryPlugin
{
	CRYINTERFACE_DECLARE(IEngineModule, 0xf899cf661df04f61, 0xa341a8a7ffdf9de4);

	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {};
};

#endif //__IEngineModule_h__
