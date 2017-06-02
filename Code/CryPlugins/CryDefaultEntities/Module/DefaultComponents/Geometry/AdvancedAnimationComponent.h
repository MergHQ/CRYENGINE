#pragma once

#include <CryGame/IGameFramework.h>
#include <ICryMannequin.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Utils/SharedString.h>

#include <CryEntitySystem/IEntityComponent.h>

#include <CryCore/Containers/CryArray.h>

static void ReflectType(Schematyc::CTypeDesc<EMotionParamID>& desc)
{
	desc.SetGUID("{310D2CFB-8564-4C8E-A73B-BF4920D14136}"_cry_guid);
	desc.SetLabel("Animation Motion Parameter");
	desc.SetDescription("Modifier for animation to affect blend spaces");
	desc.SetDefaultValue(EMotionParamID::eMotionParamID_TravelSpeed);
	desc.AddConstant(EMotionParamID::eMotionParamID_TravelSpeed, "TravelSpeed", "Travel Speed");
	desc.AddConstant(EMotionParamID::eMotionParamID_TurnSpeed, "TurnSpeed", "Turn Speed");
	desc.AddConstant(EMotionParamID::eMotionParamID_TravelAngle, "TravelAngle", "Travel Angle");
	desc.AddConstant(EMotionParamID::eMotionParamID_TravelSlope, "TravelSlope", "Travel Slope");
	desc.AddConstant(EMotionParamID::eMotionParamID_TurnAngle, "TurnAngle", "Turn Angle");
	desc.AddConstant(EMotionParamID::eMotionParamID_TravelDist, "TravelDist", "Travel Distance");
	desc.AddConstant(EMotionParamID::eMotionParamID_StopLeg, "StopLeg", "Move2Idle Stop Leg");

	desc.AddConstant(EMotionParamID::eMotionParamID_BlendWeight, "BlendWeight", "Custom 1");
	desc.AddConstant(EMotionParamID::eMotionParamID_BlendWeight2, "BlendWeight2", "Custom 2");
	desc.AddConstant(EMotionParamID::eMotionParamID_BlendWeight3, "BlendWeight3", "Custom 3");
	desc.AddConstant(EMotionParamID::eMotionParamID_BlendWeight4, "BlendWeight4", "Custom 4");
	desc.AddConstant(EMotionParamID::eMotionParamID_BlendWeight5, "BlendWeight5", "Custom 5");
	desc.AddConstant(EMotionParamID::eMotionParamID_BlendWeight6, "BlendWeight6", "Custom 6");
	desc.AddConstant(EMotionParamID::eMotionParamID_BlendWeight7, "BlendWeight7", "Custom 7");
}

