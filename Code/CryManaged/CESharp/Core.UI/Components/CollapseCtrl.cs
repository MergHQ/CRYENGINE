// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Resources;
using System;
using System.IO;
using System.Collections.Generic;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Input view-control for any element that offers collapsing.
	/// </summary>
	public class CollapseCtrl : UIComponent
	{
		public event EventHandler<bool> OnCollapseChanged; ///< Raised if the collapsed state changed. Bool value indicates new state.

		static Dictionary<Object, bool> _collapsedStateByContext = new Dictionary<object, bool>();
		bool _collapsed = false;
		ImageSource _arrRt = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "arrow_rt.png"));
		ImageSource _arrDn = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "arrow_dn.png"));
		Object _context;

		public Panel Icon { get; private set; } ///< Icon to be used for this view.

		public bool Collapsed
		{
			get
			{
				return _collapsed;
			}

			set
			{
				if (_collapsed != value)
				{
					if (_context != null)
						_collapsedStateByContext[_context] = value;
					_collapsed = value;
					if (OnCollapseChanged != null)
						OnCollapseChanged(Collapsed);
				}
			}
		} ///< Indicates whether the element should be collapsed or not.

		public Object Context
		{
			set
			{
				_context = value;
				bool val;
				if (_collapsedStateByContext.TryGetValue(_context, out val))
					Collapsed = val;
				else
					val = _collapsedStateByContext[_context] = Collapsed;

				Icon.Background.Source = val ? _arrRt : _arrDn;
			}
		} ///< Assigns the logical owner of this control, for sync with global collapsed-state memory.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			Icon = SceneObject.Instantiate<Panel>(Owner);
			Icon.RectTransform.Padding = new Padding(10, 8);
			Icon.RectTransform.Size = new Point(16, 16);
			Icon.RectTransform.Angle = 0;
			Icon.RectTransform.ClampMode = ClampMode.Parent;
			Icon.Background.Source = _arrDn;
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnUpdate()
		{
			if (Mouse.LeftDown && ParentCanvas.CursorInside(Icon))
			{
				Icon.RectTransform.Angle = 0;
				Collapsed = !Collapsed;
			}

			if (Collapsed && Icon.Background.Source == _arrDn)
				Icon.RectTransform.Angle -= 30;
			if (Icon.RectTransform.Angle <= -90)
			{
				Icon.Background.Source = _arrRt;
				Icon.RectTransform.Angle = 0;
			}

			if (!Collapsed && Icon.Background.Source == _arrRt)
				Icon.RectTransform.Angle += 30;
			if (Icon.RectTransform.Angle >= 90)
			{
				Icon.Background.Source = _arrDn;
				Icon.RectTransform.Angle = 0;
			}
		}
	}
}
