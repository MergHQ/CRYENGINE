// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "ModelUtils.h"
#include "AudioControlsEditorPlugin.h"

#include <IImplItem.h>
#include <IEditorImpl.h>
#include <ConfigurationManager.h>

namespace ACE
{
namespace ModelUtils
{
//////////////////////////////////////////////////////////////////////////
void GetPlatformNames()
{
	s_platformModellAttributes.clear();
	std::vector<dll_string> const& platforms = GetIEditor()->GetConfigurationManager()->GetPlatformNames();

	for (auto const& platform : platforms)
	{
		CItemModelAttribute platformAttribute = CItemModelAttribute(platform.c_str(), eAttributeType_Boolean, CItemModelAttribute::StartHidden, false);
		s_platformModellAttributes.emplace_back(platformAttribute);
	}
}

//////////////////////////////////////////////////////////////////////////
QStringList GetScopeNames()
{
	QStringList scopeNames;
	ScopeInfoList scopeInfoList;
	CAudioControlsEditorPlugin::GetAssetsManager()->GetScopeInfoList(scopeInfoList);

	for (auto const& scopeInfo : scopeInfoList)
	{
		scopeNames.append(QString(scopeInfo.name));
	}

	scopeNames.sort(Qt::CaseInsensitive);

	return scopeNames;
}
} // namespace ModelUtils
} // namespace ACE
