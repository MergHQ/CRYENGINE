// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EntityObject.h"

class CAIWaveObject : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CAIWaveObject)

	virtual void InitVariables() {}

	void         SetName(const string& newName);

protected:
	//! Ctor must be protected.
	CAIWaveObject();

	void DeleteThis() { delete this; }
};

class CAIWaveObjectClassDesc : public CObjectClassDesc
{
public:
	ObjectType     GetObjectType()   { return OBJTYPE_ENTITY; }
	const char*    ClassName()       { return "Entity::AIWave"; }
	const char*    Category()        { return ""; }
	CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CAIWaveObject); }
};
