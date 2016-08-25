// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryEntitySystem/IEntity.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/IConsole.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>
#include <CrySerialization/IArchiveHost.h>
#include <CryDynamicResponseSystem/IDynamicResponseCondition.h>
#include <CryGame/IGameFramework.h>

#include "ResponseSystemDataImportHelper.h"
#include "DialogLineDatabase.h"
#include "ResponseSystem.h"
#include "ResponseManager.h"
#include "ResponseInstance.h"
#include "VariableCollection.h"
#include "ResponseInstance.h"

using namespace CryDRS;

CResponseSystem* CResponseSystem::s_pInstance = nullptr;

void ReloadResponseDefinition(IConsoleCmdArgs* pArgs)
{
	CResponseSystem::GetInstance()->ReInit();
}

//--------------------------------------------------------------------------------------------------
void SendDrsSignal(IConsoleCmdArgs* pArgs)
{
	if (pArgs->GetArgCount() >= 3)
	{
		CResponseSystem* pDrs = CResponseSystem::GetInstance();
		if (pDrs)
		{
			DRS::IResponseActor* pSender = pDrs->GetResponseActor(pArgs->GetArg(1));
			if (pSender)
			{
				pSender->QueueSignal(pArgs->GetArg(2));
				return;
			}
			else
			{
				DrsLogWarning((string("Could not find an actor with that name: ") + pArgs->GetArg(1)).c_str());
			}
		}
	}

	DrsLogWarning("Format: SendDrsSignal <senderEntityName> <signalname>");
}

//--------------------------------------------------------------------------------------------------
void ChangeFileFormat(ICVar* pICVar)
{
	if (!pICVar)
		return;

	const char* pVal = pICVar->GetString();

	CResponseManager::EUsedFileFormat fileFormat = CResponseManager::eUFF_JSON;
	if (strcmp(pVal, "BIN") == 0)
		fileFormat = CResponseManager::eUFF_BIN;
	else if (strcmp(pVal, "XML") == 0)
		fileFormat = CResponseManager::eUFF_XML;

	CResponseSystem::GetInstance()->GetResponseManager()->SetFileFormat(fileFormat);
}

//--------------------------------------------------------------------------------------------------
CResponseSystem::CResponseSystem()
{
	m_bIsCurrentlyUpdating = false;
	m_pendingResetRequest = DRS::IDynamicResponseSystem::eResetHint_Nothing;
	m_pDataImporterHelper = nullptr;

	m_pSpeakerManager = new CSpeakerManager();
	m_pDialogLineDatabase = new CDialogLineDatabase();
	m_pVariableCollectionManager = new CVariableCollectionManager();
	m_pResponseManager = new CResponseManager();
	CRY_ASSERT(s_pInstance == nullptr && "ResponseSystem was created more than once!");
	s_pInstance = this;

	REGISTER_COMMAND("drs_sendSignal", SendDrsSignal, VF_NULL, "Sends a DRS Signal by hand. Useful for testing. Format SendDrsSignal <senderEntityName> <signalname>");

	m_pUsedFileFormat = REGISTER_STRING_CB("drs_fileFormat", "JSON", VF_NULL, "Specifies the file format to use (JSON, XML, BIN)", ::ChangeFileFormat);

	DRS_DEBUG_DATA_ACTION(Init());
}

