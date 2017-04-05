// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryAnimation/ICryAnimation.h>
#include <CryAnimation/IAnimEventPlayer.h>
#include <CryAudio/IAudioSystem.h>
#include <CryAction/IMaterialEffects.h>
#include <CryExtension/ClassWeaver.h>
#include <CryExtension/CryCreateClassInstance.h>
#include "EffectPlayer.h"

#include <CrySerialization/Decorators/ResourcesAudio.h>
#include <CrySerialization/Decorators/Resources.h>

namespace CharacterTool {
using Serialization::IArchive;

void SerializeParameterAudioTrigger(string& parameter, IArchive& ar)
{
	ar(Serialization::AudioTrigger(parameter), "parameter", "^");
	ar.doc("Audio triggers are defined in ATL Controls Browser.");
}

void SerializeParameterString(string& parameter, IArchive& ar) { ar(parameter, "parameter", "^"); }
void SerializeParameterEffect(string& parameter, IArchive& ar) { ar(Serialization::ParticleName(parameter), "parameter", "^"); }
void SerializeParameterNone(string& parameter, IArchive& ar)   { ar(parameter, "parameter", 0); }

void SerializeParameterAnimFXEvent(string& parameter, IArchive& ar)
{
	Serialization::StringList eventList;
	IAnimFXEvents* pAnimFXEvents = gEnv->pMaterialEffects->GetAnimFXEvents();

	if (!pAnimFXEvents)
	{
		return;
	}

	int resultEventTypeUid = -1;
	int resultEventSubTypeUid = -1;

	uint32 eventTypeUid(0u);
	uint32 eventSubTypeUid(0u);
	sscanf_s(parameter.c_str(), "%u %u", &eventTypeUid, &eventSubTypeUid);

	const int eventsCount = pAnimFXEvents->GetAnimFXEventsCount();
	eventList.reserve(eventsCount + 1);

	eventList.push_back("<unassigned>");

	SAnimFXEventInfo eventType;
	for (int eventIndex = 0; eventIndex < eventsCount; ++eventIndex)
	{
		pAnimFXEvents->GetAnimFXEventInfo(eventIndex, eventType);
		eventList.push_back(eventType.szName);
		if (eventTypeUid == eventType.uniqueId)
		{
			resultEventTypeUid = eventIndex + 1; // + 1 because we already pushed ("<unassigned>")
		}
	}

	Serialization::StringListValue eventTypeChoice(eventList, resultEventTypeUid == -1 ? 0 : resultEventTypeUid);
	ar(eventTypeChoice, "parameter", "^");

	for (int eventIndex = 0; eventIndex < eventsCount; ++eventIndex)
	{
		pAnimFXEvents->GetAnimFXEventInfo(eventIndex, eventType);

		if (!strcmpi(eventTypeChoice.c_str(), eventType.szName))
		{
			resultEventTypeUid = eventType.uniqueId;

			Serialization::StringList eventParamList;

			int eventParamCount = pAnimFXEvents->GetAnimFXEventParamCount(eventIndex);
			eventParamList.reserve(eventsCount + 1);

			eventParamList.push_back("<unassigned>");

			if (eventParamCount)
			{
				SAnimFXEventInfo eventParam;
				for (int eventParamIndex = 0; eventParamIndex < eventParamCount; eventParamIndex++)
				{
					pAnimFXEvents->GetAnimFXEventParam(eventIndex, eventParamIndex, eventParam);
					eventParamList.push_back(eventParam.szName);

					if (eventSubTypeUid == eventParam.uniqueId)
					{
						resultEventSubTypeUid = eventParamIndex + 1; // + 1 because we already pushed ("<unassigned>")
					}
				}

				Serialization::StringListValue eventSubTypeChoice(eventParamList, resultEventSubTypeUid == -1 ? 0 : resultEventSubTypeUid);
				ar(eventSubTypeChoice, "subParameter", "^");

				for (int eventParamIndex = 0; eventParamIndex < eventParamCount; ++eventParamIndex)
				{
					pAnimFXEvents->GetAnimFXEventParam(eventIndex, eventParamIndex, eventParam);
					if (!strcmpi(eventSubTypeChoice.c_str(), eventParam.szName))
					{
						resultEventSubTypeUid = eventParam.uniqueId;
					}
				}
			}
			break; // if (!strcmpi(eventTypeChoice.c_str(), eventType.name))
		}
	}

	parameter.Format("%d %d", resultEventTypeUid == -1 ? 0 : resultEventTypeUid, resultEventSubTypeUid == -1 ? 0 : resultEventSubTypeUid);
}

static void SpliterParameterValue(string* switchName, string* switchValue, const char* parameter)
{
	const char* sep = strchr(parameter, '=');
	if (!sep)
	{
		*switchName = parameter;
		*switchValue = string();
	}
	else
	{
		*switchName = string(parameter, sep);
		*switchValue = sep + 1;
	}
}

void SerializeParameterAudioSwitch(string& parameter, IArchive& ar)
{
	string switchName;
	string switchState;
	SpliterParameterValue(&switchName, &switchState, parameter.c_str());
	ar(Serialization::AudioSwitch(switchName), "switchName", "^");
	ar(Serialization::AudioSwitchState(switchState), "switchState", "^");
	if (ar.isInput())
		parameter = switchName + "=" + switchState;
}

void SerializeParameterAudioRTPC(string& parameter, IArchive& ar)
{
	string name;
	string valueString;
	SpliterParameterValue(&name, &valueString, parameter.c_str());
	float value = (float)atof(valueString.c_str());
	ar(Serialization::AudioRTPC(name), "rtpcName", "^");
	ar(value, "rtpcValue", "^");

	char newValue[32];
	cry_sprintf(newValue, "%f", value);
	if (ar.isInput())
		parameter = name + "=" + newValue;
}

typedef void (* TSerializeParameterFunc)(string&, IArchive&);

static SCustomAnimEventType g_atlEvents[] = {
	{ "audio_trigger", ANIM_EVENT_USES_BONE, "Plays audio trigger using ATL"                                    },
	{ "audio_switch",  0,                    "Sets audio switch using ATL"                                      },
	{ "audio_rtpc",    0,                    "Sets RTPC value using ATL"                                        },
	{ "sound",         ANIM_EVENT_USES_BONE, "Play audio trigger using ATL. Obsolete in favor of audio_trigger" },
};

TSerializeParameterFunc g_atlEventFunctions[sizeof(g_atlEvents) / sizeof(g_atlEvents[0])] = {
	&SerializeParameterAudioTrigger,
	&SerializeParameterAudioSwitch,
	&SerializeParameterAudioRTPC,
	&SerializeParameterAudioTrigger
};

static Vec3 GetBonePosition(ICharacterInstance* character, const QuatTS& physicalLocation, const char* boneName)
{
	int jointId = -1;
	if (boneName != '\0')
		jointId = character->GetIDefaultSkeleton().GetJointIDByName(boneName);
	if (jointId != -1)
		return (physicalLocation * character->GetISkeletonPose()->GetAbsJointByID(jointId)).t;
	else
		return character->GetAABB().GetCenter();
}

class AnimEventPlayer_AudioTranslationLayer : public IAnimEventPlayer
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimEventPlayer)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(AnimEventPlayer_AudioTranslationLayer, "AnimEventPlayer_AudioTranslationLayer", 0xa9fefa2dfe04dec4, 0xa6169d6e3ac635b0)

	AnimEventPlayer_AudioTranslationLayer();
	virtual ~AnimEventPlayer_AudioTranslationLayer();

	const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
	{
		int count = GetCustomTypeCount();
		if (customTypeIndex < 0 || customTypeIndex >= count)
			return 0;
		return &g_atlEvents[customTypeIndex];
	}

	void Initialize() override
	{
		m_audioProxy = gEnv->pAudioSystem->GetFreeAudioProxy();
		if (!m_audioProxy)
			return;
		m_audioProxy->Initialize("Character Tool", false);
		m_audioProxy->SetOcclusionType(eAudioOcclusionType_Ignore);
		SetPredefinedSwitches();
	}

	void Reset() override
	{
		if (!m_audioProxy)
			return;
		SetPredefinedSwitches();
	}

	void SetPredefinedSwitches()
	{
		if (!m_audioProxy)
			return;
		size_t num = m_predefinedSwitches.size();
		for (size_t i = 0; i < num; ++i)
		{
			SetSwitch(m_predefinedSwitches[i].name.c_str(), m_predefinedSwitches[i].state.c_str());
		}
	}

	int GetCustomTypeCount() const override
	{
		return sizeof(g_atlEvents) / sizeof(g_atlEvents[0]);
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		ar.doc("Allows to play triggers and control switches and RPC through Audio Translation Layer.");

		ar(m_predefinedSwitches, "predefinedSwitches", "Predefined Switches");
		ar.doc("Defines a list of switches that are set before audio triggers are played.\n"
		       "Can be used to test switches that are normally set by the game code.\n"
		       "Such switches could be specific to the character, environment or story.");
	}

	const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
	{
		if (customTypeIndex < 0 || customTypeIndex >= GetCustomTypeCount())
			return "";
		m_parameter = parameterValue;
		g_atlEventFunctions[customTypeIndex](m_parameter, ar);
		return m_parameter.c_str();
	}

	void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
	{
		m_playerLocation = playerLocation;

		if (!m_audioEnabled)
			return;

		SAudioRequest request;
		request.pAudioObject = nullptr;
		request.flags = eAudioRequestFlags_PriorityNormal;
		request.pOwner = 0;

		Matrix34 cameraWithOffset = cameraMatrix;
		cameraWithOffset.SetTranslation(cameraMatrix.GetTranslation());
		SAudioListenerRequestData<eAudioListenerRequestType_SetTransformation> requestData(cameraWithOffset);
		request.pData = &requestData;

		gEnv->pAudioSystem->PushRequest(request);

		if (m_audioProxy != nullptr)
		{
			Vec3 const& pos = GetBonePosition(character, m_playerLocation, m_boneToAttachTo.c_str());
			m_audioProxy->SetPosition(pos);
		}
	}

	void EnableAudio(bool enableAudio) override
	{
		m_audioEnabled = enableAudio;
	}

	bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
	{
		if (!m_audioProxy)
			return false;
		const char* name = event.m_EventName ? event.m_EventName : "";
		if (stricmp(name, "sound") == 0 || stricmp(name, "sound_tp") == 0)
		{
			Vec3 const& pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);
			PlayTrigger(event.m_CustomParameter, pos);
			m_boneToAttachTo = event.m_BonePathName;
			return true;
		}
		// If the event is an effect event, spawn the event.
		else if (stricmp(name, "audio_trigger") == 0)
		{
			Vec3 const& pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);
			PlayTrigger(event.m_CustomParameter, pos);
			m_boneToAttachTo = event.m_BonePathName;
			return true;
		}
		else if (stricmp(name, "audio_rtpc") == 0)
		{
			string rtpcName;
			string rtpcValue;
			SpliterParameterValue(&rtpcName, &rtpcValue, event.m_CustomParameter);

			SetRTPC(rtpcName.c_str(), float(atof(rtpcValue.c_str())));
			return true;
		}
		else if (stricmp(name, "audio_switch") == 0)
		{
			string switchName;
			string switchValue;
			SpliterParameterValue(&switchName, &switchValue, event.m_CustomParameter);
			SetSwitch(switchName.c_str(), switchValue.c_str());
			return true;
		}
		return false;
	}

	void SetRTPC(const char* name, float value)
	{
		AudioControlId rtpcId = INVALID_AUDIO_CONTROL_ID;
		gEnv->pAudioSystem->GetAudioRtpcId(name, rtpcId);
		m_audioProxy->SetRtpcValue(rtpcId, value);
	}

	void SetSwitch(const char* name, const char* state)
	{
		AudioControlId switchId = INVALID_AUDIO_CONTROL_ID;
		AudioControlId stateId = INVALID_AUDIO_CONTROL_ID;
		gEnv->pAudioSystem->GetAudioSwitchId(name, switchId);
		gEnv->pAudioSystem->GetAudioSwitchStateId(switchId, state, stateId);
		m_audioProxy->SetSwitchState(switchId, stateId);
	}

	void PlayTrigger(const char* trigger, Vec3 const& pos)
	{
		AudioControlId audioControlId = INVALID_AUDIO_CONTROL_ID;
		gEnv->pAudioSystem->GetAudioTriggerId(trigger, audioControlId);

		if (audioControlId != INVALID_AUDIO_CONTROL_ID)
		{
			m_audioProxy->SetPosition(pos);
			m_audioProxy->ExecuteTrigger(audioControlId);
		}
	}

