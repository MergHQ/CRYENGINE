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

void ReflectType(Schematyc::CTypeDesc<EKeyId>& desc)
{
	desc.SetGUID("{2FAB7843-AAF8-4573-ABC0-6190C5C5900A}"_cry_guid);
	desc.SetLabel("Input Key Id");
	desc.SetDescription("Input Key Identifier");
	desc.SetFlags(Schematyc::ETypeFlags::Switchable);
	desc.SetDefaultValue(EKeyId::eKI_Mouse1);
}

SERIALIZATION_ENUM_BEGIN(EKeyId, "KeyId")
SERIALIZATION_ENUM(EKeyId::eKI_Escape, "escape", "Keyboard_Escape")
SERIALIZATION_ENUM(EKeyId::eKI_1, "1", "Keyboard_1")
SERIALIZATION_ENUM(EKeyId::eKI_2, "2", "Keyboard_2")
SERIALIZATION_ENUM(EKeyId::eKI_3, "3", "Keyboard_3")
SERIALIZATION_ENUM(EKeyId::eKI_4, "4", "Keyboard_4")
SERIALIZATION_ENUM(EKeyId::eKI_5, "5", "Keyboard_5")
SERIALIZATION_ENUM(EKeyId::eKI_6, "6", "Keyboard_6")
SERIALIZATION_ENUM(EKeyId::eKI_7, "7", "Keyboard_7")
SERIALIZATION_ENUM(EKeyId::eKI_8, "8", "Keyboard_8")
SERIALIZATION_ENUM(EKeyId::eKI_9, "9", "Keyboard_9")
SERIALIZATION_ENUM(EKeyId::eKI_0, "0", "Keyboard_0")
SERIALIZATION_ENUM(EKeyId::eKI_Minus, "minus", "Keyboard_Minus")
SERIALIZATION_ENUM(EKeyId::eKI_Equals, "equals", "Keyboard_Equals")
SERIALIZATION_ENUM(EKeyId::eKI_Backspace, "backspace", "Keyboard_Backspace")
SERIALIZATION_ENUM(EKeyId::eKI_Tab, "tab", "Keyboard_Tab")
SERIALIZATION_ENUM(EKeyId::eKI_Q, "q", "Keyboard_Q")
SERIALIZATION_ENUM(EKeyId::eKI_W, "w", "Keyboard_W")
SERIALIZATION_ENUM(EKeyId::eKI_E, "e", "Keyboard_E")
SERIALIZATION_ENUM(EKeyId::eKI_R, "r", "Keyboard_R")
SERIALIZATION_ENUM(EKeyId::eKI_T, "t", "Keyboard_T")
SERIALIZATION_ENUM(EKeyId::eKI_Y, "y", "Keyboard_Y")
SERIALIZATION_ENUM(EKeyId::eKI_U, "u", "Keyboard_U")
SERIALIZATION_ENUM(EKeyId::eKI_I, "i", "Keyboard_I")
SERIALIZATION_ENUM(EKeyId::eKI_O, "o", "Keyboard_O")
SERIALIZATION_ENUM(EKeyId::eKI_P, "p", "Keyboard_P")
SERIALIZATION_ENUM(EKeyId::eKI_LBracket, "lbracket", "Keyboard_LBracket")
SERIALIZATION_ENUM(EKeyId::eKI_RBracket, "rbracket", "Keyboard_RBracket")
SERIALIZATION_ENUM(EKeyId::eKI_Enter, "enter", "Keyboard_Enter")
SERIALIZATION_ENUM(EKeyId::eKI_LCtrl, "lctrl", "Keyboard_LCtrl")
SERIALIZATION_ENUM(EKeyId::eKI_A, "a", "Keyboard_A")
SERIALIZATION_ENUM(EKeyId::eKI_S, "s", "Keyboard_S")
SERIALIZATION_ENUM(EKeyId::eKI_D, "d", "Keyboard_D")
SERIALIZATION_ENUM(EKeyId::eKI_F, "f", "Keyboard_F")
SERIALIZATION_ENUM(EKeyId::eKI_G, "g", "Keyboard_G")
SERIALIZATION_ENUM(EKeyId::eKI_H, "h", "Keyboard_H")
SERIALIZATION_ENUM(EKeyId::eKI_J, "j", "Keyboard_J")
SERIALIZATION_ENUM(EKeyId::eKI_K, "k", "Keyboard_K")
SERIALIZATION_ENUM(EKeyId::eKI_L, "l", "Keyboard_L")
SERIALIZATION_ENUM(EKeyId::eKI_Semicolon, "semicolon", "Keyboard_Semicolon")
SERIALIZATION_ENUM(EKeyId::eKI_Apostrophe, "apostrophe", "Keyboard_Apostrophe")
SERIALIZATION_ENUM(EKeyId::eKI_Tilde, "tilde", "Keyboard_Tilde")
SERIALIZATION_ENUM(EKeyId::eKI_LShift, "lshift", "Keyboard_LShift")
SERIALIZATION_ENUM(EKeyId::eKI_Backslash, "backslash", "Keyboard_Backslash")
SERIALIZATION_ENUM(EKeyId::eKI_Z, "z", "Keyboard_Z")
SERIALIZATION_ENUM(EKeyId::eKI_X, "x", "Keyboard_X")
SERIALIZATION_ENUM(EKeyId::eKI_C, "c", "Keyboard_C")
SERIALIZATION_ENUM(EKeyId::eKI_V, "v", "Keyboard_V")
SERIALIZATION_ENUM(EKeyId::eKI_B, "b", "Keyboard_B")
SERIALIZATION_ENUM(EKeyId::eKI_N, "n", "Keyboard_N")
SERIALIZATION_ENUM(EKeyId::eKI_M, "m", "Keyboard_M")
SERIALIZATION_ENUM(EKeyId::eKI_Comma, "comma", "Keyboard_Comma")
SERIALIZATION_ENUM(EKeyId::eKI_Period, "period", "Keyboard_Period")
SERIALIZATION_ENUM(EKeyId::eKI_Slash, "slash", "Keyboard_Slash")
SERIALIZATION_ENUM(EKeyId::eKI_RShift, "rshift", "Keyboard_RShift")
SERIALIZATION_ENUM(EKeyId::eKI_NP_Multiply, "np_multiply", "Keyboard_NP_Multiply")
SERIALIZATION_ENUM(EKeyId::eKI_LAlt, "lalt", "Keyboard_LAlt")
SERIALIZATION_ENUM(EKeyId::eKI_Space, "space", "Keyboard_Space")
SERIALIZATION_ENUM(EKeyId::eKI_CapsLock, "capslock", "Keyboard_CapsLock")
SERIALIZATION_ENUM(EKeyId::eKI_F1, "f1", "Keyboard_F1")
SERIALIZATION_ENUM(EKeyId::eKI_F2, "f2", "Keyboard_F2")
SERIALIZATION_ENUM(EKeyId::eKI_F3, "f3", "Keyboard_F3")
SERIALIZATION_ENUM(EKeyId::eKI_F4, "f4", "Keyboard_F4")
SERIALIZATION_ENUM(EKeyId::eKI_F5, "f5", "Keyboard_F5")
SERIALIZATION_ENUM(EKeyId::eKI_F6, "f6", "Keyboard_F6")
SERIALIZATION_ENUM(EKeyId::eKI_F7, "f7", "Keyboard_F7")
SERIALIZATION_ENUM(EKeyId::eKI_F8, "f8", "Keyboard_F8")
SERIALIZATION_ENUM(EKeyId::eKI_F9, "f9", "Keyboard_F9")
SERIALIZATION_ENUM(EKeyId::eKI_F10, "f10", "Keyboard_F10")
SERIALIZATION_ENUM(EKeyId::eKI_NumLock, "numlock", "Keyboard_NumLock")
SERIALIZATION_ENUM(EKeyId::eKI_ScrollLock, "scrollock", "Keyboard_ScrollLock")
SERIALIZATION_ENUM(EKeyId::eKI_NP_7, "np_7", "Keyboard_NP_7")
SERIALIZATION_ENUM(EKeyId::eKI_NP_8, "np_8", "Keyboard_NP_8")
SERIALIZATION_ENUM(EKeyId::eKI_NP_9, "np_9", "Keyboard_NP_9")
SERIALIZATION_ENUM(EKeyId::eKI_NP_Substract, "np_subtract", "Keyboard_NP_Substract")
SERIALIZATION_ENUM(EKeyId::eKI_NP_4, "np_4", "Keyboard_NP_4")
SERIALIZATION_ENUM(EKeyId::eKI_NP_5, "np_5", "Keyboard_NP_5")
SERIALIZATION_ENUM(EKeyId::eKI_NP_6, "np_6", "Keyboard_NP_6")
SERIALIZATION_ENUM(EKeyId::eKI_NP_Add, "np_add", "Keyboard_NP_Add")
SERIALIZATION_ENUM(EKeyId::eKI_NP_1, "np_1", "Keyboard_NP_1")
SERIALIZATION_ENUM(EKeyId::eKI_NP_2, "np_2", "Keyboard_NP_2")
SERIALIZATION_ENUM(EKeyId::eKI_NP_3, "np_3", "Keyboard_NP_3")
SERIALIZATION_ENUM(EKeyId::eKI_NP_0, "np_0", "Keyboard_NP_0")
SERIALIZATION_ENUM(EKeyId::eKI_F11, "f11", "Keyboard_F11")
SERIALIZATION_ENUM(EKeyId::eKI_F12, "f12", "Keyboard_F12")
SERIALIZATION_ENUM(EKeyId::eKI_F13, "f13", "Keyboard_F13")
SERIALIZATION_ENUM(EKeyId::eKI_F14, "f14", "Keyboard_F14")
SERIALIZATION_ENUM(EKeyId::eKI_F15, "f15", "Keyboard_F15")
SERIALIZATION_ENUM(EKeyId::eKI_Colon, "colon", "Keyboard_Colon")
SERIALIZATION_ENUM(EKeyId::eKI_Underline, "underline", "Keyboard_Underline")
SERIALIZATION_ENUM(EKeyId::eKI_NP_Enter, "np_enter", "Keyboard_NP_Enter")
SERIALIZATION_ENUM(EKeyId::eKI_RCtrl, "rctrl", "Keyboard_RCtrl")
SERIALIZATION_ENUM(EKeyId::eKI_NP_Period, "np_period", "Keyboard_NP_Period")
SERIALIZATION_ENUM(EKeyId::eKI_NP_Divide, "np_divide", "Keyboard_NP_Divide")
SERIALIZATION_ENUM(EKeyId::eKI_Print, "print", "Keyboard_Print")
SERIALIZATION_ENUM(EKeyId::eKI_RAlt, "ralt", "Keyboard_RAlt")
SERIALIZATION_ENUM(EKeyId::eKI_Pause, "pause", "Keyboard_Pause")
SERIALIZATION_ENUM(EKeyId::eKI_Home, "home", "Keyboard_Home")
SERIALIZATION_ENUM(EKeyId::eKI_Up, "up", "Keyboard_Up")
SERIALIZATION_ENUM(EKeyId::eKI_PgUp, "pgup", "Keyboard_PgUp")
SERIALIZATION_ENUM(EKeyId::eKI_Left, "left", "Keyboard_Left")
SERIALIZATION_ENUM(EKeyId::eKI_Right, "right", "Keyboard_Right")
SERIALIZATION_ENUM(EKeyId::eKI_End, "end", "Keyboard_End")
SERIALIZATION_ENUM(EKeyId::eKI_Down, "down", "Keyboard_Down")
SERIALIZATION_ENUM(EKeyId::eKI_PgDn, "pgdn", "Keyboard_PgDn")
SERIALIZATION_ENUM(EKeyId::eKI_Insert, "insert", "Keyboard_Insert")
SERIALIZATION_ENUM(EKeyId::eKI_Delete, "delete", "Keyboard_Delete")
SERIALIZATION_ENUM(EKeyId::eKI_LWin, "lwin", "Keyboard_LWin")
SERIALIZATION_ENUM(EKeyId::eKI_RWin, "rwin", "Keyboard_RWin")
SERIALIZATION_ENUM(EKeyId::eKI_Apps, "apps", "Keyboard_Apps")

