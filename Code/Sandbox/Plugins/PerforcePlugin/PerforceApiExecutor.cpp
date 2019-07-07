// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "PerforceApiExecutor.h"
#include "PerforceApiOutputParser.h"
#include <p4/clientapi.h>
#include <CrySystem/ISystem.h>

namespace Private_CPerforceApiExecutor
{

const int MAX_ARGS_OUTPUT_LENGTH = 200;

static string g_port;
static string g_workspace;
static string g_user;
static string g_password;

string ToString(const std::vector<string>& strings)
{
	string ret;
	for (const auto& str : strings)
	{
		ret += str + " ";
	}
	return ret.substr(0, ret.size() - 1);
}

string ExecutePerforceCommand(const string& command, const string& arguments = "")
{
	return "";
}

string ExecutePipedPerforceCommands(const string& firstCommand, const string& secondCommand)
{
	return "";
}

class CClientApiWrapper : public ClientApi
{
public:
	explicit CClientApiWrapper(bool bTagged)
		: m_isTagged(bTagged)
	{}

	~CClientApiWrapper()
	{
		DropConnection();
	}

	bool Run(const char* func, const std::vector<string>& arguments, ClientUser* cu = nullptr)
	{
		CArgsContainer argsContainer(arguments);

		CRY_ASSERT_MESSAGE(CryGetCurrentThreadId() != gEnv->mMainThreadId, "Version Control task can't be run on main thread");

		if (!CheckConnection())
		{
			m_pParser->SetCriticalError({ EVersionControlError::NoConnection, "Couldn't establish connection to perforce server" });
			return false;
		}
		if (arguments.size())
		{
			SetArgv(static_cast<int>(arguments.size()), argsContainer.GetArgs());
		}
		if (!cu)
		{
			cu = m_pParser->SetP4ClientUser(func);
		}
		string argsStr = "";
		for (int i = 0; i < arguments.size() && i < MAX_ARGS_OUTPUT_LENGTH; ++i)
		{
			argsStr += arguments[i] + " ";
			if (argsStr.size() >= MAX_ARGS_OUTPUT_LENGTH)
			{
				argsStr += "(...)";
				break;
			}
		}

		CryLog("Perforce command execution: p4 %s %s", func, argsStr);
		ClientApi::Run(func, cu);
		return true;
	}

	void DropConnection()
	{
		if (!Dropped())
		{
			Error e;
			Final(&e);
			if (!e.Test() && e.GetSeverity() > 0)
			{
				StrBuf msg;
				e.Fmt(&msg);
				CryLog("Perforce error: %s", msg.Text());
			}
		}
		m_isInitizlied = false;
	}

	bool CheckConnection()
	{
		if (!m_isInitizlied || Dropped())
		{
			SetProtocol("enableStreams", "");
			if (m_isTagged)
			{
				SetProtocol("tag", "");
			}

			StrBuf sb;
			sb.Set("CryEngine");
			SetProg(&sb);

			Error e;
			Final(&e);
			Init(&e);
			if (!e.Test())
			{
				m_isInitizlied = true;
				CryLog("Perforce connected");
			}
			else
			{
				StrBuf msg;
				e.Fmt(&msg);
				m_isInitizlied = false;
				CryLog("Perforce: Connection failed! %s", msg.Text());
				return false;
			}
		}
		return true;
	}

	CPerforceApiOutputParser* m_pParser{ nullptr };

private:
	static constexpr int STATIC_SIZE = 10;

	class CArgsContainer
	{
	public:
		CArgsContainer(const std::vector<string>& arguments)
			: m_originalArgs(arguments)
		{}

		~CArgsContainer()
		{
			if (m_isDymanic)
			{
				delete[] m_args;
			}
		}

		char** GetArgs()
		{
			if (m_originalArgs.size() <= STATIC_SIZE)
			{
				m_args = s_args;
			}
			else
			{
				m_args = new char*[m_originalArgs.size()];
				m_isDymanic = true;
			}
			for (int i = 0; i < m_originalArgs.size(); ++i)
			{
				m_args[i] = const_cast<char*>(m_originalArgs[i].c_str());
			}
			return m_args;
		}

