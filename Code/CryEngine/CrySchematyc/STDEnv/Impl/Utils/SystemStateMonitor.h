// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ISystem.h>

namespace Schematyc
{
class CSystemStateMonitor : public ISystemEventListener
{
public:

	CSystemStateMonitor();

	~CSystemStateMonitor();

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
	// ~ISystemEventListener

	bool LoadingLevel() const;

private:

	bool m_bLoadingLevel;
};
} // Schematyc
