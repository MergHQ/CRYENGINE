#pragma once

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/IObject.h>
#include <CrySchematyc/Utils/SharedString.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

#include <CryGame/IGameFramework.h>
#include <IActionMapManager.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CInputComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;
			// ~IEntityComponent

		public:
			CInputComponent();
			virtual ~CInputComponent() {}

			typedef std::function<void(int activationMode, float value)> TActionCallback;

			virtual void RegisterAction(const char* szGroupName, const char* szName, TActionCallback callback)
			{
				IActionMapManager *pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
				IActionMap* pActionMap = pActionMapManager->GetActionMap(szGroupName);
				if (pActionMap == nullptr)
				{
					pActionMap = pActionMapManager->CreateActionMap(szGroupName);

					m_groups.emplace_back(szGroupName);
				}
				else
				{
					bool bFound = false;

					for (const SGroup& group : m_groups)
					{
						if (!strcmp(group.m_szName, szGroupName))
						{
							bFound = true;
							break;
						}
					}

					if (!bFound)
					{
						m_groups.emplace_back(szGroupName);
					}
				}

				IActionMapAction* pAction = pActionMap->GetAction(ActionId(szName));
				if (pAction != nullptr)
				{
					pActionMap->RemoveAction(ActionId(szName));
				}
				
				pActionMap->CreateAction(ActionId(szName));

				for (SGroup& group : m_groups)
				{
					if (!strcmp(group.m_szName, szGroupName))
					{
						group.m_actions.emplace_back(SGroup::SAction{ szName, callback });
					}
				}
			}

			virtual void BindAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EActionInputDevice inputDevice, EKeyId keyId, bool bOnPress = true, bool bOnRelease = true)
			{
				IActionMapManager *pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
				IActionMap* pActionMap = pActionMapManager->GetActionMap(groupName.c_str());
				if (pActionMap == nullptr)
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Tried to bind input component action %s in group %s without registering it first!", name.c_str(), groupName.c_str());
					return;
				}

				IActionMapAction* pAction = pActionMap->GetAction(ActionId(name.c_str()));
				if (pAction == nullptr)
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Tried to bind input component action %s in group %s without registering it first!", name.c_str(), groupName.c_str());
					return;
				}

				SActionInput input;
				input.input = input.defaultInput = m_pImplementation->GetKeyId(keyId);
				input.inputDevice = inputDevice;

				if (bOnPress)
				{
					input.activationMode |= eAAM_OnPress;
				}
				if (bOnRelease)
				{
					input.activationMode |= eAAM_OnRelease;
				}

				pActionMap->AddAndBindActionInput(ActionId(name.c_str()), input);

				pActionMapManager->EnableActionMap(groupName.c_str(), true);

				if (IActionMap *pActionMap = pActionMapManager->GetActionMap(groupName.c_str()))
				{
					pActionMap->SetActionListener(GetEntityId());
				}
			}

			static void ReflectType(Schematyc::CTypeDesc<CInputComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{183E3C84-2E08-4506-95FA-B362F7E735E9}"_cry_guid;
				return id;
			}

			virtual void RegisterSchematycAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name);

		protected:
			struct IImplementation
			{
				virtual const char* GetKeyId(EKeyId id) = 0;
			};

			struct SGroup final : public IActionListener
			{
				struct SAction
				{
					const char* szName;
					TActionCallback callback;
				};

				SGroup(const char* szName)
					: m_szName(szName)
				{
					gEnv->pGameFramework->GetIActionMapManager()->AddExtraActionListener(this);
				}

				virtual ~SGroup()
				{
					gEnv->pGameFramework->GetIActionMapManager()->RemoveExtraActionListener(this);
				}

				// IActionListener
				virtual void OnAction(const ActionId &actionId, int activationMode, float value) override
				{
					for (const SAction& action : m_actions)
					{
						if (!strcmp(action.szName, actionId.c_str()))
						{
							action.callback(activationMode, value);
							break;
						}
					}
				}
				// ~IActionListener

				const char* m_szName;
				DynArray<SAction> m_actions;
			};

			DynArray<SGroup> m_groups;
			std::unique_ptr<IImplementation> m_pImplementation;
		};
	}
}