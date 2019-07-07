// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlSubmissionPopup.h"

#include "VersionControlPendingChangesWidget.h"
#include "PendingChange.h"
#include "VersionControl/VersionControl.h"
#include "PathUtils.h"
#include "QtUtil.h"
#include <QBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>

namespace Private_VersionControlSubmissionPopup
{

static QString g_description;

}

void CVersionControlSubmissionPopup::ShowPopup()
{
	CVersionControlSubmissionPopup popup;
	popup.Execute();
}

void CVersionControlSubmissionPopup::ShowPopup(const std::vector<CAsset*>& assets, const std::vector<string>& folders)
{
	CVersionControlSubmissionPopup popup;
	popup.Select(assets, folders);
	popup.Execute();
}

void CVersionControlSubmissionPopup::ShowPopup(const std::vector<string>& mainFiles, const std::vector<string>& folders)
{
	CVersionControlSubmissionPopup popup;
	popup.Select(mainFiles, folders);
	popup.Execute();
}

CVersionControlSubmissionPopup::CVersionControlSubmissionPopup(QWidget* pParent /*= nullptr*/)
	: CEditorDialog(QStringLiteral("CVersionControlSubmissionPopup"), pParent)
{
	SetTitle(tr("Submit"));
	QVBoxLayout* pLayout = new QVBoxLayout();
	pLayout->setMargin(3);

	m_pPendingChangesWidget = new CVersionControlPendingChangesWidget(this);
	pLayout->addWidget(m_pPendingChangesWidget);

	m_textEdit = new QPlainTextEdit(this);
	m_textEdit->setPlaceholderText(tr("Enter submit description"));
	m_textEdit->setBackgroundVisible(true);
	m_textEdit->setFixedHeight(60);
	m_textEdit->setPlainText(Private_VersionControlSubmissionPopup::g_description);
	pLayout->addWidget(m_textEdit);

	auto pCancelButton = new QPushButton(tr("Cancel"));
	auto pSubmitButton = new QPushButton(tr("Submit"));
	QObject::connect(pCancelButton, &QPushButton::clicked, this, [this] { OnCancelSubmit(); });
	QObject::connect(pSubmitButton, &QPushButton::clicked, this, [this] { OnSubmit(); });
	pCancelButton->setFixedWidth(120);
	pSubmitButton->setFixedWidth(190);

	auto pButtonLayout = new QHBoxLayout();
	pButtonLayout->setMargin(0);
	pButtonLayout->addStretch();
	pButtonLayout->addWidget(pCancelButton);
	pButtonLayout->addWidget(pSubmitButton);
	pLayout->addLayout(pButtonLayout);

	setLayout(pLayout);
}

CVersionControlSubmissionPopup::~CVersionControlSubmissionPopup()
{
	Private_VersionControlSubmissionPopup::g_description = m_textEdit->toPlainText();
}

void CVersionControlSubmissionPopup::Select(const std::vector<CAsset*>& assets, const std::vector<string>& folders)
{
	bool shouldDeselectCurrent = true;
	if (!assets.empty())
	{
		m_pPendingChangesWidget->SelectAssets(assets, shouldDeselectCurrent);
		shouldDeselectCurrent = false;
	}
	if (!folders.empty())
	{
		m_pPendingChangesWidget->SelectFolders(folders, shouldDeselectCurrent);
	}
}

void CVersionControlSubmissionPopup::Select(const std::vector<string>& mainFiles, const std::vector<string>& folders)
{
	bool shouldDeselectCurrent = true;
	if (!mainFiles.empty())
	{
		m_pPendingChangesWidget->SelectFiles(mainFiles, shouldDeselectCurrent);
		shouldDeselectCurrent = false;
	}
	if (!folders.empty())
	{
		m_pPendingChangesWidget->SelectFolders(folders, shouldDeselectCurrent);
	}
}

void CVersionControlSubmissionPopup::OnSubmit()
{
	if (m_pPendingChangesWidget->GetSelectedPendingChanges().empty())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "No changes selected for submission");
		return;
	}
	if (m_textEdit->toPlainText().isEmpty())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Submit description can not be empty");
		return;
	}

	using namespace Private_VersionControlSubmissionPopup;
	const auto& changes = m_pPendingChangesWidget->GetSelectedPendingChanges();
	std::vector<string> files;
	files.reserve(changes.size() * 3);
	for (const auto& pChange : changes)
	{
		if (!pChange->IsValid())
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Pending change %s is not in valid state. The problem might be because file %s is not on the files system.", QtUtil::ToString(pChange->GetName()), pChange->GetMainFile());
			return;
		}
		std::vector<string> changeFiles = pChange->GetFiles();
		std::move(changeFiles.begin(), changeFiles.end(), std::back_inserter(files));
	}

	CVersionControl::GetInstance().SubmitFiles(files, QtUtil::ToString(m_textEdit->toPlainText()), false
		, [this](const auto& result)
	{
		if (result.IsSuccess())
		{
			g_description = "";
		}

	});
	close();
}

void CVersionControlSubmissionPopup::OnCancelSubmit()
{
	close();
}
