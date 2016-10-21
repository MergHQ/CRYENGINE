// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_NodeGraphRuntimeContext.h"

#include <Schematyc/Script/Schematyc_IScriptGraph.h>

#include <QStringList>

namespace CrySchematycEditor {

QVariant CNodesDictionaryNodeItem::GetIdentifier() const
{
	return QVariant::fromValue(reinterpret_cast<quintptr>(m_pCommand.get()));
}

const CryGraphEditor::CAbstractDictionaryItem* CNodesDictionaryNodeItem::GetParentItem() const
{
	return static_cast<const CryGraphEditor::CAbstractDictionaryItem*>(m_pParent);
}

const CryGraphEditor::CAbstractDictionaryItem* CNodesDictionaryCategoryItem::GetChildItem(int32 index) const
{
	if (index < m_categories.size())
	{
		return static_cast<const CryGraphEditor::CAbstractDictionaryItem*>(m_categories[index]);
	}
	const int32 nodeIndex = index - m_categories.size();
	if (nodeIndex < m_nodes.size())
	{
		return static_cast<const CryGraphEditor::CAbstractDictionaryItem*>(m_nodes[nodeIndex]);
	}
	return nullptr;
}

const CryGraphEditor::CAbstractDictionaryItem* CNodesDictionaryCategoryItem::GetParentItem() const
{
	return static_cast<const CryGraphEditor::CAbstractDictionaryItem*>(m_pParent);
}

CNodesDictionary::CNodesDictionary()
{

}

CNodesDictionary::~CNodesDictionary()
{

}

const CryGraphEditor::CAbstractDictionaryItem* CNodesDictionary::GetItem(int32 index) const
{
	if (index < m_categories.size())
	{
		return static_cast<const CryGraphEditor::CAbstractDictionaryItem*>(m_categories[index]);
	}
	const int32 nodeIndex = index - m_categories.size();
	if (nodeIndex < m_nodes.size())
	{
		return static_cast<const CryGraphEditor::CAbstractDictionaryItem*>(m_nodes[nodeIndex]);
	}
	return nullptr;
}

class CNodesDictionaryCreator : public Schematyc::IScriptGraphNodeCreationMenu
{
public:
	CNodesDictionaryCreator(CNodesDictionary& dictionary)
		: m_dictionary(dictionary)
	{}

	virtual bool AddOption(const char* szLabel, const char* szDescription, const char* szWikiLink, const Schematyc::IScriptGraphNodeCreationMenuCommandPtr& pCommand)
	{
		QStringList categories = QString(szLabel).split("::");
		const QString name = categories.back();
		categories.removeLast();

		if (pCommand)
		{
			m_ppCommand = &pCommand;
			AddRecursive(name, categories, nullptr);
			return true;
		}

		return false;
	}

	void AddRecursive(const QString& name, QStringList& categories, CNodesDictionaryCategoryItem* pParentCategory)
	{
		if (categories.size() > 0)
		{
			std::vector<CNodesDictionaryCategoryItem*>* pCategoryItems = pParentCategory ? &pParentCategory->m_categories : &m_dictionary.m_categories;

			for (CNodesDictionaryCategoryItem* pCategoryItem : * pCategoryItems)
			{
				const QString* pName = pCategoryItem->GetName();
				if (pName && *pName == categories.front())
				{
					categories.removeFirst();
					AddRecursive(name, categories, pCategoryItem);
					return;
				}
			}

			CNodesDictionaryCategoryItem* pNewCategory = nullptr;
			for (const QString& categoryName : categories)
			{
				pNewCategory = new CNodesDictionaryCategoryItem(categoryName, pParentCategory);
				pCategoryItems->push_back(pNewCategory);
				pCategoryItems = &pNewCategory->m_categories;
				pParentCategory = pNewCategory;
			}

			if (pNewCategory)
			{
				CNodesDictionaryNodeItem* pNode = new CNodesDictionaryNodeItem(name, *m_ppCommand, pNewCategory);
				pNewCategory->m_nodes.push_back(pNode);
			}
		}
		else
		{
			CNodesDictionaryNodeItem* pNewNodeItem = new CNodesDictionaryNodeItem(name, *m_ppCommand, pParentCategory);
			if (pParentCategory)
			{
				pParentCategory->m_nodes.push_back(pNewNodeItem);
			}
			else
			{
				m_dictionary.m_nodes.push_back(pNewNodeItem);
			}
		}
	}

private:
	CNodesDictionary& m_dictionary;

	const Schematyc::IScriptGraphNodeCreationMenuCommandPtr* m_ppCommand;
};

void CNodesDictionary::LoadLoadsFromScriptGraph(Schematyc::IScriptGraph& scriptGraph)
{
	CNodesDictionaryCreator creator(*this);
	scriptGraph.PopulateNodeCreationMenu(creator);
}

CNodeGraphRuntimeContext::CNodeGraphRuntimeContext(Schematyc::IScriptGraph& scriptGraph)
{
	m_nodesDictionary.LoadLoadsFromScriptGraph(scriptGraph);
}

}
