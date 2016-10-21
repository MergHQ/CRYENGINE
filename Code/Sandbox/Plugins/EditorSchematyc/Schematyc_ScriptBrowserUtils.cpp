// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_ScriptBrowserUtils.h"

#include <Schematyc/Schematyc_FundamentalTypes.h>
#include <Schematyc/Env/Elements/Schematyc_IEnvAction.h>
#include <Schematyc/Env/Elements/Schematyc_IEnvClass.h>
#include <Schematyc/Env/Elements/Schematyc_IEnvComponent.h>
#include <Schematyc/Env/Elements/Schematyc_IEnvDataType.h>
#include <Schematyc/Env/Elements/Schematyc_IEnvSignal.h>
#include <Schematyc/Script/Schematyc_IScript.h>
#include <Schematyc/Script/Schematyc_IScriptRegistry.h>
#include <Schematyc/Script/Schematyc_IScriptView.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptActionInstance.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptClass.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptEnum.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptInterface.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptInterfaceFunction.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptInterfaceImpl.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptInterfaceTask.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptModule.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptSignal.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptSignalReceiver.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptState.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptStateMachine.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptStruct.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptTimer.h>
#include <Schematyc/Utils/Schematyc_StackString.h>

#include "Schematyc_CustomMessageBox.h"
#include "Schematyc_PluginUtils.h"
#include "Schematyc_QuickSearchDlg.h"
#include "Schematyc_ReportWidget.h"

namespace Schematyc
{
namespace ScriptBrowserUtils
{
// #SchematycTODO : Move icon declarations to separate file?
static const char* g_szScriptElementIcon = "editor/icons/cryschematyc/script_element.png";
static const char* g_szScriptModuleIcon = "editor/icons/cryschematyc/script_module.png";
static const char* g_szScriptImportIcon = "editor/icons/cryschematyc/script_import.png";
static const char* g_szScriptEnumIcon = "editor/icons/cryschematyc/script_enum.png";
static const char* g_szScriptStructIcon = "editor/icons/cryschematyc/script_struct.png";
static const char* g_szScriptSignalIcon = "editor/icons/cryschematyc/script_signal.png";
static const char* g_szScriptConstructorIcon = "editor/icons/cryschematyc/script_constructor.png";
static const char* g_szScriptDestructorIcon = "editor/icons/cryschematyc/script_destructor.png";
static const char* g_szScriptFunctionIcon = "editor/icons/cryschematyc/script_function.png";
static const char* g_szScriptInterfaceIcon = "editor/icons/cryschematyc/script_interface.png";
static const char* g_szScriptInterfaceFunctionIcon = "editor/icons/cryschematyc/script_interface_function.png";
static const char* g_szScriptInterfaceTaskIcon = "editor/icons/cryschematyc/script_interface_task.png";
static const char* g_szScriptStateMachineIcon = "editor/icons/cryschematyc/script_state_machine.png";
static const char* g_szScriptStateIcon = "editor/icons/cryschematyc/script_state.png";
static const char* g_szScriptVariableIcon = "editor/icons/cryschematyc/script_variable.png";
static const char* g_szScriptPropertyIcon = "editor/icons/cryschematyc/script_property.png";
static const char* g_szScriptTimerIcon = "editor/icons/cryschematyc/script_timer.png";
static const char* g_szScriptSignalReceiverIcon = "editor/icons/cryschematyc/script_signal_receiver.png";
static const char* g_szScriptComponentIcon = "editor/icons/cryschematyc/script_component.png";
static const char* g_szScriptActionIcon = "editor/icons/cryschematyc/script_action.png";

struct SImport
{
	inline SImport(const char* _szName, const SGUID& _moduleGUID)
		: name(_szName)
		, moduleGUID(_moduleGUID)
	{}

	string name;
	SGUID  moduleGUID;
};

class CImportQuickSearchOptions : public IQuickSearchOptions
{
private:

	typedef std::vector<SGUID>   Exclusions;
	typedef std::vector<SImport> Imports;

public:

	inline CImportQuickSearchOptions(const IScriptElement* pScriptScope)
	{
		if (pScriptScope)
		{
			Exclusions exclusions;
			exclusions.reserve(50);

			IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptScope->GetGUID());

			// #SchematycTODO : Collect exclusions.

			auto visitScriptModule = [this, &pScriptView](const IScriptModule& scriptModule) -> EVisitStatus
			{
				CStackString name;
				pScriptView->QualifyName(scriptModule, EDomainQualifier::Global, name);
				m_imports.push_back(SImport(name.c_str(), scriptModule.GetGUID()));
				return EVisitStatus::Continue;
			};
			pScriptView->VisitScriptModules(ScriptModuleConstVisitor::FromLambda(visitScriptModule));
		}
	}

	// IQuickSearchOptions

	virtual uint32 GetCount() const override
	{
		return m_imports.size();
	}

	virtual const char* GetLabel(uint32 optionIdx) const override
	{
		return optionIdx < m_imports.size() ? m_imports[optionIdx].name.c_str() : "";
	}

	virtual const char* GetDescription(uint32 optionIdx) const override
	{
		return nullptr;
	}

	virtual const char* GetWikiLink(uint32 optionIdx) const override
	{
		return nullptr;
	}

	virtual const char* GetHeader() const override
	{
		return "Module";
	}

	virtual const char* GetDelimiter() const override
	{
		return "::";
	}

	// ~IQuickSearchOptions

	inline const SImport* GetImport(uint32 importIdx) const
	{
		return importIdx < m_imports.size() ? &m_imports[importIdx] : nullptr;
	}

private:

	Imports m_imports;
};

struct SBase
{
	inline SBase() {}

