// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "VersionControlError.h"

namespace VersionControlErrorHandler
{
//! Handles standard errors from VCS such as invalid credentials, wrong setting and so on.
//! \return Returns if passed error is severe or not.
void Handle(SVersionControlError);
};
