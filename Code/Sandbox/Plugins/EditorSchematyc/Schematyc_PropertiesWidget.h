// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc_DetailWidget.h"

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
	CPropertiesWidget(IDetailItem& item);
	// ~TEMP

	~CPropertiesWidget();

	virtual void showEvent(QShowEvent* pEvent) override;

Q_SIGNALS:
	void SignalPropertyChanged(IDetailItem* pDetailItem);

protected:
	void SetupTree();
	void OnPropertiesChanged();

protected:
	QAdvancedPropertyTree*       m_pPropertyTree;
	Serialization::SStructs      m_structs;
	Serialization::CContextList* m_pContextList;

	// TEMP
	std::unique_ptr<IDetailItem> m_pDetailItem;
	//~TEMP
};

}