private:
	struct PredefinedSwitch
	{
		string name;
		string state;

		void   Serialize(Serialization::IArchive& ar)
		{
			ar(Serialization::AudioSwitch(name), "name", "^");
			ar(Serialization::AudioSwitchState(state), "state", "^");
		}
	};

	std::vector<PredefinedSwitch> m_predefinedSwitches;

	bool                          m_audioEnabled;
	IAudioProxy*                  m_audioProxy;
	string                        m_parameter;
	string                        m_boneToAttachTo;

	QuatT                         m_playerLocation;
};

AnimEventPlayer_AudioTranslationLayer::AnimEventPlayer_AudioTranslationLayer()
	: m_audioEnabled(false)
	, m_audioProxy()
{
}

AnimEventPlayer_AudioTranslationLayer::~AnimEventPlayer_AudioTranslationLayer()
{
	if (m_audioProxy)
	{
		m_audioProxy->Release();
		m_audioProxy = nullptr;
	}
}

CRYREGISTER_CLASS(AnimEventPlayer_AudioTranslationLayer)

// ---------------------------------------------------------------------------

static SCustomAnimEventType g_mfxEvents[] = {
	{ "foley",        ANIM_EVENT_USES_BONE, "Foley are used for sounds that are specific to character and material that is being contacted." },
	{ "footstep",     ANIM_EVENT_USES_BONE, "Footstep are specific to character and surface being stepped on."                               },
	{ "groundEffect", ANIM_EVENT_USES_BONE },
};

