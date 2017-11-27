// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemTypes.h"

#include <ProxyModels/ItemModelAttribute.h>

namespace ACE
{
class CImplItem;

namespace ModelUtils
{
enum class EItemStatus
{
	Placeholder,
	NoConnection,
	NoControl,
	Localized,
};

void        GetPlatformNames();
QStringList GetScopeNames();

inline char const* GetItemNotificationIcon(EItemStatus const status)
{
	switch (status)
	{
	case EItemStatus::Placeholder:
		return "icons:audio/notifications/placeholder.ico";
	case EItemStatus::NoConnection:
		return "icons:audio/notifications/no_connection.ico";
	case EItemStatus::NoControl:
		return "icons:audio/notifications/no_control.ico";
	case EItemStatus::Localized:
		return "icons:audio/notifications/localized.ico";
	}
	return "icons:Dialogs/dialog-error.ico";
}

static char const* const s_szMiddlewareMimeType = "application/cryengine-audioimplementationitem";
static QStringList const s_typeFilterList{ "Trigger","Parameter","Switch","State","Environment","Preload" };

static CItemModelAttribute              s_notificationAttribute("Notification", eAttributeType_String, CItemModelAttribute::Visible, false);
static CItemModelAttribute              s_placeholderAttribute("Valid Connection", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Unchecked);
static CItemModelAttribute              s_connectedAttribute("Connected", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Unchecked);
static CItemModelAttribute              s_localizedAttribute("Localized", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Checked);
static CItemModelAttribute              s_noControlAttribute("Empty", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Checked);
static CItemModelAttribute              s_pathAttribute("Path", eAttributeType_String, CItemModelAttribute::Visible, false);
static CItemModelAttributeEnum          s_typeAttribute("Type", s_typeFilterList, CItemModelAttribute::AlwaysHidden, true);
static CItemModelAttributeEnumFunc      s_scopeAttribute("Scope", &GetScopeNames, CItemModelAttribute::StartHidden, true);
static std::vector<CItemModelAttribute> s_platformModellAttributes;
} // namespace ModelUtils
} // namespace ACE
