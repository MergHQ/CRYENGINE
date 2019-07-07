// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PerforceApiOutputParser.h"
#include "VersionControl/VersionControlFileStatus.h"
#include "VersionControl/VersionControlFileStatusUpdate.h"
#include "VersionControl/VersionControlError.h"
#include <p4/clientapi.h>
#include <CrySystem/ISystem.h>
#include <iostream>
#include <unordered_map>
#include <memory>

namespace Private_CPerforceApiOutputParser
{

const bool g_forceLog = false;

void ReplaceBackslashes(string& str)
{
	auto pos = str.find('\\');
	while (pos != std::string::npos)
	{
		str.SetAt(pos, '/');
		pos = str.find('\\', pos + 1);
	}
}

int StringToLocalState(const string& status)
{
	if (status == "add")
		return CVersionControlFileStatus::eState_AddedLocally;
	else if (status == "edit")
		return CVersionControlFileStatus::eState_CheckedOutLocally;
	else if (status == "delete")
		return CVersionControlFileStatus::eState_DeletedLocally;

	CRY_ASSERT_MESSAGE(false, "Unknown status %s", status.c_str());
	return 0;
}

int ActionStringToRemoteState(const string& status)
{
	if (status == "added")
		return CVersionControlFileStatus::eState_AddedRemotely;
	else if (status == "updated")
		return CVersionControlFileStatus::eState_UpdatedRemotely;
	else if (status == "deleted")
		return CVersionControlFileStatus::eState_DeletedRemotely;

	CRY_ASSERT_MESSAGE(false, "Unknown status %s", status.c_str());
	return 0;
}

bool StartsWith(const string& startString, const string& searchString, size_t startPos = 0)
{
	if (searchString.length() - startPos < startString.length())
	{
		return false;
	}
	for (auto i = 0; i < startString.length(); ++i)
	{
		if (searchString[startPos + i] != startString[i])
		{
			return false;
		}
	}
	return true;
}

bool StartsWith(const char* startString, const char* searchString, size_t startPos = 0)
{
	for (int i = 0; i < startPos; ++i)
	{
		if (*searchString == '\0')
		{
			return false;
		}
		++searchString;
	}
	while (*startString != '\0')
	{
		if (*startString != *searchString)
		{
			return false;
		}
		++startString;
		++searchString;
	}
	return true;
}

bool StartsWith(const char* startString, const string& searchString, size_t startPos = 0)
{
	int i = 0;
	while (startString[i] != '\0' && i < searchString.length() - startPos)
	{
		if (searchString[startPos + i] != startString[i])
		{
			return false;
		}
		++i;
	}
	return startString[i] == '\0';
}

bool StartsWith(const string& startString, const char* searchString, size_t startPos = 0)
{
	int i = 0;
	while (searchString[i] != '\0' && i < startString.length() - startPos)
	{
		if (searchString[startPos + i] != startString[i])
		{
			return false;
		}
		++i;
	}
	return !startString.empty();
}

static string g_depotStreamPath;
static string g_trackedFolderPath;
static string g_rootFolderPath;
static bool   g_hasStream;

int UpdateDepotStreamPath(const char* data, int lineStart = 0)
{
	if (StartsWith(g_depotStreamPath, data))
	{
		return static_cast<int>(g_depotStreamPath.size());
	}
	string str = data;
	auto depotStreamLength = str.find('/', lineStart + 2) + 1;
	if (g_hasStream)
	{
		depotStreamLength = str.find('/', depotStreamLength) + 1;
	}
	g_depotStreamPath = str.substr(0, depotStreamLength);

	// if client's root doesn't match current working dir we add difference to the depot path.
	if (g_trackedFolderPath.size() > g_rootFolderPath.size())
	{
		g_depotStreamPath += g_trackedFolderPath.substr(g_rootFolderPath.size() + 1) + '/';
	}

	return static_cast<int>(g_depotStreamPath.size());
}

string ExtractFilePath(const string& str, int depotStreamLength, int lineStart, int lineEnd = -1)
{
	auto fileNameEndPos = str.rfind('#', lineEnd);
	auto fileNameStartPos = depotStreamLength + lineStart;
	return str.substr(fileNameStartPos, fileNameEndPos - fileNameStartPos);
}

string UpdateDepotStreamAndExtractFile(char const* str)
{
	auto length = UpdateDepotStreamPath(str);
	return str + length;
}

void AddFilePathToVectorIfValid(std::vector<string>& vec, const char* data)
{
	auto depotStreamLength = UpdateDepotStreamPath(data);
	const auto& filePath = ExtractFilePath(data, depotStreamLength, 0);
	auto dashPos = strrchr(data, '-');
	int filePathEndPos = depotStreamLength + static_cast<int>(filePath.size());
	if (dashPos && dashPos - data > filePathEndPos)
	{
		vec.push_back(filePath);
	}
}

static SVersionControlError g_error = EVersionControlError::None;

class CBaseClientUser : public ClientUser
{
public:
	static string GetName() { return "dummy"; }

