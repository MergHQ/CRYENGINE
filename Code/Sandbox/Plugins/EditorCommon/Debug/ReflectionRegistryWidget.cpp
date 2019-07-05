// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>

#include "ReflectionRegistryWidget.h"
#include "QAdvancedTreeView.h"
#include "QSearchBox.h"
#include "CryIcon.h"

#include <QWidget>
#include <QVariant>
#include <QHeaderView>
#include <QFilteringPanel.h>
#include <QAbstractItemModel>
#include <QMenuBar>
#include <QVBoxLayout>

#include <CryUtils/Debug.h>
#include <CryString/StringUtils.h>
#include <CryReflection/IModule.h>
#include <CryReflection/IRegistry.h>
#include <CryReflection/ITypeDesc.h>

REGISTER_VIEWPANE_FACTORY_AND_MENU(CReflectionRegistryDockable, "Reflection Registry", "Debug", true, "Advanced");

const CItemModelAttribute CReflectionRegistryModel::s_columnAttributes[] =
{
	CItemModelAttribute("TypeIndex",         &Attributes::s_intAttributeType,    CItemModelAttribute::Visible, true),
	CItemModelAttribute("Size",              &Attributes::s_intAttributeType,    CItemModelAttribute::Visible, true),
	CItemModelAttribute("Name",              &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true),
	CItemModelAttribute("Description",       &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true),
	CItemModelAttribute("Guid",              &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true),
	CItemModelAttribute("FullQualifiedName", &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true),
	CItemModelAttribute("Registered",        &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true),
	CItemModelAttribute("Source",            &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, true),
};
const int CReflectionRegistryModel::s_attributeCount = CRY_ARRAY_COUNT(CReflectionRegistryModel::s_columnAttributes);

CReflectionRegistryModel::CReflectionRegistryModel(QObject* pParent)
	: QAbstractItemModel(pParent)
{
	GenerateDataCache();
}

CReflectionRegistryModel::~CReflectionRegistryModel()
{
}

void CReflectionRegistryModel::GenerateDataCache()
{
	const size_t registryMaxCount = Cry::Reflection::CoreRegistry::GetRegistriesCount();
	const Cry::Reflection::TypeIndex::ValueType typeCount = Cry::Reflection::CoreRegistry::GetTypeCount();
	for (Cry::Reflection::TypeIndex::ValueType i = 0; i < typeCount; ++i)
	{
		const Cry::Reflection::ITypeDesc* pTypeDesc = Cry::Reflection::CoreRegistry::GetTypeByIndex(i);
		if (pTypeDesc)
		{
			stack_string registryNames(gEnv->pReflection->GetName());
			for (size_t j = 0; j < registryMaxCount; ++j)
			{
				Cry::Reflection::IRegistry* pRegistry = Cry::Reflection::CoreRegistry::GetRegistryByIndex(j);
				if (pRegistry)
				{
					if (pRegistry->GetTypeById(pTypeDesc->GetTypeId()))
					{
						registryNames.append(", ");
						registryNames.append(pRegistry->GetLabel());
					}
				}
			}
			m_typeRegistries.emplace(pTypeDesc->GetTypeId(), registryNames);
		}
	}
}

QVariant CReflectionRegistryModel::data(const QModelIndex& index, int role) const
{
	const Cry::Reflection::ITypeDesc* pTypeDesc = Cry::Reflection::CoreRegistry::GetTypeByIndex(index.row());
	if (pTypeDesc)
	{
		switch (role)
		{
		case Qt::DisplayRole:
		case Qt::ToolTipRole: // Intended fall through.
			switch (index.column())
			{
			case eColumn_TypeIndex:
				return (int)pTypeDesc->GetIndex();
			case eColumn_Size:
				return pTypeDesc->GetSize();
			case eColumn_Label:
				return pTypeDesc->GetLabel();
			case eColumn_Description:
				return pTypeDesc->GetDescription();
			case eColumn_FullQualifiedName:
				return pTypeDesc->GetFullQualifiedName();
			case eColumn_Guid:
				return pTypeDesc->GetGuid().ToDebugString();
			case eColumn_Registered:
				{
					// Retrieve registries string from this type.
					std::map<Cry::Type::CTypeId, string>::const_iterator it = m_typeRegistries.find(pTypeDesc->GetTypeId());
					if (it != m_typeRegistries.end())
					{
						return it->second.c_str();
					}
				}
			case eColumn_Source:
				return pTypeDesc->GetSourceInfo().GetFile();
			}
		case Qt::DecorationRole:
			if (index.column() == eColumn_Source)
			{
				return CryIcon("icons:General/link-external.ico");
			}
		}
	}

	return QVariant();
}

