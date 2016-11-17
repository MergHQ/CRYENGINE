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

class CEntityObjectClassRegistry
{
private:

	struct SEntityClass
	{
		SEntityClass();
		SEntityClass(const SEntityClass& rhs);

		~SEntityClass();

		string editorCategory;
		string icon;
		IEntityClassRegistry::SEntityClassDesc desc;
	};

	typedef std::map<string, SEntityClass> EntityClasses;

public:

	void Init();

private:

	void OnClassCompilation(const IRuntimeClass& runtimeClass);

private:

	EntityClasses    m_entityClasses;
	CConnectionScope m_connectionScope;
};
} // Schematyc
