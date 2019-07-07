// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include "NodeEditorData.h"
#include "AbstractNodeGraphViewModelItem.h"

#include <CrySandbox/CrySignal.h>

#include <QVariant>
#include <QString>
#include <QColor>
#include <QPointF>

namespace CryGraphEditor {

class CNodeGraphViewModel;

class CNodeGraphView;

class CNodeWidget;
class CCommentWidget;
class CConnectionWidget;
class CAbstractNodeData;

class CAbstractPinItem;
typedef std::vector<CAbstractPinItem*> PinItemArray;

class EDITOR_COMMON_API CAbstractNodeItem : public CAbstractNodeGraphViewModelItem
{
public:
	enum : int32
	{
		Type = eItemType_Node
	};

public:
	CAbstractNodeItem(CNodeEditorData& data, CNodeGraphViewModel& viewModel);
	virtual ~CAbstractNodeItem();

	// CAbstractNodeGraphViewModelItem
	virtual QVariant            GetId() const override { return m_data.GetId(); }
	virtual int32               GetType() const override { return Type; }

	virtual QString             GetName() const override { return m_name; }
	virtual void                SetName(const QString& name) override;

	virtual QPointF             GetPosition() const override;
	virtual void                SetPosition(QPointF position) override;

	virtual bool                IsDeactivated() const override { return m_isDeactivated; }
	virtual void                SetDeactivated(bool isDeactivated) override;

	virtual bool                HasWarnings() const override { return m_hasWarnings; }
	virtual void                SetWarnings(bool hasWarnings) override;

	virtual bool                HasErrors() const override { return m_hasErrors; }
	virtual void                SetErrors(bool hasErrors) override;
	// ~CAbstractNodeGraphViewModelItem

	virtual CNodeWidget*        CreateWidget(CNodeGraphView& view) = 0;
	virtual const char*         GetStyleId() const { return "Node"; }
	virtual QVariant            GetTypeId() const = 0;

	virtual const PinItemArray& GetPinItems() const = 0;
	virtual CAbstractPinItem*   GetPinItemById(QVariant id) const;
	virtual CAbstractPinItem*   GetPinItemByIndex(uint32 index) const;

public:
	CNodeEditorData&            GetEditorData() { return m_data; }
	const CNodeEditorData&      GetEditorData() const { return m_data; }

	uint32                      GetPinItemIndex(const CAbstractPinItem& pin) const;

public:
	void                        OnDataChanged();

public:
	CCrySignal<void()>                      SignalPositionChanged;
	CCrySignal<void(bool isDeactivated)>    SignalDeactivatedChanged;	

	CCrySignal<void(CAbstractPinItem& pin)> SignalPinAdded;
	CCrySignal<void(CAbstractPinItem& pin)> SignalPinRemoved;
	CCrySignal<void(CAbstractPinItem& pin)> SignalPinInvalidated;

protected:
	CNodeEditorData&   m_data;
	QString            m_name;

private:
	bool m_isDeactivated   : 1;
	bool m_hasWarnings     : 1;
	bool m_hasErrors       : 1;
};

}
