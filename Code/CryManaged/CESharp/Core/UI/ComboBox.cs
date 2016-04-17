// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.Components;

namespace CryEngine.UI
{
	/// <summary>
	/// Represents a simple CheckBox element.
	/// </summary>
	public class ComboBox : UIElement
	{
		Button _button;
		Panel _frame;

		public Panel BgPanel; ///< Holds Background image for the drop down area.
		public ComboBoxCtrl Ctrl; ///< Contaons the element's controller.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public void OnAwake()
		{
			BgPanel = SceneObject.Instantiate<Panel> (this);
			BgPanel.Background.Source = ResourceManager.ImageFromFile (Application.UIPath + "" + "button.png", false);
			BgPanel.Background.SliceType = SliceType.Nine;
			BgPanel.Background.Color = Color.Gray.WithAlpha(0.75f);
			BgPanel.RectTransform.Alignment = Alignment.Stretch;
			BgPanel.RectTransform.Spacing = new Rect () { w = 20 };
			BgPanel.RectTransform.ClampMode = ClampMode.Full;
			Ctrl = AddComponent<ComboBoxCtrl> ();
			
			_button = SceneObject.Instantiate<Button> (this);
			_button.RectTransform.Alignment = Alignment.Right;
			_button.RectTransform.Size = new Point (25, 22);
			_button.RectTransform.Padding = new Padding (-13, 0);
			_button.RectTransform.Spacing = new Rect () { x = 6 };
			_button.RectTransform.ClampMode = ClampMode.Full;
			_button.Image = ResourceManager.ImageFromFile (Application.UIPath + "" + "arrow_dn.png");

			RectTransform.Alignment = Alignment.TopHStretch;
			RectTransform.Size = new Point (0, 22);
			RectTransform.ClampMode = ClampMode.Full;

			_frame = SceneObject.Instantiate<Panel>(this);
			_frame.Background.Source = ResourceManager.ImageFromFile(Application.UIPath + "frame.png", false);
			_frame.Background.SliceType = SliceType.Nine;
			_frame.Background.Color = Color.SkyBlue;
			_frame.RectTransform.Alignment = Alignment.Stretch;
			_frame.RectTransform.ClampMode = ClampMode.Full;
			_frame.Active = false;
			_frame.RectTransform.Padding = new Padding (2,2,3,2);

			_button.Ctrl.Enabled = false;
			Ctrl.OnFocusEnter += EnterFocus;
			Ctrl.OnFocusLost += LeaveFocus;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		void EnterFocus()
		{
			_frame.Active = true;
			BgPanel.Background.Source = ResourceManager.ImageFromFile (Application.UIPath + "" + "button_inv.png", false);
			_button.Ctrl.OnEnterFocus ();
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		void LeaveFocus()
		{
			_frame.Active = false;
			BgPanel.Background.Source = ResourceManager.ImageFromFile (Application.UIPath + "" + "button.png", false);
			_button.Ctrl.OnLeaveFocus ();
		}
	}
}
