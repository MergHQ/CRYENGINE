// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SchematycEntityDrsComponent.h"
#include "../CryEntitySystem/DynamicResponseProxy.h"

#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using Schematyc::CSharedString;
using namespace DRS;

CSchematycEntityDrsComponent::CSchematycEntityDrsComponent()
{
}

CSchematycEntityDrsComponent::~CSchematycEntityDrsComponent()
{
}

bool CSchematycEntityDrsComponent::Init()
{
	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	m_pDrsEntityComp = entity.GetOrCreateComponent<CEntityComponentDynamicResponse>();
	return true;
}

void CSchematycEntityDrsComponent::Run(Schematyc::ESimulationMode simulationMode)
{
	if (simulationMode != Schematyc::ESimulationMode::Idle && simulationMode != Schematyc::ESimulationMode::Preview)
	{
		IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
		const char* szDrsActorName = (m_name.empty()) ? entity.GetName() : m_name.c_str();
		SET_DRS_USER_SCOPED("DrsProxy via Schematyc Initialize"); //this line is just for the editor, so that the name-variable change can be associated with a reason
		m_pDrsEntityComp->GetLocalVariableCollection()->SetVariableValue("Name", CHashedString(szDrsActorName));
		gEnv->pDynamicResponseSystem->GetSpeakerManager()->AddListener(this);
	}
}

void CSchematycEntityDrsComponent::Shutdown()
{
	gEnv->pDynamicResponseSystem->GetSpeakerManager()->RemoveListener(this);

	IEntity& entity = Schematyc::EntityUtils::GetEntity(*this);
	entity.RemoveComponent(m_pDrsEntityComp);  //we assume no one else needs it anymore
	m_pDrsEntityComp = nullptr;
}

void CSchematycEntityDrsComponent::ReflectType(Schematyc::CTypeDesc<CSchematycEntityDrsComponent>& desc)
{
	desc.SetGUID("25854445-cd59-4257-827d-aef984790598"_schematyc_guid);
	desc.SetLabel("DRS");
	desc.SetDescription("Dynamic Response System component");
	desc.SetIcon("icons:Dialogs/notification_text.ico");
	desc.SetComponentFlags(Schematyc::EComponentFlags::Singleton);
	desc.AddMember(&CSchematycEntityDrsComponent::m_name, 'name', "actorName", "ActorName", nullptr);
}

void CSchematycEntityDrsComponent::Register(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(Schematyc::g_entityClassGUID);
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CSchematycEntityDrsComponent));
		// Functions
		
		//TODO: Send Signal (With context variables) (for now just hardcoded string/hash/float/int)
		//	  SetVariable (Global/Local, CreateIfNotExisting)
		//	  Idea: CreateContextVariableCollection -> SetVariable -> SendSignal (Send signal frees the collection)
		//	Idea: SetVariable outputs the usedVariableCollection, so that it can be used as an input for SendSignal

		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntityDrsComponent::SendSignal, "3f00d849-3a9e-4fdf-b322-30ce546005ed"_schematyc_guid, "SendSignal");
			pFunction->SetDescription("Sends a DRS signal");
			pFunction->BindInput(1, 'sig', "SignalName");
			pFunction->BindInput(2, 'cfn', "ContextFloatName", "(optional) The name of a float-variable that can be passed along with this signal");
			pFunction->BindInput(3, 'cfv', "ContextFloatValue", "(optional) The value of the float-variable that can be passed along with this signal");
			pFunction->BindInput(4, 'csn', "ContextStringName", "(optional) The name of a (hashed) string-variable that can be passed along with this signal");
			pFunction->BindInput(5, 'csv', "ContextStringValue", "(optional) The value of the (hashed) string-variable that can be passed along with this signal");
			//pFunction->BindInput(6, 'userData', "UserData", "(optional) a string that is passed along with this signal");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntityDrsComponent::SetFloatVariable, "322c7c23-f4bd-4582-a55b-7ccce84aa6c1"_schematyc_guid, "SetFloatVariable");
			pFunction->SetDescription("Sets a float variable in a variable collection");
			pFunction->BindInput(1, 'col', "CollectionName");
			pFunction->BindInput(2, 'var', "VariableName");
			pFunction->BindInput(3, 'val', "Value");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntityDrsComponent::SetStringVariable, "47b46015-0bb9-4c15-bc40-50914746cfd3"_schematyc_guid, "SetStringVariable");
			pFunction->SetDescription("Sets a string variable in a variable collection");
			pFunction->BindInput(1, 'col', "CollectionName");
			pFunction->BindInput(2, 'var', "VariableName");
			pFunction->BindInput(3, 'val', "Value");
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSchematycEntityDrsComponent::SetIntVariable, "a0119cb0-f4c3-4c53-959b-4fe06f5d0691"_schematyc_guid, "SetIntVariable");
			pFunction->SetDescription("Sets a int variable in a variable collection");
			pFunction->BindInput(1, 'col', "CollectionName");
			pFunction->BindInput(2, 'var', "VariableName");
			pFunction->BindInput(3, 'val', "Value");
			componentScope.Register(pFunction);
		}
		// Signals
		{
			componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SResponseFinishedSignal));
			componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SResponseStartedSignal));
			componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SLineStartedSignal));
			componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SLineEndedSignal));
		}
	}
}

void CSchematycEntityDrsComponent::SendSignal(const CSharedString& signalName, const CSharedString& contextFloatName, float contextFloatValue, const CSharedString& contextStringName, const CSharedString& contextStringValue)
{
	IVariableCollectionSharedPtr pCollection = nullptr;
	if (!contextFloatName.empty() || !contextStringName.empty())
	{
		pCollection = gEnv->pDynamicResponseSystem->CreateContextCollection();
		if (contextFloatName.empty())
		{
			pCollection->CreateVariable(contextFloatName.c_str(), contextFloatValue);
		}
		if (contextStringName.empty())
		{
			pCollection->CreateVariable(contextStringName.c_str(), CHashedString(contextStringValue.c_str()));
		}
	}

	m_pDrsEntityComp->GetResponseActor()->QueueSignal(signalName.c_str(), pCollection, this);
}

