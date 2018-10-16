// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlUIHelper.h"
#include "VersionControl/VersionControlFileStatus.h"

namespace VersionControlUIHelper
{

QIcon GetIconFromStatus(int status)
{
	using FS = CVersionControlFileStatus;

	static const int notLatestStatus = FS::eState_UpdatedRemotely | FS::eState_DeletedRemotely;
	static const int notTrackedStatus = FS::eState_NotTracked | FS::eState_DeletedLocally;

	if (status & notLatestStatus)
	{
		return QIcon("icons:VersionControl/not_latest.ico");
	}
	else if (status & FS::eState_CheckedOutRemotely)
	{
		return QIcon("icons:VersionControl/locked.ico");
	}
	else if (status & FS::eState_CheckedOutLocally)
	{
		return QIcon(status & FS::eState_ModifiedLocally ? "icons:VersionControl/checked_out_modified.ico"
			: "icons:VersionControl/checked_out.ico");
	}
	else if (status & FS::eState_AddedLocally)
	{
		return QIcon("icons:VersionControl/new.ico");
	}
	else if (status & FS::eState_DeletedLocally)
	{
		return QIcon("icons:VersionControl/deleted.ico");
	}
	else if (status & notTrackedStatus)
	{
		return QIcon("icons:VersionControl/not_tracked.ico");
	}
	return QIcon("icons:VersionControl/latest.ico");
}

}