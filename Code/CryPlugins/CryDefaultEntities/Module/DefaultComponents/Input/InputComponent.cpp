#include "StdAfx.h"
#include "InputComponent.h"

static void ReflectType(Schematyc::CTypeDesc<EActionInputDevice>& desc)
{
	desc.SetGUID("{91C5E76B-8DB1-409B-8326-DA99E2CFF6E9}"_cry_guid);
	desc.SetLabel("Input Device");
	desc.SetDescription("Represents a device that can be used for input");
	desc.SetDefaultValue(eAID_KeyboardMouse);
	desc.AddConstant(eAID_KeyboardMouse, "KBM", "Keyboard and Mouse");
	desc.AddConstant(eAID_PS4Pad, "PS4", "PS4 Controller");
	desc.AddConstant(eAID_XboxPad, "Xbox", "Xbox Controller");
}

static void ReflectType(Schematyc::CTypeDesc<EActionActivationMode>& desc)
{
	desc.SetGUID("{C16D3CD2-F2D0-4BB8-BE16-FBDF09A308E7}"_cry_guid);
	desc.SetLabel("Input Activation Mode");
	desc.SetDescription("Represents the type of input event");
	desc.SetDefaultValue(eAAM_OnPress);
	desc.AddConstant(eAAM_OnPress, "Press", "Pressed");
	desc.AddConstant(eAAM_OnRelease, "Release", "Released");
	desc.AddConstant(eAAM_OnHold, "Hold", "Held");
}

namespace Cry
{
namespace DefaultComponents
{
void ReflectType(Schematyc::CTypeDesc<CInputComponent::EKeyboardInputId>& desc)
{
	desc.SetGUID("{2FAB7843-AAF8-4573-ABC0-6190C5C5900A}"_cry_guid);
	desc.SetLabel("Key Id");
	desc.SetDescription("Input Key Identifier");
	desc.SetFlags(Schematyc::ETypeFlags::Switchable);
	desc.SetDefaultValue((CInputComponent::EKeyboardInputId)EKeyId::eKI_Enter);
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Escape, "Escape", "Escape");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_1, "1", "1");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_2, "2", "2");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_3, "3", "3");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_4, "4", "4");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_5, "5", "5");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_6, "6", "6");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_7, "7", "7");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_8, "8", "8");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_9, "9", "9");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_0, "0", "0");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Minus, "Minus", "Minus");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Equals, "Equals", "Equals");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Backspace, "Backspace", "Backspace");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Tab, "Tab", "Tab");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Q, "Q", "Q");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_W, "W", "W");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_E, "E", "E");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_R, "R", "R");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_T, "T", "T");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Y, "Y", "Y");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_U, "U", "U");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_I, "I", "I");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_O, "O", "O");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_P, "P", "P");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_LBracket, "LBracket", "Left Bracket");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_RBracket, "RBracket", "Right Bracket");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Enter, "Enter", "Enter");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_LCtrl, "LCtrl", "Left Ctrl");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_A, "A", "A");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_S, "S", "S");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_D, "D", "D");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F, "F", "F");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_G, "G", "G");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_H, "H", "H");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_J, "J", "J");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_K, "K", "K");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_L, "L", "L");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Semicolon, "Semicolon", "Semicolon");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Apostrophe, "Apostrophe", "Apostrophe");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Tilde, "Tilde", "Tilde");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_LShift, "LShift", "Left Shift");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Backslash, "Backslash", "Backslash");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Z, "Z", "Z");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_X, "X", "X");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_C, "C", "C");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_V, "V", "V");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_B, "B", "B");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_N, "N", "N");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_M, "M", "M");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Comma, "Comma", "Comma");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Period, "Period", "Period");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Slash, "Slash", "Slash");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_RShift, "RShift", "RShift");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_LAlt, "LAlt", "LAlt");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Space, "Space", "Space");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_CapsLock, "CapsLock", "CapsLock");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F1, "F1", "F1");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F2, "F2", "F2");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F3, "F3", "F3");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F4, "F4", "F4");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F5, "F5", "F5");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F6, "F6", "F6");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F7, "F7", "F7");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F8, "F8", "F8");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F9, "F9", "F9");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F10, "F10", "F10");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NumLock, "NumLock", "NumLock");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_ScrollLock, "ScrollLock", "ScrollLock");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_Multiply, "NP_Multiply", "Numpad Multiply");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_7, "NP_7", "Numpad 7");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_8, "NP_8", "Numpad 8");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_9, "NP_9", "Numpad 9");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_Substract, "NP_Substract", "Numpad Substract");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_4, "NP_4", "Numpad 4");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_5, "NP_5", "Numpad 5");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_6, "NP_6", "Numpad 6");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_Add, "NP_Add", "Numpad Add");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_1, "NP_1", "Numpad 1");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_2, "NP_2", "Numpad 2");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_3, "NP_3", "Numpad 3");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_0, "NP_0", "Numpad 0");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_Enter, "NP_Enter", "Numpad Enter");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_Period, "NP_Period", "Numpad Period");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_NP_Divide, "NP_Divide", "Numpad Divide");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F11, "F11", "F11");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F12, "F12", "F12");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F13, "F13", "F13");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F14, "F14", "F14");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_F15, "F15", "F15");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Colon, "Colon", "Colon");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Underline, "Underline", "Underline");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_RCtrl, "RCtrl", "Right Ctrl");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Print, "Print", "Print");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_RAlt, "RAlt", "Right Alt");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Pause, "Pause", "Pause");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Home, "Home", "Home");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Up, "Up", "Up Arrow");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_PgUp, "PgUp", "Page Up");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Left, "Left", "Left Arrow");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Right, "Right", "Right Arrow");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_End, "End", "End");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Down, "Down", "Down Arrow");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_PgDn, "PgDn", "Page Down");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Insert, "Insert", "Insert");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Delete, "Delete", "Delete");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_LWin, "LWin", "Left Windows Key");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_RWin, "RWin", "Right Windows Key");
	desc.AddConstant((CInputComponent::EKeyboardInputId)EKeyId::eKI_Apps, "Apps", "Apps");
}

