// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ISystem.h>
#include <CrySystem/CVarOverride.h>
#include <CryCore/BaseTypes.h>
#include <CryCore/BitFiddling.h>

#include <cstdlib>

// works like CXConsoleVariableInt but when changing it sets other console variables
// getting the value returns the last value it was set to - if that is still what was applied
// to the cvars can be tested with GetRealIVal()
class CXConsoleVariableCVarGroup : public CXConsoleVariableInt, public ILoadConfigurationEntrySink
{
public:
	CXConsoleVariableCVarGroup(IConsole* pConsole, const char* szName, const char* szFileName, int flags);
	~CXConsoleVariableCVarGroup();

	// Returns:
	//   part of the help string - useful to log out detailed description without additional help text
	string GetDetailedInfo() const;

	// interface ICVar -----------------------------------------------------------------------------------

	virtual const char* GetHelp() const override;

	virtual int         GetRealIVal() const override;

	virtual void        DebugLog(const int expectedValue, const ICVar::EConsoleLogMode mode) const override;

	virtual void        Set(int i) override;

	// ConsoleVarFunc ------------------------------------------------------------------------------------

	static void OnCVarChangeFunc(ICVar* pVar);

	// interface ILoadConfigurationEntrySink -------------------------------------------------------------

	virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup) override;
	virtual void OnLoadConfigurationEntry_End() override;

	virtual void GetMemoryUsage(class ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_defaultValue);
		pSizer->AddObject(m_CVarGroupStates);

	}
private: // --------------------------------------------------------------------------------------------

	struct SCVarGroup
	{
		std::map<string, string> m_KeyValuePair;                    // e.g. m_KeyValuePair["r_fullscreen"]="0"
		void                     GetMemoryUsage(class ICrySizer* pSizer) const
		{
			pSizer->AddObject(m_KeyValuePair);
		}
	};

	SCVarGroup         m_CVarGroupDefault;
	typedef std::map<int, SCVarGroup*> TCVarGroupStateMap;
	TCVarGroupStateMap m_CVarGroupStates;
	string             m_defaultValue;                           // used by OnLoadConfigurationEntry_End()

	void ApplyCVars(const SCVarGroup& group, const SCVarGroup* pExclude = 0);

	const char* GetHelpInternal();

	// Arguments:
	//   sKey - must exist, at least in default
	//   pSpec - can be 0
	string GetValueSpec(const string& key, const int* pSpec = 0) const;

	// should only be used by TestCVars()
	// Returns:
	//   true=all console variables match the state (excluding default state), false otherwise
	bool _TestCVars(const SCVarGroup& group, const ICVar::EConsoleLogMode mode, const SCVarGroup* pExclude = 0) const;

	// Arguments:
	//   pGroup - can be 0 to test if the default state is set
	// Returns:
	//   true=all console variables match the state (including default state), false otherwise
	bool TestCVars(const SCVarGroup* pGroup, const ICVar::EConsoleLogMode mode = ICVar::eCLM_Off) const;
};
