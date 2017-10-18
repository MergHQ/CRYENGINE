// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ImplControl.h"

#include <CrySystem/XML/IXml.h>
#include <SystemTypes.h>

namespace ACE
{
class CImplItem;

namespace SDLMixer
{
class ProjectLoader
{
public:

	ProjectLoader(string const& sAssetsPath, CImplItem& rootItem);

private:

	CImplItem* CreateItem(string const& name, string const& path, EImpltemType const type, CImplItem& rootItem);
	void       LoadFolder(string const& folderPath, CImplItem& parent);

	string m_assetsPath;
};
} // namespace SDLMixer
} // namespace ACE
