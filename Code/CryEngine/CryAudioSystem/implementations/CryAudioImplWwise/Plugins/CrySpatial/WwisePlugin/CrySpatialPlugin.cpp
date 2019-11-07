// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CrySpatialPlugin.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
//////////////////////////////////////////////////////////////////////////
void CrySpatialPlugin::Destroy()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialPlugin::SetPluginPropertySet(AK::Wwise::IPluginPropertySet* pPropertySet)
{
	m_pPropertySet = pPropertySet;
}

//////////////////////////////////////////////////////////////////////////
bool CrySpatialPlugin::GetBankParameters(GUID const& guid, AK::Wwise::IWriteData* pDataWriter) const
{
	return true;
}
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