	virtual void OutputInfo(char level, const char* data) override
	{
		if (ShouldLog())
		{
			CryLog("--- %s", data);
		}
	}

	virtual void OutputText(const char* data, int length) override
	{
		if (ShouldLog())
		{
			CryLog("CBaseClientUser::OutputText: %s", data);
		}
	}

	virtual void OutputBinary(const char *data, int length)
	{
		if (ShouldLog())
		{
			CryLog("CBaseClientUser::OutputBinary: %s", data);
		}
	}

	virtual void OutputStat(StrDict* dict) override
	{
		if (ShouldLog())
		{
			OutputDics(dict);
		}
	}

	virtual void OutputError(const char *errBuf) override
	{
		if (ShouldLog())
		{
			CryLog("CBaseClientUser::OutputError: %s", errBuf);
		}
	}

	virtual void Prompt(const StrPtr& str, StrBuf& strbuf, int, Error*) override
	{
		if (ShouldLog())
		{
			CryLog("CBaseClientUser::Prompt: %s", str.Text());
		}
	}

	virtual void HandleError(Error *err) override
	{
		StrBuf msg;
		err->Fmt(&msg);
		auto message = msg.Text();
		if (ShouldLog() || err->IsError())
		{
			CryLog("CBaseClientUser::HandleError: (%d) %s", err->GetSeverity(), message);
		}
		if (StartsWith("Perforce password", message) || (StartsWith("User ", message) && strstr(message, "doesn't exist")))
		{
			SetErrorIfNew(EVersionControlError::LoginFailed, message);
		}
		else if (StartsWith("Your session has expired, please login again.", message))
		{
			SetErrorIfNew(EVersionControlError::SessionExpired);
		}
		else if (StartsWith("Client '", message) && strstr(message, "' unknown"))
		{
			SetErrorIfNew(EVersionControlError::InvalidSettings, "Unknown workspace.");
		}
		else if (StartsWith("Can't clobber writable file", message))
		{
			SetErrorIfNew(EVersionControlError::CorruptedFileState, message);
		}
		else if (StartsWith("Path '", message) && strstr(message, "' is not under client's root"))
		{
			SetErrorIfNew(EVersionControlError::InvalidSettings, message);
		}
		else if (StartsWith("TCP send failed.", message))
		{
			SetErrorIfNew(EVersionControlError::NoConnection, message);
		}
		else if (StartsWith("The security level of this server requires the password to be reset", message)
			|| StartsWith("Your password has expired, please change your password", message))
		{
			SetErrorIfNew(EVersionControlError::LoginFailed, message);
		}
		if (StartsWith("The file named '", message) && strstr(message, "' contains wildcards"))
		{
			SetErrorIfNew(EVersionControlError::InvalidFileName, message);
		}
		else if (StartsWith("Client side operation(s) failed.", message))
		{
			SetErrorIfNew(EVersionControlError::OperationFailed, message);
		}
		else if (err->IsError())
		{
			SetErrorIfNew(EVersionControlError::Unknown, message);
		}
	}

