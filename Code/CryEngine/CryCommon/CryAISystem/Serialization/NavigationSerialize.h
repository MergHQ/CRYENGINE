// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/INavigationSystem.h>
#include <CryAISystem/NavigationSystem/NavMeshQueryFilterDefault.h>

#include <CrySerialization/Forward.h>

// Serialization Decorators

namespace NavigationSerialization
{
	struct NavigationQueryFilter
	{
		SNavMeshQueryFilterDefault& variable;
		NavigationQueryFilter(SNavMeshQueryFilterDefault& filter)
			: variable(filter)
		{}
		void Serialize(Serialization::IArchive& ar);
	};
	
	struct NavigationQueryFilterWithCosts
	{
		SNavMeshQueryFilterDefaultWithCosts& variable;
		NavigationQueryFilterWithCosts(SNavMeshQueryFilterDefaultWithCosts& filter)
			: variable(filter) 
		{}
		void Serialize(Serialization::IArchive& ar);
	};

	struct NavigationAreaCost
	{
		uint32& index;
		float&  cost;
		NavigationAreaCost(uint32& index, float& cost)
			: index(index), cost(cost)
		{}
		void Serialize(Serialization::IArchive& ar);
	};

	struct NavigationAreaFlagsMask
	{
		MNM::AreaAnnotation::value_type& value;
		NavigationAreaFlagsMask(MNM::AreaAnnotation::value_type& value) 
			: value(value) 
		{}
		void Serialize(Serialization::IArchive& ar);
	};

	struct SnappingMetric
	{
		MNM::SSnappingMetric& value;
		SnappingMetric(MNM::SSnappingMetric& value)
			: value(value)
		{}
		void Serialize(Serialization::IArchive& ar);
	};
}

bool Serialize(Serialization::IArchive& archive, NavigationVolumeID& value, const char* szName, const char* szLabel);
bool Serialize(Serialization::IArchive& archive, NavigationAgentTypeID& value, const char* szName, const char* szLabel);
bool Serialize(Serialization::IArchive& archive, NavigationAreaTypeID& value, const char* szName, const char* szLabel);
bool Serialize(Serialization::IArchive& archive, NavigationAreaFlagID& value, const char* szName, const char* szLabel);
bool Serialize(Serialization::IArchive& archive, SNavMeshQueryFilterDefault& value, const char* szName, const char* szLabel);
bool Serialize(Serialization::IArchive& archive, SNavMeshQueryFilterDefaultWithCosts& value, const char* szName, const char* szLabel);

namespace MNM
{
	bool Serialize(Serialization::IArchive& archive, SSnappingMetric& value, const char* szName, const char* szLabel);
	bool Serialize(Serialization::IArchive& archive, SOrderedSnappingMetrics& value, const char* szName, const char* szLabel);
}


SERIALIZATION_ENUM_BEGIN_NESTED2(MNM, SSnappingMetric, EType, "Type")
SERIALIZATION_ENUM(MNM::SSnappingMetric::EType::Vertical, "vertical", "Vertical")
SERIALIZATION_ENUM(MNM::SSnappingMetric::EType::Box, "box", "Box")
SERIALIZATION_ENUM(MNM::SSnappingMetric::EType::Circular, "circular", "Circular")
SERIALIZATION_ENUM_END()

#include "NavigationSerialize.inl"
