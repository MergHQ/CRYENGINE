// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id: GlobalIllumination.h,v 1.0 2008/08/4 12:14:13 AntonKaplanyan Exp wwwrun $
   $DateTime$
   Description:  Routine for rendering and managing of light propagation volumes
              and its cascades for global illumination
   -------------------------------------------------------------------------
   History:
   - 4:8:2008   12:14 : Created by Anton Kaplanyan
   - 26:1:2011  11:18 : Refactored by Anton Kaplanyan
*************************************************************************/

#ifndef _GLOBALILLUMINATION_RENDERNODE_
#define _GLOBALILLUMINATION_RENDERNODE_

#pragma once

#include <CryInput/IInput.h>

class CLPVRenderNode;

class CLPVCascade : public CMultiThreadRefCount
{
protected:
	struct CLightEntity* m_pReflectiveShadowMap;    // Light entity for Reflective Shadow Map rendering from Sun
	Vec3                 m_vcGridPos;               // Center of the cascade
	float                m_fDistance;               // Maximum effective distance of the cascade
	float                m_fOffsetDistance;         // Offset (in percents, from 0 to 1) for how much the center should be shifted towards the front of the camera
	bool                 m_bIsActive;               // Activity flag
	bool                 m_bGenerateRSM;            // Indicates to refresh RSM
public:
	CLPVRenderNode*      m_pLPV;                    // The LPV used for Global Illumination propagation
public:
	CLPVCascade();
	~CLPVCascade();
	void UpdateLPV(const CCamera& camera, const bool bActive);
	void UpdateRSMFrustrum(const CCamera& camera, const bool bActive);
	void GeneralUpdate(const bool bActive);
	void PostUpdate(const bool bActive, const bool bGenerateRSM, const CLPVCascade* pNestedVolume);
	void Init(float fRadius, float fOffset, const CCamera& rCamera);
	void OffsetPosition(const Vec3& delta);
};
typedef _smart_ptr<CLPVCascade> CLPVCascadePtr;

class CGlobalIlluminationManager
{
public:
	CGlobalIlluminationManager();
	~CGlobalIlluminationManager();

	bool IsEnabled() const { return m_bEnabled; }
	void UpdatePosition(const SRenderingPassInfo& passInfo);
	void Cleanup();
	void OffsetPosition(const Vec3& delta);
private:
	void Update(const CCamera& rCamera);
	std::vector<CLPVCascadePtr> m_Cascades;
	float                       m_fMaxDistance;           // Global maximum effective distance of the GI
	float                       m_fCascadesRatio;         // The size ratio between two nested cascades (e.g. 2.0 means 50, 25, 12.5, 6.25,... etc. meters for nested cascades)
	float                       m_fOffset;                // Offset (in percents, from 0 to 1) for how much the center should be shifted towards the front of the camera
	bool                        m_GlossyReflections;      // Should it simulate glossy GI?
	bool                        m_SecondaryOcclusion;     // Should it simulate indirect shadows?
	bool                        m_bEnabled;
	int                         m_nUpdateFrameId;         // Last frame it was updated (use for GI caching)
};

#endif // #ifndef _GLOBALILLUMINATION_RENDERNODE_