	virtual void Reset() { }

	virtual void Finished() override
	{
		g_error = m_error;
	}

	void ResetData()
	{
		m_error = EVersionControlError::None;
		Reset();
	}

	bool HasError() const { return m_error.type != EVersionControlError::None; }
	SVersionControlError GetError() const { return m_error; }

	void EnableDetailedOutput() { m_hasDetailedOutput = true; }

protected:
	bool SetErrorIfNew(EVersionControlError error, const string& message = "") 
	{
		if (m_error.type == EVersionControlError::None)
		{
			m_error = { error, message };
			return true;
		}
		return false;
	}

	void SetOrAppendError(EVersionControlError error, const string& message)
	{
		if (!SetErrorIfNew(error, message) && (m_error.type == error))
		{
			m_error.message + '\n' + message;
		}
	}

private:
	bool ShouldLog() const { return m_hasDetailedOutput || g_forceLog; }

	void OutputDics(StrDict* dict)
	{
		string output = "";
		StrRef var, val;
		for (int i = 0; varList->GetVar(i, var, val); i++)
		{
			output += "--- " + string(var.Text()) + ": " + string(val.Text()) + "\n";
		}
		CryLog(output);
	}

	SVersionControlError m_error{ EVersionControlError::None };
	bool                 m_hasDetailedOutput{ false };
};

class CBaseFileStatusClientUser : public CBaseClientUser
{
public:
	virtual void Reset() override
	{
		m_fileStatuses.clear();
	}

	void CreateFileStatusUpdate(StrDict* dict, bool shouldClear = false, int state = 0)
	{
		if (auto pDepotFileVar = dict->GetVar("depotFile"))
		{
			string fileName = UpdateDepotStreamAndExtractFile(pDepotFileVar->Text());
			CreateFileStatusUpdate(std::move(fileName), shouldClear, state);
		}
	}

	void CreateFileStatusUpdate(string fileName, bool shouldClear = false, int state = 0)
	{
		if (shouldClear)
		{
			m_fileStatuses.emplace_back(std::move(fileName), state);
		}
		else
		{
			m_fileStatuses.emplace_back(std::move(fileName));
			m_fileStatuses.back().AddState(state);
		}
	}

	CVersionControlFileStatusUpdate& GetCurrentFileStatusUpdate()
	{
		return m_fileStatuses.back();
	}

	std::vector<CVersionControlFileStatusUpdate> m_fileStatuses;
};

class CChangelistsClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "changelists"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);
		if (auto pVar = dict->GetVar("change"))
		{
			m_list.push_back(pVar->Text());
		}
	}

	virtual void Reset() override
	{
		m_list.clear();
	}

	std::vector<string> m_list;
}; 

class CDescribeClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "describe"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);
		if (auto pVar = dict->GetVar("desc"))
		{
			m_output = pVar->Text();
		}
	}

	virtual void Reset() override
	{
		m_output.clear();
	}

	string m_output;
};

class CDepotFileNamesClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "depotFileNames"; }

	virtual void OutputInfo(char level, const char* data) override
	{
		CBaseClientUser::OutputInfo(level, data);
		if (!StartsWith("//", data))
			return;

		AddFilePathToVectorIfValid(m_files, data);
	}

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);
		if (auto pVar = dict->GetVar("depotFile"))
		{
			m_files.push_back(UpdateDepotStreamAndExtractFile(pVar->Text()));
		}
	}

	virtual void Reset() override
	{
		m_files.clear();
	}

	std::vector<string> m_files;
};

class CStatusClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "status"; }

	virtual void OutputInfo(char level, const char* data) override
	{
		CBaseClientUser::OutputInfo(level, data);
		string output = data;
		if (StartsWith("...", output) || StartsWith("//", output))
			return;

		auto statusEndPos = output.find("//");
		if (statusEndPos == std::string::npos)
			return;
		--statusEndPos;
		auto statusStartPos = output.rfind(' ', statusEndPos - 1) + 1;
		auto status = output.substr(statusStartPos, statusEndPos - statusStartPos);
		auto filePathEndPos = output.rfind('-', statusStartPos) - 1;
		auto filePath = output.substr(0, filePathEndPos);

		auto actionFirstChar = output[filePathEndPos + 3];
		if (actionFirstChar != 's' && actionFirstChar != 'r') // submit or reconcile
			return;

		ReplaceBackslashes(filePath);
		CreateFileStatusUpdate(filePath);

		if (actionFirstChar == 'r')
			m_reconcileList.push_back(filePath);

		GetCurrentFileStatusUpdate().AddState(StringToLocalState(status));
	}

	virtual void Reset() override
	{
		CBaseFileStatusClientUser::Reset();
		m_reconcileList.clear();
	}

	std::vector<string> m_reconcileList;
};

class CSyncClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "sync"; }

	virtual void OutputInfo(char level, const char* data) override
	{
		CBaseClientUser::OutputInfo(level, data);
		AddFilePathToVectorIfValid(m_conflictingList, data);
		SetOrAppendError(EVersionControlError::UpdateFileFailed, data);
	}

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		CreateFileStatusUpdate(dict);

		if (auto pVar = dict->GetVar("action"))
		{
			GetCurrentFileStatusUpdate().AddState(ActionStringToRemoteState(pVar->Text()));
		}
	}

	virtual void Reset() override
	{
		CBaseFileStatusClientUser::Reset();
		m_conflictingList.clear();
	}

	void AddUpdatedRemotelyState()
	{
		using FS = CVersionControlFileStatus;
		for (auto& fs : m_fileStatuses)
		{
			fs.AddState(FS::eState_UpdatedRemotely);
		}
	}

	void ClearRemoteChanges()
	{
		using FS = CVersionControlFileStatus;
		for (auto& fs : m_fileStatuses)
		{
			fs.RemoveState(FS::eState_AddedRemotely | FS::eState_DeletedRemotely | FS::eState_UpdatedRemotely);
		}
	}

	std::vector<string> m_conflictingList;
};

class CFilesClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "files"; }

	virtual void OutputInfo(char level, const char* data) override
	{
		CBaseClientUser::OutputInfo(level, data);

		string output = data;
		auto statusStart = output.rfind('-') + 2;
		if (StartsWith("move/", output, statusStart))
			statusStart += 5;

		auto statusEnd = output.find(' ', statusStart);
		auto status = output.substr(statusStart, statusEnd - statusStart);

		m_states.push_back(StringToLocalState(status));
	}

	virtual void Reset() override
	{
		m_states.clear();
	}

	std::vector<int> m_states;
};

class CChangelistClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "changelist"; }

	virtual void HandleError(Error *err) override
	{
		CBaseClientUser::HandleError(err);
		// Change 97 has shelved files associated with it and can't be deleted.
	}
};

class CSubmitClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "submit"; }

	virtual void OutputInfo(char level, const char* data) override
	{
		CBaseClientUser::OutputInfo(level, data);

		string output = data;
		if (output.find("warning: cannot submit from non-stream client") != std::string::npos)
		{
			SetErrorIfNew(EVersionControlError::SubmissionError, "Perforce cannot submit from non-stream client");
		}
	}

	virtual void HandleError(Error *err) override
	{
		CBaseFileStatusClientUser::HandleError(err);
		// Change 97 has shelved files associated with it and can't be deleted.
	}

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		CreateFileStatusUpdate(dict, true);
		auto pActionVar = dict->GetVar("action");
		if (pActionVar && strcmp(pActionVar->Text(), "delete") == 0)
		{
			GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_NotTracked);
		}
	}
};

class CInfoClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "info"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		m_info.user = dict->GetVar("userName")->Text();
		if (auto pVar = dict->GetVar("clientName"))
		{
			m_info.workspace = pVar->Text();
		}
		if (auto pVar = dict->GetVar("clientRoot"))
		{
			m_info.root = pVar->Text();
			ReplaceBackslashes(m_info.root);
			g_rootFolderPath = m_info.root;
		}
		if (auto pVar = dict->GetVar("clientCwd"))
		{
			m_info.currentDir = pVar->Text();
			CRY_ASSERT_MESSAGE(stricmp(m_info.currentDir.c_str(), g_trackedFolderPath.c_str()) == 0, 
				"Working directory provided by info (%s) doesn't match with the one set for p4 plugin (%s)", m_info.currentDir.c_str(), g_trackedFolderPath.c_str());
		}
		g_hasStream = dict->GetVar("clientStream") != nullptr;
	}

	virtual void Reset() override
	{
		m_info = IPerforceOutputParser::Info();
	}

	IPerforceOutputParser::Info m_info;
};

class CAddClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "add"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		CreateFileStatusUpdate(dict, false, CVersionControlFileStatus::eState_AddedLocally);
		GetCurrentFileStatusUpdate().RemoveState(CVersionControlFileStatus::eState_NotTracked);
	}
};

class CResolveClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "resolve"; }

	virtual void HandleError(Error *err) override
	{
		CBaseClientUser::HandleError(err);
		m_success.push_back(false);
	}

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);
		m_success.push_back(true);
	}

	virtual void Reset() override
	{
		m_success.clear();
	}

	std::vector<bool> m_success;

};

class CShelveClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "shelve"; }
};

class CUnshelveClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "unshelve"; }
};

class CEditClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "edit"; }

	virtual void OutputInfo(char level, const char* data) override
	{
		CBaseClientUser::OutputInfo(level, data);

		auto messagePos = strrchr(data, '-') + 2;
		if (StartsWith("can't edit exclusive file already opened", messagePos))
		{
			SetErrorIfNew(EVersionControlError::AlreadyCheckedOutByOthers, data);
			auto depotStreamLength = UpdateDepotStreamPath(data);
			const string& filePath = ExtractFilePath(data, depotStreamLength, 0);
			CreateFileStatusUpdate(filePath, false, CVersionControlFileStatus::eState_CheckedOutRemotely);
		}
	}

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		CreateFileStatusUpdate(dict, false, CVersionControlFileStatus::eState_CheckedOutLocally);
	}
};

class CDeleteClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "delete"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		CreateFileStatusUpdate(dict, false, CVersionControlFileStatus::eState_DeletedLocally);
	}

	virtual void OutputInfo(char level, const char* data) override
	{
		CBaseFileStatusClientUser::OutputInfo(level, data);
		string dataStr = data;
		auto lastDashPos = dataStr.rfind('-');
		if (lastDashPos != std::string::npos && StartsWith("can't delete", dataStr, lastDashPos + 2)) // "can't delete exclusive file already opened"
		{
			SetErrorIfNew(EVersionControlError::AlreadyCheckedOutByOthers);
		}
	}
};

class CRevertClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "revert"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		CreateFileStatusUpdate(dict);
		GetCurrentFileStatusUpdate().RemoveState(CVersionControlFileStatus::GetLocalStates());
		auto pHaveRevVar = dict->GetVar("haveRev");
		if (pHaveRevVar && strcmp(pHaveRevVar->Text(), "none") == 0)
		{
			GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_NotTracked);
		}
	}
};

class CFileStatClientUser : public CBaseFileStatusClientUser
{
public:
	static string GetName() { return "fstat"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);

		if (m_filter != EFileStatFilter::None)
		{
			m_files.emplace_back(UpdateDepotStreamAndExtractFile(dict->GetVar("depotFile")->Text()));
			return;
		}

