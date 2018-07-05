// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QVariant.h>
#include <CrySandbox/CrySignal.h>
#include <CrySerialization/IArchive.h>

namespace CryGraphEditor {

class EDITOR_COMMON_API CAbstractEditorData
{
public:
	CAbstractEditorData(QVariant id);
	virtual ~CAbstractEditorData() {}

	QVariant           GetId() const { return m_id; }

	virtual void       Serialize(Serialization::IArchive& archive);

public:
	CCrySignal<void()> SignalDataChanged;

private:
	QVariant           m_id;
};

} // namespace CryGraphEditor
