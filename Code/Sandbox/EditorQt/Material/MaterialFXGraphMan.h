// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "LevelIndependentFileMan.h"
#include "HyperGraph/HyperGraph.h"

class CMaterialFXGraphMan : public ILevelIndependentFileModule
{
public:
	CMaterialFXGraphMan();
	~CMaterialFXGraphMan();

	void Init();
	void ReloadFXGraphs();

	void ClearEditorGraphs();
	void SaveChangedGraphs();
	bool HasModifications();

	bool NewMaterialFx(CString& filename, CHyperGraph** pHyperGraph = NULL);

	//ILevelIndependentFileModule
	virtual bool PromptChanges();
	//~ILevelIndependentFileModule

private:
	typedef std::list<IFlowGraphPtr> TGraphList;
	TGraphList m_matFxGraphs;
};