		CreateFileStatusUpdate(dict, true);

		if (auto pActionVar = dict->GetVar("action"))
		{
			if (strcmp(pActionVar->Text(), "edit") == 0)
			{
				GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_CheckedOutLocally);
			}
			else if (strcmp(pActionVar->Text(), "delete") == 0)
			{
				GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_DeletedLocally);
			}
			else if (strcmp(pActionVar->Text(), "add") == 0)
			{
				GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_AddedLocally);
				GetCurrentFileStatusUpdate().RemoveState(CVersionControlFileStatus::eState_NotTracked);
			}
		}

		auto pHaveRevVar = dict->GetVar("haveRev");
		auto pHeadRevVar = dict->GetVar("headRev");

		if (!pHeadRevVar) // if doesn't exist on remote 
		{
			return;
		}

		if (pHaveRevVar) // also exists locally
		{
			if (std::atoi(pHeadRevVar->Text()) > std::atoi(pHaveRevVar->Text()))
			{
				auto pHeadActionVar = dict->GetVar("headAction");
				if (pHeadActionVar && strcmp(pHeadActionVar->Text(), "delete") == 0)
				{
					GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_DeletedRemotely);
				}
				else 
				{
					GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_UpdatedRemotely);
				}
			}
		}
		else // exists only on remote
		{
			GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_AddedRemotely);
		}

		if (auto pOtherOpenVar = dict->GetVar("otherOpen"))
		{
			GetCurrentFileStatusUpdate().AddState(CVersionControlFileStatus::eState_CheckedOutRemotely);
		}
	}

	virtual void Reset() override
	{
		CBaseFileStatusClientUser::Reset();
		m_files.clear();
	}

	void SetFilter(EFileStatFilter f) { m_filter = f; }

	std::vector<string> m_files;
	EFileStatFilter m_filter;
};

class CDiffClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "diff"; }

	virtual void OutputStat(StrDict* dict) override
	{
		CBaseClientUser::OutputStat(dict);
		m_files.emplace_back(UpdateDepotStreamAndExtractFile(dict->GetVar("depotFile")->Text()));
	}

	virtual void Reset() override
	{
		CBaseClientUser::Reset();
		m_files.clear();
	}

	std::vector<string> m_files;
};

class CPrintClientUser : public CBaseClientUser
{
public:
	static string GetName() { return "print"; }

	virtual void OutputText(const char* data, int length) override
	{
		CBaseClientUser::OutputText(data, length);
		m_fileContent += data;
	}

	virtual void Reset() override
	{
		CBaseClientUser::Reset();
		m_fileContent = "";
	}

	string m_fileContent;
};

std::unordered_map<string, std::shared_ptr<CBaseClientUser>, stl::hash_strcmp<string>, stl::hash_strcmp<string>> g_clientUsers;

template<class T>
T* GetP4ClientUser()
{
	T* cl = static_cast<T*>(g_clientUsers[T::GetName()].get());
	CRY_ASSERT_MESSAGE(cl, "there is no user client registered for %s", T::GetName());
	return cl;
}

template<class T>
void AddP4ClientUser(std::initializer_list<string> aliases = {}, bool enableDetailedOutput = false)
{
	std::shared_ptr<CBaseClientUser> cu = std::make_shared<T>();
	g_clientUsers[T::GetName()] = cu;
	if (enableDetailedOutput)
	{
		cu->EnableDetailedOutput();
	}
	for (const string& alias : aliases)
	{
		g_clientUsers[alias] = cu;
	}
}

