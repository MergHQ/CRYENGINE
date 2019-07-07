// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <ResourceCompilerHelpers.h>
#include "Controls/QuestionDialog.h"

#include <CryCore/ToolsHelpers/ResourceCompilerHelper.inl>
#include <CryCore/ToolsHelpers/SettingsManagerHelpers.inl>
#include <CryCore/ToolsHelpers/EngineSettingsManager.inl>
#include <CrySystem/CryVersion.h>

namespace Private_ResourceCompilerHelpers
{

static QString ToQString(const SFileVersion& version)
{
	return QString("%1.%2.%3.%4").arg(version[3]).arg(version[2]).arg(version[1]).arg(version[0]);
}

struct SRcVersion : SFileVersion, IResourceCompilerListener
{
	virtual void OnRCMessage(MessageSeverity severity, const char* szText)
	{
		if (!v[3] && severity == MessageSeverity_Info)
		{
			// skip leading spaces
			while (*szText == ' ')
			{
				++szText;
			}

			const CryPathString key("Version ");

			if (strncmp(szText, key.c_str(), key.length()) == 0)
			{
				Set(szText + key.length());
				bFound = true;
			}
		}
	}

	bool bFound = false;
};

}

bool CResourceCompilerVersion::CheckIfValid(bool bSilent)
{
	using namespace Private_ResourceCompilerHelpers;

	const SFileVersion minimumVersion = SFileVersion("1.1.8.7");

	SRcVersion rcVersion;

	const CResourceCompilerHelper::ERcCallResult result = CResourceCompilerHelper::CallResourceCompiler(
		nullptr,
		nullptr,
		&rcVersion,
		false,
		CResourceCompilerHelper::ERcExePath::eRcExePath_editor,
		true,
		true,
		nullptr);

	if (bSilent)
	{
		return result == CResourceCompilerHelper::eRcCallResult_success;
	}

	switch (result)
	{
	case CResourceCompilerHelper::eRcCallResult_success:
		if (rcVersion < minimumVersion
			&& QDialogButtonBox::StandardButton::Yes != CQuestionDialog::SCritical(QObject::tr(""), QObject::tr("The Resource Compiler (Tools/rc/rc.exe) version is old.\n"
				"Installed version: %1\n"
				"Recommended version: %2\n"
				"As a result, Sandbox may behave unexpectedly.\n"
				"It is recommended to update the Resource Compiler.\n"
				"\n"
				"Do you want to continue anyway?")
				.arg(rcVersion.bFound ? ToQString(rcVersion) : QObject::tr("Could not find RC version information"))
				.arg(ToQString(minimumVersion)),
				QDialogButtonBox::StandardButton::No | QDialogButtonBox::StandardButton::Yes))
		{
			return false;
		}
		break;
	case CResourceCompilerHelper::eRcCallResult_notFound:
		if (QDialogButtonBox::StandardButton::Yes != CQuestionDialog::SCritical(QObject::tr(""), QObject::tr("The Resource Compiler (Tools/rc/rc.exe) could not be found.\n"
			"As a result, Sandbox may behave unexpectedly.\n"
			"\n"
			"Do you want to continue anyway?"), QDialogButtonBox::StandardButton::No | QDialogButtonBox::StandardButton::Yes))
		{
			return false;
		}
		break;
	default:
		if (QDialogButtonBox::StandardButton::Yes != CQuestionDialog::SCritical(QObject::tr(""), QObject::tr("The Resource Compiler (Tools/rc/rc.exe) is broken.\n"
			"As a result, Sandbox may behave unexpectedly.\n"
			"\n"
			"Do you want to continue anyway?"),
			QDialogButtonBox::StandardButton::No | QDialogButtonBox::StandardButton::Yes))
		{
			return false;
		}
		break;
	}
	return true;
}
