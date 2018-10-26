// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/ControlInfo.h"
#include "Common/FileImportInfo.h"
#include <QWidget>
#include <FileDialogs/ExtensionFilter.h>

class QVBoxLayout;

namespace ACE
{
class CMiddlewareDataWidget final : public QWidget
{
	Q_OBJECT

public:

	CMiddlewareDataWidget() = delete;
	CMiddlewareDataWidget(CMiddlewareDataWidget const&) = delete;
	CMiddlewareDataWidget(CMiddlewareDataWidget&&) = delete;
	CMiddlewareDataWidget& operator=(CMiddlewareDataWidget const&) = delete;
	CMiddlewareDataWidget& operator=(CMiddlewareDataWidget&&) = delete;

	explicit CMiddlewareDataWidget(QWidget* const pParent);
	virtual ~CMiddlewareDataWidget() override;

signals:

	void SignalSelectConnectedSystemControl(ControlId const systemControlId, ControlId const implItemId);

private:

	void InitImplDataWidget();
	void ClearImplDataWidget();
	void GetConnectedControls(ControlId const implItemId, SControlInfos& controlInfos);
	void OnImportFiles(
		ExtensionFilterVector const& extensionFilters,
		QStringList const& supportedTypes,
		QString const& targetFolderName,
		bool const isLocalized);
	void OpenFileImporter(FileImportInfos const& fileImportInfos, QString const& targetFolderName, bool const isLocalized);

	QVBoxLayout* const m_pLayout;
	QWidget*           m_pImplDataPanel;
};
} // namespace ACE
