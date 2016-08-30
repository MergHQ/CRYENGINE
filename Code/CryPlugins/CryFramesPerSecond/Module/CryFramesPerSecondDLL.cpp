// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CryFramesPerSecondDLL.h"
#include "FramesPerSecond.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

USE_CRYPLUGIN_FLOWNODES

CPlugin_CryFramesPerSecond::CPlugin_CryFramesPerSecond()
	: m_pFramesPerSecond(nullptr)
{
	m_pThis = this;
}

CPlugin_CryFramesPerSecond::~CPlugin_CryFramesPerSecond()
{
	SAFE_DELETE(m_pFramesPerSecond);
}

bool CPlugin_CryFramesPerSecond::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	m_pFramesPerSecond = new CFramesPerSecond();

	return (m_pFramesPerSecond != nullptr);
}

void CPlugin_CryFramesPerSecond::Update(int updateFlags, int nPauseMode)
{
	if (m_pFramesPerSecond)
	{
		m_pFramesPerSecond->Update();
	}
}

IFramesPerSecond* CPlugin_CryFramesPerSecond::GetIFramesPerSecond() const
{
	return m_pFramesPerSecond;
}

CRYREGISTER_SINGLETON_CLASS(CPlugin_CryFramesPerSecond)

#include <CryCore/CrtDebugStats.h>
