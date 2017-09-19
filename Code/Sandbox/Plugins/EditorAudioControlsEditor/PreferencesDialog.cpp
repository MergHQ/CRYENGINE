// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "PreferencesDialog.h"
#include "AudioControlsEditorPlugin.h"
#include "IAudioSystemEditor.h"
#include <QtUtil.h>

// Qt
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include "FileDialogs/SystemFileDialog.h"

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CPreferencesDialog::CPreferencesDialog(QWidget* pParent)
	: CEditorDialog("AudioSystemPreferencesDialog")
{
	IAudioSystemEditor* pAudioSystem = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	if (pAudioSystem)
	{
		IImplementationSettings* pSettings = pAudioSystem->GetSettings();

		if (pSettings)
		{
			setWindowTitle(tr("Audio System Preferences"));

			QVBoxLayout* pMainLayout = new QVBoxLayout(this);
			setLayout(pMainLayout);

			QGridLayout* pLayout = new QGridLayout(this);

			pLayout->addWidget(new QLabel(tr("Audio Middleware") + ":"), 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
			pLayout->addWidget(new QLabel(QtUtil::ToQString(pAudioSystem->GetName())), 0, 1, Qt::AlignLeft | Qt::AlignVCenter);

			pLayout->addWidget(new QLabel(tr("Sound Banks Path") + ":"), 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
			pLayout->addWidget(new QLabel(pSettings->GetSoundBanksPath()), 1, 1);

			pLayout->addWidget(new QLabel(tr("Project Path") + ":"), 2, 0, Qt::AlignLeft | Qt::AlignVCenter);

			QHBoxLayout* pProjectPathLayout = new QHBoxLayout(this);
			m_projectPath = pSettings->GetProjectPath();
			QLineEdit* pLineEdit = new QLineEdit(m_projectPath);
			pLineEdit->setMinimumWidth(250);
			QObject::connect(pLineEdit, &QLineEdit::textChanged, [&](QString const& projectPath)
			{
				EnableSaveButton(m_projectPath.toLower().replace("/", R"(\)") != projectPath.toLower().replace("/", R"(\)"));
			});

			pProjectPathLayout->addWidget(pLineEdit);

			QToolButton* pBrowseButton = new QToolButton();
			pBrowseButton->setText("...");
			QObject::connect(pBrowseButton, &QToolButton::clicked, [=]()
				{
					IAudioSystemEditor* pAudioSystem = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

					if (pAudioSystem)
					{
						CSystemFileDialog::RunParams runParams;
						runParams.initialDir = pLineEdit->text();
						QString path = CSystemFileDialog::RunSelectDirectory(runParams, this);

						if (!path.isEmpty())
						{
							pLineEdit->setText(path);
						}
					}
				});
			pProjectPathLayout->addWidget(pBrowseButton);

			pLayout->addLayout(pProjectPathLayout, 2, 1);

			pMainLayout->addLayout(pLayout);

			QDialogButtonBox* pButtons = new QDialogButtonBox(this);
			pButtons->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
			pButtons->button(QDialogButtonBox::Save)->setEnabled(false);
			QObject::connect(this, &CPreferencesDialog::EnableSaveButton, pButtons->button(QDialogButtonBox::Save), &QPushButton::setEnabled);
			QObject::connect(pButtons, &QDialogButtonBox::accepted, [=]()
				{
					IAudioSystemEditor* pAudioSystem = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
					if (pAudioSystem)
					{
						IImplementationSettings* pSettings = pAudioSystem->GetSettings();

						if (pSettings)
						{
							pSettings->SetProjectPath(QtUtil::ToString(pLineEdit->text()));
							ImplementationSettingsAboutToChange();
							// clear all connections to the middleware since we are reloading everything
							CAudioControlsEditorPlugin::GetAssetsManager()->ClearAllConnections();
							pAudioSystem->Reload();
							CAudioControlsEditorPlugin::GetAssetsManager()->ReloadAllConnections();
							ImplementationSettingsChanged();
						}
					}
					accept();
				});

			QObject::connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
			pMainLayout->addWidget(pButtons, 0);

			adjustSize();
			SetResizable(false);
		}
	}
}
} // namespace ACE
