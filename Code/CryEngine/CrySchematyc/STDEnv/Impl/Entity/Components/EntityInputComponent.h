// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IActionMapManager.h>
#include <CryCore/Containers/VectorMap.h>
#include <CrySerialization/Forward.h>
#include <Schematyc/Component.h>
#include <Schematyc/Types/ResourceTypes.h>

#include <CryInput/IInput.h>


namespace Schematyc
{
// Forward declare interfaces.
struct IEnvRegistrar;

class CEntityInputComponent final : public CComponent, public IInputEventListener
{
public:
	// CComponent
	virtual bool Init() override;
	virtual void Shutdown() override;
	// ~CComponent

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent& event) override;
	// ~IInputEventListener

	static void ReflectType(CTypeDesc<CEntityInputComponent>& desc);
	static void Register(IEnvRegistrar& registrar);

	float GetKeyValue(EKeyId keyId);

private:

	static VectorMap<EKeyId, float> s_keyValue;  //as long as we don't differentiate between different input devices, one instance is enough

	struct SActionPressedSignal
	{
		static void ReflectType(CTypeDesc<SActionPressedSignal>& desc);

		EInputDeviceType deviceType = eIDT_Unknown;   //!< Device type from which the event originated.
		EKeyId           keyId = eKI_Unknown;         //!< Device-specific id corresponding to the event.
		int              deviceIndex = 0;             //!< Local index of this particular controller type.
	};

	struct SActionReleasedSignal
	{
		static void ReflectType(CTypeDesc<SActionReleasedSignal>& desc);

		EInputDeviceType deviceType = eIDT_Unknown;   //!< Device type from which the event originated.
		EKeyId           keyId = eKI_Unknown;         //!< Device-specific id corresponding to the event.
	};
};
} // Schematyc
