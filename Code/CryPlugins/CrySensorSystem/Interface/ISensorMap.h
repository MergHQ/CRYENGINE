// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Containers/VectorSet.h>

#include "ISensorTagLibrary.h"
#include "SensorBounds.h"

struct SSensorMapParams
{
	inline SSensorMapParams()
		: volumeCount(0)
		, octreeDepth(0)
		, octreeBounds(ZERO)
	{}

	uint32 volumeCount;
	uint32 octreeDepth;
	AABB   octreeBounds;
};

enum class SensorVolumeId : uint32
{
	Invalid = 0xffffffff
};

typedef VectorSet<SensorVolumeId> SensorVolumeIdSet;

enum class ESensorEventType
{
	Enter, // Sent when any volume enters a monitored volume.
	Leave  // Sent when any volume leaves a monitored volume.
};

struct SSensorEvent
{
	inline SSensorEvent(ESensorEventType _type, SensorVolumeId _monitorVolumeId, SensorVolumeId _eventVolumeId)
		: type(_type)
		, monitorVolumeId(_monitorVolumeId)
		, eventVolumeId(_eventVolumeId)
	{}

	ESensorEventType type;
	SensorVolumeId   monitorVolumeId; // Id of monitored volume.
	SensorVolumeId   eventVolumeId;   // Id of entering/leaving volume.
};

typedef Schematyc::CDelegate<void, const SSensorEvent&> SensorEventCallback;

struct SSensorVolumeParams
{
	inline SSensorVolumeParams()
		: entityId(INVALID_ENTITYID)
	{}

	inline SSensorVolumeParams(EntityId _entityId, const CSensorBounds& _bounds, const SensorTags& _tags, const SensorTags& _monitorTags = SensorTags(), const SensorEventCallback& _eventCallback = SensorEventCallback())
		: entityId(_entityId)
		, bounds(_bounds)
		, tags(_tags)
		, monitorTags(_monitorTags)
		, eventCallback(_eventCallback)
	{}

	EntityId            entityId;
	CSensorBounds       bounds;
	SensorTags          tags;
	SensorTags          monitorTags;
	SensorEventCallback eventCallback;
};

enum class ESensorMapDebugFlags
{
	DrawStats          = BIT(0),
	DrawVolumes        = BIT(1),
	DrawVolumeEntities = BIT(2),
	DrawVolumeTags     = BIT(3),
	DrawOctree         = BIT(4),
	DrawStrayVolumes   = BIT(5),
	Interactive        = BIT(6)
};

typedef Schematyc::CEnumFlags<ESensorMapDebugFlags> SensorMapDebugFlags;

struct ISensorMap
{
	virtual ~ISensorMap() {}

	virtual SensorVolumeId      CreateVolume(const SSensorVolumeParams& params) = 0;
	virtual void                DestroyVolume(SensorVolumeId volumeId) = 0;
	virtual void                UpdateVolumeBounds(SensorVolumeId volumeId, const CSensorBounds& bounds) = 0;
	virtual void                SetVolumeTags(SensorVolumeId volumeId, const SensorTags& tags, bool bValue) = 0;
	virtual void                SetVolumeMonitorTags(SensorVolumeId volumeId, const SensorTags& monitorTags, bool bValue) = 0;
	virtual SSensorVolumeParams GetVolumeParams(SensorVolumeId volumeId) const = 0;

	virtual void                Query(SensorVolumeIdSet& results, const CSensorBounds& bounds, const SensorTags& tags = SensorTags(), SensorVolumeId exclusionId = SensorVolumeId::Invalid) const = 0;

	virtual void                Update() = 0;
	virtual void                Debug(float drawRange, const SensorMapDebugFlags& flags) = 0;

	virtual void                SetOctreeDepth(uint32 depth) = 0;
	virtual void                SetOctreeBounds(const AABB& bounds) = 0;
};
