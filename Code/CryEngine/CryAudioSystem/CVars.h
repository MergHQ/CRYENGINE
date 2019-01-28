// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct ICVar;
struct IConsoleCmdArgs;

namespace CryAudio
{
class CCVars final
{
public:

	CCVars(CCVars const&) = delete;
	CCVars(CCVars&&) = delete;
	CCVars& operator=(CCVars const&) = delete;
	CCVars& operator=(CCVars&&) = delete;

	CCVars() = default;

	void RegisterVariables();
	void UnregisterVariables();

	int   m_fileCacheManagerSize = 0;
	int   m_objectPoolSize = 0;
	int   m_standaloneFilePoolSize = 0;
	int   m_accumulateOcclusion = 1;
	int   m_ignoreWindowFocus = 0;
	int   m_occlusionCollisionTypes = 0;
	int   m_setFullOcclusionOnMaxHits = 0;

	float m_occlusionMaxDistance = 500.0f;
	float m_occlusionMinDistance = 0.1f;
	float m_occlusionMaxSyncDistance = 0.0f;
	float m_occlusionHighDistance = 0.0f;
	float m_occlusionMediumDistance = 0.0f;
	float m_fullObstructionMaxDistance = 0.0f;
	float m_listenerOcclusionPlaneSize = 0.0f;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	float  m_debugDistance = 0.0f;
	int    m_loggingOptions = 0;
	int    m_drawDebug = 0;
	int    m_fileCacheManagerDebugFilter = 0;
	int    m_hideInactiveObjects = 0;
	int    m_objectsRayType = 0;
	ICVar* m_pDebugFilter = nullptr;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};

extern CCVars g_cvars;
} // namespace CryAudio
