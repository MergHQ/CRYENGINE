// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/ICryGraphEditor.h>
#include <NodeGraph/AbstractDictionary.h>

#include <Schematyc/Script/Schematyc_IScriptGraph.h>

namespace Schematyc {

struct IScriptGraph;

}

namespace CrySchematycEditor {

class CNodesDictionaryCategoryItem;

class CNodesDictionaryNodeItem : public CryGraphEditor::CAbstractDictionaryItem
{
	friend class CNodesDictionaryCreator;

public:
	CNodesDictionaryNodeItem(QString name, const Schematyc::IScriptGraphNodeCreationMenuCommandPtr pCommand, CNodesDictionaryCategoryItem* pParent = nullptr)
		: CryGraphEditor::CAbstractDictionaryItem()
		, m_name(name)
		, m_pCommand(pCommand)
		, m_pParent(pParent)
	{}
	virtual ~CNodesDictionaryNodeItem() {}

	// CryGraphEditor::CAbstractDictionaryItem
	virtual int32                                          GetType() const override { return Type_Entry; }
	virtual QVariant                                       GetIdentifier() const override;

	virtual const QString*                                 GetName() const override { return &m_name; }
	virtual const CryGraphEditor::CAbstractDictionaryItem* GetParentItem() const override;
	// ~CryGraphEditor::CAbstractDictionaryItem

	const Schematyc::IScriptGraphNodeCreationMenuCommandPtr GetCommand() const { return m_pCommand; }

private:
	CNodesDictionaryCategoryItem*                           m_pParent;
	QString                                                 m_name;
	const Schematyc::IScriptGraphNodeCreationMenuCommandPtr m_pCommand;
};

class CNodesDictionaryCategoryItem : public CryGraphEditor::CAbstractDictionaryItem
{
	friend class CNodesDictionaryCreator;

public:
	CNodesDictionaryCategoryItem(QString name, CNodesDictionaryCategoryItem* pParent = nullptr)
		: CryGraphEditor::CAbstractDictionaryItem()
		, m_name(name)
		, m_pParent(pParent)
	{}
	virtual ~CNodesDictionaryCategoryItem() {}

	// CryGraphEditor::CAbstractDictionaryItem
	virtual int32                                          GetType() const override          { return Type_Category; }
	virtual const QString*                                 GetName() const override          { return &m_name; }

	virtual int32                                          GetNumChildItems() const override { return m_categories.size() + m_nodes.size(); }
	virtual const CryGraphEditor::CAbstractDictionaryItem* GetChildItem(int32 index) const override;

	virtual const CryGraphEditor::CAbstractDictionaryItem* GetParentItem() const override;
	// ~CryGraphEditor::CAbstractDictionaryItem

private:
	QString                                    m_name;
	CNodesDictionaryCategoryItem*              m_pParent;
	std::vector<CNodesDictionaryCategoryItem*> m_categories;
	std::vector<CNodesDictionaryNodeItem*>     m_nodes;
};

class CNodesDictionary : public CryGraphEditor::CAbstractDictionary
{
	friend class CNodesDictionaryCreator;

public:
	CNodesDictionary();
	virtual ~CNodesDictionary();

	// CryGraphEditor::CAbstractDictionary
	virtual int32                                          GetNumItems() const override { return m_categories.size() + m_nodes.size(); }
	virtual const CryGraphEditor::CAbstractDictionaryItem* GetItem(int32 index) const override;
	// ~CryGraphEditor::CAbstractDictionary

	void LoadLoadsFromScriptGraph(Schematyc::IScriptGraph& scriptGraph);

private:
	std::vector<CNodesDictionaryCategoryItem*> m_categories;
	std::vector<CNodesDictionaryNodeItem*>     m_nodes;
};

class CNodeGraphRuntimeContext : public CryGraphEditor::INodeGraphRuntimeContext
{
public:
	CNodeGraphRuntimeContext(Schematyc::IScriptGraph& scriptGraph);

	virtual const char*                          GetTypeName() const           { return "Schematyc_Graph"; };
	virtual CryGraphEditor::CAbstractDictionary* GetAvailableNodesDictionary() { return &m_nodesDictionary; };

private:
	CNodesDictionary m_nodesDictionary;
};

}
