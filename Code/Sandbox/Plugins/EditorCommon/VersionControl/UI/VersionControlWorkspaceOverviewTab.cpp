// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlWorkspaceOverviewTab.h"
#include "VersionControlPendingChangesWidget.h"
#include "VersionControlSubmissionPopup.h"
#include "PendingChange.h"
#include "VersionControl/VersionControl.h"
#include "VersionControl/AssetsVCSReverter.h"
#include "AssetSystem/IFilesGroupController.h"
#include "QtUtil.h"
#include <CrySystem/ISystem.h>
#include <QPushButton>
#include <QVBoxLayout>

namespace Private_VersionControlWorkspaceOverviewTab
{

class CPendingChangeFilesGroup : public IFilesGroupController
{
public:
	CPendingChangeFilesGroup(CPendingChange* pPendingChange)
	{
		SetData(pPendingChange);
	}

	virtual std::vector<string> GetFiles(bool includeGeneratedFile = true) const { return m_files; }

	virtual const string& GetMainFile() const { return m_mainFile; }

	virtual const string& GetName() const { return m_mainFile; } 

	void SetData(CPendingChange* pPendingChange)
	{
		m_mainFile = pPendingChange->GetMainFile();
		m_files = pPendingChange->GetFiles();
	}

private:
	string              m_mainFile;
	std::vector<string> m_files;
};

}

CVersionControlWorkspaceOverviewTab::CVersionControlWorkspaceOverviewTab(QWidget* pParent /*= nullptr*/)
	: QWidget(pParent)
{
	QVBoxLayout* pLayout = new QVBoxLayout();
	pLayout->setMargin(6);

	m_pPendingChangesWidget = new CVersionControlPendingChangesWidget(this);
	pLayout->addWidget(m_pPendingChangesWidget);

	m_pPendingChangesWidget->signalSelectionChanged.Connect([this]()
	{
		UpdateButtonsStates();
	});

	CVersionControl::GetInstance().GetUpdateSignal().Connect(this, &CVersionControlWorkspaceOverviewTab::UpdateButtonsStates);

	m_pRevertButton = new QPushButton(tr("Revert..."), this);
	m_pSubmitButton = new QPushButton(tr("Submit..."), this);
	QObject::connect(m_pRevertButton, &QPushButton::clicked, this, [this] { OnRevert(); });
	QObject::connect(m_pSubmitButton, &QPushButton::clicked, this, [this] { OnSubmit(); });
	m_pRevertButton->setObjectName("revert");
	m_pSubmitButton->setObjectName("submit");
	m_pRevertButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
	m_pSubmitButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	auto pButtonLayout = new QHBoxLayout();
	pButtonLayout->setMargin(0);
	pButtonLayout->setAlignment(Qt::AlignRight);
	pButtonLayout->addWidget(m_pRevertButton);
	pButtonLayout->addWidget(m_pSubmitButton);
	pLayout->addLayout(pButtonLayout);

	setLayout(pLayout);

	UpdateButtonsStates();
}

CVersionControlWorkspaceOverviewTab::~CVersionControlWorkspaceOverviewTab()
{
	CVersionControl::GetInstance().GetUpdateSignal().DisconnectObject(this);
}

void CVersionControlWorkspaceOverviewTab::OnSubmit()
{
	CVersionControlSubmissionPopup::ShowPopup();
}

void CVersionControlWorkspaceOverviewTab::OnRevert()
{
	using namespace Private_VersionControlWorkspaceOverviewTab;
	const auto& pendingChanges = m_pPendingChangesWidget->GetSelectedPendingChanges();
	if (pendingChanges.empty())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "No changes selected for reversion");
		return;
	}

	for (const auto& pChange : pendingChanges)
	{
		if (!pChange->IsValid())
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Pending change %s is not in valid state. The problem might be because file %s is not on the files system.", QtUtil::ToString(pChange->GetName()), pChange->GetMainFile());
			return;
		}
	}

	std::vector<std::shared_ptr<IFilesGroupController>> fileGroups;
	fileGroups.reserve(pendingChanges.size());
	std::transform(pendingChanges.cbegin(), pendingChanges.cend(), std::back_inserter(fileGroups), [](CPendingChange* pPendingChange)
	{
		return std::make_shared<CPendingChangeFilesGroup>(pPendingChange);
	});

	CAssetsVCSReverter::Revert(std::move(fileGroups));
}

void CVersionControlWorkspaceOverviewTab::UpdateButtonsStates()
{
	const bool shouldEnable = !m_pPendingChangesWidget->GetSelectedPendingChanges().empty();
	m_pRevertButton->setEnabled(shouldEnable);
	m_pSubmitButton->setEnabled(shouldEnable);
}
