// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;

/// <summary>
/// UI-Components are meant to be used for modifying, controling or viewing content relative to their owning UI elements.
/// </summary>
namespace CryEngine.UI.Components
{
	/// <summary>
	/// Controls input on the Button UIElement.
	/// </summary>
	public class ButtonCtrl : UIComponent
	{
		public event EventHandler OnEnterMouse;
		public event EventHandler OnLeaveMouse;
		public event EventHandler OnFocusEnter; ///< Raised if Focus was entered by Canvas.
		public event EventHandler OnFocusLost; ///< Raised if Focus was left by Canvas.
		public event EventHandler OnPressed; ///< Raised if button was pressed.

		public Text Text { get; private set; } ///< Content of the button.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			Text = (Owner as Button).AddComponent<Text>();
			Text.Alignment = Alignment.Center;
		}

		public override void OnMouseEnter(int x, int y)
		{
			if (OnEnterMouse != null)
				OnEnterMouse();
		}

		public override void OnMouseLeave(int x, int y)
		{
			if (OnLeaveMouse != null)
				OnLeaveMouse();
		}

		/// <summary>
		/// Invoked by parent Canvas.
		/// </summary>
		public override void OnEnterFocus()
		{
			if (OnFocusEnter != null)
				OnFocusEnter();
		}

		/// <summary>
		/// Invoked by parent Canvas.
		/// </summary>
		public override void OnLeaveFocus()
		{
			if (OnFocusLost != null)
				OnFocusLost();
		}

		/// <summary>
		/// Invoked by parent Canvas.
		/// </summary>
		public override bool HitTest(int x, int y)
		{
			return (Owner as UIElement).RectTransform.ClampRect.Contains(x, y);
		}

		/// <summary>
		/// Invoked by parent Canvas.
		/// </summary>
		public override void OnLeftMouseDown(int x, int y)
		{
			(Owner as Button).SetDown();
		}

		/// <summary>
		/// Invoked by parent Canvas.
		/// </summary>
		public override void OnLeftMouseUp(int x, int y, bool inside)
		{
			(Owner as Button).SetUp();
			if (inside && OnPressed != null)
				OnPressed();
		}

		public override void OnKey(SInputEvent e)
		{
			if (e.KeyPressed(EKeyId.eKI_Space) || e.KeyPressed(EKeyId.eKI_Enter) || e.KeyPressed(EKeyId.eKI_XI_A))
			{
				if (OnPressed != null)
					OnPressed();
			}
		}
	}
}