TSerializeParameterFunc g_mfxEventFuncitons[sizeof(g_mfxEvents) / sizeof(g_mfxEvents[0])] = {
	&SerializeParameterString,
	&SerializeParameterString,
	&SerializeParameterString,
};

class AnimEventPlayerMaterialEffects : public IAnimEventPlayer
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimEventPlayer)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(AnimEventPlayerMaterialEffects, "AnimEventPlayer_MaterialEffects", 0xa9fffa2dae34d6c4, 0xa6969d6e3ac732b0)

	AnimEventPlayerMaterialEffects();
	virtual ~AnimEventPlayerMaterialEffects() {}

	const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
	{
		int count = GetCustomTypeCount();
		if (customTypeIndex < 0 || customTypeIndex >= count)
			return 0;
		return &g_mfxEvents[customTypeIndex];
	}

	int GetCustomTypeCount() const override
	{
		return sizeof(g_mfxEvents) / sizeof(g_mfxEvents[0]);
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		ar.doc("Material Effects allow to define sound and particle response to interaction with certain surface types.");

		ar(m_foleyLibrary, "foleyLibrary", "Foley Library");
		ar.doc("Foley libraries are located in Libs/MaterialEffects/FXLibs");
		ar(m_footstepLibrary, "footstepLibrary", "Footstep Library");
		ar.doc("Foostep libraries are located in Libs/MaterialEffects/FXLibs");
		ar(m_defaultFootstepEffect, "footstepEffectName", "Default Footstep Effect");
		ar.doc("Foostep effect that is used when no effect name specified in AnimEvent.");
	}

	const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
	{
		if (customTypeIndex < 0 || customTypeIndex >= GetCustomTypeCount())
			return "";
		m_parameter = parameterValue;
		g_mfxEventFuncitons[customTypeIndex](m_parameter, ar);
		return m_parameter.c_str();
	}

	void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
	{
		m_playerLocation = playerLocation;
	}

	void EnableAudio(bool enableAudio) override
	{
		m_audioEnabled = enableAudio;
	}

	bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
	{
		const char* name = event.m_EventName ? event.m_EventName : "";
		if (stricmp(name, "footstep") == 0)
		{
			SMFXRunTimeEffectParams params;
			params.pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);
			params.angle = 0;

			IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;
			TMFXEffectId effectId = InvalidEffectId;

			if (pMaterialEffects)
			{
				const char* effectName = (event.m_CustomParameter && event.m_CustomParameter[0]) ? event.m_CustomParameter : m_defaultFootstepEffect.c_str();
				effectId = pMaterialEffects->GetEffectIdByName(m_footstepLibrary.c_str(), effectName);
			}

			if (effectId != InvalidEffectId)
				pMaterialEffects->ExecuteEffect(effectId, params);
			else
				gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for footstep sounds");
			return true;
		}
		else if (stricmp(name, "groundEffect") == 0)
		{
			// setup sound params
			SMFXRunTimeEffectParams params;
			params.pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);
			params.angle = 0;

			IMaterialEffects* materialEffects = gEnv->pMaterialEffects;
			TMFXEffectId effectId = InvalidEffectId;
			if (materialEffects)
				effectId = materialEffects->GetEffectIdByName(event.m_CustomParameter, "default");

			if (effectId != 0)
				materialEffects->ExecuteEffect(effectId, params);
			else
				gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for groundEffect anim event");
			return true;
		}
		else if (stricmp(name, "foley") == 0)
		{
			// setup sound params
			SMFXRunTimeEffectParams params;
			params.angle = 0;
			params.pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);

			IMaterialEffects* materialEffects = gEnv->pMaterialEffects;
			TMFXEffectId effectId = InvalidEffectId;

			// replace with "foley"
			if (materialEffects)
			{
				effectId = materialEffects->GetEffectIdByName(m_foleyLibrary.c_str(),
				                                              event.m_CustomParameter[0] ? event.m_CustomParameter : "default");

				if (effectId != InvalidEffectId)
					materialEffects->ExecuteEffect(effectId, params);
				else
					gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find effect entry for foley sounds");
			}
			return true;
		}
		return false;
	}