SERIALIZATION_ENUM(EKeyId::eKI_Mouse1, "mouse1", "Mouse_Button_1")
SERIALIZATION_ENUM(EKeyId::eKI_Mouse2, "mouse2", "Mouse_Button_2")
SERIALIZATION_ENUM(EKeyId::eKI_Mouse3, "mouse3", "Mouse_Button_3")
SERIALIZATION_ENUM(EKeyId::eKI_Mouse4, "mouse4", "Mouse_Button_4")
SERIALIZATION_ENUM(EKeyId::eKI_Mouse5, "mouse5", "Mouse_Button_5")
SERIALIZATION_ENUM(EKeyId::eKI_Mouse6, "mouse6", "Mouse_Button_6")
SERIALIZATION_ENUM(EKeyId::eKI_Mouse7, "mouse7", "Mouse_Button_7")
SERIALIZATION_ENUM(EKeyId::eKI_Mouse8, "mouse8", "Mouse_Button_8")
SERIALIZATION_ENUM(EKeyId::eKI_MouseWheelUp, "mwheel_up", "Mouse_WheelUp")
SERIALIZATION_ENUM(EKeyId::eKI_MouseWheelDown, "mwheel_down", "Mouse_WheelDown")
SERIALIZATION_ENUM(EKeyId::eKI_MouseX, "maxis_x", "Mouse_X")
SERIALIZATION_ENUM(EKeyId::eKI_MouseY, "maxis_y", "Mouse_Y")
SERIALIZATION_ENUM(EKeyId::eKI_MouseZ, "maxis_z", "Mouse_Z")

SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadUp, "xi_dpad_up", "Pad_DPad_Up")
SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadDown, "xi_dpad_down", "Pad_DPad_Down")
SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadLeft, "xi_dpad_left", "Pad_DPad_Left")
SERIALIZATION_ENUM(EKeyId::eKI_XI_DPadRight, "xi_dpad_right", "Pad_DPad_Right")
SERIALIZATION_ENUM(EKeyId::eKI_XI_Start, "xi_start", "Pad_Start")
SERIALIZATION_ENUM(EKeyId::eKI_XI_Back, "xi_back", "Pad_Back")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbL, "xi_thumbl", "Pad_LeftThumbPress")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbR, "xi_thumbr", "Pad_RightThumbPress")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ShoulderL, "xi_shoulderl", "Pad_LeftShoulder")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ShoulderR, "xi_shoulder", "Pad_RightShoulder")
SERIALIZATION_ENUM(EKeyId::eKI_XI_A, "xi_a", "Pad_Button_A")
SERIALIZATION_ENUM(EKeyId::eKI_XI_B, "xi_b", "Pad_Button_B")
SERIALIZATION_ENUM(EKeyId::eKI_XI_X, "xi_x", "Pad_Button_X")
SERIALIZATION_ENUM(EKeyId::eKI_XI_Y, "xi_y", "Pad_Button_Y")
SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerL, "xi_triggerl", "Pad_LeftTrigger")
SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerR, "xi_triggerr", "Pad_RightTrigger")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLX, "xi_thumblx", "Pad_LeftThumb_X-Axis")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLY, "xi_thumbly", "Pad_LeftThumb_Y-Axis")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLUp, "Pad_ThumbLUp", "Pad_ThumbLUp")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLDown, "Pad_ThumbLDown", "Pad_ThumbLDown")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLLeft, "Pad_ThumbLLeft", "Pad_ThumbLLeft")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbLRight, "Pad_ThumbLRight", "Pad_ThumbLRight")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRX, "xi_thumbrx", "Pad_RightThumb_X-Axis")
SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRY, "xi_thumbry", "Pad_RightThumb_Y-Axis")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRUp, "Pad_ThumbRUp", "Pad_ThumbRUp")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRDown, "Pad_ThumbRDown", "Pad_ThumbRDown")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRLeft, "Pad_ThumbRLeft", "Pad_ThumbRLeft")
//SERIALIZATION_ENUM(EKeyId::eKI_XI_ThumbRRight, "Pad_ThumbRRight", "Pad_ThumbRRight")
SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerLBtn, "xi_triggerl_btn", "Pad_LeftTriggerBtn")
SERIALIZATION_ENUM(EKeyId::eKI_XI_TriggerRBtn, "xi_triggerr_btn", "Pad_RightTriggerBtn")

SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Options, "pad_start", "Orbis_Options")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_L3, "pad_l3", "Orbis_L3")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_R3, "pad_r3", "Orbis_R3")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Up, "pad_up", "Orbis_Up")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Right, "pad_right", "Orbis_Right")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Down, "pad_down", "Orbis_Down")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Left, "pad_left", "Orbis_Left")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_L2, "pad_l2", "Orbis_L2")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_R2, "pad_r2", "Orbis_R2")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_L1, "pad_l1", "Orbis_L1")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_R1, "pad_r1", "Orbis_R1")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Triangle, "pad_triangle", "Orbis_Triangle")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Circle, "pad_circle", "Orbis_Circle")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Cross, "pad_cross", "Orbis_Cross")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_Square, "pad_square", "Orbis_Square")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickLX, "pad_sticklx", "Orbis_StickLX")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickLY, "pad_stickly", "Orbis_StickLY")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickRX, "pad_stickrx", "Orbis_StickRX")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_StickRY, "pad_stickry", "Orbis_StickRY")
//SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotX, "Orbis_RotX", "Orbis_RotX")
//SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotY, "Orbis_RotY", "Orbis_RotY")
//SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotZ, "Orbis_RotZ", "Orbis_RotZ")
//SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotX_KeyL, "Orbis_RotX_KeyL", "Orbis_RotX_KeyL")
//SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotX_KeyR, "Orbis_RotX_KeyR", "Orbis_RotX_KeyR")
//SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotZ_KeyD, "Orbis_RotZ_KeyD", "Orbis_RotZ_KeyD")
//SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RotZ_KeyU, "Orbis_RotZ_KeyU", "Orbis_RotZ_KeyU")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_LeftTrigger, "pad_ltrigger", "Orbis_LeftTrigger")
SERIALIZATION_ENUM(EKeyId::eKI_Orbis_RightTrigger, "pad_rtrigger", "Orbis_RightTrigger")
SERIALIZATION_ENUM_END()

