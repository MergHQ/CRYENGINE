#include "StdAfx.h"
#include "AreaComponent.h"

#include "CharacterControllerComponent.h"
#include "Vehicles/VehicleComponent.h"
#include "RigidBodyComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{
void CAreaComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
}

static void ReflectType(Schematyc::CTypeDesc<CAreaComponent::EType>& desc)
{
	desc.SetGUID("{C42A564A-0024-4030-9381-1BC9004EA0E1}"_cry_guid);
	desc.SetLabel("Area Shape");
	desc.SetDescription("Shape to use for an area");
	desc.SetDefaultValue(CAreaComponent::EType::Box);
	desc.AddConstant(CAreaComponent::EType::Box, "Box", "Box");
	desc.AddConstant(CAreaComponent::EType::Sphere, "Sphere", "Sphere");
	desc.AddConstant(CAreaComponent::EType::Cylinder, "Cylinder", "Cylinder");
	desc.AddConstant(CAreaComponent::EType::Capsule, "Capsule", "Capsule");
}

void CAreaComponent::ReflectType(Schematyc::CTypeDesc<CAreaComponent>& desc)
{
	desc.SetGUID(CAreaComponent::IID());
	desc.SetEditorCategory("Physics");
	desc.SetLabel("Area");
	desc.SetDescription("Creates a physical representation of the entity, ");
	desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::Singleton });

	// Entities can only have one physical entity type, thus these are incompatible
	desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, cryiidof<CCharacterControllerComponent>());
	desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, cryiidof<CVehiclePhysicsComponent>());
	desc.AddComponentInteraction(SEntityComponentRequirements::EType::Incompatibility, cryiidof<CRigidBodyComponent>());

	desc.AddMember(&CAreaComponent::m_type, 'type', "Type", "Type", "Whether to apply custom gravity in this area", CAreaComponent::EType::Box);
	desc.AddMember(&CAreaComponent::m_shapeParameters, 'para', "Shape", "Shape Properties", "Dimensions of the area", SShapeParameters(nullptr));

	desc.AddMember(&CAreaComponent::m_bCustomGravity, 'apgr', "ApplyGravity", "Custom Gravity", "Whether to apply custom gravity in this area", false);
	desc.AddMember(&CAreaComponent::m_gravity, 'grav', "Gravity", "Gravity", "The gravity value to use", ZERO);
	desc.AddMember(&CAreaComponent::m_buoyancyParameters, 'buoy', "Buoyancy", "Buoyancy Parameters", "Fluid behavior of the area", SBuoyancyParameters());
}

static void ReflectType(Schematyc::CTypeDesc<CAreaComponent::SBuoyancyParameters::EType>& desc)
{
	desc.SetGUID("{ABBCDB1B-75CD-4B9B-B4F7-D6C43DF38AAE}"_cry_guid);
	desc.SetLabel("Buoyancy Type");
	desc.SetDescription("Type of buoyancy to apply");
	desc.SetDefaultValue(CAreaComponent::SBuoyancyParameters::EType::Air);
	desc.AddConstant(CAreaComponent::SBuoyancyParameters::EType::None, "None", "Disabled");
	desc.AddConstant(CAreaComponent::SBuoyancyParameters::EType::Air, "Air", "Air");
	desc.AddConstant(CAreaComponent::SBuoyancyParameters::EType::Water, "Water", "Water");
}

static void ReflectType(Schematyc::CTypeDesc<CAreaComponent::SBuoyancyParameters>& desc)
{
	desc.SetGUID("{A7270990-667F-4D27-B277-CF7ACFF3343C}"_cry_guid);
	desc.SetLabel("Buoyancy Parameters");
	desc.AddMember(&CAreaComponent::SBuoyancyParameters::medium, 'medi', "Medium", "Type", "Type of buoyancy to apply", CAreaComponent::SBuoyancyParameters::EType::Air);
	desc.AddMember(&CAreaComponent::SBuoyancyParameters::flow, 'flow', "Flow", "Flow", "The flow of the air or water, in meters per second", Vec3(0, 5, 0));
	desc.AddMember(&CAreaComponent::SBuoyancyParameters::density, 'dens', "Density", "Density", "Density of the fluid", 1.f);
	desc.AddMember(&CAreaComponent::SBuoyancyParameters::resistance, 'rest', "Resistance", "Resistance", "Resistance of the fluid", 1.f);
}

static void ReflectType(Schematyc::CTypeDesc<CAreaComponent::SShapeParameters>& desc)
{
	desc.SetGUID("{8A493322-1DCD-442B-BA5A-9C41133AE911}"_cry_guid);
	desc.SetLabel("Shape Parameters");
}

CAreaComponent::CAreaComponent()
	: m_shapeParameters{ this } 
{
}

CAreaComponent::~CAreaComponent()
{
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_NONE;
	m_pEntity->Physicalize(physParams);
}

void CAreaComponent::Initialize()
{
	Physicalize();
}

void CAreaComponent::SShapeParameters::Serialize(Serialization::IArchive& archive)
{
	switch (pComponent->GetType())
	{
		case CAreaComponent::EType::Box:
		{
			archive(size, "BoxSize", "Size");
		}
		break;
		case CAreaComponent::EType::Sphere:
		{
			archive(Serialization::Range(size.x, 0.f, 10000.f), "Radius", "Radius");
		}
		break;
		case CAreaComponent::EType::Cylinder:
		case CAreaComponent::EType::Capsule:
		{
			archive(Serialization::Range(size.x, 0.f, 10000.f), "Radius", "Radius");
			archive(Serialization::Range(size.y, 0.f, 10000.f), "Height", "Height");
		}
		break;
	}
}

