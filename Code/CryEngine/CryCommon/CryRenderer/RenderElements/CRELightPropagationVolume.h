// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _CRELIGHT_PROPAGATION_VOLUME_
#define _CRELIGHT_PROPAGATION_VOLUME_

#pragma once

#include <CryRenderer/VertexFormats.h>

struct IParticleEmitter;
struct ILightSource;

struct SReflectiveShadowMap
{
	Matrix44A mxLightViewProj;
	ITexture* pDepthRT;
	ITexture* pNormalsRT;
	ITexture* pFluxRT;
	SReflectiveShadowMap()
	{
		mxLightViewProj.SetIdentity();
		pDepthRT = NULL;
		pNormalsRT = NULL;
		pFluxRT = NULL;
	}
	void Release();
};

class CRELightPropagationVolume : public CRendElementBase
{
protected:
	friend struct ILPVRenderNode;
	friend class CLPVRenderNode;
	friend class CLPVCascade;
	friend class CLightPropagationVolumesManager;
private:
	// Internal methods

	//! Injects single texture or colored shadow map with vertex texture fetching.
	void _injectWithVTF(SReflectiveShadowMap& rCSM);

	//! Injects occlusion from camera depth buffer.
	void _injectOcclusionFromCamera();

protected:
	//! Injects occlusion information from depth buffer.
	void InjectOcclusionFromRSM(SReflectiveShadowMap& rCSM, bool bCamera);

	//! Propagates radiance.
	void Propagate();

	//! Post-injection phase (injection after propagation).
	bool Postinject();

	//! Post-injects single ready light.
	void PostnjectLight(const CDLight& rLight);

	//! Optimizations for faster deferred rendering.
	void ResolveToVolumeTexture();
public:

	//! \return LPV id (might be the same as light id for GI volumes).
	int           GetId() const                                 { return m_nId; }

	ILightSource* GetAttachedLightSource()                      { return m_pAttachedLightSource; }
	void          AttachLightSource(ILightSource* pLightSource) { m_pAttachedLightSource = pLightSource; }

	CRELightPropagationVolume();
	virtual ~CRELightPropagationVolume();

	//! Flags for LPV render element.
	enum EFlags
	{
		efGIVolume     = 1ul << 0,
		efHasOcclusion = 1ul << 1,
	};

	//! Define the global maximum possible grid size for LPVs (should be GPU-friendly and <= 64 - h/w limitation on old platforms).
	enum { nMaxGridSize = 32u };

	enum EStaticProperties
	{
		espMaxInjectRSMSize = 384ul,
	};

	//! LPV render settings.
	struct Settings
	{
		Vec3      m_pos;
		AABB      m_bbox;               //!< Bounding box.
		Matrix44A m_mat;
		Matrix44A m_matInv;
		int       m_nGridWidth;         //!< Grid dimensions in texels.
		int       m_nGridHeight;        //!< Grid dimensions in texels.
		int       m_nGridDepth;         //!< Grid dimensions in texels.
		int       m_nNumIterations;     //!< Number of iterations to propagate.
		Vec4      m_gridDimensions;     //!< Grid size.
		Vec4      m_invGridDimensions;  //!< 1.f / (grid size).
		Settings();
	};

	//! Render reflective shadow map to render.
	void InjectReflectiveShadowMap(SReflectiveShadowMap& rCSM);

	//! Query point light source to render.
	virtual void InsertLight(const CDLight& light);

	//! Propagate radiance.
	void Evaluate();

	//! Apply radiance to accumulation RT.
	void DeferredApply();

	//! Pass only CRELightPropagationVolume::EFlags enum flags combination here.
	void SetNewFlags(uint32 nFlags) { m_nGridFlags = nFlags; }

	//! \return CRELightPropagationVolume::EFlags enum flags.
	uint32       GetFlags() const { return m_nGridFlags; }

	virtual void mfPrepare(bool bCheckOverflow);
	virtual bool mfPreDraw(SShaderPass* sl);
	virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

	void         Invalidate()               { m_bIsUpToDate = false; }

	ITexture*    GetLPVTexture(int iTex)    { assert(iTex < 3); return m_pVolumeTextures[iTex]; }
	float        GetVisibleDistance() const { return m_fDistance; }
	float        GetIntensity() const       { return m_fAmount; }

	//! \return true if it is applicable to render in the deferred pass, false otherwise.
	inline bool       IsRenderable() const { return m_bIsRenderable; }

	virtual Settings* GetFillSettings();

	//! This function operates on both fill/render settings.
	//! Do NOT call it unless right after creating the CRELightPropagationVolume instance,
	//! at which point no other threads can access it.
	virtual void DuplicateFillSettingToOtherSettings();

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddContainer(m_lightsToPostinject[0][0]);
		pSizer->AddContainer(m_lightsToPostinject[0][1]);
		pSizer->AddContainer(m_lightsToPostinject[1][0]);
		pSizer->AddContainer(m_lightsToPostinject[1][1]);
	}
	const Settings& GetRenderSettings() const;
protected:
	virtual void    UpdateRenderParameters();
	virtual void    EnableSpecular(const bool bEnabled);
	void            Cleanup();

protected:
	Settings m_Settings[RT_COMMAND_BUF_COUNT];

	int      m_nId;                       //!< unique ID of the volume(need for RTs)
	uint32   m_nGridFlags;                //!< grid flags

	float    m_fDistance;                             //!< max affected distance
	float    m_fAmount;                               //!< affection scaler

	//! The grid itself.
	union
	{
		ITexture* m_pRT[3];
		CTexture* m_pRTex[3];
	};

	//! Volume textures.
	union
	{
		ITexture* m_pVolumeTextures[3];
		CTexture* m_pVolumeTexs[3];
	};

	//! Volume texture for fuzzy occlusion from depth buffers.
	union
	{
		ITexture* m_pOcclusionTexture;
		CTexture* m_pOcclusionTex;
	};

	struct ILPVRenderNode* m_pParent;

	ILightSource*          m_pAttachedLightSource;

	bool                   m_bIsRenderable;           //!< Is the grid not dark?.
	bool                   m_bNeedPropagate;          //!< Does the grid needsto be propagated.
	bool                   m_bNeedClear;              //!< Does the grid need to be cleared after past frame.
	bool                   m_bIsUpToDate;             //!< Invalidates dimensions.
	bool                   m_bHasSpecular;            //!< Enables specular.
	int                    m_nUpdateFrameID;          //!< Last frame updated.

	typedef DynArray<CDLight> Lights;
	Lights m_lightsToPostinject[RT_COMMAND_BUF_COUNT][2];     //!< lights in the grid
};

#endif // #ifndef _CRELIGHT_PROPAGATION_VOLUME_
