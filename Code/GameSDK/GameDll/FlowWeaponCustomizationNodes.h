// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   FlowWeaponCustomizationNodes.h
//  Version:     v1.00
//  Created:     03/05/2012 by Michiel Meesters.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __FlowWeaponCustomizationNodes_H__
#define __FlowWeaponCustomizationNodes_H__
#include "Nodes/G2FlowBaseNode.h"

//--------------------------------------------------------------------------------------------
class CFlashUIInventoryNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CFlashUIInventoryNode( SActivationInfo * pActInfo );
	~CFlashUIInventoryNode();

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );
	virtual void GetMemoryUsage(ICrySizer * s) const { s->Add(*this); }

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlashUIInventoryNode(pActInfo);
	}

private:
	enum EInputs
	{
		eI_Call = 0,
	};

	enum EOutputs
	{
		eO_OnCall = 0,
		eO_Args,
	};
};

//--------------------------------------------------------------------------------------------
class CFlashUIGetEquippedAccessoriesNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CFlashUIGetEquippedAccessoriesNode( SActivationInfo * pActInfo ) {};
	~CFlashUIGetEquippedAccessoriesNode();

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );
	virtual void GetMemoryUsage(ICrySizer * s) const { s->Add(*this); }

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlashUIGetEquippedAccessoriesNode(pActInfo);
	}

private:
	enum EInputs
	{
		eI_Call = 0,
		eI_Weapon,
	};

	enum EOutputs
	{
		eO_OnCall = 0,
		eO_Args,
	};
};

//--------------------------------------------------------------------------------------------
class CFlashUIGetCompatibleAccessoriesNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CFlashUIGetCompatibleAccessoriesNode ( SActivationInfo * pActInfo ) {};
	~CFlashUIGetCompatibleAccessoriesNode ();

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );
	virtual void GetMemoryUsage(ICrySizer * s) const { s->Add(*this); }
	
	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlashUIGetCompatibleAccessoriesNode(pActInfo);
	}
private:
	enum EInputs
	{
		eI_Call = 0,
		eI_Weapon,
	};

	enum EOutputs
	{
		eO_OnCall = 0,
		eO_Args,
	};
};

//--------------------------------------------------------------------------------------------
class CFlashUICheckAccessoryState: public CFlowBaseNode<eNCT_Instanced>
{
public:
	CFlashUICheckAccessoryState ( SActivationInfo * pActInfo ) {};
	~CFlashUICheckAccessoryState ();

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );
	virtual void GetMemoryUsage(ICrySizer * s) const { s->Add(*this); }

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlashUICheckAccessoryState(pActInfo);
	}
private:
	enum EInputs
	{
		eI_Call = 0,
		eI_Accessory,
		eI_Weapon,
	};

	enum EOutputs
	{
		eO_Equipped = 0,
		eO_InInventory,
		eO_DontHave,
	};
};

//--------------------------------------------------------------------------------------------
class CSetEquipmentLoadoutNode: public CFlowBaseNode<eNCT_Instanced>
{
public:
	CSetEquipmentLoadoutNode ( SActivationInfo * pActInfo ) {};
	~CSetEquipmentLoadoutNode () {};

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );
	virtual void GetMemoryUsage(ICrySizer * s) const { s->Add(*this); }

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CSetEquipmentLoadoutNode(pActInfo);
	}
private:
	enum EInputs
	{
		eI_Set = 0,
		eI_EquipLoadout,
	};

	enum EOutputs
	{
	};
};


#endif