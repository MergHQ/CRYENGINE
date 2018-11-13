// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

struct IRenderAuxGeom;

namespace CryAudio
{
namespace Impl
{
struct IEnvironmentConnection;
struct IEvent;
struct IParameterConnection;
struct ISwitchStateConnection;
struct ITriggerConnection;
struct IStandaloneFileConnection;

enum class EObjectFunctionality : EnumFlagsType
{
	TrackAbsoluteVelocity,
	TrackRelativeVelocity,
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
	 * @param deltaTime - accumulated game frame time in seconds.
	 * @return void
	 */
	virtual void Update(float const deltaTime) = 0;

	/**
	 * Set the the object's transformation.
	 * @param transformation - a class containing the object's position and rotation
	 * @return void
	 */
	virtual void SetTransformation(CTransformation const& transformation) = 0;

	/**
	 * Gets the objects's transformation.
	 * @return CTransformation of the object.
	 */
	virtual CTransformation const& GetTransformation() const = 0;

	/**
	 * Set the provided value for the specified environment on the object.
	 * @param pIEnvironmentConnection - implementation-specific environment to set
	 * @param amount - the fade value for the provided IEnvironment, 0.0f means no effect at all, 1.0f corresponds to the full effect
	 * @return void
	 */
	virtual void SetEnvironment(IEnvironmentConnection const* const pIEnvironmentConnection, float const amount) = 0;

	/**
	 * Set the provided parameter to the specified value on the object.
	 * @param pIParameterConnection - implementation-specific parameter to set
	 * @param value - the value to set the parameter to
	 * @return void
	 */
	virtual void SetParameter(IParameterConnection const* const pIParameterConnection, float const value) = 0;

	/**
	 * Set the provided state (on a switch) on the object.
	 * @param pISwitchState - implementation-specific state to set
	 * @return void
	 */
	virtual void SetSwitchState(ISwitchStateConnection const* const pISwitchState) = 0;

	/**
	 * Set the provided occlusion value.
	 * @param occlusion - the occlusion value to be set, it describes how much all sound paths (direct and indirect) are obstructed
	 * @return void
	 */
	virtual void SetOcclusion(float const occlusion) = 0;

	/**
	 * Set the provided occlusion type.
	 * @param occlusionType - the occlusion type to be set
	 * @return void
	 */
	virtual void SetOcclusionType(EOcclusionType const occlusionType) = 0;

	/**
	 * Activate a trigger on this object.
	 * @param pITrigger - implementation-specific trigger to activate
	 * @param pIEvent - implementation-specific event corresponding to this particular trigger activation
	 * @return ERequestStatus - indicates the outcome of underlying process
	 */
	virtual ERequestStatus ExecuteTrigger(ITriggerConnection const* const pITrigger, IEvent* const pIEvent) = 0;

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
	virtual ERequestStatus PlayFile(IStandaloneFileConnection* const pIStandaloneFile) = 0;

	/**
	 * Stop a stand alone file.
	 * @param pIStandaloneFile - file to stop
	 * @return ERequestStatus - indicates the outcome of underlying process
	 * @see PlayFile
	 */
	virtual ERequestStatus StopFile(IStandaloneFileConnection* const pIStandaloneFile) = 0;

	/**
	 * Sets this object's name.
	 * Is only used during production whenever an entity's name is changed to adjust corresponding objects as well.
	 * @param szName - name to set
	 * @return ERequestStatus - indicates the outcome of underlying process
	 */
	virtual ERequestStatus SetName(char const* const szName) = 0;

	/**
	 * Enables and disables a certain functionality on the object.
	 * @param type - defines the type of functionality.
	 * @param enable - defines if the functionality should be enabled or disabled.
	 * @return void
	 */
	virtual void ToggleFunctionality(EObjectFunctionality const type, bool const enable) = 0;

	//////////////////////////////////////////////////////////////////////////
	// NOTE: The methods below are ONLY USED when INCLUDE_AUDIO_PRODUCTION_CODE is defined!
	//////////////////////////////////////////////////////////////////////////

	/**
	 * Informs the object that it can draw its debug information.
	 * @param[out] auxGeom - a reference to the IRenderAuxGeom that draws the debug info.
	 * @param[in] posX - x-axis position of the auxGeom.
	 * @param[in] posY - y-axis position of the auxGeom.
	 * @param[in] szTextFilter - current set text filter. Is nullptr if filtering is disabled.
	 * @return void
	 */
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) = 0;
};
} // namespace Impl
} // namespace CryAudio
