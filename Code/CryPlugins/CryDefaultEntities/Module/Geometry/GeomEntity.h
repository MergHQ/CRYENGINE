#pragma once

#include "Helpers/NativeEntityBase.h"

class CGeomEntity : public CNativeEntityBase
{
public:
	enum EProperties
	{
		eProperty_Model = 0,
		eProperty_PhysicalizationType,
		eProperty_ReceiveCollisionEvents,
		eProperty_Mass,

		ePropertyGroup_AnimationsBegin,
		eProperty_Animation,
		eProperty_Speed,
		eProperty_Loop,
		ePropertyGroup_AnimationsEnd,

		eNumProperties,
	};

	enum EInputPorts
	{
		eInputPort_OnHide = 0,
		eInputPort_OnUnHide,
		eInputPort_LoadGeometry
	};

	enum EOutputPorts
	{
		eOutputPort_OnHide = 0,
		eOutputPort_OnUnHide,
		eOutputPort_OnCollision,
		eOutputPort_CollisionSurfaceName,
		eOutputPort_OnGeometryChanged
	};

	enum EPhysicalizationType
	{
		ePhysicalizationType_None = 0,
		ePhysicalizationType_Static,
		ePhysicalizationType_Rigid
	};

public:
	CGeomEntity();
	virtual ~CGeomEntity() {}

	// ISimpleExtension
	virtual void PostInit(IGameObject *pGameObject) final;

	virtual void ProcessEvent(SEntityEvent& event) override;

	virtual void HandleEvent(const SGameObjectEvent &event) final;

	virtual void OnResetState() final;
	// ~ISimpleExtension

public:
	static void OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode);

protected:
	IPhysicalEntity* m_pPhysEnt;

	bool m_bHide;
	string m_geometryName;
};
