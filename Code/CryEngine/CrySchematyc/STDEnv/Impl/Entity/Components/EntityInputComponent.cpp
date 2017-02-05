// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityInputComponent.h"

#include <CrySerialization/Enum.h>
#include <Schematyc/Entity/EntityClasses.h>

#include "AutoRegister.h"
#include "STDModules.h"


SERIALIZATION_ENUM_BEGIN(EInputDeviceType, "DeviceType")
	SERIALIZATION_ENUM(EInputDeviceType::eIDT_Keyboard, "Keyboard", "Keyboard")
	SERIALIZATION_ENUM(EInputDeviceType::eIDT_Mouse, "Mouse", "Mouse")
	SERIALIZATION_ENUM(EInputDeviceType::eIDT_Joystick, "Joystick", "Joystick")
	SERIALIZATION_ENUM(EInputDeviceType::eIDT_EyeTracker, "EyeTracker", "EyeTracker")
	SERIALIZATION_ENUM(EInputDeviceType::eIDT_Gamepad, "Gamepad", "Gamepad")
	SERIALIZATION_ENUM(EInputDeviceType::eIDT_MotionController, "MotionController", "MotionController")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN(EKeyId, "KeyId")
	SERIALIZATION_ENUM(EKeyId::eKI_Escape, "Keyboard_Escape", "Keyboard_Escape")
	SERIALIZATION_ENUM(EKeyId::eKI_1, "Keyboard_1", "Keyboard_1")
	SERIALIZATION_ENUM(EKeyId::eKI_2, "Keyboard_2", "Keyboard_2")
	SERIALIZATION_ENUM(EKeyId::eKI_3, "Keyboard_3", "Keyboard_3")
	SERIALIZATION_ENUM(EKeyId::eKI_4, "Keyboard_4", "Keyboard_4")
	SERIALIZATION_ENUM(EKeyId::eKI_5, "Keyboard_5", "Keyboard_5")
	SERIALIZATION_ENUM(EKeyId::eKI_6, "Keyboard_6", "Keyboard_6")
	SERIALIZATION_ENUM(EKeyId::eKI_7, "Keyboard_7", "Keyboard_7")
	SERIALIZATION_ENUM(EKeyId::eKI_8, "Keyboard_8", "Keyboard_8")
	SERIALIZATION_ENUM(EKeyId::eKI_9, "Keyboard_9", "Keyboard_9")
	SERIALIZATION_ENUM(EKeyId::eKI_0, "Keyboard_0", "Keyboard_0")
	SERIALIZATION_ENUM(EKeyId::eKI_Minus, "Keyboard_Minus", "Keyboard_Minus")
	SERIALIZATION_ENUM(EKeyId::eKI_Equals, "Keyboard_Equals", "Keyboard_Equals")
	SERIALIZATION_ENUM(EKeyId::eKI_Backspace, "Keyboard_Backspace", "Keyboard_Backspace")
	SERIALIZATION_ENUM(EKeyId::eKI_Tab, "Keyboard_Tab", "Keyboard_Tab")
	SERIALIZATION_ENUM(EKeyId::eKI_Q, "Keyboard_Q", "Keyboard_Q")
	SERIALIZATION_ENUM(EKeyId::eKI_W, "Keyboard_W", "Keyboard_W")
	SERIALIZATION_ENUM(EKeyId::eKI_E, "Keyboard_E", "Keyboard_E")
	SERIALIZATION_ENUM(EKeyId::eKI_R, "Keyboard_R", "Keyboard_R")
	SERIALIZATION_ENUM(EKeyId::eKI_T, "Keyboard_T", "Keyboard_T")
	SERIALIZATION_ENUM(EKeyId::eKI_Y, "Keyboard_Y", "Keyboard_Y")
	SERIALIZATION_ENUM(EKeyId::eKI_U, "Keyboard_U", "Keyboard_U")
	SERIALIZATION_ENUM(EKeyId::eKI_I, "Keyboard_I", "Keyboard_I")
	SERIALIZATION_ENUM(EKeyId::eKI_O, "Keyboard_O", "Keyboard_O")
	SERIALIZATION_ENUM(EKeyId::eKI_P, "Keyboard_P", "Keyboard_P")
	SERIALIZATION_ENUM(EKeyId::eKI_LBracket, "Keyboard_LBracket", "Keyboard_LBracket")
	SERIALIZATION_ENUM(EKeyId::eKI_RBracket, "Keyboard_RBracket", "Keyboard_RBracket")
	SERIALIZATION_ENUM(EKeyId::eKI_Enter, "Keyboard_Enter", "Keyboard_Enter")
	SERIALIZATION_ENUM(EKeyId::eKI_LCtrl, "Keyboard_LCtrl", "Keyboard_LCtrl")
	SERIALIZATION_ENUM(EKeyId::eKI_A, "Keyboard_A", "Keyboard_A")
	SERIALIZATION_ENUM(EKeyId::eKI_S, "Keyboard_S", "Keyboard_S")
	SERIALIZATION_ENUM(EKeyId::eKI_D, "Keyboard_D", "Keyboard_D")
	SERIALIZATION_ENUM(EKeyId::eKI_F, "Keyboard_F", "Keyboard_F")
	SERIALIZATION_ENUM(EKeyId::eKI_G, "Keyboard_G", "Keyboard_G")
	SERIALIZATION_ENUM(EKeyId::eKI_H, "Keyboard_H", "Keyboard_H")
	SERIALIZATION_ENUM(EKeyId::eKI_J, "Keyboard_J", "Keyboard_J")
	SERIALIZATION_ENUM(EKeyId::eKI_K, "Keyboard_K", "Keyboard_K")
	SERIALIZATION_ENUM(EKeyId::eKI_L, "Keyboard_L", "Keyboard_L")
	SERIALIZATION_ENUM(EKeyId::eKI_Semicolon, "Keyboard_Semicolon", "Keyboard_Semicolon")
	SERIALIZATION_ENUM(EKeyId::eKI_Apostrophe, "Keyboard_Apostrophe", "Keyboard_Apostrophe")
	SERIALIZATION_ENUM(EKeyId::eKI_Tilde, "Keyboard_Tilde", "Keyboard_Tilde")
	SERIALIZATION_ENUM(EKeyId::eKI_LShift, "Keyboard_LShift", "Keyboard_LShift")
	SERIALIZATION_ENUM(EKeyId::eKI_Backslash, "Keyboard_Backslash", "Keyboard_Backslash")
	SERIALIZATION_ENUM(EKeyId::eKI_Z, "Keyboard_Z", "Keyboard_Z")
	SERIALIZATION_ENUM(EKeyId::eKI_X, "Keyboard_X", "Keyboard_X")
	SERIALIZATION_ENUM(EKeyId::eKI_C, "Keyboard_C", "Keyboard_C")
	SERIALIZATION_ENUM(EKeyId::eKI_V, "Keyboard_V", "Keyboard_V")
	SERIALIZATION_ENUM(EKeyId::eKI_B, "Keyboard_B", "Keyboard_B")
	SERIALIZATION_ENUM(EKeyId::eKI_N, "Keyboard_N", "Keyboard_N")
	SERIALIZATION_ENUM(EKeyId::eKI_M, "Keyboard_M", "Keyboard_M")
	SERIALIZATION_ENUM(EKeyId::eKI_Comma, "Keyboard_Comma", "Keyboard_Comma")
	SERIALIZATION_ENUM(EKeyId::eKI_Period, "Keyboard_Period", "Keyboard_Period")
	SERIALIZATION_ENUM(EKeyId::eKI_Slash, "Keyboard_Slash", "Keyboard_Slash")
	SERIALIZATION_ENUM(EKeyId::eKI_RShift, "Keyboard_RShift", "Keyboard_RShift")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_Multiply, "Keyboard_NP_Multiply", "Keyboard_NP_Multiply")
	SERIALIZATION_ENUM(EKeyId::eKI_LAlt, "Keyboard_LAlt", "Keyboard_LAlt")
	SERIALIZATION_ENUM(EKeyId::eKI_Space, "Keyboard_Space", "Keyboard_Space")
	SERIALIZATION_ENUM(EKeyId::eKI_CapsLock, "Keyboard_CapsLock", "Keyboard_CapsLock")
	SERIALIZATION_ENUM(EKeyId::eKI_F1, "Keyboard_F1", "Keyboard_F1")
	SERIALIZATION_ENUM(EKeyId::eKI_F2, "Keyboard_F2", "Keyboard_F2")
	SERIALIZATION_ENUM(EKeyId::eKI_F3, "Keyboard_F3", "Keyboard_F3")
	SERIALIZATION_ENUM(EKeyId::eKI_F4, "Keyboard_F4", "Keyboard_F4")
	SERIALIZATION_ENUM(EKeyId::eKI_F5, "Keyboard_F5", "Keyboard_F5")
	SERIALIZATION_ENUM(EKeyId::eKI_F6, "Keyboard_F6", "Keyboard_F6")
	SERIALIZATION_ENUM(EKeyId::eKI_F7, "Keyboard_F7", "Keyboard_F7")
	SERIALIZATION_ENUM(EKeyId::eKI_F8, "Keyboard_F8", "Keyboard_F8")
	SERIALIZATION_ENUM(EKeyId::eKI_F9, "Keyboard_F9", "Keyboard_F9")
	SERIALIZATION_ENUM(EKeyId::eKI_F10, "Keyboard_F10", "Keyboard_F10")
	SERIALIZATION_ENUM(EKeyId::eKI_NumLock, "Keyboard_NumLock", "Keyboard_NumLock")
	SERIALIZATION_ENUM(EKeyId::eKI_ScrollLock, "Keyboard_ScrollLock", "Keyboard_ScrollLock")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_7, "Keyboard_NP_7", "Keyboard_NP_7")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_8, "Keyboard_NP_8", "Keyboard_NP_8")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_9, "Keyboard_NP_9", "Keyboard_NP_9")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_Substract, "Keyboard_NP_Substract", "Keyboard_NP_Substract")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_4, "Keyboard_NP_4", "Keyboard_NP_4")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_5, "Keyboard_NP_5", "Keyboard_NP_5")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_6, "Keyboard_NP_6", "Keyboard_NP_6")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_Add, "Keyboard_NP_Add", "Keyboard_NP_Add")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_1, "Keyboard_NP_1", "Keyboard_NP_1")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_2, "Keyboard_NP_2", "Keyboard_NP_2")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_3, "Keyboard_NP_3", "Keyboard_NP_3")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_0, "Keyboard_NP_0", "Keyboard_NP_0")
	SERIALIZATION_ENUM(EKeyId::eKI_F11, "Keyboard_F11", "Keyboard_F11")
	SERIALIZATION_ENUM(EKeyId::eKI_F12, "Keyboard_F12", "Keyboard_F12")
	SERIALIZATION_ENUM(EKeyId::eKI_F13, "Keyboard_F13", "Keyboard_F13")
	SERIALIZATION_ENUM(EKeyId::eKI_F14, "Keyboard_F14", "Keyboard_F14")
	SERIALIZATION_ENUM(EKeyId::eKI_F15, "Keyboard_F15", "Keyboard_F15")
	SERIALIZATION_ENUM(EKeyId::eKI_Colon, "Keyboard_Colon", "Keyboard_Colon")
	SERIALIZATION_ENUM(EKeyId::eKI_Underline, "Keyboard_Underline", "Keyboard_Underline")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_Enter, "Keyboard_NP_Enter", "Keyboard_NP_Enter")
	SERIALIZATION_ENUM(EKeyId::eKI_RCtrl, "Keyboard_RCtrl", "Keyboard_RCtrl")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_Period, "Keyboard_NP_Period", "Keyboard_NP_Period")
	SERIALIZATION_ENUM(EKeyId::eKI_NP_Divide, "Keyboard_NP_Divide", "Keyboard_NP_Divide")
	SERIALIZATION_ENUM(EKeyId::eKI_Print, "Keyboard_Print", "Keyboard_Print")
	SERIALIZATION_ENUM(EKeyId::eKI_RAlt, "Keyboard_RAlt", "Keyboard_RAlt")
	SERIALIZATION_ENUM(EKeyId::eKI_Pause, "Keyboard_Pause", "Keyboard_Pause")
	SERIALIZATION_ENUM(EKeyId::eKI_Home, "Keyboard_Home", "Keyboard_Home")
	SERIALIZATION_ENUM(EKeyId::eKI_Up, "Keyboard_Up", "Keyboard_Up")
	SERIALIZATION_ENUM(EKeyId::eKI_PgUp, "Keyboard_PgUp", "Keyboard_PgUp")
	SERIALIZATION_ENUM(EKeyId::eKI_Left, "Keyboard_Left", "Keyboard_Left")
	SERIALIZATION_ENUM(EKeyId::eKI_Right, "Keyboard_Right", "Keyboard_Right")
	SERIALIZATION_ENUM(EKeyId::eKI_End, "Keyboard_End", "Keyboard_End")
	SERIALIZATION_ENUM(EKeyId::eKI_Down, "Keyboard_Down", "Keyboard_Down")
	SERIALIZATION_ENUM(EKeyId::eKI_PgDn, "Keyboard_PgDn", "Keyboard_PgDn")
	SERIALIZATION_ENUM(EKeyId::eKI_Insert, "Keyboard_Insert", "Keyboard_Insert")
	SERIALIZATION_ENUM(EKeyId::eKI_Delete, "Keyboard_Delete", "Keyboard_Delete")
	SERIALIZATION_ENUM(EKeyId::eKI_LWin, "Keyboard_LWin", "Keyboard_LWin")
	SERIALIZATION_ENUM(EKeyId::eKI_RWin, "Keyboard_RWin", "Keyboard_RWin")
	SERIALIZATION_ENUM(EKeyId::eKI_Apps, "Keyboard_Apps", "Keyboard_Apps")
	
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse1, "Mouse_Button_1", "Mouse_Button_1")
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse2, "Mouse_Button_2", "Mouse_Button_2")
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse3, "Mouse_Button_3", "Mouse_Button_3")
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse4, "Mouse_Button_4", "Mouse_Button_4")
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse5, "Mouse_Button_5", "Mouse_Button_5")
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse6, "Mouse_Button_6", "Mouse_Button_6")
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse7, "Mouse_Button_7", "Mouse_Button_7")
	SERIALIZATION_ENUM(EKeyId::eKI_Mouse8, "Mouse_Button_8", "Mouse_Button_8")
	SERIALIZATION_ENUM(EKeyId::eKI_MouseWheelUp, "MouseWheelUp", "Mouse_WheelUp")
	SERIALIZATION_ENUM(EKeyId::eKI_MouseWheelDown, "MouseWheelDown", "Mouse_WheelDown")
	SERIALIZATION_ENUM(EKeyId::eKI_MouseX, "MouseX", "Mouse_X-Axis_Change")
	SERIALIZATION_ENUM(EKeyId::eKI_MouseY, "MouseY", "Mouse_Y-Axis_Change")
	SERIALIZATION_ENUM(EKeyId::eKI_MouseZ, "MouseZ", "Mouse_Z-Axis_Change")
	SERIALIZATION_ENUM(EKeyId::eKI_MouseXAbsolute, "MouseXAbsolute", "Mouse_X-Axis_Absolute")
	SERIALIZATION_ENUM(EKeyId::eKI_MouseYAbsolute, "MouseYAbsolute", "Mouse_Y-Axis_Absolute")
	
	SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadUp, "Pad_DPad_Up", "Pad_DPad_Up")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadDown, "Pad_DPad_Down", "Pad_DPad_Down")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadLeft, "Pad_DPad_Left", "Pad_DPad_Left")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadRight, "Pad_DPad_Right", "Pad_DPad_Right")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_Start, "Pad_Start", "Pad_Start")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_Back, "Pad_Back", "Pad_Back")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbL, "Pad_LeftThumbPress", "Pad_LeftThumbPress")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbR, "Pad_RightThumbPress", "Pad_RightThumbPress")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ShoulderL, "Pad_LeftShoulder", "Pad_LeftShoulder")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ShoulderR, "Pad_RightShoulder", "Pad_RightShoulder")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_A, "Pad_Button_A", "Pad_Button_A")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_B, "Pad_Button_B", "Pad_Button_B")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_X, "Pad_Button_X", "Pad_Button_X")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_Y, "Pad_Button_Y", "Pad_Button_Y")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerL, "Pad_LeftTrigger", "Pad_LeftTrigger")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerR, "Pad_RightTrigger", "Pad_RightTrigger")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLX, "Pad_LeftThumb_X-Axis", "Pad_LeftThumb_X-Axis")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLY, "Pad_LeftThumb_Y-Axis", "Pad_LeftThumb_Y-Axis")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLUp, "Pad_ThumbLUp", "Pad_ThumbLUp")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLDown, "Pad_ThumbLDown", "Pad_ThumbLDown")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLLeft, "Pad_ThumbLLeft", "Pad_ThumbLLeft")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLRight, "Pad_ThumbLRight", "Pad_ThumbLRight")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRX, "Pad_RightThumb_X-Axis", "Pad_RightThumb_X-Axis")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRY, "Pad_RightThumb_Y-Axis", "Pad_RightThumb_Y-Axis")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRUp, "Pad_ThumbRUp", "Pad_ThumbRUp")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRDown, "Pad_ThumbRDown", "Pad_ThumbRDown")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRLeft, "Pad_ThumbRLeft", "Pad_ThumbRLeft")
	//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRRight, "Pad_ThumbRRight", "Pad_ThumbRRight")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerLBtn, "Pad_LeftTriggerBtn", "Pad_LeftTriggerBtn")
	SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerRBtn, "Pad_RightTriggerBtn", "Pad_RightTriggerBtn")

	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Options, "Orbis_Options", "Orbis_Options")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_L3, "Orbis_L3", "Orbis_L3")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_R3, "Orbis_R3", "Orbis_R3")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Up, "Orbis_Up", "Orbis_Up")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Right, "Orbis_Right", "Orbis_Right")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Down, "Orbis_Down", "Orbis_Down")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Left, "Orbis_Left", "Orbis_Left")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_L2, "Orbis_L2", "Orbis_L2")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_R2, "Orbis_R2", "Orbis_R2")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_L1, "Orbis_L1", "Orbis_L1")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_R1, "Orbis_R1", "Orbis_R1")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Triangle, "Orbis_Triangle", "Orbis_Triangle")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Circle, "Orbis_Circle", "Orbis_Circle")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Cross, "Orbis_Cross", "Orbis_Cross")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Square, "Orbis_Square", "Orbis_Square")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickLX, "Orbis_StickLX", "Orbis_StickLX")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickLY, "Orbis_StickLY", "Orbis_StickLY")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickRX, "Orbis_StickRX", "Orbis_StickRX")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickRY, "Orbis_StickRY", "Orbis_StickRY")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotX, "Orbis_RotX", "Orbis_RotX")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotY, "Orbis_RotY", "Orbis_RotY")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotZ, "Orbis_RotZ", "Orbis_RotZ")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotX_KeyL, "Orbis_RotX_KeyL", "Orbis_RotX_KeyL")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotX_KeyR, "Orbis_RotX_KeyR", "Orbis_RotX_KeyR")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotZ_KeyD, "Orbis_RotZ_KeyD", "Orbis_RotZ_KeyD")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotZ_KeyU, "Orbis_RotZ_KeyU", "Orbis_RotZ_KeyU")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_LeftTrigger, "Orbis_LeftTrigger", "Orbis_LeftTrigger")
	SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RightTrigger, "Orbis_RightTrigger", "Orbis_RightTrigger")

	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_A, "Motion_OculusTouch_A", "Motion_OculusTouch_A")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_B, "Motion_OculusTouch_B", "Motion_OculusTouch_B")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_X, "Motion_OculusTouch_X", "Motion_OculusTouch_X")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_Y, "Motion_OculusTouch_Y", "Motion_OculusTouch_Y")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_L3, "Motion_OculusTouch_L3", "Motion_OculusTouch_L3")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_R3, "Motion_OculusTouch_R3", "Motion_OculusTouch_R3")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_TriggerBtnL, "Motion_OculusTouch_TriggerBtnL", "Motion_OculusTouch_TriggerBtnL")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_TriggerBtnR, "Motion_OculusTouch_TriggerBtnR", "Motion_OculusTouch_TriggerBtnR")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_L1, "Motion_OculusTouch_L1", "Motion_OculusTouch_L1")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_R1, "Motion_OculusTouch_R1", "Motion_OculusTouch_R1")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_L2, "Motion_OculusTouch_L2", "Motion_OculusTouch_L2")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_R2, "Motion_OculusTouch_R2", "Motion_OculusTouch_R2")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_StickL_Y, "Motion_OculusTouch_StickL_Y", "Motion_OculusTouch_StickL_Y")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_StickR_Y, "Motion_OculusTouch_StickR_Y", "Motion_OculusTouch_StickR_Y")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_StickL_X, "Motion_OculusTouch_StickL_X", "Motion_OculusTouch_StickL_X")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_StickR_X, "Motion_OculusTouch_StickR_X", "Motion_OculusTouch_StickR_X")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_Gesture_ThumbUpL, "Motion_OculusTouch_Gesture_ThumbUpL", "Motion_OculusTouch_Gesture_ThumbUpL")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_Gesture_ThumbUpR, "Motion_OculusTouch_Gesture_ThumbUpR", "Motion_OculusTouch_Gesture_ThumbUpR")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_Gesture_IndexPointingL, "Motion_OculusTouch_Gesture_IndexPointingL", "Motion_OculusTouch_Gesture_IndexPointingL")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_Gesture_IndexPointingR, "Motion_OculusTouch_Gesture_IndexPointingR", "Motion_OculusTouch_Gesture_IndexPointingR")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_LastButtonIndex, "Motion_OculusTouch_LastButtonIndex", "Motion_OculusTouch_LastButtonIndex")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_FirstGestureIndex, "Motion_OculusTouch_FirstGestureIndex", "Motion_OculusTouch_FirstGestureIndex")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_LastGestureIndex, "Motion_OculusTouch_LastGestureIndex", "Motion_OculusTouch_LastGestureIndex")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_FirstTriggerIndex, "Motion_OculusTouch_FirstTriggerIndex", "Motion_OculusTouch_FirstTriggerIndex")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OculusTouch_LastTriggerIndex, "Motion_OculusTouch_LastTriggerIndex", "Motion_OculusTouch_LastTriggerIndex")

	SERIALIZATION_ENUM(EKeyId::eKI_EyeTracker_X, "EyeTracker_X", "EyeTracker_X")
	SERIALIZATION_ENUM(EKeyId::eKI_EyeTracker_Y, "EyeTracker_Y", "EyeTracker_Y")

	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_System, "Motion_OpenVR_System", "Motion_OpenVR_System")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_ApplicationMenu, "Motion_OpenVR_ApplicationMenu", "Motion_OpenVR_ApplicationMenu")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_Grip, "Motion_OpenVR_Grip", "Motion_OpenVR_Grip")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_TouchPad_X, "Motion_OpenVR_TouchPad_X", "Motion_OpenVR_TouchPad_X")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_TouchPad_Y, "Motion_OpenVR_TouchPad_Y", "Motion_OpenVR_TouchPad_Y")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_Trigger, "Motion_OpenVR_Trigger", "Motion_OpenVR_Trigger")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_TriggerBtn, "Motion_OpenVR_TriggerBtn", "Motion_OpenVR_TriggerBtn")
	SERIALIZATION_ENUM(EKeyId::eKI_Motion_OpenVR_TouchPadBtn, "Motion_OpenVR_TouchPadBtn", "Motion_OpenVR_TouchPadBtn")
