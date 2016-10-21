// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Schematyc_PluginUtils.h"

#include <IActionMapManager.h> // #SchematycTODO : Remove dependency on CryAction!
#include <IResourceSelectorHost.h>
#include <ISourceControl.h>
#include <CryEntitySystem/IEntityClass.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryString/CryStringUtils.h>
#include <Schematyc/Utils/Schematyc_Assert.h>

#include "Schematyc_QuickSearchDlg.h"

namespace
{
class CSimpleQuickSearchOptions : public Schematyc::IQuickSearchOptions
{
public:

	inline CSimpleQuickSearchOptions(const char* szHeader, const char* szDelimiter)
		: m_header(szHeader)
		, m_delimiter(szDelimiter)
	{}

	// IQuickSearchOptions

	virtual uint32 GetCount() const override
	{
		return m_options.size();
	}

	virtual const char* GetLabel(uint32 optionIdx) const override
	{
		return optionIdx < m_options.size() ? m_options[optionIdx].c_str() : "";
	}

	virtual const char* GetDescription(uint32 optionIdx) const override
	{
		return nullptr;
	}

	virtual const char* GetWikiLink(uint32 optionIdx) const override
	{
		return nullptr;
	}

	virtual const char* GetHeader() const override
	{
		return m_header.c_str();
	}

	virtual const char* GetDelimiter() const override
	{
		return m_delimiter.c_str();
	}

	// ~IQuickSearchOptions

	void AddOption(const char* szOption)
	{
		m_options.push_back(szOption);
	}

private:

	string              m_header;
	string              m_delimiter;
	std::vector<string> m_options;
};

class CActionMapActionQuickSearchOptions : public CSimpleQuickSearchOptions, public IActionMapPopulateCallBack
{
public:

	inline CActionMapActionQuickSearchOptions()
		: CSimpleQuickSearchOptions("Action Map Action", ".")
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
		CSimpleQuickSearchOptions::AddOption(szName);
	}

	// ~IActionMapPopulateCallBack
};

dll_string EntityClassNameSelector(const SResourceSelectorContext& context, const char* szPreviousValue, Serialization::StringListValue* pStringListValue)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CPoint cursorPos;
	GetCursorPos(&cursorPos);

	IEntityClassRegistry& entityClassRegistry = *gEnv->pEntitySystem->GetClassRegistry();
	entityClassRegistry.IteratorMoveFirst();

	CSimpleQuickSearchOptions quickSearchOptions("Entity Class", "::");
	while (IEntityClass* pClass = entityClassRegistry.IteratorNext())
	{
		quickSearchOptions.AddOption(pClass->GetName());
	}

	Schematyc::CQuickSearchDlg quickSearchDlg(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 10, cursorPos.y - 10), quickSearchOptions, szPreviousValue);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		return quickSearchOptions.GetLabel(quickSearchDlg.GetResult());
	}
	else
	{
		return "";
	}
}

dll_string ActionMapNameSelector(const SResourceSelectorContext& context, const char* szPreviousValue, Serialization::StringListValue* pStringListValue)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CPoint cursorPos;
	GetCursorPos(&cursorPos);

	CSimpleQuickSearchOptions quickSearchOptions("Action Map", "::");
	IActionMapIteratorPtr itActionMap = gEnv->pGameFramework->GetIActionMapManager()->CreateActionMapIterator();
	while (IActionMap* pActionMap = itActionMap->Next())
	{
		const char* szName = pActionMap->GetName();
		if (strcmp(szName, "default"))
		{
			quickSearchOptions.AddOption(szName);
		}
	}

	Schematyc::CQuickSearchDlg quickSearchDlg(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 10, cursorPos.y - 10), quickSearchOptions, szPreviousValue);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		return quickSearchOptions.GetLabel(quickSearchDlg.GetResult());
	}
	else
	{
		return "";
	}
}

dll_string ActionMapActionNameSelector(const SResourceSelectorContext& context, const char* szPreviousValue, Serialization::StringListValue* pStringListValue)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

	CPoint cursorPos;
	GetCursorPos(&cursorPos);

	CActionMapActionQuickSearchOptions quickSearchOptions;
	Schematyc::CQuickSearchDlg quickSearchDlg(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 10, cursorPos.y - 10), quickSearchOptions, szPreviousValue);
	if (quickSearchDlg.DoModal() == IDOK)
	{
		return quickSearchOptions.GetLabel(quickSearchDlg.GetResult());
	}
	else
	{
		return "";
	}
}

