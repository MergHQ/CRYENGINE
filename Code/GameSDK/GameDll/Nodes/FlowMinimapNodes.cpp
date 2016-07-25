// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryFlowGraph/IFlowBaseNode.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>
#include <CryEntitySystem/IEntitySystem.h>

//---------------------------------------------------------------------------------------
//------------------------------------- MinimapInfo -------------------------------------
//---------------------------------------------------------------------------------------
class CMiniMapInfo
{
public:
	static CMiniMapInfo* GetInstance()
	{
		static CMiniMapInfo inst;
		return &inst;
	}

	void UpdateLevelInfo()
	{
		ILevelInfo* pLevel = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel();
		if (pLevel)
			m_minimapInfo = pLevel->GetMinimapInfo();
	}

	const ILevelInfo::SMinimapInfo& GetMinimapInfo()
	{
		UpdateLevelInfo();
		return m_minimapInfo;
	}

	void GetPlayerData(IEntity* pEntity, float& px, float& py, int& rot) const
	{
		if (pEntity)
		{
			px = clamp_tpl((pEntity->GetWorldPos().x - m_minimapInfo.fStartX) / m_minimapInfo.fDimX, 0.0f, 1.0f);
			py = clamp_tpl((pEntity->GetWorldPos().y - m_minimapInfo.fStartY) / m_minimapInfo.fDimY, 0.0f, 1.0f);
			rot = (int) (pEntity->GetWorldAngles().z * 180.0f / gf_PI - 90.0f);
		}
	}

private:
	CMiniMapInfo() {}
	CMiniMapInfo(const CMiniMapInfo&) {}
	CMiniMapInfo& operator=(const CMiniMapInfo&) { return *this; }
	~CMiniMapInfo() {}

	ILevelInfo::SMinimapInfo m_minimapInfo;
};

