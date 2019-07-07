// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QThumbnailView.h"

class CMaterialEditor;
class CMaterial;
class QPreviewWidget;

class CSubMaterialView : public QThumbnailsView
{
public:
	CSubMaterialView(CMaterialEditor* pMatEd);

private:

	void         OnMaterialChanged(CMaterial* pEditorMaterial);
	void         OnMaterialForEditChanged(CMaterial* pEditorMaterial);
	void         OnMaterialPropertiesChanged(CMaterial* pEditorMaterial);
	void         OnSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
	void         OnContextMenu(const QPoint& pos);

	virtual void customEvent(QEvent* event) override;
	virtual void closeEvent(QCloseEvent* pEvent) override;

	class Model;
	QPreviewWidget*        m_previewWidget;
	std::unique_ptr<Model> m_pModel;
	CMaterialEditor*       m_pMatEd;
};
