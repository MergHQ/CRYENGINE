// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding CryAudio::IListener object (e.g. a middleware-specific unique ID)
 */
struct IListener
{
	/** @cond */
	virtual ~IListener() = default;
	/** @endcond */

	/**
	 * Sets the listener's transformation.
	 * @param transformation - a class containing the listener's position and rotation
	 * @return void
	 */
	virtual void SetTransformation(CObjectTransformation const& transformation) = 0;
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IParameter
 * (e.g. a middleware-specific control ID or a parameter name to be passed to an API function)
 */
struct IParameter
{
	/** @cond */
	virtual ~IParameter() = default;
	/** @endcond */
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding ISwitchState
 * (e.g. a middleware-specific control ID or a switch and state names to be passed to an API function)
 */
struct ISwitchState
{
	/** @cond */
	virtual ~ISwitchState() = default;
	/** @endcond */
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IEnvironment
 * (e.g. a middleware-specific bus ID or name to be passed to an API function)
 */
struct IEnvironment
{
	/** @cond */
	virtual ~IEnvironment() = default;
	/** @endcond */
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IEvent
 * (e.g. a middleware-specific playingID of an active event/sound for a play event)
 */
struct IEvent
{
	/** @cond */
	virtual ~IEvent() = default;
	/** @endcond */

	/**
	 * Stop the playing event
	 * @return ERequestStatus::Success if the event is stopped, ERequestStatus::Failure otherwise
	 */
	virtual ERequestStatus Stop() = 0;
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding ITrigger
 * (e.g. a middleware-specific event ID or name, a sound-file name to be passed to an API function)
 */
struct ITrigger
{
	/** @cond */
	virtual ~ITrigger() = default;
	/** @endcond */

	/**
	 * Load the metadata and media needed by the audio middleware to execute this trigger
	 * Loading Triggers manually is only necessary if their data have not been loaded via PreloadRequests
	 * @return ERequestStatus::Success if the the data was successfully loaded, ERequestStatus::Failure otherwise
	 * @see Unload, LoadAsync, UnloadAsync
	 */
	virtual ERequestStatus Load() const = 0;

	/**
	 * Release the metadata and media needed by the audio middleware to execute this trigger
	 * Unloading Triggers manually is only necessary if their data are not managed via PreloadRequests.
	 * @return ERequestStatus::Success if the the data was successfully unloaded, ERequestStatus::Failure otherwise
	 * @see Load, LoadAsync, UnloadAsync
	 */
	virtual ERequestStatus Unload() const = 0;

	/**
	 * Load the metadata and media needed by the audio middleware to execute this trigger asynchronously.
	 * Loading Triggers manually is only necessary if their data have not been loaded via PreloadRequests.
	 * @param pIEvent - The callback called once the loading is done must report that this event is finished.
	 * @return ERequestStatus::Success if the the request was successfully sent to the audio middleware, ERequestStatus::Failure otherwise
	 * @see Load, Unload, UnloadAsync
	 */
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const = 0;

	/**
	 * Release the metadata and media needed by the audio middleware to execute this trigger asynchronously.
	 * Unloading Triggers manually is only necessary if their data have not been loaded via PreloadRequests.
	 * @param pIEvent - The callback called once the loading is done must report that this event is finished.
	 * @return ERequestStatus::Success if the the request was successfully sent to the audio middleware, ERequestStatus::Failure otherwise
	 * @see Load, Unload, LoadAsync
	 */
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const = 0;
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IFile.
 * (e.g. a middleware-specific bank ID if the AudioFileEntry represents a soundbank)
 */
struct IFile
{
	/** @cond */
	virtual ~IFile() = default;
	/** @endcond */
};

/**
 * This is a POD structure used to pass the information about a file preloaded into memory between
 * the CryAudioSystem and an implementation
 * Note: This struct cannot define a constructor, it needs to be a POD!
 */
struct SFileInfo
{
	void*       pFileData;            // pointer to the memory location of the file's contents
	size_t      memoryBlockAlignment; // memory alignment to be used for storing this file's contents in memory
	size_t      size;                 // file size
	char const* szFileName;           // file name
	char const* szFilePath;           // file path
	bool        bLocalized;           // is the file localized
	IFile*      pImplData;            // pointer to the implementation-specific data needed for this AudioFileEntry
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding IStandaloneFile.
 * (e.g. middleware-specific custom data that is associated with the standalone file)
 */
struct IStandaloneFile
{
	/** @cond */
	virtual ~IStandaloneFile() = default;
	/** @endcond */
};

/**
 * An implementation may use this interface to define a class for storing implementation-specific
 * data needed for identifying and using the corresponding object (e.g. a middleware-specific unique ID)
 */
struct IObject
{
	/** @cond */
	virtual ~IObject() = default;
	/** @endcond */