void CSchematycEntityDrsComponent::SetFloatVariable(const CSharedString& collectionName, const CSharedString& variableName, float value)
{
	IVariableCollection* pCollection = GetVariableCollection(collectionName);
	
	if (pCollection)
	{
		pCollection->SetVariableValue(variableName.c_str(), value);
	}
}

void CSchematycEntityDrsComponent::SetStringVariable(const CSharedString& collectionName, const CSharedString& variableName, const CSharedString& value)
{
	IVariableCollection* pCollection = GetVariableCollection(collectionName);
	if (pCollection)
	{
		pCollection->SetVariableValue(variableName.c_str(), CHashedString(value.c_str()));
	}
}

void CSchematycEntityDrsComponent::SetIntVariable(const CSharedString& collectionName, const CSharedString& variableName, int value)
{
	IVariableCollection* pCollection = GetVariableCollection(collectionName);
	if (pCollection)
	{
		pCollection->SetVariableValue(variableName.c_str(), value);
	}
}

IVariableCollection* CSchematycEntityDrsComponent::GetVariableCollection(const CSharedString& collectionName)
{
	if (collectionName == "Local" || collectionName == "local")
		return m_pDrsEntityComp->GetLocalVariableCollection();
	else
		return gEnv->pDynamicResponseSystem->GetCollection(collectionName.c_str());
}

void CSchematycEntityDrsComponent::OnSignalProcessingStarted(SSignalInfos& signal, IResponseInstance* pStartedResponse)
{
	OutputSignal(SResponseStartedSignal{(int)signal.id});
}

void CSchematycEntityDrsComponent::OnSignalProcessingFinished(SSignalInfos& signal, IResponseInstance* pFinishedResponse, eProcessingResult outcome)
{	
	OutputSignal(SResponseFinishedSignal{ signal.id, outcome });
}

void CSchematycEntityDrsComponent::OnLineEvent(const IResponseActor* pSpeaker, const CHashedString& lineID, eLineEvent lineEvent, const IDialogLine* pLine)
{
	//remark: every DRS Component will currently receive events for any Speaker
	const Schematyc::CSharedString text = (pLine) ? pLine->GetText().c_str() : lineID.GetText().c_str();
	const Schematyc::CSharedString speakerName = (pSpeaker) ? pSpeaker->GetName().GetText().c_str() : "No Actor";

	if (lineEvent == ISpeakerManager::IListener::eLineEvent_HasEndedInAnyWay)
	{
		OutputSignal(SLineEndedSignal{
			text,
			speakerName,
			(lineEvent == ISpeakerManager::IListener::eLineEvent_Canceled) });
	}
	else if (lineEvent == ISpeakerManager::IListener::eLineEvent_Started)
	{
		OutputSignal(SLineStartedSignal{
			text,
			speakerName });
	}
}

void CSchematycEntityDrsComponent::SResponseStartedSignal::ReflectType(Schematyc::CTypeDesc<SResponseStartedSignal>& desc)
{
	desc.SetGUID("f01fdb01-b03f-4eab-a0af-8d2359b4547b"_schematyc_guid);
	desc.SetLabel("ResponseStarted");
	desc.SetDescription("Sent when a response is started.");
	desc.AddMember(&SResponseStartedSignal::m_signalId, 'id', "responseId", "ResponseId", nullptr);  //actually ResponseInstanceId
}

void CSchematycEntityDrsComponent::SResponseFinishedSignal::ReflectType(Schematyc::CTypeDesc<SResponseFinishedSignal>& desc)
{
	desc.SetGUID("cece4601-9f11-4e7c-800c-222c601200fa"_schematyc_guid);
	desc.SetLabel("ResponseFinished");
	desc.SetDescription("Sent when a response has finished (or was not even started/existing).");
	desc.AddMember(&SResponseFinishedSignal::m_signalId, 'id', "responseId", "ResponseId", nullptr);  //actually ResponseInstanceId
	desc.AddMember(&SResponseFinishedSignal::m_result, 'res', "result", "Result", nullptr);
}

void CSchematycEntityDrsComponent::SLineStartedSignal::ReflectType(Schematyc::CTypeDesc<SLineStartedSignal>& desc)
{
	desc.SetGUID("e397e62c-5c7f-4fab-9195-12032f670c9f"_schematyc_guid);
	desc.SetLabel("LineStarted");
	desc.SetDescription("Sent when a dialog line is started.");
	desc.AddMember(&SLineStartedSignal::m_text, 'text', "text", "Text", nullptr);
	desc.AddMember(&SLineStartedSignal::m_speakerName, 'act', "speaker", "Speaker", nullptr);
}

void CSchematycEntityDrsComponent::SLineEndedSignal::ReflectType(Schematyc::CTypeDesc<SLineEndedSignal>& desc)
{
	desc.SetGUID("75e5e2ac-377f-4992-84ad-42c551f96e46"_schematyc_guid);
	desc.SetLabel("LineEnded");
	desc.SetDescription("Sent when a dialog line has finished/wasCanceled.");
	desc.AddMember(&SLineEndedSignal::m_text, 'text', "text", "Text", nullptr);
	desc.AddMember(&SLineEndedSignal::m_speakerName, 'act', "speaker", "Speaker", nullptr);
	desc.AddMember(&SLineEndedSignal::m_bWasCanceled, 'id', "wasCanceled", "WasCanceled", nullptr);
}
