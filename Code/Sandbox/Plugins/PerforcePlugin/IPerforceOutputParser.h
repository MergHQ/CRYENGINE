// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "VersionControl/VersionControlError.h"
#include <vector>

class CVersionControlFileStatusUpdate;

struct IPerforceOutputParser
{
	struct Info
	{
		string workspace;
		string user;
		string root;
		string currentDir;
	};

	enum class SyncMode
	{
		Default,
		Preview,
		Remove
	};

	virtual ~IPerforceOutputParser() {}

	virtual void ParsePendingChangelists(const string& perforceOutput, const string& workspace, std::vector<string>& result) const = 0;

	virtual void ParseInfo(const string& perforceOutput, Info& result) const = 0;

	virtual void ParseStatus(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result, std::vector<string>& reconcileResult) const = 0;

	virtual void ParseDepotFileNames(const string& perforceOutput, std::vector<string>& result) const = 0;

	virtual void ParseDescribe(const string& perforceOutput, string& result) const = 0;

	virtual void ParseSubmit(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const = 0;

	virtual void ParseEdit(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const = 0;

	virtual void ParseAdd(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const = 0;

	virtual void ParseDelete(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const = 0;

	virtual void ParseSync(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result, SyncMode mode = SyncMode::Default) const = 0;

	virtual void ParseSync(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result, std::vector<string>& resultConflicting, SyncMode mode = SyncMode::Default) const = 0;

	virtual void ParseFiles(const string& perforceOutput, std::vector<int>& result) const = 0;

	virtual void ParseRevert(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const = 0;

	virtual void ParseResolve(const string& perforceOutput, std::vector<bool>& results) const = 0;

	virtual void ParseShelve(const string& perforceOutput) const = 0;

	virtual void ParseUnshelve(const string& perforceOutput) const = 0;

	virtual void ParseDeleteShelve(const string& perforceOutput) const = 0;

	virtual void ParseDeleteChangelist(const string& perforceOutput) const = 0;

	virtual void ParseCreateChangelist(const string& perforceOutput, string& result) const = 0;

	virtual void ParseFileStat(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const = 0;

	virtual void ParseFileStat(const string& perforceOutput, std::vector<string>& result) const = 0;

	virtual void ParseDiff(const string& perforceOutput, std::vector<string>& result) const = 0;

	virtual void ParseReconcile(const string& perforceOutput, std::vector<string>& result, bool isPreview = true) const = 0;

	virtual void ParsePrint(const string& perforceOutput, string& result) const = 0;

	virtual SVersionControlError GetError() const = 0;

	bool ParseStatus(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const
	{
		std::vector<string> reconcileList;
		ParseStatus(perforceOutput, result, reconcileList);
		return !reconcileList.empty();
	}
};
