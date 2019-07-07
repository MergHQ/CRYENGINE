// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Util/BoostPythonHelpers.h"
#include "EntityObject.h"

class CUndoEntityParam : public IUndoObject
{
public:
	CUndoEntityParam(const char* pObjectName, const char* pVarPath, const char* pUndoDescription = "Set Entity Param");

protected:
	const char* GetDescription();
	void        Undo(bool bUndo);
	void        Redo();

private:
	string             m_entityName;
	string             m_paramName;
	SPyWrappedProperty m_undo;
	SPyWrappedProperty m_redo;
	const char*        m_undoDescription;
};