private:
	bool   m_audioEnabled;
	string m_parameter;
	QuatT  m_playerLocation;
	string m_foleyLibrary;
	string m_footstepLibrary;
	string m_defaultFootstepEffect;
};

AnimEventPlayerMaterialEffects::AnimEventPlayerMaterialEffects()
	: m_foleyLibrary("foley_player")
	, m_footstepLibrary("footsteps")
	, m_defaultFootstepEffect("concrete")
{
}

CRYREGISTER_CLASS(AnimEventPlayerMaterialEffects)

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
class AnimEventPlayerAnimFXEvents : public IAnimEventPlayer
{
	typedef std::vector<SCustomAnimEventType>    TCustomEventTypes;
	typedef std::vector<TSerializeParameterFunc> TSerializeParamFuncs;

public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimEventPlayer)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(AnimEventPlayerAnimFXEvents, "AnimEventPlayer_AnimFXEvents", 0x9688f29304f3400c, 0x886c2d31c5199481)

	AnimEventPlayerAnimFXEvents();
	virtual ~AnimEventPlayerAnimFXEvents() {}

	const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
	{
		int count = GetCustomTypeCount();
		if (customTypeIndex < 0 || customTypeIndex >= count)
			return 0;
		return &m_eventTypes[customTypeIndex];
	}

	int GetCustomTypeCount() const override
	{
		return m_eventTypes.size();
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		Serialization::StringList soundFXLibsList;

		if (IAnimFXEvents* pAnimFXEvents = gEnv->pMaterialEffects->GetAnimFXEvents())
		{
			uint32 index = 0;
			for (uint32 i = 0; i < pAnimFXEvents->GetLibrariesCount(); ++i)
			{
				const char* soundLibraryName = pAnimFXEvents->GetLibraryName(i);
				soundFXLibsList.push_back(soundLibraryName);
			}

			int foundIdx = soundFXLibsList.find(m_soundFXLib.c_str());
			index = foundIdx == Serialization::StringList::npos ? 0 : uint32(foundIdx);

			Serialization::StringListValue eventListChoice(soundFXLibsList, index);

			ar.doc("These are the defined anim fx libs coming from the game.dll");
			ar(eventListChoice, "animFxLib", "Animation FX Lib");

			if (ISurfaceTypeEnumerator* pSurfaceTypeEnum = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetEnumerator())
			{
				Serialization::StringList envList;
				for (ISurfaceType* pSurfaceType = pSurfaceTypeEnum->GetFirst(); pSurfaceType; pSurfaceType = pSurfaceTypeEnum->GetNext())
				{
					stl::push_back_unique(envList, pSurfaceType->GetType());
				}

				foundIdx = envList.find(m_enviroment.c_str());
				index = foundIdx == Serialization::StringList::npos ? 0 : uint32(foundIdx);

				Serialization::StringListValue envListChoice(envList, index);
				ar(envListChoice, "enviroment", "Enviroment");
				m_enviroment = envListChoice.c_str();
			}

			m_soundFXLib = eventListChoice.c_str();
		}
	}

	const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
	{
		if (customTypeIndex < 0 || customTypeIndex >= GetCustomTypeCount())
			return "";
		m_parameter = parameterValue;
		m_eventSerializeFuncs[customTypeIndex](m_parameter, ar);
		return m_parameter.c_str();
	}

	void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
	{
		m_playerLocation = playerLocation;
	}

	void EnableAudio(bool enableAudio) override
	{
		m_audioEnabled = enableAudio;
	}

	bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
	{
		// Only handle anim_fx events here
		if (strcmpi(event.m_EventName, "anim_fx"))
		{
			return false;
		}

		SMFXRunTimeEffectParams params;
		params.pos = GetBonePosition(character, m_playerLocation, event.m_BonePathName);
		params.angle = 0;

		int eventUid(0);
		int eventNameUid(0);
		sscanf_s(event.m_CustomParameter, "%d %d", &eventUid, &eventNameUid);

		TMFXEffectId effectId = InvalidEffectId;

		if (IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects)
		{
			SAnimFXEventInfo eventType;
			stack_string mfxLibrary;
			stack_string subType("default");

			if (IAnimFXEvents* pAnimFXEvents = pMaterialEffects->GetAnimFXEvents())
			{
				pAnimFXEvents->GetAnimFXEventInfoById(uint32(eventUid), eventType);

				mfxLibrary.Format("%s/%d/%d", m_soundFXLib.c_str(), eventUid, eventNameUid);
				subType = m_enviroment.c_str();

				// Check for enviroment effect
				effectId = pMaterialEffects->GetEffectIdByName(mfxLibrary.c_str(), subType.c_str());

				// Search for regular effect
				if (effectId == InvalidEffectId)
				{
					mfxLibrary.Format("%s/%d/0", m_soundFXLib.c_str(), eventUid);
					subType = pAnimFXEvents->GetAnimFXEventParamName(eventUid, eventNameUid);

					effectId = pMaterialEffects->GetEffectIdByName(mfxLibrary.c_str(), subType.c_str());
				}

				if (effectId != InvalidEffectId)
				{
					pMaterialEffects->ExecuteEffect(effectId, params);
				}
				else
				{
					const char* szFullEventName = pAnimFXEvents->GetAnimFXEventParamName(eventUid, eventNameUid);
					if (!eventType.szName)
					{
						gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find sound event (UID %d/%d) in environment (%s) and definition in (%s).", eventUid, eventNameUid, m_enviroment.c_str(), m_soundFXLib.c_str());
					}
					else if (!szFullEventName)
					{
						gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find sound event (%s/ UID %d) in environment (%s) and definition in (%s).", eventType.szName, eventNameUid, m_enviroment.c_str(), m_soundFXLib.c_str());
					}
					else
					{
						gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find sound event (%s/%s) in environment (%s) and definition in (%s).", eventType.szName, szFullEventName, m_enviroment.c_str(), m_soundFXLib.c_str());
					}
				}
			}
			return true;
		}

		return false;
	}