	inline SBase(const char* _szName, const char* _szDescription, const SElementId& _id)
		: name(_szName)
		, description(_szDescription)
		, id(_id)
	{}

	string     name;
	string     description;
	SElementId id;
};

typedef std::vector<SBase> Bases;

class CBaseQuickSearchOptions : public IQuickSearchOptions
{
public:

	inline CBaseQuickSearchOptions(const IScriptElement* pScriptScope)
	{
		if (pScriptScope)
		{
			m_bases.reserve(32);

			IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptScope->GetGUID());

			auto visitEnvClass = [this, &pScriptView](const IEnvClass& envClass) -> EVisitStatus
			{
				CStackString name;
				pScriptView->QualifyName(envClass, name);
				name.insert(0, "Class::");
				m_bases.push_back(SBase(name.c_str(), /*envClass.GetDescription()*/ "", SElementId(EDomain::Env, envClass.GetGUID())));
				return EVisitStatus::Continue;
			};
			pScriptView->VisitEnvClasses(EnvClassConstVisitor::FromLambda(visitEnvClass));

			/*
			auto visitScriptClass = [this, &pScriptView](const IScriptClass& scriptClass)
			{
				CStackString name;
				pScriptView->QualifyName(scriptClass, EDomainQualifier::Local, name);
				name.insert(0, "Class::");
				m_bases.push_back(SBase(name.c_str(), scriptClass.GetDescription(), SElementId(EDomain::Script, scriptClass.GetGUID())));
				return EVisitStatus::Continue;
			};
			pScriptView->VisitScriptClasses(ScriptClassConstVisitor::FromLambda(visitScriptClass), EDomainScope::Local);
			*/
		}
	}

	// IQuickSearchOptions

	virtual uint32 GetCount() const override
	{
		return m_bases.size();
	}

	virtual const char* GetLabel(uint32 optionIdx) const override
	{
		return optionIdx < m_bases.size() ? m_bases[optionIdx].name.c_str() : "";
	}

	virtual const char* GetDescription(uint32 optionIdx) const override
	{
		return optionIdx < m_bases.size() ? m_bases[optionIdx].description.c_str() : "";
	}

	virtual const char* GetWikiLink(uint32 optionIdx) const override
	{
		return nullptr;
	}

	virtual const char* GetHeader() const override
	{
		return "Base";
	}

	virtual const char* GetDelimiter() const override
	{
		return "::";
	}

	// ~IQuickSearchOptions

	inline const SBase* GetBase(uint32 baseIdx) const
	{
		return baseIdx < m_bases.size() ? &m_bases[baseIdx] : nullptr;
	}

private:

	Bases m_bases;
};

struct SType
{
	inline SType() {}

	inline SType(const char* _szName, const SElementId& _typeId)
		: name(_szName)
		, typeId(_typeId)
	{}

	string     name;
	SElementId typeId;
};

class CTypeQuickSearchOptions : public IQuickSearchOptions
{
private:

	typedef std::vector<SType> Types;

public:

	inline CTypeQuickSearchOptions(const IScriptElement* pScriptScope)
	{
		if (pScriptScope)
		{
			m_types.reserve(50);

			IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptScope->GetGUID());

			auto visitEnvDataType = [this, &pScriptView](const IEnvDataType& envDataType) -> EVisitStatus
			{
				CStackString name;
				pScriptView->QualifyName(envDataType, name);
				m_types.push_back(SType(name.c_str(), SElementId(EDomain::Env, envDataType.GetGUID())));
				return EVisitStatus::Continue;
			};
			pScriptView->VisitEnvDataTypes(EnvDataTypeVisitor::FromLambda(visitEnvDataType));

			auto visitScriptEnum = [this, &pScriptView](const IScriptEnum& scriptEnum)
			{
				CStackString name;
				pScriptView->QualifyName(scriptEnum, EDomainQualifier::Global, name);
				m_types.push_back(SType(name.c_str(), SElementId(EDomain::Script, scriptEnum.GetGUID())));
				return EVisitStatus::Continue;
			};
			pScriptView->VisitScriptEnums(ScriptEnumConstVisitor::FromLambda(visitScriptEnum), EDomainScope::Local);

			auto visitScriptStruct = [this, &pScriptView](const IScriptStruct& scriptStruct)
			{
				CStackString name;
				pScriptView->QualifyName(scriptStruct, EDomainQualifier::Global, name);
				m_types.push_back(SType(name.c_str(), SElementId(EDomain::Script, scriptStruct.GetGUID())));
				return EVisitStatus::Continue;
			};
			//pScriptView->VisitScriptStructs(ScriptEnumConstVisitor::FromLambda(visitScriptStruct), EDomainScope::Local);
		}
	}

	// IQuickSearchOptions

	virtual uint32 GetCount() const override
	{
		return m_types.size();
	}

	virtual const char* GetLabel(uint32 optionIdx) const override
	{
		return optionIdx < m_types.size() ? m_types[optionIdx].name.c_str() : "";
	}

	virtual const char* GetDescription(uint32 optionIdx) const override
	{
		return nullptr;
	}

	virtual const char* GetWikiLink(uint32 optionIdx) const override
	{
		return nullptr;
	}

	virtual const char* GetHeader() const override
	{
		return "Type";
	}

	virtual const char* GetDelimiter() const override
	{
		return "::";
	}

	// ~IQuickSearchOptions

	inline const SType* GetType(uint32 typeIdx) const
	{
		return typeIdx < m_types.size() ? &m_types[typeIdx] : nullptr;
	}

private:

	Types m_types;
};

