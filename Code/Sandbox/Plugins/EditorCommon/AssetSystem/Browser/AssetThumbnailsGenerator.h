// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace AsseThumbnailsGenerator
{

void GenerateThumbnailsAsync(const string& folder, const std::function<void()>& finalize = std::function<void()>());

};
