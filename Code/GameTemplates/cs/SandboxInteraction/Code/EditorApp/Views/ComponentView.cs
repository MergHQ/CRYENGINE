// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;
using System.Linq;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Common;
using CryEngine.Resources;
using CryEngine;
using CryEngine.Components;
using CryEngine.Attributes;

namespace CryEngine.Editor.Views
{
	public class ComponentView : UIElement
	{
		class PropertyCtrl
		{
			Object _source;
			PropertyInfo _propertyInfo;
			UIElement _name;
			Text _nameText;

			public string Text { set { _nameText.Content = value; } }
			public UIElement Value;

			public PropertyCtrl(SceneObject parent, PropertyInfo pi, Object source)
			{
				_propertyInfo = pi;
				_source = source;

				_name = SceneObject.Instantiate<UIElement>(parent);
				_name.RectTransform.Alignment = Alignment.TopLeft;
				_name.RectTransform.ClampMode = ClampMode.Full;
				_nameText = _name.AddComponent<Text> ();
				_nameText.Height = 15;
				_nameText.Alignment = Alignment.TopLeft;

				var t = _propertyInfo.PropertyType;
				if (t == typeof(bool))
				{
					var value = SceneObject.Instantiate<CheckBox> (parent);
					Value = value;
					value.RectTransform.Alignment = Alignment.TopRight;
					value.Ctrl.IsChecked = (bool)_propertyInfo.GetValue (source, null);
					value.Ctrl.CheckedChanged += (c) => _propertyInfo.SetValue(_source, c, null);
				}
				else if (t.IsEnum)
				{
					var value = SceneObject.Instantiate<ComboBox> (parent);
					Value = value;
					value.Ctrl.Items = Enum.GetValues(t).OfType<object>().OrderBy(x => x.ToString()).ToList();
					value.Ctrl.SelectedItem = _propertyInfo.GetValue (source, null);
					value.Ctrl.SelectedItemChanged += (item) => _propertyInfo.SetValue (_source, Enum.Parse(t, item.ToString()), null);
				}
				else
				{
					var value = SceneObject.Instantiate<TextInput> (parent);
					Value = value;
					var val = _propertyInfo.GetValue (source, null);
					value.Text.Content = val != null ? val.ToString () : "NULL";
					value.Ctrl.OnSubmit += (s, e) => ProcessSubmit(e);
				}
				Value.RectTransform.ClampMode = ClampMode.Full;
			}

			void ProcessSubmit(TextCtrl.SubmitEventArgs e)
			{
				try
				{
					var t = _propertyInfo.PropertyType;
					if (t == typeof(string))
					{
						_propertyInfo.SetValue(_source, e.Value, null);
					}
					else if (t == typeof(int))
					{
						_propertyInfo.SetValue(_source, int.Parse (e.Value), null);
					}
					else if (t == typeof(byte))
					{
						_propertyInfo.SetValue(_source, byte.Parse (e.Value), null);
					}
					else if (t == typeof(float))
					{
						_propertyInfo.SetValue(_source, float.Parse (e.Value), null);
					}
					else if (t == typeof(bool))
					{
						_propertyInfo.SetValue(_source, bool.Parse (e.Value), null);
					}
					else if (t == typeof(Color))
					{
						var val = e.Value.Split(',');
						_propertyInfo.SetValue(_source, new Color(float.Parse(val[0]), float.Parse(val[1]), float.Parse(val[2])), null);
					}
					else if (t == typeof(Point))
					{
						var val = e.Value.Split(',');
						_propertyInfo.SetValue(_source, new Point(float.Parse(val[0]), float.Parse(val[1])), null);
					}
					else if (t == typeof(Rect))
					{
						var val = e.Value.Split(',');
						_propertyInfo.SetValue(_source, new Rect() { x=float.Parse(val[0]), y=float.Parse(val[1]), w=float.Parse(val[2]), h=float.Parse(val[3]) }, null);
					}
					else if (t == typeof(Padding))
					{
						var val = e.Value.Split(',');
						_propertyInfo.SetValue(_source, new Padding(float.Parse(val[0]), float.Parse(val[1]), float.Parse(val[2]), float.Parse(val[3])), null);
					}
					else if (t == typeof(ImageSource))
					{
						if (e.Value.Length == 0)
							_propertyInfo.SetValue(_source, null, null);
						else
						{
							var img = ResourceManager.ImageFromFile(Application.UIPath + "" + e.Value, false);
							if (img != null)
								_propertyInfo.SetValue(_source, img, null);
						}
					}
					else if (t.IsEnum)
					{
						_propertyInfo.SetValue(_source, Enum.Parse(t, e.Value), null);
					}
					else
						return; // Not Implemented
					
					e.Handled = true;
				}
				catch 
				{
				}
			}

