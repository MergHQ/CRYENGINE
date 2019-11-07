// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CrySpatialAttachmentPlugin.h"
#include "../SoundEnginePlugin/CrySpatialFXFactory.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
//////////////////////////////////////////////////////////////////////////
void CrySpatialAttachmentPlugin::Destroy()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialAttachmentPlugin::SetPluginPropertySet(AK::Wwise::IPluginPropertySet* pPropertySet)
{
	m_pPropertySet = pPropertySet;
}

//////////////////////////////////////////////////////////////////////////
bool CrySpatialAttachmentPlugin::GetBankParameters(GUID const& guid, AK::Wwise::IWriteData* pDataWriter) const
{
	return true;
}
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