private:
	bool                 m_audioEnabled;
	string               m_parameter;
	string               m_soundFXLib;
	string               m_enviroment;
	QuatT                m_playerLocation;
	TCustomEventTypes    m_eventTypes;
	TSerializeParamFuncs m_eventSerializeFuncs;
};

AnimEventPlayerAnimFXEvents::AnimEventPlayerAnimFXEvents()
{
	m_eventTypes.push_back({ "anim_fx", ANIM_EVENT_USES_BONE, "Animation fx's are defined by designers (on the game side) and are executed through the Material FX System." });
	m_eventSerializeFuncs.push_back(&SerializeParameterAnimFXEvent);
}

CRYREGISTER_CLASS(AnimEventPlayerAnimFXEvents)

// ---------------------------------------------------------------------------

static SCustomAnimEventType g_particleEvents[] = {
	{ "effect", ANIM_EVENT_USES_BONE | ANIM_EVENT_USES_OFFSET_AND_DIRECTION, "Spawns a particle effect." }
};

TSerializeParameterFunc g_particleEventFuncitons[sizeof(g_particleEvents) / sizeof(g_particleEvents[0])] = {
	&SerializeParameterEffect,
};

class AnimEventPlayer_Particles : public IAnimEventPlayer
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimEventPlayer)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(AnimEventPlayer_Particles, "AnimEventPlayer_Particles", 0xa9fef72df304dec4, 0xa9162a6e4ac635b0)

	AnimEventPlayer_Particles();
	virtual ~AnimEventPlayer_Particles() {}

	const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
	{
		int count = GetCustomTypeCount();
		if (customTypeIndex < 0 || customTypeIndex >= count)
			return 0;
		return &g_particleEvents[customTypeIndex];
	}

	void Initialize() override
	{
		m_effectPlayer.reset(new EffectPlayer());
	}

	int GetCustomTypeCount() const override
	{
		return sizeof(g_particleEvents) / sizeof(g_particleEvents[0]);
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		ar.doc("Spawns a particle effect for each \"effect\" AnimEvent.");

		ar(m_enableRendering, "enableRendering", "Enable Rendering");
		ar.doc("Enables rendering of particle effects in the preview viewport.");
	}

	const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
	{
		if (customTypeIndex < 0 || customTypeIndex >= GetCustomTypeCount())
			return "";
		m_parameter = parameterValue;
		g_particleEventFuncitons[customTypeIndex](m_parameter, ar);
		return m_parameter.c_str();
	}

	void Reset() override
	{
		if (m_effectPlayer)
			m_effectPlayer->KillAllEffects();
	}

	void Render(const QuatT& playerPosition, SRendParams& params, const SRenderingPassInfo& passInfo) override
	{
		if (m_effectPlayer && m_enableRendering)
		{
			m_effectPlayer->Render(params, passInfo);
		}
	}

	void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
	{
		if (m_effectPlayer)
		{
			m_effectPlayer->SetSkeleton(character->GetISkeletonAnim(), character->GetISkeletonPose(), &character->GetIDefaultSkeleton());
			m_effectPlayer->Update(playerLocation);
		}
	}

	bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
	{
		if (stricmp(event.m_EventName, "effect") == 0)
		{
			if (m_effectPlayer)
			{
				m_effectPlayer->SetSkeleton(character->GetISkeletonAnim(), character->GetISkeletonPose(), &character->GetIDefaultSkeleton());
				m_effectPlayer->SpawnEffect(event.m_CustomParameter, event.m_BonePathName, event.m_vOffset, event.m_vDir);
			}
			return true;
		}
		return false;
	}

