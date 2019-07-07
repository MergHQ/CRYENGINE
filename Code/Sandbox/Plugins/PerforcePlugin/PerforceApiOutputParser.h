// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IPerforceOutputParser.h"
#include "PerforceFileStatFilter.h"

class ClientUser;

class CPerforceApiOutputParser : public IPerforceOutputParser
{
public:
	using IPerforceOutputParser::ParseStatus;

	CPerforceApiOutputParser(const string& trackedFolderPath);

	~CPerforceApiOutputParser();

	virtual void ParsePendingChangelists(const string& perforceOutput, const string& workspace, std::vector<string>& result) const override;

	virtual void ParseInfo(const string& perforceOutput, Info& result) const override;

	virtual void ParseStatus(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result, std::vector<string>& reconcileResult) const override;

	virtual void ParseDepotFileNames(const string& perforceOutput, std::vector<string>& result) const override;

	virtual void ParseDescribe(const string& perforceOutput, string& result) const override;

	virtual void ParseSubmit(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const override;

	virtual void ParseEdit(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const override;

	virtual void ParseAdd(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const override;

	virtual void ParseDelete(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const override;

	virtual void ParseSync(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result, SyncMode mode) const override;

	virtual void ParseSync(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result, std::vector<string>& resultConflicting, SyncMode mode) const override;

	virtual void ParseFiles(const string& perforceOutput, std::vector<int>& result) const override;

	virtual void ParseRevert(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const override;

	virtual void ParseResolve(const string& perforceOutput, std::vector<bool>& results) const override;

	virtual void ParseShelve(const string& perforceOutput) const override;

	virtual void ParseUnshelve(const string& perforceOutput) const override;

	virtual void ParseDeleteShelve(const string& perforceOutput) const override;

	virtual void ParseDeleteChangelist(const string& perforceOutput) const override;

	virtual void ParseCreateChangelist(const string& perforceOutput, string& result) const override;

	virtual void ParseFileStat(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const override;

	virtual void ParseFileStat(const string& perforceOutput, std::vector<string>& result) const override;

	virtual void ParseDiff(const string& perforceOutput, std::vector<string>& result) const override;

	virtual void ParseReconcile(const string& perforceOutput, std::vector<string>& result, bool isPreview) const override;

	virtual void ParsePrint(const string& perforceOutput, string& result) const override;

	void         SetFileStatFilter(EFileStatFilter f);

	ClientUser*  SetP4ClientUser(const string& command);

	void         SetCriticalError(SVersionControlError error);

	virtual SVersionControlError GetError() const override;
};