	private:
		static char* s_args[STATIC_SIZE];

		char** m_args{ nullptr };
		const std::vector<string>& m_originalArgs;

		bool m_isDymanic{ false };
	};

	bool m_isTagged{ false };
	bool m_isInitizlied{ false };
};

char* CClientApiWrapper::CArgsContainer::s_args[];

class CChangelistDescriptionClientUser : public ClientUser
{
public:
	CChangelistDescriptionClientUser(const string& d, bool bEmpty)
		: m_desc(d)
		, m_bEmpty(bEmpty)
	{}

	virtual void OutputInfo(char level, const char *data) override
	{
		m_output = data;
	}

	virtual void InputData(StrBuf *strbuf, Error *e) override
	{
		strbuf->Set(m_spec);
	}

	virtual void OutputStat(StrDict* dict) override
	{
		AddSpec(dict, "Change");
		AddSpec(dict, "Client");
		AddSpec(dict, "Date");
		AddSpec(dict, "User");
		AddSpec(dict, "Status");
		AddSpec(dict, "Description", AddTabs(m_desc), false);
		AddSpec(dict, "Jobs");
		AddSpec(dict, "Type");
		if (!m_bEmpty)
		{
			AddFiles(dict);
		}
		AddSpec(dict, "ImportedBy");
		AddSpec(dict, "Identity");
	}

	const string& GetOutput() const { return m_output; }

private:
	void AddSpec(StrDict* dict, const char* name, const string value = "", bool addLeadingTab = true)
	{
		auto pStr = dict->GetVar(name);
		if (pStr)
		{
			m_spec.Append(name);
			m_spec.Append(":\n");
			if (addLeadingTab)
			{
				m_spec.Append("\t");
			}
			m_spec.Append(value.empty() ? pStr->Text() : value.c_str());
			m_spec.Append("\n\n");
		}
	}

	void AddFiles(StrDict* dict)
	{
		m_spec.Append("Files:");
		StrRef var, val;
		for (int i = 0; dict->GetVar(i, var, val); ++i)
		{
			if (var.Text()[0] == 'F')
			{
				m_spec.Append("\n\t");
				m_spec.Append(val.Text());
			}
		}
		m_spec.Append("\n\n");
	}

	const char* AddTabs(const string& str)
	{
		string result;
		int startIndex = 0;
		while (startIndex < str.size())
		{
			auto newLineIndex = str.find('\n', startIndex);
			if (newLineIndex == std::string::npos)
			{
				newLineIndex = str.size();
			}
			result += '\t' + str.substr(startIndex, newLineIndex - startIndex + 1);
			startIndex = newLineIndex + 1;
		}
		return result;
	}

	const string& m_desc;
	StrBuf m_spec;
	string m_output;
	bool m_bEmpty;
};

void AppendVector(std::vector<string>& dest, const std::vector<string>& src)
{
	dest.insert(dest.cend(), src.begin(), src.end());
}

std::vector<string> MergeVectors(std::vector<string> v1, const std::vector<string>& v2)
{
	AppendVector(v1, v2);
	return v1;
}

class CFileStatFilterSetter
{
public:
	CFileStatFilterSetter(CClientApiWrapper& clientApi, EFileStatFilter f)
		: m_clientApi(clientApi)
	{
		m_clientApi.m_pParser->SetFileStatFilter(f);
	}

	~CFileStatFilterSetter()
	{
		m_clientApi.m_pParser->SetFileStatFilter(EFileStatFilter::None);
	}

private:
	CClientApiWrapper& m_clientApi;
};

}

CPerforceApiExecutor::CPerforceApiExecutor(const string& rootPath, CPerforceApiOutputParser* pParser)
	: m_rootPath(rootPath)
{
	using namespace Private_CPerforceApiExecutor;

	m_pP4TaggedClient.reset(new CClientApiWrapper(true));
	m_pP4UntaggedClient.reset(new CClientApiWrapper(false));
	m_pP4TaggedClient->SetCwd(m_rootPath);
	m_pP4TaggedClient->m_pParser = pParser;
	m_pP4UntaggedClient->SetCwd(m_rootPath);
	m_pP4UntaggedClient->m_pParser = pParser;
}

