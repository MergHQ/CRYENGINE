// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Resources;
using CryEngine.UI.Components;
using System.Runtime.Serialization;
using System.IO;

namespace CryEngine.UI
{
	/// <summary>
	/// Represents a window with Header and Caption.
	/// </summary>
	[DataContract]
	public class Window : Panel
	{
		Text _caption;

		public string Caption { set { _caption.Content = value; } } ///< The header text.

		public byte CaptionHeight { set { _caption.Height = value; } }

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			base.OnAwake();
			Background.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "dialog.png"));
			Background.SliceType = SliceType.Nine;
			Background.IgnoreClamping = true;
			RectTransform.Spacing = new Rect(13, 13, 14, 14);
			RectTransform.ClampMode = ClampMode.Self;

			_caption = AddComponent<Text>();
			_caption.Height = 20;
			_caption.Alignment = Alignment.Top;
			_caption.Offset = new Point(0, 11);
		}
	}
}
