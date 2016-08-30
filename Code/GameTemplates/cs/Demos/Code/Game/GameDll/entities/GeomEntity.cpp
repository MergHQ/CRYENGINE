// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "GeomEntity.h"
#include "game/GameFactory.h"
#include "flownodes/FlowGameEntityNode.h"


CGeomEntity::CGeomEntity()
{
}

CGeomEntity::~CGeomEntity()
{
}

bool CGeomEntity::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);
	return true;
}

void CGeomEntity::PostInit(IGameObject* pGameObject)
{
	Reset();
}

void CGeomEntity::Release()
{
	ISimpleExtension::Release();
}

bool CGeomEntity::RegisterProperties(SEntityScriptProperties& tables, CGameEntityNodeFactory* pNodeFactory)
{
	if (tables.pPropertiesTable)
	{		
		tables.pPropertiesTable->SetValue("object_Model", "editor/objects/sphere.cgf");
	}

	if (tables.pEditorTable)
	{
		tables.pEditorTable->SetValue("Icon", "prompt.bmp");
		tables.pEditorTable->SetValue("IconOnTop", true);
	}

	if (pNodeFactory)
	{
		std::vector<SInputPortConfig> inputs;
		inputs.push_back(InputPortConfig<string>("LoadGeometry", "Load static geometry"));
		pNodeFactory->AddInputs(inputs, OnFlowgraphActivation);

		std::vector<SOutputPortConfig> outputs;
		outputs.push_back(OutputPortConfig<bool>("Done"));
		pNodeFactory->AddOutputs(outputs);
	}

	return true;
}

void CGeomEntity::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_EDITOR_PROPERTY_CHANGED:
	case ENTITY_EVENT_RESET:
		{
			Reset();
		}
		break;
	}
}

void CGeomEntity::Reset()
{
	SmartScriptTable propertiesTable;
	GetEntity()->GetScriptTable()->GetValue("Properties", propertiesTable);

	const char* geometryPath = "";
	if (propertiesTable->GetValue("object_Model", geometryPath))
	{
		GetEntity()->LoadGeometry(0, geometryPath);
	}
}

void CGeomEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CFlowGameEntityNode* pNode)
{
	if (CGeomEntity* pGeomEntity = QueryExtension(entityId))
	{
		if (IsPortActive(pActInfo, eInputPorts_LoadGeometry))
		{
			pGeomEntity->GetEntity()->LoadGeometry(0, GetPortString(pActInfo, eInputPorts_LoadGeometry));
			ActivateOutputPort(entityId, eOutputPorts_Done, TFlowInputData(true));
		}
	}
}
