// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Platform/platform.h>
#include <CrySystem/ITimer.h>
#include <CrySystem/IConsole.h>
#ifdef ENABLE_AUDIO_LOGGING
	#include <CrySystem/ISystem.h>
#endif // ENABLE_AUDIO_LOGGING

enum EAudioLogType
{
	eAudioLogType_None    = 0,
	eAudioLogType_Comment = 1,
	eAudioLogType_Warning = 2,
	eAudioLogType_Error   = 3,
	eAudioLogType_Always  = 4,
};

enum EAudioLoggingOptions
{
	eAudioLoggingOptions_None     = 0,
	eAudioLoggingOptions_Errors   = BIT(6),  // a
	eAudioLoggingOptions_Warnings = BIT(7),  // b
	eAudioLoggingOptions_Comments = BIT(8),  // c
};

/**
 * A simpler logger wrapper that adds and audio tag and a timestamp
 */
class CAudioLogger final
{
public:

	//DOC-IGNORE-BEGIN
	CAudioLogger() = default;
	CAudioLogger(CAudioLogger const&) = delete;
	CAudioLogger& operator=(CAudioLogger const&) = delete;
	//DOC-IGNORE-END

	/**
	 * Log a message
	 * @param eType        - log message type (eALT_COMMENT, eALT_WARNING, eALT_ERROR or eALT_ALWAYS)
	 * @param sFormat, ... - printf-style format string and its arguments
	 */
	static void Log(EAudioLogType const eType, char const* const sFormat, ...)
	{
#if defined(ENABLE_AUDIO_LOGGING)
		if (sFormat && sFormat[0] && gEnv->pLog->GetVerbosityLevel() != -1)
		{
			FRAME_PROFILER("CAudioLogger::Log", GetISystem(), PROFILE_AUDIO);

			char sBuffer[256];
			va_list ArgList;
			va_start(ArgList, sFormat);
			cry_vsprintf(sBuffer, sFormat, ArgList);
			va_end(ArgList);

			float const fCurrTime = gEnv->pTimer->GetAsyncCurTime();

			ICVar* const pCVar = gEnv->pConsole->GetCVar("s_AudioLoggingOptions");

			if (pCVar != nullptr)
			{
				EAudioLoggingOptions const audioLoggingOptions = (EAudioLoggingOptions)pCVar->GetIVal();

				switch (eType)
				{
				case eAudioLogType_Warning:
					{
						if ((audioLoggingOptions & eAudioLoggingOptions_Warnings) > 0)
						{
							gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, nullptr, "<Audio> <%.3f>: %s", fCurrTime, sBuffer);
						}

						break;
					}
				case eAudioLogType_Error:
					{
						if ((audioLoggingOptions & eAudioLoggingOptions_Errors) > 0)
						{
							gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_ERROR, VALIDATOR_FLAG_AUDIO, nullptr, "<Audio> <%.3f>: %s", fCurrTime, sBuffer);
						}

						break;
					}
				case eAudioLogType_Comment:
					{
						if ((gEnv->pLog != nullptr) && (gEnv->pLog->GetVerbosityLevel() >= 4) && ((audioLoggingOptions & eAudioLoggingOptions_Comments) > 0))
						{
							CryLogAlways("<Audio> <%.3f>: %s", fCurrTime, sBuffer);
						}

						break;
					}
				case eAudioLogType_Always:
					{
						CryLogAlways("<Audio> <%.3f>: %s", fCurrTime, sBuffer);

						break;
					}
				default:
					{
						CRY_ASSERT(false);

						break;
					}
				}
			}
		}
#endif // ENABLE_AUDIO_LOGGING
	}
};
