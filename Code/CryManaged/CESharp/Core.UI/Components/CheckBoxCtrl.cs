// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.Resources;
using System.Runtime.Serialization;

using System.IO;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Controls input on a Checkbox element.
	/// </summary>
	[DataContract]
	public class CheckBoxCtrl : UIComponent
	{
		public event EventHandler<bool> CheckedChanged; ///< Invoked if the checked state of checkbox changed.

		[DataMember]
		bool _isChecked;
		Panel _frame;

		Image _unchecked { get; set; }
		Image _checked { get; set; }

		public bool IsChecked
		{
			set
			{
				if (_isChecked != value)
				{
					_isChecked = value;
					_unchecked.Active = !value;
					_checked.Active = value;
					if (CheckedChanged != null)
						CheckedChanged(value);
				}
			}
			get
			{
				return _isChecked;
			}
		} ///< Determines whether checkbox is currently checked.

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnEnterFocus()
		{
			_frame.Active = true;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnLeaveFocus()
		{
			_frame.Active = false;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnLeftMouseUp(int x, int y, bool inside)
		{
			if (inside)
				IsChecked = !IsChecked;
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			_unchecked = Owner.AddComponent<Image>();
			_unchecked.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "cb_unchecked.png"));
			_checked = Owner.AddComponent<Image>();
			_checked.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "cb_checked.png"));

			_frame = SceneObject.Instantiate<Panel>(Owner);
			_frame.Background.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "frame.png"), false);
			_frame.Background.SliceType = SliceType.Nine;
			_frame.Background.Color = Color.SkyBlue;
			_frame.RectTransform.Alignment = Alignment.Stretch;
			_frame.RectTransform.ClampMode = ClampMode.Full;
			_frame.Active = false;

			IsChecked = true;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnKey(SInputEvent e)
		{
			if (e.KeyPressed(EKeyId.eKI_Space))
				IsChecked = !IsChecked;
		}
	}
}

