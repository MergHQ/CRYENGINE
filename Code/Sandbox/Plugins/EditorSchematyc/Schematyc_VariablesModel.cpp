// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_VariablesModel.h"

namespace CrySchematycEditor {

uint32 CAbstractVariablesModelItem::GetIndex() const
{
	return m_model.GetVariableItemIndex(*this);
}

}
