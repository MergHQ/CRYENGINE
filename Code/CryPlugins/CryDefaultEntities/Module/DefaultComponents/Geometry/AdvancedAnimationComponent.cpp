#include "StdAfx.h"
#include "AdvancedAnimationComponent.h"

#include <Cry3DEngine/IRenderNode.h>


namespace Cry
{
	namespace DefaultComponents
	{
		void CAdvancedAnimationComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
		{
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAdvancedAnimationComponent::ActivateContext, "{2DA18E2A-75F6-4EEE-9C7C-60ABF275555E}"_cry_guid, "ActivateContext");
				pFunction->SetDescription("Activates a Mannequin context");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'cont', "Context Name");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAdvancedAnimationComponent::QueueFragment, "{6F5AA73B-B8F9-4392-9651-468DEB342D91}"_cry_guid, "QueueFragment");
				pFunction->SetDescription("Queues a Mannequin fragment for playback");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'frag', "Fragment Name");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAdvancedAnimationComponent::SetTag, "{04768686-96DF-4F82-8CC8-109522A9F2E0}"_cry_guid, "SetTag");
				pFunction->SetDescription("Sets a Mannequin tag's state to true or false");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'tagn', "Tag Name");
				pFunction->BindInput(2, 'set', "Set");
				componentScope.Register(pFunction);
			}
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CAdvancedAnimationComponent::SetMotionParameter, "{67B8D89B-D56B-443E-B88B-BFEBEE52AFB7}"_cry_guid, "SetMotionParameter");
				pFunction->SetDescription("Sets a motion parameter to affect a blend space");
				pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
				pFunction->BindInput(1, 'mtnp', "Motion Parameter");
				pFunction->BindInput(2, 'val', "Value");
				componentScope.Register(pFunction);
			}
		}

		void CAdvancedAnimationComponent::ReflectType(Schematyc::CTypeDesc<CAdvancedAnimationComponent>& desc)
		{
			desc.SetGUID(CAdvancedAnimationComponent::IID());
			desc.SetEditorCategory("Geometry");
			desc.SetLabel("Advanced Animations");
			desc.SetDescription("Exposes playback of more advanced animations going through the Mannequin systems");
			desc.SetIcon("icons:General/Mannequin.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			CAdvancedAnimationComponent::ReflectMembers(desc);
		}

		inline bool Serialize(Serialization::IArchive& archive, CAdvancedAnimationComponent::SDefaultScopeSettings& defaultSettings, const char* szName, const char* szLabel)
		{
			archive(Serialization::MannequinControllerDefinitionPath(defaultSettings.m_controllerDefinitionPath), "ControllerDefPath", "Controller Definition");
			archive.doc("Path to the Mannequin controller definition");

			std::shared_ptr<Serialization::SMannequinControllerDefResourceParams> pParams;

			// Load controller definition for the context and fragment selectors
			if (archive.isEdit())
			{
				pParams = std::make_shared<Serialization::SMannequinControllerDefResourceParams>();

				IAnimationDatabaseManager &animationDatabaseManager = gEnv->pGameFramework->GetMannequinInterface().GetAnimationDatabaseManager();
				if (defaultSettings.m_controllerDefinitionPath.size() > 0)
				{
					pParams->pControllerDef = animationDatabaseManager.LoadControllerDef(defaultSettings.m_controllerDefinitionPath);
				}
			}

			archive(Serialization::MannequinScopeContextName(defaultSettings.m_contextName, pParams), "DefaultScope", "Default Scope Context Name");
			archive.doc("The Mannequin scope context to activate by default");

			archive(Serialization::MannequinFragmentName(defaultSettings.m_fragmentName, pParams), "DefaultFragment", "Default Fragment Name");
			archive.doc("The fragment to play by default");

			return true;
		}

		CAdvancedAnimationComponent::~CAdvancedAnimationComponent()
		{
			SAFE_RELEASE(m_pActionController);
		}

		void CAdvancedAnimationComponent::Initialize()
		{
			LoadFromDisk();

			ResetCharacter();
		}

		void CAdvancedAnimationComponent::ProcessEvent(SEntityEvent& event)
		{
			if (event.event == ENTITY_EVENT_UPDATE)
			{
				SEntityUpdateContext* pCtx = (SEntityUpdateContext*)event.nParam[0];

				if (m_pActionController != nullptr)
				{
					m_pActionController->Update(pCtx->fFrameTime);
				}
			}
			else if (event.event == ENTITY_EVENT_ANIM_EVENT)
			{
				if (m_pActionController != nullptr)
				{
					const AnimEventInstance *pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
					ICharacterInstance *pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);

					m_pActionController->OnAnimationEvent(pCharacter, *pAnimEvent);
				}
			}
			else if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
			{
				LoadFromDisk();

				ResetCharacter();
			}
		}

		uint64 CAdvancedAnimationComponent::GetEventMask() const
		{
			uint64 bitFlags = BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);

			if (m_pActionController != nullptr)
			{
				bitFlags |= BIT64(ENTITY_EVENT_UPDATE) | BIT64(ENTITY_EVENT_ANIM_EVENT);
			}

			return bitFlags;
		}

		void CAdvancedAnimationComponent::SetCharacterFile(const char* szPath)
		{
			m_characterFile = szPath;
		}

		void CAdvancedAnimationComponent::SetMannequinAnimationDatabaseFile(const char* szPath)
		{
			m_databasePath = szPath;
		}

		void CAdvancedAnimationComponent::SetControllerDefinitionFile(const char* szPath)
		{
			m_defaultScopeSettings.m_controllerDefinitionPath = szPath;
		}

		void CAdvancedAnimationComponent::SetDefaultScopeContextName(const char* szName)
		{
			m_defaultScopeSettings.m_contextName = szName;
		}

		void CAdvancedAnimationComponent::SetDefaultFragmentName(const char* szName)
		{
			m_defaultScopeSettings.m_fragmentName = szName;
		}
	}
}