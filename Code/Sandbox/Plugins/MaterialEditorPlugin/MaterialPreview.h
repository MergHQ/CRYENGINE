// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <QWidget>

class CMaterialEditor;
class CMaterial;
class QPreviewWidget;

class CMaterialPreviewWidget : public QWidget
{
public:
	CMaterialPreviewWidget(CMaterialEditor* pMatEd);

private:

	void OnContextMenu();

	void SetPreviewModel(const char* model);
	void OnPickPreviewModel();

	void OnMaterialChanged(CMaterial* pEditorMaterial);

	virtual void dragEnterEvent(QDragEnterEvent* pEvent) override;
	virtual void dropEvent(QDropEvent* pEvent) override;
	virtual void closeEvent(QCloseEvent* pEvent) override;

	CMaterialEditor* m_pMatEd;
	QPreviewWidget*  m_pPreviewWidget;
};
