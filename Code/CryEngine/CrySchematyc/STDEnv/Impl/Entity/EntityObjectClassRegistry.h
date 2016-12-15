// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityClass.h>

namespace Schematyc
{
// Forward declare interfaces
struct IEnvRegistrar;
struct IScriptClass;
// Forward declare structures
struct SClassInfo;

struct SEntityObjectClass
{
	SEntityObjectClass();
	SEntityObjectClass(const SEntityObjectClass& rhs);

	~SEntityObjectClass();

	string                                 name;
	SGUID                                  guid;
	string                                 icon;
	IEntityClassRegistry::SEntityClassDesc entityClassDesc;
	IEntityClass*                          pEntityClass = nullptr;
};

class CEntityObjectClassRegistry
{
private:

	typedef std::map<string, SEntityObjectClass> EntityObjectClasses;
	typedef std::map<SGUID, SEntityObjectClass*> EntityObjectClassesByGUID;

public:

	void                      Init();

	const SEntityObjectClass* GetEntityObjectClass(const SGUID& guid) const; // #SchematycTODO : Remove?

private:

	void OnClassCompilation(const IRuntimeClass& runtimeClass);

private:

	EntityObjectClasses       m_entityObjectClasses;
	EntityObjectClassesByGUID m_entityObjectClassesByGUID;
	CConnectionScope          m_connectionScope;
};
} // Schematyc
