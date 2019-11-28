// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryScriptSystem/IScriptSystem.h>
#include <CryMemory/CrySizer.h>

struct IItemSystem;
struct IGameFramework;
class CItem;
class CWeapon;


class CScriptBind_Weapon :
	public CScriptableBase
{
public:
	CScriptBind_Weapon(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_Weapon();

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	void AttachTo(CWeapon *pWeapon);

	int SetAmmoCount(IFunctionHandler *pH);
	int GetAmmoCount(IFunctionHandler *pH);
	int GetClipSize(IFunctionHandler *pH);

	int GetAmmoType(IFunctionHandler *pH);

	int SupportsAccessory(IFunctionHandler *pH, const char *accessoryName);
	int GetAccessory(IFunctionHandler *pH, const char *accessoryName);
	int AttachAccessory(IFunctionHandler *pH, const char *className, bool attach, bool force);
	int SwitchAccessory(IFunctionHandler *pH, const char *className);

	int SetCurrentFireMode(IFunctionHandler *pH, const char *name);

	int Reload(IFunctionHandler *pH);

private:
	void RegisterGlobals();
	void RegisterMethods();

	CItem *GetItem(IFunctionHandler *pH);
	CWeapon *GetWeapon(IFunctionHandler *pH);

	ISystem						*m_pSystem;
	IGameFramework		*m_pGameFW;
};
