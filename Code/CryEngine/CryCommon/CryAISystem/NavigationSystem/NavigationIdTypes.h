// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <limits>

enum ENavigationIDTag
{
	MeshIDTag = 0,
	AgentTypeIDTag,
	VolumeIDTag,
	TileGeneratorExtensionIDTag,
	TileIDTag,
	TileTriangleIDTag,
	OffMeshLinkIDTag,
	AreaTypeIDTag,
	AreaFlagIDTag,
};

template<ENavigationIDTag T, uint32 valueInvalid = 0>
struct TNavigationID
{
	explicit TNavigationID(uint32 id = valueInvalid) : id(id) {}

	TNavigationID& operator=(const TNavigationID& other)        { id = other.id; return *this; }
	ILINE          operator uint32() const                      { return id; } //TODO: remove this operator and add function for returning uint32 value
	ILINE bool     operator==(const TNavigationID& other) const { return id == other.id; }
	ILINE bool     operator!=(const TNavigationID& other) const { return id != other.id; }
	ILINE bool     operator<(const TNavigationID& other) const  { return id < other.id; }
	ILINE bool     IsValid() const { return id != valueInvalid; }
	ILINE void     Invalidate() { id = valueInvalid; }
	ILINE uint32   GetValue() const { return id; }

private:
	uint32 id;
};

//TODO: Don't change 'valueInvalid' for other types than NavigationAreaTypeID and NavigationAreaFlagID,
// before all locations are checked in the code where the value is validated using operator uint32()
typedef TNavigationID<MeshIDTag>                                       NavigationMeshID;
typedef TNavigationID<AgentTypeIDTag>                                  NavigationAgentTypeID;
typedef TNavigationID<VolumeIDTag>                                     NavigationVolumeID;
typedef TNavigationID<TileGeneratorExtensionIDTag>                     TileGeneratorExtensionID;
typedef TNavigationID<AreaTypeIDTag, std::numeric_limits<UINT>::max()> NavigationAreaTypeID;
typedef TNavigationID<AreaFlagIDTag, std::numeric_limits<UINT>::max()> NavigationAreaFlagID;

namespace std
{
template <ENavigationIDTag T, uint32 valueInvalid>
struct hash<TNavigationID<T, valueInvalid>>
{
	size_t operator () (const TNavigationID<T, valueInvalid>& value) const
	{
		std::hash<uint32> hasher;
		return hasher(value);
	}
};
}
