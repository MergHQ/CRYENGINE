// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "Preferences.h"
#include "QtUtil.h"
#include <IEditor.h>

#include <CryString/CryPath.h>
#include <CrySystem/IProjectManager.h>

#include <CrySerialization/yasli/JSONOArchive.h>
#include <CrySerialization/yasli/JSONIArchive.h>
#include <CrySerialization/yasli/STL.h>

#include <QJsonDocument>
#include <QFile>

namespace Private_Preferences
{
const char* szDefaultPath = "Editor/Preferences";
const char* szPreferencesPath = "Preferences.pref";
}

QString SPreferencePage::GetSerializedProperties()
{
	yasli::JSONOArchive ar;
	ar(*this, m_name, m_name);
	return ar.buffer();
}

void SPreferencePage::FromSerializedProperties(const QByteArray& jsonBlob)
{
	yasli::JSONIArchive ar;
	ar.open(jsonBlob, jsonBlob.length());
	ar(*this, m_name, m_name);
}

CPreferences::CPreferences()
	: CUserData({ Private_Preferences::szPreferencesPath })
	, m_bIsLoading(false)
{
	signalSettingsChanged.Connect(this, &CPreferences::Save);
}

CPreferences::~CPreferences()
{
	signalSettingsChanged.DisconnectObject(this);
	Save();
	for (auto ite = m_preferences.begin(); ite != m_preferences.end(); ++ite)
	{
		std::vector<SPreferencePage*> m_preferencePages = ite->second;
		for (auto i = 0; i < m_preferencePages.size(); ++i)
			delete m_preferencePages[i];
	}
}

void CPreferences::Init()
{
	Load();
}

void CPreferences::Load()
{
	struct LoadingPreferences
	{
		LoadingPreferences(bool& loading) : m_bIsLoading(loading) { m_bIsLoading = true; }
		~LoadingPreferences() { m_bIsLoading = false; }
		bool& m_bIsLoading;
	};

	LoadingPreferences isLoading(m_bIsLoading);
	QVariant state = UserDataUtil::Load(Private_Preferences::szPreferencesPath);

	if (state.isValid())
	{
		SetState(state);
		return;
	}

	// If the user doesn't have any set preferences, then load project preferences
	QString projectPreferences(GetIEditor()->GetProjectManager()->GetCurrentProjectDirectoryAbsolute());
	projectPreferences = projectPreferences + "/" + Private_Preferences::szDefaultPath;

	QFile file(projectPreferences);

	// If no project preferences exist either, then just use engine deafaults
	if (!file.open(QIODevice::ReadOnly))
		return;

	SetState(QJsonDocument::fromJson(file.readAll()).toVariant());
}

void CPreferences::SetState(const QVariant& state)
{
	QVariantMap preferenceMap = state.toMap();
	QList<QString> keys = preferenceMap.keys();

	for (const auto& path : keys)
	{
		auto ite = m_preferences.find(QtUtil::ToString(path));

		if (ite == m_preferences.end())
			continue;

		QVariantMap pagesMap = preferenceMap[path].toMap();
		std::vector<SPreferencePage*> pages = ite->second;

		for (auto pageIte = pagesMap.constBegin(); pageIte != pagesMap.constEnd(); ++pageIte)
		{
			for (auto i = 0; i < pages.size(); ++i)
			{
				if (pageIte.key() == QtUtil::ToQString(pages[i]->GetName()))
				{
					QJsonDocument pageDoc = QJsonDocument::fromVariant(pageIte.value());
					pages[i]->FromSerializedProperties(pageDoc.toJson());
				}
			}
		}
	}
}

void CPreferences::Save()
{
	if (m_bIsLoading)
		return;

	QVariantMap preferencesVariant;
	for (auto ite = m_preferences.begin(); ite != m_preferences.end(); ++ite)
	{
		std::vector<SPreferencePage*> pages = ite->second;
		QVariantMap pagesVariant;
		for (SPreferencePage* pPage : pages)
		{
			// Create a json doc out of the yasli blob
			QJsonDocument pageDoc = QJsonDocument::fromJson(pPage->GetSerializedProperties().toUtf8());
			// Transform our yasli doc into a variant so we can then save a valid json document
			pagesVariant[QtUtil::ToQString(pPage->GetName())] = pageDoc.toVariant();
		}

		preferencesVariant[QtUtil::ToQString(ite->first)] = pagesVariant;
	}

	UserDataUtil::Save(Private_Preferences::szPreferencesPath, QJsonDocument::fromVariant(preferencesVariant).toJson());
}

void CPreferences::Reset(const char* path)
{
	if (m_bIsLoading)
		return;

	auto ite = m_preferences.find(path);
	if (ite == m_preferences.end())
		return;

	std::vector<SPreferencePage*> pages = ite->second;
	for (SPreferencePage* pPage : pages)
		pPage->signalRequestReset();

	Save();
	signalSettingsReset();
}

void CPreferences::AddPage(SPreferencePage* pPreferencePage)
{
	std::vector<SPreferencePage*>& preferencePages = m_preferences[pPreferencePage->GetPath()];
	preferencePages.push_back(pPreferencePage);
	std::sort(preferencePages.begin(), preferencePages.end(), [](const SPreferencePage* pFirst, const SPreferencePage* pSecond)
	{
		static const char* szGeneral = "General";

		if (!pFirst->GetName().find(szGeneral))
			return true;
		if (!pSecond->GetName().find(szGeneral))
			return false;

		return pFirst->GetName() < pSecond->GetName();
	});

}

std::vector<SPreferencePage*> CPreferences::GetPages(const char* path)
{
	auto ite = m_preferences.find(path);
	if (ite == m_preferences.end())
		return std::vector<SPreferencePage*>();

	return ite->second;
}

bool CPreferences::Serialize(yasli::Archive& ar)
{
	for (auto ite = m_preferences.begin(); ite != m_preferences.end(); ++ite)
	{
		std::vector<SPreferencePage*> preferencePages = ite->second;
		for (SPreferencePage* pPreferencePage : preferencePages)
			ar(*pPreferencePage, pPreferencePage->GetName(), pPreferencePage->GetName());
	}
	return true;
}