void ReflectType(Schematyc::CTypeDesc<CInputComponent::EMouseInputId>& desc)
{
	desc.SetGUID("{B09F6A7D-6AFD-48F6-B589-A25AD326CD0E}"_cry_guid);
	desc.SetLabel("Key Id");
	desc.SetDescription("Input Key Identifier");
	desc.SetFlags(Schematyc::ETypeFlags::Switchable);
	desc.SetDefaultValue((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse1);
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse1, "Button_1", "Left Button");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse2, "Button_2", "Right Button");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse3, "Button_3", "Middle Button");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse4, "Button_4", "Button_4");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse5, "Button_5", "Button_5");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse6, "Button_6", "Button_6");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse7, "Button_7", "Button_7");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_Mouse8, "Button_8", "Button_8");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_MouseWheelUp, "WheelUp", "WheelUp");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_MouseWheelDown, "WheelDown", "WheelDown");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_MouseX, "X", "X");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_MouseY, "Y", "Y");
	desc.AddConstant((CInputComponent::EMouseInputId)EKeyId::eKI_MouseZ, "Z", "Z");
}

void ReflectType(Schematyc::CTypeDesc<CInputComponent::EXboxInputId>& desc)
{
	desc.SetGUID("{8C26C065-8A61-4B89-93FA-9C102C225BBA}"_cry_guid);
	desc.SetLabel("Key Id");
	desc.SetDescription("Input Key Identifier");
	desc.SetFlags(Schematyc::ETypeFlags::Switchable);
	desc.SetDefaultValue((CInputComponent::EXboxInputId)EKeyId::eKI_XI_X);
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_DPadUp, "DUp", "D-Pad Up");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_DPadDown, "DDown", "D-Pad Down");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_DPadLeft, "DLeft", "D-Pad Left");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_DPadRight, "DRight", "D-Pad Right");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_Start, "Start", "Start");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_Back, "Back", "Back");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbL, "LeftThumbPress", "Left Thumb Press");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbR, "RightThumbPress", "Right Thumb Press");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ShoulderL, "LeftShoulder", "Left Shoulder");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ShoulderR, "RightShoulder", "Right Shoulder");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_A, "Button_A", "A");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_B, "Button_B", "B");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_X, "Button_X", "X");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_Y, "Button_Y", "Y");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_TriggerL, "LeftTrigger", "Left Trigger");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_TriggerR, "RightTrigger", "Right Trigger");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbLX, "xi_thumblx", "Left Thumb X Axis");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbLY, "xi_thumbly", "Left Thumb Y Axis");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbLUp, "ThumbLUp", "ThumbLUp");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbLDown, "ThumbLDown", "ThumbLDown");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbLLeft, "ThumbLLeft", "ThumbLLeft");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbLRight, "ThumbLRight", "ThumbLRight");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbRX, "xi_thumbrx", "Right Thumb X Axis");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbRY, "xi_thumbry", "Right Thumb Y Axis");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbRUp, "ThumbRUp", "ThumbRUp");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbRDown, "ThumbRDown", "ThumbRDown");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbRLeft, "ThumbRLeft", "ThumbRLeft");
	//desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_ThumbRRight, "ThumbRRight", "ThumbRRight");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_TriggerLBtn, "LeftTriggerBtn", "Left Trigger Button");
	desc.AddConstant((CInputComponent::EXboxInputId)EKeyId::eKI_XI_TriggerRBtn, "RightTriggerBtn", "Right Trigger Button");
}

