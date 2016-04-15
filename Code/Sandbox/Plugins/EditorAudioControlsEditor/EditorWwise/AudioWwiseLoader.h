// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/XML/IXml.h>
#include "ACETypes.h"
#include "AudioSystemControl_wwise.h"

namespace ACE
{
class CAudioSystemEditor_wwise;

class CAudioWwiseLoader
{

public:
	CAudioWwiseLoader() : m_pAudioSystemImpl(nullptr) {}
	void   Load(CAudioSystemEditor_wwise* pAudioSystemImpl);
	string GetLocalizationFolder() const;

private:
	void LoadSoundBanks(const string& rootFolder, const bool bLocalized);
	void LoadControlsInFolder(const string& folderPath);
	void LoadControl(XmlNodeRef root);
	void ExtractControlsFromXML(XmlNodeRef root, EWwiseItemTypes type, const string& controlTag, const string& controlNameAttribute);

private:
	CAudioSystemEditor_wwise* m_pAudioSystemImpl;
};
}
