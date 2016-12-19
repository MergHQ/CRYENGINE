%include "CryEngine.swig"

%import "CryCommon.i"

%{
#include <CryAudio/IAudioSystem.h>
%}

%typemap(csbase) CryAudio::ERequestFlags "uint"
%typemap(csbase) CryAudio::ERequestStatus "uint"
%typemap(csbase) CryAudio::ERequestResult "uint"
%typemap(csbase) CryAudio::ESystemEvents "uint"
%typemap(csbase) CryAudio::EDataScope "uint"
%typemap(csbase) CryAudio::EOcclusionType "uint"
%include "../../../../CryEngine/CryCommon/CryAudio/IAudioInterfacesCommonData.h"
%feature("director") IAudioListener;
%include "../../../../CryEngine/CryCommon/CryAudio/IAudioSystem.h"

%extend CryAudio::IAudioSystem {
public:
	int unsigned GetAudioTriggerId(char const* const szTriggerName)
	{
		CryAudio::ControlId triggerId;
		if ($self->GetAudioTriggerId(szTriggerName, triggerId))
			return static_cast<int unsigned>(triggerId);
		else
			return 0;
	}
	
	void PlayAudioTriggerId(int unsigned const triggerId)
	{
		gEnv->pAudioSystem->ExecuteTrigger(static_cast<CryAudio::ControlId const>(triggerId));
	}
	
	void StopAudioTriggerId(int unsigned const triggerId)
	{
		gEnv->pAudioSystem->StopTrigger(static_cast<CryAudio::ControlId const>(triggerId));
	}
}
