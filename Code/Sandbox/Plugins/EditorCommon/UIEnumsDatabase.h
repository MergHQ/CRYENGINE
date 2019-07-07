// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "EditorCommonAPI.h"

struct EDITOR_COMMON_API CUIEnumsDatabase_SEnum
{
	string              name;
	std::vector<string> strings; // Display Strings.
	std::vector<string> values;  // Corresponding Values.

	const string& NameToValue(const string& name);
	const string& ValueToName(const string& value);
	const char*   NameToValue(const char* name)  { return NameToValue(string(name)); }  // for CString conversion
	const char*   ValueToName(const char* value) { return ValueToName(string(value)); } // for CString conversion
};

//////////////////////////////////////////////////////////////////////////
// Stores string associates to the enumeration collections for UI.
//////////////////////////////////////////////////////////////////////////
class EDITOR_COMMON_API CUIEnumsDatabase
{
public:
	CUIEnumsDatabase();
	~CUIEnumsDatabase();

	void                    SetEnumStrings(const string& enumName, const std::vector<string>& sStringsArray);
	CUIEnumsDatabase_SEnum* FindEnum(const string& enumName) const;

private:
	typedef std::map<string, CUIEnumsDatabase_SEnum*> Enums;
	Enums m_enums;
};
