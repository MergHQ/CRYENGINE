// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SettingsManager.h"

#include <CrySerialization/IArchiveHost.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/Utils/StackString.h>

namespace Schematyc
{
	bool CSettingsManager::RegisterSettings(const char* szName, const ISettingsPtr& pSettings)
	{
		SCHEMATYC_CORE_ASSERT_FATAL(szName && szName[0] && pSettings);
		if(szName && szName[0] && pSettings)
		{
			// #SchematycTODO : Validate name and check for collision/overlap!
			return m_settings.insert(Settings::value_type(szName, pSettings)).second;
		}
		return false;
	}

	ISettings* CSettingsManager::GetSettings(const char* szName) const
	{
		Settings::const_iterator itSettings = m_settings.find(szName);
		return itSettings != m_settings.end() ? itSettings->second.get() : nullptr;
	}

	void CSettingsManager::VisitSettings(const SettingsVisitor& visitor) const
	{
		SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
		if(!visitor.IsEmpty())
		{
			for(const Settings::value_type& settings : m_settings)
			{
				if(visitor(settings.first, settings.second) == EVisitStatus::Stop)
				{
					break;
				}
			}
		}
	}

	void CSettingsManager::LoadAllSettings()
	{
		LOADING_TIME_PROFILE_SECTION;

		const char* szFileFormat = GetSchematycCore().GetFileFormat();
		const char* szSettingsFolder = GetSchematycCore().GetSettingsFolder();
		for(const Settings::value_type& settings : m_settings)
		{
			const char*  szName = settings.first.c_str();
			CStackString fileName = szSettingsFolder;
			fileName.append("/");
			fileName.append(szName);
			fileName.append(".");
			fileName.append(szFileFormat);
			fileName.MakeLower();
			if (!strcmp(szFileFormat, "json"))
			{
				Serialization::LoadJsonFile(*settings.second, fileName);
			}
			else if (!strcmp(szFileFormat, "xml"))
			{
				Serialization::LoadXmlFile(*settings.second, fileName);
			}
			else
			{
				SCHEMATYC_CORE_ERROR("Unsupported file format: %s", szFileFormat);
			}
		}
	}

	void CSettingsManager::SaveAllSettings()
	{
		LOADING_TIME_PROFILE_SECTION;

		const char* szFileFormat = GetSchematycCore().GetFileFormat();
		const char* szSettingsFolder = GetSchematycCore().GetSettingsFolder();
		for(Settings::value_type& settings : m_settings)
		{
			const char*  szName = settings.first.c_str();
			CStackString fileName = szSettingsFolder;
			fileName.append("/");
			fileName.append(szName);
			fileName.append(".");
			fileName.append(szFileFormat);
			fileName.MakeLower();
			if (!strcmp(szFileFormat, "json"))
			{
				Serialization::SaveJsonFile(fileName, *settings.second);
			}
			else if (!strcmp(szFileFormat, "xml"))
			{
				Serialization::SaveXmlFile(fileName, *settings.second, szName);
			}
			else
			{
				SCHEMATYC_CORE_ERROR("Unsupported file format: %s", szFileFormat);
			}
		}
	}
}