SERIALIZATION_ENUM_END()

namespace Schematyc
{
	VectorMap<EKeyId, float> CEntityInputComponent::s_keyValue;

	void CEntityInputComponent::SActionPressedSignal::ReflectType(CTypeDesc<SActionPressedSignal>& desc)
	{
		desc.SetGUID("10A6D786-19A5-41A8-8065-EFECEE730558"_schematyc_guid);
		desc.SetLabel("ActionPressed");
		desc.SetDescription("Sent when input action is pressed.");
		desc.AddMember(&SActionPressedSignal::deviceType, 'type', "deviceType", "DeviceType", "Device type");
		desc.AddMember(&SActionPressedSignal::keyId, 'id', "keyId", "Key", "Key identifier");
		desc.AddMember(&SActionPressedSignal::deviceIndex, 'dev', "deviceIndex", "DeviceIndex", "Device index");
	}

	void CEntityInputComponent::SActionReleasedSignal::ReflectType(CTypeDesc<SActionReleasedSignal>& desc)
	{
		desc.SetGUID("744FFAA6-1B4B-4BDD-B171-C2B3334EA02D"_schematyc_guid);
		desc.SetLabel("ActionReleased");
		desc.SetDescription("Sent when input action is released.");
		desc.AddMember(&SActionReleasedSignal::deviceType, 'type', "deviceType", "DeviceType", "Device type");
		desc.AddMember(&SActionReleasedSignal::keyId, 'id', "Key", "keyId", "Key identifier");
	}

