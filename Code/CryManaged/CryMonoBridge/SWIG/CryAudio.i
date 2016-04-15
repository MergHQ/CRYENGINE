%include "CryEngine.swig"

%import "CryCommon.i"

%{
#include <CryAudio/IAudioSystem.h>
%}

%typemap(csbase) EAudioRequestType "uint"
%include "../../../../CryEngine/CryCommon/CryAudio/IAudioInterfacesCommonData.h"
%feature("director") IAudioListener;
%include "../../../../CryEngine/CryCommon/CryAudio/IAudioSystem.h"

%extend IAudioSystem {
public:
	unsigned int GetAudioTriggerId(const char* audioTriggerName)
	{
		AudioControlId audioControlId;
		if ($self->GetAudioTriggerId(audioTriggerName, audioControlId))
			return (unsigned int)audioControlId;
		else
			return 0;
	}
	
	void PlayAudioTriggerId(unsigned int audioTriggerId)
	{
		SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> executeRequestData;
		executeRequestData.audioTriggerId = (AudioControlId)audioTriggerId;
		SAudioRequest request;
		request.pOwner = $self;
		request.pData  = &executeRequestData;
		gEnv->pAudioSystem->PushRequest(request);
	}
	
	void StopAudioTriggerId(unsigned int audioTriggerId)
	{
		SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> stopRequestData;
		stopRequestData.audioTriggerId = (AudioControlId)audioTriggerId;
		SAudioRequest request;
		request.pData  = &stopRequestData;
		gEnv->pAudioSystem->PushRequest(request);
	}
}