	/**
	 * Performs actions that need to be executed regularly on an AudioObject. Called with every tick of the audio thread.
	 * @return void
	 */
	virtual void Update() = 0;

	/**
	 * Set the the object's transformation.
	 * @param transformation - a class containing the object's position and rotation
	 * @return void
	 */
	virtual void SetTransformation(CObjectTransformation const& transformation) = 0;

	/**
	 * Set the provided value for the specified environment on the object.
	 * @param pIEnvironment - implementation-specific environment to set
	 * @param amount - the fade value for the provided IEnvironment, 0.0f means no effect at all, 1.0f corresponds to the full effect
	 * @return void
	 */
	virtual void SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) = 0;

	/**
	 * Set the provided parameter to the specified value on the object.
	 * @param pIParameter - implementation-specific parameter to set
	 * @param value - the value to set the parameter to
	 * @return void
	 */
	virtual void SetParameter(IParameter const* const pIParameter, float const value) = 0;

	/**
	 * Set the provided state (on a switch) on the object.
	 * @param pISwitchState - implementation-specific state to set
	 * @return void
	 */
	virtual void SetSwitchState(ISwitchState const* const pISwitchState) = 0;

	/**
	 * Set the provided obstruction and occlusion values.
	 * @param obstruction - the obstruction value to be set, it describes how much the direct sound path from the object to the listener is obstructed
	 * @param occlusion - the occlusion value to be set, it describes how much all sound paths (direct and indirect) are obstructed
	 * @return void
	 */
	virtual void SetObstructionOcclusion(float const obstruction, float const occlusion) = 0;

	/**
	 * Activate a trigger on this object.
	 * @param pITrigger - implementation-specific trigger to activate
	 * @param pIEvent - implementation-specific event corresponding to this particular trigger activation
	 * @return ERequestStatus - indicates the outcome of underlying process
	 */
	virtual ERequestStatus ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent) = 0;

	/**
	 * Stop all triggers currently active on the object.
	 * @return void
	 */
	virtual void StopAllTriggers() = 0;

	/**
	 * Play a stand alone file.
	 * @param pIStandaloneFile - file to play
	 * @return ERequestStatus - indicates the outcome of underlying process
	 * @see StopFile
	 */
	virtual ERequestStatus PlayFile(IStandaloneFile* const pIStandaloneFile) = 0;

	/**
	 * Stop a stand alone file.
	 * @param pIStandaloneFile - file to stop
	 * @return ERequestStatus - indicates the outcome of underlying process
	 * @see PlayFile
	 */
	virtual ERequestStatus StopFile(IStandaloneFile* const pIStandaloneFile) = 0;

	/**
	 * Sets this object's name.
	 * Is only used during production whenever an entity's name is changed to adjust corresponding objects as well.
	 * @param szName - name to set
	 * @return ERequestStatus - indicates the outcome of underlying process
	 */
	virtual ERequestStatus SetName(char const* const szName) = 0;
};

/**
 * This is a POD structure used to pass the information about an implementation's memory usage
 * Note: This struct cannot define a constructor, it needs to be a POD!
 */
struct SMemoryInfo
{
	size_t totalMemory;              // total amount of memory used by the implementation in bytes
	size_t secondaryPoolSize;        // total size in bytes of the Secondary Memory Pool used by an implementation
	size_t secondaryPoolUsedSize;    // bytes allocated inside the Secondary Memory Pool used by an implementation
	size_t secondaryPoolAllocations; // number of allocations performed in the Secondary Memory Pool used by an implementation
	size_t poolUsedObjects;          // total number of active pool objects
	size_t poolConstructedObjects;   // total number of constructed pool objects
	size_t poolUsedMemory;           // memory used by the constructed objects
	size_t poolAllocatedMemory;      // total memory allocated by the pool allocator
};
} // namespace Impl
} // namespace CryAudio