void CAreaComponent::Physicalize()
{
	// Always dephysicalize first, otherwise set gravity won't be reset
	SEntityPhysicalizeParams physParams;
	physParams.type = PE_NONE;
	m_pEntity->Physicalize(physParams);

	IGeometry* pGeometry;

	switch (m_type)
	{
	case CAreaComponent::EType::Box:
	{
		// Now create the new area physics
		primitives::box primitive;
		primitive.size = m_shapeParameters.size;
		primitive.Basis = IDENTITY;
		primitive.center = Vec3(0, 0, primitive.size.z);
		primitive.bOriented = 1;

		pGeometry = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(static_cast<int>(primitive.type), &primitive);
	}
	break;
	case CAreaComponent::EType::Sphere:
	{
		primitives::sphere primitive;
		primitive.center = ZERO;
		primitive.r = m_shapeParameters.size.x;

		pGeometry = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(static_cast<int>(primitive.type), &primitive);
	}
	break;
	case CAreaComponent::EType::Cylinder:
	{
		primitives::cylinder primitive;
		primitive.center = ZERO;
		primitive.axis = Vec3(0, 0, 1);
		primitive.r = m_shapeParameters.size.x;
		primitive.hh = m_shapeParameters.size.y * 0.5f;

		pGeometry = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(static_cast<int>(primitive.type), &primitive);
	}
	break;
	case CAreaComponent::EType::Capsule:
	{
		primitives::cylinder primitive;
		primitive.center = ZERO;
		primitive.axis = Vec3(0, 0, 1);
		primitive.r = m_shapeParameters.size.x;
		primitive.hh = m_shapeParameters.size.y * 0.5f;

		pGeometry = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(static_cast<int>(primitive.type), &primitive);
	}
	break;
	default:
		CRY_ASSERT(false);
		return;
	}

	IPhysicalEntity* pPhysicalEntity = gEnv->pPhysicalWorld->AddArea(pGeometry, m_pEntity->GetWorldPos(), m_pEntity->GetWorldRotation(), m_pEntity->GetScale().GetLength());
	m_pEntity->AssignPhysicalEntity(pPhysicalEntity);

	// Set gravity params
	if (m_bCustomGravity)
	{
		pe_params_area areaParams;
		areaParams.gravity = m_gravity;
		pPhysicalEntity->SetParams(&areaParams);
	}

	// Set air / water params
	if (m_buoyancyParameters.medium != SBuoyancyParameters::EType::None)
	{
		pe_params_buoyancy buoyancyParams;
		buoyancyParams.iMedium = (pe_params_buoyancy::EMediumType)m_buoyancyParameters.medium;
		buoyancyParams.waterFlow = m_buoyancyParameters.flow;
		buoyancyParams.flowVariance = 0.f;
		buoyancyParams.waterDensity = m_buoyancyParameters.density;
		buoyancyParams.waterResistance = m_buoyancyParameters.resistance;
		buoyancyParams.waterPlane.n = Vec3(0, 0, -1);
		buoyancyParams.waterPlane.origin = Vec3(0, 0, gEnv->p3DEngine->GetWaterLevel());
		pPhysicalEntity->SetParams(&buoyancyParams);
	}
}

void CAreaComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
		Physicalize();
	}
}

uint64 CAreaComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
}

#ifndef RELEASE
void CAreaComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const
{
	if (context.bSelected)
	{
		IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity();
		if (pPhysicalEntity == nullptr)
		{
			return;
		}

		pe_status_pos pos;
		if (pPhysicalEntity->GetStatus(&pos) == 0)
		{
			return;
		}

		Vec3 samples[8][8][8];
		QuatT transform(pos.pos, pos.q);
		AABB bbox = AABB(pos.BBox[0], pos.BBox[1]);
		float frameTime = gEnv->pTimer->GetCurrTime();
		float theta = frameTime - floor(frameTime);

		float len[3] =
		{
			fabs_tpl(bbox.min.x - bbox.max.x),
			fabs_tpl(bbox.min.y - bbox.max.y),
			fabs_tpl(bbox.min.z - bbox.max.z)
		};

		pe_status_area area;
		for (size_t i = 0; i < 8; ++i)
		{
			for (size_t j = 0; j < 8; ++j)
			{
				for (size_t k = 0; k < 8; ++k)
				{
					area.ctr = transform * Vec3(
						bbox.min.x + i * (len[0] / 8.f)
						, bbox.min.y + j * (len[1] / 8.f)
						, bbox.min.z + k * (len[2] / 8.f));
					samples[i][j][k] = (pPhysicalEntity->GetStatus(&area)) ? area.pb.waterFlow : Vec3(0, 0, 0);
				}
			}
		}

		for (size_t i = 0; i < 8; ++i)
		{
			for (size_t j = 0; j < 8; ++j)
			{
				for (size_t k = 0; k < 8; ++k)
				{
					Vec3 ctr = transform * Vec3(
						bbox.min.x + i * (len[0] / 8.f),
						bbox.min.y + j * (len[1] / 8.f),
						bbox.min.z + k * (len[2] / 8.f));
					Vec3 dir = samples[i][j][k].GetNormalized();

					gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(ctr, dir, 0.03f, 0.03f, context.debugDrawInfo.color);
				}
			}
		}
	}
}
#endif
}
}