namespace Cry
{
namespace DefaultComponents
{
class CAdvancedAnimationComponent
	: public IEntityComponent
{
public:
	struct SDefaultScopeSettings
	{
		inline bool operator==(const SDefaultScopeSettings& rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }
		static void ReflectType(Schematyc::CTypeDesc<SDefaultScopeSettings>& desc)
		{
			desc.SetGUID("{1ADE6AA3-79F6-48F2-9D61-913BF88573E6}"_cry_guid);
			desc.SetLabel("Advanced Animation Component Default Scope");
			desc.SetDescription("Settings for the default Mannequin scope");
		}

		string m_controllerDefinitionPath;
		string m_contextName;
		string m_fragmentName;
	};

	CAdvancedAnimationComponent() = default;
	virtual ~CAdvancedAnimationComponent();

	// IEntityComponent
	virtual void Initialize() override
	{
		// Release previous controller and context
		SAFE_RELEASE(m_pActionController);
		m_pAnimationContext.reset();

		// Start by loading the character, return if it failed
		if (m_characterFile.value.size() > 0)
		{
			m_pEntity->LoadCharacter(GetOrMakeEntitySlotId(), m_characterFile.value);
			ICharacterInstance* pCharacter = m_pEntity->GetCharacter(GetEntitySlotId());
			if (pCharacter == nullptr)
			{
				return;
			}

			SetAnimationDrivenMotion(m_bAnimationDrivenMotion);
		}
		else
		{
			return;
		}

		if (m_defaultScopeSettings.m_controllerDefinitionPath.size() > 0 && m_databasePath.value.size() > 0)
		{
			// Now start loading the Mannequin data
			IMannequin& mannequinInterface = gEnv->pGameFramework->GetMannequinInterface();
			IAnimationDatabaseManager& animationDatabaseManager = mannequinInterface.GetAnimationDatabaseManager();

			// Load the Mannequin controller definition.
			// This is owned by the animation database manager
			const SControllerDef* pControllerDefinition = animationDatabaseManager.LoadControllerDef(m_defaultScopeSettings.m_controllerDefinitionPath);
			if (pControllerDefinition == nullptr)
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load controller definition %s!", m_defaultScopeSettings.m_controllerDefinitionPath.c_str());
				return;
			}

			// Load the animation database
			m_pDatabase = animationDatabaseManager.Load(m_databasePath.value);
			if (m_pDatabase == nullptr)
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to load animation database %s!", m_databasePath.value.c_str());
				return;
			}

			// Create a new animation context for the controller definition we loaded above
			m_pAnimationContext = stl::make_unique<SAnimationContext>(*pControllerDefinition);

			// Now create the controller that will be handling animation playback
			m_pActionController = mannequinInterface.CreateActionController(GetEntity(), *m_pAnimationContext);
			CRY_ASSERT(m_pActionController != nullptr);

			if (m_defaultScopeSettings.m_contextName.size() > 0)
			{
				ActivateContext(m_defaultScopeSettings.m_contextName);
			}

			if (m_defaultScopeSettings.m_fragmentName.size() > 0)
			{
				QueueFragment(m_defaultScopeSettings.m_fragmentName);
			}

			// Trigger call to GetEventMask
			m_pEntity->UpdateComponentEventMask(this);
		}
	}

	virtual void   Run(Schematyc::ESimulationMode simulationMode) override;

	virtual void   ProcessEvent(SEntityEvent& event) override;
	virtual uint64 GetEventMask() const override;
	// ~IEntityComponent

	static void ReflectType(Schematyc::CTypeDesc<CAdvancedAnimationComponent>& desc);

	template<typename T>
	static void ReflectMembers(Schematyc::CTypeDesc<T>& desc)
	{
		desc.AddMember(&CAdvancedAnimationComponent::m_characterFile, 'file', "Character", "Character", "Determines the character to load", "");
		desc.AddMember(&CAdvancedAnimationComponent::m_databasePath, 'dbpa', "DatabasePath", "Animation Database", "Path to the Mannequin .adb file", "");
		desc.AddMember(&CAdvancedAnimationComponent::m_defaultScopeSettings, 'defs', "DefaultScope", "Default Scope Context Name", "Default Mannequin scope settings", CAdvancedAnimationComponent::SDefaultScopeSettings());
		desc.AddMember(&CAdvancedAnimationComponent::m_bAnimationDrivenMotion, 'andr', "AnimDriven", "Animation Driven Motion", "Whether or not to use root motion in the animations", true);
	}

	static CryGUID& IID()
	{
		static CryGUID id = "{3CD5DDC5-EE15-437F-A997-79C2391537FE}"_cry_guid;
		return id;
	}

	void ActivateContext(const Schematyc::CSharedString& contextName)
	{
		ICharacterInstance* pCharacterInstance = m_pEntity->GetCharacter(GetEntitySlotId());
		CRY_ASSERT(pCharacterInstance != nullptr);
		if (pCharacterInstance == nullptr)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to activate animation context, character did not exist in entity %s", m_pEntity->GetName());
			return;
		}

