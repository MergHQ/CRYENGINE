// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - Created by Marco Corbetta

*************************************************************************/

#ifndef __WorldBuilder_h__
#define __WorldBuilder_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "ILevelSystem.h"
#include "PrefabManager.h"

////////////////////////////////////////////////////////////////////////////
inline uint hash(int key)
{
	key += ~(key << 15);
	key ^= (key >> 10);
	key += (key << 3);
	key ^= (key >> 6);
	key += ~(key << 11);
	key ^= (key >> 16);
	return key;
}

inline uint VecHash(const Vec3 &vPos)
{
	uint x=*((unsigned *) & vPos.x)*73856093;
	uint y=*((unsigned *) & vPos.y)*19349663;
	uint z=*((unsigned *) & vPos.z)*83492791;
	return (x^y^z);
}

inline uint VecHash2(const Vec3 &vPos)
{
	uint x=(unsigned)(vPos.x*73856093);
	uint y=(unsigned)(vPos.y*19349663);
	uint z=(unsigned)(vPos.z*83492791);
	return hash(x^hash(y^z));
}

////////////////////////////////////////////////////////////////////////////
class CWorldBuilder : public ILevelSystemListener
{
public:
	CWorldBuilder();
	~CWorldBuilder();
	
	// ILevelSystemListener
	virtual bool OnLoadingStart(ILevelInfo* pLevel) override;
	virtual void OnLoadingComplete(ILevelInfo* pLevel) override;
	// ~ILevelSystemListener

	CryGame::CPrefabManager &GetPrefabManager() { return (m_PrefabManager); }

protected:
	// Draw debug info
	void DrawDebugInfo();

	CryGame::CPrefabManager m_PrefabManager;

};

#endif // WorldBuilder
