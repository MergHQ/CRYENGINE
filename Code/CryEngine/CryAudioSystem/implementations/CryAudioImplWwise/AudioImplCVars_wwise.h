// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
class CAudioImplCVars_wwise
{
public:

	CAudioImplCVars_wwise();
	~CAudioImplCVars_wwise();

	void RegisterVariables();
	void UnregisterVariables();

	int m_nPrimaryMemoryPoolSize;
	int m_nSecondaryMemoryPoolSize;
	int m_nPrepareEventMemoryPoolSize;
	int m_nStreamManagerMemoryPoolSize;
	int m_nStreamDeviceMemoryPoolSize;
	int m_nSoundEngineDefaultMemoryPoolSize;
	int m_nCommandQueueMemoryPoolSize;
	int m_nLowerEngineDefaultPoolSize;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	int m_nEnableCommSystem;
	int m_nEnableOutputCapture;
	int m_nMonitorMemoryPoolSize;
	int m_nMonitorQueueMemoryPoolSize;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	PREVENT_OBJECT_COPY(CAudioImplCVars_wwise);
};

extern CAudioImplCVars_wwise g_AudioImplCVars_wwise;
}
}
