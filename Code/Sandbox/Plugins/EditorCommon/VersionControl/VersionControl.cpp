// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControl.h"
#include "IEditor.h"
#include "IVersionControlAdapter.h"
#include "VersionControlInitializer.h"
#include "EditorFramework/Preferences.h"
#include <CrySerialization/StringList.h>
#include <CrySystem/ISystem.h>
#include <memory>

namespace Private_VersionControl
{

static const string VCS_NONE = "None";

struct EDITOR_COMMON_API SVersionControlPreferences : public SPreferencePage
{
	SVersionControlPreferences::SVersionControlPreferences()
		: SPreferencePage("General", "Version Control/General")
		, vcsName(VCS_NONE)
	{}

	virtual bool Serialize(yasli::Archive& ar) override
	{
		string currentVCS = vcsName;
		Serialization::StringList vcsList;
		vcsList.push_back(VCS_NONE);

		std::vector<IClassDesc*> classes;
		GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_VCS_PROVIDER, classes);
		for (const auto pClass : classes)
		{
			vcsList.push_back(pClass->ClassName());
		}

		Serialization::StringListValue vcsListValue(vcsList, std::max(vcsList.find(currentVCS), 0));

		ar(vcsListValue, "vcs", "Version Control");

		auto& vcs = CVersionControl::GetInstance();

		bool hasChanges = false;
		if (ar.isInput())
		{
			hasChanges = vcsName != vcsListValue.c_str();
			vcsName = vcsListValue.c_str();
			vcs.UpdateAdapter();
		}

		if (!vcs.IsAvailable())
		{
			return true;
		}

		if (vcs.GetPreferences())
		{
			hasChanges |= vcs.GetPreferences()->Serialize(ar); 
		}

		if (ar.isInput() && hasChanges)
		{
			vcs.CheckSettings();
		}

		return true;
	}

	string vcsName;
};

EDITOR_COMMON_API SVersionControlPreferences gVersionControlPreferences;
REGISTER_PREFERENCES_PAGE_PTR(SVersionControlPreferences, &gVersionControlPreferences)

}

bool CVersionControl::IsAvailable()
{
	using namespace Private_VersionControl;
	return gVersionControlPreferences.vcsName != VCS_NONE;
}

bool CVersionControl::UpdateAdapter()
{
	using namespace Private_VersionControl;
	static string currentAdapterClass = VCS_NONE;

	if (currentAdapterClass == gVersionControlPreferences.vcsName)
	{
		return false;
	}

	if (gVersionControlPreferences.vcsName == VCS_NONE)
	{
		// first we destroy current adapter because it will signal onlineChange event which we want to catch
		// before deactivating listeners.
		m_pAdapter = nullptr;
		CVersionControlInitializer::DeactivateListeners();
		m_pCache->Clear();
		currentAdapterClass = gVersionControlPreferences.vcsName;
		m_taskManager.Disable();
	}
	else
	{
		std::vector<IClassDesc*> classes;
		GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_VCS_PROVIDER, classes);
		for (int i = 0; i < classes.size(); i++)
		{
			IClassDesc* pClass = classes[i];
			const auto& className = pClass->ClassName();
			if (className != gVersionControlPreferences.vcsName || className == currentAdapterClass)
				continue;

			IVersionControlAdapter* pVCS = pClass->CreateObject<IVersionControlAdapter>();
			if (pVCS)
			{
				m_pAdapter = std::unique_ptr<IVersionControlAdapter>(pVCS);
				m_pCache->Clear();
				currentAdapterClass = gVersionControlPreferences.vcsName;
				m_taskManager.Reset();
				m_pAdapter->signalOnlineChanged.Connect(this, &CVersionControl::OnAdapterOnlineChanged);
				CVersionControlInitializer::ActivateListeners();
				return true;
			}
		}
	}
	return false;
}

void CVersionControl::OnAdapterOnlineChanged()
{
	if (CryGetCurrentThreadId() == gEnv->mMainThreadId)
	{
		signalOnlineChanged();
		return;
	}
	GetIEditor()->PostOnMainThread([this]()
	{
		signalOnlineChanged();
	});
}

std::shared_ptr<CVersionControlTask> CVersionControl::RemoveFilesLocally(std::vector<string> files, bool isBlocking, Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([files = std::move(files), adapter]()
	{
		return adapter->RemoveFilesLocally(files);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::UpdateFileStatus(std::vector<string> filePaths, std::vector<string> folders, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([filePaths = std::move(filePaths), folders = std::move(folders), adapter]()
	{
		return adapter->UpdateStatus(filePaths, folders);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::UpdateStatus(bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([adapter]()
	{
		return adapter->UpdateStatus();
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::GetLatest(std::vector<string> files, std::vector<string> folders, std::vector<string> fileExtensions, bool force, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	if (files.empty() && folders.empty())
	{
		CRY_ASSERT_MESSAGE(!files.empty() || !folders.empty(), "Get Latest should not be called for empty list.");
		return nullptr;
	}
	return m_taskManager.ScheduleTask([files = std::move(files), folders = std::move(folders), fileExtensions = std::move(fileExtensions), force, adapter]()
	{
		return adapter->GetLatest(files, folders, fileExtensions, force);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::SubmitFiles(std::vector<string> filePaths, const string& message, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	CRY_ASSERT_MESSAGE(!filePaths.empty(), "List of files to publish is empty");
	return m_taskManager.ScheduleTask([filePaths = std::move(filePaths), message, adapter]()
	{
		return adapter->SubmitFiles(filePaths, message);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::ResolveConflicts(std::vector<SVersionControlFileConflictStatus> resolutions, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([resolutions = std::move(resolutions), adapter]()
	{
		return adapter->ResolveConflicts(resolutions);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::AddFiles(std::vector<string> filePaths, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([filePaths = std::move(filePaths), adapter]()
	{
		return adapter->AddFiles(filePaths);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::EditFiles(std::vector<string> filePaths, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([filePaths = std::move(filePaths), adapter]()
	{
		return adapter->EditFiles(filePaths);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::DeleteFiles(std::vector<string> filePaths, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([filePaths = std::move(filePaths), adapter]()
	{
		return adapter->DeleteFiles(filePaths);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::Revert(std::vector<string> files, std::vector<string> folders, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([files = std::move(files), folders = std::move(folders), adapter]()
	{
		return adapter->Revert(files, folders);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::ClearLocalState(std::vector<string> files, std::vector<string> folders, bool clearIfUnchaged, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([files = std::move(files), folders = std::move(folders), clearIfUnchaged, adapter]()
	{
		return adapter->ClearLocalState(files, folders, clearIfUnchaged);
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::CheckSettings(bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([adapter]()
	{
		return adapter->CheckSettings();
	}, isBlocking, std::move(callback));
}

std::shared_ptr<CVersionControlTask> CVersionControl::RetrieveFilesContent(const string& file, bool isBlocking, CVersionControl::Callback callback)
{
	auto adapter = m_pAdapter;
	return m_taskManager.ScheduleTask([file, adapter]()
	{
		return adapter->RetrieveFilesContent(file);
	}, isBlocking, std::move(callback));
}
