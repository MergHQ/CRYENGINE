// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.Resources;
using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Input controller for a ComboBox.
	/// </summary>
	public class ComboBoxCtrl : UIComponent
	{
		public event EventHandler OnFocusEnter; ///< Raised if the ComboBox focus was entered.
		public event EventHandler OnFocusLost; ///< Raised if the ComboBox focus was left.
		public event EventHandler<object> SelectedItemChanged; ///< Raised if the selected item changed.

		int _idx = -1;
		Text _text;
		object _selectedItem;
		UIElement _choiceFrame;
		Panel _choice;
		List<Text> _choiceLabels = new List<Text>();
		List<object> _items = new List<object>();
		SceneObject _choiceRoot;

		public List<object> Items
		{
			get
			{
				return _items;
			}
			set
			{
				_items = value;
				SelectedItem = null;
				ConstructChoiceList();
			}
		} ///< List of items which are shown by the ComboBox.

		public object SelectedItem
		{
			get
			{
				return _selectedItem;
			}
			set
			{
				_selectedItem = value == null ? null : Items.FirstOrDefault(x => x.Equals(value));
				_idx = _selectedItem == null ? -1 : Items.IndexOf(_selectedItem);
				_text.Content = _idx < 0 ? "" : _selectedItem.ToString();
				if (SelectedItemChanged != null)
					SelectedItemChanged(_selectedItem);
			}
		} ///< Currently selected item.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			_text = (Owner as ComboBox).BgPanel.AddComponent<Text>();
			_text.Offset = new Point(5, 1);

			_choiceFrame = SceneObject.Instantiate<UIElement>(Owner);
			_choiceFrame.RectTransform.Alignment = Alignment.TopHStretch;

			_choiceRoot = SceneObject.Instantiate(null, "ChoiceRoot");
			var canvas = SceneObject.Instantiate<Canvas>(_choiceRoot);
			var pc = (Owner as UIElement).FindParentCanvas();
			canvas.TargetTexture = pc.TargetTexture;
			canvas.TargetEntity = pc.TargetEntity;
			_choice = SceneObject.Instantiate<Panel>(canvas);
			_choice.RectTransform.Alignment = Alignment.TopLeft;
			_choice.Background.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "button.png"));
			_choice.Background.IgnoreClamping = true;
			_choice.Background.SliceType = CryEngine.Resources.SliceType.Nine;
			_choice.RectTransform.ClampMode = ClampMode.Self;
			_choice.RectTransform.Spacing = new Rect(0, 0, 6, 0);
			_choice.Background.Color = Color.Gray;
			_choice.Active = false;
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnDestroy()
		{
			_choiceRoot.Destroy();
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnEnterFocus()
		{
			if (OnFocusEnter != null)
				OnFocusEnter();
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnLeaveFocus()
		{
			if (OnFocusLost != null)
				OnFocusLost();
			_choice.Active = false;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnLeftMouseDown(int x, int y)
		{
			if (Items.Count > 0)
			{
				var bounds = _choiceFrame.RectTransform.Bounds;
				_choice.RectTransform.Padding = new Padding(bounds.x + bounds.w / 2, bounds.y + bounds.h / 2);
				_choice.RectTransform.Size = new Point(bounds.w, bounds.h);
				_choice.Active = !_choice.Active;

				if (!_choice.Active && OnFocusLost != null)
					OnFocusLost();
				if (_choice.Active && OnFocusLost != null)
					OnFocusEnter();
			}
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnKey(SInputEvent e)
		{
			if (Items.Count > 0)
			{
				if (e.KeyPressed(EKeyId.eKI_Down))
					_idx = Math.Min(Items.Count - 1, _idx + 1);
				if (e.KeyPressed(EKeyId.eKI_Up))
					_idx = Math.Max(0, _idx - 1);
			}
			else
				_idx = -1;

			if (e.keyName.key.Length == 1)
			{
				var key = e.keyName.key.ToUpper();
				var chosen = Items.FirstOrDefault(x => x.ToString().ToUpper().StartsWith(key));
				if (chosen != null)
					_idx = Items.IndexOf(chosen);
			}
			SelectedItem = _idx < 0 ? null : Items[_idx];
		}

		void ConstructChoiceList()
		{
			int yOfs = 5;
			foreach (var str in _items)
			{
				var l = _choice.AddComponent<Text>();
				l.Content = str.ToString();
				l.Alignment = Alignment.TopLeft;
				l.Offset = new Point(5, yOfs);
				l.Height = 12;
				yOfs += 13;
				_choiceLabels.Add(l);
			}

			var cfRt = _choiceFrame.RectTransform;
			cfRt.Padding = new Padding(1, 21 + yOfs / 2, 3, 0);
			cfRt.Size = new Point(0, yOfs + 6);
		}
	}
}
