// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef __tracks_h__
	#define __tracks_h__

	#include <CryMovie/AnimKey.h>

class CCameraTrack : public TAnimTrack<SCameraKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_Camera; }

	virtual void           SerializeKey(SCameraKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			const char* pSelection = keyNode->getAttr("node");
			cry_strcpy(key.m_selection, pSelection);
			keyNode->getAttr("BlendTime", key.m_blendTime);
		}
		else
		{
			keyNode->setAttr("node", key.m_selection);
			keyNode->setAttr("BlendTime", key.m_blendTime);
		}
	}
};

class CFaceSequenceTrack : public TAnimTrack<SFaceSequenceKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_FaceSequence; }

	virtual void           SerializeKey(SFaceSequenceKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		key.m_duration.Serialize(keyNode, bLoading, "durationTicks", "duration");

		if (bLoading)
		{
			const char* pSelection = keyNode->getAttr("node");
			cry_strcpy(key.m_selection, pSelection);
		}
		else
		{
			keyNode->setAttr("node", key.m_selection);
		}
	}
};

class CCommentTrack : public TAnimTrack<SCommentKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_CommentText; }

	virtual void           SerializeKey(SCommentKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		key.m_duration.Serialize(keyNode, bLoading, "durationTicks", "duration");

		if (bLoading)
		{
			// Load only in the editor for loading performance.
			if (gEnv->IsEditor())
			{
				XmlString xmlComment;
				keyNode->getAttr("comment", xmlComment);
				key.m_comment = xmlComment;
				const char* strFont = keyNode->getAttr("font");
				cry_strcpy(key.m_font, strFont);
				keyNode->getAttr("color", key.m_color);
				keyNode->getAttr("size", key.m_size);
				int alignment = 0;
				keyNode->getAttr("align", alignment);
				key.m_align = (SCommentKey::ETextAlign)(alignment);
			}
		}
		else
		{
			XmlString xmlComment(key.m_comment.c_str());
			keyNode->setAttr("comment", xmlComment);
			keyNode->setAttr("font", key.m_font);
			keyNode->setAttr("color", key.m_color);
			keyNode->setAttr("size", key.m_size);
			keyNode->setAttr("align", (int)key.m_align);
		}
	}
};

class CAudioTriggerTrack : public TAnimTrack<SAudioTriggerKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_AudioTrigger; }

	//! Check if track is masked
	virtual bool IsMasked(const uint32 mask) const override { return (mask & eTrackMask_MaskSound) != 0; }

	virtual void SerializeKey(SAudioTriggerKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			const char* pStartTrigger = keyNode->getAttr("StartTrigger");
			cry_strcpy(key.m_startTrigger, pStartTrigger, sizeof(key.m_startTrigger));
			key.m_startTrigger[sizeof(key.m_startTrigger) - 1] = 0;

			const char* pStopTrigegr = keyNode->getAttr("StopTrigger");
			cry_strcpy(key.m_stopTrigger, pStopTrigegr, sizeof(key.m_stopTrigger));
			key.m_stopTrigger[sizeof(key.m_stopTrigger) - 1] = 0;

			int32 durationTicks;
			if (!keyNode->getAttr("durationTicks", durationTicks))
			{
				// Backwards compat
				float duration;
				if (!keyNode->getAttr("Duration", duration))
				{
					duration = 0.0f;
				}
				key.m_duration = SAnimTime(duration);
			}
			else
			{
				key.m_duration = SAnimTime(durationTicks);
			}
		}
		else
		{
			keyNode->setAttr("StartTrigger", key.m_startTrigger);
			keyNode->setAttr("StopTrigger", key.m_stopTrigger);
			keyNode->setAttr("durationTicks", key.m_duration.GetTicks());
		}
	}
};

class CAudioFileTrack : public TAnimTrack<SAudioFileKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_AudioFile; }

	virtual void           SerializeKey(SAudioFileKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			const char* szAudioFile = keyNode->getAttr("file");
			cry_strcpy(key.m_audioFile, szAudioFile, sizeof(key.m_audioFile));
			key.m_audioFile[sizeof(key.m_audioFile) - 1] = 0;
		}
		else
		{
			keyNode->setAttr("file", key.m_audioFile);
		}
	}
};

