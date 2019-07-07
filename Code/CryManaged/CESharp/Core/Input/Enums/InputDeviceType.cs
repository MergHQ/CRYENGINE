// Copyright 2001-2019 Crytek GmbH / CrytekGroup. All rights reserved.

namespace CryEngine
{
	/// <summary>
	/// The various input devices from which input can be received.
	/// </summary>
	public enum InputDeviceType : byte
	{
		/// <summary>
		/// Generic keyboard device.
		/// </summary>
		Keyboard = CryEngine.Common.EInputDeviceType.eIDT_Keyboard,
		/// <summary>
		/// Generic mouse device.
		/// </summary>
		Mouse = CryEngine.Common.EInputDeviceType.eIDT_Mouse,
		/// <summary>
		/// Generic joystick device.
		/// </summary>
		Joystick = CryEngine.Common.EInputDeviceType.eIDT_Joystick,
		/// <summary>
		/// An eye-tracking device.
		/// </summary>
		EyeTracker = CryEngine.Common.EInputDeviceType.eIDT_EyeTracker,
		/// <summary>
		/// Generic gamepad device.
		/// </summary>
		Gamepad = CryEngine.Common.EInputDeviceType.eIDT_Gamepad,
		/// <summary>
		/// Generic motion-controller device.
		/// </summary>
		MotionController = CryEngine.Common.EInputDeviceType.eIDT_MotionController,
		/// <summary>
		/// Device is unknown.
		/// </summary>
		Unknown = CryEngine.Common.EInputDeviceType.eIDT_Unknown
	}
}
