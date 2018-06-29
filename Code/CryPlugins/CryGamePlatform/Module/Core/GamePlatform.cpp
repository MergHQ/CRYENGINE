// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GamePlatform.h"

#include "User.h"

#include <CrySystem/ICmdLine.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

namespace Cry
{
	namespace GamePlatform
	{
		CPlugin::CPlugin()
		{
			// First element is reserved for main service
			m_services.emplace_back(nullptr);
		}

		CPlugin::~CPlugin()
		{
		}

		bool CPlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
		{
			return true;
		}

		void CPlugin::MainUpdate(float frameTime)
		{
			CRY_PROFILE_FUNCTION(PROFILE_SYSTEM);
		}

		IUser* CPlugin::GetLocalClient() const
		{
			if (const IService* pMainService = GetMainService())
			{
				if (IAccount* pMainAccount = pMainService->GetLocalAccount())
				{
					return TryGetUser(*pMainAccount);
				}
			}

			return nullptr;
		}

		IUser* CPlugin::GetUserById(const UserIdentifier& id) const
		{
			return TryGetUser(id);
		}

		IUser* CPlugin::GetUserById(const AccountIdentifier& accountId) const
		{
			return TryGetUser(accountId);
		}

		const DynArray<IUser*>& CPlugin::GetFriends() const
		{
			m_friends.clear();

			if (const IService* pMainService = GetMainService())
			{
				const DynArray<IAccount*>& accounts = pMainService->GetFriendAccounts();
				for (IAccount* pAccount : accounts)
				{
					m_friends.push_back(TryGetUser(*pAccount));
				}
			}

			for (const IService* pService : m_services)
			{
				const DynArray<IAccount*>& accounts = pService->GetFriendAccounts();
				for (IAccount* pAccount : accounts)
				{
					m_friends.push_back(TryGetUser(*pAccount));
				}
			}

			return m_friends;
		}

		IServer* CPlugin::CreateServer(bool bLocal)
		{
			if (IService* pMainService = GetMainService())
			{
				return pMainService->CreateServer(bLocal);
			}

			return nullptr;
		}

		IServer* CPlugin::GetLocalServer() const
		{
			if (const IService* pMainService = GetMainService())
			{
				return pMainService->GetLocalServer();
			}

			return nullptr;
		}

		ILeaderboards* CPlugin::GetLeaderboards() const
		{
			if (const IService* pMainService = GetMainService())
			{
				return pMainService->GetLeaderboards();
			}

			return nullptr;
		}

		IStatistics* CPlugin::GetStatistics() const
		{
			if (const IService* pMainService = GetMainService())
			{
				return pMainService->GetStatistics();
			}

			return nullptr;
		}

		IRemoteStorage* CPlugin::GetRemoteStorage() const
		{
			if (const IService* pMainService = GetMainService())
			{
				return pMainService->GetRemoteStorage();
			}

			return nullptr;
		}

		IMatchmaking* CPlugin::GetMatchmaking() const
		{
			if (const IService* pMainService = GetMainService())
			{
				return pMainService->GetMatchmaking();
			}

			return nullptr;
		}

		INetworking* CPlugin::GetNetworking() const
		{
			if (const IService* pMainService = GetMainService())
			{
				return pMainService->GetNetworking();
			}

			return nullptr;
		}

		IUser* CPlugin::TryGetUser(const UserIdentifier& id) const
		{
			for (const std::unique_ptr<CUser>& pUser : m_users)
			{
				if (pUser->GetIdentifier() == id)
				{
					return pUser.get();
				}
			}

			if (const IService* pMainService = GetMainService())
			{
				if (id.HasAccount(pMainService->GetServiceIdentifier()))
				{
					const AccountIdentifier accountId = id.GetAccount(pMainService->GetServiceIdentifier());
					if (IAccount* pUserAccount = pMainService->GetAccountById(accountId))
					{
						return AddUser(*pUserAccount);
					}
				}
			}

			return nullptr;
		}

