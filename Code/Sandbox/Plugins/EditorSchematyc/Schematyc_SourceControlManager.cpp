// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_SourceControlManager.h"

#include <Schematyc/Utils/Schematyc_Assert.h>

#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
namespace CVars
{
static int sc_SourceControlUpdateTimeMs = 2000;
} // CVars

SSourceControlStatus::SSourceControlStatus()
	: attributes(SCC_FILE_ATTRIBUTE_INVALID)
	, haveRevision(-1)
	, headRevision(-1)
{}

bool SSourceControlStatus::operator==(const SSourceControlStatus& rhs) const
{
	return !memcmp(this, &rhs, sizeof(*this));
}

bool SSourceControlStatus::operator!=(const SSourceControlStatus& rhs) const
{
	return !(*this == rhs);
}

CSourceControlManager::SStatusRequest::SStatusRequest(const string& _fileName)
	: fileName(_fileName)
{}

CSourceControlManager::CSourceControlManager()
	: m_prevUpdateTime(0)
{
	using namespace CVars;
	REGISTER_CVAR(sc_SourceControlUpdateTimeMs, sc_SourceControlUpdateTimeMs, VF_INVISIBLE, "Source control update time (ms)");
}

bool CSourceControlManager::Add(const char* szFileName)
{
	SCHEMATYC_EDITOR_ASSERT(szFileName);
	if (szFileName)
	{
		ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
		if (pSourceControl)
		{
			if (pSourceControl->Add(szFileName))
			{
				UpdateStatus(szFileName);
				return true;
			}
		}
	}
	return false;
}

bool CSourceControlManager::GetLatestRevision(const char* szFileName)
{
	SCHEMATYC_EDITOR_ASSERT(szFileName);
	if (szFileName)
	{
		ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
		if (pSourceControl)
		{
			if (pSourceControl->GetLatestVersion(szFileName))
			{
				UpdateStatus(szFileName);
				return true;
			}
		}
	}
	return false;
}

bool CSourceControlManager::CheckOut(const char* szFileName)
{
	SCHEMATYC_EDITOR_ASSERT(szFileName);
	if (szFileName)
	{
		ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
		if (pSourceControl)
		{
			if (pSourceControl->CheckOut(szFileName))
			{
				UpdateStatus(szFileName);
				return true;
			}
		}
	}
	return false;
}

bool CSourceControlManager::Revert(const char* szFileName)
{
	SCHEMATYC_EDITOR_ASSERT(szFileName);
	if (szFileName)
	{
		ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
		if (pSourceControl)
		{
			if (pSourceControl->UndoCheckOut(szFileName))
			{
				UpdateStatus(szFileName);
				return true;
			}
		}
	}
	return false;
}

void CSourceControlManager::DiffAgainstLatestRevision(const char* szFileName)
{
	/*ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
	if (pSourceControl)
	{
		char depotPath[MAX_PATH] = "";
		pSourceControl->GetInternalPath(szFileName, depotPath, MAX_PATH);

		CStackString tmp = szFileName;
		{
			int64 haveRevision = 0;
			int64 headRevision = 0;
			pSourceControl->GetFileRev(depotPath, &haveRevision, &headRevision);

			CStackString postFix;
			postFix.Format(".v%d", headRevision);

			tmp.insert(tmp.rfind('.'), postFix);
		}

		if (pSourceControl->CopyDepotFile(depotPath, tmp.c_str()))
		{
			PluginUtils::DiffAgainstHaveVersion(szFileName, tmp.c_str());

			CrySleep(1000);

			SetFileAttributes(tmp.c_str(), FILE_ATTRIBUTE_NORMAL);
			DeleteFile(tmp.c_str());
		}
	}*/
}

void CSourceControlManager::Monitor(const char* szFileName)
{
	SCHEMATYC_EDITOR_ASSERT(szFileName);
	if (szFileName)
	{
		if (m_statuses.find(szFileName) == m_statuses.end())
		{
			const SSourceControlStatus status = GetStatus(szFileName);

			m_statuses.insert(Statuses::value_type(szFileName, status));

			m_statusUpdateSignal.Send(szFileName, status);

			m_statusRequests.emplace(szFileName);
		}
	}
}

void CSourceControlManager::Update()
{
	const int64 time = gEnv->pSystem->GetITimer()->GetAsyncTime().GetMilliSecondsAsInt64();
	if ((time - m_prevUpdateTime) > CVars::sc_SourceControlUpdateTimeMs)
	{
		if (!m_statusRequests.empty())
		{
			const string fileName = m_statusRequests.front().fileName;

			m_statusRequests.pop();

			if (UpdateStatus(fileName))
			{
				m_statusRequests.emplace(fileName);
			}
		}

		m_prevUpdateTime = time;
	}
}

SourceControlStatusUpdateSignal::Slots& CSourceControlManager::GetStatusUpdateSignalSlots()
{
	return m_statusUpdateSignal.GetSlots();
}

bool CSourceControlManager::UpdateStatus(const char* szFileName)
{
	Statuses::iterator itStatus = m_statuses.find(szFileName);
	if (itStatus != m_statuses.end())
	{
		SSourceControlStatus status = GetStatus(szFileName); // #SchematycTODO : Stop monitoring file if it no longer exists?
		if (status != itStatus->second)
		{
			itStatus->second = status;

			m_statusUpdateSignal.Send(szFileName, status);
		}

		return true;
	}
	return false;
}

SSourceControlStatus CSourceControlManager::GetStatus(const char* szFileName) const
{
	SSourceControlStatus status;

	ISourceControl* pSourceControl = GetIEditor()->GetSourceControl();
	if (pSourceControl)
	{
		status.attributes = pSourceControl->GetFileAttributes(szFileName);

		char depotPath[MAX_PATH] = "";
		pSourceControl->GetInternalPath(szFileName, depotPath, MAX_PATH);
		pSourceControl->GetFileRev(depotPath, &status.haveRevision, &status.headRevision);
	}

	return status;
}
} // Schematyc
