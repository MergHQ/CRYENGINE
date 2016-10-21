// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <IResourceSelectorHost.h>
#include <Schematyc/Utils/Schematyc_Assert.h>
#include <Schematyc/SerializationUtils/Schematyc_SerializationQuickSearch.h>

#include "Schematyc_PluginUtils.h"
#include "Schematyc_QuickSearchDlg.h"

namespace Schematyc
{
	namespace SerializationUtils
	{
		dll_string StringListStaticQuickSearchSelector(const SResourceSelectorContext& context, const char* szPreviousValue)
		{
			SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

			const Private::CStringListStaticQuickSearchOptions* pOptions = static_cast<const Private::CStringListStaticQuickSearchOptions*>(context.pCustomParams.get());
			SCHEMATYC_EDITOR_ASSERT(pOptions);
			if(pOptions)
			{
				CPoint cursorPos;
				GetCursorPos(&cursorPos);

				CQuickSearchDlg quickSearchDlg(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 100, cursorPos.y - 100), *pOptions, szPreviousValue);
				if(quickSearchDlg.DoModal() == IDOK)
				{
					return pOptions->GetName(quickSearchDlg.GetResult());
				}
			}
			return "";
		}

		dll_string StringListQuickSearchSelector(const SResourceSelectorContext& context, const char* szPreviousValue)
		{
			SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE

			const Private::CStringListQuickSearchOptions* pOptions = static_cast<const Private::CStringListQuickSearchOptions*>(context.pCustomParams.get());
			SCHEMATYC_EDITOR_ASSERT(pOptions);
			if(pOptions)
			{
				CPoint cursorPos;
				GetCursorPos(&cursorPos);

				CQuickSearchDlg quickSearchDlg(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 100, cursorPos.y - 100), *pOptions, szPreviousValue);
				if(quickSearchDlg.DoModal() == IDOK)
				{
					return pOptions->GetName(quickSearchDlg.GetResult());
				}
			}
			return "";
		}

		REGISTER_RESOURCE_SELECTOR("StringListStaticQuickSearch", StringListStaticQuickSearchSelector, "icons:General/Search.ico")
		REGISTER_RESOURCE_SELECTOR("StringListQuickSearch", StringListQuickSearchSelector, "icons:General/Search.ico")
	}
}