// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QString>
#include <CrySandbox/CrySignal.h>

//////////////////////////////////////////////////////////////////////////
/** Various editor settings.
 */
struct SEditorSettings
{
	static void Load();
	static void LoadFrom(const QString& filename);

	static void LoadValue(const char* sSection, const char* sKey, int& value);
	static void LoadValue(const char* sSection, const char* sKey, uint32& value);
	static void LoadValue(const char* sSection, const char* sKey, unsigned long& value);
	static void LoadValue(const char* sSection, const char* sKey, float& value);
	static void LoadValue(const char* sSection, const char* sKey, bool& value);
	static void LoadValue(const char* sSection, const char* sKey, string& value);

private:
	SEditorSettings();
};
