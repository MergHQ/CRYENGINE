// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id: D3DLightPropagationVolume.h,v 1.0 2008/05/19 12:14:13 AntonKaplanyan Exp wwwrun $
   $DateTime$
   Description:  Routine for rendering and managing of Light Propagation Volumes
   -------------------------------------------------------------------------
   History:
   - 19:5:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#ifndef __D3D_LIGHT_PROPAGATION_VOLUME_H__
#define __D3D_LIGHT_PROPAGATION_VOLUME_H__

#include <CryRenderer/IShader.h>

// the main class that manages multiple 3D grids
class CLightPropagationVolumesManager
{
	typedef std::set<CRELightPropagationVolume*> LPVSet;
	friend class CRELightPropagationVolume;
public:
	// construct/destruct object
	CLightPropagationVolumesManager();
	~CLightPropagationVolumesManager();

	// registration framework
	void RegisterLPV(CRELightPropagationVolume* p);
	void UnregisterLPV(CRELightPropagationVolume* p);

	// heavy method, don't call it too often
	bool IsRenderable();

	// apply GI grid to pAccRT. Note: the Render() method doesn't process it
	void RenderGI();

	// evaluate all the grids
	void Render();

	// singleton instance access point
	inline static CLightPropagationVolumesManager& Instance() { if (!s_pInstance) s_pInstance = new CLightPropagationVolumesManager(); return *s_pInstance; }

	void                                           Shutdown() { SAFE_DELETE(s_pInstance); }

	void                                           UpdateReflectiveShadowmapSize(SReflectiveShadowMap& rRSM, int nWidth, int nHeight);

	CRELightPropagationVolume*                     GetVolume(int index)
	{
		if (index < (int)m_grids.size())
		{
			LPVSet::iterator i = m_grids.begin();
			std::advance(i, index);
			return *i;
		}
		return NULL;
	}

	CRELightPropagationVolume* GetGIVolumeByLight(ILightSource* pLighSource)
	{
		LPVSet::const_iterator itEnd = m_setGIVolumes.end();
		for (LPVSet::iterator it = m_setGIVolumes.begin(); it != itEnd; ++it)
			if ((*it)->GetAttachedLightSource() == pLighSource)
				return *it;
		return NULL;
	}
	CRELightPropagationVolume* GetCurrentGIVolume()
	{
		if (m_pCurrentGIVolume)
			return m_pCurrentGIVolume;
		if (!m_setGIVolumes.empty())
			return *m_setGIVolumes.begin();
		return NULL;
	}
	void SetGIVolumes(CRELightPropagationVolume* p)
	{
		for (int i = 0; i < 3; ++i)
		{
			assert(m_pTempVolRTs[i] == NULL);
			m_pTempVolRTs[i] = CTexture::s_ptexLPV_RTs[i];
			CTexture::s_ptexLPV_RTs[i] = (CTexture*)p->m_pVolumeTextures[i];
		}
	}
	void UnsetGIVolumes()
	{
		for (int i = 0; i < 3; ++i)
		{
			assert(m_pTempVolRTs[i] != NULL);
			CTexture::s_ptexLPV_RTs[i] = m_pTempVolRTs[i];
			m_pTempVolRTs[i] = NULL;
		}
	}
	bool IsGIRenderable() const { return !m_setGIVolumes.empty(); }

	// is LPVs enabled
	bool IsEnabled() const;

	SReflectiveShadowMap m_RSM;

	// destroy all resources
	void Cleanup();
protected:

	// enable/disable VIS
	void Toggle(const bool enable);

protected:
	// names for shaders
	CCryNameR     m_semLightPositionSemantic;
	CCryNameR     m_semLightColorSemantic;
	CCryNameR     m_semCameraMatrix;
	CCryNameR     m_semCameraFrustrumLB;
	CCryNameR     m_semCameraFrustrumLT;
	CCryNameR     m_semCameraFrustrumRB;
	CCryNameR     m_semCameraFrustrumRT;
	CCryNameR     m_semVisAreaParams;
	CCryNameTSCRC m_techPropagateTechName;
	CCryNameTSCRC m_techCollectTechName;
	CCryNameTSCRC m_techApplyTechName;
	CCryNameTSCRC m_techInjectRSM;
	CCryNameTSCRC m_techPostinjectLight;