		IUser* CPlugin::TryGetUser(const AccountIdentifier& id) const
		{
			const IService* pMainService = GetMainService();
			if (pMainService == nullptr)
			{
				return nullptr;
			}

			const ServiceIdentifier& svcId = id.Service();
			if (pMainService->GetServiceIdentifier() == svcId)
			{
				for (const std::unique_ptr<CUser>& pUser : m_users)
				{
					if (const IAccount* pAccount = pUser->GetAccount(svcId))
					{
						if (pAccount->GetIdentifier() == id)
						{
							return pUser.get();
						}
					}
				}

				if (IAccount* pAccount = pMainService->GetAccountById(id))
				{
					return AddUser(*pAccount);
				}

				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[GamePlatform] Account '%s' not found by '%s'!", id.ToDebugString(), pMainService->GetName());

				return nullptr;
			}

			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[GamePlatform] Attempt to use account for service '%s' that is not the main one!", svcId.ToDebugString());

			return nullptr;
		}

		IUser* CPlugin::TryGetUser(IAccount& account) const
		{
			if (const IService* pMainService = GetMainService())
			{
				const ServiceIdentifier& svcId = account.GetIdentifier().Service();
				if (pMainService->GetServiceIdentifier() == svcId)
				{
					for (const std::unique_ptr<CUser>& pUser : m_users)
					{
						if (pUser->GetAccount(svcId) == &account)
						{
							return pUser.get();
						}
					}

					return AddUser(account);
				}

				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[GamePlatform] Attempt to use account for service '%s' that is not the main one!", svcId.ToDebugString());
			}

			return nullptr;
		}

		IUser* CPlugin::AddUser(IAccount& account) const
		{
			DynArray<IAccount*> connectedAccounts;

			CollectConnectedAccounts(account, connectedAccounts);

			for (size_t idx = 0; idx < connectedAccounts.size(); ++idx)
			{
				// This call can increase the size of connectedAccounts, thus it is very important to use
				// index-based looping and not caching the DynArray's size
				CollectConnectedAccounts(*connectedAccounts[idx], connectedAccounts);
			}

			m_users.emplace_back(stl::make_unique<CUser>(account, connectedAccounts));

			return m_users.back().get();
		}

		void CPlugin::CollectConnectedAccounts(IAccount& account, DynArray<IAccount*>& connectedAccounts) const
		{
			const DynArray<AccountIdentifier>& connAccIds = account.GetConnectedAccounts();
			for (const AccountIdentifier& accId : connAccIds)
			{
				if (const IService* pService = GetService(accId.Service()))
				{
					if (IAccount* pAccount = pService->GetAccountById(accId))
					{
						stl::push_back_unique(connectedAccounts, pAccount);
					}
				}
			}
		}

		bool CPlugin::GetAuthToken(string &tokenOut, int &issuerId)
		{
			if (IService* pMainService = GetMainService())
			{
				return pMainService->GetAuthToken(tokenOut, issuerId);
			}

			return false;
		}

		EConnectionStatus CPlugin::GetConnectionStatus() const
		{
			if (const IService* pMainService = GetMainService())
			{
				return pMainService->GetConnectionStatus();
			}

			return EConnectionStatus::Disconnected;
		}

		void CPlugin::OnShutdown(const ServiceIdentifier& serviceId)
		{
			UnregisterService(serviceId);
		}

		void CPlugin::RegisterMainService(IService& service)
		{
			CRY_ASSERT_MESSAGE(GetMainService() == nullptr, "Main service already registered");
			if (GetMainService() == nullptr)
			{
				m_services[0] = &service;
				m_services[0]->AddListener(*this);
			}
		}

		void CPlugin::RegisterService(IService& service)
		{
			if (stl::push_back_unique(m_services, &service))
			{
				service.AddListener(*this);
			}
		}

		Cry::GamePlatform::ServiceIdentifier CPlugin::GetMainServiceIdentifier() const
		{
			const IService* pMainService = GetMainService();
			return pMainService ? pMainService->GetServiceIdentifier() : NullServiceID;
		}

		void CPlugin::UnregisterService(const ServiceIdentifier& service)
		{
			if (GetMainServiceIdentifier() == service)
			{
				m_services[0] = nullptr;
			}
			else
			{
				stl::find_and_erase_if(m_services, [&service](IService* pSvc) { return pSvc && pSvc->GetServiceIdentifier() == service; });
			}
		}

		IService* CPlugin::GetMainService() const
		{
			// Constructor guarantees us we can access the first element
			return m_services[0];
		}

		IService* CPlugin::GetService(const ServiceIdentifier& svcId) const
		{
			for (IService* pService : m_services)
			{
				if (pService && pService->GetServiceIdentifier() == svcId)
				{
					return pService;
				}
			}

			return nullptr;
		}

		CRYREGISTER_SINGLETON_CLASS(CPlugin)
	}
}

