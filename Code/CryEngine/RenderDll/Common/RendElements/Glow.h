// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "OpticsElement.h"
#include "AbstractMeshElement.h"
#include "MeshUtil.h"

class Glow : public COpticsElement, public AbstractMeshElement
{
private:
	static float compositionBufRatio;

protected:
	float m_fFocusFactor;
	float m_fPolyonFactor;
	float m_fGamma;

protected:
	void         ApplyDistributionParamsPS(CShader* shader);
	virtual void GenMesh();
	void         DrawMesh();
	void         Invalidate()
	{
		m_meshDirty = true;
	}

public:
	Glow(const char* name) :
		COpticsElement(name),
		m_fFocusFactor(0.3f),
		m_fPolyonFactor(32.f),
		m_fGamma(1)
	{
	}

#if defined(FLARES_SUPPORT_EDITING)
	void InitEditorParamGroups(DynArray<FuncVariableGroup>& groups);
#endif

	EFlareType GetType() { return eFT_Glow; }
	void       Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
	void       Load(IXmlNode* pNode);

	void       SetColor(ColorF t)
	{
		COpticsElement::SetColor(t);
		m_meshDirty = true;
	}
	float GetFocusFactor() const { return m_fFocusFactor; }
	void  SetFocusFactor(float f)
	{
		m_fFocusFactor = f;
	}
	int  GetPolygonFactor() const { return (int)m_fPolyonFactor; }
	void SetPolygonFactor(int f)
	{
		if (f < 0)
			f = 0;
		else if (f > 128)
			f = 128;
		if (m_fPolyonFactor != f)
		{
			m_fPolyonFactor = (float)f;
			m_meshDirty = true;
		}
	}
	float GetGamma() const { return m_fGamma; }
	void  SetGamma(float gamma)
	{
		m_fGamma = (float)gamma;
	}

	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this) + GetMeshDataSize());
	}
};
