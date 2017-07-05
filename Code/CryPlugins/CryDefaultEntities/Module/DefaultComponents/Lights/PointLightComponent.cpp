#include "StdAfx.h"
#include "PointLightComponent.h"

#include <CrySystem/IProjectManager.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>
#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
	namespace DefaultComponents
	{
		void CPointLightComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
		}

		void CPointLightComponent::ReflectType(Schematyc::CTypeDesc<CPointLightComponent>& desc)
		{
			desc.SetGUID(CPointLightComponent::IID());
			desc.SetEditorCategory("Lights");
			desc.SetLabel("Point Light");
			desc.SetDescription("Emits light from its origin into all directions");
			desc.SetIcon("icons:ObjectTypes/light.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly });

			desc.AddMember(&CPointLightComponent::m_bActive, 'actv', "Active", "Active", "Determines whether the light is enabled", true);
			desc.AddMember(&CPointLightComponent::m_radius, 'radi', "Radius", "Radius", "Determines whether the range of the point light", 10.f);

			desc.AddMember(&CPointLightComponent::m_color, 'colo', "Color", "Color", "Color emission information", CPointLightComponent::SColor());
			desc.AddMember(&CPointLightComponent::m_shadows, 'shad', "Shadows", "Shadows", "Shadow casting settings", CPointLightComponent::SShadows());
			desc.AddMember(&CPointLightComponent::m_options, 'opt', "Options", "Options", "Specific Light Options", CPointLightComponent::SOptions());
			desc.AddMember(&CPointLightComponent::m_animations, 'anim', "Animations", "Animations", "Light style / animation properties", CPointLightComponent::SAnimations());
		}

		void CPointLightComponent::Initialize()
		{
			if (!m_bActive)
			{
				FreeEntitySlot();

				return;
			}

			CDLight light;

			light.m_nLightStyle = m_animations.m_style;
			light.SetAnimSpeed(m_animations.m_speed);

			light.SetPosition(ZERO);
			light.m_Flags = DLF_DEFERRED_LIGHT | DLF_POINT;

			light.m_fRadius = m_radius;

			light.SetLightColor(m_color.m_color * m_color.m_diffuseMultiplier);
			light.SetSpecularMult(m_color.m_specularMultiplier);

			light.m_fHDRDynamic = 0.f;

			if (m_options.m_bAffectsOnlyThisArea)
				light.m_Flags |= DLF_THIS_AREA_ONLY;

			if (m_options.m_bIgnoreVisAreas)
				light.m_Flags |= DLF_IGNORES_VISAREAS;

			if (m_options.m_bVolumetricFogOnly)
				light.m_Flags |= DLF_VOLUMETRIC_FOG_ONLY;

			if (m_options.m_bAffectsVolumetricFog)
				light.m_Flags |= DLF_VOLUMETRIC_FOG;

			if (m_options.m_bAmbient)
				light.m_Flags |= DLF_AMBIENT;

			if (m_shadows.m_castShadowSpec != EMiniumSystemSpec::Disabled && (int)gEnv->pSystem->GetConfigSpec() >= (int)m_shadows.m_castShadowSpec)
			{
				light.m_Flags |= DLF_CASTSHADOW_MAPS;

				light.SetShadowBiasParams(1.f, 1.f);
				light.m_fShadowUpdateMinRadius = light.m_fRadius;

				float shadowUpdateRatio = 1.f;
				light.m_nShadowUpdateRatio = max((uint16)1, (uint16)(shadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));
			}
			else
				light.m_Flags &= ~DLF_CASTSHADOW_MAPS;

			light.m_fAttenuationBulbSize = m_options.m_attenuationBulbSize;

			light.m_fFogRadialLobe = m_options.m_fogRadialLobe;

			// Load the light source into the entity
			m_pEntity->LoadLight(GetOrMakeEntitySlotId(), &light);

			uint32 slotFlags = m_pEntity->GetSlotFlags(GetEntitySlotId());
			UpdateGIModeEntitySlotFlags((uint8)m_options.m_giMode, slotFlags);
			m_pEntity->SetSlotFlags(GetEntitySlotId(), slotFlags);
		}

		void CPointLightComponent::ProcessEvent(SEntityEvent& event)
		{
			if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
			{
				Initialize();
			}
		}

		uint64 CPointLightComponent::GetEventMask() const
		{
			return BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
		}

#ifndef RELEASE
		void CPointLightComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const
		{
			if (context.bSelected)
			{
				Matrix34 slotTransform = GetWorldTransformMatrix();

				Vec3 p0, p1;
				float step = 10.0f / 180 * gf_PI;
				float angle;

				Vec3 pos = slotTransform.GetTranslation();

				// Z Axis
				p0 = pos;
				p1 = pos;
				p0.x += m_radius * sin(0.0f);
				p0.y += m_radius * cos(0.0f);
				for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
				{
					p1.x = pos.x + m_radius * sin(angle);
					p1.y = pos.y + m_radius * cos(angle);
					p1.z = pos.z;
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p0, context.debugDrawInfo.color, p1, context.debugDrawInfo.color);
					p0 = p1;
				}

				// X Axis
				p0 = pos;
				p1 = pos;
				p0.y += m_radius * sin(0.0f);
				p0.z += m_radius * cos(0.0f);
				for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
				{
					p1.x = pos.x;
					p1.y = pos.y + m_radius * sin(angle);
					p1.z = pos.z + m_radius * cos(angle);
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p0, context.debugDrawInfo.color, p1, context.debugDrawInfo.color);
					p0 = p1;
				}

				// Y Axis
				p0 = pos;
				p1 = pos;
				p0.x += m_radius * sin(0.0f);
				p0.z += m_radius * cos(0.0f);
				for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
				{
					p1.x = pos.x + m_radius * sin(angle);
					p1.y = pos.y;
					p1.z = pos.z + m_radius * cos(angle);
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p0, context.debugDrawInfo.color, p1, context.debugDrawInfo.color);
					p0 = p1;
				}
			}
		}
#endif

		void CPointLightComponent::Enable(bool bEnable)
		{
			m_bActive = bEnable; 

			Initialize();
		}
	}
}