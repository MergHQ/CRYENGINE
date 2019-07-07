// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QWidget"
#include "EditorCommonAPI.h"
#include <Controls/EditorDialog.h>
#include <AssetSystem/Asset.h>

class QPlainTextEdit;
class CVersionControlPendingChangesWidget;

class EDITOR_COMMON_API CVersionControlSubmissionPopup : public CEditorDialog
{
	Q_OBJECT
public:
	static void ShowPopup();

	static void ShowPopup(const std::vector<CAsset*>& assets, const std::vector<string>& folders = {});

	static void ShowPopup(const std::vector<string>& mainFiles, const std::vector<string>& folders = {});

	CVersionControlSubmissionPopup(QWidget* pParent = nullptr);

	~CVersionControlSubmissionPopup();

	//! Marks given assets as selected if they can be submitted.
	void Select(const std::vector<CAsset*>& assets, const std::vector<string>& folders);

	void Select(const std::vector<string>& mainFiles, const std::vector<string>& folders);

private:
	//! Triggered when "Submit" button is pressed.
	void OnSubmit();
	//! Triggered when submission is canceled.
	void OnCancelSubmit();

	CVersionControlPendingChangesWidget* m_pPendingChangesWidget{ nullptr };
	QPlainTextEdit*                      m_textEdit{ nullptr };
};