private:
	string                        m_parameter;
	std::unique_ptr<EffectPlayer> m_effectPlayer;
	bool                          m_enableRendering;
};

AnimEventPlayer_Particles::AnimEventPlayer_Particles()
	: m_parameter()
	, m_effectPlayer()
	, m_enableRendering(true)
{
}

CRYREGISTER_CLASS(AnimEventPlayer_Particles)

// ---------------------------------------------------------------------------

class AnimEventPlayer_CharacterTool : public IAnimEventPlayer
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimEventPlayer)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(AnimEventPlayer_CharacterTool, "AnimEventPlayer_CharacterTool", 0xa5fefb2dfe05dec4, 0xa8169d6e3ac635b0)

	AnimEventPlayer_CharacterTool();
	virtual ~AnimEventPlayer_CharacterTool() {}

	const SCustomAnimEventType * GetCustomType(int customTypeIndex) const override
	{
		int listSize = m_list.size();
		int lowerLimit = 0;
		for (int i = 0; i < listSize; ++i)
		{
			if (!m_list[i])
				continue;
			int typeCount = m_list[i]->GetCustomTypeCount();
			if (customTypeIndex >= lowerLimit && customTypeIndex < lowerLimit + typeCount)
				return m_list[i]->GetCustomType(customTypeIndex - lowerLimit);
			lowerLimit += typeCount;
		}
		return 0;
	}

	void Initialize() override
	{
		size_t listSize = m_list.size();
		for (size_t i = 0; i < listSize; ++i)
		{
			if (!m_list[i].get())
				continue;
			m_list[i]->Initialize();
		}
	}

	int GetCustomTypeCount() const override
	{
		int typeCount = 0;

		size_t listSize = m_list.size();
		for (size_t i = 0; i < listSize; ++i)
		{
			if (!m_list[i].get())
				continue;
			typeCount += m_list[i]->GetCustomTypeCount();
		}

		return typeCount;
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		// m_list could be serialized directly to unlock full customization of the list:
		//   ar(m_list, "list", "^");

		size_t num = m_list.size();
		for (size_t i = 0; i < num; ++i)
			ar(*m_list[i], m_list[i]->GetFactory()->GetName(), m_names[i].c_str());
	}

	const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) override
	{
		int listSize = m_list.size();
		int lowerLimit = 0;
		for (int i = 0; i < listSize; ++i)
		{
			if (!m_list[i])
				continue;
			int typeCount = m_list[i]->GetCustomTypeCount();
			if (customTypeIndex >= lowerLimit && customTypeIndex < lowerLimit + typeCount)
				return m_list[i]->SerializeCustomParameter(parameterValue, ar, customTypeIndex - lowerLimit);
			lowerLimit += typeCount;
		}

		return "";
	}

	void Reset() override
	{
		int listSize = m_list.size();
		for (int i = 0; i < listSize; ++i)
		{
			if (!m_list[i])
				continue;
			m_list[i]->Reset();
		}
	}

	void Render(const QuatT& playerPosition, SRendParams& params, const SRenderingPassInfo& passInfo) override
	{
		int listSize = m_list.size();
		for (int i = 0; i < listSize; ++i)
		{
			if (!m_list[i])
				continue;
			m_list[i]->Render(playerPosition, params, passInfo);
		}
	}

	void Update(ICharacterInstance* character, const QuatT& playerLocation, const Matrix34& cameraMatrix) override
	{
		int listSize = m_list.size();
		for (int i = 0; i < listSize; ++i)
		{
			if (!m_list[i])
				continue;
			m_list[i]->Update(character, playerLocation, cameraMatrix);
		}
	}

	void EnableAudio(bool enableAudio) override
	{
		int listSize = m_list.size();
		for (int i = 0; i < listSize; ++i)
		{
			if (!m_list[i])
				continue;
			m_list[i]->EnableAudio(enableAudio);
		}
	}

	bool Play(ICharacterInstance* character, const AnimEventInstance& event) override
	{
		int listSize = m_list.size();
		for (int i = 0; i < listSize; ++i)
		{
			if (!m_list[i])
				continue;
			if (m_list[i]->Play(character, event))
				return true;
		}
		return false;
	}

