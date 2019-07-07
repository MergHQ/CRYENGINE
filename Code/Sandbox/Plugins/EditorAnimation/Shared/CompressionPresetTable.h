// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AnimationFilter.h"
#include "AnimSettings.h"

struct SCompressionPresetEntry
{
	string               name;
	SAnimationFilter     filter;
	SCompressionSettings settings;

	void                 Serialize(Serialization::IArchive& ar);
};

struct SCompressionPresetTable
{
	std::vector<SCompressionPresetEntry> entries;

	const SCompressionPresetEntry* FindPresetForAnimation(const char* animationPath, const vector<string>& tags) const;

	bool Load(const char* tablePath);
	bool Save(const char* tablePath);
	void Serialize(Serialization::IArchive& ar);
};
