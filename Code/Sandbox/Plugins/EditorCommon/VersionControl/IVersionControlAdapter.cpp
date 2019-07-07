// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "IVersionControlAdapter.h"
#include "VersionControl.h"

std::shared_ptr<CVersionControlCache> IVersionControlAdapter::GetCache()
{
	return CVersionControl::GetInstance().GetCache();
}
 
IVersionControlAdapter::~IVersionControlAdapter()
{
	signalOnlineChanged.DisconnectAll();
}