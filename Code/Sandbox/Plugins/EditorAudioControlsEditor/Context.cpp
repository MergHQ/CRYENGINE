// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Context.h"

#include "Common.h"
#include "ContextManager.h"

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
void CContext::SetName(string const& name)
{
	m_name = name;
	m_id = g_contextManager.GenerateContextId(name);
}
} // namespace ACE