		const TagID scopeContextId = m_pAnimationContext->controllerDef.m_scopeContexts.Find(contextName.c_str());
		if (scopeContextId == TAG_ID_INVALID)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to find scope context %s in controller definition.", contextName.c_str());
			return;
		}

		// Setting Scope contexts can happen at any time, and what entity or character instance we have bound to a particular scope context
		// can change during the lifetime of an action controller.
		m_pActionController->SetScopeContext(scopeContextId, *m_pEntity, pCharacterInstance, m_pDatabase);
	}

	// TODO: Expose resource selector for fragments
	void QueueFragment(const Schematyc::CSharedString& fragmentName)
	{
		TagID fragmentId = m_pAnimationContext->controllerDef.m_fragmentIDs.Find(fragmentName.c_str());
		if (fragmentId == TAG_ID_INVALID)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed to find Mannequin fragment %s in controller definition %s", fragmentName.c_str(), m_pAnimationContext->controllerDef.m_filename.c_str());
			return;
		}

		int priority = 0;
		m_fragments.emplace_back(new TAction<SAnimationContext>(priority, fragmentId, TAG_STATE_EMPTY, 0));

		// Queue the idle fragment to start playing immediately on next update
		m_pActionController->Queue(*m_fragments.back().get());
	}

	// TODO: Expose resource selector for tags
	void SetTag(const Schematyc::CSharedString& tagName, bool bSet)
	{
		SetTagWithId(GetTagId(tagName.c_str()), bSet);
	}

	void SetMotionParameter(EMotionParamID motionParam, float value)
	{
		ICharacterInstance* pCharacterInstance = GetCharacter();
		CRY_ASSERT(pCharacterInstance != nullptr);
		if (pCharacterInstance != nullptr)
		{
			pCharacterInstance->GetISkeletonAnim()->SetDesiredMotionParam(motionParam, value, 0.f);
		}
	}

	TagID GetTagId(const char* szTagName)
	{
		return m_pAnimationContext->state.GetDef().Find(szTagName);
	}

	void SetTagWithId(TagID id, bool bSet)
	{
		m_pAnimationContext->state.Set(id, bSet);
	}

	ICharacterInstance* GetCharacter() const { return m_pEntity->GetCharacter(GetEntitySlotId()); }

	// Enable / disable motion on entity being applied from animation on the root node
	void SetAnimationDrivenMotion(bool bSet)
	{
		m_bAnimationDrivenMotion = bSet;

		if (ICharacterInstance* pCharacterInstance = GetCharacter())
		{
			// Disable animation driven motion, note that the function takes the inverted parameter of what you would expect.
			pCharacterInstance->GetISkeletonAnim()->SetAnimationDrivenMotion(m_bAnimationDrivenMotion ? 0 : 1);
		}
	}
	bool         IsAnimationDrivenMotionEnabled() const { return m_bAnimationDrivenMotion; }

	virtual void SetCharacterFile(const char* szPath);
	const char*  SetCharacterFile() const                  { return m_characterFile.value.c_str(); }
	virtual void SetMannequinAnimationDatabaseFile(const char* szPath);
	const char*  GetMannequinAnimationDatabaseFile() const { return m_databasePath.value.c_str(); }

	virtual void SetControllerDefinitionFile(const char* szPath);
	const char*  GetControllerDefinitionFile() const { return m_defaultScopeSettings.m_controllerDefinitionPath.c_str(); }

	virtual void SetDefaultScopeContextName(const char* szName);
	const char*  GetDefaultScopeContextName() const { return m_defaultScopeSettings.m_fragmentName.c_str(); }
	virtual void SetDefaultFragmentName(const char* szName);
	const char*  GetDefaultFragmentName() const     { return m_defaultScopeSettings.m_fragmentName.c_str(); }

protected:
	bool                                      m_bAnimationDrivenMotion = true;

	Schematyc::CharacterFileName              m_characterFile;
	Schematyc::MannequinAnimationDatabasePath m_databasePath;

	SDefaultScopeSettings                     m_defaultScopeSettings;

	// Run-time info below
	IActionController*                 m_pActionController = nullptr;
	std::unique_ptr<SAnimationContext> m_pAnimationContext;
	const IAnimationDatabase*          m_pDatabase = nullptr;

	DynArray<_smart_ptr<IAction>>      m_fragments;
};
}
}
