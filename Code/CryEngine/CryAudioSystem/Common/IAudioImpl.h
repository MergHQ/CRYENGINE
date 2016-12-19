// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SharedAudioData.h"
#include "ATLEntityData.h"
#include <CrySystem/XML/IXml.h>

/**
 * @namespace CryAudio
 * @brief Most parent audio namespace used throughout the entire engine.
 */
namespace CryAudio
{
typedef uint8 TAudioGamepadUniqueID;

enum EAudioEventState : EnumFlagsType
{
	eAudioEventState_None           = 0,
	eAudioEventState_Playing        = 1,
	eAudioEventState_PlayingDelayed = 2,
	eAudioEventState_Loading        = 3,
	eAudioEventState_Unloading      = 4,
	eAudioEventState_Virtual        = 5,
};

inline ERequestStatus BoolToARS(bool bResult)
{
	return bResult ? eRequestStatus_Success : eRequestStatus_Failure;
}

/**
 * @namespace CryAudio::Impl
 * @brief Sub-namespace of the CryAudio namespace used by audio middleware implementations.
 */
namespace Impl
{
/**
 * @struct IAudioImpl
 * @brief interface that exposes audio functionality to an audio middleware implementation
 */
struct IAudioImpl
{
	/** @cond */
	virtual ~IAudioImpl() = default;
	/** @endcond */

	/**
	 * Update all of the internal components that require regular updates, update the audio middleware state.
	 * @param deltaTime - Time since the last call to Update in milliseconds.
	 * @return void
	 */
	virtual void Update(float const deltaTime) = 0;

	/**
	 * Initialize all internal components and the audio middleware.
	 * @param audioObjectPoolSize - Number of audio-object objects to preallocate storage for.
	 * @param eventPoolSize - Number of audio-event objects to preallocate storage for.
	 * @return eAudioRequestResult_Success if the initialization was successful, eAudioRequestResult_Failure otherwise.
	 * @see ShutDown
	 */
	virtual ERequestStatus Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize) = 0;

	/**
	 * Shuts down all of the internal components and the audio middleware.
	 * Note: After a call to ShutDown(), the system can still be reinitialized by calling Init().
	 * @return eAudioRequestResult_Success if the shutdown was successful, eAudioRequestResult_Failure otherwise.
	 * @see Release, Init
	 */
	virtual ERequestStatus ShutDown() = 0;

	/**
	 * Frees all of the resources used by the class and destroys the instance. This action is not reversible.
	 * @return eAudioRequestResult_Success if the action was successful, eAudioRequestResult_Failure otherwise.
	 * @see ShutDown, Init
	 */
	virtual ERequestStatus Release() = 0;

	/**
	 * Perform a "hot restart" of the audio middleware. Reset all of the internal data.
	 * @see Release, Init
	 */
	virtual void OnAudioSystemRefresh() = 0;

	/**
	 * This method is called every time the main Game (or Editor) window loses focus.
	 * @return eAudioRequestResult_Success if the action was successful, eAudioRequestResult_Failure otherwise.
	 * @see OnGetFocus
	 */
	virtual ERequestStatus OnLoseFocus() = 0;

	/**
	 * This method is called every time the main Game (or Editor) window gets focus.
	 * @return eAudioRequestResult_Success if the action was successful, eAudioRequestResult_Failure otherwise.
	 * @see OnLoseFocus
	 */
	virtual ERequestStatus OnGetFocus() = 0;

	/**
	 * Mute all sounds, after this call there should be no audio coming from the audio middleware.
	 * @return eAudioRequestResult_Success if the action was successful, eAudioRequestResult_Failure otherwise.
	 * @see UnmuteAll, StopAllSounds
	 */
	virtual ERequestStatus MuteAll() = 0;

	/**
	 * Restore the audio output of the audio middleware after a call to MuteAll().
	 * @return eAudioRequestResult_Success if the action was successful, eAudioRequestResult_Failure otherwise.
	 * @see MuteAll
	 */
	virtual ERequestStatus UnmuteAll() = 0;

	/**
	 * Stop all currently playing sounds. Has no effect on anything triggered after this method is called.
	 * @return eAudioRequestResult_Success if the action was successful, eAudioRequestResult_Failure otherwise.
	 * @see MuteAll
	 */
	virtual ERequestStatus StopAllSounds() = 0;