QVariant CReflectionRegistryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if ((orientation == Qt::Horizontal) && (section < CReflectionRegistryModel::s_attributeCount))
	{
		const CItemModelAttribute* pAttribute = &CReflectionRegistryModel::s_columnAttributes[section];
		switch (role)
		{
		case Qt::DisplayRole:
			return pAttribute->GetName();
		case Attributes::s_getAttributeRole:
			return QVariant::fromValue(const_cast<CItemModelAttribute*>(pAttribute));
		}
	}
	CRY_ASSERT(section < CReflectionRegistryModel::s_attributeCount);

	return QVariant();
}

int CReflectionRegistryModel::rowCount(const QModelIndex& parent) const
{
	return !parent.isValid() ? Cry::Reflection::CoreRegistry::GetTypeCount() : 0;
}

CReflectionRegistryWidget::CReflectionRegistryWidget(QWidget* pParent)
	: QWidget(pParent)
	, m_pModel(new CReflectionRegistryModel())
{
	m_pFilter = new QAttributeFilterProxyModel(QAttributeFilterProxyModel::BaseBehavior);
	m_pFilter->setSourceModel(m_pModel);
	m_pFilter->setFilterKeyColumn(-1);

	m_pView = new QAdvancedTreeView(QAdvancedTreeView::UseItemModelAttribute);
	m_pView->setModel(m_pFilter);
	m_pView->setSortingEnabled(true);
	m_pView->setUniformRowHeights(true);
	m_pView->header()->setStretchLastSection(true);
	m_pView->header()->setMinimumSectionSize(10);

	m_pView->sortByColumn(CReflectionRegistryModel::eColumn_Label, Qt::AscendingOrder);
	m_pView->setSelectionMode(QAbstractItemView::NoSelection);

	m_pFilteringPanel = new QFilteringPanel("ReflectionRegistryWidget", m_pFilter);
	m_pFilteringPanel->SetContent(m_pView);
	m_pFilteringPanel->GetSearchBox()->EnableContinuousSearch(true);

	const auto pLayout = new QVBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(m_pFilteringPanel);
	setLayout(pLayout);

	QObject::connect(m_pView, &QAbstractItemView::clicked, [=](const QModelIndex& index)
	{
		QModelIndex mappedIndex = m_pFilter->mapToSource(index);
		if (mappedIndex.isValid() && (mappedIndex.column() == CReflectionRegistryModel::eColumn_Source))
		{
		  const Cry::Reflection::ITypeDesc* pTypeDesc = Cry::Reflection::CoreRegistry::GetTypeByIndex(mappedIndex.row());
		  if (pTypeDesc)
		  {
		    Cry::Utils::OpenSourceFile(pTypeDesc->GetSourceInfo().GetFile());
			}
		}
	});
}

CReflectionRegistryWidget::~CReflectionRegistryWidget()
{
	delete m_pModel;
}

QAdvancedTreeView* CReflectionRegistryWidget::GetView() const
{
	return m_pView;
}

CReflectionRegistryDockable::CReflectionRegistryDockable(QWidget* const pParent)
	: CDockableEditor(pParent)
{
	const auto pMenuBar = new QMenuBar();
	const auto pFileMenu = pMenuBar->addMenu("&File");
	pFileMenu->addAction("&Export...");
	const auto pViewMenu = pMenuBar->addMenu("&View");
	pViewMenu->addAction("&Source");

	m_pReflectionRegistryWidget = new CReflectionRegistryWidget(this);

	const auto pLayout = new QVBoxLayout();
	pLayout->setContentsMargins(1, 1, 1, 1);
	pLayout->setMenuBar(pMenuBar);
	pLayout->addWidget(m_pReflectionRegistryWidget);
	SetContent(pLayout);
}

CReflectionRegistryDockable::~CReflectionRegistryDockable()
{
}

QVariantMap CReflectionRegistryDockable::GetLayout() const
{
	QVariantMap state = CDockableEditor::GetLayout();
	state.insert("view", m_pReflectionRegistryWidget->GetView()->GetState());
	return state;
}

void CReflectionRegistryDockable::SetLayout(const QVariantMap& state)
{
	CDockableEditor::SetLayout(state);

	QVariant viewState = state.value("view");
	if (viewState.isValid() && viewState.type() == QVariant::Map)
	{
		m_pReflectionRegistryWidget->GetView()->SetState(viewState.value<QVariantMap>());
	}
}