			public void Show(bool show)
			{
				_name.Active = show;
				Value.Active = show;
			}

			public void Position(int w, int y)
			{
				if (Value is CheckBox)
					Value.RectTransform.Padding = new Padding (-15, y + 8);
				else if (Value is ComboBox)
					Value.RectTransform.Padding = new Padding (w*0.4f-2, y+7, 6, 0);
				else
					Value.RectTransform.Padding = new Padding (w*0.4f, y+8, 8, 0);
				Value.RectTransform.PerformLayout (true);

				_name.RectTransform.Padding = new Padding ((int)w * 0.2f, y+8);
				_name.RectTransform.Size = new Point(w * 0.4f-5, 17);
			}
		}

		Component _item;
		CheckBox _active;
		Panel _line;
		Text _caption;
		CollapseCtrl _collapseCtrl;
		List<PropertyCtrl> _props = new List<PropertyCtrl>();

		public InspectorView Inspector;

		public void OnAwake()
		{
			RectTransform.Alignment = Alignment.TopHStretch;

			_caption = AddComponent<Text> ();
			_caption.FontStyle = System.Drawing.FontStyle.Bold;
			_caption.Height = 15;
			_caption.Offset = new Point (16+19, 0);
			_caption.Alignment = Alignment.TopLeft;

			_active = SceneObject.Instantiate<CheckBox> (this);
			_active.RectTransform.Padding = new Padding (16+10,8);

			_collapseCtrl = AddComponent<CollapseCtrl>();
			_collapseCtrl.OnCollapseChanged += OnCollapseChanged;

			_line = SceneObject.Instantiate<Panel> (this);
			_line.RectTransform.Size = new Point (0,1);
			_line.RectTransform.Alignment = Alignment.BottomHStretch;
			_line.RectTransform.Padding = new Padding (4,0,4,1);
			_line.RectTransform.ClampMode = ClampMode.Parent;
			_line.Background.Color = Color.Gray;

			RectTransform.LayoutChanged += OnLayoutChanged;
		}

		void OnLayoutChanged()
		{
			int yOfs = 19;
			foreach(var pc in _props)
			{
				pc.Position((int)RectTransform.Width, yOfs);
				yOfs += 19;
			}
		}

		void OnCollapseChanged (bool collapsed)
		{
			RectTransform.Height = collapsed ? 21 : (19 + 19 * _props.Count + 3);
			_props.ForEach (x => x.Show(!collapsed));
			if (Inspector != null)
				Inspector.UpdatePositioning ();
		}

		public Component Context
		{
			set
			{
				_item = value;
				_caption.Content = SplitWords(_item.GetType().Name);

				int yOfs = 19;
				foreach (var pi in _item.GetType().GetProperties().Where(x => x.CanRead && x.CanWrite))
				{					
					if (pi.Name == "Active")
					{
						_active.Ctrl.IsChecked = (bool)pi.GetValue (_item, null);
						_active.Ctrl.CheckedChanged += (b) => pi.SetValue (_item, b, null);
						_active.Active = !(_item is RectTransform);
						continue;
					}

					var att = pi.GetCustomAttributes(typeof(HideFromInspectorAttribute), true);
					if (att.Length > 0)
						continue;

					var pc = new PropertyCtrl (this, pi, _item);
					pc.Text = SplitWords (pi.Name);
					yOfs += 19;
					_props.Add (pc);
				}
				RectTransform.Height = yOfs + 4;

				_collapseCtrl.Context = value;
			}
			get 
			{
				return _item;
			}
		}

		public void OnDestroy()
		{
			_collapseCtrl.OnCollapseChanged -= OnCollapseChanged;
			RectTransform.LayoutChanged -= OnLayoutChanged;
		}

		string SplitWords(string org)
		{
			var res = org;
			for (int i = org.Length - 1; i > 0; i--) 
			{
				if (Char.IsUpper(org[i]) && Char.IsLower(org[i-1]))
					res = res.Insert(i, " ");
			}
			return res;
		}
	}
}