	/**
	 * Inform the audio middleware about the memory location of a preloaded audio-data file
	 * @param pAudioFileEntry - ATL-specific information describing the resources used by the preloaded file being reported
	 * @return eAudioRequestResult_Success if the audio middleware is able to use the preloaded file, eAudioRequestResult_Failure otherwise
	 * @see UnregisterInMemoryFile
	 */
	virtual ERequestStatus RegisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry) = 0;

	/**
	 * Inform the audio middleware that the memory containing the preloaded audio-data file should no longer be used
	 * @param pAudioFileEntry - ATL-specific information describing the resources used by the preloaded file being invalidated
	 * @return eAudioRequestResult_Success if the audio middleware was able to unregister the preloaded file supplied, eAudioRequestResult_Failure otherwise
	 * @see RegisterInMemoryFile
	 */
	virtual ERequestStatus UnregisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry) = 0;

	/**
	 * Parse the implementation-specific XML node that represents an AudioFileEntry, fill the fields of the struct
	 * referenced by pFileEntryInfo with the data necessary to correctly access and store the file's contents in memory.
	 * Create an object implementing IAudioFileEntry to hold implementation-specific data about the file and store a pointer to it in a member of pFileEntryInfo
	 * @param pAudioFileEntryNode - an XML node containing the necessary information about the file
	 * @param pFileEntryInfo      - a pointer to the struct containing the data used by the ATL to load the file into memory
	 * @return eAudioRequestResult_Success if the XML node was parsed successfully, eAudioRequestResult_Failure otherwise
	 */
	virtual ERequestStatus ParseAudioFileEntry(
	  XmlNodeRef const pAudioFileEntryNode,
	  SAudioFileEntryInfo* const pFileEntryInfo) = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioFileEntry instance.
	 * Normally, an IAudioFileEntry instance is created by ParseAudioFileEntry() and a pointer is stored in a member of SAudioFileEntryInfo.
	 * @param pIAudioFileEntry - pointer to the object implementing IAudioFileEntry to be discarded
	 * @see ParseAudioFileEntry
	 */
	virtual void DeleteAudioFileEntry(IAudioFileEntry* const pIAudioFileEntry) = 0;

	/**
	 * Get the full path to the folder containing the file described by the pFileEntryInfo
	 * @param pFileEntryInfo - ATL-specific information describing the file whose location is being queried
	 * @return A C-string containing the path to the folder where the file corresponding to the pFileEntryInfo is stored
	 */
	virtual char const* const GetAudioFileLocation(SAudioFileEntryInfo* const pFileEntryInfo) = 0;

	/**
	 * Parse the implementation-specific XML node that represents an ATLTriggerImpl, return a pointer to the data needed for identifying
	 * and using this ATLTriggerImpl instance inside the AudioImplementation
	 * @param pAudioTriggerNode - an XML node corresponding to the new ATLTriggerImpl to be created
	 * @param info - reference to a struct to be filled be the implementation with information about the trigger (eg. max attenuation radius)
	 * @return IAudioTrigger const* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding ATLTriggerImpl; NULL if the new AudioTriggerImplData instance was not created
	 * @see DeleteAudioTrigger
	 */
	virtual IAudioTrigger const* NewAudioTrigger(XmlNodeRef const pAudioTriggerNode) = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioTrigger instance
	 * @param pIAudioTrigger - pointer to the object implementing IAudioTrigger to be discarded
	 * @see NewAudioTrigger
	 */
	virtual void DeleteAudioTrigger(IAudioTrigger const* const pIAudioTrigger) = 0;

	/**
	 * Create an object implementing ConstructAudioStandaloneFile that stores all of the data needed by the AudioImplementation
	 * to identify and use an audio standalone file. Return a pointer to that object.
	 * @param atlStandaloneFile - reference to the CATLStandaloneFile associated with the IAudioStandaloneFile object we want to construct. It's used as an ID to link the two objects.
	 * @param szFile - full path to the file that wants to be played
	 * @param bLocalized - is the file specified in szFile localized or not
	 * @param pTrigger - if set, routes the playing of the audio file through the specified implementation trigger
	 * @return IAudioStandaloneFile pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding audio standalone file.
	 * @return NULL if the new IAudioStandaloneFile instance was not created.
	 * @see DestructAudioStandaloneFile
	 */
	virtual IAudioStandaloneFile* ConstructAudioStandaloneFile(CATLStandaloneFile& atlStandaloneFile, char const* const szFile, bool const bLocalized, IAudioTrigger const* pTrigger = nullptr) = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioStandaloneFile instance
	 * @param pAudioStandaloneFile - pointer to the object implementing IAudioStandaloneFile to be discarded
	 * @see ConstructAudioStandaloneFile
	 */
	virtual void DestructAudioStandaloneFile(IAudioStandaloneFile const* const pAudioStandaloneFile) = 0;

	/**
	 * Parse the implementation-specific XML node that represents an ATLRtpcImpl, return a pointer to the data needed for identifying
	 * and using this ATLRtpcImpl instance inside the AudioImplementation
	 * @param pAudioParameterNode - an XML node corresponding to the new ATLRtpcImpl to be created
	 * @return IAudioParameter const* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding ATLRtpcImpl; NULL if the new AudioTrigger instance was not created
	 * @see DeleteAudioParameter
	 */
	virtual IParameter const* NewAudioParameter(XmlNodeRef const pAudioParameterNode) = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioParameter instance
	 * @param pAudioParameter - pointer to the object implementing IAudioParameter to be discarded
	 * @see NewAudioParameter
	 */
	virtual void DeleteAudioParameter(IParameter const* const pIAudioParameter) = 0;

	/**
	 * Parse the implementation-specific XML node that represents an ATLSwitchStateImpl, return a pointer to the data needed for identifying
	 * and using this ATLSwitchStateImpl instance inside the AudioImplementation
	 * @param pAudioSwitchStateNode - an XML node corresponding to the new ATLSwitchStateImpl to be created
	 * @return IAudioSwitchState const* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding ATLSwitchStateImpl; NULL if the new AudioTriggerImplData instance was not created
	 * @see DeleteAudioSwitchState
	 */
	virtual IAudioSwitchState const* NewAudioSwitchState(XmlNodeRef const pAudioSwitchStateNode) = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioSwitchState instance
	 * @param pIAudioSwitchState - pointer to the object implementing IAudioSwitchState to be discarded
	 * @see NewAudioSwitchState
	 */
	virtual void DeleteAudioSwitchState(IAudioSwitchState const* const pIAudioSwitchState) = 0;

	/**
	 * Parse the implementation-specific XML node that represents an ATLEnvironmentImpl, return a pointer to the data needed for identifying
	 * and using this ATLEnvironmentImpl instance inside the AudioImplementation
	 * @param pAudioEnvironmentNode - an XML node corresponding to the new ATLEnvironmentImpl to be created
	 * @return IAudioEnvironment const* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding ATLEnvironmentImpl; NULL if the new IAudioEnvironment instance was not created
	 * @see DeleteAudioEnvironment
	 */
	virtual IAudioEnvironment const* NewAudioEnvironment(XmlNodeRef const pAudioEnvironmentNode) = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioEnvironment instance
	 * @param pIAudioEnvironment - pointer to the object implementing IAudioEnvironment to be discarded
	 * @see NewAudioEnvironment
	 */
	virtual void DeleteAudioEnvironment(IAudioEnvironment const* const pIAudioEnvironment) = 0;

	/**
	 * Create an object implementing IAudioObject that stores all of the data needed by the AudioImplementation
	 * to identify and use the GlobalAudioObject.
	 * @return IAudioObject* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding GlobalAudioObject; NULL if the new IAudioObject instance was not created
	 * @see DestructAudioObject
	 */
	virtual IAudioObject* ConstructGlobalAudioObject() = 0;

	/**
	 * Create an object implementing IAudioObject that stores all of the data needed by the AudioImplementation
	 * to identify and use the AudioObject. Return a pointer to that object.
	 * @return IAudioObject* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding GlobalAudioObject; NULL if the new IAudioObject instance was not created
	 * @see DestructAudioObject
	 */
	virtual IAudioObject* ConstructAudioObject(char const* const szAudioObjectName = "") = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioObject instance
	 * @param pIAudioObject - pointer to the object implementing IAudioObject to be discarded
	 * @see ConstructAudioObject, ConstructGlobalAudioObject
	 */
	virtual void DestructAudioObject(IAudioObject const* const pIAudioObject) = 0;

	/**
	 * Construct an object implementing IAudioListener that stores all of the data needed by the AudioImplementation
	 * to identify and use an AudioListener. Return a pointer to that object.
	 * @return IAudioListener* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding AudioListener; nullptr if the new IAudioListener instance was not created.
	 * @see DestructAudioListener
	 */
	virtual IAudioListener* ConstructAudioListener() = 0;

	/**
	 * Destruct the supplied IAudioListener instance.
	 * @param pIAudioListener - pointer to the object implementing IAudioListener to be discarded
	 * @see ConstructAudioListener
	 */
	virtual void DestructAudioListener(IAudioListener* const pIAudioListener) = 0;

	/**
	 * Create an object implementing IAudioEvent that stores all of the data needed by the AudioImplementation
	 * to identify and use an AudioEvent. Return a pointer to that object.
	 * @param audioEvent - ATL Audio-Event associated with the newly created AudioEvent
	 * @return IAudioEvent* pointer to the audio implementation-specific data needed by the audio middleware and the
	 * @return AudioImplementation code to use the corresponding AudioEvent; NULL if the new IAudioEvent instance was not created
	 * @see DestructAudioEvent
	 */
	virtual IAudioEvent* ConstructAudioEvent(CATLEvent& audioEvent) = 0;

	/**
	 * Free the memory and potentially other resources used by the supplied IAudioEvent instance
	 * @param pIAudioEvent - pointer to the object implementing IAudioEvent to be discarded
	 * @see ConstructAudioEvent
	 */
	virtual void DestructAudioEvent(IAudioEvent const* const pIAudioEvent) = 0;

	/**
	 * Called whenever a Gamepad gets connected.
	 * This is used by audio middleware that supports controller effects such as rumble.
	 * @param deviceUniqueID - unique device identifier
	 */
	virtual void GamepadConnected(TAudioGamepadUniqueID const deviceUniqueID) = 0;

	/**
	 * Called whenever a Gamepad gets disconnected.
	 * This is used by audio middleware that supports controller effects such as rumble.
	 * @param deviceUniqueID - unique device identifier
	 */
	virtual void GamepadDisconnected(TAudioGamepadUniqueID const deviceUniqueID) = 0;

	/**
	 * Informs the audio middleware that the localized sound banks and streamed files need to use
	 * a different language. NOTE: this function DOES NOT unload or reload the currently loaded audio files
	 * @param szLanguage - a C-string representing the CryEngine language
	 * @see GetAudioFileLocation
	 */
	virtual void SetLanguage(char const* const szLanguage) = 0;

	//////////////////////////////////////////////////////////////////////////
	// NOTE: The methods below are ONLY USED when INCLUDE_AUDIO_PRODUCTION_CODE is defined!
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Return a string describing the audio middleware used. This string is printed on
	 * the first line of the AudioDebug header shown on the screen whenever s_DrawAudioDebug is not 0
	 * @return A zero-terminated C-string with the description of the audio middleware used by this AudioImplementation.
	 */
	virtual char const* const GetImplementationNameString() const = 0;

	/**
	 * Fill in the memoryInfo describing the current memory usage of this AudioImplementation.
	 * This data gets displayed in the AudioDebug header shown on the screen whenever s_DrawAudioDebug is not 0
	 * @param memoryInfo - a reference to an instance of SAudioImplMemoryInfo
	 */
	virtual void GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const = 0;

	/**
	 * Asks the audio implementation to fill the audioFileData structure with data (eg. duration of track) relating to the
	 * standalone file referenced in szFilename.
	 * @param szFilename - filepath to the standalone file
	 * @param audioFileData - a reference to an instance of SAudioFileData
	 */
	virtual void GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const = 0;
};
} // namespace Impl
} // namespace CryAudio
