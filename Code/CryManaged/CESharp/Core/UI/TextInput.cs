// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;

namespace CryEngine.UI
{
	/// <summary>
	/// Allows for text input by keyboard.
	/// </summary>
	public class TextInput : Panel
	{
		public Text Text { get; private set; } ///< The current content.
		public TextCtrl Ctrl { get; private set; } ///< The input controller for this element.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			base.OnAwake ();
			Background.Color = Color.Black.WithAlpha(0.75f);
			RectTransform.Size = new Point (0,17);
			RectTransform.Alignment = Alignment.TopHStretch;
			RectTransform.Spacing = new Rect (0, 0, 1, 1);
			Text = AddComponent<Text> ();
			Text.Height = 15;
			Text.Offset = new Point (2, 1);
			Ctrl = AddComponent<TextCtrl> ();
		}
	}
}
