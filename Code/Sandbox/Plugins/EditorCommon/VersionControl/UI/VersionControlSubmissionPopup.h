// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QWidget"

class QPlainTextEdit;
class CVersionControlPendingChangesWidget;

class EDITOR_COMMON_API CVersionControlSubmissionPopup : public CEditorDialog
{
	Q_OBJECT
public:
	static void ShowPopup(const std::vector<CAsset*>& assets = {}, const std::vector<string>& layersFiles = {}, const std::vector<string>& folders = {});

	CVersionControlSubmissionPopup(QWidget* pParent = nullptr);

	~CVersionControlSubmissionPopup();

	//! Marks given assets as selected if they can be submitted.
	void Select(const std::vector<CAsset*>& assets, const std::vector<string>& layersFiles, const std::vector<string>& folders);

private:
	//! Triggered when "Submit" button is pressed.
	void OnSubmit();
	//! Triggered when submission is canceled.
	void OnCancelSubmit();

	CVersionControlPendingChangesWidget* m_pPendingChangesWidget{ nullptr };
	QPlainTextEdit*                      m_textEdit{ nullptr };
};