REGISTER_RESOURCE_SELECTOR("EntityClassName", EntityClassNameSelector, "")
REGISTER_RESOURCE_SELECTOR("ActionMapName", ActionMapNameSelector, "")
REGISTER_RESOURCE_SELECTOR("ActionMapActionName", ActionMapActionNameSelector, "")
}

namespace Schematyc
{
namespace PluginUtils
{
CLocalResourceScope::CLocalResourceScope()
	: m_hPrevInstance(AfxGetResourceHandle())
{
	AfxSetResourceHandle(GetHInstance());
}

CLocalResourceScope::~CLocalResourceScope()
{
	AfxSetResourceHandle(m_hPrevInstance);
}

SDragAndDropData::SDragAndDropData()
	: icon(InvalidIdx)
{}

SDragAndDropData::SDragAndDropData(uint32 _icon, const SGUID& _guid)
	: icon(_icon)
	, guid(_guid)
{}

const UINT SDragAndDropData::GetClipboardFormat()
{
	static const UINT clipboardFormat = RegisterClipboardFormat("Schematyc::DragAndDropItem");
	return clipboardFormat;
}

bool BeginDragAndDrop(const SDragAndDropData& data)
{
	CSharedFile sharedFile(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT);
	sharedFile.Write(&data, sizeof(data));
	if (HGLOBAL hData = sharedFile.Detach())
	{
		COleDataSource* pDataSource = new COleDataSource();
		pDataSource->CacheGlobalData(SDragAndDropData::GetClipboardFormat(), hData);
		pDataSource->DoDragDrop(DROPEFFECT_MOVE | DROPEFFECT_LINK);
		return true;
	}
	return false;
}

bool GetDragAndDropData(COleDataObject* pDataObject, SDragAndDropData& data)
{
	if (HGLOBAL hData = pDataObject->GetGlobalData(SDragAndDropData::GetClipboardFormat()))
	{
		data = *static_cast<const SDragAndDropData*>(GlobalLock(hData));
		GlobalUnlock(hData);
		return true;
	}
	return false;
}

bool IsValidName(const char* szName, CStackString& errorMessage)
{
	if (!szName || !szName[0])
	{
		errorMessage = "Name cannot be empty.";
		return false;
	}
	else if (StringUtils::CharIsNumeric(szName[0]))
	{
		errorMessage = "Name cannot start with a number.";
		return false;
	}
	else if (StringUtils::ContainsNonAlphaNumericChars(szName, "_"))
	{
		errorMessage = "Name can only contain alphanumeric characters and underscores.";
		return false;
	}
	return true;
}

bool IsValidFilePath(const char* szFilePath, CStackString& errorMessage)
{
	if (!szFilePath || !szFilePath[0])
	{
		errorMessage = "File path cannot be empty.";
		return false;
	}
	else if (StringUtils::CharIsNumeric(szFilePath[0]))
	{
		errorMessage = "File path cannot start with a number.";
		return false;
	}
	else if (StringUtils::ContainsNonAlphaNumericChars(szFilePath, "_/\\"))
	{
		errorMessage = "File path can only contain alphanumeric characters, underscores and slashes.";
		return false;
	}
	return true;
}

bool IsValidFileName(const char* szFileName, CStackString& errorMessage)
{
	if (!szFileName || !szFileName[0])
	{
		errorMessage = "File name cannot be empty.";
		return false;
	}
	else if (StringUtils::CharIsNumeric(szFileName[0]))
	{
		errorMessage = "File name cannot start with a number.";
		return false;
	}
	else if (StringUtils::ContainsNonAlphaNumericChars(szFileName, "_"))
	{
		errorMessage = "File name can only contain alphanumeric characters and underscores.";
		return false;
	}
	return true;
}

bool ValidateFilePath(HWND hWnd, const char* szFilePath, bool bDisplayErrorMessages)
{
	CStackString errorMessage;
	if (IsValidFilePath(szFilePath, errorMessage))
	{
		return true;
	}
	if (bDisplayErrorMessages)
	{
		MessageBox(hWnd, errorMessage.c_str(), "Invalid File Path", MB_OK | MB_ICONEXCLAMATION);
	}
	return false;
}

void ConstructAbsolutePath(IString& output, const char* szFileName)
{
	char currentDirectory[512] = "";
	GetCurrentDirectory(sizeof(currentDirectory) - 1, currentDirectory);

	CStackString temp = currentDirectory;
	temp.append("\\");
	temp.append(szFileName);
	temp.replace("/", "\\");

	output.assign(temp);
}

void ViewWiki(const char* szWikiLink)
{
	ShellExecute(NULL, "open", szWikiLink, NULL, NULL, SW_SHOWDEFAULT);
}

void ShowInExplorer(const char* szPath)
{
	SCHEMATYC_EDITOR_ASSERT(szPath);
	if (szPath)
	{
		CStackString path = szPath;
		path.replace("/", "\\");
		ShellExecute(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}
}

void DiffAgainstHaveVersion(const char* szLHSFileName, const char* szRHSFileName)
{
	CStackString diffToolPath = "E:\\DATA\\game_hunt\\Tools\\GameHunt\\Diff\\Diff.exe"; // #SchematycTODO : This should not be hard coded!!!

	CStackString diffToolParams = "Diff "; // #SchematycTODO : This should not be hard coded!!!
	diffToolParams.append(szLHSFileName);
	diffToolParams.append(" ");
	diffToolParams.append(szRHSFileName);
	diffToolParams.replace("/", "\\");

	ShellExecute(NULL, "open", diffToolPath.c_str(), diffToolParams.c_str(), NULL, SW_SHOWDEFAULT);
}

bool WriteToClipboard(const char* szText, const char* szPrefix)
{
	SCHEMATYC_EDITOR_ASSERT(szText);
	if (szText)
	{
		if (!OpenClipboard(NULL))
		{
			AfxMessageBox("Cannot open the Clipboard");
			return false;
		}

		if (!EmptyClipboard())
		{
			AfxMessageBox("Cannot empty the Clipboard");
			return false;
		}

		CSharedFile sharedFile(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT);
		if (szPrefix)
		{
			sharedFile.Write(szPrefix, strlen(szPrefix));
		}
		sharedFile.Write(szText, strlen(szText));

		HGLOBAL hSharedFileMemory = sharedFile.Detach();
		if (!hSharedFileMemory)
		{
			return false;
		}

		if (!::SetClipboardData(CF_TEXT, hSharedFileMemory))
		{
			AfxMessageBox("Unable to set Clipboard data");
			return false;
		}

		CloseClipboard();
		return true;
	}
	return false;
}

bool ReadFromClipboard(string& text, const char* szPrefix)
{
	if (OpenClipboard(NULL))
	{
		const char* szText = static_cast<const char*>(GetClipboardData(CF_TEXT));

		CloseClipboard();

		if (szText)
		{
			if (szPrefix)
			{
				const uint32 prefixLength = strlen(szPrefix);
				if (strncmp(szText, szPrefix, prefixLength) == 0)
				{
					text = szText + prefixLength;
					return true;
				}
			}
			else
			{
				text = szText;
				return true;
			}
		}
	}
	return false;
}

bool ValidateClipboardContents(const char* szPrefix)
{
	SCHEMATYC_EDITOR_ASSERT(szPrefix);
	if (szPrefix)
	{
		if (OpenClipboard(NULL))
		{
			const char* szText = static_cast<const char*>(GetClipboardData(CF_TEXT));

			CloseClipboard();

			if (szText)
			{
				return strncmp(szText, szPrefix, strlen(szPrefix)) == 0;
			}
		}
	}
	return false;
}

void WriteXmlToClipboard(XmlNodeRef xml)
{
	SCHEMATYC_EDITOR_ASSERT(xml);
	if (xml)
	{
		WriteToClipboard(xml->getXMLData()->GetString());
	}
}

XmlNodeRef ReadXmlFromClipboard(const char* szTag)
{
	SCHEMATYC_EDITOR_ASSERT(szTag);
	if (szTag)
	{
		string clipboardText;
		if (ReadFromClipboard(clipboardText))
		{
			string::size_type pos = clipboardText.find_first_of("<");
			if (pos != CStackString::npos)
			{
				pos = clipboardText.find_first_not_of(" \t", pos + 1);
				if ((pos != CStackString::npos) && (strncmp(clipboardText.c_str() + pos, szTag, strlen(szTag)) == 0))
				{
					return gEnv->pSystem->LoadXmlFromBuffer(clipboardText.c_str(), clipboardText.length());
				}
			}
		}
	}
	return XmlNodeRef();
}

EntityId GetSelectedEntityId()
{
	SCHEMATYC_EDITOR_ASSERT("Broken since integration from main, please fix me!!!");
	/*CBaseObject* pSelectedObject = ::GetIEditor()->GetSelectedObject();
	   if(pSelectedObject != NULL)
	   {
	   if(pSelectedObject->IsKindOf(RUNTIME_CLASS(CEntityObject)) == TRUE)
	   {
	    IEntity*	pSelectedEntity = static_cast<CEntityObject*>(pSelectedObject)->GetIEntity();
	    if(pSelectedEntity != NULL)
	    {
	      return pSelectedEntity->GetId();
	    }
	   }
	   }*/
	return 0;
}
} // PluginUtils
} // Schematyc
