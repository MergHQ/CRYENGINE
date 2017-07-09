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
			enum class EKeyboardInputId {};
			enum class EMouseInputId {};
			enum class EXboxInputId {};
			enum class EPS4InputId {};

			CInputComponent() = default;
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

			virtual void BindKeyboardAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EKeyboardInputId keyId, bool bOnPress = true, bool bOnRelease = true);
			virtual void BindMouseAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EMouseInputId keyId, bool bOnPress = true, bool bOnRelease = true);
			virtual void BindXboxAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EXboxInputId keyId, bool bOnPress = true, bool bOnRelease = true);
			virtual void BindPS4Action(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EPS4InputId keyId, bool bOnPress = true, bool bOnRelease = true);
			virtual void BindAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EActionInputDevice device, EKeyId keyId, bool bOnPress = true, bool bOnRelease = true);

			static void ReflectType(Schematyc::CTypeDesc<CInputComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{183E3C84-2E08-4506-95FA-B362F7E735E9}"_cry_guid;
				return id;
			}

			virtual void RegisterSchematycAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name);

		protected:
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
		};
	}
}