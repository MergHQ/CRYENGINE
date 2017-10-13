// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/XML/IXml.h>
#include <ACETypes.h>

namespace ACE
{
class IAudioSystemItem;

class CSdlMixerProjectLoader
{
public:

	CSdlMixerProjectLoader(string const& sAssetsPath, IAudioSystemItem& rootItem);

private:

	IAudioSystemItem* CreateItem(string const& name, string const& path, ItemType const type, IAudioSystemItem& rootItem);
	void              LoadFolder(string const& folderPath, IAudioSystemItem& parent);

	string m_assetsPath;
};
} // namespace ACE
