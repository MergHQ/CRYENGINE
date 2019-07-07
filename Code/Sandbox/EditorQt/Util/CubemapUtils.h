// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CubemapUtils
{
bool GenCubemapWithPathAndSize(string& filename, const int size, const bool dds = true);
bool GenCubemapWithObjectPathAndSize(string& filename, CBaseObject* pObject, const int size, const bool dds);
void GenHDRCubemapTiff(const string& fileName, size_t size, Vec3& pos);
void RegenerateAllEnvironmentProbeCubemaps();
void GenerateCubemaps();
}
