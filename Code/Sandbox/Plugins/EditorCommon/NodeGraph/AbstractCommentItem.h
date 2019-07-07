// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include "CommentEditorData.h"
#include "AbstractNodeGraphViewModelItem.h"

#include <CrySandbox/CrySignal.h>

namespace CryGraphEditor {

class CNodeGraphViewModel;
class CNodeGraphView;
class CCommentWidget;

class EDITOR_COMMON_API CAbstractCommentItem : public CAbstractNodeGraphViewModelItem
{
public:
	enum : int32 { Type = eItemType_Comment };

public:
	CAbstractCommentItem(CCommentEditorData& data, CNodeGraphViewModel& viewModel);
	virtual ~CAbstractCommentItem();

	// CAbstractNodeGraphViewModelItem
	virtual QVariant            GetId() const override { return m_data.GetId(); }
	virtual int32               GetType() const override { return Type; }
	virtual QPointF             GetPosition() const override;
	virtual void                SetPosition(QPointF position) override;
	virtual void                Serialize(Serialization::IArchive& archive) override;
	// ~CAbstractNodeGraphViewModelItem

public:
	virtual CCommentWidget*     CreateWidget(CNodeGraphView& view) = 0;
	virtual const char*         GetStyleId() const { return "Comment"; }

public:
	CCommentEditorData&         GetEditorData() { return m_data; }
	const CAbstractEditorData&  GetEditorData() const { return m_data; }

public:
	void                        OnDataChanged();

public:
	CCrySignal<void()>          SignalTextChanged;
	CCrySignal<void()>          SignalPositionChanged;

private:
	CCommentEditorData&         m_data;
};

} //namespace CryGraphEditor
