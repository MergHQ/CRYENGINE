// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SubIconContainer.h"

#include <CryIcon.h>
#include <QThumbnailView.h>

CSubIconContainer::CSubIconContainer()
{
	QVariant v;
	v.setValue(QThumbnailsView::SSubIcon{ CryIcon("icons:General/Project_Language_CSharp.ico"), QThumbnailsView::SSubIcon::EPosition::BottomRight });
	m_icons[0].push_back(v);

	v.setValue(QThumbnailsView::SSubIcon{ CryIcon("icons:General/Project_Language_Cpp.ico"), QThumbnailsView::SSubIcon::EPosition::BottomRight });
	m_icons[1].push_back(v);

	v.setValue(QThumbnailsView::SSubIcon{ CryIcon("icons:General/Project_Language_VisualScripting.ico"), QThumbnailsView::SSubIcon::EPosition::BottomRight });
	m_icons[2].push_back(v);

	v.setValue(QThumbnailsView::SSubIcon{ CryIcon("icons:General/Project_Language_Unknown.ico"), QThumbnailsView::SSubIcon::EPosition::BottomRight });
	m_icons[3].push_back(v);
}

const QVariantList& CSubIconContainer::GetLanguageIcon(const string& language) const
{
	if (language.CompareNoCase("C#") == 0)
	{
		return m_icons[0];
	}

	if (language.CompareNoCase("C++") == 0)
	{
		return m_icons[1];
	}

	if (language.CompareNoCase("schematyc") == 0)
	{
		return m_icons[2];
	}

	return m_icons[3];
}
