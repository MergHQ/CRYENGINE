// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QMetaType>
#include <array>

// Container with sub-icons of project languages
class CSubIconContainer
{
public:
	CSubIconContainer();

	const QVariantList& GetLanguageIcon(const string& language) const;

private:
	std::array<QVariantList, 4> m_icons;
};
