// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PerforceConflictsResolver.h"
#include "PerforceVCSAdapter.h"
#include "IPerforceExecutor.h"
#include "IPerforceOutputParser.h"
#include "VersionControl/VersionControlFileStatusUpdate.h"
#include <unordered_map>

void CPerforceConflictsResolver::ResolveMMOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	m_executor->Reconcile(filePaths);
	m_executor->Sync(filePaths);
	std::vector<bool> perFileResults;
	m_parser->ParseResolve(m_executor->ResolveOurs(filePaths), perFileResults);
	for (const auto& filePath : filePaths)
	{
		statusesMap[filePath]->ClearRemoteState();
	}
}

void CPerforceConflictsResolver::ResolveMMTheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	m_executor->Reconcile(filePaths);
	m_executor->Sync(filePaths);
	std::vector<bool> perFileResults;
	m_parser->ParseResolve(m_executor->ResolveTheir(filePaths), perFileResults);
	// TODO: revert so that it's not checked out anymore
	for (const auto& filePath : filePaths)
	{
		statusesMap[filePath]->ClearLocalState();
	}
}

void CPerforceConflictsResolver::ResolveMDOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	m_executor->Reconcile(filePaths);
	m_executor->Sync(filePaths);
	std::vector<CVersionControlFileStatusUpdate> addResults;
	m_parser->ParseAdd(m_executor->Add(filePaths, true), addResults);
	for (const auto& filePath : filePaths)
	{
		statusesMap[filePath]->SetState(CVersionControlFileStatus::eState_AddedLocally);
	}
}

void CPerforceConflictsResolver::ResolveMDTheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	m_executor->Reconcile(filePaths);
	m_executor->Sync(filePaths);
	m_executor->Revert(filePaths);
	for (const auto& filePath : filePaths)
	{
		statusesMap.erase(filePath);
	}
}

void CPerforceConflictsResolver::ResolveDMOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	m_executor->Sync(filePaths);
	for (const auto& filePath : filePaths)
	{
		statusesMap[filePath]->ClearRemoteState();
	}
}

void CPerforceConflictsResolver::ResolveDMTheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	m_executor->Reconcile(filePaths);
	m_executor->Sync(filePaths);
	m_executor->Revert(filePaths);
	for (const auto& filePath : filePaths)
	{
		statusesMap.erase(filePath);
	}
}

void CPerforceConflictsResolver::ResolveAAOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	/*using namespace Private_PerforceConflictsResolver;
	string changelist;
	std::vector<string> changelists;
	m_parser->ParseStatus(m_executor->Status(filePaths), changelists);
	m_executor->Reconcile(filePaths);
	m_parser->ParseCreateChangelist(m_executor->CreateChangelist("tmp"), changelist);
	// TODO: check if it would be possible to use the second param of CreateChangelist with prefix //depot/stream/
	m_executor->Reopen(changelist, filePaths);
	if (!m_parser->ParseShelve(m_executor->Shelve(filePaths, changelist))) CRY_ASSERT(false);
	m_executor->Revert(filePaths);
	m_executor->Sync(filePaths);
	if (!m_parser->ParseUnshelve(m_executor->Unshelve(changelist, changelist))) CRY_ASSERT(false);
	m_executor->Sync(filePaths);
	std::vector<bool> addResults;
	if (!m_parser->ParseAdd(m_executor->Add(filePaths, true), addResults)) CRY_ASSERT(false);
	if (!m_parser->ParseDeleteShelve(m_executor->DeleteShelve(changelist))) CRY_ASSERT(false);
	m_executor->Reopen(changelists[0], filePaths);
	//m_executor->Reopen("default", filePaths);
	if (!m_parser->ParseDeleteChangelist(m_executor->DeleteChangelist(changelist))) CRY_ASSERT(false);
	for (const auto& filePath : filePaths)
	{
		statusesMap[filePath].SetLocalState(EState::Added);
		statusesMap[filePath]-->SetRemoteState(EState::Unmodified);
		statusesMap[filePath]->SetConflicting(false);
	}
	*/
}

void CPerforceConflictsResolver::ResolveAATheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	m_executor->Reconcile(filePaths);
	m_executor->Revert(filePaths);
	m_executor->Sync(filePaths, true);
	for (const auto& filePath : filePaths)
	{
		statusesMap.erase(filePath);
	}
}

void CPerforceConflictsResolver::ResolveDD(const std::vector<string>& filePaths, FileStatusesMap& statusesMap)
{
	ResolveAATheir(filePaths, statusesMap);
}
