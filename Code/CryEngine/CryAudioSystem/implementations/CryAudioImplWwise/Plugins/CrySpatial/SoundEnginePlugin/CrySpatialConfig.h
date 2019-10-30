// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
// If you change the CompanyID / PluginID, make sure to also change
// the values in the xml file located in the WwisePlugin directory.
namespace CrySpatialConfig
{
static const unsigned short g_companyID = 137;
static const unsigned short g_pluginID = 1000;
}         // namespace CrySpatialConfig

namespace CrySpatialAttachmentConfig
{
static const unsigned short g_companyID = 137;
static const unsigned short g_pluginID = 1001;
} // namespace CrySpatialAttachmentConfig
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