	// helper for shared ping-pong RTs
	class PingPongRTs
	{
	public:
		PingPongRTs() : m_nCurrent(0)
		{
			m_pRT[0][0] = NULL;
			m_pRT[0][1] = NULL;
			m_pRT[0][2] = NULL;
			m_pRT[1][0] = NULL;
			m_pRT[1][1] = NULL;
			m_pRT[1][2] = NULL;
		}
		~PingPongRTs()
		{
			SetRTs(NULL, NULL);
		}
		void              SetRTs(CTexture* pRT0[3], CTexture* pRT1[3]);
		inline CTexture** GetTarget() { return m_pRT[m_nCurrent]; }
		void              Swap()      { m_nCurrent = (m_nCurrent + 1) % 2; }
	protected:

		CTexture* m_pRT[2][3];
		int       m_nCurrent;
	};

	PingPongRTs m_poolRT;                 // shared ping-pong RTs

	// Cached render targets
	class CCachedRTs
	{
	public:
		CCachedRTs()
		{
			m_pRT[0] = NULL;
			m_pRT[1] = NULL;
			m_pRT[2] = NULL;
			m_bIsAcuired = false;
		}
		CTexture*  GetRT(const uint32 i) { assert(i < 3); return m_pRT[i]; }
		const bool IsAcquired() const    { return m_bIsAcuired; }
		void       Acquire()             { assert(!m_bIsAcuired); m_bIsAcuired = true; }
		void       Release()             { assert(m_bIsAcuired); m_bIsAcuired = false; }
		void       SetCachedRTs(CTexture* pRT0, CTexture* pRT1, CTexture* pRT2)
		{
			assert(!m_bIsAcuired);
			assert(m_pRT[0] == NULL);
			m_pRT[0] = pRT0;
			if (pRT0) pRT0->AddRef();
			m_pRT[1] = pRT1;
			if (pRT1) pRT1->AddRef();
			m_pRT[2] = pRT2;
			if (pRT2) pRT2->AddRef();
			m_bIsAcuired = false;
		}
		void Cleanup()
		{
			SAFE_RELEASE(m_pRT[0]);
			SAFE_RELEASE(m_pRT[1]);
			SAFE_RELEASE(m_pRT[2]);
			m_bIsAcuired = false;
		}

		~CCachedRTs()
		{
			Cleanup();
		}
	protected:
		CTexture* m_pRT[3];
		bool      m_bIsAcuired;
	}           m_CachedRTs;

	CCachedRTs& GetCachedRTs() { return m_CachedRTs; }

	// is LPVs enabled
	bool                  m_enabled;

	D3DVertexDeclaration* m_VTFVertexDeclaration;

	// array of grids
	LPVSet m_grids;
	LPVSet m_setGIVolumes;

	// currently rendered LPV
	CRELightPropagationVolume* m_pCurrentGIVolume;
	CTexture*                  m_pTempVolRTs[3];            // temporary pointers for original s_ptexIrrVolumeRT

	// Vertex buffers
	CVertexBuffer* m_pPropagateVB;                          // VB for light propagation
	CVertexBuffer* m_pPostinjectVB;                         // VB for post-injection
	CVertexBuffer* m_pApplyVB;                              // VB for deferred apply(unit cube)
	CVertexBuffer* m_pRSMInjPointsVB;                       // VB for injection of colored shadow-maps

	volatile int   m_nNextFreeId;                 // id for Light Propagation Volumes

	// singleton instance
	static CLightPropagationVolumesManager* s_pInstance;
};

#define LPVManager (CLightPropagationVolumesManager::Instance())

#endif  // __D3D_LIGHT_PROPAGATION_VOLUME_H__
