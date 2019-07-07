// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CPakFile;

/*! Class responsible for exporting particles to game format.
 */
/** Export brushes from specified Indoor to .bld file.
 */
class CParticlesExporter
{
public:
	void ExportParticles(const string& path, const string& levelName, CPakFile& pakFile);
};
