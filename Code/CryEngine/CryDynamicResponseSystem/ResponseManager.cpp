// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CrySystem/File/ICryPak.h>
#include <CrySerialization/IArchiveHost.h>
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>

#include "ResponseManager.h"
#include "ResponseInstance.h"
#include "ResponseSystemDebugDataProvider.h"
#include "ResponseSystem.h"
#include "VariableCollection.h"
#include "Response.h"
#include "ResponseSystemDataImportHelper.h"

using namespace CryDRS;

string CResponseManager::s_currentSignal;

typedef std::pair<CHashedString, ResponsePtr> ResponsePair;
namespace std {
struct ResponsePairSerializer
{
	ResponsePairSerializer(ResponsePair& pair) : pair_(pair) {}
	void Serialize(Serialization::IArchive& ar)
	{
		ar(pair_.first, "key", "^");
		ar(pair_.second, "value", "^");
	}
	ResponsePair& pair_;
};
bool Serialize(Serialization::IArchive& ar, ResponsePair& pair, const char* szName, const char* szLabel)
{
	ResponsePairSerializer keyValue(pair);
	return ar(keyValue, szName, szLabel);
}
}

//--------------------------------------------------------------------------------------------------
typedef std::pair<string, ResponsePtr> ResponsePairSorted;
namespace std {
struct ResponsePairSortedSerializer
{
	ResponsePairSortedSerializer(ResponsePairSorted& pair) : pair_(pair) {}
	void Serialize(Serialization::IArchive& ar)
	{
		ar(pair_.first, "key", "^");
		if (pair_.first.empty())
		{
			if (ar.isEdit())
			{
				ar.warning(pair_.first, "Response without a signal name detected.");
			}
			return;
		}
		ar(pair_.second, "value", "^");
	}
	ResponsePairSorted& pair_;
};
bool Serialize(Serialization::IArchive& ar, ResponsePairSorted& pair, const char* szName, const char* szLabel)
{
	ResponsePairSortedSerializer keyValue(pair);
	return ar(keyValue, szName, szLabel);
}
}

//--------------------------------------------------------------------------------------------------
CResponseManager::CResponseManager()
{
	m_bCurrentlyUpdating = false;
	m_UsedFileFormat = CResponseManager::eUFF_JSON;
}

//--------------------------------------------------------------------------------------------------
CResponseManager::~CResponseManager()
{
	for (ResponseInstanceList::iterator it = m_runningResponses.begin(); it != m_runningResponses.end(); ++it)
	{
		ReleaseInstance(*it, false);
	}
}

static string s_responseDataPath;
static string s_currentFilePath;

