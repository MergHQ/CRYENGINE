// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Schematyc
{
class CEntityObjectDebugger
{
public:

	CEntityObjectDebugger();
	~CEntityObjectDebugger();

private:

	void Update(const SUpdateContext&);

private:

	CConnectionScope m_connectionScope;
};
} // Schematyc
