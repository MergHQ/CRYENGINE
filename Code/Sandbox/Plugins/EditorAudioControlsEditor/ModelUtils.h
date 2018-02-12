// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ProxyModels/ItemModelAttribute.h>

namespace ACE
{
struct IImplItem;

namespace ModelUtils
{
enum class EItemStatus
{
	Placeholder,
	NoConnection,
	NoControl,
	Localized,
	NotificationHeader,
};

void               GetPlatformNames();
QStringList        GetScopeNames();

inline char const* GetItemNotificationIcon(EItemStatus const status)
{
	char const* szIconPath = "icons:Dialogs/dialog-error.ico";

	switch (status)
	{
	case EItemStatus::Placeholder:
		szIconPath = "icons:audio/notifications/placeholder.ico";
		break;
	case EItemStatus::NoConnection:
		szIconPath = "icons:audio/notifications/no_connection.ico";
		break;
	case EItemStatus::NoControl:
		szIconPath = "icons:audio/notifications/no_control.ico";
		break;
	case EItemStatus::Localized:
		szIconPath = "icons:audio/notifications/localized.ico";
		break;
	case EItemStatus::NotificationHeader:
		szIconPath = "icons:General/Scripting.ico";
		break;
	default:
		szIconPath = "icons:Dialogs/dialog-error.ico";
		break;
	}

	return szIconPath;
}

static char const* const s_szSystemMimeType = "AudioSystemItems";
static char const* const s_szImplMimeType = "AudioImplItems";
static QStringList const s_typeFilterList {
	"Trigger", "Parameter", "Switch", "State", "Environment", "Preload"
};

static CItemModelAttribute s_notificationAttribute("Notification", eAttributeType_String, CItemModelAttribute::Visible, false);
static CItemModelAttribute s_placeholderAttribute("Valid Connection", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Unchecked);
static CItemModelAttribute s_connectedAttribute("Connected", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Unchecked);
static CItemModelAttribute s_localizedAttribute("Localized", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Checked);
static CItemModelAttribute s_noControlAttribute("Empty", eAttributeType_Boolean, CItemModelAttribute::AlwaysHidden, true, Qt::Checked);
static CItemModelAttribute s_pathAttribute("Path", eAttributeType_String, CItemModelAttribute::Visible, false);
static CItemModelAttributeEnum s_typeAttribute("Type", s_typeFilterList, CItemModelAttribute::AlwaysHidden, true);
static CItemModelAttributeEnumFunc s_scopeAttribute("Scope", &GetScopeNames, CItemModelAttribute::StartHidden, true);
static std::vector<CItemModelAttribute> s_platformModellAttributes;
} // namespace ModelUtils
} // namespace ACE