struct SSignalReceiver
{
	inline SSignalReceiver(EScriptSignalReceiverType _type, const SGUID& _guid, const char* _szLabel, const char* szDescription, const char* szWikiLink = nullptr)
		: type(_type)
		, guid(_guid)
		, label(_szLabel)
		, description(szDescription)
		, wikiLink(szWikiLink)
	{}

	EScriptSignalReceiverType type;
	SGUID                     guid;
	string                    label;
	string                    description;
	string                    wikiLink;
};

struct SInterface
{
	inline SInterface() {}

	inline SInterface(EDomain _domain, const SGUID& _guid, const char* _szName, const char* _szFullName, const char* szDescription, const char* szWikiLink)
		: domain(_domain)
		, guid(_guid)
		, name(_szName)
		, fullName(_szFullName)
		, description(szDescription)
		, wikiLink(szWikiLink)
	{}

	EDomain domain;
	SGUID   guid;
	string  name;
	string  fullName;
	string  description;
	string  wikiLink;
};

class CInterfaceQuickSearchOptions : public IQuickSearchOptions
{
private:

	typedef std::vector<SInterface> Interfaces;

public:

	inline CInterfaceQuickSearchOptions(const IScriptElement* pScriptScope)
	{
		if (pScriptScope)
		{
			m_interfaces.reserve(100);

			IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptScope->GetGUID());
			auto visitEnvInterface = [this, &pScriptView](const IEnvInterface& envInterface) -> EVisitStatus
			{
				CStackString fullName;
				pScriptView->QualifyName(envInterface, fullName);
				m_interfaces.push_back(SInterface(EDomain::Env, envInterface.GetGUID(), envInterface.GetName(), fullName.c_str(), envInterface.GetDescription(), envInterface.GetWikiLink()));
				return EVisitStatus::Continue;
			};
			pScriptView->VisitEnvInterfaces(EnvInterfaceConstVisitor::FromLambda(visitEnvInterface));
		}
	}

	// IQuickSearchOptions

	virtual uint32 GetCount() const override
	{
		return m_interfaces.size();
	}

	virtual const char* GetLabel(uint32 optionIdx) const override
	{
		return optionIdx < m_interfaces.size() ? m_interfaces[optionIdx].fullName.c_str() : "";
	}

	virtual const char* GetDescription(uint32 optionIdx) const override
	{
		return optionIdx < m_interfaces.size() ? m_interfaces[optionIdx].description.c_str() : "";
	}

	virtual const char* GetWikiLink(uint32 optionIdx) const override
	{
		return optionIdx < m_interfaces.size() ? m_interfaces[optionIdx].wikiLink.c_str() : "";
	}

	virtual const char* GetHeader() const override
	{
		return "Interface";
	}

	virtual const char* GetDelimiter() const override
	{
		return "::";
	}

	// ~IQuickSearchOptions

	inline const SInterface* GetInterface(uint32 interfaceIdx) const
	{
		return interfaceIdx < m_interfaces.size() ? &m_interfaces[interfaceIdx] : nullptr;
	}

private:

	Interfaces m_interfaces;
};

struct SComponent
{
	inline SComponent() {}

	inline SComponent(const SGUID& _typeGUID, const char* _szName, const char* _szFullName, const char* szDescription, const char* szWikiLink)
		: typeGUID(_typeGUID)
		, name(_szName)
		, fullName(_szFullName)
		, description(szDescription)
		, wikiLink(szWikiLink)
	{}

	SGUID  typeGUID;
	string name;
	string fullName;
	string description;
	string wikiLink;
};

class CComponentQuickSearchOptions : public IQuickSearchOptions
{
private:

	typedef std::vector<SComponent> Components;

public:

	inline CComponentQuickSearchOptions(const IScriptElement* pScriptScope)
	{
		if (pScriptScope)
		{
			m_components.reserve(100);

			bool bAttach = false;

			const IScriptComponentInstance* pScriptComponentInstance = DynamicCast<IScriptComponentInstance>(pScriptScope);
			if (pScriptComponentInstance)
			{
				const IEnvComponent* pEnvComponent = GetSchematycFramework().GetEnvRegistry().GetComponent(pScriptComponentInstance->GetComponentTypeGUID());
				if (pEnvComponent)
				{
					if (pEnvComponent->GetFlags().Check(EEnvComponentFlags::Socket))
					{
						bAttach = true;
					}
					else
					{
						return;
					}
				}
			}

			IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptScope->GetGUID());
			auto visitEnvComponent = [this, bAttach, &pScriptView](const IEnvComponent& envComponent) -> EVisitStatus
			{
				if (!bAttach || envComponent.GetFlags().Check(EEnvComponentFlags::Attach))
				{
					CStackString fullName;
					pScriptView->QualifyName(envComponent, fullName);
					m_components.push_back(SComponent(envComponent.GetGUID(), envComponent.GetName(), fullName.c_str(), envComponent.GetDescription(), envComponent.GetWikiLink()));
				}
				return EVisitStatus::Continue;
			};
			pScriptView->VisitEnvComponents(EnvComponentConstVisitor::FromLambda(visitEnvComponent));

			auto compareComponents = [](const SComponent& lhs, const SComponent& rhs)
			{
				return lhs.fullName < rhs.fullName;
			};
			std::sort(m_components.begin(), m_components.end(), compareComponents);
		}
	}

	// IQuickSearchOptions

	virtual uint32 GetCount() const override
	{
		return m_components.size();
	}

	virtual const char* GetLabel(uint32 optionIdx) const override
	{
		return optionIdx < m_components.size() ? m_components[optionIdx].fullName.c_str() : "";
	}

	virtual const char* GetDescription(uint32 optionIdx) const override
	{
		return optionIdx < m_components.size() ? m_components[optionIdx].description.c_str() : "";
	}

	virtual const char* GetWikiLink(uint32 optionIdx) const override
	{
		return optionIdx < m_components.size() ? m_components[optionIdx].wikiLink.c_str() : "";
	}

	virtual const char* GetHeader() const override
	{
		return "Component";
	}

	virtual const char* GetDelimiter() const override
	{
		return "::";
	}

	// ~IQuickSearchOptions

	inline const SComponent* GetComponent(uint32 componentIdx) const
	{
		return componentIdx < m_components.size() ? &m_components[componentIdx] : nullptr;
	}