string CPerforceApiExecutor::Info()
{
	TryRun("info", {});

	return "";
}

string CPerforceApiExecutor::Status(const string& changelist /* = "" */)
{
	std::vector<string> args;
	if (changelist != "")
	{
		args = { "-c", changelist };
	}
	TryRun("status", args, false);

	return "";
}

string CPerforceApiExecutor::Status(const std::vector<string>& filePaths)
{
	TryRun("status", filePaths, false);

	return "";
}

string CPerforceApiExecutor::Opened(const string& changelist /* = "" */)
{
	std::vector<string> args; 
	if (changelist != "")
	{
		args = { "-c", changelist };
	}
	TryRun("opened", args);

	return "";
}

string CPerforceApiExecutor::Reopen(const string& changelist, const std::vector<string>& filePaths)
{
	using namespace Private_CPerforceApiExecutor;

	TryRun("reopen", MergeVectors({ "-c", changelist }, filePaths));

	return "";
}

string CPerforceApiExecutor::Add(const std::vector<string>& filePaths, bool force /*= false*/)
{
	std::vector<string> args;
	if (force)
	{
		args.push_back("-d");
	}
	args.insert(args.end(), filePaths.cbegin(), filePaths.cend());
	TryRun("add", args);

	return "";
}

string CPerforceApiExecutor::Edit(const std::vector<string>& filePaths)
{
	TryRun("edit", filePaths);

	return "";
}

string CPerforceApiExecutor::Delete(const std::vector<string>& filePaths)
{
	TryRun("delete", filePaths);

	return "";
}

string CPerforceApiExecutor::ReconcilePreview(int filter /*= eReconcile_All*/, const std::vector<string>& filePaths /*= {}*/)
{
	using namespace Private_CPerforceApiExecutor;
	std::vector<string> args = { "-n" };
	if (filter != eReconcile_All)
	{
		string filterStr = "-";
		if (filter & eReconcile_Add)
		{
			filterStr += "a";
		}
		if (filter & eReconcile_Edit)
		{
			filterStr += "e";
		}
		if (filter & eReconcile_Delete)
		{
			filterStr += "d";
		}
		args.push_back(filterStr);
	}
	AppendVector(args, filePaths);
	TryRun("reconcile", args, false);

	return "";
}

string CPerforceApiExecutor::Reconcile(const std::vector<string>& filePaths)
{
	TryRun("reconcile", filePaths, false);

	return "";
}

string CPerforceApiExecutor::DescribeChangelist(const string& changelist)
{
	TryRun("describe", { "-s", changelist });

	return "";
}

string CPerforceApiExecutor::SetChangelistDescription(const string& changelist, const string& description)
{
	using namespace Private_CPerforceApiExecutor;

	CChangelistDescriptionClientUser cu(description, false);
	TryRun("change", { "-o", changelist }, true, &cu);
	TryRun("change", { "-i" }, true, &cu);

	return "";
}

string CPerforceApiExecutor::CreateChangelist(const string& description, const std::vector<string>& filePaths /* = {} */)
{
	using namespace Private_CPerforceApiExecutor;

	CChangelistDescriptionClientUser cu(description, true);
	TryRun("change", { "-o" }, true, &cu);
	TryRun("change", { "-i" }, true, &cu);

	return cu.GetOutput();
}

string CPerforceApiExecutor::Sync(const std::vector<string>& filePaths /* = {} */, bool force /* = false */, int rev /* = -1 */)
{
	using namespace Private_CPerforceApiExecutor;

	std::vector<string> args;
	if (force)
	{
		args.push_back("-f");
	}
	if (!filePaths.empty())
	{
		if (rev >= 0) 
		{
			args.reserve(args.size() + filePaths.size());
			const string revString = string().Format("#%d", rev);
			for (const string& path : filePaths) 
			{
				args.push_back(path + revString);
			}
		} else
		{
			AppendVector(args, filePaths);
		}
	}
	TryRun("sync", args);

	return "";
}

