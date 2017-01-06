// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DetailWidget.h"

#include "PreviewWidget.h"

#include <NodeGraph/ICryGraphEditor.h>
#include <QScrollableBox.h>
#include <CrySerialization/IArchive.h>

class QAdvancedPropertyTree;

namespace CrySchematycEditor {

class CComponentItem;
class CAbstractObjectStructureModelItem;
class CAbstractVariablesModelItem;

class CPropertiesWidget : public QScrollableBox
{
	Q_OBJECT

public:
	CPropertiesWidget(CComponentItem& item);
	CPropertiesWidget(CAbstractObjectStructureModelItem& item);
	CPropertiesWidget(CAbstractVariablesModelItem& item);
	CPropertiesWidget(CryGraphEditor::GraphItemSet& items);

	// TEMP
	CPropertiesWidget(IDetailItem& item, Schematyc::CPreviewWidget* pPreview = nullptr);
	// ~TEMP

	~CPropertiesWidget();

	virtual void showEvent(QShowEvent* pEvent) override;

	void OnContentDeleted(CryGraphEditor::CAbstractNodeGraphViewModelItem* deletedItem);

Q_SIGNALS:
	void SignalPropertyChanged();

protected:
	void SetupTree();
	void OnPropertiesChanged();
	void OnPreviewChanged();

protected:
	QAdvancedPropertyTree*       m_pPropertyTree;
	Serialization::SStructs      m_structs;
	Serialization::CContextList* m_pContextList;

	// TEMP
	Schematyc::CPreviewWidget*   m_pPreview;
	std::unique_ptr<IDetailItem> m_pDetailItem;
	//~TEMP
};

}