	void ReflectType(Schematyc::CTypeDesc<EInputDeviceType>& desc)
	{
		desc.SetGUID("FF5535D3-B4F2-416A-B9CF-B747371AD687"_schematyc_guid);
		desc.SetLabel("InputDeviceType");
		desc.SetDescription("Input device type");
		desc.SetFlags(ETypeFlags::Switchable);
		desc.SetDefaultValue(EInputDeviceType::eIDT_Gamepad);
	}

	void ReflectType(Schematyc::CTypeDesc<EKeyId>& desc)
	{
		desc.SetGUID("F113E161-96B2-4698-AAEF-F50AE7FD96C2"_schematyc_guid);
		desc.SetLabel("InputKeyId");
		desc.SetDescription("Input KeyId");
		desc.SetFlags(ETypeFlags::Switchable);
		desc.SetDefaultValue(EKeyId::eKI_XI_A);
	}

	void CEntityInputComponent::ReflectType(CTypeDesc<CEntityInputComponent>& desc)
	{
		desc.SetGUID("528CBD7D-D334-4653-9B68-9C2AB30B2861"_schematyc_guid);
		desc.SetLabel("Input");
		desc.SetDescription("Entity input component");
		desc.SetIcon("icons:Game/Game_Play.ico");
		desc.SetComponentFlags(EComponentFlags::Singleton);
	}

