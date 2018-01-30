// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ImplControls.h"

#include <CrySystem/XML/IXml.h>
#include <SystemTypes.h>

namespace ACE
{
namespace Fmod
{
class CProjectLoader final
{
public:

	CProjectLoader(string const& projectPath, string const& soundbanksPath, CImplItem& root);

private:

	CImplItem* CreateItem(string const& name, EImplItemType const type, CImplItem* const pParent);
	CImplItem* GetControl(CID const id) const;

	void       LoadBanks(string const& folderPath, bool const isLocalized, CImplItem& parent);
	void       ParseFolder(string const& folderPath, string const& folderName, CImplItem& parent);
	void       ParseFile(string const& filepath, CImplItem& parent);
	void       RemoveEmptyMixerGroups();

	CImplItem* GetContainer(string const& id, EImplItemType const type, CImplItem& parent);
	CImplItem* LoadContainer(XmlNodeRef const pNode, EImplItemType const type, string const& relationshipParamName, CImplItem& parent);
	CImplItem* LoadSnapshotGroup(XmlNodeRef const pNode, CImplItem& parent);
	CImplItem* LoadFolder(XmlNodeRef const pNode, CImplItem& parent);
	CImplItem* LoadMixerGroup(XmlNodeRef const pNode, CImplItem& parent);

	CImplItem* LoadItem(XmlNodeRef const pNode, EImplItemType const type, CImplItem& parent);
	CImplItem* LoadEvent(XmlNodeRef const pNode, CImplItem& parent);
	CImplItem* LoadSnapshot(XmlNodeRef const pNode, CImplItem& parent);
	CImplItem* LoadReturn(XmlNodeRef const pNode, CImplItem& parent);
	CImplItem* LoadParameter(XmlNodeRef const pNode, CImplItem& parent);
	CImplItem* LoadVca(XmlNodeRef const pNode, CImplItem& parent);

	using ControlsCache = std::map<CID, CImplItem*>;
	using ItemIds = std::map<string, CImplItem*>;

	CImplItem&                    m_root;
	ControlsCache                 m_controlsCache;
	ItemIds                       m_containerIds;
	ItemIds                       m_snapshotGroupItems;
	std::vector<CImplMixerGroup*> m_emptyMixerGroups;
	string const                  m_projectPath;
};
} // namespace Fmod
} // namespace ACE