namespace Cry
{
	namespace DefaultComponents
	{
		struct SActionPressedSignal
		{
			SActionPressedSignal() {}
			SActionPressedSignal(Schematyc::CSharedString actionName, float value)
				: m_actionName(actionName)
				, m_value(value) {}

			Schematyc::CSharedString m_actionName;
			float m_value;
		};

		struct SActionReleasedSignal
		{
			SActionReleasedSignal() {}
			SActionReleasedSignal(Schematyc::CSharedString actionName, float value)
				: m_actionName(actionName)
				, m_value(value) {}

			Schematyc::CSharedString m_actionName;
			float m_value;
		};

		struct SActionChangedSignal
		{
			SActionChangedSignal() {}
			SActionChangedSignal(Schematyc::CSharedString actionName, float value)
				: m_actionName(actionName)
				, m_value(value) {}

			Schematyc::CSharedString m_actionName;
			float m_value;
		};

		void RegisterInputComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CInputComponent));
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
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CInputComponent::BindAction, "{0F22767B-6AA6-4EDF-92F0-7BCD10CAB00E}"_cry_guid, "BindAction");
					pFunction->SetDescription("Binds an input to an action registered with RegisterAction");
					pFunction->SetFlags({ Schematyc::EEnvFunctionFlags::Member, Schematyc::EEnvFunctionFlags::Construction });
					pFunction->BindInput(1, 'grou', "Group Name");
					pFunction->BindInput(2, 'name', "Name");
					pFunction->BindInput(3, 'inpd', "Input Device");
					pFunction->BindInput(4, 'keyn', "Input Id");
					pFunction->BindInput(5, 'pres', "Receive Press Events");
					pFunction->BindInput(6, 'rele', "Receive Release Events");
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
		}

		void CInputComponent::ReflectType(Schematyc::CTypeDesc<CInputComponent>& desc)
		{
			desc.SetGUID(CInputComponent::IID());
			desc.SetEditorCategory("Input");
			desc.SetLabel("Input");
			desc.SetDescription("Exposes support for inputs and action maps");
			//desc.SetIcon("icons:ObjectTypes/object.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::HideFromInspector });
		}

		static void ReflectType(Schematyc::CTypeDesc<SActionPressedSignal>& desc)
		{
			desc.SetGUID("{603771B5-6321-4113-82FB-B839F1BE7F64}"_cry_guid);
			desc.SetLabel("On Action Pressed");
			desc.AddMember(&SActionPressedSignal::m_actionName, 'actn', "Action", "Action Name", "Name of the triggered action");
			desc.AddMember(&SActionPressedSignal::m_value, 'valu', "value", "Value", "Value, if any, for example the joystick analog value");
		}

		static void ReflectType(Schematyc::CTypeDesc<SActionReleasedSignal>& desc)
		{
			desc.SetGUID("{CB22A659-40AD-4CF0-AD07-42DCCEA24F44}"_cry_guid);
			desc.SetLabel("On Action Released");
			desc.AddMember(&SActionReleasedSignal::m_actionName, 'actn', "Action", "Action Name", "Name of the triggered action");
			desc.AddMember(&SActionReleasedSignal::m_value, 'valu', "value", "Value", "Value, if any, for example the joystick analog value");
		}

		static void ReflectType(Schematyc::CTypeDesc<SActionChangedSignal>& desc)
		{
			desc.SetGUID("{ECDF0D8F-7E4B-4FA1-AEFA-51DAFD382CE9}"_cry_guid);
			desc.SetLabel("On Action Changed");
			desc.AddMember(&SActionChangedSignal::m_actionName, 'actn', "Action", "Action Name", "Name of the triggered action");
			desc.AddMember(&SActionChangedSignal::m_value, 'valu', "value", "Value", "Value, if any, for example the joystick analog value");
		}

		CInputComponent::CInputComponent()
		{
			class CImplementation final : public IImplementation
			{
				virtual const char* GetKeyId(EKeyId id) override
				{
					return Serialization::getEnumName<EKeyId>(id);
				}
			};

			m_pImplementation = stl::make_unique<CImplementation>();
		}

		void CInputComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			Initialize();
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
	}
}