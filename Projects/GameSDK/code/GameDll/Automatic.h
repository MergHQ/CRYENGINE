// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Single.h"

class CAutomatic : public CSingle
{
public:
	CRY_DECLARE_GTI(CAutomatic);
	
	CAutomatic();
	virtual ~CAutomatic();

	// CSingle
	virtual void StartReload(int zoomed) override;

	virtual void GetMemoryUsage(ICrySizer * s) const override;
	void GetInternalMemoryUsage(ICrySizer * s) const;
	virtual void Update(float frameTime, uint32 frameId) override;
	virtual void StartFire() override;
	virtual void StopFire() override;
	// ~CSingle

private:
	
	typedef CSingle BaseClass;

};
