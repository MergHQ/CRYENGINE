// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IItemSystem.h>
#include "Weapon.h"


class CDebugGun :
  public CWeapon
{
public:
  CDebugGun();
  void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
  void Update(SEntityUpdateContext& ctx, int update);
  void Shoot( bool bPrimary);
	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));		
		s->AddContainer(m_fireModes);
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}

  virtual void Select(bool select);

private:
  ICVar* m_pAIDebugDraw;
  int m_aiDebugDrawPrev;
  
  typedef std::pair<string, float> TFmPair;
  std::vector<TFmPair> m_fireModes;    
  int m_fireMode;
};
