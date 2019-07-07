// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PerforceVCSAdapter.h"
#include "PerforceApiOutputParser.h"
#include "PerforceApiExecutor.h"
#include "PerforceConflictsResolver.h"
#include "PerforceFilePathUtil.h"
#include "Md5Generator.h"
#include "EditorFramework/Preferences.h"
#include "AssetSystem/FileOperationsExecutor.h"
#include "VersionControl/VersionControl.h"
#include "VersionControl/VersionControlFileStatusUpdate.h"
#include "VersionControl/VersionControlFileOperationsExecutor.h"

namespace Private_PerforceVCSAdapter
{

const string DEFAULT_CHANGELIST = "default";

bool DoesContainOnlyStars(const string& str)
{
	for (char c : str)
	{
		if (c != '*')
		{
			return false;
		}
	}
	return !str.empty();
}

struct SPerforceGeneralPreferences : public SPreferencePage
{
	SPerforceGeneralPreferences()
		: SPreferencePage("perforce", "Version Control/Perforce")
	{}

	bool Serialize(yasli::Archive& ar) override
	{
		ar.openBlock("perforce", "Perforce");

		string oldServer = server;
		string oldWorkspace = workspace;
		string oldUser = user;

		ar(server, "server", "Server");
		ar(workspace, "workspace", "Workspace");
		ar(user, "user", "User");
		bool hasChange = SerializePassword(ar) || oldServer != server || oldWorkspace != workspace || oldUser != user;

		ar.closeBlock();

		if (hasChange)
		{
			signalSettingsChanged();
		}

		return hasChange;
	}

	bool SerializePassword(yasli::Archive &ar)
	{
		bool didPasswordChanged = false;
		if (ar.isInput())
		{
			string oldPassword = password;
			ar(password, "password");
			didPasswordChanged = oldPassword != password;
			if (password.empty())
			{
				hashedPassword = "";
			}
			else if (!DoesContainOnlyStars(password))
			{
				hashedPassword = Md5Generator::Generate(password);
				password = string(password.size(), '*');
			}
		}
		else if (ar.isOutput() && ar.isEdit())
		{
			string tmp = hashedPassword.empty() ? "" : password.empty() ? string(32, '*') : password;
			ar(tmp, "password", "Password");
		}
		if (!ar.isEdit() && !hashedPassword.empty())
		{
			ar(hashedPassword, "password");
		}
		return didPasswordChanged;
	}

	string server;
	string workspace;
	string user;
	string hashedPassword;
	string password;
};

SPerforceGeneralPreferences g_preferences;

bool HasAllRequiredData()
{
	return !(g_preferences.server.empty() || g_preferences.workspace.empty() || g_preferences.user.empty());
}

// TODO: extract
string toString(const std::vector<string>& strings, const string& separator = " ")
{
	if (strings.empty())
		return "";

	string ret;
	for (const auto& str : strings)
	{
		ret += str + separator;
	}
	return ret.substr(0, ret.size() - separator.size());
}

std::vector<string> ToNamesList(std::vector<CVersionControlFileStatus*> remoteNonDeleteChanges)
{
	std::vector<string> filePaths;
	filePaths.reserve(remoteNonDeleteChanges.size());
	for (auto fileStatus : remoteNonDeleteChanges)
	{
		filePaths.push_back(fileStatus->GetFileName());
	}
	return filePaths;
}

std::vector<string> MergeFilesWithFolders(const std::vector<string>& files, const std::vector<string>& folders)
{
	std::vector<string> result;
	result.reserve(files.size() + folders.size());

	std::copy(files.cbegin(), files.cend(), std::back_inserter(result));
	std::vector<string> adjustedFolders = PerforceFilePathUtil::AdjustFolders(folders);
	std::move(adjustedFolders.begin(), adjustedFolders.end(), std::back_inserter(result));

	return result;
}

std::vector<string> ApplyFileExtensionToFolders(const std::vector<string>& folders, const std::vector<string>& fileExtensions)
{
	std::vector<string> result;
	result.reserve(folders.size() * fileExtensions.size());
	for (const string& folder : folders)
	{
		for (const string& fileExtension : fileExtensions)
		{
			result.push_back(folder + "." + fileExtension);
		}
	}
	return result;
}

std::vector<string> AdjustAndMergePaths(const std::vector<string>& paths, const std::vector<string>& folders, const std::vector<string>& fileExtensions = {})
{
	if (folders.empty())
	{
		return PerforceFilePathUtil::AdjustPaths(paths);
	}
	
	std::vector<string> result = PerforceFilePathUtil::AdjustPaths(paths);
	std::vector<string> adjustedFolders = PerforceFilePathUtil::AdjustFolders(folders);
	if (!fileExtensions.empty())
	{
		adjustedFolders = ApplyFileExtensionToFolders(adjustedFolders, fileExtensions);
	}
	std::move(adjustedFolders.begin(), adjustedFolders.end(), std::back_inserter(result));
	return result;
}

}

