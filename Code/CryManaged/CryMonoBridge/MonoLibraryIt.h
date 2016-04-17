// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoRuntime.h>
#include "MonoRuntime.h"

class CMonoLibraryIt : public IMonoLibraryIt
{
public:
	CMonoLibraryIt(CMonoRuntime* pRuntime):
		m_pRuntime(pRuntime),
		m_nRef(1)
	{
		MoveFirst();
	}

	// IMonoLibraryIt
	virtual void AddRef() { m_nRef++; }
	virtual void Release() { m_nRef--; if(m_nRef < 0){ delete this; } }
	virtual IMonoLibrary* This() { return IsEnd() ? NULL : m_pRuntime->m_loadedLibraries[m_id];  }
	virtual IMonoLibrary* Next() { return IsEnd() ? NULL : m_pRuntime->m_loadedLibraries[m_id++]; }
	virtual void MoveFirst() { m_id = 0; }
	virtual bool IsEnd() { return m_id >= m_pRuntime->m_loadedLibraries.size(); }
	// ~IMonoLibraryIt
private:
	CMonoRuntime*	m_pRuntime;
	int				m_id;
	int				m_nRef;
};