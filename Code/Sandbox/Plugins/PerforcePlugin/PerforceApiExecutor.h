// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IPerforceExecutor.h"

class CPerforceApiOutputParser;
class ClientUser;

namespace Private_CPerforceApiExecutor
{
	class CClientApiWrapper;
}

class CPerforceApiExecutor : public IPerforceExecutor
{
public:
	CPerforceApiExecutor(const string& rootPath, CPerforceApiOutputParser* pParser);

	virtual string Info() override;
	
	virtual string Status(const string& changelist /* = "" */) override;

	virtual string Status(const std::vector<string>& filePaths) override;

	virtual string Opened(const string& changelist /* = "" */) override;

	virtual string Reopen(const string& changelist, const std::vector<string>& filePaths) override;

	virtual string Add(const std::vector<string>& filePaths, bool force = false) override;

	virtual string Edit(const std::vector<string>& filePaths) override;

	virtual string Delete(const std::vector<string>& filePaths) override;

	virtual string ReconcilePreview(int filter /*= eReconcile_All*/, const std::vector<string>& filePaths /*= {}*/) override;

	virtual string Reconcile(const std::vector<string>& filePaths /* = {} */) override;

	virtual string DescribeChangelist(const string& changelist) override;

	virtual string SetChangelistDescription(const string& changelist, const string& description) override;

	virtual string CreateChangelist(const string& description, const std::vector<string>& filePaths /* = {} */) override;

	virtual string Sync(const std::vector<string>& filePaths /* = {} */, bool force /* = false */, int rev /*= -1*/) override;

	virtual string SyncPreview() override;

	virtual string Files(const std::vector<string>& filePaths) override;

	virtual string PendingChangelists(const string& user) override;

	virtual string SubmitChangelist(const string& chagnelist) override;

	virtual string ResolveOurs(const std::vector<string>& filePaths) override;

	virtual string ResolveTheir(const std::vector<string>& filePaths) override;

	virtual string Have(const std::vector<string>& filePaths) override;

	virtual string Revert(const std::vector<string>& filePaths, bool onlyUnchanged /*= false*/, bool clearOnlyState /*= false*/) override;

	virtual string Shelve(const std::vector<string>& filePaths, const string& changelist) override;

	virtual string Unshelve(const string& shelve, const string& targetChangelist) override;

	virtual string DeleteShelve(const string& shelve) override;

	virtual string DeleteChangelist(const string& changelist) override;

	virtual string FileStat(EFileStatFilter f, const std::vector<string>& filePaths /*={ "..." }*/) override;

	virtual string FileStat(const std::vector<string>& filePaths) override;

	virtual string Diff(const std::vector<string>& filePaths) override;

	virtual string CheckLogin() override;

	virtual string Logout() override;

	virtual string Print(const string& file) override;

	virtual void   UpdateSettings(const string& port, const string& workspace, const string& user /* = "" */, const string& password /* = "" */) override;

private:
	bool TryRun(const char* func, const std::vector<string>& args, bool isTagged = true, ClientUser* cu = nullptr);

	void CreateNewClients();

	std::unique_ptr<Private_CPerforceApiExecutor::CClientApiWrapper> m_pP4TaggedClient;
	std::unique_ptr<Private_CPerforceApiExecutor::CClientApiWrapper> m_pP4UntaggedClient;

	const char* m_rootPath;
};
