// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISourceControl.h>
#include <Schematyc/Utils/Schematyc_Signal.h>

namespace Schematyc
{
struct SSourceControlStatus
{
	SSourceControlStatus();

	bool operator==(const SSourceControlStatus& rhs) const;
	bool operator!=(const SSourceControlStatus& rhs) const;

	uint32 attributes;
	int64  haveRevision;
	int64  headRevision;
};

typedef CSignal<void, const char*, const SSourceControlStatus&> SourceControlStatusUpdateSignal;

class CSourceControlManager
{
public:

	typedef std::map<string, SSourceControlStatus> Statuses;

	struct SStatusRequest
	{
		SStatusRequest(const string& _fileName);

		string fileName;
	};

	typedef std::queue<SStatusRequest> StatusRequests;

public:

	CSourceControlManager();

	void                                    Monitor(const char* szFileName);
	bool                                    Add(const char* szFileName);
	bool                                    GetLatestRevision(const char* szFileName);
	bool                                    CheckOut(const char* szFileName);
	bool                                    Revert(const char* szFileName);
	void                                    DiffAgainstLatestRevision(const char* szFileName);

	void                                    Update();

	SourceControlStatusUpdateSignal::Slots& GetStatusUpdateSignalSlots();

private:

	bool                 UpdateStatus(const char* szFileName);
	SSourceControlStatus GetStatus(const char* szFileName) const;

private:

	Statuses                        m_statuses;
	StatusRequests                  m_statusRequests;

	int64                           m_prevUpdateTime;

	SourceControlStatusUpdateSignal m_statusUpdateSignal;
};
} // Schematyc
