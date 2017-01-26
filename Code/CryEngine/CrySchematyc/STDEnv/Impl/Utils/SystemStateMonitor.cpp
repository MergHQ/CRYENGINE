// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemStateMonitor.h"

namespace Schematyc
{
CSystemStateMonitor::CSystemStateMonitor()
	: m_bLoadingLevel(false)
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "CSystemStateMonitor");
}

CSystemStateMonitor::~CSystemStateMonitor()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

void CSystemStateMonitor::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		{
			m_bLoadingLevel = true;
			break;
		}
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		{
			m_bLoadingLevel = false;
		}
		break;
	}
}

bool CSystemStateMonitor::LoadingLevel() const
{
	return m_bLoadingLevel;
}
} // Schematyc
