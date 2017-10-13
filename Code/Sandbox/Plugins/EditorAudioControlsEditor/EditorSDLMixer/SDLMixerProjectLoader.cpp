// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SDLMixerProjectLoader.h"

#include "AudioSystemControl_sdlmixer.h"

#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryString/CryPath.h>
#include <IAudioSystemEditor.h>
#include <IAudioSystemItem.h>
#include <CryCore/CryCrc32.h>

using namespace PathUtil;

namespace ACE
{
CSdlMixerProjectLoader::CSdlMixerProjectLoader(string const& assetsPath, IAudioSystemItem& rootItem)
	: m_assetsPath(assetsPath)
{
	LoadFolder("", rootItem);
}

void CSdlMixerProjectLoader::LoadFolder(string const& folderPath, IAudioSystemItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(m_assetsPath + CRY_NATIVE_PATH_SEPSTR + folderPath + CRY_NATIVE_PATH_SEPSTR + "*.*", &fd);

	if (handle != -1)
	{
		do
		{
			string const name = fd.name;

			if ((name != ".") && (name != "..") && !name.empty())
			{
				if (fd.attrib & _A_SUBDIR)
				{
					if (folderPath.empty())
					{
						LoadFolder(name, *CreateItem(name, folderPath, eSdlMixerTypes_Folder, parent));
					}
					else
					{
						LoadFolder(folderPath + CRY_NATIVE_PATH_SEPSTR + name, *CreateItem(name, folderPath, eSdlMixerTypes_Folder, parent));
					}
				}
				else
				{
					string::size_type const posExtension = name.rfind('.');

					if (posExtension != string::npos)
					{
						if ((stricmp(name.data() + posExtension, ".mp3") == 0) ||
							(stricmp(name.data() + posExtension, ".ogg") == 0) ||
							(stricmp(name.data() + posExtension, ".wav") == 0))
						{
							// Create the event with the same name as the file
							CreateItem(name, folderPath, eSdlMixerTypes_Event, parent);
						}
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

IAudioSystemItem* CSdlMixerProjectLoader::CreateItem(string const& name, string const& path, ItemType const type, IAudioSystemItem& rootItem)
{
	CID id;

	if (path.empty())
	{
		id = CCrc32::ComputeLowercase(name);
	}
	else
	{
		id = CCrc32::ComputeLowercase(path + CRY_NATIVE_PATH_SEPSTR + name);
	}

	IAudioSystemControl_sdlmixer* const pControl = new IAudioSystemControl_sdlmixer(name, id, type);
	rootItem.AddChild(pControl);
	return pControl;
}
} // namespace ACE
