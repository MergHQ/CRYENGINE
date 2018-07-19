// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "DiscordService.h"
#include "IGamePlatform.h"

#include "DiscordUserIdentifier.h"

#include <CrySystem/ICmdLine.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Discord
		{
			CService* CService::s_pInstance = nullptr;

			CService::CService()
			{
				s_pInstance = this;
			}

			CService::~CService()
			{
				for (IListener* pListener : m_listeners)
				{
					pListener->OnShutdown(DiscordServiceID);
				}

				Discord_Shutdown();
			}

			bool CService::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
			{
				if (env.IsEditor())
				{
					// Don't use Discord integration in Sandbox
					return true;
				}

				const char* discord_appId = "";
				REGISTER_CVAR(discord_appId, "", VF_REQUIRE_APP_RESTART, "Discord application id. Requires application restart.");

				DiscordEventHandlers handlers;
				memset(&handlers, 0, sizeof(handlers));
				handlers.ready = OnDiscordReady;
				handlers.errored = OnDiscordError;
				handlers.disconnected = OnDiscordDisconnected;
				handlers.joinGame = nullptr;
				handlers.spectateGame = nullptr;
				handlers.joinRequest = nullptr;

				Cry::GamePlatform::IPlugin* pGamePlatform = gEnv->pSystem->GetIPluginManager()->QueryPlugin<Cry::GamePlatform::IPlugin>();
				Cry::GamePlatform::IService* pSteamService = pGamePlatform ? pGamePlatform->GetService(SteamServiceID) : nullptr;

				stack_string steamAppId;
				if (pSteamService)
				{
					steamAppId.Format("%ud", pSteamService->GetApplicationIdentifier());
				}
#ifdef CRY_PLATFORM_WINDOWS
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Discord] Unable to retrieve steam app id. Integration will be limited.");
				}
#endif

				Discord_Initialize(discord_appId, &handlers, 1, steamAppId.c_str());

				EnableUpdate(Cry::IEnginePlugin::EUpdateStep::MainUpdate, true);

				CryLogAlways("[Discord] Successfully initialized Discord Rich Presence API");

				if (pGamePlatform)
				{
					pGamePlatform->RegisterService(*this);
				}

				return true;
			}

			void CService::MainUpdate(float frameTime)
			{
				CRY_PROFILE_FUNCTION(PROFILE_SYSTEM);

				Discord_RunCallbacks();
			}

			ServiceIdentifier CService::GetServiceIdentifier() const
			{
				return DiscordServiceID;
			}

			int CService::GetBuildIdentifier() const
			{
				return 0;
			}

			CAccount* CService::GetLocalAccount() const
			{
				return m_localAccount.get();
			}
			
			bool CService::OwnsApplication(ApplicationIdentifier id) const
			{
				return false;
			}

			ApplicationIdentifier CService::GetApplicationIdentifier() const
			{
				return 0;
			}

			bool CService::OpenDialog(EDialog dialog) const
			{
				return false;
			}

			bool CService::OpenDialogWithTargetUser(EUserTargetedDialog dialog, const AccountIdentifier& accountId) const
			{
				return false;
			}

			bool CService::OpenDialog(const char* szPage) const
			{
				return false;
			}

			bool CService::OpenDialogWithTargetUser(const char* szPage, const AccountIdentifier& accountId) const
			{
				return false;
			}

			bool CService::OpenBrowser(const char* szURL) const
			{
				return false;
			}

			bool CService::CanOpenPurchaseOverlay() const
			{
				return false;
			}

			const DynArray<IAccount*>& CService::GetFriendAccounts() const
			{
				return m_friends;
			}

			CAccount* CService::GetAccountById(const AccountIdentifier& accountId) const
			{
				if (m_localAccount && m_localAccount->GetIdentifier() == accountId)
				{
					return m_localAccount.get();
				}

				return nullptr;
			}

			bool CService::IsFriendWith(const AccountIdentifier& accountId) const
			{
				return false;
			}

			EFriendRelationship CService::GetFriendRelationship(const AccountIdentifier& accountId) const
			{
				return EFriendRelationship::None;
			}

			IServer* CService::CreateServer(bool bLocal)
			{
				return nullptr;
			}

			bool CService::GetAuthToken(string &tokenOut, int &issuerId)
			{
				return false;
			}

			bool CService::RequestUserInformation(const AccountIdentifier& accountId, UserInformationMask info)
			{
				return false;
			}

			void CService::SetLocalUser(const DiscordUser* pUser)
			{
				if (pUser)
				{
					if (m_localAccount)
					{
						m_localAccount->SetDiscordUser(*pUser);
					}
					else
					{
						m_localAccount = stl::make_unique<CAccount>(*pUser);
					}

					std::for_each(m_listeners.begin(), m_listeners.end(), [this](IListener* pListener) { pListener->OnAccountAdded(*m_localAccount); });
				}
				else if(m_localAccount)
				{
					std::for_each(m_listeners.begin(), m_listeners.end(), [this](IListener* pListener) { pListener->OnAccountRemoved(*m_localAccount); });

					m_localAccount.reset();
				}
			}

			void CService::OnDiscordReady(const DiscordUser* pUser)
			{
				if (s_pInstance)
				{
					CryComment("[Discord] Rich Presence SDK user connected: userid='%s' nick='%s'", pUser->userId, pUser->username);

					s_pInstance->SetLocalUser(pUser);
				}
			}

			void CService::OnDiscordDisconnected(int errorCode, const char* szMessage)
			{
				if (errorCode)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Discord] Disconnected! API Error %d: '%s'", errorCode, szMessage);
				}
				else
				{
					CryComment("[Discord] Rich Presence SDK user disconnected.");
				}

				if (s_pInstance)
				{
					s_pInstance->SetLocalUser(nullptr);
				}
			}

			void CService::OnDiscordError(int errorCode, const char* szMessage)
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR,  "[Discord] API Error %d: '%s'", errorCode, szMessage);
			}

			CRYREGISTER_SINGLETON_CLASS(CService)
		}
	}
}
