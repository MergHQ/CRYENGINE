#include "StdAfx.h"
#include "GeomEntity.h"

#include "Helpers/EntityFlowNode.h"

#include <CryPhysics/physinterface.h>
#include <CryAnimation/ICryAnimation.h>

class CGeomEntityRegistrator
	: public IEntityRegistrator
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
		RegisterEntityProperty<bool>(pPropertyHandler, "ReceiveCollisionEvents", "", "0", "Whether or not the entity should receive and report on collision events, for example to activate flownode output ports");
		RegisterEntityProperty<float>(pPropertyHandler, "Mass", "", "1", "Sets the mass of the entity", 0.00001f, 100000.f);

		{
			SEntityPropertyGroupHelper animationGroup(pPropertyHandler, "Animations");

			RegisterEntityProperty<string>(pPropertyHandler, "Animation", "", "", "Specifies the animation to play");
			RegisterEntityProperty<float>(pPropertyHandler, "Speed", "", "1", "Speed at which to play the animation", 0.00001f, 10000.f);
			RegisterEntityProperty<bool>(pPropertyHandler, "Loop", "", "1", "Whether or not the animation should loop or only play once");
		}

		// Register flow node
		// Factory will be destroyed by flowsystem during shutdown
		CEntityFlowNodeFactory* pFlowNodeFactory = new CEntityFlowNodeFactory("entity:GeomEntity");

		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Hide", ""));
		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("UnHide", ""));
		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<string>("LoadGeometry", ""));
		pFlowNodeFactory->m_activateCallback = CGeomEntity::OnFlowgraphActivation;

		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnHide"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<bool>("OnUnHide"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<EntityId>("OnCollision"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<string>("CollisionSurfaceName"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<string>("OnGeometryChanged"));

		pFlowNodeFactory->Close();
	}
};

CGeomEntityRegistrator g_geomEntityRegistrator;

CGeomEntity::CGeomEntity()
	: m_bHide(false)
	, m_geometryName("")
{
}

void CGeomEntity::PostInit(IGameObject *pGameObject)
{
	CNativeEntityBase::PostInit(pGameObject);

	GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_CASTSHADOW);
}

void CGeomEntity::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_HIDE || event.event == ENTITY_EVENT_UNHIDE)
	{
		m_bHide = (event.event == ENTITY_EVENT_HIDE);
	}

	CNativeEntityBase::ProcessEvent(event);
}

void CGeomEntity::HandleEvent(const SGameObjectEvent &event)
{
	if (event.event == eGFE_OnCollision)
	{
		// Collision info can be retrieved using the event pointer
		EventPhysCollision *physCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);

		ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
		if(ISurfaceType* pSurfaceType = pSurfaceTypeManager->GetSurfaceType(physCollision->idmat[1]))
		{
			string surfaceTypeName = pSurfaceType->GetName();
			ActivateFlowNodeOutput(eOutputPort_CollisionSurfaceName, TFlowInputData(surfaceTypeName));
		}

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
	if (m_bHide != GetEntity()->IsHidden())
	{
		GetEntity()->Hide(m_bHide);

		if (GetEntity()->IsHidden())
		{
			ActivateFlowNodeOutput(eOutputPort_OnHide, TFlowInputData(m_bHide));
		}
		else
		{
			ActivateFlowNodeOutput(eOutputPort_OnUnHide, TFlowInputData(!m_bHide));
		}
	}

	const char* szModelPath = m_geometryName.empty() ? GetPropertyValue(eProperty_Model) : m_geometryName.c_str();
	if (strlen(szModelPath) > 0)
	{
		auto& gameObject = *GetGameObject();

		const int geometrySlot = 0;
		LoadMesh(geometrySlot, szModelPath);
		ActivateFlowNodeOutput(eOutputPort_OnGeometryChanged, TFlowInputData(string(szModelPath)));

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

		bool bReceiveCollisionEvents = GetPropertyBool(eProperty_ReceiveCollisionEvents);

		// Make sure we get logged collision events
		// Note the difference between immediate (directly on the physics thread) and logged (delayed to run on main thread) physics events.
		GetGameObject()->EnablePhysicsEvent(bReceiveCollisionEvents, eEPE_OnCollisionLogged);

		const int requiredEvents[] = { eGFE_OnCollision };

		if (bReceiveCollisionEvents)
		{
			GetGameObject()->RegisterExtForEvents(this, requiredEvents, sizeof(requiredEvents) / sizeof(int));
		}
		else
		{
			GetGameObject()->UnRegisterExtForEvents(this, requiredEvents, sizeof(requiredEvents) / sizeof(int));
		}

		const char* szAnimationName = GetPropertyValue(eProperty_Animation);
		if (strlen(szAnimationName) > 0)
		{
			if (auto* pCharacter = GetEntity()->GetCharacter(geometrySlot))
			{
				CryCharAnimationParams animParams;
				animParams.m_fPlaybackSpeed = GetPropertyFloat(eProperty_Speed);
				animParams.m_nFlags = GetPropertyBool(eProperty_Loop) ? CA_LOOP_ANIMATION : 0;

				pCharacter->GetISkeletonAnim()->StartAnimation(szAnimationName, animParams);
			}
			else
			{
				gEnv->pLog->LogWarning("Tried to play back animation %s on entity with no character! Make sure to use a CDF or CHR geometry file!", szAnimationName);
			}
		}
	}
}

void CGeomEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode)
{
	if (auto* pGameObject = gEnv->pGameFramework->GetGameObject(entityId))
	{
		auto* pGeomEntity = static_cast<CGeomEntity*>(pGameObject->QueryExtension("GeomEntity"));
		if (IsPortActive(pActInfo, eInputPort_OnHide) || IsPortActive(pActInfo, eInputPort_OnUnHide))
		{
			pGeomEntity->m_bHide = IsPortActive(pActInfo, eInputPort_OnHide);
			pGeomEntity->OnResetState();
		}
		else if (IsPortActive(pActInfo, eInputPort_LoadGeometry))
		{
			pGeomEntity->m_geometryName = GetPortString(pActInfo, eInputPort_LoadGeometry);
			pGeomEntity->OnResetState();
		}
	}
}