class CDynamicResponseSignalTrack : public TAnimTrack<SDynamicResponseSignalKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_DynamicResponseSignal; }

	virtual void           SerializeKey(SDynamicResponseSignalKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			const char* pSignalName = keyNode->getAttr("signalName");
			cry_strcpy(key.m_signalName, pSignalName, sizeof(key.m_signalName));
			key.m_signalName[sizeof(key.m_signalName) - 1] = 0;

			const char* pContextVariableName = keyNode->getAttr("contextVariableName");
			cry_strcpy(key.m_contextVariableName, pContextVariableName, sizeof(key.m_contextVariableName));
			key.m_contextVariableName[sizeof(key.m_contextVariableName) - 1] = 0;

			const char* pContextVariableValue = keyNode->getAttr("contextVariableValue");
			cry_strcpy(key.m_contextVariableValue, pContextVariableValue, sizeof(key.m_contextVariableValue));
			key.m_contextVariableValue[sizeof(key.m_contextVariableValue) - 1] = 0;
		}
		else
		{
			keyNode->setAttr("signalName", key.m_signalName);
			keyNode->setAttr("contextVariableName", key.m_contextVariableName);
			keyNode->setAttr("contextVariableValue", key.m_contextVariableValue);
		}
	}
};

class CExprTrack : public TAnimTrack<SExprKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_Expression; }

	virtual void           SerializeKey(SExprKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			const char* desc;
			desc = keyNode->getAttr("name");
			cry_strcpy(key.m_name, desc, sizeof(key.m_name));
			key.m_name[sizeof(key.m_name) - 1] = 0;
			keyNode->getAttr("amp", key.m_amp);
			keyNode->getAttr("blendin", key.m_blendIn);
			keyNode->getAttr("hold", key.m_hold);
			keyNode->getAttr("blendout", key.m_blendOut);
		}
		else
		{
			keyNode->setAttr("name", key.m_name);
			keyNode->setAttr("amp", key.m_amp);
			keyNode->setAttr("blendin", key.m_blendIn);
			keyNode->setAttr("hold", key.m_hold);
			keyNode->setAttr("blendout", key.m_blendOut);
		}
	}
};

class CSequenceTrack : public TAnimTrack<SSequenceKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_Sequence; }

	virtual void           SerializeKey(SSequenceKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			keyNode->getAttr("GUIDLo", key.m_sequenceGUID.lopart);
			keyNode->getAttr("GUIDHi", key.m_sequenceGUID.hipart);

			if (key.m_sequenceGUID == CryGUID::Null())
			{
				const char* pSelection = keyNode->getAttr("node");
				if (pSelection)
				{
					IAnimSequence* pSequence = gEnv->pMovieSystem->FindSequence(pSelection);
					key.m_sequenceGUID = pSequence ? pSequence->GetGUID() : CryGUID::Null();
				}
			}

			key.m_boverrideTimes = false;
			keyNode->getAttr("overridetimes", key.m_boverrideTimes);
			keyNode->getAttr("speed", key.m_speed);

			if (key.m_speed <= 0.0f)
			{
				key.m_speed = 1.0f;
			}
		}
		else
		{
			keyNode->setAttr("GUIDLo", key.m_sequenceGUID.lopart);
			keyNode->setAttr("GUIDHi", key.m_sequenceGUID.hipart);
			keyNode->setAttr("overridetimes", key.m_boverrideTimes);
			keyNode->setAttr("speed", key.m_speed);
		}

		if (key.m_boverrideTimes)
		{
			key.m_startTime.Serialize(keyNode, bLoading, "startTimeTicks", "starttime");
			key.m_endTime.Serialize(keyNode, bLoading, "endTimeTicks", "endtime");
		}
	}
};

class CEventTrack : public TAnimTrack<SEventKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_Event; }

	virtual void           SerializeKey(SEventKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		key.m_duration.Serialize(keyNode, bLoading, "durationTicks", "length");

		if (bLoading)
		{
			key.m_event = keyNode->getAttr("event");
			key.m_eventValue = keyNode->getAttr("eventValue");
			key.m_animation = keyNode->getAttr("animation");
			keyNode->getAttr("value", key.m_value);
			keyNode->getAttr("noTriggerInScrubbing", key.m_bNoTriggerInScrubbing);
		}
		else
		{
			if (key.m_event.size() > 0)
			{
				keyNode->setAttr("event", key.m_event);
			}

			if (key.m_eventValue.size() > 0)
			{
				keyNode->setAttr("eventValue", key.m_eventValue);
			}

			if (key.m_animation.size() > 0)
			{
				keyNode->setAttr("anim", key.m_animation);
			}

			keyNode->setAttr("value", key.m_value);
			keyNode->setAttr("noTriggerInScrubbing", key.m_bNoTriggerInScrubbing);
		}
	}
};

