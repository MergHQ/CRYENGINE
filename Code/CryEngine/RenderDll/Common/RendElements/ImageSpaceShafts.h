// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "OpticsElement.h"

class CTexture;
class CD3D9Renderer;

class ImageSpaceShafts : public COpticsElement
{
protected:
	static CTexture* m_pOccBuffer;
	static CTexture* m_pDraftBuffer;

protected:
	bool                 m_bHighQualityMode;
	bool                 m_bTexDirty;
	_smart_ptr<CTexture> m_pGoboTex;

	virtual void InitTextures();

public:

	ImageSpaceShafts(const char* name) :
		COpticsElement(name),
		m_bTexDirty(true),
		m_bHighQualityMode(false)
	{
		m_Color.a = 1.f;
		SetSize(0.7f);
	}

	EFlareType GetType() { return eFT_ImageSpaceShafts; }

	void       InitEditorParamGroups(DynArray<FuncVariableGroup>& groups);
	void       Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
	void       Load(IXmlNode* pNode);

	bool       IsHighQualityMode() const  { return m_bHighQualityMode; }
	void       SetHighQualityMode(bool b) { m_bHighQualityMode = b; m_bTexDirty = true; }

	CTexture*  GetGoboTex() const         { return m_pGoboTex; }
	void       SetGoboTex(CTexture* tex)  { m_pGoboTex = tex; }
};
