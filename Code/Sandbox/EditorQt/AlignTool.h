// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <LevelEditor/LevelEditorSharedState.h>
#include <Preferences/SnappingPreferences.h>

class CAlignPickCallback : public IPickObjectCallback
{
public:
	CAlignPickCallback()
	{ 
		m_bActive = true; 
	}

	~CAlignPickCallback() 
	{ 
		m_bActive = false; 
		gSnappingPreferences.EnablePivotSnapping(false);
	}

	//! Called when object picked.
	virtual void OnPick(CBaseObject* picked);
	//! Called when pick mode cancelled.
	virtual void OnCancelPick();
	//! Return true if specified object is pickable.
	virtual bool OnPickFilter(CBaseObject* filterObject);

	static bool  IsActive() { return m_bActive; }

private:
	static bool m_bActive;
};