private:

	Components m_components;
};

struct SAction
{
	inline SAction() {}

	inline SAction(const SGUID& _guid, const SGUID& _componentInstanceGUID, const char* _szName, const char* _szFullName, const char* _szDescription, const char* _szWikiLink)
		: guid(_guid)
		, componentInstanceGUID(_componentInstanceGUID)
		, name(_szName)
		, fullName(_szFullName)
		, description(_szDescription)
		, wikiLink(_szWikiLink)
	{}

	SGUID  guid;
	SGUID  componentInstanceGUID;
	string name;
	string fullName;
	string description;
	string wikiLink;
};

class CActionQuickSearchOptions : public IQuickSearchOptions
{
private:

	typedef std::vector<const IScriptComponentInstance*> ScriptComponentInstances;
	typedef std::vector<SAction>                         Actions;

public:

	inline CActionQuickSearchOptions(const IScriptElement* pScriptScope)
	{
		if (pScriptScope)
		{
			m_actions.reserve(100);

			IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptScope->GetGUID());

			ScriptComponentInstances scriptComponentInstances;
			CollectScriptComponentInstances(scriptComponentInstances, *pScriptView);

			auto visitEnvAction = [this, &pScriptView, &scriptComponentInstances](const IEnvAction& envAction) -> EVisitStatus
			{
				const IEnvElement* pParent = envAction.GetParent();
				if (pParent && (pParent->GetElementType() == EEnvElementType::Component))
				{
					const SGUID componentTypeGUID = pParent->GetGUID();
					for (const IScriptComponentInstance* pScriptComponentInstance : scriptComponentInstances)
					{
						if (pScriptComponentInstance->GetComponentTypeGUID() == componentTypeGUID)
						{
							CStackString fullName;
							pScriptView->QualifyName(envAction, fullName);
							m_actions.push_back(SAction(envAction.GetGUID(), pScriptComponentInstance->GetGUID(), envAction.GetName(), fullName.c_str(), envAction.GetDescription(), envAction.GetWikiLink()));
						}
					}
				}
				else
				{
					CStackString fullName;
					pScriptView->QualifyName(envAction, fullName);
					m_actions.push_back(SAction(envAction.GetGUID(), SGUID(), envAction.GetName(), fullName.c_str(), envAction.GetDescription(), envAction.GetWikiLink()));
				}
				return EVisitStatus::Continue;
			};
			pScriptView->VisitEnvActions(EnvActionConstVisitor::FromLambda(visitEnvAction));

			auto actionSortPredicate = [](const SAction& lhs, const SAction& rhs)
			{
				return lhs.fullName < rhs.fullName;
			};
			std::sort(m_actions.begin(), m_actions.end(), actionSortPredicate);
		}
	}

	// IQuickSearchOptions

	virtual uint32 GetCount() const override
	{
		return m_actions.size();
	}

	virtual const char* GetLabel(uint32 optionIdx) const override
	{
		return optionIdx < m_actions.size() ? m_actions[optionIdx].fullName.c_str() : "";
	}

	virtual const char* GetDescription(uint32 optionIdx) const override
	{
		return optionIdx < m_actions.size() ? m_actions[optionIdx].description.c_str() : "";
	}

	virtual const char* GetWikiLink(uint32 optionIdx) const override
	{
		return optionIdx < m_actions.size() ? m_actions[optionIdx].wikiLink.c_str() : "";
	}

	virtual const char* GetHeader() const override
	{
		return "Action";
	}

	virtual const char* GetDelimiter() const override
	{
		return "::";
	}

	// ~IQuickSearchOptions

	inline const SAction* GetAction(uint32 actionIdx) const
	{
		return actionIdx < m_actions.size() ? &m_actions[actionIdx] : nullptr;
	}

private:

	inline void CollectScriptComponentInstances(ScriptComponentInstances& scriptComponentInstances, const IScriptView& scriptView)
	{
		scriptComponentInstances.reserve(100);

		auto visitScriptComponentInstance = [this, &scriptComponentInstances](const IScriptComponentInstance& scriptComponentInstance) -> EVisitStatus
		{
			scriptComponentInstances.push_back(&scriptComponentInstance);
			return EVisitStatus::Continue;
		};
		scriptView.VisitScriptComponentInstances(ScriptComponentInstanceConstVisitor::FromLambda(visitScriptComponentInstance), EDomainScope::Local);
	}

	/*EVisitStatus VisitAction(const IEnvAction& envAction)
	   {
	   const SGUID& componentGUID = envAction.GetComponentGUID();
	   if(componentGUID)
	   {
	    const IScriptClass* pScriptClass = DocUtils::FindOwnerClass(m_scriptFile, m_scopeGUID);
	    if(pScriptClass)
	    {
	      TScriptComponentInstanceConstVector componentInstances;
	      DocUtils::CollectComponentInstances(m_scriptFile, pScriptClass->GetGUID(), true, componentInstances);
	      for(TScriptComponentInstanceConstVector::const_iterator iComponentInstance = componentInstances.begin(), iEndComponentInstance = componentInstances.end(); iComponentInstance != iEndComponentInstance; ++ iComponentInstance)
	      {
	        const IScriptComponentInstance& componentInstance = *(*iComponentInstance);
	        if(componentInstance.GetComponentGUID() == componentGUID)
	        {
	          const char*  szName = envAction.GetName();
	          CStackString fullName;
	          DocUtils::GetFullElementName(m_scriptFile, componentInstance, fullName, EScriptElementType::Class);
	          fullName.append("::");
	          fullName.append(szName);
	          m_actions.push_back(SAction(envAction.GetGUID(), componentInstance.GetGUID(), szName, fullName.c_str(), envAction.GetDescription(), envAction.GetWikiLink(), (envAction.GetFlags() & EActionFlags::Singleton) != 0));
	        }
	      }
	    }
	   }
	   else
	   {
	    const char* szNamespace = envAction.GetNamespace();
	    if(DocUtils::IsElementAvailableInScope(m_scriptFile, m_scopeGUID, szNamespace))
	    {
	      const char*  szName = envAction.GetName();
	      CStackString fullName;
	      EnvRegistryUtils::GetFullName(szName, szNamespace, componentGUID, fullName);
	      m_actions.push_back(SAction(envAction.GetGUID(), componentGUID, szName, fullName.c_str(), envAction.GetDescription(), envAction.GetWikiLink(), (envAction.GetFlags() & EActionFlags::Singleton) != 0));
	    }
	   }
	   return EVisitStatus::Continue;
	   }*/