private:
	std::vector<IAnimEventPlayerPtr> m_list;
	std::vector<string>              m_names;
	string                           m_defaultGamePlayerName;
};

AnimEventPlayer_CharacterTool::AnimEventPlayer_CharacterTool()
{
	if (ICVar* gameName = gEnv->pConsole->GetCVar("sys_game_name"))
	{
		// Expect game dll to have its own player, add it first into the list,
		// so it has the highest priority.
		string defaultGamePlayerName = "AnimEventPlayer_";
		defaultGamePlayerName += gameName->GetString();
		defaultGamePlayerName.replace(" ", "_");

		IAnimEventPlayerPtr defaultGamePlayer;
		if (::CryCreateClassInstance<IAnimEventPlayer>(defaultGamePlayerName.c_str(), defaultGamePlayer))
		{
			m_list.push_back(defaultGamePlayer);
			m_names.push_back(gameName->GetString());
		}
	}

	IAnimEventPlayerPtr player;
	if (::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_AudioTranslationLayer", player))
	{
		m_list.push_back(player);
		m_names.push_back("Audio Translation Layer");
	}
	if (::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_MaterialEffects", player))
	{
		m_list.push_back(player);
		m_names.push_back("Material Effects");
	}
	if (::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_Particles", player))
	{
		m_list.push_back(player);
		m_names.push_back("Particles");
	}
	if (::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_AnimFXEvents", player))
	{
		m_list.push_back(player);
		m_names.push_back("AnimFX");
	}
}

CRYREGISTER_CLASS(AnimEventPlayer_CharacterTool)

}