CPerforceVCSAdapter::CPerforceVCSAdapter(const string& rootPath)
	: m_pParser(std::make_unique<CPerforceApiOutputParser>(rootPath))
	, m_pExecutor(std::make_unique<CPerforceApiExecutor>(rootPath, static_cast<CPerforceApiOutputParser*>(m_pParser.get())))
	, m_pConflictsResolver(std::make_unique<CPerforceConflictsResolver>(m_pExecutor.get(), m_pParser.get()))
	, m_rootPath(rootPath)
{
	using namespace Private_PerforceVCSAdapter;

	CFileOperationExecutor::SetExecutor(std::make_unique<CVersionControlFileOperationsExecutor>());
}

CPerforceVCSAdapter::CPerforceVCSAdapter(std::unique_ptr<IPerforceExecutor> executor, std::unique_ptr<IPerforceOutputParser> parser,
	std::unique_ptr<IPerforceConflictsResolver> conflictsResolver)
	: m_pExecutor(std::move(executor))
	, m_pParser(std::move(parser))
	, m_pConflictsResolver(std::move(conflictsResolver))
{}

CPerforceVCSAdapter::~CPerforceVCSAdapter()
{
	using namespace Private_PerforceVCSAdapter;
	CFileOperationExecutor::ResetToDefault();
	if (m_isOnline)
	{
		m_isOnline = false;
		signalOnlineChanged();
	}
}

SVersionControlError CPerforceVCSAdapter::UpdateStatus()
{
	using namespace Private_PerforceVCSAdapter;

	return UpdateStatus({}, { m_rootPath });
}

