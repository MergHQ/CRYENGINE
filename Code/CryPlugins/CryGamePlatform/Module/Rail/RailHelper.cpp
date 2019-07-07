// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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

			const char* Helper::ErrorString(rail::RailResult code)
			{
				switch (code)
				{
				case rail::kSuccess:													return "Success";
				case rail::kFailure:													return "Failure: general failure error code.";
				case rail::kErrorInvalidParam:											return "ErrorInvalidParam: input parameter is invalid.";
				case rail::kErrorImageNotFound:											return "ErrorImageNotFound: image not found.";
				case rail::kErrorBufferTooSmall:										return "ErrorBufferTooSmall: input buffer is too small.";
				case rail::kErrorNetworkError:											return "ErrorNetworkError: network is unavailable.";
				case rail::kErrorUnimplemented:											return "ErrorUnimplemented: called interface is not implemented.";
				case rail::kErrorInvalidCustomKey:										return "ErrorInvalidCustomKey: custom key used in game can not start with 'rail_' prefix.";
				case rail::kErrorClientInOfflineMode:									return "ErrorClientInOfflineMode: client is in off-line mode, all asynchronous interface will return this error code.";
				case rail::kErrorParameterLengthTooLong:								return "ErrorParameterLengthTooLong: parameter length is too much long.";
				case rail::kErrorWebApiKeyNoAccessOnThisGame:							return "ErrorWebApiKeyNoAccessOnThisGame: Web API key has no access to this game.";
				case rail::kErrorOperationTimeout:										return "ErrorOperationTimeout: operations timeout, might caused by network issues.";
				case rail::kErrorServerResponseInvalid:									return "ErrorServerResponseInvalid: response from server is invalid.";
				case rail::kErrorServerInternalError:									return "ErrorServerInternalError: server internal error.";
				case rail::kErrorFileNotFound:											return "ErrorFileNotFound: cant not find file.";
				case rail::kErrorAccessDenied:											return "ErrorAccessDenied: cant access file.";
				case rail::kErrorOpenFileFailed:										return "ErrorOpenFileFailed: cant open file.";
				case rail::kErrorCreateFileFailed:										return "ErrorCreateFileFailed: create file failed.";
				case rail::kErrorReadFailed:											return "ErrorReadFailed: read file failed.";
				case rail::kErrorWriteFailed:											return "ErrorWriteFailed: write file failed.";
				case rail::kErrorFileDestroyed:											return "ErrorFileDestroyed: file have been destroyed.";
				case rail::kErrorFileDelete:											return "ErrorFileDelete: delete file failed.";
				case rail::kErrorFileQueryIndexOutofRange:								return "ErrorFileQueryIndexOutofRange: param index of GetFileNameAndSize out of range.";
				case rail::kErrorFileAvaiableQuotaMoreThanTotal:						return "ErrorFileAvaiableQuotaMoreThanTotal: cloud file size bigger than quota.";
				case rail::kErrorFileGetRemotePathError:								return "ErrorFileGetRemotePathError: get local cloud path failed when query quota.";
				case rail::kErrorFileIllegal:											return "ErrorFileIllegal: file is illegal.";
				case rail::kErrorStreamFileWriteParamError:								return "ErrorStreamFileWriteParamError: passing wrong param to AsyncWrite in StreamFile";
				case rail::kErrorStreamFileReadParamError:								return "ErrorStreamFileReadParamError: passing wrong param to AsyncRead in StreamFile.";
				case rail::kErrorStreamCloseErrorIOWritting:							return "ErrorStreamCloseErrorIOWritting: close writing stream file failed.";
				case rail::kErrorStreamCloseErrorIOReading:								return "ErrorStreamCloseErrorIOReading: close reading stream file failed.";
				case rail::kErrorStreamDeleteFileOpenFileError:							return "ErrorStreamDeleteFileOpenFileError: open stream file failed when delete stream file.";
				case rail::kErrorStreamRenameFileOpenFileError:							return "ErrorStreamRenameFileOpenFileError: open stream file failed when Rename stream file.";
				case rail::kErrorStreamReadOnlyError:									return "ErrorStreamReadOnlyError: write to a stream file when the stream file is read only.";
				case rail::kErrorStreamCreateFileRemoveOldFileError:					return "ErrorStreamCreateFileRemoveOldFileError: delete an old stream file when truncate a stream file.";
				case rail::kErrorStreamCreateFileNameNotAvailable:						return "ErrorStreamCreateFileNameNotAvailable: file name size bigger than 256 when open stream file.";
				case rail::kErrorStreamOpenFileErrorCloudStorageDisabledByPlatform:		return "ErrorStreamOpenFileErrorCloudStorageDisabledByPlatform: app cloud storage is disabled.";
				case rail::kErrorStreamOpenFileErrorCloudStorageDisabledByPlayer:		return "ErrorStreamOpenFileErrorCloudStorageDisabledByPlayer: player's cloud storage is disabled.";
				case rail::kErrorStoragePathNotFound:									return "ErrorStoragePathNotFound: file path is not available.";
				case rail::kErrorStorageFileCantOpen:									return "ErrorStorageFileCantOpen: cant open file.";
				case rail::kErrorStorageFileRefuseAccess:								return "ErrorStorageFileRefuseAccess: cant open file because of access.";
				case rail::kErrorStorageFileInvalidHandle:								return "ErrorStorageFileInvalidHandle: read or write to a file that file handle is not available.";
				case rail::kErrorStorageFileInUsingByAnotherProcess:					return "ErrorStorageFileInUsingByAnotherProcess: cant open file because it's using by another process.";
				case rail::kErrorStorageFileLockedByAnotherProcess:						return "ErrorStorageFileLockedByAnotherProcess: cant lock file because it's locked by another process.";
				case rail::kErrorStorageFileWriteDiskNotEnough:							return "ErrorStorageFileWriteDiskNotEnough: cant write to disk because it's not enough.";
				case rail::kErrorStorageFileCantCreateFileOrPath:						return "ErrorStorageFileCantCreateFileOrPath: path is not available when create file.";
				case rail::kErrorRoomCreateFailed:										return "ErrorRoomCreateFailed: create room failed.";
				case rail::kErrorKickOffFailed:											return "ErrorKickOffFailed: kickoff failed.";
				case rail::kErrorKickOffPlayerNotInRoom:								return "ErrorKickOffPlayerNotInRoom: the player not in room.";
				case rail::kErrorKickOffNotRoomOwner:									return "ErrorKickOffNotRoomOwner: only the room owner can kick off others.";
				case rail::kErrorKickOffPlayingGame:									return "ErrorKickOffPlayingGame: same game can't kick off player who is playing game.";
				case rail::kErrorRoomServerRequestInvalidData:							return "ErrorRoomServerRequestInvalidData: the request parameter is invalid.";
				case rail::kErrorRoomServerConnectTcaplusFail:							return "ErrorRoomServerConnectTcaplusFail: the back end server connects db failed.";
				case rail::kErrorRoomServerConnectTcaplusTimeOut:						return "ErrorRoomServerConnectTcaplusTimeOut: the back end server connects db timeout.";
				case rail::kErrorRoomServerWrongDataInTcaplus:							return "ErrorRoomServerWrongDataInTcaplus: the data in back end db is wrong.";
				case rail::kErrorRoomServerNoDataInTcaplus:								return "ErrorRoomServerNoDataInTcaplus: no related data found in back end db.";
				case rail::kErrorRoomServerAllocateRoomIdFail:							return "ErrorRoomServerAllocateRoomIdFail: allocate room id failed when creating room.";
				case rail::kErrorRoomServerCreateGroupInImCloudFail:					return "ErrorRoomServerCreateGroupInImCloudFail: allocate room resource failed when creating room.";
				case rail::kErrorRoomServerUserAlreadyInGame:							return "ErrorRoomServerUserAlreadyInGame: player already join one room of the game.";
				case rail::kErrorRoomServerQueryResultEmpty:							return "ErrorRoomServerQueryResultEmpty: the query result is empty.";
				case rail::kErrorRoomServerRoomFull:									return "ErrorRoomServerRoomFull: the joined room is full.";
				case rail::kErrorRoomServerRoomNotExist:								return "ErrorRoomServerRoomNotExist: the room doesn't exist.";
				case rail::kErrorRoomServerUserAlreadyInRoom:							return "ErrorRoomServerUserAlreadyInRoom: player already join the room.";
				case rail::kErrorRoomServerQueryRailIdServiceFail:						return "ErrorRoomServerQueryRailIdServiceFail: query rail id failed.";
				case rail::kErrorRoomServerImCloudFail:									return "ErrorRoomServerImCloudFail: system error.";
				case rail::kErrorRoomServerPbSerializeFail:								return "ErrorRoomServerPbSerializeFail: system error.";
				case rail::kErrorRoomServerDirtyWord:									return "ErrorRoomServerDirtyWord: the input content includes dirty word.";
				case rail::kErrorRoomServerNoPermission:								return "ErrorRoomServerNoPermission: no permission.";
				case rail::kErrorRoomServerLeaveUserNotInRoom:							return "ErrorRoomServerLeaveUserNotInRoom: the leaving player is not in the room.";
				case rail::kErrorRoomServerDestroiedRoomNotExist:						return "ErrorRoomServerDestroiedRoomNotExist: the room to destroy doesn't exist.";
				case rail::kErrorRoomServerUserIsNotRoomMember:							return "ErrorRoomServerUserIsNotRoomMember: the player is not the room member.";
				case rail::kErrorRoomServerLockFailed:									return "ErrorRoomServerLockFailed: system error.";
				case rail::kErrorRoomServerRouteMiss:									return "ErrorRoomServerRouteMiss: system error.";
				case rail::kErrorRoomServerRetry:										return "ErrorRoomServerRetry: need retry.";
				case rail::kErrorRoomSendDataNotImplemented:							return "ErrorRoomSendDataNotImplemented: this method is not supported.";
				case rail::kErrorRoomInvokeFailed:										return "ErrorRoomInvokeFailed: network error.";
				case rail::kErrorRoomServerPasswordIncorrect:							return "ErrorRoomServerPasswordIncorrect: room password doesn't match.";
				case rail::kErrorRoomServerRoomIsNotJoinable:							return "ErrorRoomServerRoomIsNotJoinable: the room is not joinable.";
				case rail::kErrorStats:													return "ErrorStats: 1013000";
				case rail::kErrorStatsDontSetOtherPlayerStat:							return "ErrorStatsDontSetOtherPlayerStat: can't set other player's statistics.";
				case rail::kErrorAchievement:											return "ErrorAchievement: 1014000";
				case rail::kErrorAchievementOutofRange:									return "ErrorAchievementOutofRange: not any more achievement.";
				case rail::kErrorAchievementNotMyAchievement:							return "ErrorAchievementNotMyAchievement: can't set other player's achievement.";
				case rail::kErrorLeaderboard:											return "ErrorLeaderboard: 1015000";
				case rail::kErrorLeaderboardNotExists:									return "ErrorLeaderboardNotExists: leaderboard not exists.";
				case rail::kErrorLeaderboardNotBePrepared:								return "ErrorLeaderboardNotBePrepared: leaderboard not be prepared, call IRailLeaderboard::				AsyncGetLeaderboard first.";
				case rail::kErrorLeaderboardCreattionNotSupport:						return "ErrorLeaderboardCreattionNotSupport: not support AsyncCreateLeaderboard because your game doesn't configure leaderboard.";
				case rail::kErrorAssets:												return "ErrorAssets: 1016000";
				case rail::kErrorAssetsPending:											return "ErrorAssetsPending: assets still in pending.";
				case rail::kErrorAssetsOK:												return "ErrorAssetsOK: 1016002";
				case rail::kErrorAssetsExpired:											return "ErrorAssetsExpired: assets expired.";
				case rail::kErrorAssetsInvalidParam:									return "ErrorAssetsInvalidParam: passing invalid param.";
				case rail::kErrorAssetsServiceUnavailable:								return "ErrorAssetsServiceUnavailable: service unavailable.";
				case rail::kErrorAssetsLimitExceeded:									return "ErrorAssetsLimitExceeded: assets exceed limit.";
				case rail::kErrorAssetsFailed:											return "ErrorAssetsFailed: 1016007";
				case rail::kErrorAssetsRailIdInvalid:									return "ErrorAssetsRailIdInvalid: rail id invalid.";
				case rail::kErrorAssetsGameIdInvalid:									return "ErrorAssetsGameIdInvalid: game id invalid.";
				case rail::kErrorAssetsRequestInvokeFailed:								return "ErrorAssetsRequestInvokeFailed: request failed.";
				case rail::kErrorAssetsUpdateConsumeProgressNull:						return "ErrorAssetsUpdateConsumeProgressNull: progress is null when update consume progress.";
				case rail::kErrorAssetsCanNotFindAssetId:								return "ErrorAssetsCanNotFindAssetId: cant file asset when do split or exchange or merge.";
				case rail::kErrorAssetInvalidRequest:									return "ErrorAssetInvalidRequest: invalid request.";
				case rail::kErrorAssetDBFail:											return "ErrorAssetDBFail: query db failed in server back end.";
				case rail::kErrorAssetDataTooOld:										return "ErrorAssetDataTooOld: local asset data too old.";
				case rail::kErrorAssetInConsume:										return "ErrorAssetInConsume: asset still in consuming.";
				case rail::kErrorAssetNotExist:											return "ErrorAssetNotExist: asset not exist.";
				case rail::kErrorAssetExchangNotMatch:									return "ErrorAssetExchangNotMatch: asset exchange not match.";
				case rail::kErrorAssetSystemError:										return "ErrorAssetSystemError: asset system error back end.";
				case rail::kErrorAssetBadJasonData:										return "ErrorAssetBadJasonData: 1016020";
				case rail::kErrorAssetStateNotConsuing:									return "ErrorAssetStateNotConsuing: asset state is not consuming.";
				case rail::kErrorAssetStateConsuing:									return "ErrorAssetStateConsuing: asset state is consuming.";
				case rail::kErrorAssetDifferentProductId:								return "ErrorAssetDifferentProductId: exchange asset error with different product.";
				case rail::kErrorAssetConsumeQuantityTooBig:							return "ErrorAssetConsumeQuantityTooBig: consume quantity bigger than exists.";
				case rail::kErrorAssetMissMatchRailId:									return "ErrorAssetMissMatchRailId: rail id miss match the serialized buffer.";
				case rail::kErrorAssetProductInfoNotReady:								return "ErrorAssetProductInfoNotReady: IRailInGamePurchase::lProducts should be called to cache product info to local memory.";
				case rail::kErrorInGamePurchase:										return "ErrorInGamePurchase: 1017000";
				case rail::kErrorInGamePurchaseProductInfoExpired:						return "ErrorInGamePurchaseProductInfoExpired: product information in client is expired.";
				case rail::kErrorInGamePurchaseAcquireSessionTicketFailed:				return "ErrorInGamePurchaseAcquireSessionTicketFailed: acquire session ticket failed.";
				case rail::kErrorInGamePurchaseParseWebContentFaild:					return "ErrorInGamePurchaseParseWebContentFaild: parse product information from web content failed.";
				case rail::kErrorInGamePurchaseProductIsNotExist:						return "ErrorInGamePurchaseProductIsNotExist: product information is not exist.";
				case rail::kErrorInGamePurchaseOrderIDIsNotExist:						return "ErrorInGamePurchaseOrderIDIsNotExist: order id is not exist.";
				case rail::kErrorInGamePurchasePreparePaymentRequestTimeout:			return "ErrorInGamePurchasePreparePaymentRequestTimeout: prepare payment request timeout.";
				case rail::kErrorInGamePurchaseCreateOrderFailed:						return "ErrorInGamePurchaseCreateOrderFailed: create order failed.";
				case rail::kErrorInGamePurchaseQueryOrderFailed:						return "ErrorInGamePurchaseQueryOrderFailed: query order failed.";
				case rail::kErrorInGamePurchaseFinishOrderFailed:						return "ErrorInGamePurchaseFinishOrderFailed: finish order failed.";
				case rail::kErrorInGamePurchasePaymentFailed:							return "ErrorInGamePurchasePaymentFailed: payment is failed.";
				case rail::kErrorInGamePurchasePaymentCancle:							return "ErrorInGamePurchasePaymentCancle: payment is canceled.";
				case rail::kErrorInGamePurchaseCreatePaymentBrowserFailed:				return "ErrorInGamePurchaseCreatePaymentBrowserFailed: create payment browser failed.";
				case rail::kErrorInGamePurchaseExceededProductPurchaseLimit:			return "ErrorInGamePurchaseExceededProductPurchaseLimit: exceeded product purchase limit.";
				case rail::kErrorInGameStorePurchase:									return "ErrorInGameStorePurchase: 1017500";
				case rail::kErrorInGameStorePurchasePaymentSuccess:						return "ErrorInGameStorePurchasePaymentSuccess: payment is success.";
				case rail::kErrorInGameStorePurchasePaymentFailure:						return "ErrorInGameStorePurchasePaymentFailure: payment is failed.";
				case rail::kErrorInGameStorePurchasePaymentCancle:						return "ErrorInGameStorePurchasePaymentCancle: payment is canceled.";
				case rail::kErrorPlayer:												return "ErrorPlayer: 1018000";
				case rail::kErrorPlayerUserFolderCreateFailed:							return "ErrorPlayerUserFolderCreateFailed: create user data folder failed.";
				case rail::kErrorPlayerUserFolderCanntFind:								return "ErrorPlayerUserFolderCanntFind: can't find user data folder.";
				case rail::kErrorPlayerUserNotFriend:									return "ErrorPlayerUserNotFriend: player is not friend.";
				case rail::kErrorPlayerGameNotSupportPurchaseKey:						return "ErrorPlayerGameNotSupportPurchaseKey: not support purchase key.";
				case rail::kErrorPlayerGetAuthenticateURLFailed:						return "ErrorPlayerGetAuthenticateURLFailed: get authenticate url failed.";
				case rail::kErrorPlayerGetAuthenticateURLServerError:					return "ErrorPlayerGetAuthenticateURLServerError: server error while get authenticate url.";
				case rail::kErrorPlayerGetAuthenticateURLInvalidURL:					return "ErrorPlayerGetAuthenticateURLInvalidURL: input url is not in the white list.";
				case rail::kErrorFriends:												return "ErrorFriends: 1019000";
				case rail::kErrorFriendsKeyFrontUseRail:								return "ErrorFriendsKeyFrontUseRail: 1019001";
				case rail::kErrorFriendsMetadataSizeInvalid:							return "ErrorFriendsMetadataSizeInvalid: the size of key_values is more than 50.";
				case rail::kErrorFriendsMetadataKeyLenInvalid:							return "ErrorFriendsMetadataKeyLenInvalid: the length of key is more than 256.";
				case rail::kErrorFriendsMetadataValueLenInvalid:						return "ErrorFriendsMetadataValueLenInvalid: the length of Value is more than 256.";
				case rail::kErrorFriendsMetadataKeyInvalid:								return "ErrorFriendsMetadataKeyInvalid: user's key name can not start with 'rail'.";
				case rail::kErrorFriendsGetMetadataFailed:								return "ErrorFriendsGetMetadataFailed: CommonKeyValueNode's error_code is not 0.";
				case rail::kErrorFriendsSetPlayTogetherSizeZero:						return "ErrorFriendsSetPlayTogetherSizeZero: player_list count is 0.";
				case rail::kErrorFriendsSetPlayTogetherContentSizeInvalid:				return "ErrorFriendsSetPlayTogetherContentSizeInvalid: the size of user_rich_content is more than 100.";
				case rail::kErrorFriendsInviteResponseTypeInvalid:						return "ErrorFriendsInviteResponseTypeInvalid: the invite result is invalid.";
				case rail::kErrorFriendsListUpdateFailed:								return "ErrorFriendsListUpdateFailed: the friend_list update failed.";
				case rail::kErrorFriendsAddFriendInvalidID:								return "ErrorFriendsAddFriendInvalidID: Request sent failed by invalid rail_id.";
				case rail::kErrorFriendsAddFriendNetworkError:							return "ErrorFriendsAddFriendNetworkError: Request sent failed by network error.";
				case rail::kErrorFriendsServerBusy:										return "ErrorFriendsServerBusy: server is busy. you could process the same handle later.";
				case rail::kErrorFriendsUpdateFriendsListTooFrequent:					return "ErrorFriendsUpdateFriendsListTooFrequent: update friends list too frequent.";
				case rail::kErrorSessionTicket:											return "ErrorSessionTicket: 1020000";
				case rail::kErrorSessionTicketGetTicketFailed:							return "ErrorSessionTicketGetTicketFailed: get session ticket failed.";
				case rail::kErrorSessionTicketAuthFailed:								return "ErrorSessionTicketAuthFailed: authenticate the session ticket failed.";
				case rail::kErrorSessionTicketAuthTicketAbandoned:						return "ErrorSessionTicketAuthTicketAbandoned: the session ticket is abandoned.";
				case rail::kErrorSessionTicketAuthTicketExpire:							return "ErrorSessionTicketAuthTicketExpire: the session ticket expired.";
				case rail::kErrorSessionTicketAuthTicketInvalid:						return "ErrorSessionTicketAuthTicketInvalid: the session ticket is invalid.";
				case rail::kErrorSessionTicketInvalidParameter:							return "ErrorSessionTicketInvalidParameter: the request parameter is invalid.";
				case rail::kErrorSessionTicketInvalidTicket:							return "ErrorSessionTicketInvalidTicket: invalid session ticket.";
				case rail::kErrorSessionTicketIncorrectTicketOwner:						return "ErrorSessionTicketIncorrectTicketOwner: the session ticket owner is not correct.";
				case rail::kErrorSessionTicketHasBeenCanceledByTicketOwner:				return "ErrorSessionTicketHasBeenCanceledByTicketOwner: the session ticket has been canceled by owner.";
				case rail::kErrorSessionTicketExpired:									return "ErrorSessionTicketExpired: the session ticket expired";
				case rail::kErrorFloatWindow:											return "ErrorFloatWindow: 1021000";
				case rail::kErrorFloatWindowInitFailed:									return "ErrorFloatWindowInitFailed: initialize is failed.";
				case rail::kErrorFloatWindowShowStoreInvalidPara:						return "ErrorFloatWindowShowStoreInvalidPara: input parameter is invalid.";
				case rail::kErrorFloatWindowShowStoreCreateBrowserFailed:				return "ErrorFloatWindowShowStoreCreateBrowserFailed: create store browser window failed.";
				case rail::kErrorUserSpace:												return "ErrorUserSpace: 1022000";
				case rail::kErrorUserSpaceGetWorkDetailFailed:							return "ErrorUserSpaceGetWorkDetailFailed: unable to query spacework's data.";
				case rail::kErrorUserSpaceDownloadError:								return "ErrorUserSpaceDownloadError: failed to download at least one file of spacework.";
				case rail::kErrorUserSpaceDescFileInvalid:								return "ErrorUserSpaceDescFileInvalid: spacework maybe broken, re-upload it to repair.";
				case rail::kErrorUserSpaceReplaceOldFileFailed:							return "ErrorUserSpaceReplaceOldFileFailed: cannot update disk using download files.";
				case rail::kErrorUserSpaceUserCancelSync:								return "ErrorUserSpaceUserCancelSync: user canceled the sync.";
				case rail::kErrorUserSpaceIDorUserdataPathInvalid:						return "ErrorUserSpaceIDorUserdataPathInvalid: internal error.";
				case rail::kErrorUserSpaceNoData:										return "ErrorUserSpaceNoData: there is no data in such field.";
				case rail::kErrorUserSpaceSpaceWorkIDInvalid:							return "ErrorUserSpaceSpaceWorkIDInvalid: use 0 as spacework id.";
				case rail::kErrorUserSpaceNoSyncingNow:									return "ErrorUserSpaceNoSyncingNow: there is no syncing to cancel.";
				case rail::kErrorUserSpaceSpaceWorkAlreadySyncing:						return "ErrorUserSpaceSpaceWorkAlreadySyncing: only one syncing is allowed to the same spacework.";
				case rail::kErrorUserSpaceSubscribePartialSuccess:						return "ErrorUserSpaceSubscribePartialSuccess: not all (un)subscribe operations success.";
				case rail::kErrorUserSpaceNoVersionField:								return "ErrorUserSpaceNoVersionField: missing version field when changing files for spacework.";
				case rail::kErrorUserSpaceUpdateFailedWhenUploading:					return "ErrorUserSpaceUpdateFailedWhenUploading: can not query spacework's data when uploading, the spacework may not exist.";
				case rail::kErrorUserSpaceGetTicketFailed:								return "ErrorUserSpaceGetTicketFailed: can not get ticket when uploading, usually network issues.";
				case rail::kErrorUserSpaceVersionOccupied:								return "ErrorUserSpaceVersionOccupied: new version value must not equal to the last version.";
				case rail::kErrorUserSpaceCallCreateMethodFailed:						return "ErrorUserSpaceCallCreateMethodFailed: facing network issues when create a spacework.";
				case rail::kErrorUserSpaceCreateMethodRspFailed:						return "ErrorUserSpaceCreateMethodRspFailed: server failed to create a spacework.";
				case rail::kErrorUserSpaceNoEditablePermission:							return "ErrorUserSpaceNoEditablePermission: you have no permissions to change this spacework.";
				case rail::kErrorUserSpaceCallEditMethodFailed:							return "ErrorUserSpaceCallEditMethodFailed: facing network issues when committing changes of a spacework.";
				case rail::kErrorUserSpaceEditMethodRspFailed:							return "ErrorUserSpaceEditMethodRspFailed: server failed to commit changes of a spacework.";
				case rail::kErrorUserSpaceMetadataHasInvalidKey:						return "ErrorUserSpaceMetadataHasInvalidKey: the key of metadata should not start with Rail_(case insensitive).";
				case rail::kErrorUserSpaceModifyFavoritePartialSuccess:					return "ErrorUserSpaceModifyFavoritePartialSuccess: not all (un)favorite operations success.";
				case rail::kErrorUserSpaceFilePathTooLong:								return "ErrorUserSpaceFilePathTooLong: the path of file is too long to upload or download.";
				case rail::kErrorUserSpaceInvalidContentFolder:							return "ErrorUserSpaceInvalidContentFolder: the content folder provided is invalid, check the folder path is exist.";
				case rail::kErrorUserSpaceInvalidFilePath:								return "ErrorUserSpaceInvalidFilePath: internal error, the upload file path is invalid.";
				case rail::kErrorUserSpaceUploadFileNotMeetLimit:						return "ErrorUserSpaceUploadFileNotMeetLimit: file to be uploaded don't meet requirements, such as size and format and so on.";
				case rail::kErrorUserSpaceCannotReadFileToBeUploaded:					return "ErrorUserSpaceCannotReadFileToBeUploaded: can not read the file need to be uploaded technically won't happen in updating a existing spacework, check whether the.";
				case rail::kErrorUserSpaceUploadSpaceWorkHasNoVersionField:				return "ErrorUserSpaceUploadSpaceWorkHasNoVersionField: usually network issues, try again.";
				case rail::kErrorUserSpaceDownloadCurrentDescFileFailed:				return "ErrorUserSpaceDownloadCurrentDescFileFailed: download current version's description file failed when no file changed in the new version, call StartSync again w.";
				case rail::kErrorUserSpaceCannotGetSpaceWorkDownloadUrl:				return "ErrorUserSpaceCannotGetSpaceWorkDownloadUrl: can not get the download url of spacework, this spacework maybe broken.";
				case rail::kErrorUserSpaceCannotGetSpaceWorkUploadUrl:					return "ErrorUserSpaceCannotGetSpaceWorkUploadUrl: can not get the upload url of spacework, spacework maybe broken.";
				case rail::kErrorUserSpaceCannotReadFileWhenUploading:					return "ErrorUserSpaceCannotReadFileWhenUploading: can not read file when uploading, make sure you haven't changed the file when uploading.";
				case rail::kErrorUserSpaceUploadFileTooLarge:							return "ErrorUserSpaceUploadFileTooLarge: file uploaded should be smaller than 2^53 - 1 bytes.";
				case rail::kErrorUserSpaceUploadRequestTimeout:							return "ErrorUserSpaceUploadRequestTimeout: upload http request time out, check your network connections.";
				case rail::kErrorUserSpaceUploadRequestFailed:							return "ErrorUserSpaceUploadRequestFailed: upload file failed.";
				case rail::kErrorUserSpaceUploadInternalError:							return "ErrorUserSpaceUploadInternalError: internal error.";
				case rail::kErrorUserSpaceUploadCloudServerError:						return "ErrorUserSpaceUploadCloudServerError: get error from cloud server or can not get needed data from response.";
				case rail::kErrorUserSpaceUploadCloudServerRspInvalid:					return "ErrorUserSpaceUploadCloudServerRspInvalid: cloud server response invalid data";
				case rail::kErrorUserSpaceUploadCopyNoExistCloudFile:					return "ErrorUserSpaceUploadCopyNoExistCloudFile: reuse the old version's files need copy them to new version location, the old version file is not exist, it may be cleng time, please re-upload the full content folder.";
				case rail::kErrorUserSpaceShareLevelNotSatisfied:						return "ErrorUserSpaceShareLevelNotSatisfied: there are some limits of this type.";
				case rail::kErrorUserSpaceHasntBeenApproved:							return "ErrorUserSpaceHasntBeenApproved: the spacework has been submit with public share level and hasn't been approved or rejected so you can't submit again until it is approved or rejected.";
				case rail::kErrorGameServer:											return "ErrorGameServer: 1023000";
				case rail::kErrorGameServerCreateFailed:								return "ErrorGameServerCreateFailed: create game server failed.";
				case rail::kErrorGameServerDisconnectedServerlist:						return "ErrorGameServerDisconnectedServerlist: the game server disconnects from game server list.";
				case rail::kErrorGameServerConnectServerlistFailure:					return "ErrorGameServerConnectServerlistFailure: report game server to game server list failed.";
				case rail::kErrorGameServerSetMetadataFailed:							return "ErrorGameServerSetMetadataFailed: set game server meta data failed.";
				case rail::kErrorGameServerGetMetadataFailed:							return "ErrorGameServerGetMetadataFailed: get game server meta data failed.";
				case rail::kErrorGameServerGetServerListFailed:							return "ErrorGameServerGetServerListFailed: query game server list failed.";
				case rail::kErrorGameServerGetPlayerListFailed:							return "ErrorGameServerGetPlayerListFailed: query game server player list failed.";
				case rail::kErrorGameServerPlayerNotJoinGameserver:						return "ErrorGameServerPlayerNotJoinGameserver: 1023008";
				case rail::kErrorGameServerNeedGetFovariteFirst:						return "ErrorGameServerNeedGetFovariteFirst: should get favorite list first.";
				case rail::kErrorGameServerAddFovariteFailed:							return "ErrorGameServerAddFovariteFailed: add game server to favorite list failed.";
				case rail::kErrorGameServerRemoveFovariteFailed:						return "ErrorGameServerRemoveFovariteFailed: remove game server from favorite list failed.";
				case rail::kErrorNetwork:												return "ErrorNetwork: 1024000";
				case rail::kErrorNetworkInitializeFailed:								return "ErrorNetworkInitializeFailed: initialize is failed.";
				case rail::kErrorNetworkSessionIsNotExist:								return "ErrorNetworkSessionIsNotExist: session is not exist.";
				case rail::kErrorNetworkNoAvailableDataToRead:							return "ErrorNetworkNoAvailableDataToRead: there is not available data to be read.";
				case rail::kErrorNetworkUnReachable:									return "ErrorNetworkUnReachable: network is unreachable.";
				case rail::kErrorNetworkRemotePeerOffline:								return "ErrorNetworkRemotePeerOffline: remote peer is offline.";
				case rail::kErrorNetworkServerUnavailabe:								return "ErrorNetworkServerUnavailabe: network server is unavailable.";
				case rail::kErrorNetworkConnectionDenied:								return "ErrorNetworkConnectionDenied: connect request is denied.";
				case rail::kErrorNetworkConnectionClosed:								return "ErrorNetworkConnectionClosed: connected session has been closed by remote peer.";
				case rail::kErrorNetworkConnectionReset:								return "ErrorNetworkConnectionReset: connected session has been reset.";
				case rail::kErrorNetworkSendDataSizeTooLarge:							return "ErrorNetworkSendDataSizeTooLarge: send data size is too big.";
				case rail::kErrorNetworkSessioNotRegistered:							return "ErrorNetworkSessioNotRegistered: remote peer does not register to server.";
				case rail::kErrorNetworkSessionTimeout:									return "ErrorNetworkSessionTimeout: remote register but no response.";
				case rail::kErrorDlc:													return "ErrorDlc: 1025000";
				case rail::kErrorDlcInstallFailed:										return "ErrorDlcInstallFailed: install dlc failed.";
				case rail::kErrorDlcUninstallFailed:									return "ErrorDlcUninstallFailed: uninstall dlc failed.";
				case rail::kErrorDlcGetDlcListTimeout:									return "ErrorDlcGetDlcListTimeout: deprecated.";
				case rail::kErrorDlcRequestInvokeFailed:								return "ErrorDlcRequestInvokeFailed: request failed when query dlc authority.";
				case rail::kErrorDlcRequestToofrequently:								return "ErrorDlcRequestToofrequently: request too frequently when query dlc authority.";
				case rail::kErrorUtils:													return "ErrorUtils: 1026000";
				case rail::kErrorUtilsImagePathNull:									return "ErrorUtilsImagePathNull: the image path is null.";
				case rail::kErrorUtilsImagePathInvalid:									return "ErrorUtilsImagePathInvalid: the image path is invalid.";
				case rail::kErrorUtilsImageDownloadFail:								return "ErrorUtilsImageDownloadFail: failed to download the image.";
				case rail::kErrorUtilsImageOpenLocalFail:								return "ErrorUtilsImageOpenLocalFail: failed to open local image file.";
				case rail::kErrorUtilsImageBufferAllocateFail:							return "ErrorUtilsImageBufferAllocateFail: failed to allocate image buffer.";
				case rail::kErrorUtilsImageReadLocalFail:								return "ErrorUtilsImageReadLocalFail: failed to read local image.";
				case rail::kErrorUtilsImageParseFail:									return "ErrorUtilsImageParseFail: failed parse the image.";
				case rail::kErrorUtilsImageScaleFail:									return "ErrorUtilsImageScaleFail: failed to scale the image.";
				case rail::kErrorUtilsImageUnknownFormat:								return "ErrorUtilsImageUnknownFormat: image image format is unknown.";
				case rail::kErrorUtilsImageNotNeedResize:								return "ErrorUtilsImageNotNeedResize: the image is not need to resize.";
				case rail::kErrorUtilsImageResizeParameterInvalid:						return "ErrorUtilsImageResizeParameterInvalid: the parameter used to resize image is invalid.";
				case rail::kErrorUtilsImageSaveFileFail:								return "ErrorUtilsImageSaveFileFail: could not save image.";
				case rail::kErrorUtilsDirtyWordsFilterTooManyInput:						return "ErrorUtilsDirtyWordsFilterTooManyInput: there are too many inputs for dirty words filter.";
				case rail::kErrorUtilsDirtyWordsHasInvalidString:						return "ErrorUtilsDirtyWordsHasInvalidString: there are invalid strings in the dirty words.";
				case rail::kErrorUtilsDirtyWordsNotReady:								return "ErrorUtilsDirtyWordsNotReady: dirty words utility is not ready.";
				case rail::kErrorUtilsDirtyWordsDllUnloaded:							return "ErrorUtilsDirtyWordsDllUnloaded: dirty words library is not loaded.";
				case rail::kErrorUtilsCrashAllocateFailed:								return "ErrorUtilsCrashAllocateFailed: crash report buffer can not be allocated.";
				case rail::kErrorUtilsCrashCallbackSwitchOff:							return "ErrorUtilsCrashCallbackSwitchOff: crash report callback switch is currently off.";
				case rail::kErrorUsers:													return "ErrorUsers: 1027000";
				case rail::kErrorUsersInvalidInviteCommandLine:							return "ErrorUsersInvalidInviteCommandLine: the invite command line provided is invalid.";
				case rail::kErrorUsersSetCommandLineFailed:								return "ErrorUsersSetCommandLineFailed: failed to set command line.";
				case rail::kErrorUsersInviteListEmpty:									return "ErrorUsersInviteListEmpty: the invite user list is empty.";
				case rail::kErrorUsersGenerateRequestFail:								return "ErrorUsersGenerateRequestFail: failed to generate invite request.";
				case rail::kErrorUsersUnknownInviteType:								return "ErrorUsersUnknownInviteType: the invite type provided is unknown.";
				case rail::kErrorUsersInvalidInviteUsersSize:							return "ErrorUsersInvalidInviteUsersSize: the user count to invite is invalid.";
				case rail::kErrorScreenshot:											return "ErrorScreenshot: 1028000";
				case rail::kErrorScreenshotWorkNotExist:								return "ErrorScreenshotWorkNotExist: create space work for the screenshot failed.";
				case rail::kErrorScreenshotCantConvertPng:								return "ErrorScreenshotCantConvertPng: convert the screenshot image to png format failed.";
				case rail::kErrorScreenshotCopyFileFailed:								return "ErrorScreenshotCopyFileFailed: copy the screenshot image to publish folder failed.";
				case rail::kErrorScreenshotCantCreateThumbnail:							return "ErrorScreenshotCantCreateThumbnail: create a thumbnail image for screenshot failed.";
				case rail::kErrorVoiceCapture:											return "ErrorVoiceCapture: 1029000";
				case rail::kErrorVoiceCaptureInitializeFailed:							return "ErrorVoiceCaptureInitializeFailed: initialized failed.";
				case rail::kErrorVoiceCaptureDeviceLost:								return "ErrorVoiceCaptureDeviceLost: voice device lost.";
				case rail::kErrorVoiceCaptureIsRecording:								return "ErrorVoiceCaptureIsRecording: is already recording.";
				case rail::kErrorVoiceCaptureNotRecording:								return "ErrorVoiceCaptureNotRecording: is not recording.";
				case rail::kErrorVoiceCaptureNoData:									return "ErrorVoiceCaptureNoData: currently no voice data to get.";
				case rail::kErrorVoiceCaptureMoreData:									return "ErrorVoiceCaptureMoreData: there is more data to get.";
				case rail::kErrorVoiceCaptureDataCorrupted:								return "ErrorVoiceCaptureDataCorrupted: illegal data captured.";
				case rail::kErrorVoiceCapturekUnsupportedCodec:							return "ErrorVoiceCapturekUnsupportedCodec: illegal data to decode.";
				case rail::kErrorVoiceChannelHelperNotReady:							return "ErrorVoiceChannelHelperNotReady: voice module is not ready now, try again later.";
				case rail::kErrorVoiceChannelIsBusy:									return "ErrorVoiceChannelIsBusy: voice channel is too busy to handle operation, try again later or slow down the operations.";
				case rail::kErrorVoiceChannelNotJoinedChannel:							return "ErrorVoiceChannelNotJoinedChannel: player haven't joined this channel, you can not do some operations on this channel like leave.";
				case rail::kErrorVoiceChannelLostConnection:							return "ErrorVoiceChannelLostConnection: lost connection to server now, sdk will automatically reconnect later.";
				case rail::kErrorVoiceChannelAlreadyJoinedAnotherChannel:				return "ErrorVoiceChannelAlreadyJoinedAnotherChannel: player could only join one channel at the same time.";
				case rail::kErrorVoiceChannelPartialSuccess:							return "ErrorVoiceChannelPartialSuccess: operation is not fully success.";
				case rail::kErrorVoiceChannelNotTheChannelOwner:						return "ErrorVoiceChannelNotTheChannelOwner: only the owner could remove users.";
				case rail::kErrorTextInputTextInputSendMessageToPlatformFailed:			return "ErrorTextInputTextInputSendMessageToPlatformFailed: 1030000";
				case rail::kErrorTextInputTextInputSendMessageToOverlayFailed:			return "ErrorTextInputTextInputSendMessageToOverlayFailed: 1030001";
				case rail::kErrorTextInputTextInputUserCanceled:						return "ErrorTextInputTextInputUserCanceled: 1030002";
				case rail::kErrorTextInputTextInputEnableChineseFailed:					return "ErrorTextInputTextInputEnableChineseFailed: 1030003";
				case rail::kErrorTextInputTextInputShowFailed:							return "ErrorTextInputTextInputShowFailed: 1030004";
				case rail::kErrorTextInputEnableIMEHelperTextInputWindowFailed:			return "ErrorTextInputEnableIMEHelperTextInputWindowFailed: 1030005";
				case rail::kErrorApps:													return "ErrorApps: 1031000";
				case rail::kErrorAppsCountingKeyExists:									return "ErrorAppsCountingKeyExists: 1031001";
				case rail::kErrorAppsCountingKeyDoesNotExist:							return "ErrorAppsCountingKeyDoesNotExist: 1031002";
				case rail::kErrorHttpSession:											return "ErrorHttpSession: 1032000";
				case rail::kErrorHttpSessionPostBodyContentConflictWithPostParameter:	return "ErrorHttpSessionPostBodyContentConflictWithPostParameter: 1032001";
				case rail::kErrorHttpSessionRequestMehotdNotPost:						return "ErrorHttpSessionRequestMehotdNotPost: 1032002";
				case rail::kErrorSmallObjectService:									return "ErrorSmallObjectService: 1033000";
				case rail::kErrorSmallObjectServiceObjectNotExist:						return "ErrorSmallObjectServiceObjectNotExist: 1033001";
				case rail::kErrorSmallObjectServiceFailedToRequestDownload:				return "ErrorSmallObjectServiceFailedToRequestDownload: 1033002";
				case rail::kErrorSmallObjectServiceDownloadFailed:						return "ErrorSmallObjectServiceDownloadFailed: 1033003";
				case rail::kErrorSmallObjectServiceFailedToWriteDisk:					return "ErrorSmallObjectServiceFailedToWriteDisk: 1033004";
				case rail::kErrorSmallObjectServiceFailedToUpdateObject:				return "ErrorSmallObjectServiceFailedToUpdateObject: 1033005";
				case rail::kErrorSmallObjectServicePartialDownloadSuccess:				return "ErrorSmallObjectServicePartialDownloadSuccess: 1033006";
				case rail::kErrorSmallObjectServiceObjectNetworkIssue:					return "ErrorSmallObjectServiceObjectNetworkIssue: 1033007";
				case rail::kErrorSmallObjectServiceObjectServerError:					return "ErrorSmallObjectServiceObjectServerError: 1033008";
				case rail::kErrorSmallObjectServiceInvalidBranch:						return "ErrorSmallObjectServiceInvalidBranch: 1033009";
				case rail::kErrorZoneServer:											return "ErrorZoneServer: 1034000";
				case rail::kErrorZoneServerValueDataIsNotExist:							return "ErrorZoneServerValueDataIsNotExist: 1034001";
				case rail::kErrorInGameCoin:											return "ErrorInGameCoin: 1035000";
				case rail::kErrorInGameCoinCreatePaymentBrowserFailed:					return "ErrorInGameCoinCreatePaymentBrowserFailed: create payment browser failed.";
				case rail::kErrorInGameCoinOperationTimeout:							return "ErrorInGameCoinOperationTimeout: operation time out.";
				case rail::kErrorInGameCoinPaymentFailed:								return "ErrorInGameCoinPaymentFailed: payment is failed.";
				case rail::kErrorInGameCoinPaymentCanceled:								return "ErrorInGameCoinPaymentCanceled: payment is canceled.";
				case rail::kRailErrorServerBegin:										return "RailErrorServerBegin: 2000000";
				case rail::kErrorPaymentSystem:											return "ErrorPaymentSystem: 2080001";
				case rail::kErrorPaymentParameterIlleage:								return "ErrorPaymentParameterIlleage: 2080008";
				case rail::kErrorPaymentOrderIlleage:									return "ErrorPaymentOrderIlleage: 2080011";
				case rail::kErrorAssetsInvalidParameter:								return "ErrorAssetsInvalidParameter: 2230001";
				case rail::kErrorAssetsSystemError:										return "ErrorAssetsSystemError: 2230007";
				case rail::kErrorDirtyWordsFilterNoPermission:							return "ErrorDirtyWordsFilterNoPermission: 2290028";
				case rail::kErrorDirtyWordsFilterCheckFailed:							return "ErrorDirtyWordsFilterCheckFailed: 2290029";
				case rail::kErrorDirtyWordsFilterSystemBusy:							return "ErrorDirtyWordsFilterSystemBusy: 2290030";
				case rail::kRailErrorInnerServerBegin:									return "RailErrorInnerServerBegin: 2500000";
				case rail::kErrorGameGrayCheckSnowError:								return "ErrorGameGrayCheckSnowError: 2500001";
				case rail::kErrorGameGrayParameterIlleage:								return "ErrorGameGrayParameterIlleage: 2500002";
				case rail::kErrorGameGraySystemError:									return "ErrorGameGraySystemError: 2500003";
				case rail::kErrorGameGrayQQToWegameidError:								return "ErrorGameGrayQQToWegameidError: 2500004";
				case rail::kRailErrorInnerServerEnd:									return "RailErrorInnerServerEnd: 2699999";
				case rail::kRailErrorServerEnd:											return "RailErrorServerEnd: 2999999";
				case rail::kErrorUnknown:												return "ErrorUnknown: unknown error";
				}

				return "Unknown Result";
			}
		}
	}
}

