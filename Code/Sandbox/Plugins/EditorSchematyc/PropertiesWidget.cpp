// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PropertiesWidget.h"

#include "ComponentsModel.h"
#include "AbstractObjectModel.h"
#include "VariablesModel.h"

#include "ScriptBrowserUtils.h"

#include <NodeGraph/AbstractNodeGraphViewModelItem.h>

#include <QAdvancedPropertyTree.h>
#include <QPropertyTree/ContextList.h>

// TEMP
#include "DetailWidget.h"
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

	PropertyTreeStyle treeStyle(QPropertyTree::defaultTreeStyle());
	treeStyle.propertySplitter = false;
	m_pPropertyTree->setTreeStyle(treeStyle);

	QObject::connect(m_pPropertyTree, &QAdvancedPropertyTree::signalChanged, this, &CPropertiesWidget::OnPropertiesChanged);
}

void CPropertiesWidget::OnPropertiesChanged()
{
	SignalPropertyChanged();

	if (m_pPropertyTree)
	{
		m_pPropertyTree->revertNoninterrupting();
	}
}

void CPropertiesWidget::showEvent(QShowEvent* pEvent)
{
	QScrollableBox::showEvent(pEvent);

	if (m_pPropertyTree)
		m_pPropertyTree->setSizeToContent(true);
}

}
