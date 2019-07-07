// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "VersionControl/IVersionControlAdapter.h"
#include "IPerforceExecutor.h"
#include "IPerforceOutputParser.h"
#include "IPerforceConflictsResolver.h"
#include <memory>

class CPerforceVCSAdapter : public IVersionControlAdapter
{
public:
	CPerforceVCSAdapter(std::unique_ptr<IPerforceExecutor> executor, std::unique_ptr<IPerforceOutputParser> parser,
		std::unique_ptr<IPerforceConflictsResolver> conflictsResolver);

	explicit CPerforceVCSAdapter(const string& rootPath);

	~CPerforceVCSAdapter();

	virtual SVersionControlError UpdateStatus() override;

	virtual SVersionControlError UpdateStatus(const std::vector<string>& filePaths, const std::vector<string>& folders) override;

	virtual SVersionControlError GetLatest(const std::vector<string>& files, const std::vector<string>& folders, const std::vector<string>& fileExtensions, bool force) override;

	virtual SVersionControlError SubmitFiles(const std::vector<string>& filePaths, const string& message = "") override;

	virtual SVersionControlError ResolveConflicts(const std::vector<SVersionControlFileConflictStatus>& conflictStatuses) override;

	virtual SVersionControlError AddFiles(const std::vector<string>& filePaths) override;

	virtual SVersionControlError EditFiles(const std::vector<string>& filePaths) override;

	virtual SVersionControlError DeleteFiles(const std::vector<string>& filePaths) override;

	virtual SVersionControlError Revert(const std::vector<string>& files, const std::vector<string>& folders) override;

	virtual SVersionControlError ClearLocalState(const std::vector<string>& files, const std::vector<string>& folders, bool clearIfUnchanged) override;

	virtual SVersionControlError RetrieveFilesContent(const string& file) override;

	virtual SVersionControlError RemoveFilesLocally(const std::vector<string>& filePaths) override;

	virtual SVersionControlError CheckSettings() override;

	virtual SPreferencePage* GetPreferences() override;

	virtual bool             IsOnline() const override { return m_isOnline; }
	
private:
	void UpdateSettings();

	SVersionControlError SubmitChangelist(const string& changelist);

	SVersionControlError GetChangelists(std::vector<string>& result);

	SVersionControlError GetFilesInChangelist(const string& changelist, std::vector<string>& result);

	SVersionControlError CreateChangelist(const string& description);

	SVersionControlError DeleteChangelist(const string& changelist);

	SVersionControlError MoveFilesToChangelist(const string& changelist, const std::vector<string>& filePaths);

	SVersionControlError SetChangelistDescription(const string& changelist, const string& description);

	SVersionControlError GetChangelistDescription(const string& changelist, string& result);

	bool UpdateStatusesForFiles(const std::vector<string>& files, std::vector<CVersionControlFileStatusUpdate>& result);

	bool UpdateStatusesForFolders(const std::vector<string>& folders, std::vector<CVersionControlFileStatusUpdate>& result);

	bool UpdateOnlineState(const SVersionControlError* pError = nullptr);

	std::unique_ptr<IPerforceExecutor>          m_pExecutor;
	std::unique_ptr<IPerforceOutputParser>      m_pParser;
	std::unique_ptr<IPerforceConflictsResolver> m_pConflictsResolver;
	
	string m_rootPath;

	bool  m_isOnline{ false };
};
