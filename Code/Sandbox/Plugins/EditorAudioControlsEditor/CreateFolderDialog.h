// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Controls/EditorDialog.h>

namespace ACE
{
class CCreateFolderDialog final : public CEditorDialog
{
	Q_OBJECT

public:

	CCreateFolderDialog() = delete;
	CCreateFolderDialog(CCreateFolderDialog const&) = delete;
	CCreateFolderDialog(CCreateFolderDialog&&) = delete;
	CCreateFolderDialog& operator=(CCreateFolderDialog const&) = delete;
	CCreateFolderDialog& operator=(CCreateFolderDialog&&) = delete;

	explicit CCreateFolderDialog(QWidget* pParent);
	virtual ~CCreateFolderDialog() override = default;

signals:

	void SignalSetFolderName(QString const& folderName);

private:

	void OnAccept();

	QString m_folderName;
};
} // namespace ACE
