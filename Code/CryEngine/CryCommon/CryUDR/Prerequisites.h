// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Platform/platform.h>
#include <CrySystem/ISystem.h>  // prerequisite for IGame.h
#include <CryGame/IGame.h>
#include <CryGame/IGameFramework.h>
#include <CryExtension/ClassWeaver.h>
#include <CryExtension/CryCreateClassInstance.h>

#include <CrySerialization/ClassFactory.h>  // This header includes ClassFactory.cpp which has some methods marked as inline so we need to include that header to prevent unresolved symbols at link time