string CPerforceApiExecutor::SyncPreview()
{
	TryRun("sync", { "-n" });

	return "";
}

string CPerforceApiExecutor::Files(const std::vector<string>& filePaths)
{
	TryRun("files", filePaths, false);

	return "";
}

string CPerforceApiExecutor::PendingChangelists(const string& user)
{
	TryRun("changelists", { "-s", "pending", "-u", m_pP4TaggedClient->GetUser().Text(), "-c", m_pP4TaggedClient->GetClient().Text() });

	return "";
}

string CPerforceApiExecutor::SubmitChangelist(const string& changelist)
{
	TryRun("submit", { "-c", changelist });

	return "";
}

string CPerforceApiExecutor::ResolveOurs(const std::vector<string>& filePaths)
{
	using namespace Private_CPerforceApiExecutor;

	TryRun("resolve", MergeVectors({ "-ay" }, filePaths));

	return "";
}

string CPerforceApiExecutor::ResolveTheir(const std::vector<string>& filePaths)
{
	using namespace Private_CPerforceApiExecutor;

	TryRun("resolve", MergeVectors({ "-at" }, filePaths));

	return "";
}

string CPerforceApiExecutor::Have(const std::vector<string>& filePaths)
{
	TryRun("have", filePaths);

	return "";
}

string CPerforceApiExecutor::Revert(const std::vector<string>& paths, bool onlyUnchanged /*= false*/, bool clearOnlyState /*= false*/)
{
	using namespace Private_CPerforceApiExecutor;

	std::vector<string> args;
	if (onlyUnchanged)
	{
		args.push_back("-a");
	}
	if (clearOnlyState)
	{
		args.push_back("-k");
	}
	AppendVector(args, paths);
	TryRun("revert", args);

	return "";
}

string CPerforceApiExecutor::Shelve(const std::vector<string>& filePaths, const string& changelist)
{
	using namespace Private_CPerforceApiExecutor;

	TryRun("shelve", MergeVectors({ "-f", "-c", changelist }, filePaths), false);

	return "";
}

string CPerforceApiExecutor::Unshelve(const string& shelve, const string& targetChangelist)
{
	TryRun("unshelve", { "-s", shelve, "-c", targetChangelist }, false);

	return "";
}

string CPerforceApiExecutor::DeleteShelve(const string& shelve)
{
	TryRun("shelve", { "-c", shelve, "-d" }, false);

	return "";
}

string CPerforceApiExecutor::DeleteChangelist(const string& changelist)
{
	TryRun("change", { "-d", changelist });

	return "";
}

string CPerforceApiExecutor::FileStat(EFileStatFilter f, const std::vector<string>& filePaths /*={ "..." }*/)
{
	using namespace Private_CPerforceApiExecutor;

	auto filterSetter = CFileStatFilterSetter(*m_pP4TaggedClient, f);

	std::vector<string> args;
	switch(f)
	{
	case EFileStatFilter::AddedLocally:
		AppendVector(args, { "-Ro", "-F", "action=add" }); // Ro - Limit output to open files in the current workspace
		break;
	case EFileStatFilter::AddedRemotely:
		AppendVector(args, { "-F", "headRev | ^haveRev" }); 
		break;
	case EFileStatFilter::CheckedOutByOthers:
		AppendVector(args, { "-F", "otherOpen" });
		break;
	case EFileStatFilter::Tracked:
		AppendVector(args, { "-F", "headRev & ^headAction=delete & ^headAction=move/delete" });
		break;
	case EFileStatFilter::DeletedRemotely:
		AppendVector(args, { "-F", "haveRev & (headAction=delete | headAction=move/delete)" });
		break;
	}
	AppendVector(args, { "-T", "depotFile" });
	AppendVector(args, filePaths);
	TryRun("fstat", args);

	return "";
}

