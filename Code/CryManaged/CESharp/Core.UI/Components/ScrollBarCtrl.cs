// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Runtime.Serialization;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Input control for ScrollBar.
	/// </summary>
	[DataContract]
	public class ScrollBarCtrl : UIComponent
	{
		bool _drag = false;
		Point _startPos;

		public float VisibleArea; ///< Normed visible area, relative to the Content panel's size.
		public bool IsVertical; ///< Determines whether or not ths ScrollBar is vertical.
		public float TargetAlpha { get; private set; } ///< Defines the alpha for this ScrollBar.
		public ScrollBar Parent; ///< Owning ScrollBar.

		/// <summary>
		/// Initialized by ScrollBar element.
		/// </summary>
		public ScrollBarCtrl()
		{
			TargetAlpha = 0.3f;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnMouseMove(int x, int y)
		{
			if (_drag && ((y - _startPos.y) != 0 || (x - _startPos.x) != 0))
			{
				var prt = Parent.RectTransform;
				var knobSize = IsVertical ? Math.Max(30, VisibleArea * prt.Height) : Math.Max(30, VisibleArea * prt.Width);
				Parent.MoveScrollAreaPos(IsVertical ? ((y - _startPos.y) / (prt.Height - knobSize)) : ((x - _startPos.x) / (prt.Width - knobSize)));
				_startPos = new Point(x, y);
			}
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnMouseEnter(int x, int y)
		{
			TargetAlpha = 1;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnMouseLeave(int x, int y)
		{
			if (!_drag)
				TargetAlpha = 0.3f;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnLeftMouseDown(int x, int y)
		{
			_drag = true;
			_startPos = new Point(x, y);
			TargetAlpha = 1;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnLeftMouseUp(int x, int y, bool inside)
		{
			_drag = false;
			TargetAlpha = 0.3f;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override bool HitTest(int x, int y)
		{
			var ort = (Owner as UIElement).RectTransform;
			return ort.Bounds.Contains(x, y);
		}
	}
}

