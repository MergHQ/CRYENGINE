// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.Components;

/// <summary>
/// CE#Framework's UI System, comprized of various elements, components and controllers. Any UIElement must live inside a Canvas. 
/// A Canvas element functions as the root element for an UI entity. There is no restriction to the amount of UI entities.
/// The Canvas Element processes input and delegates it to the element in focus. Drawing of the UI to screen or render textures is handled by the Canvas.
/// </summary>
namespace CryEngine.UI
{
	/// <summary>
	/// The Button element represents a simple button.
	/// </summary>
	public class Button : Panel
	{
		private Panel _image;
		private Panel _frame;

		public ButtonCtrl Ctrl; ///< The controler of the button.
		public ImageSource Image 
		{ 
			set
			{
				if (_image == null) 
					_image = SceneObject.Instantiate<Panel> (this);
				_image.Background.Source = value;
				_image.RectTransform.Alignment = Alignment.Center;
				_image.RectTransform.Size = new Point (value.Width, value.Height);
				_image.RectTransform.Padding = new Padding (1,1);
				_image.RectTransform.ClampMode = ClampMode.Parent;
			}
		} ///< Optional image to be shown on the clickable area.

		private string _backgroundImageUrl = Application.UIPath + "button.png";
		public string BackgroundImageUrl
		{
			get { return _backgroundImageUrl; }
			set
			{
				_backgroundImageUrl = value;

				// Reflect change
				if (!_frame.Active)
					SetUp();
			}
		}

		private string _backgroundImageInvertedUrl = Application.UIPath + "button_inv.png";
		public string BackgroundImageInvertedUrl
		{
			get { return _backgroundImageInvertedUrl; }
			set
			{
				_backgroundImageInvertedUrl = value;

				// Reflect change
				if (_frame.Active)
					SetDown();
			}
		}

		/// <summary>
		/// Puts the button on Pressed State.
		/// </summary>
		public void SetDown()
		{
			Background.Source = ResourceManager.ImageFromFile(_backgroundImageInvertedUrl, false);
			if (_image != null)
				_image.RectTransform.Padding = new Padding (1, 2);
			Ctrl.Text.Offset = new Point (0, 1);
		}

		/// <summary>
		/// Puts the button in unpressed state.
		/// </summary>
		public void SetUp()
		{
			Background.Source = ResourceManager.ImageFromFile(_backgroundImageUrl, false);
			if (_image != null)
				_image.RectTransform.Padding = new Padding (1, 1);
			Ctrl.Text.Offset = new Point (0, 0);
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			base.OnAwake ();
			RectTransform.ClampMode = ClampMode.Full;
			Background.Source = ResourceManager.ImageFromFile(_backgroundImageUrl, false);
			Background.SliceType = SliceType.Nine;
			Ctrl = AddComponent<ButtonCtrl> ();

			_frame = SceneObject.Instantiate<Panel>(this);
			_frame.Background.Source = ResourceManager.ImageFromFile(Application.UIPath + "frame.png", false);
			_frame.Background.SliceType = SliceType.Nine;
			_frame.Background.Color = Color.SkyBlue;
			_frame.RectTransform.Alignment = Alignment.Stretch;
			_frame.RectTransform.ClampMode = ClampMode.Full;
			_frame.Active = false;
			_frame.RectTransform.Padding = new Padding(2);

			Ctrl.OnFocusEnter += () => 
			{
				_frame.Active = true;
				SetDown();
			};
			Ctrl.OnFocusLost += () => 
			{
				_frame.Active = false;
				SetUp();
			};
		}
	}
}
