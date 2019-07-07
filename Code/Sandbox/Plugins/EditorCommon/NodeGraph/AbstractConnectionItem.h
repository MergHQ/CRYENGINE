// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"

#include "AbstractNodeGraphViewModelItem.h"

#include <CrySandbox/CrySignal.h>

class QColor;

namespace CryGraphEditor {

class CNodeGraphViewModel;
class CNodeGraphView;

class CAbstractPinItem;
class CConnectionWidget;

class EDITOR_COMMON_API CAbstractConnectionItem : public CAbstractNodeGraphViewModelItem
{
public:
	enum : int32 { Type = eItemType_Connection };

public:
	CAbstractConnectionItem(CNodeGraphViewModel& viewModel); //[[deprecated("Use new constructor that requires to set source and target pin.")]]
	CAbstractConnectionItem(CAbstractPinItem& sourcePin, CAbstractPinItem& targetPin, CNodeGraphViewModel& viewModel);
	virtual ~CAbstractConnectionItem();

	// CAbstractNodeGraphViewModelItem
	virtual int32 GetType() const override       { return Type; }

	virtual bool  IsDeactivated() const override { return m_isDeactivated; }
	virtual void  SetDeactivated(bool isDeactivated) override;
	// ~CAbstractNodeGraphViewModelItem

	virtual QVariant           GetId() const = 0;
	virtual bool               HasId(QVariant id) const = 0;

	virtual CConnectionWidget* CreateWidget(CNodeGraphView& view) = 0;
	virtual const char*        GetStyleId() const { return "Connection"; }

	virtual CAbstractPinItem&  GetSourcePinItem() const
	{
		CRY_ASSERT_MESSAGE(m_pSourcePin, "Source pin not set. Application about to crash.");
		return *m_pSourcePin;
	}
	virtual CAbstractPinItem& GetTargetPinItem() const
	{
		CRY_ASSERT_MESSAGE(m_pTargetPin, "Target pin not set. Application about to crash.");
		return *m_pTargetPin;
	}

public:
	CCrySignal<void()> SignalDeactivatedChanged;

private:
	CAbstractPinItem* m_pSourcePin;
	CAbstractPinItem* m_pTargetPin;

	bool              m_isDeactivated : 1;
};

}