SVersionControlError CPerforceVCSAdapter::UpdateStatus(const std::vector<string>& filePaths, const std::vector<string>& folders)
{
	using namespace Private_PerforceVCSAdapter;
	std::vector<string> allFiles;
	std::vector<string> allFolders;
	PerforceFilePathUtil::SeparateFolders(filePaths, allFiles, allFolders);
	allFolders.insert(allFolders.end(), folders.cbegin(), folders.cend());
	allFolders = PerforceFilePathUtil::AdjustFolders(allFolders);

	std::vector<CVersionControlFileStatusUpdate> filesStatuses;
	if (!UpdateStatusesForFiles(allFiles, filesStatuses))
	{
		return m_pParser->GetError();
	}

	std::vector<CVersionControlFileStatusUpdate> foldersStatuses;
	if (!UpdateStatusesForFolders(allFolders, foldersStatuses))
	{
		return m_pParser->GetError();
	}

	if (foldersStatuses.empty())
	{
		GetCache()->UpdateFiles(filesStatuses);
	}
	else if (filesStatuses.empty())
	{
		GetCache()->UpdateFiles(foldersStatuses);
	}
	else
	{
		filesStatuses.reserve(filesStatuses.size() + foldersStatuses.size());
		std::move(foldersStatuses.begin(), foldersStatuses.end(), std::back_inserter(filesStatuses));
		GetCache()->UpdateFiles(filesStatuses);
	}

	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::GetLatest(const std::vector<string>& files, const std::vector<string>& folders, const std::vector<string>& fileExtensions, bool force)
{
	using namespace Private_PerforceVCSAdapter;
	std::vector<CVersionControlFileStatusUpdate> fileStatuses;
	m_pParser->ParseSync(m_pExecutor->Sync(AdjustAndMergePaths(files, folders, fileExtensions), force), fileStatuses);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	GetCache()->UpdateFiles(fileStatuses);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::SubmitFiles(const std::vector<string>& filePaths, const string& message)
{
	string cl;
	m_pParser->ParseCreateChangelist(m_pExecutor->CreateChangelist(message), cl);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	MoveFilesToChangelist(cl, PerforceFilePathUtil::AdjustPaths(filePaths));
	SubmitChangelist(cl);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::SubmitChangelist(const string& changelist)
{
	// By default, the files in a failed submit operation are left locked unless the submit.unlocklocked configurable is set
	std::vector<CVersionControlFileStatusUpdate> fileStatuses;
	m_pParser->ParseSubmit(m_pExecutor->SubmitChangelist(changelist), fileStatuses);
	GetCache()->UpdateFiles(fileStatuses);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::ResolveConflicts(const std::vector<std::pair<string, EConflictResolution>>& conflictStatuses)
{
	using namespace Private_PerforceVCSAdapter;
	using FS = CVersionControlFileStatus;

	std::vector<string> mmOurs;
	std::vector<string> mmTheir;
	std::vector<string> mdOurs;
	std::vector<string> mdTheir;
	std::vector<string> dmOurs;
	std::vector<string> dmTheir;
	std::vector<string> aaOurs;
	std::vector<string> aaTheir;
	std::vector<string> dd;

	/*for (const auto& resolutionPair : conflictStatuses)
	{
		const auto& fs = GetCache()->GetData()[resolutionPair.first];
		if (fs->HasState(FS::eState_DeletedLocally) && fs->HasState(FS::eState_DeletedRemotely))
		{
			dd.push_back(resolutionPair.first);
		}
		else if (resolutionPair.second == EConflictResolution::Ours)
		{
			if (fs->HasState(FS::eState_CheckedOutLocally) && fs->HasState(FS::eState_DeletedRemotely))
			{
				mdOurs.push_back(resolutionPair.first);
			}
			else if (fs->HasState(FS::eState_CheckedOutLocally) && fs->HasState(FS::eState_UpdatedRemotely))
			{
				mmOurs.push_back(resolutionPair.first);
			}
			else if (fs->HasState(FS::eState_DeletedLocally) && fs->HasState(FS::eState_UpdatedRemotely))
			{
				dmOurs.push_back(resolutionPair.first);
			}
			else if (fs->HasState(FS::eState_AddedLocally) && fs->HasState(FS::eState_AddedRemotely))
			{
				aaOurs.push_back(resolutionPair.first);
			}
		}
		else if (resolutionPair.second == EConflictResolution::Their)
		{
			if (fs->HasState(FS::eState_CheckedOutLocally) && fs->HasState(FS::eState_DeletedRemotely))
			{
				mdTheir.push_back(resolutionPair.first);
			}
			else if (fs->HasState(FS::eState_CheckedOutLocally) && fs->HasState(FS::eState_UpdatedRemotely))
			{
				mmTheir.push_back(resolutionPair.first);
			}
			else if (fs->HasState(FS::eState_DeletedLocally) && fs->HasState(FS::eState_UpdatedRemotely))
			{
				dmTheir.push_back(resolutionPair.first);
			}
			else if (fs->HasState(FS::eState_AddedLocally) && fs->HasState(FS::eState_AddedRemotely))
			{
				aaTheir.push_back(resolutionPair.first);
			}
		}
	}

	if (!mmOurs.empty())
	{
		m_conflictsResolver->ResolveMMOurs(mmOurs, GetCache()->GetData());
	}

	if (!mmTheir.empty())
	{
		m_conflictsResolver->ResolveMMTheir(mmTheir, GetCache()->GetData());
	}

	if (!mdOurs.empty())
	{
		m_conflictsResolver->ResolveMDOurs(mdOurs, GetCache()->GetData());
	}
	
	if (!mdTheir.empty())
	{
		m_conflictsResolver->ResolveMDTheir(mdTheir, GetCache()->GetData());
	}

	if (!dmOurs.empty())
	{
		m_conflictsResolver->ResolveDMOurs(dmOurs, GetCache()->GetData());
	}

	if (!dmTheir.empty())
	{
		m_conflictsResolver->ResolveDMTheir(dmTheir, GetCache()->GetData());
	}

	if (!aaOurs.empty())
	{
		m_conflictsResolver->ResolveAAOurs(aaOurs, GetCache()->GetData());
	}

	if (!aaTheir.empty())
	{
		m_conflictsResolver->ResolveAATheir(aaTheir, GetCache()->GetData());
	}

	if (!dd.empty())
	{
		m_conflictsResolver->ResolveDD(dd, GetCache()->GetData());
	}*/
	return EVersionControlError::None;
}

SVersionControlError CPerforceVCSAdapter::AddFiles(const std::vector<string>& filePaths)
{
	std::vector<CVersionControlFileStatusUpdate> addedFiles;
	m_pParser->ParseAdd(m_pExecutor->Add(PerforceFilePathUtil::AdjustPaths(filePaths)), addedFiles);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	GetCache()->UpdateFiles(addedFiles);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::EditFiles(const std::vector<string>& filePaths)
{
	std::vector<CVersionControlFileStatusUpdate> fileStatuses;
	m_pParser->ParseEdit(m_pExecutor->Edit(PerforceFilePathUtil::AdjustPaths(filePaths)), fileStatuses);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	GetCache()->UpdateFiles(fileStatuses);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::DeleteFiles(const std::vector<string>& filePaths)
{
	std::vector<CVersionControlFileStatusUpdate> fileStatuses;
	m_pParser->ParseDelete(m_pExecutor->Delete(PerforceFilePathUtil::AdjustPaths(filePaths)), fileStatuses);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	GetCache()->UpdateFiles(fileStatuses);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::Revert(const std::vector<string>& files, const std::vector<string>& folders)
{
	using namespace Private_PerforceVCSAdapter;
	std::vector<CVersionControlFileStatusUpdate> fileStatuses;
	m_pParser->ParseRevert(m_pExecutor->Revert(AdjustAndMergePaths(files, folders)), fileStatuses);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	GetCache()->UpdateFiles(fileStatuses);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::ClearLocalState(const std::vector<string>& files, const std::vector<string>& folders, bool clearIfUnchanged)
{
	using namespace Private_PerforceVCSAdapter;
	std::vector<CVersionControlFileStatusUpdate> fileStatuses;
	m_pParser->ParseRevert(m_pExecutor->Revert(AdjustAndMergePaths(files, folders), clearIfUnchanged, true), fileStatuses);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	GetCache()->UpdateFiles(fileStatuses);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::RetrieveFilesContent(const string& file)
{
	string filesContent;
	m_pParser->ParsePrint(m_pExecutor->Print(file), filesContent);

	GetCache()->SaveFilesContent(file, filesContent);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::RemoveFilesLocally(const std::vector<string>& filePaths)
{
	using namespace Private_PerforceVCSAdapter;
	std::vector<CVersionControlFileStatusUpdate> fileStatuses;
	m_pParser->ParseSync(m_pExecutor->Sync(PerforceFilePathUtil::AdjustPaths(filePaths), false, 0), fileStatuses
		, IPerforceOutputParser::SyncMode::Remove);
	if (!UpdateOnlineState())
	{
		return m_pParser->GetError();
	}
	GetCache()->UpdateFiles(fileStatuses);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::GetChangelists(std::vector<string>& result)
{
	using namespace Private_PerforceVCSAdapter;
	m_pParser->ParsePendingChangelists(m_pExecutor->PendingChangelists(g_preferences.user), g_preferences.workspace, result);
	result.push_back(DEFAULT_CHANGELIST);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::GetFilesInChangelist(const string& changelist, std::vector<string>& result)
{
	m_pParser->ParseDepotFileNames(m_pExecutor->Opened(changelist), result);
	if (changelist == "default")
	{
		std::vector<string> notOpened;
		m_pParser->ParseDepotFileNames(m_pExecutor->ReconcilePreview(), notOpened);
		for (const auto& fileName : notOpened)
		{
			if (std::find(result.begin(), result.end(), fileName) == result.end())
				result.emplace_back(fileName);
		}
	}
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::CreateChangelist(const string& description)
{
	m_pExecutor->CreateChangelist(description);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::DeleteChangelist(const string& changelist)
{
	m_pParser->ParseDeleteChangelist(m_pExecutor->DeleteChangelist(changelist));
	if (m_pParser->GetError().type == EVersionControlError::None)
	{
		m_pExecutor->DeleteShelve(changelist);
		m_pExecutor->DeleteChangelist(changelist);
	}
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::MoveFilesToChangelist(const string& changelist, const std::vector<string>& filePaths)
{
	m_pExecutor->Reopen(changelist, filePaths);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::SetChangelistDescription(const string& changelist, const string& description)
{
	m_pExecutor->SetChangelistDescription(changelist, description);
	return m_pParser->GetError();
}

SVersionControlError CPerforceVCSAdapter::GetChangelistDescription(const string& changelist, string& result)
{
	if (changelist == "default")
	{
		result = "";
		return EVersionControlError::None;
	}
	m_pParser->ParseDescribe(m_pExecutor->DescribeChangelist(changelist), result);
	return m_pParser->GetError();
}

bool CPerforceVCSAdapter::UpdateStatusesForFiles(const std::vector<string>& files, std::vector<CVersionControlFileStatusUpdate>& result)
{
	if (files.empty())
	{
		return true;
	}

	m_pParser->ParseFileStat(m_pExecutor->FileStat(files), result);

	if (!UpdateOnlineState())
	{
		return false;
	}

	std::vector<string> modifiedFiles;
	m_pParser->ParseDiff(m_pExecutor->Diff(files), modifiedFiles);

	for (const string& file : modifiedFiles)
	{
		result.emplace_back(file);
		result.back().AddState(CVersionControlFileStatus::eState_ModifiedLocally);
	}

	return true;
}

bool CPerforceVCSAdapter::UpdateStatusesForFolders(const std::vector<string>& folders, std::vector<CVersionControlFileStatusUpdate>& result)
{
	if (folders.empty())
	{
		return true;
	}

	std::vector<string> tmpFiles;
	m_pParser->ParseFileStat(m_pExecutor->FileStat(EFileStatFilter::Tracked, folders), tmpFiles);

	if (!UpdateOnlineState())
	{
		return false;
	}

	m_pParser->ParseFileStat(m_pExecutor->FileStat(tmpFiles), result);

	m_pParser->ParseFileStat(m_pExecutor->FileStat(EFileStatFilter::AddedLocally, folders), tmpFiles);

	for (string& addedFile : tmpFiles)
	{
		result.emplace_back(std::move(addedFile), CVersionControlFileStatus::eState_AddedLocally);
	}

	m_pParser->ParseFileStat(m_pExecutor->FileStat(EFileStatFilter::DeletedRemotely, folders), tmpFiles);

	for (string& deletedFile : tmpFiles)
	{
		result.emplace_back(std::move(deletedFile), CVersionControlFileStatus::eState_DeletedRemotely);
	}

	m_pParser->ParseDiff(m_pExecutor->Diff(folders), tmpFiles);
	for (string& modifiedFile : tmpFiles)
	{
		result.emplace_back(std::move(modifiedFile));
		result.back().AddState(CVersionControlFileStatus::eState_ModifiedLocally);
	}

	return true;
}

bool CPerforceVCSAdapter::UpdateOnlineState(const SVersionControlError* pError /*= nullptr*/)
{
	const SVersionControlError parserError = m_pParser->GetError();
	if (!pError)
	{
		pError = &parserError;
	}
	const bool oldIsOnline = m_isOnline;
	m_isOnline = !pError->isCritical;
	if (!m_isOnline)
	{
		GetCache()->Clear();
	}
	if (oldIsOnline != m_isOnline)
	{
		signalOnlineChanged();
	}
	return m_isOnline;
}

SVersionControlError CPerforceVCSAdapter::CheckSettings()
{
	using namespace Private_PerforceVCSAdapter;
	if (!HasAllRequiredData())
	{
		GetCache()->Clear();
		return { EVersionControlError::InvalidSettings, "Not all required data entered." };
	}
	UpdateSettings();

	m_pExecutor->CheckLogin();
	SVersionControlError error = m_pParser->GetError();

	if (error.type == EVersionControlError::SessionExpired)
	{
		m_pExecutor->Logout();
		UpdateSettings();
		m_pExecutor->CheckLogin();
		error = m_pParser->GetError();
	}

	if (error.isCritical)
	{
		UpdateOnlineState();
		return error;
	}

	IPerforceOutputParser::Info info;
	m_pParser->ParseInfo(m_pExecutor->Info(), info);
	if (info.root.empty() || info.workspace == "*unknown*")
	{
		error = { EVersionControlError::InvalidSettings, "Unknown workspace." };
	}
	else if (info.currentDir.compareNoCase(0, info.root.size(), info.root) != 0)
	{
		error = { EVersionControlError::InvalidSettings, "Current assets' folder is not under client's root path." };
	}
	UpdateOnlineState(&error);
	return error;
}

SPreferencePage* CPerforceVCSAdapter::GetPreferences()
{
	using namespace Private_PerforceVCSAdapter;
	return &g_preferences;
}

void CPerforceVCSAdapter::UpdateSettings()
{
	using namespace Private_PerforceVCSAdapter;
	m_pExecutor->UpdateSettings(g_preferences.server, g_preferences.workspace, g_preferences.user, g_preferences.hashedPassword);
}
