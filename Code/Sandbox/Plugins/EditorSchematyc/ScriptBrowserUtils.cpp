// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ScriptBrowserUtils.h"

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/Env/Elements/IEnvAction.h>
#include <Schematyc/Env/Elements/IEnvClass.h>
#include <Schematyc/Env/Elements/IEnvComponent.h>
#include <Schematyc/Env/Elements/IEnvDataType.h>
#include <Schematyc/Env/Elements/IEnvSignal.h>
#include <Schematyc/Script/IScript.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/IScriptView.h>
#include <Schematyc/Script/Elements/IScriptActionInstance.h>
#include <Schematyc/Script/Elements/IScriptClass.h>
#include <Schematyc/Script/Elements/IScriptEnum.h>
#include <Schematyc/Script/Elements/IScriptInterface.h>
#include <Schematyc/Script/Elements/IScriptInterfaceFunction.h>
#include <Schematyc/Script/Elements/IScriptInterfaceImpl.h>
#include <Schematyc/Script/Elements/IScriptInterfaceTask.h>
#include <Schematyc/Script/Elements/IScriptModule.h>
#include <Schematyc/Script/Elements/IScriptSignal.h>
#include <Schematyc/Script/Elements/IScriptSignalReceiver.h>
#include <Schematyc/Script/Elements/IScriptState.h>
#include <Schematyc/Script/Elements/IScriptStateMachine.h>
#include <Schematyc/Script/Elements/IScriptStruct.h>
#include <Schematyc/Script/Elements/IScriptTimer.h>
#include <Schematyc/Utils/StackString.h>

#include <Controls/QuestionDialog.h>
#include "PluginUtils.h"
#include "ReportWidget.h"

#include "TypesDictionary.h"
#include <Controls/DictionaryWidget.h>
#include <Controls/QPopupWidget.h>

#include <QPointer>
#include <QCursor>
#include <QtUtil.h>

#include "ComponentsDictionaryModel.h"

namespace Schematyc
{
namespace ScriptBrowserUtils
{
// #SchematycTODO : Move icon declarations to separate file?

static const char* g_szScriptElementIcon = "icons:schematyc/script_element.png";
static const char* g_szScriptModuleIcon = "icons:schematyc/script_module.png";
static const char* g_szScriptImportIcon = "icons:schematyc/script_import.png";
static const char* g_szScriptEnumIcon = "icons:schematyc/script_enum.png";
static const char* g_szScriptStructIcon = "icons:schematyc/script_struct.png";
static const char* g_szScriptSignalIcon = "icons:schematyc/script_signal.png";
static const char* g_szScriptConstructorIcon = "icons:schematyc/script_constructor.png";
static const char* g_szScriptDestructorIcon = "icons:schematyc/script_destructor.png";
static const char* g_szScriptFunctionIcon = "icons:schematyc/script_function.png";
static const char* g_szScriptInterfaceIcon = "icons:schematyc/script_interface.png";
static const char* g_szScriptInterfaceFunctionIcon = "icons:schematyc/script_interface_function.png";
static const char* g_szScriptInterfaceTaskIcon = "icons:schematyc/script_interface_task.png";
static const char* g_szScriptStateMachineIcon = "icons:schematyc/script_state_machine.png";
static const char* g_szScriptStateIcon = "icons:schematyc/script_state.png";
static const char* g_szScriptVariableIcon = "icons:schematyc/script_variable.png";
static const char* g_szScriptTimerIcon = "icons:schematyc/script_timer.png";
static const char* g_szScriptSignalReceiverIcon = "icons:schematyc/script_signal_receiver.png";
static const char* g_szScriptComponentIcon = "icons:schematyc/script_component.png";
static const char* g_szScriptActionIcon = "icons:schematyc/script_action.png";

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

			IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptScope->GetGUID());

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
		return "Folder";
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

			IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptScope->GetGUID());

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

/*struct SType
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

      IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptScope->GetGUID());

      auto visitEnvDataType = [this, &pScriptView](const IEnvDataType& envDataType) -> EVisitStatus
      {
        CStackString name;
        pScriptView->QualifyName(envDataType, name);
        m_types.push_back(SType(name.c_str(), SElementId(EDomain::Env, envDataType.GetGUID())));
        return EVisitStatus::Continue;
      };
      pScriptView->VisitEnvDataTypes(EnvDataTypeConstVisitor::FromLambda(visitEnvDataType));

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
   };*/

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

			IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptScope->GetGUID());
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
				const IEnvComponent* pEnvComponent = GetSchematycCore().GetEnvRegistry().GetComponent(pScriptComponentInstance->GetTypeGUID());
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

			IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptScope->GetGUID());
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

			IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(pScriptScope->GetGUID());

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
						if (pScriptComponentInstance->GetTypeGUID() == componentTypeGUID)
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
	}
}