//----------------------------------- MiniMap info node ---------------------------------
class CFlowMiniMapInfoNode
	: public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowMiniMapInfoNode(SActivationInfo* pActInfo)
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Get", _HELP("Get minimap info")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("OnGet",          _HELP("Tirggers of port <Get> is activeated")),
			OutputPortConfig<string>("MapFile",     _HELP("Name of minimap dds file")),
			OutputPortConfig<int>("Width",          _HELP("Minimap width")),
			OutputPortConfig<int>("Height",         _HELP("Minimap height")),
			OutputPortConfig_Void("InvalidMiniMap", _HELP("Triggered when minimap path was empty")),
			{ 0 }
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Info about minimap");
		config.SetCategory(EFLN_ADVANCED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (event == eFE_Activate && IsPortActive(pActInfo, eI_Get))
		{
			const ILevelInfo::SMinimapInfo& mapInfo = CMiniMapInfo::GetInstance()->GetMinimapInfo();
			if (!mapInfo.sMinimapName.empty())
			{
				ActivateOutput(pActInfo, eO_OnGet, true);
				ActivateOutput(pActInfo, eO_MapName, mapInfo.sMinimapName);
				ActivateOutput(pActInfo, eO_MapWidth, mapInfo.iWidth);
				ActivateOutput(pActInfo, eO_MapHeight, mapInfo.iHeight);
			}
			else
			{
				ActivateOutput(pActInfo, eO_InvalidPath, true);
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	enum EInputPorts
	{
		eI_Get,
	};

	enum EOutputPorts
	{
		eO_OnGet = 0,
		eO_MapName,
		eO_MapWidth,
		eO_MapHeight,
		eO_InvalidPath,
	};

};

//----------------------------------- Entity pos info --------------------------------------
class CFlowMiniMapEntityPosInfo
	: public CFlowBaseNode<eNCT_Instanced>
	  , public IEntityEventListener
{
public:
	CFlowMiniMapEntityPosInfo(SActivationInfo* pActInfo)
		: m_entityId(0)
	{
	}

	virtual ~CFlowMiniMapEntityPosInfo()
	{
		UnRegisterEntity();
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Get",     _HELP("Trigger outputs.")),
			InputPortConfig_Void("Enable",  _HELP("Trigger to enable")),
			InputPortConfig_Void("Disable", _HELP("Trigger to enable")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("OnPosChange", _HELP("Triggers if position has changed")),
			OutputPortConfig<float>("PosX",      _HELP("X pos on minimap")),
			OutputPortConfig<float>("PosY",      _HELP("Y pos on minimap")),
			OutputPortConfig<int>("Rotation",    _HELP("Minimap rotation")),
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.SetCategory(EFLN_ADVANCED);

		SetFlagsAndDescriton(config);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			m_actInfo = *pActInfo;
			CMiniMapInfo::GetInstance()->UpdateLevelInfo();
			m_entityId = GetEntityId(pActInfo);
			break;
		case eFE_Activate:
			if (IsPortActive(pActInfo, eI_Trigger))
				TriggerPorts(pActInfo);

			if (IsPortActive(pActInfo, eI_Disable))
				UnRegisterEntity();

			if (IsPortActive(pActInfo, eI_Enable))
			{
				UnRegisterEntity();
				m_entityId = GetEntityId(pActInfo);
				RegisterEntity();
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowMiniMapEntityPosInfo(pActInfo);
	}

	virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
	{
		assert(pEntity->GetId() == m_entityId);
		if (event.event == ENTITY_EVENT_XFORM)
		{
			float px, py;
			int r;
			CMiniMapInfo::GetInstance()->GetPlayerData(pEntity, px, py, r);
			ActivateOutput(&m_actInfo, eO_OnPosChange, true);
			ActivateOutput(&m_actInfo, eO_PosX, px);
			ActivateOutput(&m_actInfo, eO_PosY, py);
			ActivateOutput(&m_actInfo, eO_Rotation, r);
		}
	}

protected:
	virtual EntityId GetEntityId(SActivationInfo* pActInfo)
	{
		return pActInfo->pEntity != 0 ? pActInfo->pEntity->GetId() : 0;
	}

	virtual void SetFlagsAndDescriton(SFlowNodeConfig& config)
	{
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.sDescription = _HELP("Info about entity position on minimap");
	}

private:
	void RegisterEntity()
	{
		if (m_entityId == 0)
			return;
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
		if (pEntity != 0)
		{
			gEnv->pEntitySystem->AddEntityEventListener(m_entityId, ENTITY_EVENT_XFORM, this);
			return;
		}
		m_entityId = 0;
	}

	void UnRegisterEntity()
	{
		if (m_entityId == 0)
			return;

		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
		if (pEntity != 0)
		{
			gEnv->pEntitySystem->RemoveEntityEventListener(m_entityId, ENTITY_EVENT_XFORM, this);
			return;
		}
		m_entityId = 0;
	}

	void TriggerPorts(SActivationInfo* pActInfo)
	{
		if (m_entityId == 0)
			return;

		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
		if (pEntity)
		{
			float px, py;
			int r;
			CMiniMapInfo::GetInstance()->GetPlayerData(pEntity, px, py, r);
			ActivateOutput(&m_actInfo, eO_OnPosChange, true);
			ActivateOutput(&m_actInfo, eO_PosX, px);
			ActivateOutput(&m_actInfo, eO_PosY, py);
			ActivateOutput(&m_actInfo, eO_Rotation, r);
		}
	}

private:
	EntityId        m_entityId;
	SActivationInfo m_actInfo;

	enum EInputPorts
	{
		eI_Trigger = 0,
		eI_Enable,
		eI_Disable,
	};

	enum EOutputPorts
	{
		eO_OnPosChange,
		eO_PosX,
		eO_PosY,
		eO_Rotation,
	};
};

//----------------------------------- Player pos info --------------------------------------
class CFlowMiniMapPlayerPosInfo
	: public CFlowMiniMapEntityPosInfo
{
public:
	CFlowMiniMapPlayerPosInfo(SActivationInfo* pActInfo)
		: CFlowMiniMapEntityPosInfo(pActInfo)
	{
	}

	virtual ~CFlowMiniMapPlayerPosInfo()
	{
	}

protected:
	virtual EntityId GetEntityId(SActivationInfo* pActInfo)
	{
		return gEnv->pGame->GetIGameFramework()->GetClientActorId();
	}

	virtual void SetFlagsAndDescriton(SFlowNodeConfig& config)
	{
		config.sDescription = _HELP("Info about player position on minimap");
	}

};

REGISTER_FLOW_NODE("Minimap:MapInfo", CFlowMiniMapInfoNode);
REGISTER_FLOW_NODE("Minimap:PlayerPos", CFlowMiniMapPlayerPosInfo);
REGISTER_FLOW_NODE("Minimap:EntityPos", CFlowMiniMapEntityPosInfo);
