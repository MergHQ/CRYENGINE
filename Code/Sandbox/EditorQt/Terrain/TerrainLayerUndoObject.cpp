// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TerrainLayerUndoObject.h"
#include "IEditorImpl.h"

#include "Terrain/TerrainManager.h"

CTerrainLayersPropsUndoObject::CTerrainLayersPropsUndoObject()
{
	m_undo.root = GetISystem()->CreateXmlNode("Undo");
	m_redo.root = GetISystem()->CreateXmlNode("Redo");
	m_undo.bLoading = false;
	GetIEditorImpl()->GetTerrainManager()->SerializeLayerSettings(m_undo);
}

const char* CTerrainLayersPropsUndoObject::GetDescription()
{ 
	return "Terrain Layers Properties"; 
}

void CTerrainLayersPropsUndoObject::Undo(bool bUndo)
{
	if (bUndo)
	{
		m_redo.bLoading = false; // save redo
		GetIEditorImpl()->GetTerrainManager()->SerializeLayerSettings(m_redo);
	}
	m_undo.bLoading = true; // load undo
	GetIEditorImpl()->GetTerrainManager()->SerializeLayerSettings(m_undo);

	GetIEditorImpl()->Notify(eNotify_OnInvalidateControls);
}

void CTerrainLayersPropsUndoObject::Redo()
{
	m_redo.bLoading = true; // load redo
	GetIEditorImpl()->GetTerrainManager()->SerializeLayerSettings(m_redo);

	GetIEditorImpl()->Notify(eNotify_OnInvalidateControls);
}
