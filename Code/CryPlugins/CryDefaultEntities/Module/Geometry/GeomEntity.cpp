#include "StdAfx.h"
#include "GeomEntity.h"

#include "Helpers/EntityFlowNode.h"

#include <CryPhysics/physinterface.h>

class CGeomEntityRegistrator
	: public IEntityRegistrator
	, public IFlowNodeRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("GeomEntity") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class GeomEntity, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CGeomEntity>("GeomEntity", "Geometry");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("GeomEntity");
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityPropertyObject(pPropertyHandler, "Geometry", "object_Model", "", "Sets the object of the entity");
		RegisterEntityPropertyEnum(pPropertyHandler, "Physicalize", "", "1", "Determines the physicalization type of the entity - None, Static or Rigid (movable)", 0, 2);
		RegisterEntityProperty<float>(pPropertyHandler, "Mass", "", "1", "Sets the mass of the entity", 0.00001f, 100000.f);
		RegisterEntityProperty<bool>(pPropertyHandler, "Hide", "", "0", "Sets the visibility of the entity");

		// Register flow node
		m_pFlowNodeFactory = new CEntityFlowNodeFactory("entity:GeomEntity");

		m_pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Hide", ""));
		m_pFlowNodeFactory->m_activateCallback = CGeomEntity::OnFlowgraphActivation;

		m_pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnHide"));
		m_pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnCollision"));

		m_pFlowNodeFactory->Close();
	}
};

CGeomEntityRegistrator g_geomEntityRegistrator;

CGeomEntity::CGeomEntity()
	: m_bHide(false)
{
}

void CGeomEntity::PostInit(IGameObject *pGameObject)
{
	CNativeEntityBase::PostInit(pGameObject);

	GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_CASTSHADOW);

	// Make sure we get logged collision events
	// Note the difference between immediate (directly on the physics thread) and logged (delayed to run on main thread) physics events.
	pGameObject->EnablePhysicsEvent(true, eEPE_OnCollisionLogged);

	const int requiredEvents[] = { eGFE_OnCollision };
	pGameObject->RegisterExtForEvents(this, requiredEvents, sizeof(requiredEvents) / sizeof(int));
}

void CGeomEntity::HandleEvent(const SGameObjectEvent &event)
{
	if (event.event == eGFE_OnCollision)
	{
		// Collision info can be retrieved using the event pointer
		EventPhysCollision *physCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);

		if (IEntity* pOtherEntity = gEnv->pEntitySystem->GetEntityFromPhysics(physCollision->pEntity[1]))
		{
			ActivateFlowNodeOutput(eOutputPort_OnCollision, TFlowInputData(pOtherEntity->GetId()));
		}
		else
		{
			ActivateFlowNodeOutput(eOutputPort_OnCollision, TFlowInputData());
		}
	}
}

void CGeomEntity::OnResetState()
{
	if (GetPropertyBool(eProperty_Hide) != GetEntity()->IsHidden())
	{
		m_bHide = GetPropertyBool(eProperty_Hide);
	}
	else if (m_bHide != GetEntity()->IsHidden())
	{
		SetPropertyBool(eProperty_Hide, m_bHide);
	}

	GetEntity()->Hide(m_bHide);

	ActivateFlowNodeOutput(eOutputPort_OnHide, TFlowInputData(m_bHide));

	const char* modelPath = GetPropertyValue(eProperty_Model);
	if (strlen(modelPath) > 0)
	{
		auto& gameObject = *GetGameObject();

		const int geometrySlot = 0;
		LoadMesh(geometrySlot, modelPath);

		SEntityPhysicalizeParams physicalizationParams;

		switch ((EPhysicalizationType)GetPropertyInt(eProperty_PhysicalizationType))
		{
			case ePhysicalizationType_None:
				physicalizationParams.type = PE_NONE;
				break;
			case ePhysicalizationType_Static:
				physicalizationParams.type = PE_STATIC;
				break;
			case ePhysicalizationType_Rigid:
				physicalizationParams.type = PE_RIGID;
				break;
		}

		physicalizationParams.mass = GetPropertyFloat(eProperty_Mass);

		GetEntity()->Physicalize(physicalizationParams);
	}
}

void CGeomEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode)
{
	auto* pGameObject = gEnv->pGameFramework->GetGameObject(entityId);
	auto* pGeomEntity = static_cast<CGeomEntity*>(pGameObject->QueryExtension("GeomEntity"));

	if (IsPortActive(pActInfo, eInputPort_OnHide))
	{
		pGeomEntity->m_bHide = GetPortBool(pActInfo, eInputPort_OnHide);
		pGeomEntity->OnResetState();
	}
}