//--------------------------------------------------------------------------------------------------
CResponseSystem::~CResponseSystem()
{
	gEnv->pConsole->UnregisterVariable("drs_fileFormat", true);
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	delete m_pSpeakerManager;
	delete m_pDialogLineDatabase;

	delete m_pDataImporterHelper;
	delete m_pResponseManager;
	delete m_pVariableCollectionManager;
	s_pInstance = nullptr;
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystem::Init(const char* szFilesFolder)
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	m_filesFolder = szFilesFolder;
	m_pSpeakerManager->Init();
	m_pDialogLineDatabase->InitFromFiles(m_filesFolder + "/DialogLines");

	m_CurrentTime.SetSeconds(0.0f);

	m_pVariableCollectionManager->GetCollection("Global")->CreateVariable("CurrentTime", 0.0f);

	return m_pResponseManager->LoadFromFiles(m_filesFolder + "/Responses");
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystem::ReInit()
{
	Reset();  //stop running responses and reset variables
	m_pDialogLineDatabase->InitFromFiles(m_filesFolder + "/DialogLines");
	return m_pResponseManager->LoadFromFiles(m_filesFolder + "/Responses");
}

//--------------------------------------------------------------------------------------------------
void CResponseSystem::Update()
{
	if (m_pendingResetRequest != DRS::IDynamicResponseSystem::eResetHint_Nothing)
	{
		InternalReset(m_pendingResetRequest);
		m_pendingResetRequest = DRS::IDynamicResponseSystem::eResetHint_Nothing;
	}

	m_CurrentTime += gEnv->pTimer->GetFrameTime();

	m_pVariableCollectionManager->GetCollection("Global")->SetVariableValue("CurrentTime", m_CurrentTime.GetSeconds());

	m_bIsCurrentlyUpdating = true;
	m_pVariableCollectionManager->Update();
	m_pResponseManager->Update();
	m_pSpeakerManager->Update();
	m_bIsCurrentlyUpdating = false;
}

//--------------------------------------------------------------------------------------------------
CVariableCollection* CResponseSystem::CreateVariableCollection(const CHashedString& name)
{
	return m_pVariableCollectionManager->CreateVariableCollection(name);
}
//--------------------------------------------------------------------------------------------------
void CResponseSystem::ReleaseVariableCollection(DRS::IVariableCollection* pToBeReleased)
{
	m_pVariableCollectionManager->ReleaseVariableCollection(static_cast<CVariableCollection*>(pToBeReleased));
}

//--------------------------------------------------------------------------------------------------
CVariableCollection* CResponseSystem::GetCollection(const CHashedString& name)
{
	return m_pVariableCollectionManager->GetCollection(name);
}

//--------------------------------------------------------------------------------------------------
CVariableCollection* CResponseSystem::GetCollection(const CHashedString& name, DRS::IResponseInstance* pResponseInstance)
{
	// taken from IVariableUsingBase::GetCurrentCollection
	if (name == CVariableCollection::s_localCollectionName)  //local variable collection
	{
		CResponseActor* pCurrentActor = static_cast<CResponseInstance*>(pResponseInstance)->GetCurrentActor();
		return pCurrentActor->GetLocalVariables();
	}
	else if (name == CVariableCollection::s_contextCollectionName)  //context variable collection
	{
		return static_cast<CResponseInstance*>(pResponseInstance)->GetContextVariablesImpl().get();
	}
	else
	{
		static CVariableCollectionManager* const pVariableCollectionMgr = CResponseSystem::GetInstance()->GetVariableCollectionManager();
		return pVariableCollectionMgr->GetCollection(name);
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
CDataImportHelper* CResponseSystem::GetCustomDataformatHelper()
{
	if (!m_pDataImporterHelper)
	{
		m_pDataImporterHelper = new CDataImportHelper();
	}
	return m_pDataImporterHelper;
}

//--------------------------------------------------------------------------------------------------
CResponseActor* CResponseSystem::CreateResponseActor(const CHashedString& pActorName, const EntityId userData)
{
	CResponseActor* newActor = new CResponseActor(pActorName, userData);
	m_CreatedActors.push_back(newActor);
	return newActor;
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystem::ReleaseResponseActor(DRS::IResponseActor* pActorToFree)
{
	for (ResponseActorList::iterator it = m_CreatedActors.begin(), itEnd = m_CreatedActors.end(); it != itEnd; ++it)
	{
		if (*it == pActorToFree)
		{
			delete *it;
			m_CreatedActors.erase(it);
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
CResponseActor* CResponseSystem::GetResponseActor(const CHashedString& actorName)
{
	for (CResponseActor* pActor : m_CreatedActors)
	{
		if (pActor->GetName() == actorName)
		{
			return pActor;
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
void CResponseSystem::Reset(uint32 resetHint)
{
	if (m_bIsCurrentlyUpdating)
	{
		//we delay the actual execution until the beginning of the next update, because we do not want the reset to happen, while we are iterating over the currently running responses
		m_pendingResetRequest = resetHint;
	}
	else
	{
		InternalReset(resetHint);
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystem::CancelSignalProcessing(const CHashedString& signalName, DRS::IResponseActor* pSender /* = nullptr */, DRS::SignalInstanceId instanceToSkip /* = s_InvalidSignalId */)
{
	SSignal temp(signalName, static_cast<CResponseActor*>(pSender), nullptr);
	temp.m_id = instanceToSkip;
	return m_pResponseManager->CancelSignalProcessing(temp);
}

//--------------------------------------------------------------------------------------------------
DRS::ActionSerializationClassFactory& CResponseSystem::GetActionSerializationFactory()
{
	return Serialization::ClassFactory<DRS::IResponseAction>::the();
}

//--------------------------------------------------------------------------------------------------
DRS::ConditionSerializationClassFactory& CResponseSystem::GetConditionSerializationFactory()
{
	return Serialization::ClassFactory<DRS::IResponseCondition>::the();
}

//--------------------------------------------------------------------------------------------------
void CResponseSystem::Serialize(Serialization::IArchive& ar)
{
	m_pVariableCollectionManager->Serialize(ar);

	m_pResponseManager->SerializeResponseStates(ar);

	m_CurrentTime.SetSeconds(m_pVariableCollectionManager->GetCollection("Global")->GetVariableValue("CurrentTime").GetValueAsFloat());  //we serialize our DRS time manually via the time variable
}

//--------------------------------------------------------------------------------------------------
DRS::IVariableCollectionSharedPtr CResponseSystem::CreateContextCollection()
{
	static int uniqueNameCounter = 0;
	string uniqueName("Context");
	uniqueName += CryStringUtils::toString(++uniqueNameCounter);

	return std::make_shared<CVariableCollection>(uniqueName);  //drs-todo: pool me
}

//--------------------------------------------------------------------------------------------------
void CResponseSystem::OnSystemEvent(ESystemEvent event, UINT_PTR pWparam, UINT_PTR pLparam)
{
	if (event == ESYSTEM_EVENT_LEVEL_UNLOAD)
	{
		//stop active responses + active speakers on level change
		Reset(DRS::IDynamicResponseSystem::eResetHint_StopRunningResponses | DRS::IDynamicResponseSystem::eResetHint_Speaker);
	}
}

//--------------------------------------------------------------------------------------------------
#if !defined(_RELEASE)
void CResponseSystem::SetCurrentDrsUserName(const char* szNewDrsUserName)
{
	m_currentDrsUserName = szNewDrsUserName;
}
#endif

//--------------------------------------------------------------------------------------------------
void CResponseSystem::InternalReset(uint32 resetFlags)
{
	if (resetFlags & (DRS::IDynamicResponseSystem::eResetHint_StopRunningResponses | DRS::IDynamicResponseSystem::eResetHint_ResetAllResponses))
	{
		m_pResponseManager->Reset((resetFlags& DRS::IDynamicResponseSystem::eResetHint_ResetAllResponses) > 0);
	}
	if (resetFlags & DRS::IDynamicResponseSystem::eResetHint_Variables)
	{
		m_pVariableCollectionManager->Reset();
		m_CurrentTime.SetSeconds(0.0f);
		m_pVariableCollectionManager->GetCollection("Global")->SetVariableValue("CurrentTime", m_CurrentTime.GetSeconds());
	}
	if (resetFlags & DRS::IDynamicResponseSystem::eResetHint_Speaker)
	{
		m_pSpeakerManager->Reset();
		m_pDialogLineDatabase->Reset();
	}
	if (resetFlags & DRS::IDynamicResponseSystem::eResetHint_DebugInfos)
	{
		DRS_DEBUG_DATA_ACTION(Reset());
	}
}

constexpr const char* szDrsDataTag = "<DRS_DATA>";
constexpr const char* szDrsVariablesStartTag = "VariablesStart";
constexpr const char* szDrsVariablesEndTag = "VariablesEnd";
constexpr const char* szDrsResponsesStartTag = "ResponsesStart";
constexpr const char* szDrsResponsesEndTag = "ResponsesEnd";
constexpr const char* szDrsLinesStartTag = "LinesStart";
constexpr const char* szDrsLinesEndTag = "LinesEnd";

//--------------------------------------------------------------------------------------------------
DRS::ValuesListPtr CResponseSystem::GetCurrentState(uint32 saveHints) const
{
	const bool bSkipDefaultValues = (saveHints & SaveHints_SkipDefaultValues) > 0;

	DRS::ValuesListPtr variableList(new DRS::ValuesList(), [](DRS::ValuesList* pList)
	{
		pList->clear();
		delete pList;
	});

	std::pair<DRS::ValuesString, DRS::ValuesString> temp;
	temp.first = szDrsDataTag;
	if (saveHints & SaveHints_Variables)
	{
		temp.second = szDrsVariablesStartTag;
		variableList->push_back(temp);
		m_pVariableCollectionManager->GetAllVariableCollections(variableList.get(), bSkipDefaultValues);
		temp.second = szDrsVariablesEndTag;
		variableList->push_back(temp);
	}

	if (saveHints & SaveHints_ResponseData)
	{
		temp.second = szDrsResponsesStartTag;
		variableList->push_back(temp);
		m_pResponseManager->GetAllResponseData(variableList.get(), bSkipDefaultValues);
		temp.second = szDrsResponsesEndTag;
		variableList->push_back(temp);
	}

	if (saveHints & SaveHints_LineData)
	{
		temp.second = szDrsLinesStartTag;
		variableList->push_back(temp);
		m_pDialogLineDatabase->GetAllLineData(variableList.get(), bSkipDefaultValues);
		temp.second = szDrsLinesEndTag;
		variableList->push_back(temp);
	}

	return variableList;
}

//--------------------------------------------------------------------------------------------------
void CResponseSystem::SetCurrentState(const DRS::ValuesList& collectionsList)
{
	DRS::ValuesListIterator itStartOfVariables = collectionsList.begin();
	DRS::ValuesListIterator itStartOfResponses = collectionsList.begin();
	DRS::ValuesListIterator itStartOfLines = collectionsList.begin();
	DRS::ValuesListIterator itEndOfVariables = collectionsList.begin();
	DRS::ValuesListIterator itEndOfResponses = collectionsList.begin();
	DRS::ValuesListIterator itEndOfLines = collectionsList.begin();

	for (DRS::ValuesListIterator it = collectionsList.begin(); it != collectionsList.end(); ++it)
	{
		if (it->first == szDrsDataTag)
		{
			if (it->second == szDrsVariablesStartTag)
				itStartOfVariables = it + 1;
			if (it->second == szDrsVariablesEndTag)
				itEndOfVariables = it;
		}
	}

	for (DRS::ValuesListIterator it = collectionsList.begin(); it != collectionsList.end(); ++it)
	{
		if (it->first == szDrsDataTag)
		{
			if (it->second == szDrsResponsesStartTag)
				itStartOfResponses = it + 1;
			if (it->second == szDrsResponsesEndTag)
				itEndOfResponses = it;
		}
	}

	for (DRS::ValuesListIterator it = collectionsList.begin(); it != collectionsList.end(); ++it)
	{
		if (it->first == szDrsDataTag)
		{
			if (it->second == szDrsLinesStartTag)
				itStartOfLines = it + 1;
			if (it->second == szDrsLinesEndTag)
				itEndOfLines = it;
		}
	}

	if (itStartOfVariables != itEndOfVariables)
	{
		m_pVariableCollectionManager->SetAllVariableCollections(itStartOfVariables, itEndOfVariables);
		//we serialize our DRS time manually via the "CurrentTime" variable
		m_CurrentTime.SetSeconds(m_pVariableCollectionManager->GetCollection("Global")->GetVariableValue("CurrentTime").GetValueAsFloat());
	}

	if (itStartOfResponses != itEndOfResponses)
	{
		m_pResponseManager->SetAllResponseData(itStartOfResponses, itEndOfResponses);
	}

	if (itStartOfLines != itEndOfLines)
	{
		m_pDialogLineDatabase->SetAllLineData(itStartOfLines, itEndOfLines);
	}
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

CResponseActor::CResponseActor(const CHashedString& name, EntityId usedEntityID)
	: m_linkedEntityID(usedEntityID)
	, m_localVariablesCollectionName(name)
{
}

//--------------------------------------------------------------------------------------------------
CResponseActor::~CResponseActor()
{
	CResponseSystem::GetInstance()->GetSpeakerManager()->OnActorRemoved(this);
	CVariableCollection* pLocalVariables = CResponseSystem::GetInstance()->GetCollection(m_localVariablesCollectionName);
	if (pLocalVariables)
	{
		CResponseSystem::GetInstance()->ReleaseVariableCollection(pLocalVariables);
	}
}

//--------------------------------------------------------------------------------------------------
const CHashedString& CResponseActor::GetName() const
{
	return m_localVariablesCollectionName;
}

//--------------------------------------------------------------------------------------------------
IEntity* CResponseActor::GetLinkedEntity() const
{
	if (m_linkedEntityID == INVALID_ENTITYID)  //if no actor exists, we assume it's a virtual speaker (radio, walkie talkie...) and therefore use the client actor
	{
		if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
		{
			return gEnv->pGame->GetIGameFramework()->GetClientEntity();
		}
		return nullptr;
	}
	// We are using the User Data of the IResponseActor to store the ID of the Entity associated with that Actor.
	return gEnv->pEntitySystem->GetEntity(m_linkedEntityID);
}

//--------------------------------------------------------------------------------------------------
DRS::SignalInstanceId CResponseActor::QueueSignal(const CHashedString& signalName, DRS::IVariableCollectionSharedPtr pSignalContext /*= nullptr*/, DRS::IResponseManager::IListener* pSignalListener /*= nullptr*/)
{
	CResponseManager* pResponseMgr = CResponseSystem::GetInstance()->GetResponseManager();
	SSignal signal(signalName, this, std::static_pointer_cast<CVariableCollection>(pSignalContext));
	if (pSignalListener)
	{
		pResponseMgr->AddListener(pSignalListener, signal.m_id);
	}
	pResponseMgr->QueueSignal(signal);
	return signal.m_id;
}

//--------------------------------------------------------------------------------------------------
CVariableCollection* CResponseActor::GetLocalVariables()
{
	CVariableCollection* pLocalVariables = CResponseSystem::GetInstance()->GetCollection(m_localVariablesCollectionName);
	if (!pLocalVariables)
	{
		pLocalVariables = CResponseSystem::GetInstance()->CreateVariableCollection(m_localVariablesCollectionName);
	}
	return pLocalVariables;
}

//--------------------------------------------------------------------------------------------------
const CVariableCollection* CResponseActor::GetLocalVariables() const
{
	const CVariableCollection* pLocalVariables = CResponseSystem::GetInstance()->GetCollection(m_localVariablesCollectionName);  //the collection might already be created by conditions or actions.
	if (!pLocalVariables)
	{
		pLocalVariables = CResponseSystem::GetInstance()->CreateVariableCollection(m_localVariablesCollectionName);
	}
	return pLocalVariables;
}

namespace DRS
{
SERIALIZATION_CLASS_NULL(IResponseAction, 0);
SERIALIZATION_CLASS_NULL(IResponseCondition, 0);
}
