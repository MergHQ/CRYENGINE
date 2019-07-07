// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PerforcePlugin.h"

#include "CryCore/Platform/platform_impl.inl"
#include "PathUtils.h"
#include "PerforceVCSAdapter.h"
#include "VersionControl/VersionControlInitializer.h"

namespace Private_PerforcePlugin
{

const char* kName = "Perforce Plugin";
const int kVersion = 1;

class CPerforceVersionControl_ClassDesc : public IClassDesc
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VCS_PROVIDER; }
	virtual const char*    ClassName() { return "Perforce"; }
	virtual const char*    Category() { return "VersionControl"; }
	virtual void*          CreateObject()
	{
		return new CPerforceVCSAdapter(PathUtil::MatchAbsolutePathToCaseOnFileSystem(PathUtil::GetGameProjectAssetsPath()));
	}
};

REGISTER_CLASS_DESC(CPerforceVersionControl_ClassDesc);

}

CPerforcePlugin::CPerforcePlugin()
{
	CVersionControlInitializer::Initialize();
}

int32 CPerforcePlugin::GetPluginVersion()
{
	return Private_PerforcePlugin::kVersion;
}

const char* CPerforcePlugin::GetPluginName()
{
	return Private_PerforcePlugin::kName;
}

REGISTER_PLUGIN(CPerforcePlugin);
