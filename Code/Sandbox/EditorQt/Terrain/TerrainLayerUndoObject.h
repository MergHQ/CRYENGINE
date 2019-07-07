// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IUndoObject.h>
#include <Util/XmlArchive.h>

// Stores only properties of all terrain layers without textures
class CTerrainLayersPropsUndoObject : public IUndoObject
{
public:
	CTerrainLayersPropsUndoObject();

private:
	virtual const char* GetDescription();
	virtual void        Undo(bool bUndo);
	virtual void        Redo();

	CXmlArchive m_undo;
	CXmlArchive m_redo;
};
