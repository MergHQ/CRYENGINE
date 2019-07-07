// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "PerforceFileStatFilter.h"
#include <vector>

struct IPerforceExecutor
{
	enum Reconcile
	{
		eReconcile_Add =    0x00000001,
		eReconcile_Edit =   0x00000002,
		eReconcile_Delete = 0x00000004,
		eReconcile_All =    0xFFFFFFFF,
	};

	virtual ~IPerforceExecutor() {}

	virtual string Info() = 0;

	virtual string Status(const string& changelist = "") = 0;

	virtual string Status(const std::vector<string>& filePaths) = 0;

	virtual string Opened(const string& changelist = "") = 0;

	virtual string Reopen(const string& changelist, const std::vector<string>& filePaths) = 0;

	virtual string Add(const std::vector<string>& filePaths, bool force = false) = 0;

	virtual string Edit(const std::vector<string>& filePaths) = 0;

	virtual string Delete(const std::vector<string>& filePaths) = 0;

	virtual string ReconcilePreview(int filter = eReconcile_All, const std::vector<string>& filePaths = {}) = 0;

	virtual string Reconcile(const std::vector<string>& filePaths = {}) = 0;

	virtual string DescribeChangelist(const string& changelist) = 0;

	virtual string SetChangelistDescription(const string& changelist, const string& description) = 0;

	virtual string CreateChangelist(const string& description, const std::vector<string>& filePaths = {}) = 0;

	virtual string Sync(const std::vector<string>& filePaths = {}, bool force = false, int rev = -1) = 0;
	
	virtual string SyncPreview() = 0;

	virtual string Files(const std::vector<string>& filePaths) = 0;

	virtual string PendingChangelists(const string& user) = 0;

	virtual string SubmitChangelist(const string& changelist) = 0;

	virtual string ResolveOurs(const std::vector<string>& filePaths) = 0;

	virtual string ResolveTheir(const std::vector<string>& filePaths) = 0;

	virtual string Have(const std::vector<string>& filePaths) = 0;

	virtual string Revert(const std::vector<string>& filePaths, bool onlyUnchanged = false, bool clearOnlyState = false) = 0;

	virtual string Shelve(const std::vector<string>& filePaths, const string& changelist) = 0;

	virtual string Unshelve(const string& shelve, const string& targetChangelist) = 0;

	virtual string DeleteShelve(const string& shelve) = 0;

	virtual string DeleteChangelist(const string& changelist) = 0;

	virtual string FileStat(EFileStatFilter f, const std::vector<string>& filePaths = { "..." }) = 0;

	virtual string FileStat(const std::vector<string>& filePaths) = 0;

	virtual string Diff(const std::vector<string>& filePaths = { "..." }) = 0;

	virtual string CheckLogin() = 0;

	virtual string Logout() = 0;

	virtual string Print(const string& file) = 0;

	virtual void   UpdateSettings(const string& port, const string& workspace, const string& user = "", const string& password = "") = 0;
};