void InitP4ClientUsers()
{
	AddP4ClientUser<CChangelistsClientUser>({ "changelists", "changes" });
	AddP4ClientUser<CDescribeClientUser>({ "describe" });
	AddP4ClientUser<CDepotFileNamesClientUser>({ "opened", "reconcile", "rec" });
	AddP4ClientUser<CStatusClientUser>({ "status" });
	AddP4ClientUser<CSyncClientUser>({ "sync" });
	AddP4ClientUser<CFilesClientUser>({ "files" });
	AddP4ClientUser<CChangelistClientUser>({ "changelist", "change" });
	AddP4ClientUser<CSubmitClientUser>({ "submit" }, true);
	AddP4ClientUser<CInfoClientUser>({ "info" }, true);
	AddP4ClientUser<CAddClientUser>({ "add" });
	AddP4ClientUser<CResolveClientUser>({ "resolve" });
	AddP4ClientUser<CShelveClientUser>({ "shelve" });
	AddP4ClientUser<CUnshelveClientUser>({ "unshelve" });
	AddP4ClientUser<CEditClientUser>({ "edit" });
	AddP4ClientUser<CDeleteClientUser>({ "delete" });
	AddP4ClientUser<CRevertClientUser>({ "revert" });
	AddP4ClientUser<CFileStatClientUser>({ "fstat" });
	AddP4ClientUser<CDiffClientUser>({ "diff" });
	AddP4ClientUser<CPrintClientUser>({ "print" });
	AddP4ClientUser<CBaseClientUser>({ "dummy" });
}

void ClearP4ClientUsers()
{
	g_clientUsers.clear();
}

}

CPerforceApiOutputParser::CPerforceApiOutputParser(const string& trackedFolderPath)
{
	using namespace Private_CPerforceApiOutputParser;
	g_trackedFolderPath = trackedFolderPath;
	InitP4ClientUsers();
}

CPerforceApiOutputParser::~CPerforceApiOutputParser()
{
	Private_CPerforceApiOutputParser::ClearP4ClientUsers();
}

void CPerforceApiOutputParser::ParsePendingChangelists(const string&, const string& workspace, std::vector<string>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CChangelistsClientUser>();
	result = std::move(cu->m_list);
}

void CPerforceApiOutputParser::ParseInfo(const string&, Info& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CInfoClientUser>();
	result = std::move(cu->m_info);
}

void CPerforceApiOutputParser::ParseStatus(const string&, std::vector<CVersionControlFileStatusUpdate>& result, std::vector<string>& reconcileResult) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CStatusClientUser>();
	result = std::move(cu->m_fileStatuses);
	reconcileResult = std::move(cu->m_reconcileList);
}

void CPerforceApiOutputParser::ParseDepotFileNames(const string&, std::vector<string>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CDepotFileNamesClientUser>();
	result = std::move(cu->m_files);
}

void CPerforceApiOutputParser::ParseDescribe(const string&, string& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CDescribeClientUser>();
	result = std::move(cu->m_output);
}

void CPerforceApiOutputParser::ParseSubmit(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CSubmitClientUser>();
	result = std::move(cu->m_fileStatuses);
}

void CPerforceApiOutputParser::ParseEdit(const string&, std::vector<CVersionControlFileStatusUpdate>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CEditClientUser>();
	result = std::move(cu->m_fileStatuses);
}

void CPerforceApiOutputParser::ParseAdd(const string&, std::vector<CVersionControlFileStatusUpdate>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CAddClientUser>();
	result = std::move(cu->m_fileStatuses);
}

void CPerforceApiOutputParser::ParseDelete(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CDeleteClientUser>();
	result = std::move(cu->m_fileStatuses);
}

void CPerforceApiOutputParser::ParseSync(const string&, std::vector<CVersionControlFileStatusUpdate>& result, 
	std::vector<string>& resultConflicting, SyncMode mode /* = Default*/) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CSyncClientUser>();
	if (mode == SyncMode::Default)
	{
		cu->ClearRemoteChanges();
	}
	else if (mode == SyncMode::Remove)
	{
		cu->AddUpdatedRemotelyState();
	}
	result = std::move(cu->m_fileStatuses);
	resultConflicting = std::move(cu->m_conflictingList);
}

