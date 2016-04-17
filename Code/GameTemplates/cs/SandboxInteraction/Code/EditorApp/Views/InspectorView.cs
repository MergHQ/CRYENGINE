// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Attributes;

namespace CryEngine.Editor.Views
{
	[HideFromInspector, DataContract]
	public class InspectorView : Window
	{
		SceneObject _context;
		CheckBox _active;
		Text _name;
		ScrollPanel _scrollPlanel;
		Panel _line;

		public SceneObject Context
		{
			set 
			{
				if (_context != null)
					_context.ActiveChanged -= CheckContextActivity;
				
				_scrollPlanel.Clear();
				_active.Active = _name.Active = _line.Active = value != null;

				_context = value;
				if (value == null)
					return;
				
				_name.Content = value.Name;
				_active.Ctrl.IsChecked = value.Active;
				value.ActiveChanged += CheckContextActivity;

				float yOfs = 3;
				foreach (var c in value.Components) 
				{
					if (c is Transform)
						continue;
					var cv = SceneObject.Instantiate<ComponentView> (_scrollPlanel.Panel);
					cv.Context = c;
					cv.Inspector = this;
					cv.RectTransform.Padding = new Padding (0, yOfs + cv.RectTransform.Height/2, 0, 0);
					cv.RectTransform.PerformLayout (true);
					cv.RectTransform.ClampMode = ClampMode.Full;
					yOfs += cv.RectTransform.Height + 5;
				}

				_scrollPlanel.SetPanelSize (RectTransform.Width-13-14-8, yOfs);
			}
		}

		public void UpdatePositioning()
		{
			float yOfs = 3;
			foreach (var t in _scrollPlanel.Panel.Transform) 
			{
				var cv = (ComponentView)t.Owner;
				cv.RectTransform.Padding = new Padding (0, yOfs + cv.RectTransform.Height/2, 0, 0);
				cv.RectTransform.PerformLayout (true);
				yOfs += cv.RectTransform.Height + 6;
			}
			_scrollPlanel.SetPanelSize (RectTransform.Width-13-14-8, yOfs);
		}

		void CheckContextActivity(SceneObject so)
		{
			_active.Ctrl.IsChecked = so.Active;
		}

		void ChangeContextActivity(bool active)
		{
			if (_context != null)
				_context.Active = active;
		}

		public override void OnAwake()
		{
			base.OnAwake();

			Caption = "Inspector";

			_name = AddComponent<Text> ();
			_name.Height = 15;
			_name.Offset = new Point (13+19, 36);
			_name.Alignment = Alignment.TopLeft;

			_active = SceneObject.Instantiate<CheckBox> (this);
			_active.RectTransform.Padding = new Padding (23,44);
			_active.Ctrl.CheckedChanged += (b) => ChangeContextActivity (b);
			_active.Active = false;

			_line = SceneObject.Instantiate<Panel> (this);
			_line.RectTransform.Size = new Point (0,1);
			_line.RectTransform.Alignment = Alignment.TopHStretch;
			_line.RectTransform.Padding = new Padding (17,56,26,0);
			_line.Background.Color = Color.Gray;
			_line.Active = false;

			_scrollPlanel = SceneObject.Instantiate<ScrollPanel>(this);
			_scrollPlanel.RectTransform.Padding = new Padding (13,13+23+22,14,14);

			RectTransform.LayoutChanged+= () => 
			{
				_scrollPlanel.SetPanelSize (RectTransform.Width-13-14-8, _scrollPlanel.Panel.RectTransform.Height);
			};
		}
	}
}
