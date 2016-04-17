// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "OpticsElement.h"
#include "AbstractMeshElement.h"
#include "MeshUtil.h"

class CTexture;
class IrisShafts : public COpticsElement, public AbstractMeshElement
{
private:
	_smart_ptr<CTexture> m_pBaseTex;
	_smart_ptr<CTexture> m_pSpectrumTex;
	bool                 m_bUseSpectrumTex : 1;

	int                  m_nComplexity;
	int                  m_nSmoothLevel;
	int                  m_nColorComplexity;

	float                m_fPrevOcc;
	float                m_fPrimaryDir;
	float                m_fAngleRange;
	float                m_fConcentrationBoost;
	float                m_fBrightnessBoost;

	float                m_fSizeNoiseStrength;
	float                m_fThicknessNoiseStrength;
	float                m_fSpreadNoiseStrength;
	float                m_fSpacingNoiseStrength;

	float                m_fSpread;
	float                m_fThickness;
	int                  m_nNoiseSeed;

	int                  m_MaxNumberOfPolygon;

protected:
	virtual void GenMesh();
	float        ComputeSpreadParameters(const float thickness);
	int          ComputeDynamicSmoothLevel(int maxLevel, float spanAngle, float threshold);
	void         Invalidate()
	{
		m_meshDirty = true;
	}

public:
	IrisShafts(const char* name) :
		COpticsElement(name, 0.5f),
		m_fThickness(0.3f),
		m_fSpread(0.2f),
		m_nSmoothLevel(2),
		m_nNoiseSeed(81),
		m_pBaseTex(0),
		m_fSizeNoiseStrength(0.8f),
		m_fThicknessNoiseStrength(0.6f),
		m_fSpacingNoiseStrength(0.2f),
		m_fSpreadNoiseStrength(0.0f),
		m_bUseSpectrumTex(false),
		m_fPrimaryDir(0),
		m_fAngleRange(1),
		m_fConcentrationBoost(0),
		m_fPrevOcc(-1.f),
		m_fBrightnessBoost(0),
		m_MaxNumberOfPolygon(0)
	{
		m_vMovement.x = 1.f;
		m_vMovement.y = 1.f;
		m_Color.a = 1.f;

		SetAutoRotation(false);
		SetAspectRatioCorrection(true);
		SetColorComplexity(2);
		SetComplexity(32);

		m_meshDirty = true;
	}

#if defined(FLARES_SUPPORT_EDITING)
	void InitEditorParamGroups(DynArray<FuncVariableGroup>& groups);
#endif

	EFlareType GetType() { return eFT_IrisShafts; }
	void       Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
	void       Load(IXmlNode* pNode);

	bool       GetEnableSpectrumTex() const  { return m_bUseSpectrumTex; }
	void       SetEnableSpectrumTex(bool b)  { m_bUseSpectrumTex = b; }

	CTexture*  GetSpectrumTex() const        { return m_pSpectrumTex; }
	void       SetSpectrumTex(CTexture* tex) { m_pSpectrumTex = tex; }

	CTexture*  GetBaseTex() const            { return m_pBaseTex; }
	void       SetBaseTex(CTexture* tex)     { m_pBaseTex = tex; }

	int        GetNoiseSeed() const          { return m_nNoiseSeed; }
	void       SetNoiseSeed(int seed)
	{
		m_nNoiseSeed = seed;
		m_meshDirty = true;
	}

	int  GetComplexity() const { return m_nComplexity; }
	void SetComplexity(int n)
	{
		m_nComplexity = n;
		m_meshDirty = true;
	}

	int  GetColorComplexity() const { return m_nColorComplexity; }
	void SetColorComplexity(int n)
	{
		m_nColorComplexity = n;
		m_meshDirty = true;
	}

	int  GetSmoothLevel() const { return m_nSmoothLevel; }
	void SetSmoothLevel(int n)
	{
		m_nSmoothLevel = n;
		m_meshDirty = true;
	}

	float GetThickness() const { return m_fThickness; }
	void  SetThickness(float f)
	{
		m_fThickness = f;
		m_meshDirty = true;
	}

	float GetSpread() const { return m_fSpread; }
	void  SetSpread(float s)
	{
		m_fSpread = s;
		m_meshDirty = true;
	}

	float GetThicknessNoise() const { return m_fThicknessNoiseStrength; }
	void  SetThicknessNoise(float noise)
	{
		m_fThicknessNoiseStrength = noise;
		m_meshDirty = true;
	}

	float GetSpreadNoise() const { return m_fSpreadNoiseStrength; }
	void  SetSpreadNoise(float noise)
	{
		m_fThicknessNoiseStrength = noise;
		m_meshDirty = true;
	}

	float GetSizeNoise() const { return m_fSizeNoiseStrength; }
	void  SetSizeNoise(float noise)
	{
		m_fSizeNoiseStrength = noise;
		m_meshDirty = true;
	}

	float GetSpacingNoise() const { return m_fSpacingNoiseStrength; }
	void  SetSpacingNoise(float noise)
	{
		m_fSpacingNoiseStrength = noise;
		m_meshDirty = true;
	}

	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this) + GetMeshDataSize());
	}
};
