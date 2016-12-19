// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityComponent.h"

#include "STDEnv.h"
#include "EntityObjectMap.h"
#include "Utils/SystemStateMonitor.h"

#include <Schematyc/Entity/EntityUserData.h>
#include <CrySerialization/Decorators/ActionButton.h>

CRYREGISTER_CLASS(Schematyc::CEntityObjectComponent);

namespace Schematyc
{
	CEntityObjectComponent::CEntityObjectComponent()
		: m_pObject(nullptr)
		, m_bIsPreview(false)
	{
	}

	CEntityObjectComponent::~CEntityObjectComponent()
	{
		DestroyObject();
	}

	void CEntityObjectComponent::SetRuntimeClass(const Schematyc::IRuntimeClass* pRuntimeClass, bool bIsPreview)
	{
		m_pRuntimeClass = pRuntimeClass;

		m_propertiesLabel = m_pRuntimeClass->GetName();
		m_propertiesLabel.append(" Properties");

		ESimulationMode simulationMode;
		if (bIsPreview)
		{
			simulationMode = ESimulationMode::Preview;
		}
		else if (gEnv->IsEditing() && !CSTDEnv::GetInstance().GetSystemStateMonitor().LoadingLevel())
		{
			simulationMode = ESimulationMode::Editor;
		}
		else
		{
			simulationMode = ESimulationMode::Idle;
		}

		CreateObject(simulationMode);

		// Clone the default properties for this runtime class
		m_pProperties = m_pRuntimeClass->GetDefaultProperties().Clone();
	}

	void CEntityObjectComponent::ProcessEvent(SEntityEvent& event)
	{
		if (m_pObject && ((m_pEntity->GetFlags() & ENTITY_FLAG_SPAWNED) == 0))
		{
			switch (event.event)
			{
				case ENTITY_EVENT_RESET:
					{
						m_pObject->Reset(event.nParam[0] == 1 ? ESimulationMode::Game : ESimulationMode::Editor, EObjectResetPolicy::Always);
						break;
					}
				case ENTITY_EVENT_START_LEVEL:
					{
						if (!gEnv->IsEditor())
						{
							m_pObject->Reset(ESimulationMode::Game, EObjectResetPolicy::OnChange);
						}
						break;
					}
				case ENTITY_EVENT_XFORM:
				case ENTITY_EVENT_DONE:
				case ENTITY_EVENT_HIDE:
				case ENTITY_EVENT_UNHIDE:
				case ENTITY_EVENT_SCRIPT_EVENT:
				case ENTITY_EVENT_ENTERAREA:
				case ENTITY_EVENT_LEAVEAREA:
					{
						m_eventSignal.Send(event);
						break;
					}
			}
		}
	}

	void CEntityObjectComponent::CreateObject(ESimulationMode simulationMode)
	{
		SCHEMATYC_ENV_ASSERT(m_pEntity && !m_pObject);
		if (m_pEntity && !m_pObject)
		{
			SObjectParams objectParams(m_pRuntimeClass->GetGUID());
			objectParams.pCustomData = (IEntityComponentWrapper*)this;
			objectParams.pProperties = m_pProperties;
			objectParams.simulationMode = simulationMode;

			m_pObject = gEnv->pSchematyc->CreateObject(objectParams);
			SCHEMATYC_ENV_ASSERT(m_pObject);
			if (m_pObject)
			{
				CSTDEnv::GetInstance().GetEntityObjectMap().AddEntity(m_pEntity->GetId(), m_pObject->GetId());
			}
		}
	}

	void CEntityObjectComponent::DestroyObject()
	{
		if (m_pObject)
		{
			CSTDEnv::GetInstance().GetEntityObjectMap().RemoveEntity(GetEntityId());
			gEnv->pSchematyc->DestroyObject(m_pObject->GetId());

			m_pObject = nullptr;
		}
	}

	void CEntityObjectComponent::GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->Add(*this);
	}

	void CEntityObjectComponent::DisplayDetails(Serialization::IArchive& archive)
	{
		if (archive.openBlock("overview", "Overview"))
		{
			const IScriptClass* pScriptClass = DynamicCast<IScriptClass>(gEnv->pSchematyc->GetScriptRegistry().GetElement(m_pRuntimeClass->GetGUID()));
			if (pScriptClass)
			{
				{
					string name = pScriptClass->GetName();
					archive(name, "class", "!Class");
				}
				{
					string author = pScriptClass->GetAuthor();
					if (!author.empty())
					{
						archive(author, "author", "!Author");
					}
				}
				{
					string description = pScriptClass->GetDescription();
					if (!description.empty())
					{
						archive(description, "description", "!Description");
					}
				}
			}
			archive(Serialization::ActionButton(functor(*this, &CEntityObjectComponent::ShowInSchematyc)), "showInSchematyc", "Show In Schematyc");
			archive.closeBlock();
		}
	}

	void CEntityObjectComponent::ShowInSchematyc()
	{
		CryLinkUtils::ExecuteCommand(CryLinkUtils::ECommand::Show, m_pRuntimeClass->GetGUID());
	}

	void CEntityObjectComponent::SerializeProperties(Serialization::IArchive& archive)
	{
		archive(*m_pProperties, "properties", "Properties");

		if (archive.isInput())
		{
			if (m_pObject && gEnv->IsEditing())
			{
				DestroyObject();
				CreateObject(ESimulationMode::Editor);
			}
		}
	}

	void CEntityObjectComponent::SerializeToUI(Serialization::IArchive& archive)
	{
		if (archive.isEdit() && archive.isOutput())
		{
			DisplayDetails(archive);
		}

		SerializeProperties(archive);
	}

	EntityEventSignal::Slots& CEntityObjectComponent::GetEventSignalSlots()
	{
		return m_eventSignal.GetSlots();
	}

	GeomUpdateSignal& CEntityObjectComponent::GetGeomUpdateSignal()
	{
		return m_geomUpdateSignal;
	}
} // Schematyc
