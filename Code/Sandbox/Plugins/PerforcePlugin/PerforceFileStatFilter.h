// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

enum class EFileStatFilter
{
	None = 0,
	AddedLocally,
	AddedRemotely,
	CheckedOutByOthers,
	Tracked,
	DeletedRemotely
};
