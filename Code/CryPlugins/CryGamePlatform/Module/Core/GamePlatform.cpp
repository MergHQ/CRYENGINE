// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GamePlatform.h"

#include "User.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Predicate
		{
			struct SWithServiceId
			{
				explicit SWithServiceId(const ServiceIdentifier& svcId) : m_svcId(svcId) {}
				bool operator()(const IService* pService) const { return pService && pService->GetServiceIdentifier() == m_svcId; }
				bool operator()(const IAccount* pAccount) const { return pAccount->GetServiceIdentifier() == m_svcId; }

			private:
				ServiceIdentifier m_svcId;
			};

			struct SWithAccount
			{
				explicit SWithAccount(const IAccount& acc) : m_acc(acc) {}
				bool operator()(const std::unique_ptr<CUser>& pUser) const { return pUser->HasAccount(m_acc); }

			private:
				const IAccount& m_acc;
			};
		}

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
			if (IAccount* pMainAccount = GetMainLocalAccount())
			{
				return TryGetUser(*pMainAccount);
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
			
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[GamePlatform::TryGetUser] no user '%s' was found", id.ToDebugString());

			return nullptr;
		}

		IUser* CPlugin::TryGetUser(const AccountIdentifier& id) const
		{
			const ServiceIdentifier& svcId = id.Service();
			
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

			if (IAccount* pAccount = GetAccount(id))
			{
				return AddUser(*pAccount);
			}

			return nullptr;
		}

		IUser* CPlugin::TryGetUser(IAccount& account) const
		{
			const ServiceIdentifier& svcId = account.GetServiceIdentifier();
			if (GetMainServiceIdentifier() == svcId)
			{
				for (const std::unique_ptr<CUser>& pUser : m_users)
				{
					if (pUser->GetAccount(svcId) == &account)
					{
						return pUser.get();
					}
				}
			}

			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[GamePlatform] Attempt to use account for service '%s' that is not the main one!", svcId.ToDebugString());

			return AddUser(account);
		}

		IUser* CPlugin::AddUser(IAccount& account) const
		{
			DynArray<IAccount*> userAccounts;
			CollectConnectedAccounts(account, userAccounts);

			if (EnsureMainAccountFirst(userAccounts))
			{
				m_users.emplace_back(stl::make_unique<CUser>(userAccounts));
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[GamePlatform::AddUser] main account not found for '%s'", account.GetIdentifier().ToDebugString());
				return nullptr;
			}

			return m_users.back().get();
		}

		void CPlugin::CollectConnectedAccounts(IAccount& account, DynArray<IAccount*>& userAccounts) const
		{
			userAccounts.clear();
			userAccounts.push_back(&account); // Beware: we rely on userAccounts never being empty

			// Assume all local accounts are connected, even if no connection info is available
			if (account.IsLocal())
			{
				for (IService* pSvc : m_services)
				{
					IAccount* pSvcLocalAccount = pSvc ? pSvc->GetLocalAccount() : nullptr;

					if (pSvcLocalAccount && pSvcLocalAccount != &account)
					{
						userAccounts.push_back(pSvcLocalAccount);
					}
				}
			}

			for (size_t idx = 0; idx < userAccounts.size(); ++idx)
			{
				// This call can increase the size of userAccounts, thus it is very important to use
				// index-based looping and not caching the DynArray's size
				AddAccountConnections(*userAccounts[idx], userAccounts);
			}
		}

		void CPlugin::AddAccountConnections(const IAccount& account, DynArray<IAccount*>& userAccounts) const
		{
			const DynArray<AccountIdentifier>& connAccIds = account.GetConnectedAccounts();
			for (const AccountIdentifier& accId : connAccIds)
			{
				if (IAccount* pAccount = GetAccount(accId))
				{
					stl::push_back_unique(userAccounts, pAccount);
				}
			}
		}

		DynArray<IAccount*>::iterator CPlugin::FindMainAccount(DynArray<IAccount*>& userAccounts) const
		{
			return std::find_if(userAccounts.begin(), userAccounts.end(), Predicate::SWithServiceId(GetMainServiceIdentifier()));
		}

		bool CPlugin::EnsureMainAccountFirst(DynArray<IAccount*>& userAccounts) const
		{
			if (userAccounts.empty())
			{
				return false;
			}

			if (userAccounts[0]->GetServiceIdentifier() == GetMainServiceIdentifier())
			{
				return true;
			}

			DynArray<IAccount*>::iterator mainAccPos = FindMainAccount(userAccounts);
			if (mainAccPos == userAccounts.end())
			{
				return false;
			}

			std::swap(userAccounts[0], *mainAccPos);

			return true;
		}

		void CPlugin::AddOrUpdateUser(DynArray<IAccount*> userAccounts)
		{
			if (EnsureMainAccountFirst(userAccounts))
			{
				IAccount* pMainAcc = userAccounts[0];
				
				auto userPos = std::find_if(m_users.begin(), m_users.end(), Predicate::SWithAccount(*pMainAcc));
				if (userPos != m_users.end())
				{
					(*userPos)->SetAccounts(userAccounts);
				}
				else
				{
					AddUser(*pMainAcc);
				}
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[GamePlatform::AddOrUpdateUser] main account not found for '%s'", userAccounts[0]->GetIdentifier().ToDebugString());
			}
		}

		IAccount* CPlugin::GetAccount(const AccountIdentifier& id) const
		{
			if (IService* pSvc = GetService(id.Service()))
			{
				return pSvc->GetAccountById(id);
			}

			return nullptr;
		}
		
		IAccount* CPlugin::GetMainLocalAccount() const
		{
			if (IService* pMainSvc = GetMainService())
			{
				return pMainSvc->GetLocalAccount();
			}

			return nullptr;
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

		void CPlugin::OnAccountAdded(IAccount& account)
		{
			DynArray<IAccount*> userAccounts;
			CollectConnectedAccounts(account, userAccounts);

			AddOrUpdateUser(std::move(userAccounts));
		}

		void CPlugin::OnAccountRemoved(IAccount& account)
		{
			auto usrPos = std::find_if(m_users.begin(), m_users.end(), Predicate::SWithAccount(account));
			if (usrPos != m_users.end())
			{
				CUser* const pUser = usrPos->get();

				if (account.GetServiceIdentifier() == GetMainServiceIdentifier())
				{
					stl::find_and_erase(m_friends, pUser);
					m_users.erase(usrPos);
				}
				else
				{
					pUser->RemoveAccount(account);
				}
			}
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
				stl::find_and_erase_if(m_services, Predicate::SWithServiceId(service));
			}
		}

		IService* CPlugin::GetMainService() const
		{
			// Constructor guarantees us we can access the first element
			return m_services[0];
		}

		IService* CPlugin::GetService(const ServiceIdentifier& svcId) const
		{
			auto svcPos = std::find_if(m_services.begin(), m_services.end(), Predicate::SWithServiceId(svcId));
			return svcPos != m_services.end() ? *svcPos : nullptr;
		}

		CRYREGISTER_SINGLETON_CLASS(CPlugin)
	}
}

