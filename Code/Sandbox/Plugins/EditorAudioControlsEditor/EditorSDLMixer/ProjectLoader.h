// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Item.h"

namespace ACE
{
namespace Impl
{
namespace SDLMixer
{
class CImpl;

class CProjectLoader final
{
public:

	CProjectLoader() = delete;

	explicit CProjectLoader(string const& assetsPath, string const& localizedAssetsPath, CItem& rootItem, ItemCache& itemCache, CImpl const& impl);

private:

	CItem* CreateItem(string const& assetsPath, string const& name, string const& path, EItemType const type, CItem& rootItem, EItemFlags flags);
	void   LoadFolder(string const& assetsPath, string const& folderPath, bool const isLocalized, CItem& parent);

	CImpl const& m_impl;
	ItemCache&   m_itemCache;
};
} // namespace SDLMixer
} // namespace Impl
} // namespace ACE
