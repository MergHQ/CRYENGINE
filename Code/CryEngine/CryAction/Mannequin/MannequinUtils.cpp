// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MannequinUtils.h"

#include <ICryMannequin.h>

const char* mannequin::FindProcClipTypeName(const IProceduralClipFactory::THash& typeNameHash)
{
	if (gEnv && gEnv->pGame && gEnv->pGame->GetIGameFramework())
	{
		const IProceduralClipFactory& proceduralClipFactory = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetProceduralClipFactory();
		const char* const typeName = proceduralClipFactory.FindTypeName(typeNameHash);
		return typeName;
	}
	return NULL;
}
