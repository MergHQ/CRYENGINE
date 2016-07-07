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
	m_pCurrentTimeVariable = m_pVariableCollectionManager->GetCollection("Global")->CreateVariable("CurrentTime", m_CurrentTime.GetSeconds());

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
		_Reset(m_pendingResetRequest);
		m_pendingResetRequest = DRS::IDynamicResponseSystem::eResetHint_Nothing;
	}

	m_CurrentTime += gEnv->pTimer->GetFrameTime();
	m_pCurrentTimeVariable->SetValue(m_CurrentTime.GetSeconds());

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
CResponseActor* CResponseSystem::CreateResponseActor(const CHashedString& pActorName, const uint userData)
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
		_Reset(resetHint);
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystem::CancelSignalProcessing(const CHashedString& signalName, DRS::IResponseActor* pSender)
{
	return m_pResponseManager->CancelSignalProcessing(SSignal(signalName, static_cast<CResponseActor*>(pSender), nullptr));
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

	m_CurrentTime.SetSeconds(m_pCurrentTimeVariable->GetValueAsFloat());  //we serialize our DRS time manually via the time variable
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
void CResponseSystem::_Reset(uint32 resetFlags)
{
	if (resetFlags & DRS::IDynamicResponseSystem::eResetHint_StopRunningResponses | DRS::IDynamicResponseSystem::eResetHint_ResetAllResponses)
	{
		m_pResponseManager->Reset((resetFlags& DRS::IDynamicResponseSystem::eResetHint_ResetAllResponses) > 0);
	}
	if (resetFlags & DRS::IDynamicResponseSystem::eResetHint_Variables)
	{
		m_pVariableCollectionManager->Reset();
		m_CurrentTime.SetSeconds(0.0f);
		m_pCurrentTimeVariable->SetValue(m_CurrentTime.GetSeconds());
	}
	if (resetFlags & DRS::IDynamicResponseSystem::eResetHint_Speaker)
	{
		m_pSpeakerManager->Reset();
	}
	if (resetFlags & DRS::IDynamicResponseSystem::eResetHint_DebugInfos)
	{
		DRS_DEBUG_DATA_ACTION(Reset());
	}
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

CResponseActor::CResponseActor(const CHashedString& name, EntityId usedEntityID)
	: m_linkedEntityID(usedEntityID)
{
	m_pLocalVariables = CResponseSystem::GetInstance()->GetCollection(name);  //the collection might already be created by conditions or actions.
	if (!m_pLocalVariables)
	{
		m_pLocalVariables = CResponseSystem::GetInstance()->CreateVariableCollection(name);
	}
}

//--------------------------------------------------------------------------------------------------
CResponseActor::~CResponseActor()
{
	CResponseSystem::GetInstance()->GetSpeakerManager()->OnActorRemoved(this);
	CResponseSystem::GetInstance()->ReleaseVariableCollection(m_pLocalVariables);
}

//--------------------------------------------------------------------------------------------------
const CHashedString& CResponseActor::GetName() const
{
	return m_pLocalVariables->GetName();
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
DRS::SignalId CResponseActor::QueueSignal(const CHashedString& signalName, DRS::IVariableCollectionSharedPtr pSignalContext /*= nullptr*/, DRS::IResponseManager::IListener* pSignalListener /*= nullptr*/)
{
	CResponseManager* pResponseMgr = CResponseSystem::GetInstance()->GetResponseManager();
	SSignal signal(signalName, this, std::static_pointer_cast<CVariableCollection>(pSignalContext));
	if (pSignalListener)
	{
		pResponseMgr->AddListener(pSignalListener, signal.m_id);
	}
	CResponseSystem::GetInstance()->GetResponseManager()->QueueSignal(signal);
	return signal.m_id;
}

namespace DRS
{
SERIALIZATION_CLASS_NULL(IResponseAction, 0);
SERIALIZATION_CLASS_NULL(IResponseCondition, 0);
}