void ReflectType(Schematyc::CTypeDesc<CInputComponent::EPS4InputId>& desc)
{
	desc.SetGUID("{9AC5093B-737C-493E-8866-D116996EA6E4}"_cry_guid);
	desc.SetLabel("Key Id");
	desc.SetDescription("Input Key Identifier");
	desc.SetFlags(Schematyc::ETypeFlags::Switchable);
	desc.SetDefaultValue((CInputComponent::EPS4InputId)EKeyId::eKI_XI_X);
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Options, "Options", "Options");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_L3, "L3", "L3");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_R3, "R3", "R3");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Up, "Up", "D-Pad Up");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Right, "Right", "D-Pad Right");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Down, "Down", "D-Pad Down");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Left, "Left", "D-Pad Left");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_L2, "L2", "L2");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_R2, "R2", "R2");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_L1, "L1", "L1");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_R1, "R1", "R1");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Triangle, "Triangle", "Triangle");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Circle, "Circle", "Circle");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Cross, "Cross", "Cross");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_Square, "Square", "Square");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_StickLX, "StickLX", "Left Stick X");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_StickLY, "StickLY", "Left Stick Y");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_StickRX, "StickRX", "Right Stick X");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_StickRY, "StickRY", "Right Stick Y");
	//desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RotX, "RotX", "RotX");
	//desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RotY, "RotY", "RotY");
	//desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RotZ, "RotZ", "RotZ");
	//desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RotX_KeyL, "RotX_KeyL", "RotX_KeyL");
	//desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RotX_KeyR, "RotX_KeyR", "RotX_KeyR");
	//desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RotZ_KeyD, "RotZ_KeyD", "RotZ_KeyD");
	//desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RotZ_KeyU, "RotZ_KeyU", "RotZ_KeyU");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_LeftTrigger, "LeftTrigger", "Left Trigger");
	desc.AddConstant((CInputComponent::EPS4InputId)EKeyId::eKI_Orbis_RightTrigger, "RightTrigger", "Right Trigger");
}

struct SActionPressedSignal
{
	SActionPressedSignal() {}
	SActionPressedSignal(Schematyc::CSharedString actionName, float value)
		: m_actionName(actionName)
		, m_value(value) {}

	Schematyc::CSharedString m_actionName;
	float                    m_value;
};

struct SActionReleasedSignal
{
	SActionReleasedSignal() {}
	SActionReleasedSignal(Schematyc::CSharedString actionName, float value)
		: m_actionName(actionName)
		, m_value(value) {}

	Schematyc::CSharedString m_actionName;
	float                    m_value;
};

struct SActionChangedSignal
{
	SActionChangedSignal() {}
	SActionChangedSignal(Schematyc::CSharedString actionName, float value)
		: m_actionName(actionName)
		, m_value(value) {}

	Schematyc::CSharedString m_actionName;
	float                    m_value;
};

void CInputComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CInputComponent::RegisterSchematycAction, "{1AC261E2-D7E0-4F80-99D1-73D34D670F0C}"_cry_guid, "RegisterAction");
		pFunction->SetDescription("Registers an action that results in OnAction signal being sent when the input is modified, requires BindInput to connect to keys");
		pFunction->SetFlags({ Schematyc::EEnvFunctionFlags::Member, Schematyc::EEnvFunctionFlags::Construction });
		pFunction->BindInput(1, 'grou', "Group Name");
		pFunction->BindInput(2, 'name', "Name");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CInputComponent::BindKeyboardAction, "{0F22767B-6AA6-4EDF-92F0-7BCD10CAB00E}"_cry_guid, "BindKeyboardAction");
		pFunction->SetDescription("Binds a keyboard input to an action registered with RegisterAction");
		pFunction->SetFlags({ Schematyc::EEnvFunctionFlags::Member, Schematyc::EEnvFunctionFlags::Construction });
		pFunction->BindInput(1, 'grou', "Group Name");
		pFunction->BindInput(2, 'name', "Name");
		pFunction->BindInput(3, 'keyn', "Input Id");
		pFunction->BindInput(4, 'pres', "Receive Press Events");
		pFunction->BindInput(5, 'rele', "Receive Release Events");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CInputComponent::BindMouseAction, "{64199BB3-88A5-4404-BAEF-212BC0D0ACCC}"_cry_guid, "BindMouseAction");
		pFunction->SetDescription("Binds a mouse input to an action registered with RegisterAction");
		pFunction->SetFlags({ Schematyc::EEnvFunctionFlags::Member, Schematyc::EEnvFunctionFlags::Construction });
		pFunction->BindInput(1, 'grou', "Group Name");
		pFunction->BindInput(2, 'name', "Name");
		pFunction->BindInput(3, 'keyn', "Input Id");
		pFunction->BindInput(4, 'pres', "Receive Press Events");
		pFunction->BindInput(5, 'rele', "Receive Release Events");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CInputComponent::BindXboxAction, "{5D8DDAD6-AEFD-4A1A-8BA0-F27F7836AC05}"_cry_guid, "BindXboxAction");
		pFunction->SetDescription("Binds an Xbox input to an action registered with RegisterAction");
		pFunction->SetFlags({ Schematyc::EEnvFunctionFlags::Member, Schematyc::EEnvFunctionFlags::Construction });
		pFunction->BindInput(1, 'grou', "Group Name");
		pFunction->BindInput(2, 'name', "Name");
		pFunction->BindInput(3, 'keyn', "Input Id");
		pFunction->BindInput(4, 'pres', "Receive Press Events");
		pFunction->BindInput(5, 'rele', "Receive Release Events");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CInputComponent::BindPS4Action, "{D7CE1BF1-F192-4E0D-B3D6-305B31E8C693}"_cry_guid, "BindPS4Action");
		pFunction->SetDescription("Binds a PlayStation input to an action registered with RegisterAction");
		pFunction->SetFlags({ Schematyc::EEnvFunctionFlags::Member, Schematyc::EEnvFunctionFlags::Construction });
		pFunction->BindInput(1, 'grou', "Group Name");
		pFunction->BindInput(2, 'name', "Name");
		pFunction->BindInput(3, 'keyn', "Input Id");
		pFunction->BindInput(4, 'pres', "Receive Press Events");
		pFunction->BindInput(5, 'rele', "Receive Release Events");
		componentScope.Register(pFunction);
	}

	// Signals
	{
		componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SActionPressedSignal));
	}
	{
		componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SActionReleasedSignal));
	}
	{
		componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SActionChangedSignal));
	}
}

void CInputComponent::ReflectType(Schematyc::CTypeDesc<CInputComponent>& desc)
{
	desc.SetGUID(CInputComponent::IID());
	desc.SetEditorCategory("Input");
	desc.SetLabel("Input");
	desc.SetDescription("Exposes support for inputs and action maps");
	//desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::HideFromInspector });
}

static void ReflectType(Schematyc::CTypeDesc<SActionPressedSignal>& desc)
{
	desc.SetGUID("{603771B5-6321-4113-82FB-B839F1BE7F64}"_cry_guid);
	desc.SetLabel("On Action Pressed");
	desc.AddMember(&SActionPressedSignal::m_actionName, 'actn', "Action", "Action Name", "Name of the triggered action", "");
	desc.AddMember(&SActionPressedSignal::m_value, 'valu', "value", "Value", "Value, if any, for example the joystick analog value", 0.0f);
}

static void ReflectType(Schematyc::CTypeDesc<SActionReleasedSignal>& desc)
{
	desc.SetGUID("{CB22A659-40AD-4CF0-AD07-42DCCEA24F44}"_cry_guid);
	desc.SetLabel("On Action Released");
	desc.AddMember(&SActionReleasedSignal::m_actionName, 'actn', "Action", "Action Name", "Name of the triggered action", "");
	desc.AddMember(&SActionReleasedSignal::m_value, 'valu', "value", "Value", "Value, if any, for example the joystick analog value", 0.0f);
}

static void ReflectType(Schematyc::CTypeDesc<SActionChangedSignal>& desc)
{
	desc.SetGUID("{ECDF0D8F-7E4B-4FA1-AEFA-51DAFD382CE9}"_cry_guid);
	desc.SetLabel("On Action Changed");
	desc.AddMember(&SActionChangedSignal::m_actionName, 'actn', "Action", "Action Name", "Name of the triggered action", "");
	desc.AddMember(&SActionChangedSignal::m_value, 'valu', "value", "Value", "Value, if any, for example the joystick analog value", 0.0f);
}

