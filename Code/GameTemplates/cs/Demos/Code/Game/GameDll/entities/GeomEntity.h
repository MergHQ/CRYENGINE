// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#pragma once


struct SEntityScriptProperties;
class CGameEntityNodeFactory;


class CGeomEntity : public CGameObjectExtensionHelper<CGeomEntity, ISimpleExtension>
{
public:
	CGeomEntity();
	virtual ~CGeomEntity();

	//ISimpleExtension
	virtual bool Init(IGameObject* pGameObject) override;
	virtual void PostInit(IGameObject* pGameObject) override;
	virtual void Release() override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	//~ISimpleExtension

	static bool RegisterProperties(SEntityScriptProperties& tables, CGameEntityNodeFactory* pNodeFactory);
	
private:
	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CFlowGameEntityNode* pNode);
	void Reset();

	enum EFlowgraphInputPorts
	{
		eInputPorts_LoadGeometry,
	};

	enum EFlowgraphOutputPorts
	{
		eOutputPorts_Done,
	};

	string m_geometryPath;
};