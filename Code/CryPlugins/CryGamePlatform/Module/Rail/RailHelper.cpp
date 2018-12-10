// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "RailHelper.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			const char* const Helper::szLibName =
#if CRY_PLATFORM_X64
				CryLibraryDefName("rail_api64");
#else
				CryLibraryDefName("rail_api");
#endif
			// Map ISO 639-1 language codes to engine
			// See: https://msdn.microsoft.com/en-us/library/cc233982.aspx
			const std::unordered_map<string, ILocalizationManager::EPlatformIndependentLanguageID, stl::hash_stricmp<string>, stl::hash_stricmp<string>> Helper::s_translationMappings
			{
				{ "ar",    ILocalizationManager::ePILID_Arabic },
				{ "ar_AE", ILocalizationManager::ePILID_Arabic },
				{ "ar_EG", ILocalizationManager::ePILID_Arabic },
				{ "ar_IQ", ILocalizationManager::ePILID_Arabic },
				{ "ar_JO", ILocalizationManager::ePILID_Arabic },
				{ "ar_KW", ILocalizationManager::ePILID_Arabic },
				{ "ar_LB", ILocalizationManager::ePILID_Arabic },
				{ "ar_LY", ILocalizationManager::ePILID_Arabic },
				{ "ar_MA", ILocalizationManager::ePILID_Arabic },
				{ "ar_OM", ILocalizationManager::ePILID_Arabic },
				{ "ar_QA", ILocalizationManager::ePILID_Arabic },
				{ "ar_SA", ILocalizationManager::ePILID_Arabic },
				{ "ar_SY", ILocalizationManager::ePILID_Arabic },
				{ "de",    ILocalizationManager::ePILID_German },
				{ "de_AT", ILocalizationManager::ePILID_German },
				{ "de_CH", ILocalizationManager::ePILID_German },
				{ "de_DE", ILocalizationManager::ePILID_German },
				{ "de_LU", ILocalizationManager::ePILID_German },
				// Technically supported by SDK but not (yet) by Engine
				// { "el"   , ILocalizationManager::ePILID_Greek },
				// { "el_GR", ILocalizationManager::ePILID_Greek },
				{ "en",    ILocalizationManager::ePILID_English },
				{ "en_AU", ILocalizationManager::ePILID_English },
				{ "en_CA", ILocalizationManager::ePILID_English },
				{ "en_GB", ILocalizationManager::ePILID_English },
				{ "en_IE", ILocalizationManager::ePILID_English },
				{ "en_NZ", ILocalizationManager::ePILID_English },
				{ "en_PH", ILocalizationManager::ePILID_English },
				{ "en_US", ILocalizationManager::ePILID_English },
				{ "es",    ILocalizationManager::ePILID_Spanish },
				{ "es_AR", ILocalizationManager::ePILID_Spanish },
				{ "es_CL", ILocalizationManager::ePILID_Spanish },
				{ "es_CO", ILocalizationManager::ePILID_Spanish },
				{ "es_MX", ILocalizationManager::ePILID_Spanish },
				{ "fr",    ILocalizationManager::ePILID_French },
				{ "fr_CA", ILocalizationManager::ePILID_French },
				{ "fr_CH", ILocalizationManager::ePILID_French },
				{ "fr_FR", ILocalizationManager::ePILID_French },
				{ "fr_LU", ILocalizationManager::ePILID_French },
				{ "it",    ILocalizationManager::ePILID_Italian },
				{ "it_IT", ILocalizationManager::ePILID_Italian },
				{ "it_CH", ILocalizationManager::ePILID_Italian },
				{ "ja",    ILocalizationManager::ePILID_Japanese },
				{ "ja_JP", ILocalizationManager::ePILID_Japanese },
				{ "ko",    ILocalizationManager::ePILID_Korean },
				{ "ko_KR", ILocalizationManager::ePILID_Korean },
				{ "ru",    ILocalizationManager::ePILID_Russian },
				{ "ru_RU", ILocalizationManager::ePILID_Russian },
				// Technically supported by SDK but not (yet) by Engine
				// { "th"   , ILocalizationManager::ePILID_Thai },
				// { "th_TH", ILocalizationManager::ePILID_Thai },
				// { "vi"   , ILocalizationManager::ePILID_Vietnamese },
				// { "vi_VN", ILocalizationManager::ePILID_Vietnamese },
				{ "zh",    ILocalizationManager::ePILID_ChineseS },
				{ "zh_CN", ILocalizationManager::ePILID_ChineseS },
				{ "zh_HK", ILocalizationManager::ePILID_ChineseT },
				{ "zh_MO", ILocalizationManager::ePILID_ChineseT },
				{ "zh_SG", ILocalizationManager::ePILID_ChineseS },
				{ "zh_TW", ILocalizationManager::ePILID_ChineseT },
				// Not yet supported by SDK
				{"tr",    ILocalizationManager::ePILID_Turkish },
				{"tr_TR", ILocalizationManager::ePILID_Turkish },
				{"tr_CY", ILocalizationManager::ePILID_Turkish },
				{"nl",    ILocalizationManager::ePILID_Dutch },
				{ "nl_AW", ILocalizationManager::ePILID_Dutch },
				{ "nl_BE", ILocalizationManager::ePILID_Dutch },
				{ "nl_BQ", ILocalizationManager::ePILID_Dutch },
				{ "nl_CW", ILocalizationManager::ePILID_Dutch },
				{ "nl_NL", ILocalizationManager::ePILID_Dutch },
				{ "nl_SX", ILocalizationManager::ePILID_Dutch },
				{ "nl_SR", ILocalizationManager::ePILID_Dutch },
				{ "pt",    ILocalizationManager::ePILID_Portuguese },
				{ "pt_AO", ILocalizationManager::ePILID_Portuguese },
				{ "pt_BR", ILocalizationManager::ePILID_Portuguese },
				{ "pt_CV", ILocalizationManager::ePILID_Portuguese },
				{ "pt_GQ", ILocalizationManager::ePILID_Portuguese },
				{ "pt_GW", ILocalizationManager::ePILID_Portuguese },
				{ "pt_LU", ILocalizationManager::ePILID_Portuguese },
				{ "pt_MO", ILocalizationManager::ePILID_Portuguese },
				{ "pt_MZ", ILocalizationManager::ePILID_Portuguese },
				{ "pt_PT", ILocalizationManager::ePILID_Portuguese },
				{ "pt_ST", ILocalizationManager::ePILID_Portuguese },
				{ "pt_CH", ILocalizationManager::ePILID_Portuguese },
				{ "pt_TL", ILocalizationManager::ePILID_Portuguese },
				{ "fi",    ILocalizationManager::ePILID_Finnish },
				{ "fi_FI", ILocalizationManager::ePILID_Finnish },
				{ "sv",    ILocalizationManager::ePILID_Swedish },
				{ "sv_AX", ILocalizationManager::ePILID_Swedish },
				{ "sv_FI", ILocalizationManager::ePILID_Swedish },
				{ "sv_SE", ILocalizationManager::ePILID_Swedish },
				{ "da",    ILocalizationManager::ePILID_Danish },
				{ "da_DK", ILocalizationManager::ePILID_Danish },
				{ "da_GL", ILocalizationManager::ePILID_Danish },
				{ "no",    ILocalizationManager::ePILID_Norwegian },
				{ "nb",    ILocalizationManager::ePILID_Norwegian },
				{ "nb_NO", ILocalizationManager::ePILID_Norwegian },
				{ "nn",    ILocalizationManager::ePILID_Norwegian },
				{ "nn_NO", ILocalizationManager::ePILID_Norwegian },
				{ "nb_SJ", ILocalizationManager::ePILID_Norwegian },
				{ "pl",    ILocalizationManager::ePILID_Polish },
				{ "pl_PL", ILocalizationManager::ePILID_Polish },
				{ "cs",    ILocalizationManager::ePILID_Czech },
				{ "cs_CZ", ILocalizationManager::ePILID_Czech }
			};

			std::unordered_map<rail::RailResult, rail::RailString> Helper::s_errorCache;

			HMODULE Helper::s_libHandle = nullptr;
			rail::helper::RailFuncPtr_RailNeedRestartAppForCheckingEnvironment Helper::s_pNeedRestart = nullptr;
			rail::helper::RailFuncPtr_RailInitialize Helper::s_pInitialize = nullptr;
			rail::helper::RailFuncPtr_RailFinalize Helper::s_pFinalize = nullptr;
			rail::helper::RailFuncPtr_RailRegisterEvent Helper::s_pRegisterEvent = nullptr;
			rail::helper::RailFuncPtr_RailUnregisterEvent Helper::s_pUnregisterEvent = nullptr;
			rail::helper::RailFuncPtr_RailFireEvents Helper::s_pFireEvents = nullptr;
			rail::helper::RailFuncPtr_RailFactory Helper::s_pFactory = nullptr;
			rail::helper::RailFuncPtr_RailGetSdkVersion Helper::s_pGetSdkVersion = nullptr;

			bool Helper::Setup(rail::RailGameID gameId)
			{
				if (!s_libHandle)
				{
					s_libHandle = CryLoadLibrary(szLibName);
					if (s_libHandle == nullptr)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Helper] Loading of dynamic library '%s' failed!", szLibName);
						return false;
					}
				}

				bool bFailed = false;
				bFailed |= GetFptr("RailNeedRestartAppForCheckingEnvironment", s_pNeedRestart);
				bFailed |= GetFptr("RailInitialize", s_pInitialize);
				bFailed |= GetFptr("RailFinalize", s_pFinalize);
				bFailed |= GetFptr("RailRegisterEvent", s_pRegisterEvent);
				bFailed |= GetFptr("RailUnregisterEvent", s_pUnregisterEvent);
				bFailed |= GetFptr("RailFireEvents", s_pFireEvents);
				bFailed |= GetFptr("RailFactory", s_pFactory);
				bFailed |= GetFptr("RailGetSdkVersion", s_pGetSdkVersion);

				if (bFailed)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Helper] Function(s) not found in '%s'. Version mismatch?", szLibName);
					CryFreeLibrary(s_libHandle);
					s_libHandle = nullptr;

					return false;
				}

				rail::RailString railVersion;
				rail::RailString railDescription;
				s_pGetSdkVersion(&railVersion, &railDescription);

				CryLogAlways("[Rail][Helper] Using SDK version '%s' : %s", railVersion.c_str(), railDescription.c_str());

#if !defined(RELEASE)
				const char* argv[] = { "--rail_debug_mode" };
				constexpr size_t argc = sizeof(argv) / sizeof(const char*);
#else
				const char** argv = nullptr;
				constexpr size_t argc = 0;
#endif

				// Validate that the application was started through Rail. If this fails we should quit the application.
				if (s_pNeedRestart(gameId, argc, argv))
				{
#if defined(RELEASE)
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "[Rail][Helper] WeGame client is not running! Quitting...");
					gEnv->pSystem->Quit();
					return false;
#else
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "[Rail][Helper] Unable to connect to WeGame client. Please start the client.");
#endif
				}

				return true;
			}

			bool Helper::MapLanguageCode(const rail::RailString& code, ILocalizationManager::EPlatformIndependentLanguageID& languageId)
			{
				const auto remapIt = s_translationMappings.find(code.c_str());
				if (remapIt != s_translationMappings.end())
				{
					languageId = remapIt->second;
					return true;
				}

				return false;
			}
		}
	}
}

