// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#define SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE Schematyc::PluginUtils::CLocalResourceScope localResourceScope;

#include <Schematyc/Utils/Schematyc_IString.h>

namespace Schematyc
{
namespace PluginUtils
{
class CLocalResourceScope
{
public:

	CLocalResourceScope();

	~CLocalResourceScope();

private:

	HINSTANCE m_hPrevInstance;
};

struct SDragAndDropData
{
	SDragAndDropData();

	SDragAndDropData(uint32 _icon, const SGUID& _guid = SGUID());

	static inline const UINT GetClipboardFormat();

	uint32                   icon;
	SGUID                    guid;
};

bool       BeginDragAndDrop(const SDragAndDropData& data);
bool       GetDragAndDropData(COleDataObject* pDataObject, SDragAndDropData& data);

bool       IsValidName(const char* szName, CStackString& errorMessage);
bool       IsValidFilePath(const char* szFilePath, CStackString& errorMessage);
bool       IsValidFileName(const char* szFileName, CStackString& errorMessage);
bool       ValidateFilePath(HWND hWnd, const char* szFilePath, bool bDisplayErrorMessages);
void       ConstructAbsolutePath(IString& output, const char* szFileName);

void       ViewWiki(const char* szWikiLink);
void       ShowInExplorer(const char* szFileName);
void       DiffAgainstHaveVersion(const char* szLHSFileName, const char* szRHSFileName);

bool       WriteToClipboard(const char* szText, const char* szPrefix = nullptr);
bool       ReadFromClipboard(string& text, const char* szPrefix = nullptr);
bool       ValidateClipboardContents(const char* szPrefix);

void       WriteXmlToClipboard(XmlNodeRef xml);
XmlNodeRef ReadXmlFromClipboard(const char* szTag);

EntityId   GetSelectedEntityId();
} // PluginUtils
} // Schematyc
