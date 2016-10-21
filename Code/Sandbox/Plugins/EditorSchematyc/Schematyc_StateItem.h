// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc_AbstractObjectItem.h"
#include "Schematyc_VariablesModel.h"

#include "Schematyc_GraphItem.h"

#include <QString>
#include <QIcon>

namespace Schematyc {

struct IScriptState;

}

namespace CrySchematycEditor {

class CVariableItem;

class CStateItem : public CAbstractObjectStructureModelItem, public CAbstractVariablesModel
{
public:
	CStateItem(Schematyc::IScriptState& scriptState, CAbstractObjectStructureModel& model);
	virtual ~CStateItem();

	// CAbstractObjectStructureModelItem
	virtual void                               SetName(QString name) override;
	virtual int32                              GetType() const override       { return eObjectItemType_State; }
	virtual const QIcon*                       GetIcon() const override       { return &s_icon; }
	virtual CAbstractObjectStructureModelItem* GetParentItem() const override { return m_pParentItem; }

	virtual uint32                             GetNumChildItems() const override;
	virtual CAbstractObjectStructureModelItem* GetChildItemByIndex(uint32 index) const override;
	virtual uint32                             GetChildItemIndex(const CAbstractObjectStructureModelItem& item) const override;

	virtual uint32                             GetIndex() const;
	virtual void                               Serialize(Serialization::IArchive& archive) override;

	virtual bool                               AllowsRenaming() const override;
	// ~CAbstractObjectStructureModelItem

	void        SetParentItem(CAbstractObjectStructureModelItem* pParent) { m_pParentItem = pParent; }

	CGraphItem* CreateGraph(CGraphItem::EGraphType type);
	bool        RemoveGraph(CGraphItem& functionItem);

	CStateItem* CreateState();
	bool        RemoveState(CStateItem& stateItem);

	//CSignalItem* CreateSignal();
	//bool         RemoveState(CSignalItem& stateItem);

	// CAbstractVariablesViewModel
	virtual uint32                       GetNumVariables() const override { return m_variables.size(); }
	virtual CAbstractVariablesModelItem* GetVariableItemByIndex(uint32 index) override;
	virtual uint32                       GetVariableItemIndex(const CAbstractVariablesModelItem& variableItem) const;
	virtual CAbstractVariablesModelItem* CreateVariable() override;
	virtual bool                         RemoveVariable(CAbstractVariablesModelItem& variableItem) override;
	// ~CAbstractVariablesViewModel

	// CAbstractSignalsModel
	// TODO:!
	// ~CAbstractSignalsModel

	Schematyc::SGUID GetGUID() const;

protected:
	void LoadFromScriptElement();

private:
	static QIcon                       s_icon;

	CAbstractObjectStructureModelItem* m_pParentItem;
	Schematyc::IScriptState&           m_scriptState;

	std::vector<CGraphItem*>           m_graphs;
	std::vector<CStateItem*>           m_states;

	// CAbstractVariablesViewModel
	std::vector<CVariableItem*> m_variables;
};

}
