// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/IArchive.h>
#include <CrySerialization/CryExtension.h>
#include <CryExtension/ClassWeaver.h>

struct AnimEventInstance;
struct ICharacterInstance;
struct SRendParams;
struct SRenderingPassInfo;

//! Defines parameters that are used by a custom event type.
enum EAnimEventParameters
{
	ANIM_EVENT_USES_BONE                 = 1 << 0,
	ANIM_EVENT_USES_BONE_INLINE          = 1 << 1,
	ANIM_EVENT_USES_OFFSET_AND_DIRECTION = 1 << 2,
	ANIM_EVENT_USES_ALL_PARAMETERS       = ANIM_EVENT_USES_BONE | ANIM_EVENT_USES_OFFSET_AND_DIRECTION
};

struct SCustomAnimEventType
{
	const char* name;
	int         parameters;   //!< EAnimEventParameters.
	const char* description;  //!< Used for tooltips in Character Tool.
};

//! Allows to preview different kinds of animevents within Character Tool.
struct IAnimEventPlayer : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IAnimEventPlayer, 0x2e2f7475542447f3, 0xb6729edb4a3af495);

	//! Can be used to customize parameter type for editing.
	virtual bool Play(ICharacterInstance* character, const AnimEventInstance& animEvent) = 0;
	virtual void Initialize() {}

	//! Used when animation is being rewound.
	virtual void Reset()                                                                                          {}
	virtual void Update(ICharacterInstance* character, const QuatT& playerPosition, const Matrix34& cameraMatrix) {}
	virtual void Render(const QuatT& playerPosition, SRendParams& params, const SRenderingPassInfo& passInfo)     {}
	virtual void EnableAudio(bool enable)                                                                         {}
	virtual void Serialize(Serialization::IArchive& ar)                                                           {}

	//! Allows to customize editing of custom parameter of AnimEvent.
	virtual const char*                 SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) { return ""; }
	virtual const SCustomAnimEventType* GetCustomType(int customTypeIndex) const                                                               { return 0; }
	virtual int                         GetCustomTypeCount() const                                                                             { return 0; }
};

DECLARE_SHARED_POINTERS(IAnimEventPlayer);

inline bool Serialize(Serialization::IArchive& ar, IAnimEventPlayerPtr& pointer, const char* name, const char* label)
{
	Serialization::CryExtensionPointer<IAnimEventPlayer, IAnimEventPlayer> serializer(pointer);
	return ar(serializer, name, label);
}

// Game-specific anim event player, will be automatically found by the character tool
struct IAnimEventPlayerGame : public IAnimEventPlayer
{
	CRYINTERFACE_DECLARE(IAnimEventPlayerGame, 0x3218AD9C82374C5F, 0x8B6487BDEDAF1C4A);
};