void CInputComponent::Initialize()
{
	IActionMapManager *pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
	
	pActionMapManager->ClearInputDevicesMappings();

	pActionMapManager->AddInputDeviceMapping(eAID_KeyboardMouse, "keyboard");
	pActionMapManager->AddInputDeviceMapping(eAID_XboxPad, "xboxpad");
	pActionMapManager->AddInputDeviceMapping(eAID_PS4Pad, "ps4pad");
	pActionMapManager->AddInputDeviceMapping(eAID_OculusTouch, "oculustouch");

	pActionMapManager->SetVersion(0);
	pActionMapManager->Enable(true);
}

void CInputComponent::RegisterSchematycAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name)
{
	CRY_ASSERT(m_pEntity->GetSchematycObject() != nullptr);

	RegisterAction(groupName.c_str(), name.c_str(), [this, name](int activationMode, float value)
			{
				if ((activationMode & eAAM_OnPress) != 0)
				{
				  m_pEntity->GetSchematycObject()->ProcessSignal(SActionPressedSignal(name, value), GetGUID());
				}
				if ((activationMode & eAAM_OnRelease) != 0)
				{
				  m_pEntity->GetSchematycObject()->ProcessSignal(SActionReleasedSignal(name, value), GetGUID());
				}
				if ((activationMode & eAAM_OnHold) != 0 || (activationMode & eAAM_Always) != 0)
				{
				  m_pEntity->GetSchematycObject()->ProcessSignal(SActionChangedSignal(name, value), GetGUID());
				}
	});
}

template <typename TEnum = EKeyId>
void InternalBindAction(EntityId id, Schematyc::CSharedString groupName, Schematyc::CSharedString name, EActionInputDevice inputDevice, TEnum keyId, bool bOnPress = true, bool bOnRelease = true)
{
	IActionMapManager *pActionMapManager = gEnv->pGameFramework->GetIActionMapManager();
	IActionMap* pActionMap = pActionMapManager->GetActionMap(groupName.c_str());
	if (pActionMap == nullptr)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Tried to bind input component action %s in group %s without registering it first!", name.c_str(), groupName.c_str());
		return;
	}

	IActionMapAction* pAction = pActionMap->GetAction(ActionId(name.c_str()));
	if (pAction == nullptr)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Tried to bind input component action %s in group %s without registering it first!", name.c_str(), groupName.c_str());
		return;
	}

	SActionInput input;
	input.input = input.defaultInput = Serialization::getEnumName<TEnum>(keyId);
	input.inputDevice = inputDevice;

	if (bOnPress)
	{
		input.activationMode |= eAAM_OnPress;
	}
	if (bOnRelease)
	{
		input.activationMode |= eAAM_OnRelease;
	}

	pActionMap->AddAndBindActionInput(ActionId(name.c_str()), input);

	pActionMapManager->EnableActionMap(groupName.c_str(), true);

	if (IActionMap *pActionMap = pActionMapManager->GetActionMap(groupName.c_str()))
	{
		pActionMap->SetActionListener(id);
	}
}

void CInputComponent::BindKeyboardAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EKeyboardInputId keyId, bool bOnPress, bool bOnRelease)
{
	InternalBindAction(GetEntityId(), groupName, name, EActionInputDevice::eAID_KeyboardMouse, keyId, bOnPress, bOnRelease);
}

void CInputComponent::BindMouseAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EMouseInputId keyId, bool bOnPress, bool bOnRelease)
{
	InternalBindAction(GetEntityId(), groupName, name, EActionInputDevice::eAID_KeyboardMouse, keyId, bOnPress, bOnRelease);
}

void CInputComponent::BindXboxAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EXboxInputId keyId, bool bOnPress, bool bOnRelease)
{
	InternalBindAction(GetEntityId(), groupName, name, EActionInputDevice::eAID_XboxPad, keyId, bOnPress, bOnRelease);
}

void CInputComponent::BindPS4Action(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EPS4InputId keyId, bool bOnPress, bool bOnRelease)
{
	InternalBindAction(GetEntityId(), groupName, name, EActionInputDevice::eAID_PS4Pad, keyId, bOnPress, bOnRelease);
}

void CInputComponent::BindAction(Schematyc::CSharedString groupName, Schematyc::CSharedString name, EActionInputDevice device, EKeyId keyId, bool bOnPress, bool bOnRelease)
{
	InternalBindAction(GetEntityId(), groupName, name, device, keyId, bOnPress, bOnRelease);
}
}
}