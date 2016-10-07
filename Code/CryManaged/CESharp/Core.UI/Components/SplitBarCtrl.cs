// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System.Runtime.Serialization;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Input control for SplitBar.
	/// </summary>
	[DataContract]
	public class SplitBarCtrl : UIComponent
	{
		public event EventHandler<int, int> Pulling; ///< Raised if mouse is pulling the SplitBar. Coordinates define mouse delta since pull start.

		bool _drag = false;
		Point _startPos;
		[DataMember]

		public float TargetAlpha { get; private set; } ///< Defines the color alpha which the SplitBar should have.

		/// <summary>
		/// Initialized by SplitBar.
		/// </summary>
		public SplitBarCtrl()
		{
			TargetAlpha = 0.3f;
		}

		/// <summary>
		/// Called by Canvas. Do not call directly.
		/// </summary>
		public override void OnMouseMove(int x, int y)
		{
			if (_drag && Pulling != null)
			{
				Pulling((int)(x - _startPos.x), (int)(y - _startPos.y));
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

