// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   extensionmanager.h
//  Version:     v1.00
//  Created:     5/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __extensionmanager_h__
#define __extensionmanager_h__
#pragma once

// forward declarations.
struct IConvertor;

/** Manages mapping between file extensions and convertors.
*/
class ExtensionManager
{
public:
	ExtensionManager();
	~ExtensionManager();

	//! Register new convertor with extension manager.
	//!  \param conv must not be 0
	//!  \param rc must not be 0
	void RegisterConvertor(const char* name, IConvertor* conv, IResourceCompiler* rc);
	//! Unregister all convertors.
	void UnregisterAll();

	//! Find convertor that matches given platform and extension.
	IConvertor* FindConvertor(const char* filename) const;

private:
	// Links extensions and convertors.
	typedef std::vector<std::pair<string, IConvertor*> > ExtVector;
	ExtVector m_extVector;

	std::vector<IConvertor*> m_convertors;
};

#endif // __extensionmanager_h__
