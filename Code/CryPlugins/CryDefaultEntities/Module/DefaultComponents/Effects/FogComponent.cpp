#include "StdAfx.h"
#include "FogComponent.h" 

static void ReflectType(Schematyc::CTypeDesc<IFogVolumeRenderNode::eFogVolumeType>& desc)
{
	desc.SetGUID("{2402BB2D-F5D3-47E6-9EDE-D83CC05DC2BF}"_cry_guid);
	desc.SetLabel("Fog Volume Type");
	desc.SetDescription("Type of fog volume");
	desc.SetDefaultValue(IFogVolumeRenderNode::eFogVolumeType_Ellipsoid);
	desc.AddConstant(IFogVolumeRenderNode::eFogVolumeType_Ellipsoid, "Ellipsoid", "Ellipsoid");
	desc.AddConstant(IFogVolumeRenderNode::eFogVolumeType_Box, "Box", "Box");
}

namespace Cry
{
	namespace DefaultComponents
	{
		static void RegisterFogComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CFogComponent));
			}
		}

		CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterFogComponent)

		void CFogComponent::ReflectType(Schematyc::CTypeDesc<CFogComponent>& desc)
		{
			desc.SetGUID(CFogComponent::IID());
			desc.SetEditorCategory("Effects");
			desc.SetLabel("Fog Volume");
			desc.SetDescription("Renders a fog effect");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CFogComponent::m_bActive, 'actv', "Active", "Active", "Whether or not the fog volume is currently active", true);
			desc.AddMember(&CFogComponent::m_type, 'type', "Type", "Type", "Type of shape to use for rendering the fog volume", IFogVolumeRenderNode::eFogVolumeType_Ellipsoid);
			desc.AddMember(&CFogComponent::m_size, 'size', "Size", "Size", "Size of the fog volume", Vec3(1.f));
			desc.AddMember(&CFogComponent::m_color, 'col', "Color", "Color", "Color of the fog volume", Vec3(1.f));
			desc.AddMember(&CFogComponent::m_emission, 'emmi', "Emission", "Emission", nullptr, Vec3(0.f));
			desc.AddMember(&CFogComponent::m_emissionIntensity, 'emin', "EmissionIntensity", "Emission Intensity", nullptr, 1.f);

			desc.AddMember(&CFogComponent::m_options, 'opti', "Options", "Options", nullptr, CFogComponent::SOptions());
		}

		void ReflectType(Schematyc::CTypeDesc<CFogComponent::SOptions>& desc)
		{
			desc.SetGUID("{7EFB4456-3395-42EE-BE30-5AB565EC7707}"_cry_guid);
			desc.AddMember(&CFogComponent::SOptions::m_bUseGlobalFogColor, 'glob', "UseGlobalFogColor", "Use Global Fog Color", "Whether or not to use the global fog color for this volume", false);
			desc.AddMember(&CFogComponent::SOptions::m_bIgnoreVisAreas, 'igvi', "IgnoreVisAreas", "Ignore VisAreas", nullptr, false);
			desc.AddMember(&CFogComponent::SOptions::m_bAffectsOnlyThisArea, 'area', "OnlyAffectThisArea", "Only Affect This Area", nullptr, false);

			desc.AddMember(&CFogComponent::SOptions::m_globalDensity, 'glde', "GlobalDensity", "Global Density", nullptr, 1.f);
			desc.AddMember(&CFogComponent::SOptions::m_densityOffset, 'deno', "DensityOffset", "Density Offset", nullptr, 0.f);

			//desc.AddMember(&CFogComponent::SOptions::m_nearCutoff, 'necu', "NearCutoff", "Near Cutoff", nullptr, 0.f);
			//desc.AddMember(&CFogComponent::SOptions::m_softEdges, 'soed', "SoftEdges", "Soft Edges", nullptr, 1.f);
			desc.AddMember(&CFogComponent::SOptions::m_heightFallOffDirLong, 'helo', "HeightFalloffDirLong", "Height Falloff Longitude", nullptr, 0.f);
			desc.AddMember(&CFogComponent::SOptions::m_heightFallOffDirLati, 'hela', "HeightFalloffDirLati", "Height Falloff Latitude", nullptr, 0.f);
			desc.AddMember(&CFogComponent::SOptions::m_heightFallOffShift, 'hesh', "HeightFalloffShift", "Height Falloff Shift", nullptr, 0.f);
			desc.AddMember(&CFogComponent::SOptions::m_heightFallOffScale, 'hesc', "HeightFalloffScale", "Height Falloff Scale", nullptr, 1.f);
			//desc.AddMember(&CFogComponent::SOptions::m_rampStart, 'rast', "RampStart", "Ramp Start", nullptr, 0.f);
			//desc.AddMember(&CFogComponent::SOptions::m_rampEnd, 'raen', "RampEnd", "Ramp End", nullptr, 50.f);
			//desc.AddMember(&CFogComponent::SOptions::m_rampInfluence, 'rain', "RampInfluence", "Ramp Influence", nullptr, 0.f);
			//desc.AddMember(&CFogComponent::SOptions::m_windInfluence, 'wiin', "WindInfluence", "Wind Influence", nullptr, 1.f);
			//desc.AddMember(&CFogComponent::SOptions::m_densityNoiseScale, 'dens', "DensityNoiseScale", "Density Noise Scale", nullptr, 0.f);
			//desc.AddMember(&CFogComponent::SOptions::m_densityNoiseOffset, 'deof', "DensityNoiseOffset", "Density Noise Offset", nullptr, 0.f);
			//desc.AddMember(&CFogComponent::SOptions::m_densityNoiseTimeFrequency, 'denf', "DensityNoiseTimeFrequency", "Density Noise Time Frequency", nullptr, 0.f);
			//desc.AddMember(&CFogComponent::SOptions::m_densityNoiseFrequency, 'defn', "DensityNoiseFrequence", "Density Noise Frequenc", nullptr, Vec3(1.f));
		}

		void CFogComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			Initialize();
		}
	}
}