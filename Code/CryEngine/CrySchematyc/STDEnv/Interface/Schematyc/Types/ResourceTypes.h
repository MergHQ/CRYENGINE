// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Move serialization utils to separate header?

#pragma once

#include <CrySerialization/IArchive.h>
#include <CrySerialization/Decorators/ResourceFilePath.h>
#include <CrySerialization/Decorators/Resources.h>
#include <CrySerialization/Decorators/ResourcesAudio.h>

namespace Schematyc
{
namespace SerializationUtils
{

template<Serialization::ResourceFilePath(* SERIALIZER) (string&)> struct SResourceNameSerializer
{
	inline SResourceNameSerializer() {}

	inline SResourceNameSerializer(const char* _szValue)
		: value(_szValue)
	{}

	inline bool operator==(const SResourceNameSerializer& rhs) const
	{
		return value == rhs.value;
	}

	string value;
};

template<Serialization::ResourceFilePath(* SERIALIZER) (string&)> inline bool Serialize(Serialization::IArchive& archive, SResourceNameSerializer<SERIALIZER>& value, const char* szName, const char* szLabel)
{
	archive(SERIALIZER(value.value), szName, szLabel);
	return true;
}

template<Serialization::ResourceSelector<string>(* SELECTOR)(string&)> struct SResourceNameSelector
{
	inline SResourceNameSelector() {}

	inline SResourceNameSelector(const char* _szValue)
		: value(_szValue)
	{}

	inline bool operator==(const SResourceNameSelector& rhs) const
	{
		return value == rhs.value;
	}

	string value;
};

template<Serialization::ResourceSelector<string>(* SELECTOR)(string&)> inline bool Serialize(Serialization::IArchive& archive, SResourceNameSelector<SELECTOR>& value, const char* szName, const char* szLabel)
{
	archive(SELECTOR(value.value), szName, szLabel);
	return true;
}

inline Serialization::ResourceFilePath GeomPath(string& value)
{
	return Serialization::ResourceFilePath(value, "Geometry (cgf)|*.cgf");
}

inline Serialization::ResourceFilePath SkinName(string& value)
{
	return Serialization::ResourceFilePath(value, "Attachment Geometry (skin)|*.skin", "Objects");
}

inline Serialization::ResourceFilePath ParticleEffectName(string& value)
{
	return Serialization::ResourceFilePath(value, "Particle Effect (pfx)|*.pfx", "Objects");
}

inline Serialization::ResourceFilePath ObjectIconPath(string& value)
{
	return Serialization::ResourceFilePath(value, "Bitmaps (bmp)|*.bmp");
}

inline Serialization::ResourceSelector<string> EntityClassName(string& value)
{
	return Serialization::ResourceSelector<string>(value, "EntityClassName");
}

inline Serialization::ResourceFilePath MannequinAnimationDatabasePath(string& value)
{
	return Serialization::ResourceFilePath(value, "Animation Database (adb)|*.adb");
}

inline Serialization::ResourceFilePath MannequinControllerDefinitionPath(string& value)
{
	return Serialization::ResourceFilePath(value, "Constroller Definition (xml)|*.xml");
}

inline Serialization::ResourceSelector<string> ActionMapName(string& value)
{
	return Serialization::ResourceSelector<string>(value, "ActionMapName");
}

inline Serialization::ResourceSelector<string> ActionMapActionName(string& value)
{
	return Serialization::ResourceSelector<string>(value, "ActionMapActionName");
}
} // SerializationUtils

typedef SerializationUtils::SResourceNameSelector<& Serialization::MaterialPicker<string>> MaterialFileName;

inline void ReflectType(CTypeDesc<MaterialFileName>& desc)
{
	desc.SetGUID("2c8887a2-0878-42cc-8cee-a0595baa0820"_schematyc_guid);
	desc.SetLabel("MaterialFileName");
	desc.SetDescription("Material file name");
}

typedef SerializationUtils::SResourceNameSerializer<&SerializationUtils::GeomPath> GeomFileName;

inline void ReflectType(CTypeDesc<GeomFileName>& desc)
{
	desc.SetGUID("bd6f2953-1127-4cdd-bfe7-79f98c97058c"_schematyc_guid);
	desc.SetLabel("GeomFileName");
	desc.SetDescription("Geometry file name");
}

typedef SerializationUtils::SResourceNameSerializer<&SerializationUtils::SkinName> SkinName;

inline void ReflectType(CTypeDesc<SkinName>& desc)
{
	desc.SetGUID("243e1764-0bca-4af4-8a24-bf8a2fc8f9c8"_schematyc_guid);
	desc.SetLabel("SkinName");
	desc.SetDescription("Skin name");
}

typedef SerializationUtils::SResourceNameSelector<&Serialization::CharacterPath<string>> CharacterFileName;

inline void ReflectType(CTypeDesc<CharacterFileName>& desc)
{
	desc.SetGUID("cb3189c1-92de-4851-b26a-22894ec039b0"_schematyc_guid);
	desc.SetLabel("CharacterFileName");
	desc.SetDescription("Character file name");
}

typedef SerializationUtils::SResourceNameSerializer<&SerializationUtils::ParticleEffectName> ParticleEffectName;

inline void ReflectType(CTypeDesc<ParticleEffectName>& desc)
{
	desc.SetGUID("a6f326e7-d3d3-428a-92c9-97185f003bec"_schematyc_guid);
	desc.SetLabel("ParticleEffectName");
	desc.SetDescription("Particle effect name");
}

typedef SerializationUtils::SResourceNameSelector<&Serialization::AudioSwitch<string>> AudioSwitchName;

inline void ReflectType(CTypeDesc<AudioSwitchName>& desc)
{
	desc.SetGUID("43ae9bb4-9831-449d-b082-3e37b71ec872"_schematyc_guid);
	desc.SetLabel("AudioSwitchName");
	desc.SetDescription("Audio switch name");
}

typedef SerializationUtils::SResourceNameSelector<&Serialization::AudioSwitchState<string>> AudioSwitchStateName;

inline void ReflectType(CTypeDesc<AudioSwitchStateName>& desc)
{
	desc.SetGUID("1877be04-a1db-490c-b6a2-47f99cac7fc2"_schematyc_guid);
	desc.SetLabel("AudioSwitchStateName");
	desc.SetDescription("Audio switch state name");
}

typedef SerializationUtils::SResourceNameSelector<&Serialization::AudioRTPC<string>> AudioRtpcName;

inline void ReflectType(CTypeDesc<AudioRtpcName>& desc)
{
	desc.SetGUID("730c191c-531f-48ae-bba9-5c1d8216b701"_schematyc_guid);
	desc.SetLabel("AudioRtpcName");
	desc.SetDescription("Audio Rtpc name");
}

typedef SerializationUtils::SResourceNameSelector<&Serialization::DialogName<string>> DialogName;

inline void ReflectType(CTypeDesc<DialogName>& desc)
{
	desc.SetGUID("839adde8-cc62-4636-9f26-16dd4236855f"_schematyc_guid);
	desc.SetLabel("DialogName");
	desc.SetDescription("Dialog name");
}

typedef SerializationUtils::SResourceNameSelector<&SerializationUtils::EntityClassName> EntityClassName;

inline void ReflectType(CTypeDesc<EntityClassName>& desc)
{
	desc.SetGUID("c366ed5c-5242-453f-bb2d-7247f7be2655"_schematyc_guid);
	desc.SetLabel("EntityClassName");
	desc.SetDescription("Entity class name");
}

typedef SerializationUtils::SResourceNameSelector<&Serialization::ForceFeedbackIdName<string>> ForceFeedbackId;

inline void ReflectType(CTypeDesc<ForceFeedbackId>& desc)
{
	desc.SetGUID("40dce92c-9532-4c02-8752-4afac04331ef"_schematyc_guid);
	desc.SetLabel("ForceFeedbackId");
	desc.SetDescription("Force feedback id");
}

} // Schematyc
