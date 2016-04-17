// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleSystem/ParticleFeature.h"
#include "ClipVolumeManager.h"
#include "ParamMod.h"

CRY_PFX2_DBG

volatile bool gFeatureLightSource = false;

namespace pfx2
{

class CFeatureLightSource : public CParticleFeature, public Cry3DEngineBase
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureLightSource()
		: m_intensity(1.0f)
		, m_radius(1.0f)
		, m_affectsThisAreaOnly(false) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->AddToUpdateList(EUL_Render, this);
		pComponent->AddParticleData(EPVF_Position);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_intensity, "Intensity", "Intensity");
		ar(m_radius, "Radius", "Radius");
		ar(m_affectsThisAreaOnly, "AffectsThisAreaOnly", "Affects This Area Only");
	}

	virtual void Render(ICommonParticleComponentRuntime* pCommonComponentRuntime, CParticleComponent* pComponent, IRenderNode* pNode, const SRenderContext& renderContext) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		if (!GetCVars()->e_DynamicLights || renderContext.m_passInfo.IsRecursivePass())
			return;

		CParticleComponentRuntime* pComponentRuntime = pCommonComponentRuntime->GetCpuRuntime();
		if (!pComponentRuntime)
			return;
		auto context = SUpdateContext(pComponentRuntime);
		auto& passInfo = renderContext.m_passInfo;

		const CParticleContainer& container = context.m_container;
		const bool hasColors = container.HasData(EPDT_Color);
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const IColorStream colors = container.GetIColorStream(EPDT_Color);
		const TIStream<uint8> states = container.GetTIStream<uint8>(EPDT_State);

		CDLight light;
		light.m_nStencilRef[0] = m_affectsThisAreaOnly ? renderContext.m_renderParams.nClipVolumeStencilRef : CClipVolumeManager::AffectsEverythingStencilRef;
		light.m_nStencilRef[1] = CClipVolumeManager::InactiveVolumeStencilRef;
		light.m_Flags |= DLF_DEFERRED_LIGHT | DLF_VOLUMETRIC_FOG;

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const uint8 state = states.Load(particleId);
			if (state == ES_Expired)
				continue;

			const Vec3 position = positions.Load(particleId);
			UCol color;
			if (hasColors)
				color = colors.Load(particleId);
			else
				color.dcolor = ~0;
			const float lightIntensity = m_intensity;
			const float lightRadius = m_radius;

			light.SetPosition(position);
			light.m_Color = ToColorF(color) * ColorF(lightIntensity);
			light.m_fRadius = lightRadius;

			const float minColorThreshold = GetFloatCVar(e_ParticlesLightMinColorThreshold);
			const float minRadiusThreshold = GetFloatCVar(e_ParticlesLightMinRadiusThreshold);

			const ColorF& cColor = light.m_Color;
			const float spectralIntensity = cColor.r * 0.299f + cColor.g * 0.587f + cColor.b * 0.114f;
			const Vec4* vLight = (Vec4*)&light.m_Origin.x;
			if (spectralIntensity > minColorThreshold && vLight->w > minRadiusThreshold)
			{
				Get3DEngine()->SetupLightScissors(&light, passInfo);
				light.m_n3DEngineUpdateFrameID = passInfo.GetMainFrameID();
				Get3DEngine()->AddLightToRenderer(light, 1.0f, passInfo);
			}
		}
		CRY_PFX2_FOR_END;
	}

private:
	UFloat10 m_intensity;
	UFloat10 m_radius;
	bool     m_affectsThisAreaOnly;
};

static const ColorB lightColor = ColorB(230, 216, 0);
CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLightSource, "Light", "Light", defaultIcon, lightColor);

}
