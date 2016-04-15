// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Drawing;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.Components;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Enhances Component by some UI specific functionality
	/// </summary>
	public class UIComponent : Component
	{
		Canvas _parentCanvas;

		public Canvas ParentCanvas
		{
			get 
			{
				if (_parentCanvas == null)
					_parentCanvas = (Owner as UIElement).FindParentCanvas();
				return _parentCanvas;
			}
		} ///< Returns Canvas owning this Conponent in hierarchy.

		/// <summary>
		/// Returns Layouted Bounds for this Component.
		/// </summary>
		public virtual Rect GetAlignedRect()
		{
			return (Owner as UIElement).RectTransform.Bounds;
		}

		/// <summary>
		/// Tries to hit Bounds or ClampRect if existing.
		/// </summary>
		/// <returns><c>True</c> if any of the rects was hit.</returns>
		/// <param name="x">The x coordinate.</param>
		/// <param name="y">The y coordinate.</param>
		public override bool HitTest(int x, int y)
		{
			var prt = (Owner as UIElement).RectTransform;
			return prt.ClampRect == null ? prt.Bounds.Contains(x, y) : prt.ClampRect.Contains (x, y);
		}
	}
}
