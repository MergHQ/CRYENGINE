// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

/**
 * @namespace CryAudio
 * @brief Most parent audio namespace used throughout the entire engine.
 */
namespace CryAudio
{
/**
 * @namespace CryAudio::Impl
 * @brief Sub-namespace of the CryAudio namespace used by audio middleware implementations.
 */
namespace Impl
{
struct SObject3DAttributes;

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding CryAudio::IListener object (e.g. a middleware-specific unique ID)
 */
struct IAudioListener
{
	/** @cond */
	virtual ~IAudioListener() = default;
	/** @endcond */

	/**
	 * Set the world position of the listener inside the audio middleware
	 * @param attributes - a struct containing the audio listener's transformation and velocity
	 * @return eRequestStatus_Success if the AudioListener's position has been successfully set, eRequestStatus_Failure otherwise
	 */
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) = 0;
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IAudioParameter
 * (e.g. a middleware-specific control ID or a parameter name to be passed to an API function)
 */
struct IParameter
{
	/** @cond */
	virtual ~IParameter() = default;
	/** @endcond */
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IAudioSwitchState
 * (e.g. a middleware-specific control ID or a switch and state names to be passed to an API function)
 */
struct IAudioSwitchState
{
	/** @cond */
	virtual ~IAudioSwitchState() = default;
	/** @endcond */
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IAudioEnvironment
 * (e.g. a middleware-specific bus ID or name to be passed to an API function)
 */
struct IAudioEnvironment
{
	/** @cond */
	virtual ~IAudioEnvironment() = default;
	/** @endcond */
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IAudioEvent
 * (e.g. a middleware-specific playingID of an active event/sound for a play event)
 */
struct IAudioEvent
{
	/** @cond */
	virtual ~IAudioEvent() = default;
	/** @endcond */

	/**
	 * Stop the playing event
	 * @return eRequestStatus_Success if the event is stopped, eRequestStatus_Failure otherwise
	 */
	virtual ERequestStatus Stop() = 0;
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IAudioTrigger
 * (e.g. a middleware-specific event ID or name, a sound-file name to be passed to an API function)
 */
struct IAudioTrigger
{
	/** @cond */
	virtual ~IAudioTrigger() = default;
	/** @endcond */

	/**
	 * Load the metadata and media needed by the audio middleware to execute this trigger
	 * Loading Triggers manually is only necessary if their data have not been loaded via PreloadRequests
	 * @return eRequestStatus_Success if the the data was successfully loaded, eRequestStatus_Failure otherwise
	 * @see Unload, LoadAsync, UnloadAsync
	 */
	virtual ERequestStatus Load() const = 0;

	/**
	 * Release the metadata and media needed by the audio middleware to execute this trigger
	 * Unloading Triggers manually is only necessary if their data are not managed via PreloadRequests.
	 * @return eRequestStatus_Success if the the data was successfully unloaded, eRequestStatus_Failure otherwise
	 * @see Load, LoadAsync, UnloadAsync
	 */
	virtual ERequestStatus Unload() const = 0;

	/**
	 * Load the metadata and media needed by the audio middleware to execute this trigger asynchronously.
	 * Loading Triggers manually is only necessary if their data have not been loaded via PreloadRequests.
	 * @param pAudioEvent - The callback called once the loading is done must report that this event is finished.
	 * @return eRequestStatus_Success if the the request was successfully sent to the audio middleware, eRequestStatus_Failure otherwise	 * @see Load, Unload, UnloadAsync
	 */
	virtual ERequestStatus LoadAsync(IAudioEvent* const pIAudioEvent) const = 0;

	/**
	 * Release the metadata and media needed by the audio middleware to execute this trigger asynchronously.
	 * Unloading Triggers manually is only necessary if their data have not been loaded via PreloadRequests.
	 * @param pAudioEvent - The callback called once the loading is done must report that this event is finished.
	 * @return eRequestStatus_Success if the the request was successfully sent to the audio middleware, eRequestStatus_Failure otherwise
	 * @see Load, Unload, LoadAsync
	 */
	virtual ERequestStatus UnloadAsync(IAudioEvent* const pIAudioEvent) const = 0;
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IAudioFileEntry.
 * (e.g. a middleware-specific bank ID if the AudioFileEntry represents a soundbank)
 */
struct IAudioFileEntry
{
	/** @cond */
	virtual ~IAudioFileEntry() = default;
	/** @endcond */
};

/**
 * This is a POD structure used to pass the information about a file preloaded into memory between
 * the CryAudioSystem and an AudioImpl
 * Note: This struct cannot define a constructor, it needs to be a POD!
 */
struct SAudioFileEntryInfo
{
	void*            pFileData;                // pointer to the memory location of the file's contents
	size_t           memoryBlockAlignment;     // memory alignment to be used for storing this file's contents in memory
	size_t           size;                     // file size
	char const*      szFileName;               // file name
	bool             bLocalized;               // is the file localized
	IAudioFileEntry* pImplData;                // pointer to the implementation-specific data needed for this AudioFileEntry
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IAudioStandaloneFile.
 * (e.g. middleware-specific custom data that is associated with the standalone file)
 */
struct IAudioStandaloneFile
{
	/** @cond */
	virtual ~IAudioStandaloneFile() = default;
	/** @endcond */
};

/**
 * An AudioImpl may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding audio object (e.g. a middleware-specific unique ID)
 */
struct IAudioObject
{
	/** @cond */
	virtual ~IAudioObject() = default;
	/** @endcond */

