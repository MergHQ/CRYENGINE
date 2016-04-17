// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ResponseSystemDebugDataProvider.h"
#include "Response.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryString/StringUtils.h>
#include "ConditionImpl.h"
#include "ResponseSystem.h"

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::AddVariableSet(const string& variableName, const string& collectionName, const CVariableValue& oldValue, const CVariableValue& newValue, float time)
{
	if (strncmp(collectionName.c_str(), "Context", 7) == 0)
	{
		return;  //We dont want our list polluted by these...
	}

	if (oldValue.GetType() == eDRVT_Undefined && newValue.GetType() == eDRVT_Undefined)
	{
		return;
	}

	if (newValue == oldValue && oldValue.GetType() != eDRVT_Undefined)
	{
		return;
	}

	VariableChangeInfo newEntry;
	newEntry.timeOfChange = time;
	newEntry.variableName = collectionName + "::" + variableName;
	newEntry.change = "'" + oldValue.GetValueAsString() + "' to '" + newValue.GetValueAsString() + "', Type: " + newValue.GetTypeAsString();
	newEntry.drsUserName = CResponseSystem::GetInstance()->GetCurrentDrsUserName();

	//hard coded limit
	if (m_VariableChangeHistory.size() > 256)
	{
		m_VariableChangeHistory.erase(m_VariableChangeHistory.begin());
	}
	m_VariableChangeHistory.push_back(newEntry);
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::AddResponseStarted(const string& signalName)
{
	const size_t numExecutedResponses = m_ExecutedResponses.size();
	for (int i = 0; i < numExecutedResponses; i++)
	{
		SStartedResponses& current = m_ExecutedResponses[i];
		if (current.currentState == CResponseSystemDebugDataProvider::eER_Queued && current.signalName.compareNoCase(signalName) == 0)
		{
			current.currentState = CResponseSystemDebugDataProvider::eER_Running;
			m_CurrentResponse = current.id;
			return;
		}
	}
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystemDebugDataProvider::AddResponseSegmentStarted(CResponseSegment* pResponseSegment)
{
	if (m_CurrentResponse >= m_ExecutedResponses.size())
	{
		return false;
	}

	SStartedResponses& response = m_ExecutedResponses[m_CurrentResponse];

	for (auto& segment : response.responseSegments)
	{
		segment.bRunning = false;
	}
	SStartedResponsesSegment newResponseSegment;
	newResponseSegment.bStarted = false;
	newResponseSegment.bRunning = false;
	newResponseSegment.segmentName = "Response: '";
	newResponseSegment.segmentName += pResponseSegment->GetName();
	newResponseSegment.segmentName += "' ";
	newResponseSegment.pResponseSegment = pResponseSegment;
	newResponseSegment.levelInHierarchy = response.currentlevelInHierarchy;
	response.responseSegments.push_back(newResponseSegment);
	return true;
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystemDebugDataProvider::AddConditionChecked(const CConditionsCollection::SConditionInfo* pCondition, bool result)
{
	if (m_CurrentResponse >= m_ExecutedResponses.size())
	{
		return false;
	}

	SStartedResponses& response = m_ExecutedResponses[m_CurrentResponse];
	SCheckedCondition newCondition;
	newCondition.bMet = result;
	if (pCondition->m_bNegated)
	{
		newCondition.conditionDesc = "NOT ";
	}
	IVariableUsingBase::s_bDoDisplayCurrentValueInDebugOutput = true;
	newCondition.conditionDesc += pCondition->m_pCondition->GetVerboseInfoWithType();
	IVariableUsingBase::s_bDoDisplayCurrentValueInDebugOutput = false;
	response.responseSegments.back().checkedConditions.push_back(newCondition);

	return true;
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystemDebugDataProvider::AddActionStarted(const string& actionDesc, DRS::IResponseActionInstance* pInstance, DRS::IResponseActor* pActor, CResponseSegment* pResponseSegment)
{
	if (m_CurrentResponse >= m_ExecutedResponses.size())
	{
		return false;
	}
	SExecutedAction newAction;
	newAction.actorName = (pActor) ? pActor->GetName().GetText() : "NoActor";
	newAction.actionDesc = actionDesc;
	newAction.pInstance = pInstance;
	newAction.bEnded = false;
	SStartedResponses& response = m_ExecutedResponses[m_CurrentResponse];
	for (std::vector<SStartedResponsesSegment>::iterator it = response.responseSegments.begin(); it != response.responseSegments.end(); ++it)
	{
		if (pResponseSegment == it->pResponseSegment)
		{
			it->bRunning = true;
			it->bStarted = true;
			it->executedAction.push_back(newAction);
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystemDebugDataProvider::AddActionFinished(DRS::IResponseActionInstance* pInstance)
{
	if (m_CurrentResponse >= m_ExecutedResponses.size())
	{
		return false;
	}
	SStartedResponses& response = m_ExecutedResponses[m_CurrentResponse];
	for (std::vector<SStartedResponsesSegment>::iterator itEnd = response.responseSegments.begin(); itEnd != response.responseSegments.end(); ++itEnd)
	{
		for (std::vector<SExecutedAction>::iterator it = itEnd->executedAction.begin(); it != itEnd->executedAction.end(); ++it)
		{
			if (it->pInstance == pInstance && !it->bEnded)
			{
				it->bEnded = true;
				break;
			}
		}
	}
	return true;
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystemDebugDataProvider::AddResponseInstanceFinished(EStatus reason)
{
	if (m_CurrentResponse >= m_ExecutedResponses.size())
	{
		return false;
	}
	SStartedResponses& response = m_ExecutedResponses[m_CurrentResponse];

	if (response.currentState != eER_Canceled)  //we dont want to change from canceled to finished
	{
		response.currentState = reason;
	}

	if (reason != eER_Canceled)  //we are waiting for the actual 'finished' to remove it from our tracking list
	{
		for (SStartedResponsesSegment& current : response.responseSegments)
		{
			current.bRunning = false;
		}

		for (auto it = m_InstanceToID.begin(); it != m_InstanceToID.end(); ++it)
		{
			if (it->second == m_CurrentResponse)
			{
				m_InstanceToID.erase(it);
				m_CurrentResponse = MAX_NUMBER_OF_TRACKED_SIGNALS + 1;
				break;
			}
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
SERIALIZATION_ENUM_BEGIN_NESTED(CResponseSystemDebugDataProvider, EStatus, "EndReason")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_ExecuteOnlyOnce, "ExecuteOnlyOnce", "NotStarted - ExecuteOnlyOnce")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_NoValidSegment, "NoValidSegment", "NotStarted - No Valid Responses")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_Canceled, "Canceled", "Canceled")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_Finished, "Finished", "Finished")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_NotStarted, "NotStarted", "NotStarted")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_Running, "Running", "Running")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_NoResponse, "NoResponse", "NotStarted - No Response")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_Queued, "Queued", "Queued")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_CanceledWhileQueued, "CanceledWhileQueued", "Canceled - while queued")
SERIALIZATION_ENUM(CResponseSystemDebugDataProvider::eER_Canceling, "Canceling", "Currently Canceling...")
SERIALIZATION_ENUM_END()

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SStartedResponses::Serialize(Serialization::IArchive& ar)
{
	int numStartedResponses = 0;

	bool bRunning = currentState == CResponseSystemDebugDataProvider::eER_Running;
	ar(bRunning, "running", "!^Running:");
	ar(timeOfEvent, "time", "^>70> Time:");
	ar(signalName, "signal", "^<Signal:");

	if (ar.openBlock("signalInfos", "- "))
	{
		ar(currentState, "status", "^^>150>Status:");
		ar(drsUserName, "source", "Source:");
		ar(senderName, "sender", "^^Actor:");
		ar(contextVariables, "ContextVariables", "!ContextVariables");
		ar.closeBlock();
	}

	if (ar.isEdit())
	{
		for (std::vector<SStartedResponsesSegment>::const_iterator it = responseSegments.begin(); it != responseSegments.end(); ++it)
		{
			if (it->bStarted)
			{
				++numStartedResponses;
			}
		}
		ar(CryStringUtils::toString(numStartedResponses), "responses", "^StartedResponses:");
	}

	int currentBlockDepth = 0;
	for (SStartedResponsesSegment& segment : responseSegments)
	{
		if (currentBlockDepth < segment.levelInHierarchy)
		{
			if (ar.openBlock("FollowUp", "+Follow Up Responses"))
				currentBlockDepth++;
		}

		ar(segment, "segment", segment.segmentName);
	}
	for (int i = 0; i < currentBlockDepth; i++)
	{
		ar.closeBlock();
	}

	static string seperator = "______________________________________________________________________________________________________________________________________________________________________________________";
	ar(seperator, "seperator", "!< ");
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SStartedResponsesSegment::Serialize(Serialization::IArchive& ar)
{
	if (bStarted)
	{
		static string executedLabel = "(EXECUTED)";
		ar(executedLabel, "started", "!^^>80>");
	}

	if (checkedConditions.empty())
	{
		ar(checkedConditions, "checkedConditions", "^! No conditions");
	}
	else
	{
		bool bConditionsMet = true;
		for (SCheckedCondition& currentCondition : checkedConditions)
		{
			if (!currentCondition.bMet)
			{
				bConditionsMet = false;
				break;
			}
		}
		ar(bConditionsMet, "conditionsMet", "!>85>^ ConditionsMet");
		ar(checkedConditions, "checkedConditions", "+CheckedConditions");
	}

	if (!executedAction.empty())
	{
		ar(executedAction, "executedAction", "!+<ExecutedAction");
	}
	else
	{
		ar(executedAction, "executedAction", "!No actions executed");
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SExecutedAction::Serialize(Serialization::IArchive& ar)
{
	ar(bEnded, "hasEnded", "^!Done");
	ar(actionDesc, "actionDesc", "<^");
	ar(actorName, "actor", "^Actor:");
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SCheckedCondition::Serialize(Serialization::IArchive& ar)
{
	ar(bMet, "wasMet", "^^!Was Met");
	ar(conditionDesc, "conditionDesc", "!^Condition:");
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SetCurrentResponseInstance(CResponseInstance* pInstanceForCurrentID)
{
	ResponseInstanceToResponseInstanceIDMapping::iterator foundIT = m_InstanceToID.find(pInstanceForCurrentID);
	if (foundIT == m_InstanceToID.end())
	{
		m_CurrentResponse = MAX_NUMBER_OF_TRACKED_SIGNALS + 1;
	}
	else
	{
		m_CurrentResponse = foundIT->second;
	}
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystemDebugDataProvider::AddResponseInstanceCreated(CResponseInstance* pInstanceForCurrentID)
{
	m_InstanceToID[pInstanceForCurrentID] = m_CurrentResponse;
	return true;
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::Reset()
{
	m_ExecutedResponses.clear();
	m_VariableChangeHistory.clear();
	m_InstanceToID.clear();
	m_LineHistory.clear();
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::VariableChangeInfo::Serialize(Serialization::IArchive& ar)
{
	ar(timeOfChange, "time", "!^Time");
	ar(variableName, "variableName", "!^Variable");
	ar(change, "change", "!^< Changed from");
	if (!drsUserName.empty())
	{
		ar(drsUserName, "Changer", "!^< Source");
	}
	else
	{
		ar(drsUserName, "Changer", "!^< Source Unknown");
	}
}

//--------------------------------------------------------------------------------------------------
CResponseSystemDebugDataProvider::CResponseSystemDebugDataProvider() : m_CurrentResponse(MAX_NUMBER_OF_TRACKED_SIGNALS + 1)
{
}

//--------------------------------------------------------------------------------------------------
CResponseSystemDebugDataProvider::~CResponseSystemDebugDataProvider()
{
	CResponseSystem::GetInstance()->GetSpeakerManager()->RemoveListener(this);
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::OnLineEvent(const DRS::IResponseActor* pSpeaker, const CHashedString& lineID, eLineEvent lineEvent, const DRS::IDialogLine* pLine)
{
	string speakerName = (pSpeaker) ? pSpeaker->GetName().GetText() : "Missing Speaker";
	string lineText = (pLine) ? pLine->GetText() : "Missing: " + lineID.GetText();

	switch (lineEvent)
	{
	case DRS::ISpeakerManager::IListener::eLineEvent_Started:
		AddDialogLineStarted(lineID, lineText, speakerName, eER_Running, "");
		break;
	case DRS::ISpeakerManager::IListener::eLineEvent_Queued:
		AddDialogLineStarted(lineID, lineText, speakerName, eER_Queued, "Another line needs to finish first");
		break;
	case DRS::ISpeakerManager::IListener::eLineEvent_SkippedBecauseOfPriority:
		AddDialogLineStarted(lineID, lineText, speakerName, eER_NotStarted, "line ('" + lineText + "') with same/higher priority was already playing");
		break;
	case DRS::ISpeakerManager::IListener::eLineEvent_CouldNotBeStarted:
		AddDialogLineStarted(lineID, lineText, speakerName, eER_NotStarted, "missing entity for speaker");
		break;
	case DRS::ISpeakerManager::IListener::eLineEvent_Finished:
		AddDialogLineFinished(lineID, speakerName, eER_Finished, "Done");
		break;
	case DRS::ISpeakerManager::IListener::eLineEvent_Canceling:
		AddDialogLineFinished(lineID, speakerName, eER_Canceling, "Waiting for stop trigger");
		break;
	case DRS::ISpeakerManager::IListener::eLineEvent_Canceled:
		AddDialogLineFinished(lineID, speakerName, eER_Canceled, "Canceled or actor removed");
		break;
	case DRS::ISpeakerManager::IListener::eLineEvent_CanceledWhileQueued:
		AddDialogLineFinished(lineID, speakerName, eER_CanceledWhileQueued, "Canceled or actor removed while queued");
		break;
	}
	CResponseSystem::GetInstance()->GetDialogLineDatabase()->GetLineByID(lineID);
	return;
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::Init()
{
	CResponseSystem::GetInstance()->GetSpeakerManager()->AddListener(this);
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::AddSignalFired(const string& signalName, const string& senderName, const string& contextVariables)
{
	//hard coded limit
	if (m_ExecutedResponses.size() > MAX_NUMBER_OF_TRACKED_SIGNALS)
	{
		m_ExecutedResponses.clear();
		m_CurrentResponse = MAX_NUMBER_OF_TRACKED_SIGNALS + 1;
		m_InstanceToID.clear();
	}

	SStartedResponses newResponse;
	newResponse.signalName = signalName;
	newResponse.senderName = senderName;
	newResponse.contextVariables = contextVariables;
	newResponse.drsUserName = CResponseSystem::GetInstance()->GetCurrentDrsUserName();
	newResponse.currentState = CResponseSystemDebugDataProvider::eER_Queued;
	newResponse.id = static_cast<ResponseInstanceID>(m_ExecutedResponses.size());
	newResponse.currentlevelInHierarchy = 0;
	newResponse.timeOfEvent = CResponseSystem::GetInstance()->GetCurrentDrsTime();

	m_ExecutedResponses.push_back(newResponse);
}

//--------------------------------------------------------------------------------------------------
bool CResponseSystemDebugDataProvider::IncrementSegmentHierarchyLevel()
{
	if (m_CurrentResponse >= m_ExecutedResponses.size())
	{
		return false;
	}

	SStartedResponses& response = m_ExecutedResponses[m_CurrentResponse];
	response.currentlevelInHierarchy++;
	return true;
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::AddDialogLineFinished(const CHashedString& lineID, const string& speakerName, EStatus status, const string& reason)
{
	for (LineHistoryList::iterator it = m_LineHistory.begin(); it != m_LineHistory.end(); ++it)
	{
		if ((it->status == eER_Running || it->status == eER_Queued || it->status == eER_Canceling) &&
		    it->lineID == lineID &&
		    it->speakerName == speakerName)
		{
			it->status = status;
			it->description = reason;
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::AddDialogLineStarted(const CHashedString& lineID, const string& lineText, const string& speakerName, EStatus status, const string& reason)
{
	SLineHistory temp;
	temp.lineID = lineID;
	temp.lineText = lineText;
	temp.speakerName = speakerName;
	temp.status = status;
	temp.description = reason;
	temp.source = CResponseSystem::GetInstance()->GetCurrentDrsUserName();
	temp.timeOfEvent = CResponseSystem::GetInstance()->GetCurrentDrsTime();

	m_LineHistory.push_back(temp);
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::GetRecentSignals(DynArray<const char*>& outResults, DRS::IResponseManager::eSignalFilter filter /* = eSF_All */)
{
	for (StartedResponsesList::const_iterator it = m_ExecutedResponses.begin(); it != m_ExecutedResponses.end(); ++it)
	{
		if (filter == DRS::IResponseManager::eSF_All ||
		    (filter == DRS::IResponseManager::eSF_OnlyWithoutResponses && it->currentState == eER_NoResponse) ||
		    (filter == DRS::IResponseManager::eSF_OnlyWithResponses && it->currentState != eER_NoResponse))
		{
			outResults.push_back(it->signalName.c_str());
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SerializeRecentResponse(Serialization::IArchive& ar, const DynArray<stack_string>& signalNames, int maxElemets)
{
	if (ar.getFilter() != 0)
	{
		if (signalNames.empty())  // ALL
		{
			m_ExecutedResponses.clear();
		}
		else
		{
			for (DynArray<stack_string>::const_iterator itSignalNames = signalNames.begin(); itSignalNames != signalNames.end(); ++itSignalNames)
			{
				string signalName = itSignalNames->c_str();

				for (StartedResponsesList::iterator it = m_ExecutedResponses.begin(); it != m_ExecutedResponses.end(); )
				{
					if (it->signalName.compareNoCase(signalName) == 0)
					{
						it = m_ExecutedResponses.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
		}
	}
	else
	{
		static string signalslist;
		static StartedResponsesList tempcopy;

		if (signalNames.empty()) // ALL
		{
			if (maxElemets > m_ExecutedResponses.size())
			{
				maxElemets = m_ExecutedResponses.size();
			}
			tempcopy.assign(m_ExecutedResponses.rbegin(), m_ExecutedResponses.rbegin() + maxElemets);
			signalslist = "ALL";
		}
		else
		{
			signalslist.clear();
			for (DynArray<stack_string>::const_iterator itSignalNames = signalNames.begin(); itSignalNames != signalNames.end(); ++itSignalNames)
			{
				string signalName = itSignalNames->c_str();
				if (!signalslist.empty())
					signalslist += ", ";
				signalslist += signalName;
			}
			tempcopy.clear();

			for (StartedResponsesList::const_reverse_iterator it = m_ExecutedResponses.rbegin(); it != m_ExecutedResponses.rend(); ++it)
			{
				for (DynArray<stack_string>::const_iterator itSignalNames = signalNames.begin(); itSignalNames != signalNames.end(); ++itSignalNames)
				{
					if (it->signalName.compareNoCase(itSignalNames->c_str()) == 0 && (maxElemets <= 0 || tempcopy.size() < maxElemets))
					{
						tempcopy.push_back(*it);
					}
				}
			}
		}

		static string label;
		if (tempcopy.empty())
		{
			label = "No history for signal '" + signalslist + "'";
			ar(tempcopy, "ExecutedResponses", label.c_str());
		}
		else
		{
			label = "History for signal '" + signalslist + "'";
			ar(tempcopy, "ExecutedResponses", label.c_str());
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SerializeVariableChanges(Serialization::IArchive& ar, const string& variableName, int maxElemets)
{
	bool bSerializeAll = variableName == "ALL";

	if (ar.getFilter() != 0)
	{
		if (bSerializeAll)
		{
			m_VariableChangeHistory.clear();
		}
		else
		{
			for (VariableChangeInfoList::iterator it = m_VariableChangeHistory.begin(); it != m_VariableChangeHistory.end(); )
			{
				if (it->variableName.compareNoCase(variableName) == 0)
					it = m_VariableChangeHistory.erase(it);
				else
					++it;
			}
		}
	}
	else
	{
		static VariableChangeInfoList tempcopy;
		tempcopy.clear();

		for (VariableChangeInfoList::const_iterator it = m_VariableChangeHistory.begin(); it != m_VariableChangeHistory.end(); ++it)
		{
			if ((bSerializeAll ||
			     it->variableName.find(variableName.c_str()) != string::npos) &&
			    (maxElemets <= 0 ||
			     tempcopy.size() < maxElemets))
			{
				tempcopy.push_back(*it);
			}
		}

		static string label;
		label = "History of changes for '" + variableName + "'";
		ar(tempcopy, "VariableChanges", label.c_str());
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SerializeDialogLinesHistory(Serialization::IArchive& ar)
{
	if (ar.getFilter() != 0)
	{
		m_LineHistory.clear();
	}
	else
	{
		ar(m_LineHistory, "linesHistory", "+!Spoken Lines");
	}
}

//--------------------------------------------------------------------------------------------------
void CResponseSystemDebugDataProvider::SLineHistory::Serialize(Serialization::IArchive& ar)
{
	ar(timeOfEvent, "time", "!^Time:");
	ar(lineID, "lineID", "!^LineID:");
	ar(speakerName, "speaker", "!^Speaker:");
	ar(status, "status", "!^Status:");
	ar(description, "desc", "!Desc:");
	ar(lineText, "lineText", "!LineText:");
	ar(source, "source", "!Source:");
}
