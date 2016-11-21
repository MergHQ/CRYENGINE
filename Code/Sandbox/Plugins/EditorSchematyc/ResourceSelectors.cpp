// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <IResourceSelectorHost.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/SerializationUtils/SerializationQuickSearch.h>

#include "PluginUtils.h"
#include "TypesDictionary.h"

#include <Controls/DictionaryWidget.h>
#include <Controls/QPopupWidget.h>

#include <QtUtil.h>
#include <QPointer>

enum EColumn : int32
{
	Column_Name,
	Column_COUNT
};

class CStringListDictionaryEntry : public CAbstractDictionaryEntry
{
	friend class CStringListDictionary;

public:
	CStringListDictionaryEntry() {}
	virtual ~CStringListDictionaryEntry() {}

	// CAbstractDictionaryEntry
	virtual uint32   GetType() const override { return CAbstractDictionaryEntry::Type_Entry; }

	virtual QVariant GetColumnValue(int32 columnIndex) const override
	{
		if (columnIndex == Column_Name)
		{
			return m_name;
		}
		return QVariant();
	}
	// ~CAbstractDictionaryEntry

	QString GetName() const { return m_name; }

private:
	QString m_name;
};

class CStringListDictionary : public CAbstractDictionary
{
public:
	CStringListDictionary(const Serialization::StringListStatic& names)
	{
		Load(names);
	}
	CStringListDictionary(const Serialization::StringList& names)
	{
		Load(names);
	}
	virtual ~CStringListDictionary() {}

	// CryGraphEditor::CAbstractDictionary
	virtual int32                           GetNumEntries() const override       { return m_names.size(); }
	virtual const CAbstractDictionaryEntry* GetEntry(int32 index) const override { return (m_names.size() > index) ? &m_names[index] : nullptr; }

	virtual int32                           GetNumColumns() const override       { return Column_COUNT; };
	virtual QString                         GetColumnName(int32 index) const override
	{
		if (index == Column_Name)
			return QString("Name");
		return QString();
	}

	virtual int32 GetDefaultFilterColumn() const override { return Column_Name; }
	// ~CryGraphEditor::CAbstractDictionary

	void Load(const Serialization::StringListStatic& names)
	{
		for (string name : names)
		{
			CStringListDictionaryEntry entry;
			entry.m_name = name;
			m_names.emplace_back(entry);
		}
	}

	void Load(const Serialization::StringList& names)
	{
		for (string name : names)
		{
			CStringListDictionaryEntry entry;
			entry.m_name = name;
			m_names.emplace_back(entry);
		}
	}

private:
	std::vector<CStringListDictionaryEntry> m_names;
};

namespace Schematyc {
namespace SerializationUtils {

dll_string StringListStaticQuickSearchSelector(const SResourceSelectorContext& context, const char* szPreviousValue)
{
	const Private::CStringListStaticQuickSearchOptions* pOptions = static_cast<const Private::CStringListStaticQuickSearchOptions*>(context.pCustomParams.get());
	SCHEMATYC_EDITOR_ASSERT(pOptions);
	if (pOptions)
	{
		CStringListDictionary dict(pOptions->m_names);
		QPointer<CModalPopupDictionary> pDictionary = new CModalPopupDictionary("Resource Picker Menu", dict);

		const QPoint pos = QCursor::pos();
		pDictionary->ExecAt(pos, QPopupWidget::TopRight);

		CStringListDictionaryEntry* pEntry = static_cast<CStringListDictionaryEntry*>(pDictionary->GetResult());
		if (pEntry)
		{
			return QtUtil::ToString(pEntry->GetName()).c_str();
		}
	}
	return "";
}

dll_string StringListQuickSearchSelector(const SResourceSelectorContext& context, const char* szPreviousValue)
{
	const Private::CStringListQuickSearchOptions* pOptions = static_cast<const Private::CStringListQuickSearchOptions*>(context.pCustomParams.get());
	SCHEMATYC_EDITOR_ASSERT(pOptions);
	if (pOptions)
	{
		CStringListDictionary dict(pOptions->m_names);
		QPointer<CModalPopupDictionary> pDictionary = new CModalPopupDictionary("Resource Picker Menu", dict);

		const QPoint pos = QCursor::pos();
		pDictionary->ExecAt(pos, QPopupWidget::TopRight);

		CStringListDictionaryEntry* pEntry = static_cast<CStringListDictionaryEntry*>(pDictionary->GetResult());
		if (pEntry)
		{
			return QtUtil::ToString(pEntry->GetName()).c_str();
		}
	}
	return "";
}

REGISTER_RESOURCE_SELECTOR("StringListStaticQuickSearch", StringListStaticQuickSearchSelector, "icons:General/Search.ico")
REGISTER_RESOURCE_SELECTOR("StringListQuickSearch", StringListQuickSearchSelector, "icons:General/Search.ico")
}
}