	/**
	 * Performs actions that need to be executed regularly on an AudioObject. Called with every tick of the audio thread.
	 * @return eRequestStatus_Success if the object has been successfully updated, eRequestStatus_Failure otherwise.
	 */
	virtual ERequestStatus Update() = 0;

	/**
	 * Set the 3D attributes of the audio object
	 * @param attributes   - a struct containing the audio object's transformation and velocity
	 * @return eRequestStatus_Success if the AudioObject's position has been successfully set, eRequestStatus_Failure otherwise
	 */
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) = 0;

	/**
	 * Set the provided value for the specified environment on the audio object
	 * @param pIAudioEnvironment - implementation-specific environment to set
	 * @param amount             - the fade value for the provided IAudioEnvironment, 0.0f means no effect at all, 1.0f corresponds to the full effect
	 * @return eRequestStatus_Success if the provided the value has been successfully set, eRequestStatus_Failure otherwise
	 */
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) = 0;

	/**
	 * Set the provided parameter to the specified value on the audio object
	 * @param pIAudioParameter - implementation-specific parameter to set
	 * @param value            - the value to set the parameter to
	 * @return eRequestStatus_Success if the provided value has been successfully set on the passed IAudioParameter, eRequestStatus_Failure otherwise
	 * @see ExecuteTrigger, SetSwitchState, SetEnvironment, SetListener3DAttributes
	 */
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioParameter, float const value) = 0;

	/**
	 * Set the provided state (on a switch) on the audio object
	 * @param pIAudioSwitchState - implementation-specific state to set
	 * @return eRequestStatus_Success if the provided IAudioSwitchState has been successfully set, eRequestStatus_Failure otherwise
	 * @see ExecuteTrigger, SetParameter, SetEnvironment, Set3DAttributes
	 */
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) = 0;

	/**
	 * Set the provided Obstruction and Occlusion values
	 * @param obstruction - the obstruction value to be set; it describes how much the direct sound path from the AudioObject to the Listener is obstructed
	 * @param occlusion   - the occlusion value to be set; it describes how much all sound paths (direct and indirect) are obstructed
	 * @return eRequestStatus_Success if the provided the values been successfully set, eRequestStatus_Failure otherwise
	 * @see SetEnvironment
	 */
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) = 0;

	/**
	 * Activate a trigger on this audio object
	 * @param pIAudioTrigger - implementation-specific trigger to activate
	 * @param pIAudioEvent   - implementation-specific event corresponding to this particular trigger activation
	 * @return eRequestStatus_Success if the IAudioTrigger has been successfully activated by the audio middleware, eRequestStatus_Failure otherwise
	 * @see SetParameter, SetSwitchState
	 */
	virtual ERequestStatus ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent) = 0;

	/**
	 * Stop all triggers currently active on the audio object
	 * @return eRequestStatus_Success if all of the triggers have been successfully stopped, eRequestStatus_Failure otherwise
	 * @see ExecuteTrigger
	 */
	virtual ERequestStatus StopAllTriggers() = 0;

	/**
	 * Play a stand alone audio file
	 * @param pIFile         - stand alone file to play
	 * @return eRequestStatus_Success if the file started playback, eRequestStatus_Failure otherwise.
	 * @see StopFile
	 */
	virtual ERequestStatus PlayFile(IAudioStandaloneFile* const pIFile) = 0;

	/**
	 * Stop currently playing standalone file
	 * @param pIFile - file to stop playing
	 * @return eRequestStatus_Success if the file stopped, is stopping or is not playing anymore, eRequestStatus_Failure otherwise.
	 * @see PlayFile
	 */
	virtual ERequestStatus StopFile(IAudioStandaloneFile* const pIFile) = 0;
};

/**
 * This is a POD structure used to pass the information about an AudioImpl's memory usage
 * Note: This struct cannot define a constructor, it needs to be a POD!
 */
struct SAudioImplMemoryInfo
{
	size_t totalMemory;              // total amount of memory used by the AudioImpl in bytes
	size_t secondaryPoolSize;        // total size in bytes of the Secondary Memory Pool used by an AudioImpl
	size_t secondaryPoolUsedSize;    // bytes allocated inside the Secondary Memory Pool used by an AudioImpl
	size_t secondaryPoolAllocations; // number of allocations performed in the Secondary Memory Pool used by an AudioImpl
	size_t poolUsedObjects;          // total number of active pool objects
	size_t poolConstructedObjects;   // total number of constructed pool objects
	size_t poolUsedMemory;           // memory used by the constructed objects
	size_t poolAllocatedMemory;      // total memory allocated by the pool allocator
};
}
}