	//////////////////////////////////////////////////////////////////////////

	bool CEntityInputComponent::Init()
	{
		gEnv->pInput->AddEventListener(this);
		return true;
	}

	void CEntityInputComponent::Shutdown()
	{
		gEnv->pInput->RemoveEventListener(this);
	}

	bool CEntityInputComponent::OnInputEvent(const SInputEvent& event)
	{
		if (event.keyId >= eKI_SYS_Commit)
			return false;

		s_keyValue[event.keyId] = event.value;

		if (event.state == eIS_Pressed)
		{
			SActionPressedSignal temp;
			temp.deviceType = event.deviceType;
			temp.keyId = event.keyId;
			temp.deviceIndex = event.deviceIndex;
			CComponent::GetObject().ProcessSignal(temp);
		}
		else if (event.state == eIS_Released)
		{
			SActionReleasedSignal temp;
			temp.deviceType = event.deviceType;
			temp.keyId = event.keyId;
			CComponent::GetObject().ProcessSignal(temp);
		}
		return false;
	}

	void CEntityInputComponent::Register(IEnvRegistrar& registrar)
	{
		CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
		{
			CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntityInputComponent));

			// Types
			{
				componentScope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(EInputDeviceType));
				componentScope.Register(SCHEMATYC_MAKE_ENV_DATA_TYPE(EKeyId));
			}

			// Signals
			{
				componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SActionPressedSignal));
				componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SActionReleasedSignal));
			}

			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityInputComponent::GetKeyValue, "9108B96B-AEC2-4E6E-8211-FA65B31CBB82"_schematyc_guid, "GetKeyValue");
				pFunction->SetDescription("Returns the current value of the button");
				pFunction->BindInput(1, 'dis', "Key");
				pFunction->BindOutput(0, 'val', "Value");
				componentScope.Register(pFunction);
			}
		}
	}

	float CEntityInputComponent::GetKeyValue(EKeyId keyId)
	{
		return stl::find_in_map(s_keyValue, keyId, 0.0f);
	}


}  //namespace Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntityInputComponent::Register)