private:

	Actions m_actions;
};

inline CPoint GetDlgPos()
{
	CPoint dlgPos;
	GetCursorPos(&dlgPos);

	static const int s_dlgOffset = 20;
	dlgPos.x -= s_dlgOffset;
	dlgPos.y -= s_dlgOffset;

	return dlgPos;
}

inline void MoveRenameScriptsRecursive(IScriptElement& element)
{
	IScript* pScript = element.GetScript();
	if (pScript)
	{
		const CStackString prevFileName = pScript->GetName();
		const CStackString fileName = pScript->SetNameFromRoot();

		CStackString folder = fileName.substr(0, fileName.rfind('/'));
		if (!folder.empty())
		{
			gEnv->pCryPak->MakeDir(folder.c_str());
		}

		MoveFile(prevFileName.c_str(), fileName.c_str());

		// #SchematycTODO : Use RemoveDirectory() to remove empty directories?
		// #SchematycTODO : Update source control!
	}
	for (IScriptElement* pChild = element.GetFirstChild(); pChild; pChild = pChild->GetNextSibling())
	{
		MoveRenameScriptsRecursive(*pChild);
		;
	}
}

bool CanAddScriptElement(EScriptElementType elementType, IScriptElement* pScope)
{
	const EScriptElementType scopeElementType = pScope ? pScope->GetElementType() : EScriptElementType::Root;
	switch (elementType)
	{
	case EScriptElementType::Module:
		{
			return scopeElementType == EScriptElementType::Root || scopeElementType == EScriptElementType::Module;
		}
	case EScriptElementType::Import:
		{
			return scopeElementType == EScriptElementType::Module || scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::Enum:
		{
			return scopeElementType == EScriptElementType::Module || scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::Struct:
		{
			return scopeElementType == EScriptElementType::Module || scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::Signal:
		{
			return scopeElementType == EScriptElementType::Module || scopeElementType == EScriptElementType::Class || scopeElementType == EScriptElementType::State;
		}
	case EScriptElementType::Function:
		{
			return scopeElementType == EScriptElementType::Module || scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::Interface:
		{
			return scopeElementType == EScriptElementType::Module;
		}
	case EScriptElementType::InterfaceFunction:
		{
			return scopeElementType == EScriptElementType::Interface;
		}
	case EScriptElementType::InterfaceTask:
		{
			return scopeElementType == EScriptElementType::Interface;
		}
	case EScriptElementType::Class:
		{
			return scopeElementType == EScriptElementType::Module;
		}
	case EScriptElementType::StateMachine:
		{
			return scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::State:
		{
			return scopeElementType == EScriptElementType::StateMachine || scopeElementType == EScriptElementType::State;
		}
	case EScriptElementType::Variable:
		{
			return scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::Property:
		{
			return scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::Timer:
		{
			return scopeElementType == EScriptElementType::Class || scopeElementType == EScriptElementType::State;
		}
	case EScriptElementType::SignalReceiver:
		{
			return scopeElementType == EScriptElementType::Class || scopeElementType == EScriptElementType::State;
		}
	case EScriptElementType::InterfaceImpl:
		{
			return scopeElementType == EScriptElementType::Class;
		}
	case EScriptElementType::ComponentInstance:
		{
			switch (scopeElementType)
			{
			case EScriptElementType::Class:
				{
					return true;
				}
			case EScriptElementType::ComponentInstance:
				{
					const IScriptComponentInstance& componentInstance = DynamicCast<IScriptComponentInstance>(*pScope);
					const IEnvComponent* pEnvComponent = GetSchematycFramework().GetEnvRegistry().GetComponent(componentInstance.GetComponentTypeGUID());
					if (pEnvComponent)
					{
						if (pEnvComponent->GetFlags().Check(EEnvComponentFlags::Socket))
						{
							return true;
						}
					}
					return false;
				}
			default:
				{
					return false;
				}
			}
		}
	case EScriptElementType::ActionInstance:
		{
			return scopeElementType == EScriptElementType::Class || scopeElementType == EScriptElementType::State;
		}
	default:
		{
			return false;
		}
	}
}

bool CanRemoveScriptElement(const IScriptElement& element)
{
	const EScriptElementType elementType = element.GetElementType();
	switch (elementType)
	{
	case EScriptElementType::Root:
	case EScriptElementType::Base:
		{
			return false;
		}
		/*case EScriptElementType::Graph:
		   {
		    switch(static_cast<const IDocGraph&>(element).GetType())
		    {
		    case EScriptGraphType::InterfaceFunction:
		    case EScriptGraphType::Transition_DEPRECATED:
		      {
		        return false;
		      }
		    }
		    break;
		   }*/
	}
	return true;
}

bool CanRenameScriptElement(const IScriptElement& element)
{
	return !element.GetElementFlags().Check(EScriptElementFlags::FixedName);
}

const char* GetScriptElementTypeName(EScriptElementType scriptElementType)
{
	switch (scriptElementType)
	{
	case EScriptElementType::Import:
		{
			return "Import";
		}
	case EScriptElementType::Enum:
		{
			return "Enumeration";
		}
	case EScriptElementType::Struct:
		{
			return "Structure";
		}
	case EScriptElementType::Signal:
		{
			return "Signal";
		}
	case EScriptElementType::Function:
		{
			return "Function";
		}
	case EScriptElementType::SignalReceiver:
		{
			return "Signal Receiver";
		}
	case EScriptElementType::Interface:
		{
			return "Interface";
		}
	case EScriptElementType::Class:
		{
			return "Class";
		}
	case EScriptElementType::StateMachine:
		{
			return "State Machine";
		}
	case EScriptElementType::Variable:
		{
			return "Variable";
		}
	case EScriptElementType::Property:
		{
			return "Property";
		}
	case EScriptElementType::Timer:
		{
			return "Timer";
		}
	case EScriptElementType::InterfaceImpl:
		{
			return "Interface";
		}
	case EScriptElementType::ComponentInstance:
		{
			return "Component";
		}
	case EScriptElementType::ActionInstance:
		{
			return "Action";
		}
	}
	return nullptr;
}

const char* GetScriptElementFilterName(EScriptElementType scriptElementType)
{
	switch (scriptElementType)
	{
	case EScriptElementType::Import:
		{
			return "Imports";
		}
	case EScriptElementType::Enum:
		{
			return "Enumerations";
		}
	case EScriptElementType::Struct:
		{
			return "Structures";
		}
	case EScriptElementType::Signal:
		{
			return "Signals";
		}
	case EScriptElementType::Function:
		{
			return "Functions";
		}
	case EScriptElementType::SignalReceiver:
		{
			return "Signal Receivers";
		}
	case EScriptElementType::Interface:
		{
			return "Interfaces";
		}
	case EScriptElementType::Class:
		{
			return "Classes";
		}
	case EScriptElementType::StateMachine:
		{
			return "State Machines";
		}
	case EScriptElementType::Variable:
		{
			return "Variables";
		}
	case EScriptElementType::Property:
		{
			return "Properties";
		}
	case EScriptElementType::Timer:
		{
			return "Timers";
		}
	case EScriptElementType::InterfaceImpl:
		{
			return "Interfaces";
		}
	case EScriptElementType::ComponentInstance:
		{
			return "Components";
		}
	case EScriptElementType::ActionInstance:
		{
			return "Actions";
		}
	}
	return nullptr;
}

const char* GetScriptElementIcon(const IScriptElement& scriptElement)
{
	switch (scriptElement.GetElementType())
	{
	case EScriptElementType::Module:
		{
			return g_szScriptModuleIcon;
		}
	case EScriptElementType::Import:
		{
			return g_szScriptImportIcon;
		}
	case EScriptElementType::Enum:
		{
			return g_szScriptEnumIcon;
		}
	case EScriptElementType::Struct:
		{
			return g_szScriptStructIcon;
		}
	case EScriptElementType::Signal:
		{
			return g_szScriptSignalIcon;
		}
	case EScriptElementType::Constructor:
		{
			return g_szScriptConstructorIcon;
		}
	case EScriptElementType::Destructor:
		{
			return g_szScriptDestructorIcon;
		}
	case EScriptElementType::Function:
		{
			return g_szScriptFunctionIcon;
		}
	case EScriptElementType::Interface:
		{
			return g_szScriptInterfaceIcon;
		}
	case EScriptElementType::InterfaceFunction:
		{
			return g_szScriptInterfaceFunctionIcon;
		}
	case EScriptElementType::InterfaceTask:
		{
			return g_szScriptInterfaceTaskIcon;
		}
	case EScriptElementType::Class:
		{
			return g_szScriptElementIcon;
		}
	case EScriptElementType::Base:
		{
			return g_szScriptElementIcon;
		}
	case EScriptElementType::StateMachine:
		{
			return g_szScriptStateMachineIcon;
		}
	case EScriptElementType::State:
		{
			return g_szScriptStateIcon;
		}
	case EScriptElementType::Variable:
		{
			return g_szScriptVariableIcon;
		}
	case EScriptElementType::Property:
		{
			return g_szScriptPropertyIcon;
		}
	case EScriptElementType::Timer:
		{
			return g_szScriptTimerIcon;
		}
	case EScriptElementType::SignalReceiver:
		{
			return g_szScriptSignalReceiverIcon;
		}
	case EScriptElementType::InterfaceImpl:
		{
			return g_szScriptInterfaceIcon;
		}
	case EScriptElementType::ComponentInstance:
		{
			const SGUID guid = DynamicCast<IScriptComponentInstance>(scriptElement).GetComponentTypeGUID();
			const IEnvComponent* pEnvComponent = GetSchematycFramework().GetEnvRegistry().GetComponent(guid);
			if (pEnvComponent)
			{
				const char* szIcon = pEnvComponent->GetIcon();
				if (szIcon && szIcon[0])
				{
					return szIcon;
				}
			}
			return g_szScriptComponentIcon;
		}
	case EScriptElementType::ActionInstance:
		{
			const SGUID guid = DynamicCast<IScriptActionInstance>(scriptElement).GetActionTypeGUID();
			const IEnvAction* pEnvAction = GetSchematycFramework().GetEnvRegistry().GetAction(guid);
			if (pEnvAction)
			{
				const char* szIcon = pEnvAction->GetIcon();
				if (szIcon && szIcon[0])
				{
					return szIcon;
				}
			}
			return g_szScriptActionIcon;
		}
	default:
		{
			return "";
		}
	}
}

void MakeScriptElementNameUnique(CStackString& name, IScriptElement* pScope)
{
	IScriptRegistry& scriptRegistry = GetSchematycFramework().GetScriptRegistry();
	if (!scriptRegistry.IsElementNameUnique(name.c_str(), pScope))
	{
		CStackString::size_type counterPos = name.find(".");
		if (counterPos == CStackString::npos)
		{
			counterPos = name.length();
		}

		char stringBuffer[16] = "";
		CStackString::size_type counterLength = 0;
		uint32 counter = 1;
		do
		{
			ltoa(counter, stringBuffer, 10);
			name.replace(counterPos, counterLength, stringBuffer);
			counterLength = strlen(stringBuffer);
			++counter;
		}
		while (!scriptRegistry.IsElementNameUnique(name.c_str(), pScope));
	}
}

void FindReferences(const IScriptElement& element)
{
	const SGUID& guid = element.GetGUID();
	const IScriptElement* pCurrentScriptElement = nullptr;
	std::vector<const IScriptElement*> references;
	references.reserve(100);

	auto enumerateDependencies = [&guid, &pCurrentScriptElement, &references](const SGUID& referenceGUID)
	{
		if (referenceGUID == guid)
		{
			references.push_back(pCurrentScriptElement);
		}
	};

	auto visitScriptElement = [&pCurrentScriptElement, &enumerateDependencies](const IScriptElement& scriptElement) -> EVisitStatus
	{
		pCurrentScriptElement = &scriptElement;
		scriptElement.EnumerateDependencies(ScriptDependencyEnumerator::FromLambda(enumerateDependencies), EScriptDependencyType::Reference);
		return EVisitStatus::Recurse;
	};

	GetSchematycFramework().GetScriptRegistry().GetRootElement().VisitChildren(ScriptElementConstVisitor::FromLambda(visitScriptElement));

	IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(element.GetGUID());

	CReportWidget* pReportWidget = new CReportWidget(nullptr);
	pReportWidget->setAttribute(Qt::WA_DeleteOnClose);
	pReportWidget->setWindowTitle("References");

	for (const IScriptElement* pReference : references)
	{
		CStackString uri;
		CryLinkUtils::CreateUri(uri, CryLinkUtils::ECommand::Show, pReference->GetGUID());

		CStackString name;
		pScriptView->QualifyName(*pReference, EDomainQualifier::Global, name);

		pReportWidget->WriteUri(uri.c_str(), name.c_str());
	}

	pReportWidget->show();
}

IScriptModule* AddScriptModule(IScriptElement* pScope)
{
	CStackString name = "NewModule";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddModule(name.c_str(), pScope);
}

IScriptImport* AddScriptImport(IScriptElement* pScope)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CImportQuickSearchOptions quickSearchOptions(pScope);
	CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		const SImport* pImport = quickSearchOptions.GetImport(quickSearchDlg.GetResult());
		SCHEMATYC_EDITOR_ASSERT(pImport);
		if (pImport)
		{
			return GetSchematycFramework().GetScriptRegistry().AddImport(pImport->moduleGUID, pScope);
		}
	}
	return nullptr;
}

IScriptEnum* AddScriptEnum(IScriptElement* pScope)
{
	CStackString name = "NewEnumeration";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddEnum(name.c_str(), pScope);
}

IScriptStruct* AddScriptStruct(IScriptElement* pScope)
{
	CStackString name = "NewStructure";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddStruct(name.c_str(), pScope);
}

IScriptSignal* AddScriptSignal(IScriptElement* pScope)
{
	CStackString name = "NewSignal";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddSignal(name.c_str(), pScope);
}

IScriptFunction* AddScriptFunction(IScriptElement* pScope)
{
	CStackString name = "NewFunction";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddFunction(name.c_str(), pScope);
}

IScriptInterface* AddScriptInterface(IScriptElement* pScope)
{
	CStackString name = "NewInterface";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddInterface(name.c_str(), pScope);
}

IScriptInterfaceFunction* AddScriptInterfaceFunction(IScriptElement* pScope)
{
	CStackString name = "NewFunction";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddInterfaceFunction(name.c_str(), pScope);
}

IScriptInterfaceTask* AddScriptInterfaceTask(IScriptElement* pScope)
{
	CStackString name = "NewTask";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddInterfaceTask(name.c_str(), pScope);
}

IScriptClass* AddScriptClass(IScriptElement* pScope)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	const SBase* pBase = nullptr;

	CBaseQuickSearchOptions quickSearchOptions(pScope);
	if (quickSearchOptions.GetCount() == 1)
	{
		pBase = quickSearchOptions.GetBase(0);
	}
	else
	{
		CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
		if (quickSearchDlg.DoModal() == IDOK)
		{
			pBase = quickSearchOptions.GetBase(quickSearchDlg.GetResult());
		}
	}

	if (pBase)
	{
		CStackString name = "NewClass";
		MakeScriptElementNameUnique(name, pScope);
		return GetSchematycFramework().GetScriptRegistry().AddClass(name.c_str(), pBase->id, pScope);
	}

	return nullptr;
}

IScriptStateMachine* AddScriptStateMachine(IScriptElement* pScope)
{
	CStackString name = "NewStateMachine";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddStateMachine(name.c_str(), EScriptStateMachineLifetime::Persistent, SGUID(), SGUID(), pScope);
}

IScriptState* AddScriptState(IScriptElement* pScope)
{
	CStackString name = "NewState";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddState(name.c_str(), pScope);
}

IScriptVariable* AddScriptVariable(IScriptElement* pScope)
{
	CStackString name = "NewVariable";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddVariable(name.c_str(), SElementId(), pScope); // #SchematycTODO : Remove typeId parameter from IScriptRegistry::AddVariable()?
}

IScriptProperty* AddScriptProperty(IScriptElement* pScope)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CTypeQuickSearchOptions quickSearchOptions(pScope);
	CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		const SType* pType = quickSearchOptions.GetType(quickSearchDlg.GetResult());
		SCHEMATYC_EDITOR_ASSERT(pType);
		if (pType)
		{
			CStackString name = "NewProperty";
			MakeScriptElementNameUnique(name, pScope);
			return GetSchematycFramework().GetScriptRegistry().AddProperty(name.c_str(), pType->typeId, SGUID(), pScope);
		}
	}
	return nullptr;
}

IScriptTimer* AddScriptTimer(IScriptElement* pScope)
{
	CStackString name = "NewTimer";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddTimer(name.c_str(), pScope);
}

IScriptSignalReceiver* AddScriptSignalReceiver(IScriptElement* pScope)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CStackString name = "NewSignalReceiver";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycFramework().GetScriptRegistry().AddSignalReceiver(name.c_str(), EScriptSignalReceiverType::Universal, SGUID(), pScope);
}

IScriptInterfaceImpl* AddScriptInterfaceImpl(IScriptElement* pScope)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CInterfaceQuickSearchOptions quickSearchOptions(pScope);
	CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		const SInterface* pInterface = quickSearchOptions.GetInterface(quickSearchDlg.GetResult());
		SCHEMATYC_EDITOR_ASSERT(pInterface);
		if (pInterface)
		{
			return GetSchematycFramework().GetScriptRegistry().AddInterfaceImpl(pInterface->domain, pInterface->guid, pScope);
		}
	}
	return nullptr;
}