// #SchematycTODO : Move logic to Schematyc core (add IsValidScope() function to IStriptElement).
bool CanAddScriptElement(EScriptElementType elementType, IScriptElement* pScope)
{
	return GetSchematycCore().GetScriptRegistry().IsValidScope(elementType, pScope);
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
	case EScriptElementType::Constructor:
	case EScriptElementType::Destructor:
	case EScriptElementType::SignalReceiver:
		{
			return "Graphs";
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
			const SGUID guid = DynamicCast<IScriptComponentInstance>(scriptElement).GetTypeGUID();
			const IEnvComponent* pEnvComponent = GetSchematycCore().GetEnvRegistry().GetComponent(guid);
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
			const IEnvAction* pEnvAction = GetSchematycCore().GetEnvRegistry().GetAction(guid);
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
	IScriptRegistry& scriptRegistry = GetSchematycCore().GetScriptRegistry();
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

	GetSchematycCore().GetScriptRegistry().GetRootElement().VisitChildren(ScriptElementConstVisitor::FromLambda(visitScriptElement));

	IScriptViewPtr pScriptView = GetSchematycCore().CreateScriptView(element.GetGUID());

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
	CStackString name = "NewFolder";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddModule(name.c_str(), pScope);
}

IScriptImport* AddScriptImport(IScriptElement* pScope)
{
	/*CImportQuickSearchOptions quickSearchOptions(pScope);
	   CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	   if (quickSearchDlg.DoModal() == IDOK)
	   {
	   const SImport* pImport = quickSearchOptions.GetImport(quickSearchDlg.GetResult());
	   SCHEMATYC_EDITOR_ASSERT(pImport);
	   if (pImport)
	   {
	    return GetSchematycCore().GetScriptRegistry().AddImport(pImport->moduleGUID, pScope);
	   }
	   }*/
	return nullptr;
}

IScriptEnum* AddScriptEnum(IScriptElement* pScope)
{
	CStackString name = "NewEnumeration";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddEnum(name.c_str(), pScope);
}

IScriptStruct* AddScriptStruct(IScriptElement* pScope)
{
	CStackString name = "NewStructure";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddStruct(name.c_str(), pScope);
}

IScriptSignal* AddScriptSignal(IScriptElement* pScope)
{
	CStackString name = "NewSignal";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddSignal(name.c_str(), pScope);
}

IScriptFunction* AddScriptFunction(IScriptElement* pScope)
{
	CStackString name = "NewFunction";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddFunction(name.c_str(), pScope);
}

IScriptInterface* AddScriptInterface(IScriptElement* pScope)
{
	CStackString name = "NewInterface";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddInterface(name.c_str(), pScope);
}

IScriptInterfaceFunction* AddScriptInterfaceFunction(IScriptElement* pScope)
{
	CStackString name = "NewFunction";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddInterfaceFunction(name.c_str(), pScope);
}

IScriptInterfaceTask* AddScriptInterfaceTask(IScriptElement* pScope)
{
	CStackString name = "NewTask";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddInterfaceTask(name.c_str(), pScope);
}

IScriptClass* AddScriptClass(IScriptElement* pScope)
{
	const SBase* pBase = nullptr;

	CBaseQuickSearchOptions quickSearchOptions(pScope);
	if (quickSearchOptions.GetCount() == 1)
	{
		pBase = quickSearchOptions.GetBase(0);
	}
	/*else
	   {
	   CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	   if (quickSearchDlg.DoModal() == IDOK)
	   {
	    pBase = quickSearchOptions.GetBase(quickSearchDlg.GetResult());
	   }
	   }*/

	if (pBase)
	{
		CStackString name = "NewClass";
		MakeScriptElementNameUnique(name, pScope);
		return GetSchematycCore().GetScriptRegistry().AddClass(name.c_str(), pBase->id, pScope);
	}

	return nullptr;
}

IScriptStateMachine* AddScriptStateMachine(IScriptElement* pScope)
{
	CStackString name = "NewStateMachine";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddStateMachine(name.c_str(), EScriptStateMachineLifetime::Persistent, SGUID(), SGUID(), pScope);
}

IScriptState* AddScriptState(IScriptElement* pScope)
{
	CStackString name = "NewState";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddState(name.c_str(), pScope);
}

IScriptVariable* AddScriptVariable(IScriptElement* pScope)
{
	CStackString name = "NewVariable";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddVariable(name.c_str(), SElementId(), SGUID(), pScope);
}

IScriptTimer* AddScriptTimer(IScriptElement* pScope)
{
	CStackString name = "NewTimer";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddTimer(name.c_str(), pScope);
}

IScriptSignalReceiver* AddScriptSignalReceiver(IScriptElement* pScope)
{
	CStackString name = "NewSignalReceiver";
	MakeScriptElementNameUnique(name, pScope);
	return GetSchematycCore().GetScriptRegistry().AddSignalReceiver(name.c_str(), EScriptSignalReceiverType::Universal, SGUID(), pScope);
}

IScriptInterfaceImpl* AddScriptInterfaceImpl(IScriptElement* pScope)
{
	/*CInterfaceQuickSearchOptions quickSearchOptions(pScope);
	   CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	   if (quickSearchDlg.DoModal() == IDOK)
	   {
	   const SInterface* pInterface = quickSearchOptions.GetInterface(quickSearchDlg.GetResult());
	   SCHEMATYC_EDITOR_ASSERT(pInterface);
	   if (pInterface)
	   {
	    return GetSchematycCore().GetScriptRegistry().AddInterfaceImpl(pInterface->domain, pInterface->guid, pScope);
	   }
	   }*/
	return nullptr;
}

IScriptComponentInstance* AddScriptComponentInstance(IScriptElement* pScope, const QPoint* pPosition)
{
	CrySchematycEditor::CComponentsDictionary dict(pScope);
	QPointer<CModalPopupDictionary> pDictionary = new CModalPopupDictionary("Schematyc::AddComponent", dict);

	QPoint pos;
	if (pPosition)
	{
		pos = *pPosition;
	}
	else
	{
		pos = QCursor::pos();
	}

	pDictionary->ExecAt(pos, QPopupWidget::TopLeft);

	CrySchematycEditor::CComponentDictionaryEntry* pEntry = static_cast<CrySchematycEditor::CComponentDictionaryEntry*>(pDictionary->GetResult());
	if (pEntry)
	{
		CStackString name = QtUtil::ToString(pEntry->GetName()).c_str();
		MakeScriptElementNameUnique(name, pScope);
		return GetSchematycCore().GetScriptRegistry().AddComponentInstance(name.c_str(), pEntry->GetTypeGUID(), pScope);
	}

	return nullptr;
}

IScriptActionInstance* AddScriptActionInstance(IScriptElement* pScope)
{
	/*CActionQuickSearchOptions quickSearchOptions(pScope);
	   CQuickSearchDlg quickSearchDlg(nullptr, GetDlgPos(), quickSearchOptions);
	   if (quickSearchDlg.DoModal() == IDOK)
	   {
	   const SAction* pAction = quickSearchOptions.GetAction(quickSearchDlg.GetResult());
	   SCHEMATYC_EDITOR_ASSERT(pAction);
	   if (pAction)
	   {
	    CStackString name = pAction->name.c_str();
	    MakeScriptElementNameUnique(name, pScope);
	    return GetSchematycCore().GetScriptRegistry().AddActionInstance(name.c_str(), pAction->guid, pAction->componentInstanceGUID, pScope);
	   }
	   }*/
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
				const char* szErrorMessage = nullptr;
				if (!GetSchematycCore().GetScriptRegistry().IsValidName(szName, scriptElement.GetParent(), szErrorMessage))
				{
					stack_string message;
					message.Format("Invalid name '%s'. %s", szName, szErrorMessage);

					CQuestionDialog dialog;
					dialog.SetupQuestion("Invalid Name", message.c_str(), QDialogButtonBox::Ok);

					dialog.Execute();

					return false;
				}

				bool bRenameScriptElement = true;
				bool bMoveRenameScriptFiles = false;

				const IScript* pScript = scriptElement.GetScript();
				if (pScript)
				{
					const char* szScriptName = pScript->GetName();
					if (szScriptName[0] != '\0')
					{
						CStackString fileName;
						CrySchematycEditor::Utils::ConstructAbsolutePath(fileName, szScriptName);
						if (gEnv->pCryPak->IsFileExist(fileName.c_str()))
						{
							CStackString question;
							question.Format("Renaming '%s' to '%s'.\nWould you like to move/rename script files to match?", szPrevName, szName);

							CQuestionDialog dialog;
							dialog.SetupQuestion("Move/Rename Script Files", question.c_str(), QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel);

							QDialogButtonBox::StandardButton result = dialog.Execute();
							bRenameScriptElement = (result != QDialogButtonBox::Cancel);
							bMoveRenameScriptFiles = (result == QDialogButtonBox::Yes);
						}
					}
				}

				if (bRenameScriptElement)
				{
					scriptElement.SetName(szName);
					GetSchematycCore().GetScriptRegistry().ElementModified(scriptElement); // #SchematycTODO : Call from script element?
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
		CStackString question = "Are you sure you want to remove '";
		question.append(element.GetName());
		question.append("'?");

		CQuestionDialog dialog;
		dialog.SetupQuestion("Remove", question.c_str(), QDialogButtonBox::Yes | QDialogButtonBox::No);

		QDialogButtonBox::StandardButton result = dialog.Execute();

		if (result == QDialogButtonBox::Yes)
		{
			GetSchematycCore().GetScriptRegistry().RemoveElement(element.GetGUID());
		}
	}
}
} // ScriptBrowserUtils
} // Schematyc
