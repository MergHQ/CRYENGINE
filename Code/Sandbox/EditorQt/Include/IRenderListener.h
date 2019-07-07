// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct SDisplayContext;

struct IRenderListener
{
	virtual void Render(SDisplayContext& rDisplayContext) = 0;
	virtual ~IRenderListener() {}
};