//--------------------------------------------------------------------------------------------------
bool CResponseManager::_LoadFromFiles(const string& dataPath)
{
	string searchPath = dataPath + string("*.*");
	_finddata_t fd;
	ICryPak* pCryPak = gEnv->pCryPak;
	intptr_t handle = pCryPak->FindFirst(searchPath.c_str(), &fd);
	if (handle != -1)
	{
		do
		{
			const string sName = fd.name;
			if (sName != "." && sName != ".." && !sName.empty())
			{
				if (fd.attrib & _A_SUBDIR)
				{
					_LoadFromFiles(dataPath + sName + '/');
				}
				else
				{
					s_currentFilePath = dataPath + sName;
					if (m_UsedFileFormat == eUFF_JSON)
					{
						Serialization::LoadJsonFile(*this, s_currentFilePath);
					}
					else if (m_UsedFileFormat == eUFF_XML)
					{
						Serialization::LoadXmlFile(*this, s_currentFilePath);
					}
					else if (m_UsedFileFormat == eUFF_BIN)
					{
						Serialization::LoadBinaryFile(*this, s_currentFilePath);
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);
		pCryPak->FindClose(handle);
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
bool CResponseManager::LoadFromFiles(const char* szDataPath)
{
	s_responseDataPath = PathUtil::AddSlash(szDataPath);

	m_mappedSignals.clear();

	return _LoadFromFiles(s_responseDataPath);
}

//--------------------------------------------------------------------------------------------------
bool CResponseManager::SaveToFiles(const char* szDataPath)
{
	const string outFolder = szDataPath;
	for (MappedSignals::iterator it = m_mappedSignals.begin(); it != m_mappedSignals.end(); ++it)
	{
		if (m_UsedFileFormat == eUFF_JSON)
		{
			string outfile = outFolder + it->first.GetText() + ".response";
			Serialization::SaveJsonFile(outfile, Serialization::SStruct(*it->second));
		}
		else if (m_UsedFileFormat == eUFF_XML)
		{
			string outfile = outFolder + it->first.GetText() + ".xml";
			Serialization::SaveXmlFile(outfile, Serialization::SStruct(*it->second), "responses");
		}
		else if (m_UsedFileFormat == eUFF_BIN)
		{
			string outfile = outFolder + it->first.GetText() + ".bin";
			Serialization::SaveBinaryFile(outfile, Serialization::SStruct(*it->second));
		}
	}
	return true;
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::QueueSignal(const SSignal& signal)
{
	DRS_DEBUG_DATA_ACTION(AddSignalFired(signal.m_signalName.GetText(), (signal.m_pSender) ? signal.m_pSender->GetName().GetText() : "no sender", (signal.m_pSignalContext) ? signal.m_pSignalContext->GetVariablesAsString() : "(no context variables)"));

	if (!m_bCurrentlyUpdating)
	{
		m_currentlyQueuedSignals.push_back(signal);
	}
	else
	{
		m_currentlyQueuedSignalsDuringUpdate.push_back(signal);
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::CancelSignalProcessing(const SSignal& signal)
{
	m_currentlySignalsWaitingToBeCanceled.push_back(signal);
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::Update()
{
	float currentTime = CResponseSystem::GetInstance()->GetCurrentDrsTime();

	for (SignalList::iterator itToCancel = m_currentlySignalsWaitingToBeCanceled.begin(); itToCancel != m_currentlySignalsWaitingToBeCanceled.end(); )
	{
		SSignal& signalToCancel = *itToCancel;
		for (SignalList::iterator itQueued = m_currentlyQueuedSignals.begin(); itQueued != m_currentlyQueuedSignals.end(); )
		{
			SSignal& currentSignal = *itQueued;
			if ((currentSignal.m_pSender == signalToCancel.m_pSender || signalToCancel.m_pSender == nullptr) && currentSignal.m_signalName == signalToCancel.m_signalName)
			{
				InformListenerAboutSignalProcessingFinished(currentSignal.m_signalName, currentSignal.m_pSender, currentSignal.m_pSignalContext, currentSignal.m_id, nullptr, DRS::IResponseManager::IListener::ProcessingResult_Canceled);
				itQueued = m_currentlyQueuedSignals.erase(itQueued);
			}
			else
			{
				++itQueued;
			}
		}

		for (CResponseInstance* runningResponse : m_runningResponses)
		{
			if ((runningResponse->GetCurrentActor() == signalToCancel.m_pSender || signalToCancel.m_pSender == nullptr) && runningResponse->GetSignalName() == signalToCancel.m_signalName)
			{
				runningResponse->Cancel();
			}
		}

		itToCancel = m_currentlySignalsWaitingToBeCanceled.erase(itToCancel);
	}

	for (ResponseInstanceList::iterator it = m_runningResponses.begin(); it != m_runningResponses.end(); )
	{
		if ((*it)->Update())
		{
			++it;
		}
		else
		{
			InformListenerAboutSignalProcessingFinished((*it)->GetSignalName(), (*it)->GetOriginalSender(), (*it)->GetContextVariablesImpl(), (*it)->GetSignalInstanceId(), *it, IResponseManager::IListener::ProcessingResult_Done);
			ReleaseInstance(*it, false);
			it = m_runningResponses.erase(it);
		}
	}

	m_bCurrentlyUpdating = true;

	for (SignalList::iterator itQueued = m_currentlyQueuedSignals.begin(); itQueued != m_currentlyQueuedSignals.end(); )
	{
		SSignal& currentSignal = *itQueued;
		//find the mapped Response for the Signal
		MappedSignals::const_iterator mappedResponse = m_mappedSignals.find(currentSignal.m_signalName);

		if (mappedResponse != m_mappedSignals.end())
		{
			//drs-todo: handle queuing for a specified amount of time. Check if the response can be started right now, based on priorities of already running responses
			CResponseInstance* pResponseInstance = mappedResponse->second->StartExecution(currentSignal);
			InformListenerAboutSignalProcessingStarted(*itQueued, pResponseInstance);
			if (!pResponseInstance) //no instance = conditions not met, we are done
			{
				InformListenerAboutSignalProcessingFinished(currentSignal.m_signalName, currentSignal.m_pSender, currentSignal.m_pSignalContext, currentSignal.m_id, pResponseInstance, DRS::IResponseManager::IListener::ProcessingResult_ConditionsNotMet);
			}
		}
		else
		{
			InformListenerAboutSignalProcessingStarted(*itQueued, nullptr);
			DRS_DEBUG_DATA_ACTION(AddResponseStarted(currentSignal.m_signalName.GetText()));
			DRS_DEBUG_DATA_ACTION(AddResponseInstanceFinished(CResponseSystemDebugDataProvider::eER_NoResponse));
			InformListenerAboutSignalProcessingFinished(currentSignal.m_signalName, currentSignal.m_pSender, currentSignal.m_pSignalContext, currentSignal.m_id, nullptr, DRS::IResponseManager::IListener::ProcessingResult_NoResponseDefined);
		}
		itQueued = m_currentlyQueuedSignals.erase(itQueued);  //we finished handling this signal, therefore we can remove it from the queued-list
	}

	m_bCurrentlyUpdating = false;

	if (!m_currentlyQueuedSignalsDuringUpdate.empty())
	{
		//we might have received queueSignals while updating, we now store these to our list for the next update
		for (const SSignal& queuedSignal : m_currentlyQueuedSignalsDuringUpdate)
		{
			m_currentlyQueuedSignals.push_back(queuedSignal);
		}
		m_currentlyQueuedSignalsDuringUpdate.clear();
	}
}

//--------------------------------------------------------------------------------------------------
CResponseInstance* CResponseManager::CreateInstance(SSignal& signal, CResponse* pResponse)
{
	CResponseInstance* newInstance = new CResponseInstance(signal, pResponse);
	m_runningResponses.push_back(newInstance);
	DRS_DEBUG_DATA_ACTION(AddResponseInstanceCreated(newInstance));

	return newInstance;
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::ReleaseInstance(CResponseInstance* pInstance, bool removeFromRunningInstances /* = true */)
{
	DRS_DEBUG_DATA_ACTION(AddResponseInstanceFinished(CResponseSystemDebugDataProvider::eER_Finished));

	if (removeFromRunningInstances)
	{
		for (ResponseInstanceList::iterator it = m_runningResponses.begin(); it != m_runningResponses.end(); ++it)
		{
			if (pInstance == *it)
			{
				m_runningResponses.erase(it);
				delete pInstance;
				return;
			}
		}
		DrsLogError("Someone tried to delete an ResponseInstance through the DynamicResponseManager that was not managed by the DynamicResponseManager!");
	}
	else
	{
		delete pInstance;
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::Reset(bool bResetExecutionCounter)
{
	m_currentlyQueuedSignals.clear();
	m_currentlyQueuedSignalsDuringUpdate.clear();

	for (ResponseInstanceList::iterator it = m_runningResponses.begin(); it != m_runningResponses.end(); ++it)
	{
		(*it)->Cancel();
		(*it)->Update();
		ReleaseInstance(*it, false);
	}

	if (bResetExecutionCounter)
	{
		for (MappedSignals::iterator it = m_mappedSignals.begin(); it != m_mappedSignals.end(); ++it)
		{
			it->second->Reset();
		}
	}

	m_runningResponses.clear();
}

//--------------------------------------------------------------------------------------------------
namespace std
{
bool Serialize(Serialization::IArchive& ar, ResponsePtr& ptr, const char* szName, const char* szLabel)
{
	if (!ptr.get())
	{
		ptr.reset(new CResponse());
	}
	ar(*ptr.get(), szName, szLabel);
	return true;
}
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::Serialize(Serialization::IArchive& ar)
{
	if (!m_runningResponses.empty() && ar.isInput())  //easiest way to make sure, we are not modifying responses currently running -> stop all running
	{
		for (ResponseInstanceList::iterator it = m_runningResponses.begin(); it != m_runningResponses.end(); ++it)
		{
			SET_DRS_USER_SCOPED("Reset because of ResponseManager::Serialize");
			(*it)->Cancel();
			(*it)->Update();
			ReleaseInstance(*it, false);
		}
		m_runningResponses.clear();
	}

	if ((ar.getFilter() & eSH_ReplaceAllExisting) > 0 || ar.caps(Serialization::IArchive::BINARY))
	{
		if (ar.isEdit())
		{
			typedef std::map<string, ResponsePtr> SortedMappedSignal;
			SortedMappedSignal sortedSignals;
			for (MappedSignals::iterator it = m_mappedSignals.begin(); it != m_mappedSignals.end(); ++it)
			{
				sortedSignals[it->first.GetText()] = std::move(it->second);
			}

			for (SortedMappedSignal::iterator it = sortedSignals.begin(); it != sortedSignals.end(); ++it)
			{
				ar(*it->second, it->first.c_str(), it->first.c_str());
			}

			m_mappedSignals.clear();
			for (SortedMappedSignal::iterator it = sortedSignals.begin(); it != sortedSignals.end(); ++it)
			{
				m_mappedSignals[it->first] = std::move(it->second);
			}
		}
		else
		{
			ar(m_mappedSignals, "signals_responses", "+Signal Responses:");
		}
	}
	else
	{
		s_currentSignal = PathUtil::GetFileName(s_currentFilePath);
		m_mappedSignals[s_currentSignal] = ResponsePtr(new CResponse);
		m_mappedSignals[s_currentSignal]->Serialize(ar);
		s_currentSignal.clear();
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::SetFileFormat(EUsedFileFormat format)
{
	m_UsedFileFormat = format;
}

//--------------------------------------------------------------------------------------------------
DynArray<const char*> CResponseManager::GetRecentSignals(DRS::IResponseManager::eSignalFilter filter /* = DRS::IResponseManager::eSF_All */)
{
	DynArray<const char*> temp;
	DRS_DEBUG_DATA_ACTION(GetRecentSignals(temp, filter));
	return temp;
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::SerializeResponse(Serialization::IArchive& ar, const DynArray<stack_string>& signalNames, DRS::IResponseActor* pActorForEvaluation)
{
#if !defined(_RELEASE)
	CConditionsCollection::s_pActorForEvaluation = pActorForEvaluation;
#endif

	if (signalNames.empty())
	{
		if (ar.isEdit() || ar.caps(Serialization::IArchive::BINARY))
		{
			ar.setFilter(ar.getFilter() | eSH_ReplaceAllExisting);
			return Serialize(ar);
		}
		else
		{
			SaveToFiles(s_responseDataPath);
			return;
		}
	}

	for (const string signalName : signalNames)
	{
		const CHashedString hashedSignalName = signalName;
		bool bFound = false;
		for (MappedSignals::iterator it = m_mappedSignals.begin(); it != m_mappedSignals.end(); ++it)
		{
			if (it->first == hashedSignalName)
			{
				if (ar.isInput() && !m_runningResponses.empty()) //stop the response first, if currently running
				{
					for (ResponseInstanceList::iterator itRunning = m_runningResponses.begin(); itRunning != m_runningResponses.end(); )
					{
						if ((*itRunning)->GetSignalName() == hashedSignalName)
						{
							(*itRunning)->Cancel();
							(*itRunning)->Update();
							ReleaseInstance(*itRunning, false);
							itRunning = m_runningResponses.erase(itRunning);
						}
						else
						{
							++itRunning;
						}
					}
				}

				bFound = true;
				s_currentSignal = signalName;
				bool bBlockOpened = false;
				if (ar.isEdit())
				{
					bBlockOpened = ar.openBlock("Signal", " ");
					ar(it->first.GetText(), "response-name", "!^^< RESPONSE TREE for SIGNAL ");
				}
				it->second->Serialize(ar);
				s_currentSignal.clear();

				if (bBlockOpened)
				{
					ar.closeBlock();
				}
			}
		}
		if (!bFound && ar.isEdit())
		{
			ar(signalName, "NoResponse", "!No Response defined for:");
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::SerializeRecentResponse(Serialization::IArchive& ar, const DynArray<stack_string>& signalNames, int maxElemets /* = -1 */)
{
	DRS_DEBUG_DATA_ACTION(SerializeRecentResponse(ar, signalNames, maxElemets));
}

//--------------------------------------------------------------------------------------------------
bool CResponseManager::AddResponse(const stack_string& signalName)
{
	string copyString = signalName;
	if (m_mappedSignals.find(copyString) != m_mappedSignals.end())
		return false;
	m_mappedSignals[copyString] = ResponsePtr(new CResponse);
	return true;
}

//--------------------------------------------------------------------------------------------------
bool CResponseManager::RemoveResponse(const stack_string& signalName)
{
	const CHashedString hashedSignalName = signalName.c_str();

	if (!m_runningResponses.empty()) //stop the response first, if currently running
	{
		for (ResponseInstanceList::iterator it = m_runningResponses.begin(); it != m_runningResponses.end(); )
		{
			if ((*it)->GetSignalName() == hashedSignalName)
			{
				(*it)->Cancel();
				(*it)->Update();
				ReleaseInstance(*it, false);
				it = m_runningResponses.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	auto foundIT = m_mappedSignals.find(hashedSignalName);
	if (foundIT != m_mappedSignals.end())
	{
		m_mappedSignals.erase(foundIT);
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::SerializeVariableChanges(Serialization::IArchive& ar, const stack_string& variableName /*= "ALL"*/, int maxElemets /*= -1*/)
{
	DRS_DEBUG_DATA_ACTION(SerializeVariableChanges(ar, variableName.c_str(), maxElemets));
}

//--------------------------------------------------------------------------------------------------
ResponsePtr CResponseManager::GetResponse(const CHashedString& signalName)
{
	MappedSignals::iterator foundIT = m_mappedSignals.find(signalName);
	if (foundIT != m_mappedSignals.end())
	{
		return foundIT->second;
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
bool CResponseManager::AddListener(DRS::IResponseManager::IListener* pNewListener, DRS::SignalInstanceId signalID /* = DRS::s_InvalidSignalID */)
{
	m_Listener.push_back(std::make_pair(pNewListener, signalID));
	return true;
}

//--------------------------------------------------------------------------------------------------
bool CResponseManager::RemoveListener(DRS::IResponseManager::IListener* pListenerToRemove)
{
	for (ListenerList::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it)
	{
		if (it->first == pListenerToRemove)
		{
			m_Listener.erase(it);
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::InformListenerAboutSignalProcessingStarted(const SSignal& signal, DRS::IResponseInstance* pInstance)
{
	DRS::IResponseManager::IListener::SSignalInfos signalInfo(signal.m_signalName, signal.m_pSender, signal.m_pSignalContext, signal.m_id);

	for (std::pair<DRS::IResponseManager::IListener*, DRS::SignalInstanceId>& current : m_Listener)
	{
		if (current.second == signal.m_id || current.second == DRS::s_InvalidSignalId)
		{
			current.first->OnSignalProcessingStarted(signalInfo, pInstance);
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::InformListenerAboutSignalProcessingFinished(
  const CHashedString& signalName,
  CResponseActor* pSender,
  const VariableCollectionSharedPtr& pSignalContext,
  const DRS::SignalInstanceId signalID,
  DRS::IResponseInstance* pInstance,
  DRS::IResponseManager::IListener::eProcessingResult outcome)
{
	DRS::IResponseManager::IListener::SSignalInfos signalInfo(signalName, pSender, pSignalContext, signalID);

	for (ListenerList::iterator it = m_Listener.begin(); it != m_Listener.end(); )
	{
		if (it->second == signalID)
		{
			it->first->OnSignalProcessingFinished(signalInfo, pInstance, outcome);
			it = m_Listener.erase(it);  //the processing for this signalId has finished, no need to keep the listener around.
		}
		else
		{
			if (it->second == DRS::s_InvalidSignalId)
			{
				it->first->OnSignalProcessingFinished(signalInfo, pInstance, outcome);
			}
			++it;
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseManager::SerializeResponseStates(Serialization::IArchive& ar)
{
	struct responseStateInfo
	{
		CHashedString responsename;
		uint32        executionCount;
		float         lastStartTime;
		float         lastEndTime;
		void          Serialize(Serialization::IArchive& ar)
		{
			if (lastEndTime < lastStartTime)  //check if currently running
			{
				ar(responsename, "name", "^Name");
			}
			else
			{
				ar(responsename, "name", "^!Name");
			}
			ar(executionCount, "executionCount", "^>70>Executions");
			ar(lastStartTime, "LastStart", "^>70>LastStart");
			ar(lastEndTime, "LastEnd", "^>70>LastEnd");
		}
	};
	std::vector<responseStateInfo> responseStates;

	if (ar.isInput())
	{
		ar(responseStates, "ResponseStates", "-ResponseStates");
		for (responseStateInfo& current : responseStates)
		{
			ResponsePtr pResponse = GetResponse(current.responsename);
			if (pResponse)
			{
				pResponse->SetExecutionCounter(current.executionCount);
				pResponse->SetLastStartTime(current.lastStartTime);
				pResponse->SetLastEndTime(current.lastEndTime);
			}
		}
	}
	else
	{
		int i = 0;
		responseStates.resize(m_mappedSignals.size());
		for (MappedSignals::iterator it = m_mappedSignals.begin(); it != m_mappedSignals.end(); ++it)
		{
			responseStates[i].responsename = it->first;
			responseStates[i].executionCount = it->second->GetExecutionCounter();
			responseStates[i].lastStartTime = it->second->GetLastStartTime();
			responseStates[i].lastEndTime = it->second->GetLastEndTime();
			++i;
		}
		ar(responseStates, "ResponseStates", "ResponseStates");
	}
}

//--------------------------------------------------------------------------------------------------
static int s_currentSignalId = 0;

SSignal::SSignal(
  const CHashedString& signalName,
  CResponseActor* pSender,
  VariableCollectionSharedPtr pSignalContext)
	: m_signalName(signalName)
	, m_pSender(pSender)
	, m_pSignalContext(pSignalContext)
{
	m_id = static_cast<DRS::SignalInstanceId>(++s_currentSignalId);
}
