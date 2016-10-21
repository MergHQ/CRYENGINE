// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_PropertiesWidget.h"

#include "Schematyc_ComponentsModel.h"
#include "Schematyc_AbstractObjectModel.h"
#include "Schematyc_VariablesModel.h"

#include "Schematyc_ScriptBrowserUtils.h"

#include <NodeGraph/AbstractNodeGraphViewModelItem.h>

#include <QAdvancedPropertyTree.h>
#include <QPropertyTree/ContextList.h>

// TEMP
#include "Schematyc_DetailWidget.h"
// ~TEMP

namespace CrySchematycEditor {

CPropertiesWidget::CPropertiesWidget(CComponentItem& item)
{
	SetupTree();

	m_structs.push_back(Serialization::SStruct(item));
	m_pPropertyTree->attach(m_structs);

	m_pContextList = new Serialization::CContextList();
	m_pPropertyTree->setArchiveContext(m_pContextList->Tail());

	addWidget(m_pPropertyTree);
}

CPropertiesWidget::CPropertiesWidget(CAbstractObjectStructureModelItem& item)
{
	SetupTree();

	m_structs.push_back(Serialization::SStruct(item));
	m_pPropertyTree->attach(m_structs);

	m_pContextList = new Serialization::CContextList();
	m_pPropertyTree->setArchiveContext(m_pContextList->Tail());

	addWidget(m_pPropertyTree);
}

CPropertiesWidget::CPropertiesWidget(CAbstractVariablesModelItem& item)
{
	SetupTree();

	m_structs.push_back(Serialization::SStruct(item));
	m_pPropertyTree->attach(m_structs);

	m_pContextList = new Serialization::CContextList();
	m_pPropertyTree->setArchiveContext(m_pContextList->Tail());

	addWidget(m_pPropertyTree);
}

CPropertiesWidget::CPropertiesWidget(CryGraphEditor::GraphItemSet& items)
{
	SetupTree();

	for (CryGraphEditor::CAbstractNodeGraphViewModelItem* pAbstractItem : items)
	{
		m_structs.push_back(Serialization::SStruct(*pAbstractItem));
	}
	m_pPropertyTree->attach(m_structs);

	m_pContextList = new Serialization::CContextList();
	m_pPropertyTree->setArchiveContext(m_pContextList->Tail());

	addWidget(m_pPropertyTree);
}

CPropertiesWidget::CPropertiesWidget(IDetailItem& item)
	: m_pDetailItem(&item)
	, m_pContextList(nullptr)
{
	SetupTree();

	m_structs.push_back(Serialization::SStruct(item));
	m_pPropertyTree->attach(m_structs);

	Serialization::CContextList* pContextList = item.GetContextList();
	if (pContextList)
	{
		m_pPropertyTree->setArchiveContext(pContextList->Tail());
	}

	addWidget(m_pPropertyTree);

	QObject::connect(m_pPropertyTree, &QAdvancedPropertyTree::signalChanged, this, &CPropertiesWidget::OnPropertiesChanged);
}

CPropertiesWidget::~CPropertiesWidget()
{
	delete m_pContextList;
}

void CPropertiesWidget::SetupTree()
{
	m_pPropertyTree = new QAdvancedPropertyTree("Component Properties");
	m_pPropertyTree->setExpandLevels(2);
	m_pPropertyTree->setValueColumnWidth(0.6f);
	m_pPropertyTree->setAggregateMouseEvents(false);
	m_pPropertyTree->setFullRowContainers(true);
}

void CPropertiesWidget::OnPropertiesChanged()
{
	SignalPropertyChanged(m_pDetailItem.get());
}

void CPropertiesWidget::showEvent(QShowEvent* pEvent)
{
	QScrollableBox::showEvent(pEvent);

	if (m_pPropertyTree)
		m_pPropertyTree->setSizeToContent(true);
}

}
