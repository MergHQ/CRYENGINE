#include "StdAfx.h"
#include "GeomEntity.h"

#include "Helpers/EntityFlowNode.h"

#include <CryPhysics/physinterface.h>
#include <CryAnimation/ICryAnimation.h>

#include <CrySerialization/Enum.h>

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

		RegisterEntityWithDefaultComponent<CGeomEntity>("GeomEntity", "Geometry", "physicsobject.bmp", true);

		// Register flow node
		// Factory will be destroyed by flowsystem during shutdown
		pFlowNodeFactory = new CEntityFlowNodeFactory("entity:GeomEntity");

		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("Hide", ""));
		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<bool>("UnHide", ""));
		pFlowNodeFactory->m_inputs.push_back(InputPortConfig<string>("LoadGeometry", ""));
		pFlowNodeFactory->m_activateCallback = CGeomEntity::OnFlowgraphActivation;

		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig_Void("OnHide"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig_Void("OnUnHide"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<EntityId>("OnCollision"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<string>("CollisionSurfaceName"));
		pFlowNodeFactory->m_outputs.push_back(OutputPortConfig<string>("OnGeometryChanged"));

		pFlowNodeFactory->Close();
	}

public:
	~CGeomEntityRegistrator()
	{
		if (pFlowNodeFactory)
			pFlowNodeFactory->UnregisterFactory();
		pFlowNodeFactory = nullptr;
	}

private:
	_smart_ptr<CEntityFlowNodeFactory> pFlowNodeFactory = nullptr;
};

CGeomEntityRegistrator g_geomEntityRegistrator;

YASLI_ENUM_BEGIN_NESTED(CGeomEntity, EPhysicalizationType, "PhysicalizationType")
YASLI_ENUM_VALUE_NESTED(CGeomEntity, ePhysicalizationType_None, "None")
YASLI_ENUM_VALUE_NESTED(CGeomEntity, ePhysicalizationType_Static, "Static")
YASLI_ENUM_VALUE_NESTED(CGeomEntity, ePhysicalizationType_Rigid, "Rigid")
YASLI_ENUM_VALUE_NESTED(CGeomEntity, ePhysicalizationType_Area, "Area")
YASLI_ENUM_END()

CRYREGISTER_CLASS(CGeomEntity);

void CGeomEntity::Initialize()
{
	CDesignerEntityComponent::Initialize();

	GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_CASTSHADOW);
}

void CGeomEntity::ProcessEvent(SEntityEvent& event)
{
	CDesignerEntityComponent::ProcessEvent(event);

	switch (event.event)
	{
		case ENTITY_EVENT_HIDE:
			{
				ActivateFlowNodeOutput(eOutputPort_OnHide, TFlowInputData());
			}
			break;
		case ENTITY_EVENT_UNHIDE:
			{
				ActivateFlowNodeOutput(eOutputPort_OnUnHide, TFlowInputData());
			}
			break;
		case ENTITY_EVENT_COLLISION:
			{
				// Collision info can be retrieved using the event pointer
				EventPhysCollision *physCollision = reinterpret_cast<EventPhysCollision *>(event.nParam[0]);

				ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
				if (ISurfaceType* pSurfaceType = pSurfaceTypeManager->GetSurfaceType(physCollision->idmat[1]))
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
			break;
	}
}

void CGeomEntity::SetGeometry(const char* szFilePath)
{
	m_model = szFilePath;

	OnResetState();
}

void CGeomEntity::OnResetState()
{
	if (m_model.size() > 0)
	{
		const int geometrySlot = 0;
		LoadMesh(geometrySlot, m_model);
		ActivateFlowNodeOutput(eOutputPort_OnGeometryChanged, TFlowInputData(m_model));

		SEntityPhysicalizeParams physicalizationParams;
		SEntityPhysicalizeParams::AreaDefinition areaDef;
		pe_params_buoyancy buoy;

		switch (m_physicalizationType)
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
			case ePhysicalizationType_Area:
				physicalizationParams.type = PE_AREA;
				physicalizationParams.pAreaDef = &areaDef;
				areaDef.areaType = SEntityPhysicalizeParams::AreaDefinition::AREA_GEOMETRY;
				physicalizationParams.nSlot = 0;
				physicalizationParams.pBuoyancy = &buoy;
				break;
		}

		physicalizationParams.mass = m_mass;

		GetEntity()->Physicalize(physicalizationParams);

		if(m_animation.size() > 0)
		{
			if (auto* pCharacter = GetEntity()->GetCharacter(geometrySlot))
			{
				CryCharAnimationParams animParams;
				animParams.m_fPlaybackSpeed = m_animationSpeed;
				animParams.m_nFlags = m_bLoopAnimation ? CA_LOOP_ANIMATION : 0;

				pCharacter->GetISkeletonAnim()->StartAnimation(m_animation, animParams);
			}
			else
			{
				gEnv->pLog->LogWarning("Tried to play back animation %s on entity with no character! Make sure to use a CDF or CHR geometry file!", m_animation.c_str());
			}
		}
	}
}

void CGeomEntity::OnFlowgraphActivation(EntityId entityId, IFlowNode::SActivationInfo* pActInfo, const class CEntityFlowNode* pNode)
{
	if (auto* pEntity = gEnv->pEntitySystem->GetEntity(entityId))
	{
		if (IsPortActive(pActInfo, eInputPort_UnHide) || IsPortActive(pActInfo, eInputPort_Hide))
		{
			pEntity->Hide(IsPortActive(pActInfo, eInputPort_Hide));
		}
		else if (IsPortActive(pActInfo, eInputPort_Geometry))
		{
			auto* pGeomEntity = pEntity->GetComponent<CGeomEntity>();

			pGeomEntity->m_model = GetPortString(pActInfo, eInputPort_Geometry);
			pGeomEntity->OnResetState();
		}
	}
}
