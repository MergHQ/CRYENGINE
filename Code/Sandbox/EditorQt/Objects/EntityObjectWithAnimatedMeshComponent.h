// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EntityObjectWithComponent.h"

class CEntityObjectWithAnimatedMeshComponent : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CEntityObjectWithAnimatedMeshComponent)

	// CEntityObject
	virtual bool Init(CBaseObject* prev, const string& file) override;
	virtual bool CreateGameObject() override;
	// ~CEntityObject

protected:
	string m_file;
};

/*!
 * Class Description of Entity with a default component
 */
class CEntityObjectWithAnimatedMeshComponentDesc : public CObjectClassDesc
{
public:
	ObjectType          GetObjectType()                     { return OBJTYPE_ENTITY; }
	const char*         ClassName()                         { return "EntityWithAnimatedMeshComponent"; }
	const char*         Category()                          { return ""; }
	CRuntimeClass*      GetRuntimeClass()                   { return RUNTIME_CLASS(CEntityObjectWithAnimatedMeshComponent); }
	const char*         GetFileSpec()                       { return "*.cga;*.cdf"; }
	virtual const char* GetDataFilesFilterString() override { return GetFileSpec(); }
};