class CTrackEventTrack : public TAnimTrack<STrackEventKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_TrackEvent; }

	virtual void           SerializeKey(STrackEventKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			key.m_event = keyNode->getAttr("event");
			key.m_eventValue = keyNode->getAttr("eventValue");
			keyNode->getAttr("noTriggerInScrubbing", key.m_bNoTriggerInScrubbing);
		}
		else
		{
			if (key.m_event.size() > 0)
			{
				keyNode->setAttr("event", key.m_event);
			}

			if (key.m_eventValue.size() > 0)
			{
				keyNode->setAttr("eventValue", key.m_eventValue);
			}

			keyNode->setAttr("noTriggerInScrubbing", key.m_bNoTriggerInScrubbing);
		}
	}
};

class CConsoleTrack : public TAnimTrack<SConsoleKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_Console; }

	virtual void           SerializeKey(SConsoleKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			const char* str;
			str = keyNode->getAttr("command");
			cry_strcpy(key.m_command, str, sizeof(key.m_command));
			key.m_command[sizeof(key.m_command) - 1] = 0;
		}
		else
		{
			if (strlen(key.m_command) > 0)
			{
				keyNode->setAttr("command", key.m_command);
			}
		}
	}
};

class CMannequinTrack : public TAnimTrack<SMannequinKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_Mannequin; }

	virtual void           SerializeKey(SMannequinKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		key.m_duration.Serialize(keyNode, bLoading, "durationTicks", "duration");

		if (bLoading)
		{
			const char* str;

			str = keyNode->getAttr("fragName");
			cry_strcpy(key.m_fragmentName, str, sizeof(key.m_fragmentName));
			key.m_fragmentName[sizeof(key.m_fragmentName) - 1] = 0;

			str = keyNode->getAttr("tags");
			cry_strcpy(key.m_tags, str, sizeof(key.m_tags));
			key.m_tags[sizeof(key.m_tags) - 1] = 0;

			key.m_priority = 0;
			keyNode->getAttr("priority", key.m_priority);
		}
		else
		{
			if (strlen(key.m_fragmentName) > 0)
			{
				keyNode->setAttr("fragName", key.m_fragmentName);
			}

			if (strlen(key.m_tags) > 0)
			{
				keyNode->setAttr("tags", key.m_tags);
			}

			if (key.m_priority > 0)
			{
				keyNode->setAttr("priority", key.m_priority);
			}
		}
	}
};

class CCaptureTrack : public TAnimTrack<SCaptureKey>
{
public:
	virtual CAnimParamType GetParameterType() const override { return eAnimParamType_Capture; }

	virtual void           SerializeKey(SCaptureKey& key, XmlNodeRef& keyNode, bool bLoading) override
	{
		if (bLoading)
		{
			const char* desc;
			keyNode->getAttr("frameRate", key.m_frameRate);
			keyNode->getAttr("timeStep", key.m_timeStep);
			desc = keyNode->getAttr("format");
			key.m_captureFormat = SCaptureFormatInfo::GetCaptureFormatByExtension(desc);

			desc = keyNode->getAttr("folder");
			cry_strcpy(key.m_folder, desc);
			keyNode->getAttr("once", key.m_bOnce);
			desc = keyNode->getAttr("prefix");

			if (desc)
			{
				cry_strcpy(key.m_prefix, desc);
			}

			keyNode->getAttr("bufferToCapture", reinterpret_cast<int&>(key.m_bufferToCapture));
		}
		else
		{
			keyNode->setAttr("frameRate", key.m_frameRate);
			keyNode->setAttr("timeStep", key.m_timeStep);
			keyNode->setAttr("format", SCaptureFormatInfo::GetCaptureFormatExtension(key.m_captureFormat));
			keyNode->setAttr("folder", key.m_folder);
			keyNode->setAttr("once", key.m_bOnce);
			keyNode->setAttr("prefix", key.m_prefix);
			keyNode->setAttr("bufferToCapture", key.m_bufferToCapture);
		}

		key.m_duration.Serialize(keyNode, bLoading, "durationTicks", "duration");
	}
};

#endif
