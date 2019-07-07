// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include <vector>
#include <functional>
#include <CrySystem/ISystem.h>
#include <CrySystem/IConsole.h>
#include "ITestSystem.h"
#include "Command.h"

namespace CryTest
{
class CCommandFunction
{
public:
	explicit CCommandFunction(std::function<void()> callback) : m_callback(std::move(callback)) {}

	template<typename T, typename MemberT, typename ... ArgsT>
	CCommandFunction(T* object, MemberT memberFunc, ArgsT&& ... args)
	{
		static_assert(std::is_member_function_pointer<MemberT>::value, "Not member function");

		m_callback = [object, memberFunc, args ... /*copy capture needed for later*/]
		{ (object->*memberFunc)(std::move(args) ...); };
	}

	bool Update()
	{
		m_callback();
		return true;
	}

private:

	std::function<void()> m_callback;
};

class CCommandLog
{
public:
	template<typename ... ArgsT>
	explicit CCommandLog(const char* format, ArgsT&& ... args)
	{
		m_sContent.Format(format, std::forward<ArgsT>(args) ...);
	}

	bool Update()
	{
#ifdef CRY_TESTING
		gEnv->pSystem->GetITestSystem()->GetLog()->LogAlways("%s", m_sContent.c_str());
#endif // CRY_TESTING
		return true;
	}

private:

	string m_sContent;
};

class CCommandConsoleCmd
{
public:
	constexpr explicit CCommandConsoleCmd(const char* cmd) : m_cmd(cmd)
	{
	}

	bool Update()
	{
		CRY_ASSERT(m_cmd);
		gEnv->pConsole->ExecuteString(m_cmd);
		return true;
	}

private:

	const char* m_cmd = nullptr;
};

class CCommandLoadLevel : public ISystemEventListener
{
public:
	explicit CCommandLoadLevel(const char* szLevelName) : m_szLevelName(szLevelName)
	{
	}

	~CCommandLoadLevel()
	{
		 gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
	}

	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
	{
		if (event == ESYSTEM_EVENT_LEVEL_LOAD_END)
		{
			m_IsLoaded = true;
		}
		else if(event == ESYSTEM_EVENT_LEVEL_LOAD_ERROR)
		{
			CRY_ASSERT(false);
		}
	}

	bool Update()
	{
		if (!m_WasCalled)
		{
			gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "LoadLevelCommand");

			string mapCommand = "map ";
			mapCommand += m_szLevelName;
			gEnv->pConsole->ExecuteString(mapCommand);

			m_WasCalled = true;
		}

		if (m_IsLoaded)
		{
			return true;
		}

		return false;
	}

private:
	const char* m_szLevelName = nullptr;
	bool m_IsLoaded = false;
	bool m_WasCalled = false;
};

class CCommandWait
{
public:
	constexpr explicit CCommandWait(float timeOut)
		: m_timeOut(timeOut)
	{
	}

	bool Update()
	{
		if (!m_bStarted)
		{
			m_bStarted = true;
			m_startTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		}
		else
		{
			if (gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_startTime >= m_timeOut)
			{
				m_bStarted = false;
				return true;
			}
		}

		return false;
	}

private:

	bool  m_bStarted = false;
	float m_startTime = 0.f;
	float m_timeOut;
};

class CCommandRepeat
{
public:
	CCommandRepeat(int times, std::initializer_list<CCommand> commands)
		: m_times(times)
		, m_commands(std::move(commands))
	{}

	CCommandRepeat(int times, CCommand command)
		: m_times(times)
		, m_commands({ std::move(command) })
	{}

	bool Update()
	{
		if (m_currTimes == -1)
			m_currTimes = 0;
		if (m_currTimes >= m_times)
		{
			m_currTimes = -1;
			return true;
		}
		++m_currTimes;
		return false;
	}

	const std::vector<CCommand>& GetSubCommands() const
	{
		if (m_currTimes == -1)    //not running
			return m_emptyCommands;
		else
			return m_commands;
	}

private:
	int                   m_currTimes = 0;
	int                   m_times = 0;
	std::vector<CCommand> m_commands;
	std::vector<CCommand> m_emptyCommands;
};

}