IScriptComponentInstance* AddScriptComponentInstance(IScriptElement* pScope)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CComponentQuickSearchOptions quickSearchOptions(pScope);
	CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		const SComponent* pComponent = quickSearchOptions.GetComponent(quickSearchDlg.GetResult());
		SCHEMATYC_EDITOR_ASSERT(pComponent);
		if (pComponent)
		{
			CStackString name = pComponent->name.c_str();
			MakeScriptElementNameUnique(name, pScope);
			return GetSchematycFramework().GetScriptRegistry().AddComponentInstance(name.c_str(), pComponent->typeGUID, pScope);
		}
	}
	return nullptr;
}

IScriptActionInstance* AddScriptActionInstance(IScriptElement* pScope)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CActionQuickSearchOptions quickSearchOptions(pScope);
	CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		const SAction* pAction = quickSearchOptions.GetAction(quickSearchDlg.GetResult());
		SCHEMATYC_EDITOR_ASSERT(pAction);
		if (pAction)
		{
			CStackString name = pAction->name.c_str();
			MakeScriptElementNameUnique(name, pScope);
			return GetSchematycFramework().GetScriptRegistry().AddActionInstance(name.c_str(), pAction->guid, pAction->componentInstanceGUID, pScope);
		}
	}
	return nullptr;
}

