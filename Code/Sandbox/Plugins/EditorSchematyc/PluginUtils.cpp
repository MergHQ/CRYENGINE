// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "PluginUtils.h"

#include <IActionMapManager.h> // #SchematycTODO : Remove dependency on CryAction!
#include <IResourceSelectorHost.h>
#include <ISourceControl.h>
#include <CryEntitySystem/IEntityClass.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryString/CryStringUtils.h>
#include <Schematyc/Utils/Assert.h>

#include <Controls/DictionaryWidget.h>
#include <Controls/QPopupWidget.h>

#include <QtUtil.h>
#include <QPointer>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <CryGame/IGameFramework.h>

namespace CrySchematycEditor {

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

class CActionMapActionQuickSearchOptions : public IActionMapPopulateCallBack
{
public:
	CActionMapActionQuickSearchOptions()
	{
		IActionMapManager* pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
		if (pActionMapManager && (pActionMapManager->GetActionsCount() > 0))
		{
			pActionMapManager->EnumerateActions(this);
		}
	}

	// IActionMapPopulateCallBack
	virtual void AddActionName(const char* const szName) override
	{
		m_names.push_back(szName);
	}
	// ~IActionMapPopulateCallBack

	Serialization::StringListStatic& GetNames() { return m_names; }

private:
	Serialization::StringListStatic m_names;
};

}

namespace {

dll_string EntityClassNameSelector(const SResourceSelectorContext& context, const char* szPreviousValue, Serialization::StringListValue* pStringListValue)
{
	Serialization::StringListStatic names;

	IEntityClassRegistry& entityClassRegistry = *gEnv->pEntitySystem->GetClassRegistry();
	entityClassRegistry.IteratorMoveFirst();

	while (IEntityClass* pClass = entityClassRegistry.IteratorNext())
	{
		names.push_back(pClass->GetName());
	}

	CrySchematycEditor::CStringListDictionary dict(names);
	QPointer<CModalPopupDictionary> pDictionary = new CModalPopupDictionary("Entity Class", dict);

	const QPoint pos = QCursor::pos();
	pDictionary->ExecAt(pos, QPopupWidget::TopRight);

	CrySchematycEditor::CStringListDictionaryEntry* pEntry = static_cast<CrySchematycEditor::CStringListDictionaryEntry*>(pDictionary->GetResult());
	if (pEntry)
	{
		return QtUtil::ToString(pEntry->GetName()).c_str();
	}

	return "";
}

dll_string ActionMapNameSelector(const SResourceSelectorContext& context, const char* szPreviousValue, Serialization::StringListValue* pStringListValue)
{
	Serialization::StringListStatic names;

	IActionMapIteratorPtr itActionMap = gEnv->pGameFramework->GetIActionMapManager()->CreateActionMapIterator();
	while (IActionMap* pActionMap = itActionMap->Next())
	{
		const char* szName = pActionMap->GetName();
		if (strcmp(szName, "default"))
		{
			names.push_back(szName);
		}
	}

	CrySchematycEditor::CStringListDictionary dict(names);
	QPointer<CModalPopupDictionary> pDictionary = new CModalPopupDictionary("Action Map", dict);

	const QPoint pos = QCursor::pos();
	pDictionary->ExecAt(pos, QPopupWidget::TopRight);

	CrySchematycEditor::CStringListDictionaryEntry* pEntry = static_cast<CrySchematycEditor::CStringListDictionaryEntry*>(pDictionary->GetResult());
	if (pEntry)
	{
		return QtUtil::ToString(pEntry->GetName()).c_str();
	}

	return "";
}

dll_string ActionMapActionNameSelector(const SResourceSelectorContext& context, const char* szPreviousValue, Serialization::StringListValue* pStringListValue)
{
	CrySchematycEditor::CActionMapActionQuickSearchOptions quickSearchOptions;

	CrySchematycEditor::CStringListDictionary dict(quickSearchOptions.GetNames());
	QPointer<CModalPopupDictionary> pDictionary = new CModalPopupDictionary("Action Map Action", dict);

	const QPoint pos = QCursor::pos();
	pDictionary->ExecAt(pos, QPopupWidget::TopRight);

	CrySchematycEditor::CStringListDictionaryEntry* pEntry = static_cast<CrySchematycEditor::CStringListDictionaryEntry*>(pDictionary->GetResult());
	if (pEntry)
	{
		return QtUtil::ToString(pEntry->GetName()).c_str();
	}

	return "";
}

REGISTER_RESOURCE_SELECTOR("EntityClassName", EntityClassNameSelector, "")
REGISTER_RESOURCE_SELECTOR("ActionMapName", ActionMapNameSelector, "")
REGISTER_RESOURCE_SELECTOR("ActionMapActionName", ActionMapActionNameSelector, "")
}

namespace CrySchematycEditor {
namespace Utils {

void ConstructAbsolutePath(Schematyc::IString& output, const char* szFileName)
{
	char currentDirectory[512] = "";
	GetCurrentDirectory(sizeof(currentDirectory) - 1, currentDirectory);

	Schematyc::CStackString temp = currentDirectory;
	temp.append("\\");
	temp.append(szFileName);
	temp.replace("/", "\\");

	output.assign(temp);
}

bool WriteToClipboard(const char* szText, const char* szPrefix)
{
	SCHEMATYC_EDITOR_ASSERT(szText);
	if (szText)
	{
		QMimeData* pMimeData = new QMimeData();
		if (szPrefix)
			pMimeData->setData(szPrefix, szText);
		else
			pMimeData->setText(szText);

		QClipboard* pClipboard = QApplication::clipboard();
		pClipboard->setMimeData(pMimeData);

		return true;
	}
	return false;
}

bool ReadFromClipboard(string& text, const char* szPrefix)
{
	QClipboard* pClipboard = QApplication::clipboard();
	if (pClipboard)
	{
		const QMimeData* pMimeData = pClipboard->mimeData();
		if (szPrefix)
		{
			QByteArray byteArray = pMimeData->data(szPrefix);
			if (byteArray.length() > 0)
			{
				text.reserve(byteArray.length());
				text.assign(byteArray.data(), byteArray.length());
				return true;
			}
		}
		else
		{
			text = pMimeData->text().toUtf8();
			return text.length() > 0;
		}
	}

	text.clear();
	return false;
}

bool ValidateClipboardContents(const char* szPrefix)
{
	SCHEMATYC_EDITOR_ASSERT(szPrefix);

	QClipboard* pClipboard = QApplication::clipboard();
	if (pClipboard)
	{
		const QMimeData* pMimeData = pClipboard->mimeData();
		QByteArray byteArray = pMimeData->data(szPrefix);
		if (byteArray.length() > 0)
		{
			return true;
		}
	}
	return false;
}

}
}
