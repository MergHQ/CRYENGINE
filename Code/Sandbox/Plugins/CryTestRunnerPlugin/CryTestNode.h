// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include <QString>

struct ICryTestNode
{
	virtual ~ICryTestNode() = default;

	virtual ICryTestNode* GetParent() const = 0;
	virtual size_t        GetChildrenCount() const = 0;
	virtual QString       GetDisplayName() const = 0;
	virtual QString       GetTestSummary() const = 0;
	virtual void          SetIndex(int index) = 0;
	virtual int           GetIndex() const = 0;
	virtual void          RequireRefresh() = 0;
	virtual void          Run() = 0;
	virtual QString       GetOutput() const = 0;
};