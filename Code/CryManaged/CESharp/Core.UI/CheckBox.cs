// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.UI.Components;
using System.Runtime.Serialization;

namespace CryEngine.UI
{
	/// <summary>
	/// Represents a simple Checkbox element.
	/// </summary>
	[DataContract]
	public class CheckBox : UIElement
	{
		public CheckBoxCtrl Ctrl; ///< The UIComponent controlling this element.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			RectTransform.Size = new Point(16, 16);
			RectTransform.ClampMode = ClampMode.Full;
			Ctrl = AddComponent<CheckBoxCtrl>();
			Ctrl.IsChecked = true;
		}
	}
}