void CPerforceApiOutputParser::ParseSync(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result, SyncMode mode /*= Default*/) const
{
	std::vector<string> conflicting;
	return ParseSync(perforceOutput, result, conflicting, mode);
}

void CPerforceApiOutputParser::ParseFiles(const string& perforceOutput, std::vector<int>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CFilesClientUser>();
	result = std::move(cu->m_states);
}

void CPerforceApiOutputParser::ParseRevert(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CRevertClientUser>();
	result = std::move(cu->m_fileStatuses);
}

void CPerforceApiOutputParser::ParseResolve(const string&, std::vector<bool>& results) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CResolveClientUser>();
	results = std::move(cu->m_success);
}

void CPerforceApiOutputParser::ParseShelve(const string&) const
{
	using namespace Private_CPerforceApiOutputParser;
	//GetP4ClientUser<CShelveClientUser>();
}

void CPerforceApiOutputParser::ParseUnshelve(const string&) const
{
	using namespace Private_CPerforceApiOutputParser;
	//GetP4ClientUser<CUnshelveClientUser>();
}

void CPerforceApiOutputParser::ParseDeleteShelve(const string&) const
{
	using namespace Private_CPerforceApiOutputParser;
	//GetP4ClientUser<CShelveClientUser>();
}

void CPerforceApiOutputParser::ParseDeleteChangelist(const string&) const
{
	using namespace Private_CPerforceApiOutputParser;
	//GetP4ClientUser<CChangelistClientUser>();
}

void CPerforceApiOutputParser::ParseCreateChangelist(const string& perforceOutput, string& result) const
{
	auto clStarPos = perforceOutput.find(' ') + 1;
	auto clEndPos = perforceOutput.find(' ', clStarPos);
	result = perforceOutput.substr(clStarPos, clEndPos - clStarPos);
}

void CPerforceApiOutputParser::ParseFileStat(const string& perforceOutput, std::vector<CVersionControlFileStatusUpdate>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CFileStatClientUser>();
	result = std::move(cu->m_fileStatuses);
}

void CPerforceApiOutputParser::ParseFileStat(const string& perforceOutput, std::vector<string>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CFileStatClientUser>();
	result = std::move(cu->m_files);
}

void CPerforceApiOutputParser::ParseDiff(const string& perforceOutput, std::vector<string>& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CDiffClientUser>();
	result = std::move(cu->m_files);
}

void CPerforceApiOutputParser::ParseReconcile(const string& perforceOutput, std::vector<string>& result, bool isPreview /*= true*/) const
{
	using namespace Private_CPerforceApiOutputParser;
	if (!isPreview)
	{
		result.clear();
	}
	else
	{
		auto cu = GetP4ClientUser<CDepotFileNamesClientUser>();
		result = std::move(cu->m_files);
	}
}

void CPerforceApiOutputParser::ParsePrint(const string& perforceOutput, string& result) const
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CPrintClientUser>();
	result = cu->m_fileContent;
}

SVersionControlError CPerforceApiOutputParser::GetError() const
{
	return Private_CPerforceApiOutputParser::g_error;
}

void CPerforceApiOutputParser::SetCriticalError(SVersionControlError error)
{
	if (error.isCritical)
	{
		Private_CPerforceApiOutputParser::g_error = error;
	}
}

void CPerforceApiOutputParser::SetFileStatFilter(EFileStatFilter f)
{
	using namespace Private_CPerforceApiOutputParser;
	auto cu = GetP4ClientUser<CFileStatClientUser>();
	cu->SetFilter(f);
}

ClientUser* CPerforceApiOutputParser::SetP4ClientUser(const string& command)
{
	using namespace Private_CPerforceApiOutputParser;
	auto it = g_clientUsers.find(command);
	CBaseClientUser* cu = nullptr;
	if (it != g_clientUsers.cend())
	{
		cu = it->second.get();
	}
	else
	{
		cu = g_clientUsers["dummy"].get();
	}
	cu->ResetData();
	return cu;
}