string CPerforceApiExecutor::FileStat(const std::vector<string>& filePaths)
{
	using namespace Private_CPerforceApiExecutor;

	TryRun("fstat", MergeVectors({ "-T", "depotFile haveRev headRev headAction action otherOpen" }, filePaths));

	return "";
}

string CPerforceApiExecutor::Diff(const std::vector<string>& filePaths /*= {"..."}*/)
{
	using namespace Private_CPerforceApiExecutor;

	// -sa Show only the names of opened files that are different from the revision in the depot, or are missing.
	// -se Show only the names of unopened files in the client workspace that are different than the revision in the depot.
	// -sd Show only the names of unopened files that are missing from the client workspace, but present in the depot.
	TryRun("diff", MergeVectors({"-sa"}, filePaths));

	return "";
}

string CPerforceApiExecutor::Logout()
{
	TryRun("logout", {});

	return "";
}

string CPerforceApiExecutor::Print(const string& file)
{
	string fileWithRevision = file + "#head";
	TryRun("print", { "-q" , fileWithRevision }, false);

	return "";
}

string CPerforceApiExecutor::CheckLogin()
{
	TryRun("login", { "-s" });

	return "";
}

void CPerforceApiExecutor::UpdateSettings(const string& port, const string& workspace, const string& user, const string& password)
{
	using namespace Private_CPerforceApiExecutor;

	auto oldPort = m_pP4TaggedClient->GetPort().Text();
	auto newPort = port.c_str();
	auto oldUser = m_pP4TaggedClient->GetUser().Text();
	auto newUser = user.c_str();
	auto oldPass = m_pP4TaggedClient->GetPassword().Text();
	auto newPass = password.c_str();
	auto oldWorkspace = m_pP4TaggedClient->GetClient().Text();
	auto newWorkspace = workspace.c_str();

	if (strcmp(oldPort, newPort) != 0 || strcmp(oldUser, newUser) != 0 || strcmp(oldPass, newPass) != 0
		|| strcmp(oldWorkspace, newWorkspace) != 0)
	{
		g_port = port;
		g_workspace = workspace;
		g_user = user;
		g_password = password;
		CreateNewClients();
	}
	
}

bool CPerforceApiExecutor::TryRun(const char* func, const std::vector<string>& args, bool isTagged /*= true*/, ClientUser* cu /*= nullptr*/)
{
	auto pClient = isTagged ? m_pP4TaggedClient.get() : m_pP4UntaggedClient.get();
	if (!pClient->Run(func, args, cu))
	{
		CryLog("Perforce: Try to reestablish connection.");
		CreateNewClients();
		pClient = isTagged ? m_pP4TaggedClient.get() : m_pP4UntaggedClient.get();
		return pClient->Run(func, args, cu);
	}
	return true;
}

void CPerforceApiExecutor::CreateNewClients()
{
	using namespace Private_CPerforceApiExecutor;

	m_pP4TaggedClient->DropConnection();
	m_pP4UntaggedClient->DropConnection();

	auto pParser = m_pP4TaggedClient->m_pParser;

	const char* port = g_port;
	const char* workspace = g_workspace;
	const char* user = g_user;
	const char* password = g_password;

	m_pP4TaggedClient.reset(new CClientApiWrapper(true));
	m_pP4UntaggedClient.reset(new CClientApiWrapper(false));
	m_pP4TaggedClient->SetCwd(m_rootPath);
	m_pP4TaggedClient->m_pParser = pParser;
	m_pP4UntaggedClient->SetCwd(m_rootPath);
	m_pP4UntaggedClient->m_pParser = pParser;

	Error e;
	m_pP4TaggedClient->DefinePort(port, &e);
	m_pP4UntaggedClient->DefinePort(port, &e);
	m_pP4TaggedClient->DefineClient(workspace, &e);
	m_pP4UntaggedClient->DefineClient(workspace, &e);
	m_pP4TaggedClient->DefineUser(user, &e);
	m_pP4UntaggedClient->DefineUser(user, &e);
	m_pP4TaggedClient->DefinePassword(password, &e);
	m_pP4UntaggedClient->DefinePassword(password, &e);
}