bool RenameScriptElement(IScriptElement& scriptElement, const char* szName)
{
	SCHEMATYC_EDITOR_ASSERT(szName);
	if (szName)
	{
		if (CanRenameScriptElement(scriptElement))
		{
			const char* szPrevName = scriptElement.GetName();
			if (strcmp(szName, szPrevName) != 0)
			{
				bool bRenameScriptElement = true;
				bool bMoveRenameScriptFiles = false;

				const IScript* pScript = scriptElement.GetScript();
				if (pScript)
				{
					const char* szScriptName = pScript->GetName();
					if (szScriptName[0] != '\0')
					{
						CStackString fileName;
						PluginUtils::ConstructAbsolutePath(fileName, szScriptName);
						if (gEnv->pCryPak->IsFileExist(fileName.c_str()))
						{
							CStackString message;
							message.Format("Renaming '%s' to '%s'.\nWould you like to move/rename script files to match?", szPrevName, szName);

							const int result = CCustomMessageBox::Execute(message.c_str(), "Move/Rename Script Files", MB_YESNOCANCEL | MB_ICONQUESTION);
							bRenameScriptElement = result != IDCANCEL;
							bMoveRenameScriptFiles = result == IDYES;
						}
					}
				}

				if (bRenameScriptElement)
				{
					scriptElement.SetName(szName);
					GetSchematycFramework().GetScriptRegistry().ElementModified(scriptElement); // #SchematycTODO : Call from script element?
					if (bMoveRenameScriptFiles)
					{
						MoveRenameScriptsRecursive(scriptElement);
					}
				}

				return true;
			}
		}
	}
	return false;
}

void RemoveScriptElement(const IScriptElement& element)
{
	if (CanRemoveScriptElement(element))
	{
		string message = "Are you sure you want to remove '";
		message.append(element.GetName());
		message.append("'?");
		if (CCustomMessageBox::Execute(GetDlgPos(), message.c_str(), "Remove", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
		{
			GetSchematycFramework().GetScriptRegistry().RemoveElement(element.GetGUID());
		}
	}
}
} // ScriptBrowserUtils
} // Schematyc
