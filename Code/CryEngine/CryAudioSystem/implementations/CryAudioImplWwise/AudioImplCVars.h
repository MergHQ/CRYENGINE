// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CAudioImplCVars final
{
public:

	CAudioImplCVars() = default;
	CAudioImplCVars(CAudioImplCVars const&) = delete;
	CAudioImplCVars(CAudioImplCVars&&) = delete;
	CAudioImplCVars& operator=(CAudioImplCVars const&) = delete;
	CAudioImplCVars& operator=(CAudioImplCVars&&) = delete;

	void RegisterVariables();
	void UnregisterVariables();

	int m_secondaryMemoryPoolSize = 0;
	int m_prepareEventMemoryPoolSize = 0;
	int m_streamManagerMemoryPoolSize = 0;
	int m_streamDeviceMemoryPoolSize = 0;
	int m_soundEngineDefaultMemoryPoolSize = 0;
	int m_commandQueueMemoryPoolSize = 0;
	int m_lowerEngineDefaultPoolSize = 0;
	int m_enableEventManagerThread = 0;
	int m_enableSoundBankManagerThread = 0;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	int m_enableCommSystem = 0;
	int m_enableOutputCapture = 0;
	int m_monitorMemoryPoolSize = 0;
	int m_monitorQueueMemoryPoolSize = 0;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};

extern CAudioImplCVars g_audioImplCVars;
}
}
}
